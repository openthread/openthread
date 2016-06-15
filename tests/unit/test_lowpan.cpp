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
#include <common/debug.hpp>
#include <string.h>

#include <openthread.h>
#include <mac/mac.hpp>
#include <thread/thread_netif.hpp>
#include <thread/lowpan.hpp>

using namespace Thread;

namespace Thread {

extern "C" void otSignalTaskletPending(void) {}

ThreadNetif sMockThreadNetif;
Lowpan::Lowpan sMockLowpan(sMockThreadNetif);

typedef const struct
{
    const char *mResult;

    uint16_t mFcf;
    uint16_t mSeq;
    uint16_t mPanid;       ///< default panid is destination panid
    const char *mSrc;
    const char *mDst;
} test_mac_vector_t;

typedef const struct
{
    const char *mTest;
    const char *mCompressed;
    const char *mRaw;

    test_mac_vector_t mMac;

    uint16_t mTraffic;
    uint16_t mFlow;
    uint16_t mHops;
    const char *mSrc;
    const char *mDst;
} test_lowpan_vector_t;



test_lowpan_vector_t tests[] = {
  {
    // l1_t1_AF_pass.pcap
    .mTest = "LL64 unicast ICMP ping request",
    .mCompressed = 
        "61 cc 1d ce fa 03 00 00  00 00 0a 6e 14 01 00 00 "
        "00 00 0a 6e 14 7a 33 3a  80 00 30 be 00 00 00 00 "
        "41 42 43 44 45 46 47 48  48 ff",
    .mRaw = 
        "60 00 00 00 00 10 3a 40  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 01  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 03  80 00 30 be 00 00 00 00 "
        "41 42 43 44 45 46 47 48",
    .mMac = 
    {
        .mSrc = "14 6e 0a 00 00 00 00 01",
        .mDst = "14 6e 0a 00 00 00 00 03",
        .mPanid = 0xFACE,
    },
    .mSrc = "fe80::166e:a00:0:1",
    .mDst = "fe80::166e:a00:0:3",
    .mHops = 64
  },
  {
    // l1_t1_AF_pass.pcap
    .mTest = "LL64 unicast ICMP ping reply",
    .mCompressed = 
        "61 cc 07 ce fa 01 00 00  00 00 0a 6e 14 03 00 00 "
        "00 00 0a 6e 14 7a 33 3a  81 00 2f be 00 00 00 00 "
        "41 42 43 44 45 46 47 48  37 59",
    .mRaw = 
        "60 00 00 00 00 10 3a 40  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 03  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 01  81 00 2f be 00 00 00 00 "
        "41 42 43 44 45 46 47 48",
    .mMac = 
    {
        .mSrc = "14 6e 0a 00 00 00 00 03",
        .mDst = "14 6e 0a 00 00 00 00 01",
        .mPanid = 0xFACE,
    },
  },
  {
    // l1_t2_AF_AS_pass.pcap
    .mTest = "LL16 unicast ICMP ping request",
    .mCompressed =
        "61 88 13 ce fa 00 10 00  00 7a 33 3a 80 00 63 9e "
        "00 00 00 00 41 42 43 44  45 46 47 48 84 5f",
    .mRaw =
        "60 00 00 00 00 10 3a 40  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 00 00  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 10 00  80 00 63 9e 00 00 00 00 "
        "41 42 43 44 45 46 47 48"
  },
  {
    // l1_t2_AF_AS_pass.pcap
    .mTest = "LL16 unicast ICMP ping reply",
    .mCompressed =
        "61 88 0f ce fa 00 00 00 10 7a 33 3a 81 00 62 9e "
        "00 00 00 00 41 42 43 44 45 46 47 48 e0 35",
    .mRaw =
        "60 00 00 00 00 10 3a 40  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 10 00  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 00 00  81 00 62 9e 00 00 00 00 "
        "41 42 43 44 45 46 47 48"
  },
  {
    // l1_t3_SF_pass.pcap
    .mTest = "LL64 multicast FF02::1 ICMP ping request",
    .mCompressed =
       "41 c8 99 ce fa ff ff 01 00 00 00 00 0a 6e 14 7a "
       "3b 3a 01 80 00 54 b4 40 41 42 43 44 45 46 47 68 "
       "44",
    .mRaw =
        "60 00 00 00 00 0c 3a 40  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 01  ff 02 00 00 00 00 00 00 "
        "00 00 00 00 00 00 00 01  80 00 54 b4 40 41 42 43 "
        "44 45 46 47 "
  },
  {
    // l1_t3_SF_pass.pcap
    .mTest = "LL64 multicast FF02::1 ICMP ping reply",
    .mCompressed =
       "61 cc fc ce fa 01 00 00 00 00 0a 6e 14 02 00 00 "
       "00 00 0a 6e 14 7a 33 3a 81 00 33 c7 40 41 42 43 "
       "44 45 46 47 1a 80",
    .mRaw =
        "60 00 00 00 00 0c 3a 40  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 02  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 01  81 00 33 c7 40 41 42 43 "
        "44 45 46 47"
  },
  {
    // l1_t4_FS_pass.pcap
    .mTest = "LL16 multicast FF02::1 ICMP ping request",
    .mCompressed =
       "41 88 df ce fa ff ff 00   08 7a 3b 3a 01 80 00 76 "
       "0e 00 01 00 04 50 50 50   50 50 50 50 50 50 50 50 "
       "50 50 50 50 50 50 50 50   50 50 50 50 50 50 50 50 "
       "50 50 50 50 50 a7 d2",
    .mRaw =
        "60 00 00 00 00 28 3a 40  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 08 00  ff 02 00 00 00 00 00 00 "
        "00 00 00 00 00 00 00 01  80 00 76 0e 00 01 00 04 "
        "50 50 50 50 50 50 50 50  50 50 50 50 50 50 50 50 "
        "50 50 50 50 50 50 50 50  50 50 50 50 50 50 50 50 "
  },
  {
    // l1_t4_FS_pass.pcap
    .mTest = "LL16 multicast FF02::1 ICMP ping reply",
    .mCompressed =
       "61 c8 41 ce fa 00 08 03 00 00 00 00 0a 6e 14 7a "
       "33 3a 81 00 55 20 00 01 00 04 50 50 50 50 50 50 "
       "50 50 50 50 50 50 50 50 50 50 50 50 50 50 50 50 "
       "50 50 50 50 50 50 50 50 50 50 ab 56",
    .mRaw =
        "60 00 00 00 00 28 3a 40  fe 80 00 00 00 00 00 00 "
        "16 6e 0a 00 00 00 00 03  fe 80 00 00 00 00 00 00 "
        "00 00 00 ff fe 00 08 00  81 00 55 20 00 01 00 04 "
        "50 50 50 50 50 50 50 50  50 50 50 50 50 50 50 50 "
        "50 50 50 50 50 50 50 50  50 50 50 50 50 50 50 50 "
  },

};


void TestLowpanIphc(void)
{
    Message *message = NULL;

    uint8_t  result[1500];
    unsigned resultLength = 0;
    
    Mac::Address macSource;
    Mac::Address macDest;

    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
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
	frame.mLength = iphcVector.size();
	frame.GetSrcAddr(macSource);
	frame.GetDstAddr(macDest);

	VerifyOrQuit((message = Ip6::Ip6::NewMessage(0)) != NULL, 
		     "6lo: Ip6::NewMessage failed");

	// ===> Test Lowpan::Decompress
        int decompressedBytes = 
	    sMockLowpan.Decompress(*message, macSource, macDest, 
				   frame.GetPayload(), 
				   frame.GetPayloadLength(), 0);

	uint16_t ip6PayloadLength = frame.GetPayloadLength() -
	                            decompressedBytes;
	message->Append(frame.GetPayload() + decompressedBytes, 
			ip6PayloadLength);
	ip6PayloadLength = HostSwap16(message->GetLength() - 
				      sizeof(Ip6::Header));
	message->Write(Ip6::Header::GetPayloadLengthOffset(), 
		       sizeof(ip6PayloadLength), &ip6PayloadLength);

	resultLength = message->GetLength();
	message->Read(0, resultLength, result);

	printf("Decompressed OpenThread:\n");
	otTestPrintHex(result, resultLength);

	SuccessOrQuit(memcmp(ipVector.data(), result, resultLength),
		      "6lo: Lowpan::Decompress failed");
	
	SuccessOrQuit(Message::Free(*message), "6lo: Message:Free failed");

	printf("PASS\n\n");
    }
}

}  // namespace Thread


int main(void)
{
    Message::Init();

    TestLowpanIphc();

    printf("All tests passed\n");
    return 0;
}
