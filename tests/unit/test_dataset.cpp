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

#include "test_platform.h"
#include "test_util.hpp"

#include "meshcop/dataset.hpp"

namespace ot {
namespace MeshCoP {

template <size_t kSize> Error SetFromTlvsAndValidate(const uint8_t (&aTlvs)[kSize])
{
    Dataset dataset;

    // `SetFrom()` takes the length as `uint8_t`, so guard against an array
    // that would be silently narrowed when passed to it.
    static_assert(kSize <= Dataset::kMaxLength, "aTlvs is too long for a Dataset");

    SuccessOrQuit(dataset.SetFrom(aTlvs, kSize));

    return dataset.ValidateTlvs();
}

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

    // PAN ID TLV - the broadcast PAN ID is not allowed.
    static const uint8_t kInvalidPanId[] = {0x01, 0x02, 0xff, 0xff};
    static const uint8_t kValidPanId[]   = {0x01, 0x02, 0xff, 0xfe};

    // Extended PAN ID TLV - all-zeros and all-ones are disallowed.
    static const uint8_t kInvalidExtPanIdAllZeros[] = {0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t kInvalidExtPanIdAllOnes[]  = {0x02, 0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static const uint8_t kValidExtPanId[]           = {0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    // Mesh-Local Prefix TLV - must be a locally assigned ULA prefix (`fd00::/8`).
    static const uint8_t kInvalidMeshLocalPrefix[]     = {0x07, 0x08, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t kInvalidMeshLocalPrefixLBit[] = {0x07, 0x08, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t kValidMeshLocalPrefix[]       = {0x07, 0x08, 0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00};

    // Network Name TLV - 1 to 16 bytes of valid UTF-8 without control characters.
    // A zero-length name is valid only when `ALLOW_EMPTY_NETWORK_NAME` is enabled.
    static const uint8_t kEmptyNetworkName[]          = {0x03, 0x00};
    static const uint8_t kInvalidTooLongNetworkName[] = {0x03, 0x11, 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
                                                         'a',  'a',  'a', 'a', 'a', 'a', 'a', 'a', 'a'};
    static const uint8_t kInvalidUtf8NetworkName[]    = {0x03, 0x03, 'a', 0x00, 'b'};
    static const uint8_t kValidMaxLenNetworkName[]    = {0x03, 0x10, 'a', 'a', 'a', 'a', 'a', 'a', 'a',
                                                         'a',  'a',  'a', 'a', 'a', 'a', 'a', 'a', 'a'};

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

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, 1));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kTlvBytes, sizeof(kTlvBytes) - 1));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kDuplicateChannels, sizeof(kDuplicateChannels) / 2 + 1));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kDuplicateChannels, sizeof(kDuplicateChannels)));
    VerifyOrQuit(dataset.ValidateTlvs() == kErrorParse);

    SuccessOrQuit(dataset.SetFrom(kDuplicateChannels, sizeof(kDuplicateChannels) / 2));
    SuccessOrQuit(dataset.ValidateTlvs());

    // Invalid TLV values

    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidPanId) == kErrorParse);
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidExtPanIdAllZeros) == kErrorParse);
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidExtPanIdAllOnes) == kErrorParse);
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidMeshLocalPrefix) == kErrorParse);
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidMeshLocalPrefixLBit) == kErrorParse);
#if OPENTHREAD_CONFIG_ALLOW_EMPTY_NETWORK_NAME
    SuccessOrQuit(SetFromTlvsAndValidate(kEmptyNetworkName));
#else
    VerifyOrQuit(SetFromTlvsAndValidate(kEmptyNetworkName) == kErrorParse);
#endif
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidTooLongNetworkName) == kErrorParse);
    VerifyOrQuit(SetFromTlvsAndValidate(kInvalidUtf8NetworkName) == kErrorParse);

    // Valid variants of the same TLVs

    SuccessOrQuit(SetFromTlvsAndValidate(kValidPanId));
    SuccessOrQuit(SetFromTlvsAndValidate(kValidExtPanId));
    SuccessOrQuit(SetFromTlvsAndValidate(kValidMeshLocalPrefix));
    SuccessOrQuit(SetFromTlvsAndValidate(kValidMaxLenNetworkName));

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

