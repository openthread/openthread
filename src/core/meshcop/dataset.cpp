/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

/**
 * @file
 *   This file implements common methods for manipulating MeshCoP Datasets.
 *
 */

#include "dataset.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "instance/instance.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/timestamp.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Dataset");

Error Dataset::Info::GenerateRandom(Instance &aInstance)
{
    Error            error;
    Mac::ChannelMask supportedChannels = aInstance.Get<Mac::Mac>().GetSupportedChannelMask();
    Mac::ChannelMask preferredChannels(aInstance.Get<Radio>().GetPreferredChannelMask());
    StringWriter     nameWriter(mNetworkName.m8, sizeof(mNetworkName));

    // If the preferred channel mask is not empty, select a random
    // channel from it, otherwise choose one from the supported
    // channel mask.

    preferredChannels.Intersect(supportedChannels);

    if (preferredChannels.IsEmpty())
    {
        preferredChannels = supportedChannels;
    }

    Clear();

    mActiveTimestamp.mSeconds       = 1;
    mActiveTimestamp.mTicks         = 0;
    mActiveTimestamp.mAuthoritative = false;
    mChannel                        = preferredChannels.ChooseRandomChannel();
    mChannelMask                    = supportedChannels.GetMask();
    mPanId                          = Mac::GenerateRandomPanId();
    AsCoreType(&mSecurityPolicy).SetToDefault();

    SuccessOrExit(error = AsCoreType(&mNetworkKey).GenerateRandom());
    SuccessOrExit(error = AsCoreType(&mPskc).GenerateRandom());
    SuccessOrExit(error = Random::Crypto::Fill(mExtendedPanId));
    SuccessOrExit(error = AsCoreType(&mMeshLocalPrefix).GenerateRandomUla());

    nameWriter.Append("%s-%04x", NetworkName::kNetworkNameInit, mPanId);

    mComponents.mIsActiveTimestampPresent = true;
    mComponents.mIsNetworkKeyPresent      = true;
    mComponents.mIsNetworkNamePresent     = true;
    mComponents.mIsExtendedPanIdPresent   = true;
    mComponents.mIsMeshLocalPrefixPresent = true;
    mComponents.mIsPanIdPresent           = true;
    mComponents.mIsChannelPresent         = true;
    mComponents.mIsPskcPresent            = true;
    mComponents.mIsSecurityPolicyPresent  = true;
    mComponents.mIsChannelMaskPresent     = true;

exit:
    return error;
}

bool Dataset::Info::IsSubsetOf(const Info &aOther) const
{
    bool isSubset = false;

    if (IsPresent<kNetworkKey>())
    {
        VerifyOrExit(aOther.IsPresent<kNetworkKey>() && Get<kNetworkKey>() == aOther.Get<kNetworkKey>());
    }

    if (IsPresent<kNetworkName>())
    {
        VerifyOrExit(aOther.IsPresent<kNetworkName>() && Get<kNetworkName>() == aOther.Get<kNetworkName>());
    }

    if (IsPresent<kExtendedPanId>())
    {
        VerifyOrExit(aOther.IsPresent<kExtendedPanId>() && Get<kExtendedPanId>() == aOther.Get<kExtendedPanId>());
    }

    if (IsPresent<kMeshLocalPrefix>())
    {
        VerifyOrExit(aOther.IsPresent<kMeshLocalPrefix>() && Get<kMeshLocalPrefix>() == aOther.Get<kMeshLocalPrefix>());
    }

    if (IsPresent<kPanId>())
    {
        VerifyOrExit(aOther.IsPresent<kPanId>() && Get<kPanId>() == aOther.Get<kPanId>());
    }

    if (IsPresent<kChannel>())
    {
        VerifyOrExit(aOther.IsPresent<kChannel>() && Get<kChannel>() == aOther.Get<kChannel>());
    }

    if (IsPresent<kPskc>())
    {
        VerifyOrExit(aOther.IsPresent<kPskc>() && Get<kPskc>() == aOther.Get<kPskc>());
    }

    if (IsPresent<kSecurityPolicy>())
    {
        VerifyOrExit(aOther.IsPresent<kSecurityPolicy>() && Get<kSecurityPolicy>() == aOther.Get<kSecurityPolicy>());
    }

    if (IsPresent<kChannelMask>())
    {
        VerifyOrExit(aOther.IsPresent<kChannelMask>() && Get<kChannelMask>() == aOther.Get<kChannelMask>());
    }

    isSubset = true;

exit:
    return isSubset;
}

Dataset::Dataset(void)
    : mUpdateTime(0)
    , mLength(0)
{
    ClearAllBytes(mTlvs);
}

void Dataset::Clear(void) { mLength = 0; }

