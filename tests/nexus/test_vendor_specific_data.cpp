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

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "thread/mle_vendor_specific_tlv.hpp"
#include "thread/vendor_info.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kVendor1Oui24 = 0x641666; // MA-L OUI for "vendor 1"
static constexpr uint32_t kVendor2Oui24 = 0xa45e60; // MA-L OUI for "vendor 2"

// MA-M test bytes — OUI prefix + 4 extra ID bits in upper nibble of byte 3.
static const uint8_t kVendor1MaMBytes[] = {0x64, 0x16, 0x66, 0xa0};
// MA-S test bytes — OUI prefix + 12 extra ID bits in upper 12 bits of bytes 3-4.
static const uint8_t kVendor1MaSBytes[] = {0x64, 0x16, 0x66, 0x12, 0x30};

void VerifyOuiRoundTrip(Node &aNode)
{
    VendorInfo &info = aNode.Get<VendorInfo>();
    uint8_t     bytes[VendorInfo::kMaxOuiLength];

    Log("- MA-L set/get round-trip");
    SuccessOrQuit(info.SetOui(kVendor1Oui24));
    VerifyOrQuit(info.GetOui() == kVendor1Oui24);
    VerifyOrQuit(info.GetOuiLength() == VendorInfo::kOuiMaLLength);
    info.GetOuiBytes(bytes);
    VerifyOrQuit(bytes[0] == 0x64 && bytes[1] == 0x16 && bytes[2] == 0x66);

    Log("- MA-M set/get round-trip");
    SuccessOrQuit(info.SetOuiBytes(kVendor1MaMBytes, sizeof(kVendor1MaMBytes)));
    VerifyOrQuit(info.GetOuiLength() == VendorInfo::kOuiMaMLength);
    info.GetOuiBytes(bytes);
    VerifyOrQuit(memcmp(bytes, kVendor1MaMBytes, sizeof(kVendor1MaMBytes)) == 0);

    Log("- MA-S set/get round-trip");
    SuccessOrQuit(info.SetOuiBytes(kVendor1MaSBytes, sizeof(kVendor1MaSBytes)));
    VerifyOrQuit(info.GetOuiLength() == VendorInfo::kOuiMaSLength);
    info.GetOuiBytes(bytes);
    VerifyOrQuit(memcmp(bytes, kVendor1MaSBytes, sizeof(kVendor1MaSBytes)) == 0);

    Log("- MA-M with non-zero low nibble in last byte is rejected");
    {
        const uint8_t kBadMaM[] = {0x64, 0x16, 0x66, 0xa5};
        VerifyOrQuit(info.SetOuiBytes(kBadMaM, sizeof(kBadMaM)) == kErrorInvalidArgs);
    }

    Log("- SetOui() resets MA-S extension to MA-L even when 24-bit prefix matches");
    SuccessOrQuit(info.SetOuiBytes(kVendor1MaSBytes, sizeof(kVendor1MaSBytes)));
    VerifyOrQuit(info.GetOuiLength() == VendorInfo::kOuiMaSLength);
    SuccessOrQuit(info.SetOui(kVendor1Oui24));
    VerifyOrQuit(info.GetOuiLength() == VendorInfo::kOuiMaLLength);
}

// "Receive" handler — what a vendor's MLE handler does with an incoming Vendor Specific Data TLV.
// Decodes the OUI prefix, dispatches by recognized OUI, prints the payload, and silently ignores
// non-recognized OUIs (the spec-conformant multi-vendor coexistence behavior).
void HandleIncomingVendorTlv(const char *aReceiverName, uint32_t aRecognizedOui, Message &aMessage)
{
    OffsetRange valueRange;
    uint32_t    oui;

    SuccessOrQuit(Mle::VendorSpecificDataTlv::FindValue(aMessage, valueRange));
    SuccessOrQuit(Mle::VendorSpecificDataTlv::ReadOui24(aMessage, valueRange, oui));

    if (oui != aRecognizedOui)
    {
        Log("[%s] received TLV with unrecognized OUI 0x%06x — ignoring", aReceiverName, oui);
        return;
    }

    uint8_t  payload[16];
    uint16_t payloadLen = valueRange.GetLength() - Mle::VendorSpecificDataTlv::kOuiMaLBytes;

    VerifyOrQuit(payloadLen <= sizeof(payload));
    SuccessOrQuit(aMessage.Read(valueRange.GetOffset() + Mle::VendorSpecificDataTlv::kOuiMaLBytes, payload, payloadLen));

    Log("[%s] received TLV from OUI 0x%06x, payload (%u bytes):", aReceiverName, oui, payloadLen);
    for (uint16_t i = 0; i < payloadLen; i++)
    {
        Log("    [%u] = 0x%02x", i, payload[i]);
    }
}

