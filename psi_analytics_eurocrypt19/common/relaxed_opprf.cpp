//
// \author Oleksandr Tkachenko
// \email tkachenko@encrypto.cs.tu-darmstadt.de
// \organization Cryptography and Privacy Engineering Group (ENCRYPTO)
// \TU Darmstadt, Computer Science department
//
// \copyright The MIT License. Copyright Oleksandr Tkachenko
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "relaxed_opprf.h"
#include "psi_analytics.h"
//#include "constants.h"
#include "connection.h"
#include "socket.h"
//#include "abycore/sharing/boolsharing.h"
//#include "abycore/sharing/sharing.h"

//#include "ots/ots.h"
#include "polynomials/Poly.h"

#include "HashingTables/cuckoo_hashing/cuckoo_hashing.h"
#include "HashingTables/simple_hashing/simple_hashing.h"
#include "psi_analytics_context.h"
#include "table_opprf.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <ratio>
#include <unordered_set>
#include <thread>

namespace RELAXEDNS {
  struct hashlocmap {
    int bin;
    int index;
  };

//using share_ptr = std::shared_ptr<share>;

using milliseconds_ratio = std::ratio<1, 1000>;
using duration_millis = std::chrono::duration<double, milliseconds_ratio>;


void multi_oprf_thread(int tid, std::vector<std::vector<osuCrypto::block>> &masks_with_dummies, std::vector<uint64_t> table,
			ENCRYPTO::PsiAnalyticsContext &context, std::vector<osuCrypto::Channel> &chl) {
  for(int i=tid; i<context.np-1; i=i+context.nthreads) {
    masks_with_dummies[i] = RELAXEDNS::ot_receiver(table, chl[i], context);
  }
}

void OpprgPsiNonLeader(std::vector<uint64_t> &actual_contents_of_bins, std::vector<std::vector<uint64_t>> &simple_table_v, std::vector<std::vector<osuCrypto::block>> &masks, ENCRYPTO::PsiAnalyticsContext & context, std::unique_ptr<CSocket> &sock, osuCrypto::Channel &chl) {
  //const auto filter_start_time = std::chrono::system_clock::now();
  const auto filter_start_time = std::chrono::system_clock::now();
  std::vector<uint64_t> content_of_bins;
  uint64_t bufferlength = (uint64_t)ceil(context.nbins/2.0);
  osuCrypto::PRNG prng(osuCrypto::sysRandomSeed(), bufferlength);

  for( int i=0; i<context.nbins; i++) {
    content_of_bins.push_back(prng.get<uint64_t>());
  }

  /*std::cout<<"***********************************"<<std::endl;
  std::cout<<"The Bin Random Values are: ["<<std::endl;
  for(int i=0;i<context.nbins;i++) {
    std::cout<<"( "<<i<<", "<<content_of_bins[i]<<"), ";
  }
  std::cout<<"]"<<std::endl;
  std::cout<<"***********************************"<<std::endl;*/

  std::unordered_map<uint64_t,hashlocmap> tloc;
  std::vector<uint64_t> filterinputs;
  for(int i=0; i<context.nbins; i++) {
    int binsize = simple_table_v[i].size();
    for(int j=0; j<binsize; j++) {
      tloc[simple_table_v[i][j]].bin = i;
      tloc[simple_table_v[i][j]].index = j;
      filterinputs.push_back(simple_table_v[i][j]);
    }
  }

  ENCRYPTO::CuckooTable cuckoo_table(static_cast<std::size_t>(context.fbins));
  cuckoo_table.SetNumOfHashFunctions(context.ffuns);
  cuckoo_table.Insert(filterinputs);
  cuckoo_table.MapElements();
  //cuckoo_table.Print();

  if (cuckoo_table.GetStashSize() > 0u) {
    std::cerr << "[Error] Stash of size " << cuckoo_table.GetStashSize() << " occured\n";
  }

  std::vector<uint64_t> garbled_cuckoo_filter;
  garbled_cuckoo_filter.reserve(context.fbins);

  bufferlength = (uint64_t)ceil(context.fbins - 3*context.nbins);
  osuCrypto::PRNG prngo(osuCrypto::sysRandomSeed(), bufferlength);

  for(int i=0; i<context.fbins; i++){
    if(!cuckoo_table.hash_table_.at(i).IsEmpty()) {
      uint64_t element = cuckoo_table.hash_table_.at(i).GetElement();
      uint64_t function_id = cuckoo_table.hash_table_.at(i).GetCurrentFunctinId();
      hashlocmap hlm = tloc[element];
      osuCrypto::PRNG prng(masks[hlm.bin][hlm.index], 2);
      uint64_t pad = 0u;
      for(int j=0;j<=function_id;j++) {
         pad = prng.get<uint64_t>();
      }
      garbled_cuckoo_filter[i] = content_of_bins[hlm.bin] ^ pad;
    } else {
      garbled_cuckoo_filter[i] = prngo.get<uint64_t>();
    }
  }

  sock->Send(garbled_cuckoo_filter.data(), context.fbins * sizeof(uint64_t));
  //context.timings.polynomials_transmission = polynomial_trans.count();

    const int ts=4;
    auto masks_with_dummies = RELAXEDNS::ot_receiver(content_of_bins, chl, context);

    std::vector<osuCrypto::block> padding_vals;
padding_vals.reserve(context.nbins);
std::vector<uint64_t> table_opprf;
table_opprf.reserve(ts*context.nbins);
//Receive nonces
sock->Receive(padding_vals.data(), context.nbins * sizeof(osuCrypto::block));
//Receive table
sock->Receive(table_opprf.data(), context.nbins * ts* sizeof(uint64_t));

//context.timings.table_transmission = ttrans_duration.count();

uint64_t addresses1;
uint8_t bitaddress;
uint64_t mask_ad = (1ULL << 2) - 1;

actual_contents_of_bins.reserve(context.nbins);

for(int i=0; i<context.nbins; i++) {
      addresses1 = hashToPosition(reinterpret_cast<uint64_t *>(&masks_with_dummies[i])[0], padding_vals[i]);
      bitaddress = addresses1 & mask_ad;
      actual_contents_of_bins[i] = reinterpret_cast<uint64_t *>(&masks_with_dummies[i])[0] ^ table_opprf[ts*i+bitaddress];
}
}


