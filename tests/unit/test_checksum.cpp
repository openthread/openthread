/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
#include "common/message.hpp"
#include "common/numeric_limits.hpp"
#include "common/random.hpp"
#include "common/string.hpp"
#include "instance/instance.hpp"
#include "net/checksum.hpp"
#include "net/icmp6.hpp"
#include "net/ip4_types.hpp"
#include "net/udp6.hpp"
#include "utils/verhoeff_checksum.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

uint16_t CalculateChecksum(const void *aBuffer, uint16_t aLength)
{
    // Calculates checksum over a given buffer data. This implementation
    // is inspired by the algorithm from RFC-1071.

    uint32_t       sum   = 0;
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(aBuffer);

    while (aLength >= sizeof(uint16_t))
    {
        sum += BigEndian::ReadUint16(bytes);
        bytes += sizeof(uint16_t);
        aLength -= sizeof(uint16_t);
    }

    if (aLength > 0)
    {
        sum += (static_cast<uint32_t>(bytes[0]) << 8);
    }

    // Fold 32-bit sum to 16 bits.

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return static_cast<uint16_t>(sum & 0xffff);
}

uint16_t CalculateChecksum(const Ip6::Address &aSource,
                           const Ip6::Address &aDestination,
                           uint8_t             aIpProto,
                           const Message      &aMessage)
{
    // This method calculates the checksum over an IPv6 message.
    constexpr uint16_t kMaxPayload = 1024;

    OT_TOOL_PACKED_BEGIN
    struct PseudoHeader
    {
        Ip6::Address mSource;
        Ip6::Address mDestination;
        uint32_t     mPayloadLength;
        uint32_t     mProtocol;
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    struct ChecksumData
    {
        PseudoHeader mPseudoHeader;
        uint8_t      mPayload[kMaxPayload];
    } OT_TOOL_PACKED_END;

    ChecksumData data;
    uint16_t     payloadLength;

    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    data.mPseudoHeader.mSource        = aSource;
    data.mPseudoHeader.mDestination   = aDestination;
    data.mPseudoHeader.mProtocol      = BigEndian::HostSwap32(aIpProto);
    data.mPseudoHeader.mPayloadLength = BigEndian::HostSwap32(payloadLength);

    SuccessOrQuit(aMessage.Read(aMessage.GetOffset(), data.mPayload, payloadLength));

    return CalculateChecksum(&data, sizeof(PseudoHeader) + payloadLength);
}

uint16_t CalculateChecksum(const Ip4::Address &aSource,
                           const Ip4::Address &aDestination,
                           uint8_t             aIpProto,
                           const Message      &aMessage)
{
    // This method calculates the checksum over an IPv4 message.
    constexpr uint16_t kMaxPayload = 1024;

    OT_TOOL_PACKED_BEGIN
    struct PseudoHeader
    {
        Ip4::Address mSource;
        Ip4::Address mDestination;
        uint16_t     mPayloadLength;
        uint16_t     mProtocol;
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    struct ChecksumData
    {
        PseudoHeader mPseudoHeader;
        uint8_t      mPayload[kMaxPayload];
    } OT_TOOL_PACKED_END;

    ChecksumData data;
    uint16_t     payloadLength;

    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    data.mPseudoHeader.mSource        = aSource;
    data.mPseudoHeader.mDestination   = aDestination;
    data.mPseudoHeader.mProtocol      = BigEndian::HostSwap16(aIpProto);
    data.mPseudoHeader.mPayloadLength = BigEndian::HostSwap16(payloadLength);

    SuccessOrQuit(aMessage.Read(aMessage.GetOffset(), data.mPayload, payloadLength));

    return CalculateChecksum(&data, sizeof(PseudoHeader) + payloadLength);
}

void CorruptMessage(Message &aMessage)
{
    // Change a random bit in the message.

    uint16_t byteOffset;
    uint8_t  bitOffset;
    uint8_t  byte;

    byteOffset = Random::NonCrypto::GetUint16InRange(0, aMessage.GetLength());

    SuccessOrQuit(aMessage.Read(byteOffset, byte));

    bitOffset = Random::NonCrypto::GetUint8InRange(0, kBitsPerByte);

    byte ^= (1 << bitOffset);

    aMessage.Write(byteOffset, byte);
}

void TestUdpMessageChecksum(void)
{
    constexpr uint16_t kMinSize = sizeof(Ip6::Udp::Header);
    constexpr uint16_t kMaxSize = kBufferSize * 3 + 24;

    const char *kSourceAddress = "fd00:1122:3344:5566:7788:99aa:bbcc:ddee";
    const char *kDestAddress   = "fd01:2345:6789:abcd:ef01:2345:6789:abcd";

    Instance *instance = static_cast<Instance *>(testInitInstance());

    VerifyOrQuit(instance != nullptr);

    for (uint16_t size = kMinSize; size <= kMaxSize; size++)
    {
        Message         *message = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Udp::Header));
        Ip6::Udp::Header udpHeader;
        Ip6::MessageInfo messageInfo;

        VerifyOrQuit(message != nullptr, "Ip6::NewMesssage() failed");
        SuccessOrQuit(message->SetLength(size));

        // Write UDP header with a random payload.

        Random::NonCrypto::Fill(udpHeader);
        udpHeader.SetChecksum(0);
        message->Write(0, udpHeader);

        if (size > sizeof(udpHeader))
        {
            uint8_t  buffer[kMaxSize];
            uint16_t payloadSize = size - sizeof(udpHeader);

            Random::NonCrypto::FillBuffer(buffer, payloadSize);
            message->WriteBytes(sizeof(udpHeader), &buffer[0], payloadSize);
        }

        SuccessOrQuit(messageInfo.GetSockAddr().FromString(kSourceAddress));
        SuccessOrQuit(messageInfo.GetPeerAddr().FromString(kDestAddress));

        // Verify that the `Checksum::UpdateMessageChecksum` correctly
        // updates the checksum field in the UDP header on the message.

        Checksum::UpdateMessageChecksum(*message, messageInfo.GetSockAddr(), messageInfo.GetPeerAddr(), Ip6::kProtoUdp);

        SuccessOrQuit(message->Read(message->GetOffset(), udpHeader));
        VerifyOrQuit(udpHeader.GetChecksum() != 0);

        // Verify that the calculated UDP checksum is valid.

        VerifyOrQuit(CalculateChecksum(messageInfo.GetSockAddr(), messageInfo.GetPeerAddr(), Ip6::kProtoUdp,
                                       *message) == 0xffff);

        // Verify that `Checksum::VerifyMessageChecksum()` accepts the
        // message and its calculated checksum.

        SuccessOrQuit(Checksum::VerifyMessageChecksum(*message, messageInfo, Ip6::kProtoUdp));

        // Corrupt the message and verify that checksum is no longer accepted.

        CorruptMessage(*message);

        VerifyOrQuit(Checksum::VerifyMessageChecksum(*message, messageInfo, Ip6::kProtoUdp) != kErrorNone,
                     "Checksum passed on corrupted message");

        message->Free();
    }
}

