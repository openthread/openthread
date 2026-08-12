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

#include <openthread/config.h>

#include <string.h>

#include <openthread/dataset.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "meshcop/dataset.hpp"

namespace ot {
namespace MeshCoP {

void TestDataset(void)
{
    static const uint8_t kTlvBytes[] = {
        0x0e, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x0f, 0x35, 0x06, 0x00,
        0x04, 0x00, 0x1f, 0xff, 0xe0, 0x02, 0x08, 0x1d, 0xe5, 0xbf, 0xec, 0xd5, 0x16, 0x5b, 0x8f, 0x07, 0x08, 0xfd,
        0xe2, 0x1f, 0x0c, 0x8a, 0x13, 0xe8, 0xe7, 0x05, 0x10, 0xea, 0xf9, 0x14, 0x9f, 0xdc, 0x73, 0x78, 0x77, 0x06,
        0x98, 0xd5, 0x91, 0x80, 0x22, 0x19, 0x58, 0x03, 0x0f, 0x4f, 0x70, 0x65, 0x6e, 0x54, 0x68, 0x72, 0x65, 0x61,
        0x64, 0x2d, 0x61, 0x61, 0x63, 0x33, 0x01, 0x02, 0xfa, 0xce, 0x04, 0x10, 0x2e, 0xaa, 0xe2, 0x94, 0x84, 0x38,
        0x8e, 0x31, 0x19, 0x58, 0x1a, 0x7b, 0x5a, 0x94, 0x8c, 0x07, 0x0c, 0x04, 0x02, 0xa0, 0xf7, 0xf8,
    };

    static const otNetworkKey kNetworkKey = {
        {0xea, 0xf9, 0x14, 0x9f, 0xdc, 0x73, 0x78, 0x77, 0x06, 0x98, 0xd5, 0x91, 0x80, 0x22, 0x19, 0x58}};

    static const otNetworkKey kNewNetworkKey = {
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}};

    static const uint8_t kDuplicateChannels[] = {
        0x00, 0x03, 0x00, 0x00, 0x1a, 0x00, 0x03, 0x00, 0x00, 0x1a,
    };

    static const Tlv::Type kDatasetTlvTypes[] = {
        Tlv::kChannel,    Tlv::kPanId,           Tlv::kExtendedPanId,  Tlv::kNetworkName,     Tlv::kPskc,
        Tlv::kNetworkKey, Tlv::kMeshLocalPrefix, Tlv::kSecurityPolicy, Tlv::kActiveTimestamp,
    };