bool Dataset::IsValid(void) const
{
    bool       rval = true;
    const Tlv *end  = GetTlvsEnd();

    for (const Tlv *cur = GetTlvsStart(); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit(!cur->IsExtended() && (cur + 1) <= end && cur->GetNext() <= end && IsTlvValid(*cur), rval = false);
    }

exit:
    return rval;
}

bool Dataset::IsTlvValid(const Tlv &aTlv)
{
    bool    isValid   = true;
    uint8_t minLength = 0;

    switch (aTlv.GetType())
    {
    case Tlv::kPanId:
        minLength = sizeof(PanIdTlv::UintValueType);
        break;
    case Tlv::kExtendedPanId:
        minLength = sizeof(ExtendedPanIdTlv::ValueType);
        break;
    case Tlv::kPskc:
        minLength = sizeof(PskcTlv::ValueType);
        break;
    case Tlv::kNetworkKey:
        minLength = sizeof(NetworkKeyTlv::ValueType);
        break;
    case Tlv::kMeshLocalPrefix:
        minLength = sizeof(MeshLocalPrefixTlv::ValueType);
        break;
    case Tlv::kChannel:
        VerifyOrExit(aTlv.GetLength() >= sizeof(ChannelTlvValue), isValid = false);
        isValid = aTlv.ReadValueAs<ChannelTlv>().IsValid();
        break;
    case Tlv::kNetworkName:
        isValid = As<NetworkNameTlv>(aTlv).IsValid();
        break;

    case Tlv::kSecurityPolicy:
        isValid = As<SecurityPolicyTlv>(aTlv).IsValid();
        break;

    case Tlv::kChannelMask:
        isValid = As<ChannelMaskTlv>(aTlv).IsValid();
        break;

    default:
        break;
    }

    if (minLength > 0)
    {
        isValid = (aTlv.GetLength() >= minLength);
    }

exit:
    return isValid;
}

const Tlv *Dataset::FindTlv(Tlv::Type aType) const { return As<Tlv>(Tlv::FindTlv(mTlvs, mLength, aType)); }

void Dataset::ConvertTo(Info &aDatasetInfo) const
{
    aDatasetInfo.Clear();

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case Tlv::kActiveTimestamp:
            aDatasetInfo.Set<kActiveTimestamp>(cur->ReadValueAs<ActiveTimestampTlv>());
            break;

        case Tlv::kChannel:
            aDatasetInfo.Set<kChannel>(cur->ReadValueAs<ChannelTlv>().GetChannel());
            break;

        case Tlv::kChannelMask:
        {
            uint32_t mask;

            if (As<ChannelMaskTlv>(cur)->ReadChannelMask(mask) == kErrorNone)
            {
                aDatasetInfo.Set<kChannelMask>(mask);
            }

            break;
        }

        case Tlv::kDelayTimer:
            aDatasetInfo.Set<kDelay>(cur->ReadValueAs<DelayTimerTlv>());
            break;

        case Tlv::kExtendedPanId:
            aDatasetInfo.Set<kExtendedPanId>(cur->ReadValueAs<ExtendedPanIdTlv>());
            break;

        case Tlv::kMeshLocalPrefix:
            aDatasetInfo.Set<kMeshLocalPrefix>(cur->ReadValueAs<MeshLocalPrefixTlv>());
            break;

        case Tlv::kNetworkKey:
            aDatasetInfo.Set<kNetworkKey>(cur->ReadValueAs<NetworkKeyTlv>());
            break;

        case Tlv::kNetworkName:
            IgnoreError(aDatasetInfo.Update<kNetworkName>().Set(As<NetworkNameTlv>(cur)->GetNetworkName()));
            break;

        case Tlv::kPanId:
            aDatasetInfo.Set<kPanId>(cur->ReadValueAs<PanIdTlv>());
            break;

        case Tlv::kPendingTimestamp:
            aDatasetInfo.Set<kPendingTimestamp>(cur->ReadValueAs<PendingTimestampTlv>());
            break;

        case Tlv::kPskc:
            aDatasetInfo.Set<kPskc>(cur->ReadValueAs<PskcTlv>());
            break;

        case Tlv::kSecurityPolicy:
            aDatasetInfo.Set<kSecurityPolicy>(As<SecurityPolicyTlv>(cur)->GetSecurityPolicy());
            break;

        default:
            break;
        }
    }
}

void Dataset::ConvertTo(Tlvs &aTlvs) const
{
    memcpy(aTlvs.mTlvs, mTlvs, mLength);
    aTlvs.mLength = static_cast<uint8_t>(mLength);
}

void Dataset::Set(Type aType, const Dataset &aDataset)
{
    memcpy(mTlvs, aDataset.mTlvs, aDataset.mLength);
    mLength = aDataset.mLength;

    if (aType == kActive)
    {
        RemoveTlv(Tlv::kPendingTimestamp);
        RemoveTlv(Tlv::kDelayTimer);
    }

    mUpdateTime = aDataset.GetUpdateTime();
}

