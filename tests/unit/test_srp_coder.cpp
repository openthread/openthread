/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "test_platform.h"
#include "test_util.hpp"

#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "net/srp_coder.hpp"

#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE

namespace ot {
namespace Srp {

class UnitTester
{
public:
    static void TestCompactUint(void)
    {
        Instance *instance;
        Message  *message;

        printf("=================================================================================================\n");
        printf("\nTestCompactUint\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);

        for (uint8_t firstSegBitLength = 8; firstSegBitLength > 1; firstSegBitLength--)
        {
            printf("-----------------------------------------------\n");

            for (uint8_t bit = 0; bit < 31; bit++)
            {
                ValidateCompactUint(*message, (1 << bit), firstSegBitLength);
            }

            for (uint8_t bit = 0; bit < 31; bit++)
            {
                ValidateCompactUint(*message, (1 << bit) - 1, firstSegBitLength);
            }

            for (uint8_t shift = 0; shift < 31; shift++)
            {
                ValidateCompactUint(*message, 0xffffffff >> shift, firstSegBitLength);
                ValidateCompactUint(*message, 0x55555555 >> shift, firstSegBitLength);
                ValidateCompactUint(*message, 0xdbdbdbdb >> shift, firstSegBitLength);
                ValidateCompactUint(*message, 0x36cb25ea >> shift, firstSegBitLength);
            }

            for (uint8_t bit = 1; bit < 31; bit++)
            {
                uint32_t mask = ~((1 << bit) - 1);

                ValidateCompactUint(*message, 0xffffffff & mask, firstSegBitLength);
                ValidateCompactUint(*message, 0x55555555 & mask, firstSegBitLength);
                ValidateCompactUint(*message, 0xdbdbdbdb & mask, firstSegBitLength);
                ValidateCompactUint(*message, 0x36cb25ea & mask, firstSegBitLength);
            }

            ValidateCompactUint(*message, 0x2c, firstSegBitLength);
            ValidateCompactUint(*message, 0x1234, firstSegBitLength);
            ValidateCompactUint(*message, 1020315, firstSegBitLength);
            ValidateCompactUint(*message, 0xcafebeef, firstSegBitLength);
        }

        testFreeInstance(instance);

        printf("\n\n");
    }

    static void TestAppendReadLabel(void)
    {
        // Dispatch types.
        static constexpr uint8_t kNormal         = 0x00;
        static constexpr uint8_t kService        = 0x40;
        static constexpr uint8_t kReferOffset    = 0x80;
        static constexpr uint8_t kCommonConstant = 0xc0;
        static constexpr uint8_t kGenerative     = 0xe0;

        struct TestLabel
        {
            const char *mLabel;
            uint8_t     mExpectedDispatch;
        };

        static const TestLabel kTestLabels[] = {
            {"hostname", kNormal + 8},                              // Regular name
            {"_test", kService + 4},                                // Service name starting with `_`
            {"_udp", kCommonConstant + 0},                          // Commonly used constant label
            {"_tcp", kCommonConstant + 1},                          // Commonly used constant label
            {"_matter", kCommonConstant + 2},                       // Commonly used constant label
            {"_matterc", kCommonConstant + 3},                      // Commonly used constant label
            {"_matterd", kCommonConstant + 4},                      // Commonly used constant label
            {"_hap", kCommonConstant + 5},                          // Commonly used constant label
            {"hostname", kReferOffset + 0},                         // Refer to previous regular name.
            {"_test", kReferOffset + 9},                            // Refer to previous service name
            {"0123456789ABCDEF", kGenerative + 0},                  // Gen pattern - single hex value
            {"DEADBEEFCAFE7777-0011223344556677", kGenerative + 1}, // Gen pattern - two hex values
            {"_XAA557733CC00EE11", kGenerative + 2},                // Gen pattern - sub-type _<char><hex-value>
            {"_IAA557733CC00EE11", kGenerative + 3},                // Gen pattern - sub-type refer
            {"_v0011223344556677", kGenerative + 3},                // Gen pattern, -sub-type refer
            {"", kNormal + 0},                                      // Empty label
            {"_matter", kCommonConstant + 2},                       // Repeated constant (should not use refer)
            {"0123456789ABCDEf", kNormal + 16},                     // Lowercase letter in hex value
            {"023456789ABCDEF", kNormal + 15},                      // Short hex value
            {"00112233445566778", kNormal + 17},                    // Long hex value
        };

        Instance               *instance;
        Message                *message;
        Coder::OffsetRangeArray prevOffsetRanges;
        OffsetRange             offsetRange;
        Coder::LabelBuffer      label;
        uint8_t                 buffer[200];

        printf("=================================================================================================\n");
        printf("\nTestAppendReadLabel\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);

        prevOffsetRanges.PushBack()->Init(0, 0);

        for (const TestLabel &testLabel : kTestLabels)
        {
            prevOffsetRanges[0].InitFromMessageFullLength(*message);

            SuccessOrQuit(Coder::AppendLabel(*message, testLabel.mLabel, prevOffsetRanges));
        }

        SuccessOrQuit(message->Read(/* aOffset */ 0, buffer, message->GetLength()));
        DumpBuffer("Encoded Names", buffer, message->GetLength());

        printf("\n\nReading labels");

        offsetRange.InitFromMessageFullLength(*message);

        for (const TestLabel &testLabel : kTestLabels)
        {
            printf("\n- offset %-3u  dispatch 0x%02x -> \"%s\"", offsetRange.GetOffset(), testLabel.mExpectedDispatch,
                   testLabel.mLabel);

            VerifyOrQuit(buffer[offsetRange.GetOffset()] == testLabel.mExpectedDispatch);
            SuccessOrQuit(Coder::ReadLabel(*message, offsetRange, label));
            VerifyOrQuit(StringMatch(label, testLabel.mLabel));
        }

        VerifyOrQuit(offsetRange.IsEmpty());

        printf("\n\n");

        testFreeInstance(instance);
    }

    static void TestAppendReadName(void)
    {
        struct TestName
        {
            const char *mName;
            const char *mReadName;
        };

        static const TestName kTestNames[] = {
            {"_srv._udp", "_srv._udp."},
            {"_matter._tcp.", "_matter._tcp."},
            {".", "."},
            {"", "."},
            {"foo.bar.baz", "foo.bar.baz."},
            {"1122334455667788-ABCDEF0123456789._srv._udp", "1122334455667788-ABCDEF0123456789._srv._udp."},
        };

        Instance               *instance;
        Message                *message;
        Coder::OffsetRangeArray prevOffsetRanges;
        OffsetRange             offsetRange;
        Coder::NameBuffer       name;
        uint8_t                 buffer[200];

        printf("=================================================================================================\n");
        printf("\nTestAppendReadName\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);

        prevOffsetRanges.PushBack()->Init(0, 0);

        for (const TestName &testName : kTestNames)
        {
            prevOffsetRanges[0].InitFromMessageFullLength(*message);

            SuccessOrQuit(Coder::AppendName(*message, testName.mName, prevOffsetRanges));
        }

        SuccessOrQuit(message->Read(/* aOffset */ 0, buffer, message->GetLength()));
        DumpBuffer("Encoded Names", buffer, message->GetLength());

        printf("\n\nReading labels");

        offsetRange.InitFromMessageFullLength(*message);

        for (const TestName &testName : kTestNames)
        {
            printf("\n- offset %-3u  \"%s\" -> \"%s\"", offsetRange.GetOffset(), testName.mName, testName.mReadName);

            SuccessOrQuit(Coder::ReadName(*message, offsetRange, name));
            VerifyOrQuit(StringMatch(name, testName.mReadName));
        }

        VerifyOrQuit(offsetRange.IsEmpty());

        printf("\n\n");

        testFreeInstance(instance);
    }

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

    static void TestMsgEncoder(void)
    {
        static const char *kSubLabels1[] = {
            "_IAA557733CC00EE11",
            nullptr,
        };

        Instance                        *instance = static_cast<Instance *>(testInitInstance());
        Ip6::Udp::Socket                 udpSocket(*instance, nullptr, nullptr);
        Coder::MsgEncoder                encoder;
        Coder::MsgEncoder::ClientService service1;

        uint8_t buffer[300];

        printf("=================================================================================================\n");
        printf("\nTestMsgEncoder\n");

        VerifyOrQuit(!encoder.HasMessage());

        SuccessOrQuit(encoder.AllocateMessage(udpSocket));
        VerifyOrQuit(encoder.HasMessage());

        SuccessOrQuit(encoder.EncodeHeaderBlock(0x1234, "default.service.arpa", 7200, "0011223344556677"));

        ClearAllBytes(service1);
        service1.mName          = "_matter._tcp";
        service1.mInstanceName  = "AA557733CC00EE11-0123456789ABCDEF";
        service1.mSubTypeLabels = kSubLabels1;

        SuccessOrQuit(encoder.EncodeServiceBlock(service1, /* aRemoving */ false));

        SuccessOrQuit(encoder.GetMessage()->Read(/* aOffset */ 0, buffer, encoder.GetMessage()->GetLength()));
        DumpBuffer("Encoded msg", buffer, encoder.GetMessage()->GetLength());

        printf("\n\n");

        testFreeInstance(instance);
    }

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

private:
    static void ValidateCompactUint(Message &aMessage, uint32_t aUint, uint8_t aFirstSegBitLength)
    {
        static constexpr uint8_t kBufferSize = 16;

        uint8_t numIters = (aFirstSegBitLength == 8) ? 1 : 2;

        for (uint8_t iter = 0; iter < numIters; iter++)
        {
            // Use different extra bits for the first segment
            // in different iterations.

            uint8_t     firstSegMask      = ~((1 << aFirstSegBitLength) - 1);
            uint8_t     firstSegExtraBits = (iter == 0) ? 0 : firstSegMask;
            OffsetRange offsetRange;
            uint8_t     buffer[16];
            uint16_t    length;
            uint32_t    readUint;
            uint8_t     numBits;

            SuccessOrQuit(aMessage.SetLength(0));

            // Append the compact uint

            if (aFirstSegBitLength == 8)
            {
                SuccessOrQuit(Coder::AppendCompactUint(aMessage, aUint));
            }
            else
            {
                SuccessOrQuit(Coder::AppendCompactUint(aMessage, aUint, aFirstSegBitLength, firstSegExtraBits));
            }

            // Read the encoded bytes from message and validate that first
            // byte contains the expected extra bits.

            length = aMessage.GetLength();
            VerifyOrQuit(length < kBufferSize);
            SuccessOrQuit(aMessage.Read(/* aOffset */ 0, buffer, length));

            printf("Compact uint 0x%-8x %-10u  1st-seg-len=%u seg-extra=0x%02x -> len = %u  [ ", aUint, aUint,
                   aFirstSegBitLength, firstSegExtraBits, length);

            for (uint16_t index = 0; index < length; index++)
            {
                printf("%02x ", buffer[index]);
            }

            printf("]\n");

            if (aFirstSegBitLength != 8)
            {
                VerifyOrQuit((buffer[0] & firstSegMask) == firstSegExtraBits);
            }

            // Read the compact uint and validate it matches
            // original written value and the `offsetRange` is
            // advanced properly.

            offsetRange.InitFromMessageFullLength(aMessage);

            if (aFirstSegBitLength == 8)
            {
                SuccessOrQuit(Coder::ReadCompactUint(aMessage, offsetRange, readUint));
            }
            else
            {
                SuccessOrQuit(Coder::ReadCompactUint(aMessage, offsetRange, readUint, aFirstSegBitLength));
            }

            VerifyOrQuit(readUint == aUint);
            VerifyOrQuit(offsetRange.IsEmpty());
        }
    }
};

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CODER_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE
    ot::Srp::UnitTester::TestCompactUint();
    ot::Srp::UnitTester::TestAppendReadLabel();
    ot::Srp::UnitTester::TestAppendReadName();
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    ot::Srp::UnitTester::TestMsgEncoder();
#endif
    printf("\nAll tests passed.\n");
#else
    printf("\nSRP_CODER is not enabled.\n");
#endif
    return 0;
}
