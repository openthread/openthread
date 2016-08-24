/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "test_util.hpp"
#include "test_vector.h"
#include <common/debug.hpp>
#include <string.h>

#include <openthread.h>
#include <mac/mac.hpp>
#include <thread/thread_netif.hpp>
#include <thread/lowpan.hpp>
#include <openthreadinstance.h>

using namespace Thread;

namespace Thread {

otInstance sContext;
Lowpan::Lowpan sMockLowpan(sContext.mThreadNetif);

void TestLowpanIphc(void)
{
    Message *message = NULL;

    uint8_t  result[1500];
    unsigned resultLength = 0;

    Mac::Address macSource;
    Mac::Address macDest;

    test_lowpan_vector_t *tests = sTestVectorLowpan;

    for (unsigned i = 0; i < sTestVectorLowpanLen; i++)
    {
        // Prepare next test vector
        std::string ipString(tests[i].mRaw);
        std::vector<uint8_t> ipVector;
        otTestHexToVector(ipString, ipVector);

        std::string iphcString(tests[i].mCompressed);
        std::vector<uint8_t> iphcVector;
        otTestHexToVector(iphcString, iphcVector);

        printf("=== Packet #%d: %s ===\n", i, tests[i].mTest);
        printf("6lo Packet:\n");
        otTestPrintHex(iphcVector);

        printf("Decompressed Reference:\n");
        otTestPrintHex(ipVector);

        // Test decompression

        Mac::Frame frame;
        frame.mPsdu = iphcVector.data();
        frame.mLength = (uint8_t)iphcVector.size();
        frame.GetSrcAddr(macSource);
        frame.GetDstAddr(macDest);

        VerifyOrQuit((message = Ip6::Ip6::NewMessage(&sContext, 0)) != NULL,
                     "6lo: Ip6::NewMessage failed");

        // ===> Test Lowpan::Decompress
        int decompressedBytes =
            sMockLowpan.Decompress(*message, macSource, macDest,
                                   frame.GetPayload(),
                                   frame.GetPayloadLength(), 0);

        uint16_t ip6PayloadLength = frame.GetPayloadLength() -
                                    decompressedBytes;
        SuccessOrQuit(message->Append(frame.GetPayload() + decompressedBytes,
                                      ip6PayloadLength),
                      "6lo: Message::Append failed");
        ip6PayloadLength = HostSwap16(message->GetLength() -
                                      sizeof(Ip6::Header));
        message->Write(Ip6::Header::GetPayloadLengthOffset(),
                       sizeof(ip6PayloadLength), &ip6PayloadLength);

        resultLength = message->GetLength();
        message->Read(0, resultLength, result);

        printf("Decompressed OpenThread:\n");
        otTestPrintHex(result, resultLength);

        VerifyOrQuit(memcmp(ipVector.data(), result, resultLength) == 0,
                     "6lo: Lowpan::Decompress failed");

        // ===> Test Lowpan::Compress
        int compressBytes = sMockLowpan.Compress(*message, macSource, macDest,
                                                 result);
        printf("Compressed OpenThread:\n");
        otTestPrintHex(result, compressBytes);

        VerifyOrQuit(memcmp(frame.GetPayload(), result, compressBytes) == 0,
                     "6lo: Lowpan::Compress failed");

        SuccessOrQuit(Message::Free(*message), "6lo: Message:Free failed");
        printf("PASS\n\n");

    }
}

}  // namespace Thread


int main(void)
{
    TestLowpanIphc();

    printf("All tests passed\n");
    return 0;
}
