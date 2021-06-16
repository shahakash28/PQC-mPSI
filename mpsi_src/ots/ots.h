#pragma once

//
// \file ots.h
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

#include <cinttypes>
#include <string>
#include <vector>

#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Network/Channel.h"

#include "common/psi_analytics_context.h"
#include "common/constants.h"

namespace ENCRYPTO {

osuCrypto::Session ot_receiver_connect(ENCRYPTO::PsiAnalyticsContext& context, int server_index,
				       osuCrypto::IOService& ios, osuCrypto::Channel& recvChl);

std::vector<std::uint64_t> ot_receiver(const std::vector<std::uint64_t>& inputs, osuCrypto::Channel& recvChl,
                                       ENCRYPTO::PsiAnalyticsContext& context, int server_index);

void ot_receiver_disconnect(osuCrypto::Channel& recvChl, osuCrypto::IOService& ios, osuCrypto::Session& ep);

osuCrypto::Session ot_sender_connect(ENCRYPTO::PsiAnalyticsContext& context, osuCrypto::IOService& ios,
				     osuCrypto::Channel& sendChl);

std::vector<std::vector<std::uint64_t>> ot_sender(const std::vector<std::vector<std::uint64_t>>& inputs, 
						  osuCrypto::Channel& sendChl,ENCRYPTO::PsiAnalyticsContext& context);
void ot_sender_disconnect(osuCrypto::Channel& sendChl, osuCrypto::IOService& ios, osuCrypto::Session& ep);
}