  void OpprgPsiLeader(std::vector<uint64_t> &content_of_bins, std::vector<uint64_t> &cuckoo_table_v, std::vector<osuCrypto::block> &masks_with_dummies, ENCRYPTO::PsiAnalyticsContext &context, std::unique_ptr<CSocket> &sock, osuCrypto::Channel &chl) {
      std::vector<uint64_t> garbled_cuckoo_filter;
      garbled_cuckoo_filter.reserve(context.fbins);

      sock->Receive(garbled_cuckoo_filter.data(), context.fbins * sizeof(uint64_t));

      ENCRYPTO::CuckooTable garbled_cuckoo_table(static_cast<std::size_t>(context.fbins));
      garbled_cuckoo_table.SetNumOfHashFunctions(context.ffuns);
      garbled_cuckoo_table.Insert(cuckoo_table_v);
      auto addresses = garbled_cuckoo_table.GetElementAddresses();

      std::vector<std::vector<uint64_t>> opprf_values(context.nbins, std::vector<uint64_t>(context.ffuns));

      for(int i=0; i<context.nbins; i++) {
        osuCrypto::PRNG prngo(masks_with_dummies[i], 2);
        for(int j=0; j< context.ffuns; j++) {
          opprf_values[i][j]=garbled_cuckoo_filter[addresses[i*context.ffuns+j]] ^ prngo.get<uint64_t>();
        }
      }

    const int ts=4;
    auto table_masks = RELAXEDNS::ot_sender(opprf_values, chl, context);

    uint64_t bufferlength = (uint64_t)ceil(context.nbins/2.0);
osuCrypto::PRNG tab_prng(osuCrypto::sysRandomSeed(), bufferlength);

content_of_bins.reserve(context.nbins);
for( int i=0; i<context.nbins; i++) {
  content_of_bins[i] = tab_prng.get<uint64_t>();
}

/*std::cout<<"***********************************"<<std::endl;
std::cout<<"The actual contents are: ["<<std::endl;
for(int i=0;i<context.nbins;i++) {
  std::cout<<"( "<<i<<", "<<content_of_bins[i]<<"), ";
}
std::cout<<"]"<<std::endl;
std::cout<<"***********************************"<<std::endl;*/

std::vector<osuCrypto::block> padding_vals;
padding_vals.reserve(context.nbins);
std::vector<uint64_t> table_opprf;
table_opprf.reserve(ts*context.nbins);
osuCrypto::PRNG padding_prng(osuCrypto::sysRandomSeed(), 2*context.nbins);

bufferlength = (uint64_t)ceil(context.nbins/2.0);
osuCrypto::PRNG dummy_prng(osuCrypto::sysRandomSeed(), bufferlength);

//Get addresses
uint64_t addresses1[context.ffuns];
uint8_t bitaddress[context.ffuns];
uint8_t bitindex[ts];
uint64_t mask_ad = (1ULL << 2) - 1;

double ave_ctr=0.0;

for(int i=0; i<context.nbins; i++) {
  bool uniqueMap = false;
  int ctr=0;
  while (!uniqueMap) {
    auto nonce = padding_prng.get<osuCrypto::block>();

    for(int j=0; j< context.ffuns; j++) {
      addresses1[j] = hashToPosition(reinterpret_cast<uint64_t *>(&table_masks[i][j])[0], nonce);
      bitaddress[j] = addresses1[j] & mask_ad;
    }

    uniqueMap = true;
    for(int j=0; j<ts; j++)
      bitindex[j]=ts;

    for(uint8_t j=0; j< context.ffuns; j++) {
      if(bitindex[bitaddress[j]] != ts) {
        uniqueMap = false;
        break;
      } else {
        bitindex[bitaddress[j]] = j;
      }
    }

    if(uniqueMap) {
      padding_vals.push_back(nonce);
      for(int j=0; j<ts; j++)
        if(bitindex[j]!=-1) {
          table_opprf[i*ts+j] = reinterpret_cast<uint64_t *>(&table_masks[i][bitindex[j]])[0] ^ content_of_bins[i];
        } else {
          table_opprf[i*ts+j] = dummy_prng.get<uint64_t>();
        }
      ave_ctr += ctr;
    }
    ctr++;
  }
//table_opprf[i*4+]
}

ave_ctr = ave_ctr/context.nbins;
std::cout<<"Average counts: "<<ave_ctr<<std::endl;
//const duration_millis table_duration = table_end_time - table_start_time;
//context.timings.table_compute = table_duration.count();

//Send nonces
sock->Send(padding_vals.data(), context.nbins * sizeof(osuCrypto::block));
//Send table
sock->Send(table_opprf.data(), context.nbins * ts* sizeof(uint64_t));

  }