void VerifyMultiVendorExchange(Node &aSender, Node &aReceiver)
{
    Message *message;
    uint32_t senderOui   = aSender.Get<VendorInfo>().GetOui();
    uint32_t receiverOui = aReceiver.Get<VendorInfo>().GetOui();

    // Sender builds a TLV with their OUI and a dummy 4-byte payload.
    const uint8_t kPayload[] = {0xde, 0xad, 0xbe, 0xef};

    message = aSender.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Mle::VendorSpecificDataTlv::AppendTo(*message, Mle::kCommandChildUpdateRequest, senderOui, kPayload,
                                                       sizeof(kPayload)));

    // Receiver inspects the same bytes — represents what its MLE handler would do.
    Log("[%s] sent VendorSpecificDataTlv from OUI 0x%06x (payload 0x%02x%02x%02x%02x)", aSender.GetName(), senderOui,
        kPayload[0], kPayload[1], kPayload[2], kPayload[3]);
    HandleIncomingVendorTlv(aReceiver.GetName(), receiverOui, *message);

    message->Free();
}

void VerifyAdvertisementRejection(Node &aNode)
{
    Message *message;
    uint16_t lengthBefore;

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);
    lengthBefore = message->GetLength();

    const uint8_t kPayload[] = {0x01, 0x02};

    Error error = Mle::VendorSpecificDataTlv::AppendTo(*message, Mle::kCommandAdvertisement, kVendor1Oui24, kPayload,
                                                      sizeof(kPayload));

    VerifyOrQuit(error == kErrorInvalidArgs);
    VerifyOrQuit(message->GetLength() == lengthBefore); // message must NOT have been mutated

    message->Free();
}

void VerifyChildIdRequestFragmentationGuard(Node &aNode)
{
    Message *message;
    uint16_t lengthBefore;

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    // Pre-fill the message with bytes representing the standard TLVs the caller has already appended.
    uint8_t filler[60];
    memset(filler, 0xa5, sizeof(filler));
    SuccessOrQuit(message->AppendBytes(filler, sizeof(filler)));
    lengthBefore = message->GetLength();

    const uint8_t kBigPayload[40] = {};

    // Budget that current-length + (TLV header 2 + OUI 3 + payload 40 = 45) would exceed.
    Error error = Mle::VendorSpecificDataTlv::AppendTo(*message, Mle::kCommandChildIdRequest, kVendor1Oui24, kBigPayload,
                                                      sizeof(kBigPayload), /* aSingleFrameMaxLength */ 80);

    VerifyOrQuit(error == kErrorNoBufs);
    VerifyOrQuit(message->GetLength() == lengthBefore); // message must NOT have been mutated

    // Same call with a generous budget: must succeed.
    error = Mle::VendorSpecificDataTlv::AppendTo(*message, Mle::kCommandChildIdRequest, kVendor1Oui24, kBigPayload,
                                                 sizeof(kBigPayload), /* aSingleFrameMaxLength */ 200);
    SuccessOrQuit(error);
    VerifyOrQuit(message->GetLength() > lengthBefore);

    message->Free();
}

void TestVendorSpecificData(void)
{
    Core nexus;

    Node &node1 = nexus.CreateNode();
    Node &node2 = nexus.CreateNode();

    node1.SetName("VENDOR1");
    node2.SetName("VENDOR2");

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("OUI set/get round-trip (MA-L / MA-M / MA-S) on VENDOR1");
    SuccessOrQuit(node1.Get<VendorInfo>().SetOui(kVendor1Oui24));
    VerifyOuiRoundTrip(node1);

    // Re-establish each node's OUI for the exchange below.
    SuccessOrQuit(node1.Get<VendorInfo>().SetOui(kVendor1Oui24));
    SuccessOrQuit(node2.Get<VendorInfo>().SetOui(kVendor2Oui24));

    Log("---------------------------------------------------------------------------------------");
    Log("Multi-vendor exchange — VENDOR1 -> VENDOR2 (OUI mismatch, must be silently ignored)");
    VerifyMultiVendorExchange(node1, node2);

    Log("---------------------------------------------------------------------------------------");
    Log("Multi-vendor exchange — VENDOR2 -> VENDOR1 (OUI mismatch, must be silently ignored)");
    VerifyMultiVendorExchange(node2, node1);

    Log("---------------------------------------------------------------------------------------");
    Log("Same-vendor exchange — VENDOR1 -> VENDOR1 (OUI match, payload is delivered)");
    VerifyMultiVendorExchange(node1, node1);

    Log("---------------------------------------------------------------------------------------");
    Log("Spec rule: MUST NOT append Vendor Specific Data TLV to Link Advertisement");
    VerifyAdvertisementRejection(node1);

    Log("---------------------------------------------------------------------------------------");
    Log("Spec rule: Child ID Request — TLV skipped when single-frame budget exceeded");
    VerifyChildIdRequestFragmentationGuard(node1);

    Log("All tests passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestVendorSpecificData();
    return 0;
}
