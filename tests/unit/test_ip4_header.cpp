/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#include "common/encoding.hpp"
#include "net/ip4_headers.hpp"
#include "net/ip4_types.hpp"

#include "test_util.hpp"

using ot::Encoding::BigEndian::ReadUint16;

namespace ot {
namespace Ip4 {

void VerifyEcnDscp(const Header &aHeader, uint8_t aDscp, Ecn aEcn)
{
    uint8_t expectedDscpEcn = static_cast<uint8_t>((aDscp << 2) + aEcn);

    printf("%08x {dscp:%d, ecn:%d, flow:%d}\n", aHeader.GetDscpEcn(), aHeader.GetDscp(), aHeader.GetEcn());

    VerifyOrQuit(aHeader.GetDscp() == aDscp);
    VerifyOrQuit(aHeader.GetEcn() == aEcn);
    VerifyOrQuit(aHeader.GetDscpEcn() == expectedDscpEcn);
}

void TestIp4Header(void)
{
    static constexpr uint16_t kTotalLength = 84;
    static constexpr uint8_t  kTTL         = 64;

    const uint16_t kFlows[] = {0x0, 0x1, 0xfff, 0xffff, 0xff00};
    const uint8_t  kFlags[] = {0x0, 0x1, 0x3, 0xf, 0x30, 0x2f, 0x3f};

    Header         header;
    Address        source;
    Address        destination;
    const uint8_t *headerBytes = reinterpret_cast<const uint8_t *>(&header);

    SuccessOrQuit(source.FromString("10.0.0.235"), "Address::FromString() failed");
    SuccessOrQuit(destination.FromString("10.0.0.1"), "Address::FromString() failed");

    header.InitVersionIHL();

    header.Clear();
    header.InitVersionIHL();
    VerifyOrQuit(header.IsValid());
    VerifyOrQuit(header.GetTotalLength() == 0);
    VerifyOrQuit(header.GetProtocol() == 0);
    VerifyOrQuit(header.GetTTL() == 0);
    VerifyOrQuit(header.GetSource().mFields.m32 == 0);
    VerifyOrQuit(header.GetDestination().mFields.m32 == 0);
    VerifyOrQuit(header.GetFragmentOffset() == 0);

    header.SetTotalLength(kTotalLength);
    header.SetProtocol(Ip4::kProtoIcmp);
    header.SetTTL(kTTL);
    header.SetSource(source);
    header.SetDestination(destination);

    VerifyOrQuit(header.IsValid());
    VerifyOrQuit(header.GetProtocol() == kProtoIcmp);
    VerifyOrQuit(header.GetTTL() == kTTL);
    VerifyOrQuit(header.GetSource() == source);
    VerifyOrQuit(header.GetDestination() == destination);

    // Verify the offsets to different fields.

    VerifyOrQuit(ReadUint16(headerBytes + Header::kTotalLengthOffset) == kTotalLength, "kTotalLength is incorrect");
    VerifyOrQuit(headerBytes[Header::kProtocolOffset] == kProtoIcmp, "kProtocol is incorrect");
    VerifyOrQuit(headerBytes[Header::kTTLOffset] == kTTL, "kTTL is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[Header::kSourceAddressOffset], &source, sizeof(source)) == 0,
                 "Source address is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[Header::kDestinationAddressOffset], &destination, sizeof(destination)) == 0,
                 "Destination address is incorrect");

    const uint8_t kDscps[] = {0x0, 0x1, 0x3, 0xf, 0x30, 0x2f, 0x3f};
    const Ecn     kEcns[]  = {Ecn::kEcnNotCapable, Ecn::kEcnCapable0, Ecn::kEcnCapable1, Ecn::kEcnMarked};
    for (uint8_t dscp : kDscps)
    {
        for (Ecn ecn : kEcns)
        {
            printf("Expecting {dscp:%-2d, ecn:%d} => ", dscp, ecn);
            header.SetEcn(ecn);
            header.SetDscp(dscp);
            VerifyEcnDscp(header, dscp, ecn);
        }
    }

    const uint8_t kExampleIp4Header[] = "\x45\x00\x00\x54\x23\xed\x00\x00\x40\x01\x41\xd1\x0a\x00\x00\xeb"
                                        "\x0a\x00\x00\x01";
    memcpy(&header, kExampleIp4Header, sizeof(header));
    VerifyOrQuit(header.IsValid());
    VerifyOrQuit(memcmp(&headerBytes[Header::kSourceAddressOffset], &source, sizeof(source)) == 0,
                 "Source address is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[Header::kDestinationAddressOffset], &destination, sizeof(destination)) == 0,
                 "Destination address is incorrect");
    VerifyOrQuit(ReadUint16(headerBytes + Header::kTotalLengthOffset) == kTotalLength, "kTotalLength is incorrect");
    VerifyOrQuit(headerBytes[Header::kProtocolOffset] == kProtoIcmp, "kProtocol is incorrect");
    VerifyOrQuit(headerBytes[Header::kTTLOffset] == kTTL, "kTTL is incorrect");
}

} // namespace Ip4
} // namespace ot

int main(void)
{
    ot::Ip4::TestIp4Header();
    printf("All tests passed\n");
    return 0;
}