void TestSecurityPolicy(void)
{
    // Default Security Policy flags. All reserved bits are 1.
    static const uint8_t kDefaultFlags[] = {0xf7, 0xf8};

    // Default flags. All reserved bits are 0.
    static const uint8_t kAllReservedClearedFlags[] = {0xf7, 0xc0};

    // Default flags. One reserved bit is 0 in each array.
    static const uint8_t kReservedBit10ClearedFlags[] = {0xf7, 0xd8};
    static const uint8_t kReservedBit11ClearedFlags[] = {0xf7, 0xe8};
    static const uint8_t kReservedBit12ClearedFlags[] = {0xf7, 0xf0};

    // Default flags with reserved bit 11 cleared and a non-zero
    // Version Threshold (0x05) in the low bits. Decode and encode these
    // flags. The reserved bits and the Version Threshold field must not
    // change each other.
    static const uint8_t kReservedAndVersionFlags[] = {0xf7, 0xed};

    // Default flags with all reserved bits cleared and the maximum
    // Version Threshold in the low bits. The field is 3 bits wide, so
    // `0x07` is its maximum value.
    static const uint8_t kVersionThresholdMaxFlags[] = {0xf7, 0xc7};

    // Default flags with all reserved bits cleared, and one extra byte.
    static const uint8_t kExtraByteFlags[] = {0xf7, 0xc0, 0xff};

    // Test flags, and the expected value of each `mReservedBit*Cleared`
    // field and of `mVersionThresholdForRouting` after `SetFlags()`
    // decodes the flags.
    struct ClearedReservedTestCase
    {
        const uint8_t *mFlags;
        bool           mBit10Cleared;
        bool           mBit11Cleared;
        bool           mBit12Cleared;
        uint8_t        mVersionThreshold;
    };

    static const ClearedReservedTestCase kClearedReservedTestCases[] = {
        {kDefaultFlags, false, false, false, 0},
        {kAllReservedClearedFlags, true, true, true, 0},
        {kReservedBit10ClearedFlags, true, false, false, 0},
        {kReservedBit11ClearedFlags, false, true, false, 0},
        {kReservedBit12ClearedFlags, false, false, true, 0},
        {kReservedAndVersionFlags, false, true, false, 5},
        {kVersionThresholdMaxFlags, true, true, true, 7},
    };

    // Mask of reserved bits 10-12 in the second flags byte. These are
    // the bits of `kReservedBit10Mask`, `kReservedBit11Mask`, and
    // `kReservedBit12Mask` in `SecurityPolicy`.
    static constexpr uint8_t kReservedBitsMask = (1 << 5) | (1 << 4) | (1 << 3);

    SecurityPolicy policy;
    uint8_t        flags[2];

    // A default policy must encode all reserved bits as `1`.

    policy.SetToDefault();
    policy.GetFlags(flags, sizeof(flags));
    VerifyOrQuit(memcmp(flags, kDefaultFlags, sizeof(flags)) == 0);

    // Decode the flags of each test case. Each reserved bit must map to
    // its `mReservedBit*Cleared` field. The Version Threshold must map
    // to `mVersionThresholdForRouting`. Encode the policy again. The
    // output must equal the input.

    for (const ClearedReservedTestCase &testCase : kClearedReservedTestCases)
    {
        policy.SetFlags(testCase.mFlags, sizeof(flags));
        VerifyOrQuit(policy.mReservedBit10Cleared == testCase.mBit10Cleared);
        VerifyOrQuit(policy.mReservedBit11Cleared == testCase.mBit11Cleared);
        VerifyOrQuit(policy.mReservedBit12Cleared == testCase.mBit12Cleared);
        VerifyOrQuit(policy.mVersionThresholdForRouting == testCase.mVersionThreshold);

        memset(flags, 0, sizeof(flags));
        policy.GetFlags(flags, sizeof(flags));
        VerifyOrQuit(memcmp(flags, testCase.mFlags, sizeof(flags)) == 0);
    }

    // Decode a Thread 1.1 single-byte flags value. The reserved bits
    // are absent, so no `mReservedBit*Cleared` field may be set. The
    // encode must output the default second byte, with all reserved
    // bits `1`.

    policy.SetFlags(kAllReservedClearedFlags, 1);
    VerifyOrQuit(!policy.mReservedBit10Cleared);
    VerifyOrQuit(!policy.mReservedBit11Cleared);
    VerifyOrQuit(!policy.mReservedBit12Cleared);

    policy.GetFlags(flags, sizeof(flags));
    VerifyOrQuit(memcmp(flags, kDefaultFlags, sizeof(flags)) == 0);

    // Encode into one byte. `GetFlags()` must write the first byte only.
    // A cleared reserved bit must not change the second byte.

    policy.SetFlags(kAllReservedClearedFlags, sizeof(kAllReservedClearedFlags));
    memset(flags, 0xaa, sizeof(flags));
    policy.GetFlags(flags, 1);
    VerifyOrQuit(flags[0] == kAllReservedClearedFlags[0]);
    VerifyOrQuit(flags[1] == 0xaa, "GetFlags() with length 1 must not write the second byte");

    // Decode flags that are longer than two bytes. `SetFlags()` must
    // read the first two bytes and ignore the rest.

    policy.SetFlags(kExtraByteFlags, sizeof(kExtraByteFlags));
    VerifyOrQuit(policy.mReservedBit10Cleared);
    VerifyOrQuit(policy.mReservedBit11Cleared);
    VerifyOrQuit(policy.mReservedBit12Cleared);

    policy.GetFlags(flags, sizeof(flags));
    VerifyOrQuit(memcmp(flags, kExtraByteFlags, sizeof(flags)) == 0);

    // A zero-initialized `SecurityPolicy` must encode all reserved bits
    // as `1`, their default in the specification. `otSecurityPolicy` is
    // a public C struct with no constructor. Applications zero-initialize
    // it and set only the fields that they know. Thus the zero value of
    // each `mReservedBit*Cleared` field must give the default encoding.

    memset(&policy, 0, sizeof(policy));
    policy.GetFlags(flags, sizeof(flags));
    VerifyOrQuit((flags[1] & kReservedBitsMask) == kReservedBitsMask,
                 "zero-initialized policy must encode reserved bits as 1");

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    // Convert a dataset to `Dataset::Info` and back. The reserved bits
    // must not change. `otOperationalDataset` uses `Dataset::Info`.

    {
        static const uint8_t kSecurityPolicyTlv[] = {0x0c, 0x04, 0x02, 0xa0, 0xf7, 0xc8};

        Dataset                  dataset;
        Dataset                  dataset2;
        Dataset::Info            datasetInfo;
        const SecurityPolicyTlv *tlv;

        SuccessOrQuit(dataset.SetFrom(kSecurityPolicyTlv, sizeof(kSecurityPolicyTlv)));
        SuccessOrQuit(dataset.ValidateTlvs());

        dataset.ConvertTo(datasetInfo);
        VerifyOrQuit(datasetInfo.mComponents.mIsSecurityPolicyPresent);

        // The flags in the TLV are `0xf7 0xc8`. Reserved bits 10 and 11
        // are 0. Reserved bit 12 is 1.

        VerifyOrQuit(datasetInfo.mSecurityPolicy.mReservedBit10Cleared);
        VerifyOrQuit(datasetInfo.mSecurityPolicy.mReservedBit11Cleared);
        VerifyOrQuit(!datasetInfo.mSecurityPolicy.mReservedBit12Cleared);

        dataset2.SetFrom(datasetInfo);

        tlv = As<SecurityPolicyTlv>(dataset2.FindTlv(Tlv::kSecurityPolicy));
        VerifyOrQuit(tlv != nullptr);
        VerifyOrQuit(memcmp(tlv, kSecurityPolicyTlv, sizeof(kSecurityPolicyTlv)) == 0);
    }

    // Create a dataset from a zero-initialized `otOperationalDataset`
    // that has the Security Policy marked present. The Security Policy
    // TLV must encode all reserved bits as `1`. Applications build
    // datasets this way, field by field.

    {
        Dataset                  dataset;
        Dataset::Info            datasetInfo;
        const SecurityPolicyTlv *tlv;
        const uint8_t           *tlvFlags;

        memset(&datasetInfo, 0, sizeof(datasetInfo));
        datasetInfo.mSecurityPolicy.mRotationTime        = 672;
        datasetInfo.mComponents.mIsSecurityPolicyPresent = true;

        dataset.SetFrom(datasetInfo);

        tlv = As<SecurityPolicyTlv>(dataset.FindTlv(Tlv::kSecurityPolicy));
        VerifyOrQuit(tlv != nullptr);

        tlvFlags = tlv->GetValue() + sizeof(uint16_t); // Skip the Rotation Time field
        VerifyOrQuit((tlvFlags[1] & kReservedBitsMask) == kReservedBitsMask,
                     "dataset from zero-initialized struct must encode reserved bits as 1");
    }
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
}

} // namespace MeshCoP
} // namespace ot

int main(void)
{
    ot::MeshCoP::TestDataset();
    ot::MeshCoP::TestSecurityPolicy();

    printf("All tests passed\n");
    return 0;
}
