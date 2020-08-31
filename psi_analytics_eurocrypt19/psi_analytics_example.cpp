//
// \file psi_analytics_example.cpp
// \author Oleksandr Tkachenko
// \email tkachenko@encrypto.cs.tu-darmstadt.de
// \organization Cryptography and Privacy Engineering Group (ENCRYPTO)
// \TU Darmstadt, Computer Science department
//
// \copyright The MIT License. Copyright Oleksandr Tkachenko
//

#include <cassert>
#include <iostream>

#include <boost/program_options.hpp>

#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>
#include "abycore/aby/abyparty.h"

#include "common/psi_analytics.h"
#include "common/psi_analytics_context.h"

auto read_test_options(int32_t argcp, char **argvp) {
  namespace po = boost::program_options;
  ENCRYPTO::PsiAnalyticsContext context;
  po::options_description allowed("Allowed options");
  std::string type;
  // clang-format off
  allowed.add_options()("help,h", "produce this message")
  ("role,r",         po::value<decltype(context.role)>(&context.role)->required(),                                  "Role of the node")
  ("neles,n",        po::value<decltype(context.neles)>(&context.neles)->default_value(1000u),                      "Number of my elements")
  ("bit-length,b",   po::value<decltype(context.bitlen)>(&context.bitlen)->default_value(61u),                      "Bit-length of the elements")
  ("epsilon,e",      po::value<decltype(context.epsilon)>(&context.epsilon)->default_value(2.4f),                   "Epsilon, a table size multiplier")
  ("threads,t",      po::value<decltype(context.nthreads)>(&context.nthreads)->default_value(1),                    "Number of threads")
  ("threshold,c",    po::value<decltype(context.threshold)>(&context.threshold)->default_value(0u),                 "Show PSI size if it is > threshold")
  ("nmegabins,m",    po::value<decltype(context.nmegabins)>(&context.nmegabins)->default_value(1u),                 "Number of mega bins")
  ("polysize,s",     po::value<decltype(context.polynomialsize)>(&context.polynomialsize)->default_value(0u),       "Size of the polynomial(s), default: neles")
  ("functions,f",    po::value<decltype(context.nfuns)>(&context.nfuns)->default_value(2u),                         "Number of hash functions in hash tables")
  ("num_parties,np",    po::value<decltype(context.np)>(&context.np)->default_value(100u),                         "Number of parties")
  ("file_address,np",    po::value<decltype(context.file_address)>(&context.file_address)->default_value("../../files/addresses"),                         "IP Addresses")
  ("type,y",         po::value<std::string>(&type)->default_value("None"),                                          "Function type {None, Threshold, Sum, SumIfGtThreshold}");
  // clang-format on

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argcp, argvp, allowed), vm);
    po::notify(vm);
  } catch (const boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<
               boost::program_options::required_option> > &e) {
    if (!vm.count("help")) {
      std::cout << e.what() << std::endl;
      std::cout << allowed << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if (vm.count("help")) {
    std::cout << allowed << "\n";
    exit(EXIT_SUCCESS);
  }

  if (type.compare("None") == 0) {
    context.analytics_type = ENCRYPTO::PsiAnalyticsContext::NONE;
  } else if (type.compare("Threshold") == 0) {
    context.analytics_type = ENCRYPTO::PsiAnalyticsContext::THRESHOLD;
  } else if (type.compare("Sum") == 0) {
    context.analytics_type = ENCRYPTO::PsiAnalyticsContext::SUM;
  } else if (type.compare("SumIfGtThreshold") == 0) {
    context.analytics_type = ENCRYPTO::PsiAnalyticsContext::SUM_IF_GT_THRESHOLD;
  } else {
    std::string error_msg(std::string("Unknown function type: " + type));
    throw std::runtime_error(error_msg.c_str());
  }

  if (context.polynomialsize == 0) {
    context.polynomialsize = context.neles * context.nfuns;
  }
  context.polynomialbytelength = context.polynomialsize * sizeof(std::uint64_t);

  context.nbins = context.neles * context.epsilon;

  //Setting network parameters
  if(context.role == P0) {
    context.address.reserve(context.np);
    context.port.reserve(context.np);
    //store addresses of other parties
    std::ifstream in(filename, std::ifstream::in);
    if(!exists(filename)) {
      std::cerr << "Address file doesn't exist" << std::endl;
      exit(-1);
    }

    for(int i=0; i< context.np; i++) {
      in >> context.address[i];
      context.port[i] = REF_PORT;
    }
    in.close();
  } else {
    context.address.reserve(1);
    context.port.reserve(1);
    context.address[0] = DEF_ADDRESS;
    context.port[0] = REF_PORT;
  }

  return context;
}

int main(int argc, char **argv) {
  auto context = read_test_options(argc, argv);
  auto gen_bitlen = static_cast<std::size_t>(std::ceil(std::log2(context.neles))) + 3;
  auto inputs = ENCRYPTO::GeneratePseudoRandomElements(context.neles, gen_bitlen);
  ENCRYPTO::run_psi_analytics(inputs, context);
  std::cout << "PSI circuit successfully executed" << std::endl;
  PrintTimings(context);
  return EXIT_SUCCESS;
}