    Dataset       dataset;
    Dataset       dataset2;
    Dataset::Tlvs datasetTlvs;
    Dataset::Info datasetInfo;
    uint16_t      panId;
    NetworkKey    networkKey;

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));

    VerifyOrQuit(dataset.GetLength() == sizeof(kTlvBytes));

    SuccessOrQuit(dataset.ValidateTlvs());

    for (Tlv::Type tlvType : kDatasetTlvTypes)
    {
        VerifyOrQuit(dataset.ContainsTlv(tlvType));
    }

    // Converting to `Dataset::Tlvs`

    dataset.ConvertTo(datasetTlvs);
    VerifyOrQuit(datasetTlvs.mLength == sizeof(kTlvBytes));
    VerifyOrQuit(memcmp(datasetTlvs.mTlvs, kTlvBytes, sizeof(kTlvBytes)) == 0);

    // Converting to `Dataset::Info`

    dataset.ConvertTo(datasetInfo);

    VerifyOrQuit(datasetInfo.mComponents.mIsActiveTimestampPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsNetworkKeyPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsNetworkNamePresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsExtendedPanIdPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsMeshLocalPrefixPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsPanIdPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsChannelPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsPskcPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsSecurityPolicyPresent);
    VerifyOrQuit(datasetInfo.mComponents.mIsChannelMaskPresent);
    VerifyOrQuit(!datasetInfo.mComponents.mIsPendingTimestampPresent);
    VerifyOrQuit(!datasetInfo.mComponents.mIsDelayPresent);

    VerifyOrQuit(datasetInfo.mPanId == 0xface);
    VerifyOrQuit(AsCoreType(&datasetInfo.mNetworkKey) == AsCoreType(&kNetworkKey));

    // Finding, reading TLVs

    VerifyOrQuit(dataset.Contains<PanIdTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kPanId) != nullptr);
    SuccessOrQuit(dataset.Read<PanIdTlv>(panId));
    VerifyOrQuit(panId == 0xface);

    VerifyOrQuit(dataset.Contains<NetworkKeyTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kNetworkKey) != nullptr);
    SuccessOrQuit(dataset.Read<NetworkKeyTlv>(networkKey));
    VerifyOrQuit(networkKey == AsCoreType(&kNetworkKey));

    // Change PAN ID TLV

    SuccessOrQuit(dataset.Write<PanIdTlv>(0xcafe));

    SuccessOrQuit(dataset.ValidateTlvs());

    VerifyOrQuit(dataset.Contains<PanIdTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kPanId) != nullptr);
    SuccessOrQuit(dataset.Read<PanIdTlv>(panId));
    VerifyOrQuit(panId == 0xcafe);

    for (Tlv::Type tlvType : kDatasetTlvTypes)
    {
        VerifyOrQuit(dataset.ContainsTlv(tlvType));
    }

    // Change Network Key TLV

    SuccessOrQuit(dataset.Write<NetworkKeyTlv>(AsCoreType(&kNewNetworkKey)));
    VerifyOrQuit(dataset.Contains<NetworkKeyTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kNetworkKey) != nullptr);
    SuccessOrQuit(dataset.Read<NetworkKeyTlv>(networkKey));
    VerifyOrQuit(networkKey == AsCoreType(&kNewNetworkKey));

    for (Tlv::Type tlvType : kDatasetTlvTypes)
    {
        VerifyOrQuit(dataset.ContainsTlv(tlvType));
    }

    // Remove PAN ID TLV

    dataset.RemoveTlv(Tlv::kPanId);
    VerifyOrQuit(!dataset.Contains<PanIdTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kPanId) == nullptr);
    VerifyOrQuit(dataset.Read<PanIdTlv>(panId) == kErrorNotFound);

    SuccessOrQuit(dataset.ValidateTlvs());

    // Invalid datasets

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes) - 1));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kDuplicateChannels, sizeof(kDuplicateChannels)));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kDuplicateChannels, sizeof(kDuplicateChannels) / 2));
    SuccessOrQuit(dataset.ValidateTlvs());

    // Combining/Merging TLVs from two Datasets.

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));

    datasetInfo.Clear();
    datasetInfo.mComponents.mIsPanIdPresent      = true;
    datasetInfo.mComponents.mIsNetworkKeyPresent = true;
    datasetInfo.mPanId                           = 0xcafe;
    datasetInfo.mNetworkKey                      = kNewNetworkKey;

    dataset2.SetFrom(datasetInfo);
    SuccessOrQuit(dataset2.ValidateTlvs());

    SuccessOrQuit(dataset.WriteTlvsFrom(dataset2));

    SuccessOrQuit(dataset.ValidateTlvs());

    VerifyOrQuit(dataset.Contains<PanIdTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kPanId) != nullptr);
    SuccessOrQuit(dataset.Read<PanIdTlv>(panId));
    VerifyOrQuit(panId == 0xcafe);

    VerifyOrQuit(dataset.Contains<NetworkKeyTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kNetworkKey) != nullptr);
    SuccessOrQuit(dataset.Read<NetworkKeyTlv>(networkKey));
    VerifyOrQuit(networkKey == AsCoreType(&kNewNetworkKey));

    // Combining/Merging TLVs from two Datasets (using `Dataset::Info`).

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));

    SuccessOrQuit(dataset.WriteTlvsFrom(datasetInfo));

    SuccessOrQuit(dataset.ValidateTlvs());

    VerifyOrQuit(dataset.Contains<PanIdTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kPanId) != nullptr);
    SuccessOrQuit(dataset.Read<PanIdTlv>(panId));
    VerifyOrQuit(panId == 0xcafe);

    VerifyOrQuit(dataset.Contains<NetworkKeyTlv>());
    VerifyOrQuit(dataset.FindTlv(Tlv::kNetworkKey) != nullptr);
    SuccessOrQuit(dataset.Read<NetworkKeyTlv>(networkKey));
    VerifyOrQuit(networkKey == AsCoreType(&kNewNetworkKey));

    // Append TLVs

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));
    VerifyOrQuit(dataset.GetLength() == sizeof(kTlvBytes));
    VerifyOrQuit(memcmp(dataset.GetBytes(), kTlvBytes, sizeof(kTlvBytes)) == 0);

    SuccessOrQuit(dataset.AppendTlvsFrom(kTlvBytes, sizeof(kTlvBytes)));
    VerifyOrQuit(dataset.GetLength() == 2 * sizeof(kTlvBytes));
    VerifyOrQuit(memcmp(dataset.GetBytes(), kTlvBytes, sizeof(kTlvBytes)) == 0);
    VerifyOrQuit(memcmp(dataset.GetBytes() + sizeof(kTlvBytes), kTlvBytes, sizeof(kTlvBytes)) == 0);

    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    // Validate `IsSubsetOf()`

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));

    datasetInfo.Clear();
    datasetInfo.mComponents.mIsPanIdPresent      = true;
    datasetInfo.mComponents.mIsNetworkKeyPresent = true;
    datasetInfo.mPanId                           = 0xface;
    datasetInfo.mNetworkKey                      = kNetworkKey;

    dataset2.SetFrom(datasetInfo);

    SuccessOrQuit(dataset2.ValidateTlvs());
    SuccessOrQuit(dataset.ValidateTlvs());

    VerifyOrQuit(dataset2.IsSubsetOf(dataset));
    VerifyOrQuit(!dataset.IsSubsetOf(dataset2));

    datasetInfo.mComponents.mIsActiveTimestampPresent  = true;
    datasetInfo.mComponents.mIsPendingTimestampPresent = true;
    datasetInfo.mComponents.mIsDelayPresent            = true;
    datasetInfo.mActiveTimestamp.mSeconds              = 0xffff;
    datasetInfo.mPendingTimestamp.mSeconds             = 0x1000;
    datasetInfo.mDelay                                 = 5000;
    dataset2.SetFrom(datasetInfo);

    VerifyOrQuit(dataset2.IsSubsetOf(dataset));
    VerifyOrQuit(!dataset.IsSubsetOf(dataset2));

    datasetInfo.mPanId = 0xcafe;
    dataset2.SetFrom(datasetInfo);

    VerifyOrQuit(!dataset2.IsSubsetOf(dataset));
    VerifyOrQuit(!dataset.IsSubsetOf(dataset2));

    // Validate `Equals()`

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes)));
    SuccessOrQuit(dataset2.SetFrom(kTlvBytes, sizeof(kTlvBytes)));

    VerifyOrQuit(dataset.Equals(dataset2));
    VerifyOrQuit(dataset2.Equals(dataset));

    // Order of TLVs should not matter for `Equals()`
    ChannelTlvValue channel;
    channel.SetChannelAndPage(11);

    dataset.Clear();
    SuccessOrQuit(dataset.Write<PanIdTlv>(0xface));
    SuccessOrQuit(dataset.Write<ChannelTlv>(channel));

    dataset2.Clear();
    SuccessOrQuit(dataset2.Write<ChannelTlv>(channel));
    SuccessOrQuit(dataset2.Write<PanIdTlv>(0xface));

    VerifyOrQuit(dataset.Equals(dataset2));
    VerifyOrQuit(dataset2.Equals(dataset));

    // Different TLVs

    dataset.Clear();
    SuccessOrQuit(dataset.Write<PanIdTlv>(0xface));

    dataset2.Clear();
    SuccessOrQuit(dataset2.Write<PanIdTlv>(0xface));
    SuccessOrQuit(dataset2.Write<ChannelTlv>(channel));

    VerifyOrQuit(!dataset.Equals(dataset2));
    VerifyOrQuit(!dataset2.Equals(dataset));

    // Validate `Equals()` with unknown TLVs

    {
        const uint8_t   kUnknownTlvValue[]  = {0x01, 0x02, 0x03, 0x04};
        const uint8_t   kUnknownTlvValue2[] = {0x01, 0x02, 0x03, 0x05};
        const Tlv::Type kUnknownTlvType     = static_cast<Tlv::Type>(222);

        dataset.Clear();
        SuccessOrQuit(dataset.Write<PanIdTlv>(0xface));
        SuccessOrQuit(dataset.WriteTlv(kUnknownTlvType, kUnknownTlvValue, sizeof(kUnknownTlvValue)));

        dataset2.Clear();
        SuccessOrQuit(dataset2.WriteTlv(kUnknownTlvType, kUnknownTlvValue, sizeof(kUnknownTlvValue)));
        SuccessOrQuit(dataset2.Write<PanIdTlv>(0xface));

        VerifyOrQuit(dataset.Equals(dataset2));
        VerifyOrQuit(dataset2.Equals(dataset));

        // Different values for the same unknown TLV type
        SuccessOrQuit(dataset2.WriteTlv(kUnknownTlvType, kUnknownTlvValue2, sizeof(kUnknownTlvValue2)));
        VerifyOrQuit(!dataset.Equals(dataset2));
        VerifyOrQuit(!dataset2.Equals(dataset));
    }
}

