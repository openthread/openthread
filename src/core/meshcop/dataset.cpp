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
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

Dataset::Dataset(Type aType)
    : mUpdateTime(0)
    , mLength(0)
    , mType(aType)
{
    memset(mTlvs, 0, sizeof(mTlvs));
}

void Dataset::Clear(void)
{
    mLength = 0;
}

bool Dataset::IsValid(void) const
{
    bool       rval = true;
    const Tlv *end  = GetTlvsEnd();

    for (const Tlv *cur = GetTlvsStart(); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end && Tlv::IsValid(*cur), rval = false);
    }

exit:
    return rval;
}

const Tlv *Dataset::GetTlv(Tlv::Type aType) const
{
    return Tlv::FindTlv(mTlvs, mLength, aType);
}

void Dataset::ConvertTo(otOperationalDataset &aDataset) const
{
    memset(&aDataset, 0, sizeof(aDataset));

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case Tlv::kActiveTimestamp:
            aDataset.mActiveTimestamp                      = static_cast<const ActiveTimestampTlv *>(cur)->GetSeconds();
            aDataset.mComponents.mIsActiveTimestampPresent = true;
            break;

        case Tlv::kChannel:
            aDataset.mChannel                      = static_cast<const ChannelTlv *>(cur)->GetChannel();
            aDataset.mComponents.mIsChannelPresent = true;
            break;

        case Tlv::kChannelMask:
        {
            uint32_t mask;

            if ((mask = static_cast<const ChannelMaskTlv *>(cur)->GetChannelMask()) != 0)
            {
                aDataset.mChannelMask                      = mask;
                aDataset.mComponents.mIsChannelMaskPresent = true;
            }

            break;
        }

        case Tlv::kDelayTimer:
            aDataset.mDelay                      = static_cast<const DelayTimerTlv *>(cur)->GetDelayTimer();
            aDataset.mComponents.mIsDelayPresent = true;
            break;

        case Tlv::kExtendedPanId:
            aDataset.mExtendedPanId = static_cast<const ExtendedPanIdTlv *>(cur)->GetExtendedPanId();
            aDataset.mComponents.mIsExtendedPanIdPresent = true;
            break;

        case Tlv::kMeshLocalPrefix:
            aDataset.mMeshLocalPrefix = static_cast<const MeshLocalPrefixTlv *>(cur)->GetMeshLocalPrefix();
            aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
            break;

        case Tlv::kNetworkMasterKey:
            aDataset.mMasterKey = static_cast<const NetworkMasterKeyTlv *>(cur)->GetNetworkMasterKey();
            aDataset.mComponents.mIsMasterKeyPresent = true;
            break;

        case Tlv::kNetworkName:
            IgnoreError(static_cast<Mac::NetworkName &>(aDataset.mNetworkName)
                            .Set(static_cast<const NetworkNameTlv *>(cur)->GetNetworkName()));
            aDataset.mComponents.mIsNetworkNamePresent = true;
            break;

        case Tlv::kPanId:
            aDataset.mPanId                      = static_cast<const PanIdTlv *>(cur)->GetPanId();
            aDataset.mComponents.mIsPanIdPresent = true;
            break;

        case Tlv::kPendingTimestamp:
            aDataset.mPendingTimestamp = static_cast<const PendingTimestampTlv *>(cur)->GetSeconds();
            aDataset.mComponents.mIsPendingTimestampPresent = true;
            break;

        case Tlv::kPskc:
            aDataset.mPskc                      = static_cast<const PskcTlv *>(cur)->GetPskc();
            aDataset.mComponents.mIsPskcPresent = true;
            break;

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *tlv                  = static_cast<const SecurityPolicyTlv *>(cur);
            aDataset.mSecurityPolicy.mRotationTime        = tlv->GetRotationTime();
            aDataset.mSecurityPolicy.mFlags               = tlv->GetFlags();
            aDataset.mComponents.mIsSecurityPolicyPresent = true;
            break;
        }

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

