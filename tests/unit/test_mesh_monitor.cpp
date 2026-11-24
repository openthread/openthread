/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <openthread/config.h>

#include "common/message.hpp"
#include "instance/instance.hpp"
#include "thread/mesh_monitor_types.hpp"

#include "test_util.h"
#include "test_util.hpp"

#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE || OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE

namespace ot {

namespace MeshMonitor {

// The wire format tests below hard-code the TLV type values.

static_assert(Tlv::kExtAddress == 0, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kMode == 1, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kMacLinkErrorRatesTx == 8, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kMlEid == 9, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kVendorSwVersion == 16, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kIp6LinkLocalAddressList == 18, "TLV type value changed, update the wire format tests");
static_assert(Tlv::kLinkMarginOut == 23, "TLV type value changed, update the wire format tests");
static_assert(TlvSet::kMaskSize == 4, "kMaskSize changed, update the wire format tests");

static Message *NewMessage(Instance &aInstance)
{
    Message *message = aInstance.Get<MessagePool>().Allocate(Message::kTypeIp6);

    VerifyOrQuit(message != nullptr);

    return message;
}

static Message *NewMessage(Instance &aInstance, const uint8_t *aBytes, uint16_t aLength)
{
    Message *message = NewMessage(aInstance);

    SuccessOrQuit(message->AppendBytes(aBytes, aLength));

    return message;
}

// Encodes `aSet` and checks the literal bytes on the wire, then decodes those bytes again and checks that the
// same set is recovered.
static void VerifyWireFormat(Instance      &aInstance,
                             const TlvSet  &aSet,
                             const uint8_t *aExpectedBytes,
                             uint16_t       aExpectedLength,
                             uint8_t        aExpectedSetCount)
{
    Message *message  = NewMessage(aInstance);
    uint8_t  setCount = 0xff;
    uint16_t offset   = 0;
    TlvSet   decoded;

    SuccessOrQuit(aSet.AppendTo(*message, setCount));

    VerifyOrQuit(setCount == aExpectedSetCount);
    VerifyOrQuit(message->GetLength() == aExpectedLength);
    VerifyOrQuit(message->CompareBytes(0, aExpectedBytes, aExpectedLength));

    SuccessOrQuit(decoded.ReadFrom(*message, offset, setCount));

    VerifyOrQuit(offset == aExpectedLength);
    VerifyOrQuit(decoded == aSet);

    message->Free();
}

void TestTlvSetWireFormat(void)
{
    Instance *instance = testInitInstance();

    VerifyOrQuit(instance != nullptr);

    printf("TestTlvSetWireFormat\n");

    // Each Request Set is a header byte (Offset in the upper nibble, Length in the lower nibble) followed by
    // `Length` mask bytes. Mask byte `k` covers TLVs `8k` to `8k + 7`, the byte covering the lowest TLVs is
    // sent first, and TLV `N` is bit `0x80 >> (N % 8)` of its byte.

    // Empty set: nothing is written.
    {
        static constexpr uint8_t kExpectedSetCount = 0;
        const TlvSet             set;

        VerifyWireFormat(*instance, set, nullptr, 0, kExpectedSetCount);
    }

    // Single TLV, lowest bit.
    {
        static const uint8_t     kBytes[]          = {0x01, 0x80};
        static constexpr uint8_t kExpectedSetCount = 1;
        const TlvSet             set(Tlv::kExtAddress);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // Single TLV, highest supported TLV, lowest bit on wire: non-zero Offset.
    {
        static const uint8_t     kBytes[]          = {0x21, 0x01};
        static constexpr uint8_t kExpectedSetCount = 1;
        const TlvSet             set(Tlv::kLinkMarginOut);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // TLVs 0, 1 and 8: two consecutive bytes in a single Request Set.
    {
        static const uint8_t     kBytes[]          = {0x02, 0xc0, 0x80};
        static constexpr uint8_t kExpectedSetCount = 1;
        const TlvSet             set(Tlv::kExtAddress, Tlv::kMode, Tlv::kMacLinkErrorRatesTx);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // TLVs 0, 9 and 18: three consecutive bytes.
    {
        static const uint8_t     kBytes[]          = {0x03, 0x80, 0x40, 0x20};
        static constexpr uint8_t kExpectedSetCount = 1;
        const TlvSet             set(Tlv::kExtAddress, Tlv::kMlEid, Tlv::kIp6LinkLocalAddressList);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // TLVs 0, 1 and 16: the all-zero byte in between is skipped, giving two Request Sets.
    {
        static const uint8_t     kBytes[]          = {0x01, 0xc0, 0x21, 0x80};
        static constexpr uint8_t kExpectedSetCount = 2;
        const TlvSet             set(Tlv::kExtAddress, Tlv::kMode, Tlv::kVendorSwVersion);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // All supported TLVs: one Request Set covering the bytes in use.
    {
        static const uint8_t     kBytes[]          = {0x03, 0xff, 0xff, 0xff};
        static constexpr uint8_t kExpectedSetCount = 1;
        const TlvSet             set(TlvSet::kAllSupportedTlvMask);

        VerifyWireFormat(*instance, set, kBytes, sizeof(kBytes), kExpectedSetCount);
    }

    // Decode at a non-zero message offset, with Request Sets in descending Offset order and trailing data.
    {
        static const uint8_t     kBytes[]  = {0xaa, 0xbb, 0x21, 0x80, 0x01, 0xc0, 0xcc};
        static constexpr uint8_t kSetCount = 2;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 2;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == 6);
        VerifyOrQuit(set == TlvSet(Tlv::kExtAddress, Tlv::kMode, Tlv::kVendorSwVersion));

        message->Free();
    }

    // Bits of unknown TLVs and bytes beyond the mask are consumed but ignored.
    {
        static const uint8_t     kBytes[]  = {0x22, 0x01, 0xff, 0x51, 0xff};
        static constexpr uint8_t kSetCount = 2;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 0;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == sizeof(kBytes));
        VerifyOrQuit(set == TlvSet(Tlv::kLinkMarginOut));

        message->Free();
    }

    // Successful decode replaces the previous contents of the set.
    {
        static const uint8_t     kBytes[]  = {0x01, 0x80};
        static constexpr uint8_t kSetCount = 1;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 0;
        TlvSet   set(Tlv::kMlEid);

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == sizeof(kBytes));
        VerifyOrQuit(set == TlvSet(Tlv::kExtAddress));

        message->Free();
    }

    // Truncated Request Set: fails and leaves the set and offset untouched.
    {
        static const uint8_t     kBytes[]  = {0x02, 0xc0};
        static constexpr uint8_t kSetCount = 1;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 0;
        TlvSet   set(Tlv::kMlEid);

        VerifyOrQuit(set.ReadFrom(*message, offset, kSetCount) == kErrorParse);

        VerifyOrQuit(offset == 0);
        VerifyOrQuit(set == TlvSet(Tlv::kMlEid));

        message->Free();
    }

    testFreeInstance(instance);
}

void TestTlvSetLength16(void)
{
    // A Request Set of 16 mask bytes does not fit the 4-bit Length field and is encoded as Length 0 in the lower
    // nibble of the header byte, regardless of the Offset in the upper nibble.

    static constexpr uint8_t kNumMaskBytes = 16;

    static const uint8_t kBytes[] = {0x00,                                           // Offset 0, Length 16
                                     0xc0, 0x80, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, // Mask bytes 0-7
                                     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Mask bytes 8-15
                                     0x21, 0x80};                                    // Offset 2, Length 1

    static const uint8_t kBytesOffset1[] = {0x10,                                           // Offset 1, Length 16
                                            0xc0, 0x80, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, // Mask bytes 1-8
                                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Mask bytes 9-16
                                            0x21, 0x40};                                    // Offset 2, Length 1

    Instance *instance = testInitInstance();

    VerifyOrQuit(instance != nullptr);

    printf("TestTlvSetLength16\n");

    // All 16 mask bytes are consumed, the ones covering unknown TLVs are ignored.
    {
        static constexpr uint8_t kSetCount = 1;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 0;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == 1 + kNumMaskBytes);
        VerifyOrQuit(set == TlvSet(Tlv::kExtAddress, Tlv::kMode, Tlv::kMacLinkErrorRatesTx));

        message->Free();
    }

    // A following Request Set is located right after the 16 mask bytes.
    {
        static constexpr uint8_t kSetCount = 2;

        Message *message = NewMessage(*instance, kBytes, sizeof(kBytes));
        uint16_t offset  = 0;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == sizeof(kBytes));
        VerifyOrQuit(set == TlvSet(Tlv::kExtAddress, Tlv::kMode, Tlv::kMacLinkErrorRatesTx, Tlv::kVendorSwVersion));

        message->Free();
    }

    // Fewer than 16 mask bytes after a 0x00 header is a parse failure, set remains unchanged.
    {
        static constexpr uint8_t kSetCount = 1;

        // Message holds only the first kNumMaskBytes bytes: the header byte + 15 of the 16 required mask bytes.
        Message *message = NewMessage(*instance, kBytes, kNumMaskBytes);
        uint16_t offset  = 0;
        TlvSet   set(Tlv::kMlEid);

        VerifyOrQuit(set.ReadFrom(*message, offset, kSetCount) == kErrorParse);

        VerifyOrQuit(offset == 0);
        VerifyOrQuit(set == TlvSet(Tlv::kMlEid));

        message->Free();
    }

    // A non-zero Offset with Length 16 (encoded as 0) still consumes 16 mask bytes. Mask bytes beyond the TlvSet
    // and bits of unknown TLVs are ignored.
    {
        static constexpr uint8_t kSetCount = 1;

        Message *message = NewMessage(*instance, kBytesOffset1, sizeof(kBytesOffset1));
        uint16_t offset  = 0;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        // Message offset advances by 1 header byte + 16 mask bytes, independent of the Request Set Offset.
        VerifyOrQuit(offset == 1 + kNumMaskBytes);
        VerifyOrQuit(set == TlvSet(Tlv::kMacLinkErrorRatesTx, Tlv::kMlEid, Tlv::kVendorSwVersion));

        message->Free();
    }

    // A following Request Set is located right after the 16 mask bytes and overrides mask byte 2.
    {
        static constexpr uint8_t kSetCount = 2;

        Message *message = NewMessage(*instance, kBytesOffset1, sizeof(kBytesOffset1));
        uint16_t offset  = 0;
        TlvSet   set;

        SuccessOrQuit(set.ReadFrom(*message, offset, kSetCount));

        VerifyOrQuit(offset == sizeof(kBytesOffset1));
        VerifyOrQuit(set == TlvSet(Tlv::kMacLinkErrorRatesTx, Tlv::kMlEid, Tlv::kVendorAppUrl));

        message->Free();
    }

    // Fewer than 16 mask bytes after a 0x10 header is a parse failure, set remains unchanged.
    {
        static constexpr uint8_t kSetCount = 1;

        // Message holds only the first kNumMaskBytes bytes: the header byte + 15 of the 16 required mask bytes.
        Message *message = NewMessage(*instance, kBytesOffset1, kNumMaskBytes);
        uint16_t offset  = 0;
        TlvSet   set(Tlv::kMlEid);

        VerifyOrQuit(set.ReadFrom(*message, offset, kSetCount) == kErrorParse);

        VerifyOrQuit(offset == 0);
        VerifyOrQuit(set == TlvSet(Tlv::kMlEid));

        message->Free();
    }

    testFreeInstance(instance);
}

} // namespace MeshMonitor
} // namespace ot

#endif // OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE || OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE || OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE
    ot::MeshMonitor::TestTlvSetWireFormat();
    ot::MeshMonitor::TestTlvSetLength16();
    printf("All tests passed\n");
#else
    printf("Mesh monitor is not enabled\n");
#endif

    return 0;
}