void TestIcmp6MessageChecksum(void)
{
    constexpr uint16_t kMinSize = sizeof(Ip6::Icmp::Header);
    constexpr uint16_t kMaxSize = kBufferSize * 3 + 24;

    const char *kSourceAddress = "fd00:feef:dccd:baab:9889:7667:5444:3223";
    const char *kDestAddress   = "fd01:abab:beef:cafe:1234:5678:9abc:0";

    Instance *instance = static_cast<Instance *>(testInitInstance());

    VerifyOrQuit(instance != nullptr, "Null OpenThread instance\n");

    for (uint16_t size = kMinSize; size <= kMaxSize; size++)
    {
        Message          *message = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Icmp::Header));
        Ip6::Icmp::Header icmp6Header;
        Ip6::MessageInfo  messageInfo;

        VerifyOrQuit(message != nullptr, "Ip6::NewMesssage() failed");
        SuccessOrQuit(message->SetLength(size));

        // Write ICMP6 header with a random payload.

        Random::NonCrypto::Fill(icmp6Header);
        icmp6Header.SetChecksum(0);
        message->Write(0, icmp6Header);

        if (size > sizeof(icmp6Header))
        {
            uint8_t  buffer[kMaxSize];
            uint16_t payloadSize = size - sizeof(icmp6Header);

            Random::NonCrypto::FillBuffer(buffer, payloadSize);
            message->WriteBytes(sizeof(icmp6Header), &buffer[0], payloadSize);
        }

        SuccessOrQuit(messageInfo.GetSockAddr().FromString(kSourceAddress));
        SuccessOrQuit(messageInfo.GetPeerAddr().FromString(kDestAddress));

        // Verify that the `Checksum::UpdateMessageChecksum` correctly
        // updates the checksum field in the ICMP6 header on the message.

        Checksum::UpdateMessageChecksum(*message, messageInfo.GetSockAddr(), messageInfo.GetPeerAddr(),
                                        Ip6::kProtoIcmp6);

        SuccessOrQuit(message->Read(message->GetOffset(), icmp6Header));
        VerifyOrQuit(icmp6Header.GetChecksum() != 0, "Failed to update checksum");

        // Verify that the calculated ICMP6 checksum is valid.

        VerifyOrQuit(CalculateChecksum(messageInfo.GetSockAddr(), messageInfo.GetPeerAddr(), Ip6::kProtoIcmp6,
                                       *message) == 0xffff);

        // Verify that `Checksum::VerifyMessageChecksum()` accepts the
        // message and its calculated checksum.

        SuccessOrQuit(Checksum::VerifyMessageChecksum(*message, messageInfo, Ip6::kProtoIcmp6));

        // Corrupt the message and verify that checksum is no longer accepted.

        CorruptMessage(*message);

        VerifyOrQuit(Checksum::VerifyMessageChecksum(*message, messageInfo, Ip6::kProtoIcmp6) != kErrorNone,
                     "Checksum passed on corrupted message");

        message->Free();
    }
}

