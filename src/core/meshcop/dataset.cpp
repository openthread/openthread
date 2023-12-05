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

    snprintf(mNetworkName.m8, sizeof(mNetworkName), "%s-%04x", NetworkName::kNetworkNameInit, mPanId);

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

    if (IsNetworkKeyPresent())
    {
        VerifyOrExit(aOther.IsNetworkKeyPresent() && GetNetworkKey() == aOther.GetNetworkKey());
    }

    if (IsNetworkNamePresent())
    {
        VerifyOrExit(aOther.IsNetworkNamePresent() && GetNetworkName() == aOther.GetNetworkName());
    }

    if (IsExtendedPanIdPresent())
    {
        VerifyOrExit(aOther.IsExtendedPanIdPresent() && GetExtendedPanId() == aOther.GetExtendedPanId());
    }

    if (IsMeshLocalPrefixPresent())
    {
        VerifyOrExit(aOther.IsMeshLocalPrefixPresent() && GetMeshLocalPrefix() == aOther.GetMeshLocalPrefix());
    }

    if (IsPanIdPresent())
    {
        VerifyOrExit(aOther.IsPanIdPresent() && GetPanId() == aOther.GetPanId());
    }

    if (IsChannelPresent())
    {
        VerifyOrExit(aOther.IsChannelPresent() && GetChannel() == aOther.GetChannel());
    }

    if (IsPskcPresent())
    {
        VerifyOrExit(aOther.IsPskcPresent() && GetPskc() == aOther.GetPskc());
    }

    if (IsSecurityPolicyPresent())
    {
        VerifyOrExit(aOther.IsSecurityPolicyPresent() && GetSecurityPolicy() == aOther.GetSecurityPolicy());
    }

    if (IsChannelMaskPresent())
    {
        VerifyOrExit(aOther.IsChannelMaskPresent() && GetChannelMask() == aOther.GetChannelMask());
    }

    isSubset = true;

exit:
    return isSubset;
}

Dataset::Dataset(void)
    : mUpdateTime(0)
    , mLength(0)
{
    memset(mTlvs, 0, sizeof(mTlvs));
}

void Dataset::Clear(void) { mLength = 0; }

bool Dataset::IsValid(void) const
{
    bool       rval = true;
    const Tlv *end  = GetTlvsEnd();

    for (const Tlv *cur = GetTlvsStart(); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit(!cur->IsExtended() && (cur + 1) <= end && cur->GetNext() <= end && Tlv::IsValid(*cur),
                     rval = false);
    }

exit:
    return rval;
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
            aDatasetInfo.SetActiveTimestamp(cur->ReadValueAs<ActiveTimestampTlv>());
            break;

        case Tlv::kChannel:
            aDatasetInfo.SetChannel(cur->ReadValueAs<ChannelTlv>().GetChannel());
            break;

        case Tlv::kChannelMask:
        {
            uint32_t mask;

            if (As<ChannelMaskTlv>(cur)->ReadChannelMask(mask) == kErrorNone)
            {
                aDatasetInfo.SetChannelMask(mask);
            }

            break;
        }

        case Tlv::kDelayTimer:
            aDatasetInfo.SetDelay(cur->ReadValueAs<DelayTimerTlv>());
            break;

        case Tlv::kExtendedPanId:
            aDatasetInfo.SetExtendedPanId(cur->ReadValueAs<ExtendedPanIdTlv>());
            break;

        case Tlv::kMeshLocalPrefix:
            aDatasetInfo.SetMeshLocalPrefix(cur->ReadValueAs<MeshLocalPrefixTlv>());
            break;

        case Tlv::kNetworkKey:
            aDatasetInfo.SetNetworkKey(cur->ReadValueAs<NetworkKeyTlv>());
            break;

        case Tlv::kNetworkName:
            aDatasetInfo.SetNetworkName(As<NetworkNameTlv>(cur)->GetNetworkName());
            break;

        case Tlv::kPanId:
            aDatasetInfo.SetPanId(cur->ReadValueAs<PanIdTlv>());
            break;

        case Tlv::kPendingTimestamp:
            aDatasetInfo.SetPendingTimestamp(cur->ReadValueAs<PendingTimestampTlv>());
            break;

        case Tlv::kPskc:
            aDatasetInfo.SetPskc(cur->ReadValueAs<PskcTlv>());
            break;

        case Tlv::kSecurityPolicy:
            aDatasetInfo.SetSecurityPolicy(As<SecurityPolicyTlv>(cur)->GetSecurityPolicy());
            break;

        default:
            break;
        }
    }
}