void Dataset::Set(const Dataset &aDataset)
{
    memcpy(mTlvs, aDataset.mTlvs, aDataset.mLength);
    mLength = aDataset.mLength;

    if (mType == kActive)
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

otError Dataset::SetFrom(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;

    if (aDataset.mComponents.mIsActiveTimestampPresent)
    {
        ActiveTimestampTlv tlv;
        tlv.Init();
        tlv.SetSeconds(aDataset.mActiveTimestamp);
        tlv.SetTicks(0);
        IgnoreError(SetTlv(tlv));
    }

    if (aDataset.mComponents.mIsPendingTimestampPresent)
    {
        PendingTimestampTlv tlv;
        tlv.Init();
        tlv.SetSeconds(aDataset.mPendingTimestamp);
        tlv.SetTicks(0);
        IgnoreError(SetTlv(tlv));
    }

    if (aDataset.mComponents.mIsDelayPresent)
    {
        IgnoreError(SetUint32Tlv(Tlv::kDelayTimer, aDataset.mDelay));
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannel(aDataset.mChannel);
        IgnoreError(SetTlv(tlv));
    }

    if (aDataset.mComponents.mIsChannelMaskPresent)
    {
        ChannelMaskTlv tlv;
        tlv.Init();
        tlv.SetChannelMask(aDataset.mChannelMask);
        IgnoreError(SetTlv(tlv));
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        IgnoreError(SetTlv(Tlv::kExtendedPanId, &aDataset.mExtendedPanId, sizeof(Mac::ExtendedPanId)));
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        IgnoreError(SetTlv(Tlv::kMeshLocalPrefix, &aDataset.mMeshLocalPrefix, sizeof(Mle::MeshLocalPrefix)));
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        IgnoreError(SetTlv(Tlv::kNetworkMasterKey, &aDataset.mMasterKey, sizeof(MasterKey)));
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        Mac::NameData nameData = static_cast<const Mac::NetworkName &>(aDataset.mNetworkName).GetAsData();

        IgnoreError(SetTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        IgnoreError(SetUint16Tlv(Tlv::kPanId, aDataset.mPanId));
    }

    if (aDataset.mComponents.mIsPskcPresent)
    {
        IgnoreError(SetTlv(Tlv::kPskc, &aDataset.mPskc, sizeof(Pskc)));
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(aDataset.mSecurityPolicy.mRotationTime);
        tlv.SetFlags(aDataset.mSecurityPolicy.mFlags);
        IgnoreError(SetTlv(tlv));
    }

    mUpdateTime = TimerMilli::GetNow();

    return error;
}

const Timestamp *Dataset::GetTimestamp(void) const
{
    const Timestamp *timestamp = nullptr;

    if (mType == kActive)
    {
        const ActiveTimestampTlv *tlv = GetTlv<ActiveTimestampTlv>();
        VerifyOrExit(tlv != nullptr, OT_NOOP);
        timestamp = static_cast<const Timestamp *>(tlv);
    }
    else
    {
        const PendingTimestampTlv *tlv = GetTlv<PendingTimestampTlv>();
        VerifyOrExit(tlv != nullptr, OT_NOOP);
        timestamp = static_cast<const Timestamp *>(tlv);
    }

exit:
    return timestamp;
}

void Dataset::SetTimestamp(const Timestamp &aTimestamp)
{
    IgnoreError(
        SetTlv((mType == kActive) ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp, &aTimestamp, sizeof(Timestamp)));
}

otError Dataset::SetTlv(Tlv::Type aType, const void *aValue, uint8_t aLength)
{
    otError  error          = OT_ERROR_NONE;
    uint16_t bytesAvailable = sizeof(mTlvs) - mLength;
    Tlv *    old            = GetTlv(aType);
    Tlv      tlv;

    if (old != nullptr)
    {
        bytesAvailable += sizeof(Tlv) + old->GetLength();
    }

    VerifyOrExit(sizeof(Tlv) + aLength <= bytesAvailable, error = OT_ERROR_NO_BUFS);

    if (old != nullptr)
    {
        RemoveTlv(old);
    }

    tlv.SetType(aType);
    tlv.SetLength(aLength);
    memcpy(mTlvs + mLength, &tlv, sizeof(Tlv));
    mLength += sizeof(Tlv);

    memcpy(mTlvs + mLength, aValue, aLength);
    mLength += aLength;

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

otError Dataset::SetTlv(const Tlv &aTlv)
{
    return SetTlv(aTlv.GetType(), aTlv.GetValue(), aTlv.GetLength());
}

otError Dataset::SetUint16Tlv(Tlv::Type aType, uint16_t aValue)
{
    uint16_t value16 = HostSwap16(aValue);

    return SetTlv(aType, &value16, sizeof(uint16_t));
}

otError Dataset::SetUint32Tlv(Tlv::Type aType, uint32_t aValue)
{
    uint32_t value32 = HostSwap32(aValue);

    return SetTlv(aType, &value32, sizeof(uint32_t));
}

otError Dataset::Set(const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aLength == aMessage.Read(aOffset, aLength, mTlvs), error = OT_ERROR_INVALID_ARGS);
    mLength = aLength;

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

void Dataset::RemoveTlv(Tlv::Type aType)
{
    Tlv *tlv;

    VerifyOrExit((tlv = GetTlv(aType)) != nullptr, OT_NOOP);
    RemoveTlv(tlv);

exit:
    return;
}

otError Dataset::AppendMleDatasetTlv(Message &aMessage) const
{
    otError        error = OT_ERROR_NONE;
    Mle::Tlv       tlv;
    Mle::Tlv::Type type;

    VerifyOrExit(mLength > 0, OT_NOOP);

    type = (mType == kActive ? Mle::Tlv::kActiveDataset : Mle::Tlv::kPendingDataset);

    tlv.SetType(type);
    tlv.SetLength(static_cast<uint8_t>(mLength) - sizeof(Tlv) - sizeof(Timestamp));
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv)));

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        if (((mType == kActive) && (cur->GetType() == Tlv::kActiveTimestamp)) ||
            ((mType == kPending) && (cur->GetType() == Tlv::kPendingTimestamp)))
        {
            ; // skip Active or Pending Timestamp TLV
        }
        else if (cur->GetType() == Tlv::kDelayTimer)
        {
            uint32_t      elapsed = TimerMilli::GetNow() - mUpdateTime;
            DelayTimerTlv delayTimer(static_cast<const DelayTimerTlv &>(*cur));

            if (delayTimer.GetDelayTimer() > elapsed)
            {
                delayTimer.SetDelayTimer(delayTimer.GetDelayTimer() - elapsed);
            }
            else
            {
                delayTimer.SetDelayTimer(0);
            }

            SuccessOrExit(error = delayTimer.AppendTo(aMessage));
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
    uint8_t *start  = reinterpret_cast<uint8_t *>(aTlv);
    uint16_t length = sizeof(Tlv) + aTlv->GetLength();

    memmove(start, start + length, mLength - (static_cast<uint8_t>(start - mTlvs) + length));
    mLength -= length;
}

otError Dataset::ApplyConfiguration(Instance &aInstance, bool *aIsMasterKeyUpdated) const
{
    Mac::Mac &  mac        = aInstance.Get<Mac::Mac>();
    KeyManager &keyManager = aInstance.Get<KeyManager>();
    otError     error      = OT_ERROR_NONE;

    VerifyOrExit(IsValid(), error = OT_ERROR_PARSE);

    if (aIsMasterKeyUpdated)
    {
        *aIsMasterKeyUpdated = false;
    }

    for (const Tlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case Tlv::kChannel:
        {
            uint8_t channel = static_cast<uint8_t>(static_cast<const ChannelTlv *>(cur)->GetChannel());

            error = mac.SetPanChannel(channel);

            if (error != OT_ERROR_NONE)
            {
                otLogWarnMeshCoP("DatasetManager::ApplyConfiguration() Failed to set channel to %d (%s)", channel,
                                 otThreadErrorToString(error));
                ExitNow();
            }

            break;
        }

        case Tlv::kPanId:
            mac.SetPanId(static_cast<const PanIdTlv *>(cur)->GetPanId());
            break;

        case Tlv::kExtendedPanId:
            mac.SetExtendedPanId(static_cast<const ExtendedPanIdTlv *>(cur)->GetExtendedPanId());
            break;

        case Tlv::kNetworkName:
            IgnoreError(mac.SetNetworkName(static_cast<const NetworkNameTlv *>(cur)->GetNetworkName()));
            break;

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *key = static_cast<const NetworkMasterKeyTlv *>(cur);

            if (aIsMasterKeyUpdated && (key->GetNetworkMasterKey() != keyManager.GetMasterKey()))
            {
                *aIsMasterKeyUpdated = true;
            }

            IgnoreError(keyManager.SetMasterKey(key->GetNetworkMasterKey()));
            break;
        }

#if OPENTHREAD_FTD

        case Tlv::kPskc:
            keyManager.SetPskc(static_cast<const PskcTlv *>(cur)->GetPskc());
            break;

#endif

        case Tlv::kMeshLocalPrefix:
            aInstance.Get<Mle::MleRouter>().SetMeshLocalPrefix(
                static_cast<const MeshLocalPrefixTlv *>(cur)->GetMeshLocalPrefix());
            break;

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *securityPolicy = static_cast<const SecurityPolicyTlv *>(cur);
            IgnoreError(keyManager.SetKeyRotation(securityPolicy->GetRotationTime()));
            keyManager.SetSecurityPolicyFlags(securityPolicy->GetFlags());
            break;
        }

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
    mType = kActive;
}

const char *Dataset::TypeToString(Type aType)
{
    return (aType == kActive) ? "Active" : "Pending";
}

} // namespace MeshCoP
} // namespace ot
