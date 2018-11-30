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

#define WPP_NAME "dataset.tmh"

#include "dataset.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

Dataset::Dataset(const Tlv::Type aType)
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
    const Tlv *cur  = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *end  = reinterpret_cast<const Tlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end && Tlv::IsValid(*cur), rval = false);
    }

exit:
    return rval;
}

Tlv *Dataset::Get(Tlv::Type aType)
{
    Tlv *cur  = reinterpret_cast<Tlv *>(mTlvs);
    Tlv *end  = reinterpret_cast<Tlv *>(mTlvs + mLength);
    Tlv *rval = NULL;

    while (cur < end)
    {
        if (cur->GetType() == aType)
        {
            ExitNow(rval = cur);
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

const Tlv *Dataset::Get(Tlv::Type aType) const
{
    const Tlv *cur  = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *end  = reinterpret_cast<const Tlv *>(mTlvs + mLength);
    const Tlv *rval = NULL;

    while (cur < end)
    {
        if (cur->GetType() == aType)
        {
            ExitNow(rval = cur);
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

void Dataset::Get(otOperationalDataset &aDataset) const
{
    const Tlv *cur = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *end = reinterpret_cast<const Tlv *>(mTlvs + mLength);

    memset(&aDataset, 0, sizeof(aDataset));

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case Tlv::kActiveTimestamp:
        {
            const ActiveTimestampTlv *tlv                  = static_cast<const ActiveTimestampTlv *>(cur);
            aDataset.mActiveTimestamp                      = tlv->GetSeconds();
            aDataset.mComponents.mIsActiveTimestampPresent = true;
            break;
        }

        case Tlv::kChannel:
        {
            const ChannelTlv *tlv                  = static_cast<const ChannelTlv *>(cur);
            aDataset.mChannel                      = tlv->GetChannel();
            aDataset.mComponents.mIsChannelPresent = true;
            break;
        }

        case Tlv::kChannelMask:
        {
            const ChannelMaskBaseTlv *tlv   = static_cast<const ChannelMaskBaseTlv *>(cur);
            const ChannelMaskEntry *  entry = tlv->GetMaskEntry(OT_RADIO_CHANNEL_PAGE);

            if (entry != NULL)
            {
                aDataset.mChannelMaskPage0                      = entry->GetMask();
                aDataset.mComponents.mIsChannelMaskPage0Present = true;
            }

            break;
        }

        case Tlv::kDelayTimer:
        {
            const DelayTimerTlv *tlv             = static_cast<const DelayTimerTlv *>(cur);
            aDataset.mDelay                      = tlv->GetDelayTimer();
            aDataset.mComponents.mIsDelayPresent = true;
            break;
        }

        case Tlv::kExtendedPanId:
        {
            const ExtendedPanIdTlv *tlv                  = static_cast<const ExtendedPanIdTlv *>(cur);
            aDataset.mExtendedPanId                      = tlv->GetExtendedPanId();
            aDataset.mComponents.mIsExtendedPanIdPresent = true;
            break;
        }

        case Tlv::kMeshLocalPrefix:
        {
            const MeshLocalPrefixTlv *tlv                  = static_cast<const MeshLocalPrefixTlv *>(cur);
            aDataset.mMeshLocalPrefix                      = tlv->GetMeshLocalPrefix();
            aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
            break;
        }

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *tlv           = static_cast<const NetworkMasterKeyTlv *>(cur);
            aDataset.mMasterKey                      = tlv->GetNetworkMasterKey();
            aDataset.mComponents.mIsMasterKeyPresent = true;
            break;
        }

        case Tlv::kNetworkName:
        {
            const NetworkNameTlv *tlv = static_cast<const NetworkNameTlv *>(cur);
            memcpy(aDataset.mNetworkName.m8, tlv->GetNetworkName(), tlv->GetLength());
            aDataset.mNetworkName.m8[tlv->GetLength()] = '\0';
            aDataset.mComponents.mIsNetworkNamePresent = true;
            break;
        }

        case Tlv::kPanId:
        {
            const PanIdTlv *panid                = static_cast<const PanIdTlv *>(cur);
            aDataset.mPanId                      = panid->GetPanId();
            aDataset.mComponents.mIsPanIdPresent = true;
            break;
        }

        case Tlv::kPendingTimestamp:
        {
            const PendingTimestampTlv *tlv                  = static_cast<const PendingTimestampTlv *>(cur);
            aDataset.mPendingTimestamp                      = tlv->GetSeconds();
            aDataset.mComponents.mIsPendingTimestampPresent = true;
            break;
        }

        case Tlv::kPSKc:
        {
            const PSKcTlv *tlv = static_cast<const PSKcTlv *>(cur);
            memcpy(aDataset.mPSKc.m8, tlv->GetPSKc(), tlv->GetLength());
            aDataset.mComponents.mIsPSKcPresent = true;
            break;
        }

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *tlv                  = static_cast<const SecurityPolicyTlv *>(cur);
            aDataset.mSecurityPolicy.mRotationTime        = tlv->GetRotationTime();
            aDataset.mSecurityPolicy.mFlags               = tlv->GetFlags();
            aDataset.mComponents.mIsSecurityPolicyPresent = true;
            break;
        }

        default:
        {
            break;
        }
        }

        cur = cur->GetNext();
    }
}

otError Dataset::Set(const Dataset &aDataset)
{
    memcpy(mTlvs, aDataset.mTlvs, aDataset.mLength);
    mLength = aDataset.mLength;

    if (mType == Tlv::kActiveTimestamp)
    {
        Remove(Tlv::kPendingTimestamp);
        Remove(Tlv::kDelayTimer);
    }

    mUpdateTime = aDataset.GetUpdateTime();

    return OT_ERROR_NONE;
}

otError Dataset::Set(const otOperationalDataset &aDataset)
{
    otError                     error = OT_ERROR_NONE;
    MeshCoP::ActiveTimestampTlv activeTimestampTlv;

    VerifyOrExit(aDataset.mComponents.mIsActiveTimestampPresent, error = OT_ERROR_INVALID_ARGS);

    activeTimestampTlv.Init();
    activeTimestampTlv.SetSeconds(aDataset.mActiveTimestamp);
    activeTimestampTlv.SetTicks(0);
    Set(activeTimestampTlv);

    if (mType == Tlv::kPendingTimestamp)
    {
        MeshCoP::PendingTimestampTlv pendingTimestampTlv;

        VerifyOrExit(aDataset.mComponents.mIsPendingTimestampPresent, error = OT_ERROR_INVALID_ARGS);

        pendingTimestampTlv.Init();
        pendingTimestampTlv.SetSeconds(aDataset.mPendingTimestamp);
        pendingTimestampTlv.SetTicks(0);
        Set(pendingTimestampTlv);

        if (aDataset.mComponents.mIsDelayPresent)
        {
            MeshCoP::DelayTimerTlv tlv;
            tlv.Init();
            tlv.SetDelayTimer(aDataset.mDelay);
            Set(tlv);
        }
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        MeshCoP::ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannelPage(OT_RADIO_CHANNEL_PAGE);
        tlv.SetChannel(aDataset.mChannel);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsChannelMaskPage0Present)
    {
        MeshCoP::ChannelMaskTlv tlv;
        tlv.Init();
        tlv.SetChannelPage(OT_RADIO_CHANNEL_PAGE);
        tlv.SetMask(aDataset.mChannelMaskPage0);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        MeshCoP::ExtendedPanIdTlv tlv;
        tlv.Init();
        tlv.SetExtendedPanId(aDataset.mExtendedPanId);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        MeshCoP::MeshLocalPrefixTlv tlv;
        tlv.Init();
        tlv.SetMeshLocalPrefix(aDataset.mMeshLocalPrefix);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        MeshCoP::NetworkMasterKeyTlv tlv;
        tlv.Init();
        tlv.SetNetworkMasterKey(aDataset.mMasterKey);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        MeshCoP::NetworkNameTlv tlv;
        tlv.Init();
        tlv.SetNetworkName(aDataset.mNetworkName.m8);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        MeshCoP::PanIdTlv tlv;
        tlv.Init();
        tlv.SetPanId(aDataset.mPanId);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsPSKcPresent)
    {
        MeshCoP::PSKcTlv tlv;
        tlv.Init();
        tlv.SetPSKc(aDataset.mPSKc.m8);
        Set(tlv);
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        MeshCoP::SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(aDataset.mSecurityPolicy.mRotationTime);
        tlv.SetFlags(aDataset.mSecurityPolicy.mFlags);
        Set(tlv);
    }

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

const Timestamp *Dataset::GetTimestamp(void) const
{
    const Timestamp *timestamp = NULL;

    if (mType == Tlv::kActiveTimestamp)
    {
        const ActiveTimestampTlv *tlv = static_cast<const ActiveTimestampTlv *>(Get(mType));
        VerifyOrExit(tlv != NULL);
        timestamp = static_cast<const Timestamp *>(tlv);
    }
    else
    {
        const PendingTimestampTlv *tlv = static_cast<const PendingTimestampTlv *>(Get(mType));
        VerifyOrExit(tlv != NULL);
        timestamp = static_cast<const Timestamp *>(tlv);
    }

exit:
    return timestamp;
}

void Dataset::SetTimestamp(const Timestamp &aTimestamp)
{
    if (mType == Tlv::kActiveTimestamp)
    {
        ActiveTimestampTlv activeTimestamp;
        activeTimestamp.Init();
        *static_cast<Timestamp *>(&activeTimestamp) = aTimestamp;
        Set(activeTimestamp);
    }
    else
    {
        PendingTimestampTlv pendingTimestamp;
        pendingTimestamp.Init();
        *static_cast<Timestamp *>(&pendingTimestamp) = aTimestamp;
        Set(pendingTimestamp);
    }
}

otError Dataset::Set(const Tlv &aTlv)
{
    otError  error          = OT_ERROR_NONE;
    uint16_t bytesAvailable = sizeof(mTlvs) - mLength;
    Tlv *    old            = Get(aTlv.GetType());

    if (old != NULL)
    {
        bytesAvailable += sizeof(Tlv) + old->GetLength();
    }

    VerifyOrExit(sizeof(Tlv) + aTlv.GetLength() <= bytesAvailable, error = OT_ERROR_NO_BUFS);

    // remove old TLV
    if (old != NULL)
    {
        Remove(reinterpret_cast<uint8_t *>(old), sizeof(Tlv) + old->GetLength());
    }

    // add new TLV
    memcpy(mTlvs + mLength, &aTlv, sizeof(Tlv) + aTlv.GetLength());
    mLength += sizeof(Tlv) + aTlv.GetLength();

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
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

void Dataset::Remove(Tlv::Type aType)
{
    Tlv *tlv;

    VerifyOrExit((tlv = Get(aType)) != NULL);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(Tlv) + tlv->GetLength());

exit:
    return;
}

otError Dataset::AppendMleDatasetTlv(Message &aMessage) const
{
    otError        error = OT_ERROR_NONE;
    Mle::Tlv       tlv;
    Mle::Tlv::Type type;
    const Tlv *    cur = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *    end = reinterpret_cast<const Tlv *>(mTlvs + mLength);

    VerifyOrExit(mLength > 0);

    type = (mType == Tlv::kActiveTimestamp ? Mle::Tlv::kActiveDataset : Mle::Tlv::kPendingDataset);

    tlv.SetType(type);
    tlv.SetLength(static_cast<uint8_t>(mLength) - sizeof(Tlv) - sizeof(Timestamp));
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv)));

    while (cur < end)
    {
        if (cur->GetType() == mType)
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

            SuccessOrExit(error = aMessage.Append(&delayTimer, sizeof(delayTimer)));
        }
        else
        {
            SuccessOrExit(error = aMessage.Append(cur, sizeof(Tlv) + cur->GetLength()));
        }

        cur = cur->GetNext();
    }