void TestTcp4MessageChecksum(void)
{
    constexpr size_t kMinSize = sizeof(Ip4::Tcp::Header);
    constexpr size_t kMaxSize = kBufferSize * 3 + 24;

    const char *kSourceAddress = "12.34.56.78";
    const char *kDestAddress   = "87.65.43.21";

    Ip4::Address sourceAddress;
    Ip4::Address destAddress;

    Instance *instance = static_cast<Instance *>(testInitInstance());

    VerifyOrQuit(instance != nullptr);

    SuccessOrQuit(sourceAddress.FromString(kSourceAddress));
    SuccessOrQuit(destAddress.FromString(kDestAddress));

    for (uint16_t size = kMinSize; size <= kMaxSize; size++)
    {
        Message         *message = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip4::Tcp::Header));
        Ip4::Tcp::Header tcpHeader;

        VerifyOrQuit(message != nullptr, "Ip6::NewMesssage() failed");
        SuccessOrQuit(message->SetLength(size));

        // Write TCP header with a random payload.

        Random::NonCrypto::Fill(tcpHeader);
        message->Write(0, tcpHeader);

        if (size > sizeof(tcpHeader))
        {
            uint8_t  buffer[kMaxSize];
            uint16_t payloadSize = size - sizeof(tcpHeader);

            Random::NonCrypto::FillBuffer(buffer, payloadSize);
            message->WriteBytes(sizeof(tcpHeader), &buffer[0], payloadSize);
        }

        // Verify that the `Checksum::UpdateMessageChecksum` correctly
        // updates the checksum field in the UDP header on the message.

        Checksum::UpdateMessageChecksum(*message, sourceAddress, destAddress, Ip4::kProtoTcp);

        SuccessOrQuit(message->Read(message->GetOffset(), tcpHeader));
        VerifyOrQuit(tcpHeader.GetChecksum() != 0);

        // Verify that the calculated UDP checksum is valid.

        VerifyOrQuit(CalculateChecksum(sourceAddress, destAddress, Ip4::kProtoTcp, *message) == 0xffff);
        message->Free();
    }
}

void TestUdp4MessageChecksum(void)
{
    constexpr uint16_t kMinSize = sizeof(Ip4::Udp::Header);
    constexpr uint16_t kMaxSize = kBufferSize * 3 + 24;

    const char *kSourceAddress = "12.34.56.78";
    const char *kDestAddress   = "87.65.43.21";

    Ip4::Address sourceAddress;
    Ip4::Address destAddress;

    Instance *instance = static_cast<Instance *>(testInitInstance());

    SuccessOrQuit(sourceAddress.FromString(kSourceAddress));
    SuccessOrQuit(destAddress.FromString(kDestAddress));

    VerifyOrQuit(instance != nullptr);

    for (uint16_t size = kMinSize; size <= kMaxSize; size++)
    {
        Message         *message = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip4::Udp::Header));
        Ip4::Udp::Header udpHeader;

        VerifyOrQuit(message != nullptr, "Ip6::NewMesssage() failed");
        SuccessOrQuit(message->SetLength(size));

        // Write UDP header with a random payload.

        Random::NonCrypto::Fill(udpHeader);
        udpHeader.SetChecksum(0);
        message->Write(0, udpHeader);

        if (size > sizeof(udpHeader))
        {
            uint8_t  buffer[kMaxSize];
            uint16_t payloadSize = size - sizeof(udpHeader);

            Random::NonCrypto::FillBuffer(buffer, payloadSize);
            message->WriteBytes(sizeof(udpHeader), &buffer[0], payloadSize);
        }

        // Verify that the `Checksum::UpdateMessageChecksum` correctly
        // updates the checksum field in the UDP header on the message.

        Checksum::UpdateMessageChecksum(*message, sourceAddress, destAddress, Ip4::kProtoUdp);

        SuccessOrQuit(message->Read(message->GetOffset(), udpHeader));
        VerifyOrQuit(udpHeader.GetChecksum() != 0);

        // Verify that the calculated UDP checksum is valid.

        VerifyOrQuit(CalculateChecksum(sourceAddress, destAddress, Ip4::kProtoUdp, *message) == 0xffff);
        message->Free();
    }
}