  void multi_hint_thread(int tid, std::vector<std::vector<uint64_t>> &sub_bins, std::vector<uint64_t> &cuckoo_table_v, std::vector<std::vector<osuCrypto::block>> &masks_with_dummies, ENCRYPTO::PsiAnalyticsContext &context, std::vector<std::unique_ptr<CSocket>> &allsocks, std::vector<osuCrypto::Channel> &chls) {
    for(int i=tid; i<context.np-1; i=i+context.nthreads) {
      OpprgPsiLeader(sub_bins[i], cuckoo_table_v, masks_with_dummies[i], context, allsocks[i], chls[i]);
    }
  }

std::vector<uint64_t> run_relaxed_opprf(ENCRYPTO::PsiAnalyticsContext &context, const std::vector<std::uint64_t> &inputs,
					std::vector<std::unique_ptr<CSocket>> &allsocks, std::vector<osuCrypto::Channel> &chls) {

  std::vector<uint64_t> bins;

  // create hash tables from the elements
  if (context.role == P_0) {

    bins.reserve(context.nbins);
    for(uint64_t i=0; i<context.nbins; i++) {
    	bins[i] = 0;
    }

    std::vector<std::vector<uint64_t>> sub_bins(context.np-1);
    std::vector<uint64_t> table;
    std::vector<std::vector<osuCrypto::block>> masks_with_dummies(context.np-1);
    table = ENCRYPTO::cuckoo_hash(context, inputs);

    const auto oprf_start_time = std::chrono::system_clock::now();
    std::thread oprf_threads[context.nthreads];
    for(int i=0; i<context.nthreads; i++) {
      oprf_threads[i] = std::thread(multi_oprf_thread, i, std::ref(masks_with_dummies), table, std::ref(context), std::ref(chls));
    }

    for(int i=0; i<context.nthreads; i++) {
      oprf_threads[i].join();
    }
    const auto oprf_end_time = std::chrono::system_clock::now();
    const duration_millis oprf_duration = oprf_end_time - oprf_start_time;
    context.timings.oprf = oprf_duration.count();

    const auto phase_ts_time = std::chrono::system_clock::now();
    std::thread hint_threads[context.nthreads];
    for(int i=0; i<context.nthreads; i++) {
      hint_threads[i] = std::thread(multi_hint_thread, i, std::ref(sub_bins), std::ref(table), std::ref(masks_with_dummies), std::ref(context), std::ref(allsocks), std::ref(chls));
    }

    for(int i=0; i<context.nthreads; i++) {
      hint_threads[i].join();
    }
    const auto phase_te_time = std::chrono::system_clock::now();
    const duration_millis phase_two_duration = phase_te_time - phase_ts_time;

    context.timings.polynomials = phase_two_duration.count();

    const auto agg_start_time = std::chrono::system_clock::now();

    TemplateField<ZpMersenneLongElement1> *field;
    std::vector<ZpMersenneLongElement1> field_bins;
    for(uint64_t j=0; j< context.nbins; j++) {
      field_bins.push_back(field->GetElement(sub_bins[0][j]));
    }

    for(uint64_t i=1; i< context.np-1; i++) {
      for(uint64_t j=0; j< context.nbins; j++) {
          field_bins[j] = field_bins[j]+field->GetElement(sub_bins[i][j]);
      }
    }

    for(uint64_t i=0; i< context.np-1; i++) {
      for(uint64_t j=0; j< context.nbins; j++) {
        bins[j] = field_bins[j].elem;
      }
    }

    const auto agg_end_time = std::chrono::system_clock::now();
    const duration_millis agg_duration = agg_end_time - agg_start_time;
    context.timings.aggregation += agg_duration.count();

  } else {
    bins.reserve(context.nbins);

    auto simple_table_v = ENCRYPTO::simple_hash(context, inputs);

    auto masks = RELAXEDNS::ot_sender(simple_table_v, chls[0], context);
    std::vector<uint64_t> actual_contents_of_bins;
    OpprgPsiNonLeader(actual_contents_of_bins, simple_table_v, masks, context, allsocks[0], chls[0]);

    TemplateField<ZpMersenneLongElement1> *field;
    std::vector<ZpMersenneLongElement1> field_bins;
    for(uint64_t j=0; j< context.nbins; j++) {
      field_bins.push_back(field->GetElement(actual_contents_of_bins[j]));
    }

    for(uint64_t j=0; j< context.nbins; j++) {
      bins[j] = field_bins[j].elem;
    }
/*    std::vector<uint64_t> polynomials = ClientEvaluateHint(context, masks);
    bins = ClientSendHint(context, allsocks[0], polynomials);*/
   }

  return bins;
}

}