// `otDatasetAffectsConnectivity()` builds its working `Dataset` from raw caller
// bytes via `SetFrom()`, which only checks the overall buffer length, not that each
// TLV's declared length matches what its type requires. Without a `ValidateTlvs()`
// call, a `NetworkKeyTlv` (type 5, needs 16 bytes) declared with length 0 lets
// `Read<NetworkKeyTlv>()` read 16 bytes past the TLV header regardless -- past the
// `mTlvs` array and the `Dataset` object itself. This checks the malformed case is
// now safely rejected, and that a validly-formed dataset containing a real 16-byte
// `NetworkKeyTlv` is still accepted (no regression for legitimate callers).
void TestDatasetAffectsConnectivityRejectsMalformedTlvs(void)
{
    otInstance *instance = testInitInstance();

    {
        // Malformed: type 222 filler TLV (length 250) fills bytes [0..251], then a
        // NetworkKeyTlv (type 5) with declared length 0 sits at [252..253]. The
        // 254-byte buffer is internally consistent (every TLV's own header+declared
        // length fits), but the NetworkKeyTlv's declared length is short of the 16
        // bytes `NetworkKeyTlv::ValueType` requires.
        otOperationalDatasetTlvs tlvs;

        tlvs.mLength = 254;
        memset(tlvs.mTlvs, 0x41, sizeof(tlvs.mTlvs));
        tlvs.mTlvs[0]   = 222;
        tlvs.mTlvs[1]   = 250;
        tlvs.mTlvs[252] = OT_MESHCOP_TLV_NETWORKKEY;
        tlvs.mTlvs[253] = 0;

        VerifyOrQuit(!otDatasetAffectsConnectivity(instance, &tlvs));
    }

    {
        // Well-formed: a single, correctly-sized `NetworkKeyTlv` (type 5, length 16).
        otOperationalDatasetTlvs tlvs;

        tlvs.mTlvs[0] = OT_MESHCOP_TLV_NETWORKKEY;
        tlvs.mTlvs[1] = sizeof(otNetworkKey);
        memset(&tlvs.mTlvs[2], 0x11, sizeof(otNetworkKey));
        tlvs.mLength = 2 + sizeof(otNetworkKey);

        // Must not crash; the returned value only depends on whether this key
        // differs from the instance's own, which this test does not assert.
        otDatasetAffectsConnectivity(instance, &tlvs);
    }
}

} // namespace MeshCoP
} // namespace ot

int main(void)
{
    ot::MeshCoP::TestDataset();
    ot::MeshCoP::TestDatasetAffectsConnectivityRejectsMalformedTlvs();

    printf("All tests passed\n");
    return 0;
}