void TestIcmp4MessageChecksum(void)
{
    // A captured ICMP echo request (ping) message. Checksum field is set to zero.
    const uint8_t kExampleIcmpMessage[]      = "\x08\x00\x00\x00\x67\x2e\x00\x00\x62\xaf\xf1\x61\x00\x04\xfc\x24"
                                               "\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17"
                                               "\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27"
                                               "\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37";
    uint16_t      kChecksumForExampleMessage = 0x5594;
    Instance     *instance                   = static_cast<Instance *>(testInitInstance());
    Message      *message                    = instance->Get<Ip6::Ip6>().NewMessage(sizeof(kExampleIcmpMessage));

    Ip4::Address source;
    Ip4::Address dest;

    uint8_t           mPayload[sizeof(kExampleIcmpMessage)];
    Ip4::Icmp::Header icmpHeader;

    SuccessOrQuit(message->AppendBytes(kExampleIcmpMessage, sizeof(kExampleIcmpMessage)));

    // Random IPv4 address, ICMP message checksum does not include a presudo header like TCP and UDP.
    source.mFields.m32 = 0x12345678;
    dest.mFields.m32   = 0x87654321;

    Checksum::UpdateMessageChecksum(*message, source, dest, Ip4::kProtoIcmp);

    SuccessOrQuit(message->Read(0, icmpHeader));
    VerifyOrQuit(icmpHeader.GetChecksum() == kChecksumForExampleMessage);

    SuccessOrQuit(message->Read(message->GetOffset(), mPayload, sizeof(mPayload)));
    VerifyOrQuit(CalculateChecksum(mPayload, sizeof(mPayload)) == 0xffff);
}

class ChecksumTester
{
public:
    static void TestExampleVector(void)
    {
        // Example from RFC 1071
        const uint8_t  kTestVector[]       = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
        const uint16_t kTestVectorChecksum = 0xddf2;

        Checksum checksum;

        VerifyOrQuit(checksum.GetValue() == 0, "Incorrect initial checksum value");

        checksum.AddData(kTestVector, sizeof(kTestVector));
        VerifyOrQuit(checksum.GetValue() == kTestVectorChecksum);
        VerifyOrQuit(checksum.GetValue() == CalculateChecksum(kTestVector, sizeof(kTestVector)), );
    }
};

#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

void TestVerhoeffChecksum(void)
{
    static constexpr uint16_t kMaxStringSize = 50;

    const char *kExamples[] = {"307318421", "487300178", "123455672", "0",   "15",
                               "999999994", "000000001", "100000000", "2363"};

    const char *kInvalidFormats[] = {
        "307 318421",
        "307318421 ",
        " 307318421",
        "ABCDE",
    };

    char string[kMaxStringSize];
    char checksum;
    char expectedChecksum;

    printf("\nVerhoeffChecksum\n");

    for (const char *example : kExamples)
    {
        uint16_t length = StringLength(example, kMaxStringSize - 1);

        memcpy(string, example, length + 1);

        printf("- \"%s\"\n", string);

        SuccessOrQuit(Utils::VerhoeffChecksum::Validate(string));

        expectedChecksum = string[length - 1];

        string[length - 1] = (expectedChecksum == '0') ? '9' : (expectedChecksum - 1);
        VerifyOrQuit(Utils::VerhoeffChecksum::Validate(string) == kErrorFailed);

        string[length - 1] = '\0';
        SuccessOrQuit(Utils::VerhoeffChecksum::Calculate(string, checksum));
        VerifyOrQuit(checksum == expectedChecksum);

        string[length - 1] = expectedChecksum == '0' ? '9' : (expectedChecksum - 1);
    }

    printf("\nInvalid format:\n");

    for (const char *example : kInvalidFormats)
    {
        printf("- \"%s\"\n", example);
        VerifyOrQuit(Utils::VerhoeffChecksum::Validate(example) == kErrorInvalidArgs);
        VerifyOrQuit(Utils::VerhoeffChecksum::Calculate(example, checksum) == kErrorInvalidArgs);
    }
}

#endif // OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

} // namespace ot

int main(void)
{
    ot::ChecksumTester::TestExampleVector();
    ot::TestUdpMessageChecksum();
    ot::TestIcmp6MessageChecksum();
    ot::TestTcp4MessageChecksum();
    ot::TestUdp4MessageChecksum();
    ot::TestIcmp4MessageChecksum();
#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE
    ot::TestVerhoeffChecksum();
#endif

    printf("All tests passed\n");
    return 0;
}