exit:
    return error;
}

void Dataset::Remove(uint8_t *aStart, uint8_t aLength)
{
    memmove(aStart, aStart + aLength, mLength - (static_cast<uint8_t>(aStart - mTlvs) + aLength));
    mLength -= aLength;
}

otError Dataset::ApplyConfiguration(Instance &aInstance, bool *aIsMasterKeyUpdated) const
{
    ThreadNetif &netif = aInstance.GetThreadNetif();
    Mac::Mac &   mac   = netif.GetMac();
    otError      error = OT_ERROR_NONE;
    const Tlv *  cur   = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *  end   = reinterpret_cast<const Tlv *>(mTlvs + mLength);

    VerifyOrExit(IsValid(), error = OT_ERROR_PARSE);

    if (aIsMasterKeyUpdated)
    {
        *aIsMasterKeyUpdated = false;
    }

    while (cur < end)
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
        {
            const NetworkNameTlv *name = static_cast<const NetworkNameTlv *>(cur);
            mac.SetNetworkName(name->GetNetworkName(), name->GetLength());
            break;
        }

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *key = static_cast<const NetworkMasterKeyTlv *>(cur);

            if (aIsMasterKeyUpdated &&
                memcmp(&key->GetNetworkMasterKey(), &netif.GetKeyManager().GetMasterKey(), OT_MASTER_KEY_SIZE))
            {
                *aIsMasterKeyUpdated = true;
            }

            netif.GetKeyManager().SetMasterKey(key->GetNetworkMasterKey());
            break;
        }

#if OPENTHREAD_FTD

        case Tlv::kPSKc:
        {
            const PSKcTlv *pskc = static_cast<const PSKcTlv *>(cur);
            netif.GetKeyManager().SetPSKc(pskc->GetPSKc());
            break;
        }

#endif

        case Tlv::kMeshLocalPrefix:
        {
            const MeshLocalPrefixTlv *prefix = static_cast<const MeshLocalPrefixTlv *>(cur);
            netif.GetMle().SetMeshLocalPrefix(prefix->GetMeshLocalPrefix());
            break;
        }

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *securityPolicy = static_cast<const SecurityPolicyTlv *>(cur);
            netif.GetKeyManager().SetKeyRotation(securityPolicy->GetRotationTime());
            netif.GetKeyManager().SetSecurityPolicyFlags(securityPolicy->GetFlags());
            break;
        }

        default:
        {
            break;
        }
        }

        cur = cur->GetNext();
    }

exit:
    return error;
}

otError Dataset::ConvertToActive(void)
{
    Remove(Tlv::kPendingTimestamp);
    Remove(Tlv::kDelayTimer);
    mType = Tlv::kActiveTimestamp;
    return OT_ERROR_NONE;
}

} // namespace MeshCoP
} // namespace ot