void Dataset::SetFrom(const Tlvs &aTlvs)
{
    mLength = aTlvs.mLength;
    memcpy(mTlvs, aTlvs.mTlvs, mLength);
}

Error Dataset::SetFrom(const Info &aDatasetInfo)
{
    Error error = kErrorNone;

    if (aDatasetInfo.IsPresent<kActiveTimestamp>())
    {
        Timestamp activeTimestamp;

        aDatasetInfo.Get<kActiveTimestamp>(activeTimestamp);
        IgnoreError(Write<ActiveTimestampTlv>(activeTimestamp));
    }

    if (aDatasetInfo.IsPresent<kPendingTimestamp>())
    {
        Timestamp pendingTimestamp;

        aDatasetInfo.Get<kPendingTimestamp>(pendingTimestamp);
        IgnoreError(Write<PendingTimestampTlv>(pendingTimestamp));
    }

    if (aDatasetInfo.IsPresent<kDelay>())
    {
        IgnoreError(Write<DelayTimerTlv>(aDatasetInfo.Get<kDelay>()));
    }

    if (aDatasetInfo.IsPresent<kChannel>())
    {
        ChannelTlvValue channelValue;

        channelValue.SetChannelAndPage(aDatasetInfo.Get<kChannel>());
        IgnoreError(Write<ChannelTlv>(channelValue));
    }

    if (aDatasetInfo.IsPresent<kChannelMask>())
    {
        ChannelMaskTlv::Value value;

        ChannelMaskTlv::PrepareValue(value, aDatasetInfo.Get<kChannelMask>());
        IgnoreError(WriteTlv(Tlv::kChannelMask, value.mData, value.mLength));
    }

    if (aDatasetInfo.IsPresent<kExtendedPanId>())
    {
        IgnoreError(Write<ExtendedPanIdTlv>(aDatasetInfo.Get<kExtendedPanId>()));
    }

    if (aDatasetInfo.IsPresent<kMeshLocalPrefix>())
    {
        IgnoreError(Write<MeshLocalPrefixTlv>(aDatasetInfo.Get<kMeshLocalPrefix>()));
    }

    if (aDatasetInfo.IsPresent<kNetworkKey>())
    {
        IgnoreError(Write<NetworkKeyTlv>(aDatasetInfo.Get<kNetworkKey>()));
    }

    if (aDatasetInfo.IsPresent<kNetworkName>())
    {
        NameData nameData = aDatasetInfo.Get<kNetworkName>().GetAsData();

        IgnoreError(WriteTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (aDatasetInfo.IsPresent<kPanId>())
    {
        IgnoreError(Write<PanIdTlv>(aDatasetInfo.Get<kPanId>()));
    }

    if (aDatasetInfo.IsPresent<kPskc>())
    {
        IgnoreError(Write<PskcTlv>(aDatasetInfo.Get<kPskc>()));
    }

    if (aDatasetInfo.IsPresent<kSecurityPolicy>())
    {
        SecurityPolicyTlv tlv;

        tlv.Init();
        tlv.SetSecurityPolicy(aDatasetInfo.Get<kSecurityPolicy>());
        IgnoreError(WriteTlv(tlv));
    }

    mUpdateTime = TimerMilli::GetNow();

    return error;
}

Error Dataset::ReadTimestamp(Type aType, Timestamp &aTimestamp) const
{
    return (aType == kActive) ? Read<ActiveTimestampTlv>(aTimestamp) : Read<PendingTimestampTlv>(aTimestamp);
}

Error Dataset::WriteTlv(Tlv::Type aType, const void *aValue, uint8_t aLength)
{
    Error    error          = kErrorNone;
    uint16_t bytesAvailable = sizeof(mTlvs) - mLength;
    Tlv     *oldTlv         = FindTlv(aType);
    Tlv     *newTlv;

    if (oldTlv != nullptr)
    {
        bytesAvailable += sizeof(Tlv) + oldTlv->GetLength();
    }

    VerifyOrExit(sizeof(Tlv) + aLength <= bytesAvailable, error = kErrorNoBufs);

    RemoveTlv(oldTlv);

    newTlv = GetTlvsEnd();
    mLength += sizeof(Tlv) + aLength;

    newTlv->SetType(aType);
    newTlv->SetLength(aLength);
    memcpy(newTlv->GetValue(), aValue, aLength);

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

Error Dataset::WriteTlv(const Tlv &aTlv) { return WriteTlv(aTlv.GetType(), aTlv.GetValue(), aTlv.GetLength()); }

Error Dataset::ReadFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error error = kErrorParse;

    VerifyOrExit(aLength <= kMaxSize);

    SuccessOrExit(aMessage.Read(aOffset, mTlvs, aLength));
    mLength = aLength;

    VerifyOrExit(IsValid(), error = kErrorParse);

    mUpdateTime = TimerMilli::GetNow();
    error       = kErrorNone;

exit:
    return error;
}

void Dataset::RemoveTlv(Tlv::Type aType) { RemoveTlv(FindTlv(aType)); }

void Dataset::RemoveTlv(Tlv *aTlv)
{
    if (aTlv != nullptr)
    {
        uint8_t *start  = reinterpret_cast<uint8_t *>(aTlv);
        uint16_t length = sizeof(Tlv) + aTlv->GetLength();

        memmove(start, start + length, mLength - (static_cast<uint8_t>(start - mTlvs) + length));
        mLength -= length;
    }
}

Error Dataset::ApplyConfiguration(Instance &aInstance) const
{
    bool isNetworkKeyUpdated;

    return ApplyConfiguration(aInstance, isNetworkKeyUpdated);
}

Error Dataset::ApplyConfiguration(Instance &aInstance, bool &aIsNetworkKeyUpdated) const
{
    Mac::Mac   &mac        = aInstance.Get<Mac::Mac>();
    KeyManager &keyManager = aInstance.Get<KeyManager>();
    Error       error      = kErrorNone;

    VerifyOrExit(IsValid(), error = kErrorParse);

    aIsNetworkKeyUpdated = false;

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case Tlv::kChannel:
        {
            uint8_t channel = static_cast<uint8_t>(cur->ReadValueAs<ChannelTlv>().GetChannel());

            error = mac.SetPanChannel(channel);

            if (error != kErrorNone)
            {
                LogWarn("ApplyConfiguration() Failed to set channel to %d (%s)", channel, ErrorToString(error));
                ExitNow();
            }

            break;
        }

        case Tlv::kPanId:
            mac.SetPanId(cur->ReadValueAs<PanIdTlv>());
            break;

        case Tlv::kExtendedPanId:
            aInstance.Get<ExtendedPanIdManager>().SetExtPanId(cur->ReadValueAs<ExtendedPanIdTlv>());
            break;

        case Tlv::kNetworkName:
            IgnoreError(aInstance.Get<NetworkNameManager>().SetNetworkName(As<NetworkNameTlv>(cur)->GetNetworkName()));
            break;

        case Tlv::kNetworkKey:
        {
            NetworkKey networkKey;

            keyManager.GetNetworkKey(networkKey);

            if (cur->ReadValueAs<NetworkKeyTlv>() != networkKey)
            {
                aIsNetworkKeyUpdated = true;
            }

            keyManager.SetNetworkKey(cur->ReadValueAs<NetworkKeyTlv>());
            break;
        }

#if OPENTHREAD_FTD
        case Tlv::kPskc:
            keyManager.SetPskc(cur->ReadValueAs<PskcTlv>());
            break;
#endif

        case Tlv::kMeshLocalPrefix:
            aInstance.Get<Mle::MleRouter>().SetMeshLocalPrefix(cur->ReadValueAs<MeshLocalPrefixTlv>());
            break;

        case Tlv::kSecurityPolicy:
            keyManager.SetSecurityPolicy(As<SecurityPolicyTlv>(cur)->GetSecurityPolicy());
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

void Dataset::ConvertToActive(void)
{
    RemoveTlv(Tlv::kPendingTimestamp);
    RemoveTlv(Tlv::kDelayTimer);
}

const char *Dataset::TypeToString(Type aType) { return (aType == kActive) ? "Active" : "Pending"; }

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

void Dataset::SaveTlvInSecureStorageAndClearValue(Tlv::Type aTlvType, Crypto::Storage::KeyRef aKeyRef)
{
    using namespace ot::Crypto::Storage;

    Tlv *tlv = FindTlv(aTlvType);

    VerifyOrExit(tlv != nullptr);
    VerifyOrExit(tlv->GetLength() > 0);

    SuccessOrAssert(ImportKey(aKeyRef, kKeyTypeRaw, kKeyAlgorithmVendor, kUsageExport, kTypePersistent, tlv->GetValue(),
                              tlv->GetLength()));

    memset(tlv->GetValue(), 0, tlv->GetLength());

exit:
    return;
}

Error Dataset::ReadTlvFromSecureStorage(Tlv::Type aTlvType, Crypto::Storage::KeyRef aKeyRef)
{
    using namespace ot::Crypto::Storage;

    Error  error = kErrorNone;
    Tlv   *tlv   = FindTlv(aTlvType);
    size_t readLength;

    VerifyOrExit(tlv != nullptr);
    VerifyOrExit(tlv->GetLength() > 0);

    SuccessOrExit(error = ExportKey(aKeyRef, tlv->GetValue(), tlv->GetLength(), readLength));
    VerifyOrExit(readLength == tlv->GetLength(), error = OT_ERROR_FAILED);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

} // namespace MeshCoP
} // namespace ot