void Dataset::ConvertTo(otOperationalDatasetTlvs &aDataset) const
{
    memcpy(aDataset.mTlvs, mTlvs, mLength);
    aDataset.mLength = static_cast<uint8_t>(mLength);
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

void Dataset::SetFrom(const otOperationalDatasetTlvs &aDataset)
{
    mLength = aDataset.mLength;
    memcpy(mTlvs, aDataset.mTlvs, mLength);
}

Error Dataset::SetFrom(const Info &aDatasetInfo)
{
    Error error = kErrorNone;

    if (aDatasetInfo.IsActiveTimestampPresent())
    {
        Timestamp activeTimestamp;

        aDatasetInfo.GetActiveTimestamp(activeTimestamp);
        IgnoreError(Write<ActiveTimestampTlv>(activeTimestamp));
    }

    if (aDatasetInfo.IsPendingTimestampPresent())
    {
        Timestamp pendingTimestamp;

        aDatasetInfo.GetPendingTimestamp(pendingTimestamp);
        IgnoreError(Write<PendingTimestampTlv>(pendingTimestamp));
    }

    if (aDatasetInfo.IsDelayPresent())
    {
        IgnoreError(Write<DelayTimerTlv>(aDatasetInfo.GetDelay()));
    }

    if (aDatasetInfo.IsChannelPresent())
    {
        ChannelTlvValue channelValue;

        channelValue.SetChannelAndPage(aDatasetInfo.GetChannel());
        IgnoreError(Write<ChannelTlv>(channelValue));
    }

    if (aDatasetInfo.IsChannelMaskPresent())
    {
        ChannelMaskTlv::Value value;

        ChannelMaskTlv::PrepareValue(value, aDatasetInfo.GetChannelMask());
        IgnoreError(WriteTlv(Tlv::kChannelMask, value.mData, value.mLength));
    }

    if (aDatasetInfo.IsExtendedPanIdPresent())
    {
        IgnoreError(Write<ExtendedPanIdTlv>(aDatasetInfo.GetExtendedPanId()));
    }

    if (aDatasetInfo.IsMeshLocalPrefixPresent())
    {
        IgnoreError(Write<MeshLocalPrefixTlv>(aDatasetInfo.GetMeshLocalPrefix()));
    }

    if (aDatasetInfo.IsNetworkKeyPresent())
    {
        IgnoreError(Write<NetworkKeyTlv>(aDatasetInfo.GetNetworkKey()));
    }

    if (aDatasetInfo.IsNetworkNamePresent())
    {
        NameData nameData = aDatasetInfo.GetNetworkName().GetAsData();

        IgnoreError(WriteTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (aDatasetInfo.IsPanIdPresent())
    {
        IgnoreError(Write<PanIdTlv>(aDatasetInfo.GetPanId()));
    }

    if (aDatasetInfo.IsPskcPresent())
    {
        IgnoreError(Write<PskcTlv>(aDatasetInfo.GetPskc()));
    }

    if (aDatasetInfo.IsSecurityPolicyPresent())
    {
        SecurityPolicyTlv tlv;

        tlv.Init();
        tlv.SetSecurityPolicy(aDatasetInfo.GetSecurityPolicy());
        IgnoreError(WriteTlv(tlv));
    }

    mUpdateTime = TimerMilli::GetNow();

    return error;
}

Error Dataset::GetTimestamp(Type aType, Timestamp &aTimestamp) const
{
    Error      error = kErrorNone;
    const Tlv *tlv;

    if (aType == kActive)
    {
        tlv = FindTlv(Tlv::kActiveTimestamp);
        VerifyOrExit(tlv != nullptr, error = kErrorNotFound);
        aTimestamp = tlv->ReadValueAs<ActiveTimestampTlv>();
    }
    else
    {
        tlv = FindTlv(Tlv::kPendingTimestamp);
        VerifyOrExit(tlv != nullptr, error = kErrorNotFound);
        aTimestamp = tlv->ReadValueAs<PendingTimestampTlv>();
    }

exit:
    return error;
}

void Dataset::SetTimestamp(Type aType, const Timestamp &aTimestamp)
{
    if (aType == kActive)
    {
        IgnoreError(Write<ActiveTimestampTlv>(aTimestamp));
    }
    else
    {
        IgnoreError(Write<PendingTimestampTlv>(aTimestamp));
    }
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

Error Dataset::AppendMleDatasetTlv(Type aType, Message &aMessage) const
{
    Error          error = kErrorNone;
    Mle::Tlv       tlv;
    Mle::Tlv::Type type;

    VerifyOrExit(mLength > 0);

    type = (aType == kActive ? Mle::Tlv::kActiveDataset : Mle::Tlv::kPendingDataset);

    tlv.SetType(type);
    tlv.SetLength(static_cast<uint8_t>(mLength) - sizeof(Tlv) - sizeof(Timestamp));
    SuccessOrExit(error = aMessage.Append(tlv));

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        if (((aType == kActive) && (cur->GetType() == Tlv::kActiveTimestamp)) ||
            ((aType == kPending) && (cur->GetType() == Tlv::kPendingTimestamp)))
        {
            ; // skip Active or Pending Timestamp TLV
        }
        else if (cur->GetType() == Tlv::kDelayTimer)
        {
            uint32_t elapsed    = TimerMilli::GetNow() - mUpdateTime;
            uint32_t delayTimer = cur->ReadValueAs<DelayTimerTlv>();

            if (delayTimer > elapsed)
            {
                delayTimer -= elapsed;
            }
            else
            {
                delayTimer = 0;
            }

            SuccessOrExit(error = Tlv::Append<DelayTimerTlv>(aMessage, delayTimer));
        }
        else
        {
            SuccessOrExit(error = cur->AppendTo(aMessage));
        }
    }

exit:
    return error;
}

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

Error Dataset::ApplyConfiguration(Instance &aInstance, bool *aIsNetworkKeyUpdated) const
{
    Mac::Mac   &mac        = aInstance.Get<Mac::Mac>();
    KeyManager &keyManager = aInstance.Get<KeyManager>();
    Error       error      = kErrorNone;

    VerifyOrExit(IsValid(), error = kErrorParse);

    if (aIsNetworkKeyUpdated)
    {
        *aIsNetworkKeyUpdated = false;
    }

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

            if (aIsNetworkKeyUpdated && (cur->ReadValueAs<NetworkKeyTlv>() != networkKey))
            {
                *aIsNetworkKeyUpdated = true;
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
