#pragma once
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

#include "abycore/aby/abyparty.h"
#include<string>
#include<memory>
#include "socket.h"
#include "helpers.h"
#include "psi_analytics_context.h"

#define ceil_divide(x, y)			(( ((x) + (y)-1)/(y)))

#include <vector>

namespace ENCRYPTO {

std::vector<uint64_t> run_psi_analytics(const std::vector<std::uint64_t> &inputs, PsiAnalyticsContext &context);

auto cuckoo_hash(const std::vector<uint64_t> &elements, PsiAnalyticsContext &context);
auto simple_hash(const std::vector<uint64_t> &elements, PsiAnalyticsContext &context);

std::vector<uint64_t> OprfClient(const std::vector<uint64_t> &cuckoo_table_v, PsiAnalyticsContext &context, int server_index);
std::vector<std::vector<uint64_t>> OprfServer(const std::vector<std::vector<uint64_t>> &simple_table_v, PsiAnalyticsContext &context);

std::vector<uint64_t> PolynomialsServer(const std::vector<std::vector<uint64_t>> &masks, PsiAnalyticsContext &context);

std::vector<uint64_t> OpprgPsiClient(const std::vector<uint64_t> &elements,
                                     PsiAnalyticsContext &context, int i, std::unique_ptr<CSocket> &sock);

std::vector<uint64_t> OpprgPsiServer(const std::vector<uint64_t> &polynomials,
                                     PsiAnalyticsContext &context, std::unique_ptr<CSocket> &sock);

void InterpolatePolynomials(std::vector<uint64_t> &polynomials,
                            std::vector<uint64_t> &content_of_bins,
                            const std::vector<std::vector<uint64_t>> &masks,
                            PsiAnalyticsContext &context);

void InterpolatePolynomialsPaddedWithDummies(
    std::vector<uint64_t>::iterator polynomial_offset,
    std::vector<uint64_t>::const_iterator random_value_in_bin,
    std::vector<std::vector<uint64_t>>::const_iterator masks_for_elems_in_bin,
    std::size_t nbins_in_megabin, PsiAnalyticsContext &context);

std::unique_ptr<CSocket> EstablishConnection(const std::string &address, uint16_t port,
                                             e_role role);

std::size_t PlainIntersectionSize(std::vector<std::uint64_t> v1, std::vector<std::uint64_t> v2);

void PrintBins(std::vector<std::uint64_t> &bins, std::string outfile, PsiAnalyticsContext &context);
void PrintTimings(const PsiAnalyticsContext &context);

void multi_hint_thread(int tid, std::vector<std::vector<uint64_t>> &sub_bins, std::vector<std::vector<uint64_t>> masks_with_dummies,
                        PsiAnalyticsContext &context, std::vector<std::unique_ptr<CSocket>> &allsocks);
void multi_oprf_thread(int tid, std::vector<std::vector<uint64_t>> &masks_with_dummies, std::vector<uint64_t> table, PsiAnalyticsContext &context);
}
