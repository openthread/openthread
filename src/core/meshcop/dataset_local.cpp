/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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

#define WPP_NAME "dataset_local.tmh"

#include <openthread/config.h>

#include "dataset_local.hpp"

#include <stdio.h>

#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

DatasetLocal::DatasetLocal(otInstance &aInstance, const Tlv::Type aType) :
    InstanceLocator(aInstance),
    mUpdateTime(0),
    mType(aType)
{
}

void DatasetLocal::Clear(void)
{
    otPlatSettingsDelete(&GetInstance(), GetSettingsKey(), -1);
}

bool DatasetLocal::IsPresent(void) const
{
    return otPlatSettingsGet(&GetInstance(), GetSettingsKey(), 0, NULL, NULL) == OT_ERROR_NONE;
}

otError DatasetLocal::Get(Dataset &aDataset)
{
    DelayTimerTlv *delayTimer;
    uint32_t elapsed;
    otError error;

    aDataset.mLength = sizeof(aDataset.mTlvs);
    error = otPlatSettingsGet(&GetInstance(), GetSettingsKey(), 0, aDataset.mTlvs, &aDataset.mLength);
    VerifyOrExit(error == OT_ERROR_NONE, aDataset.mLength = 0);

    delayTimer = static_cast<DelayTimerTlv *>(aDataset.Get(Tlv::kDelayTimer));
    VerifyOrExit(delayTimer);

    elapsed = TimerMilli::GetNow() - mUpdateTime;

    if (delayTimer->GetDelayTimer() > elapsed)
    {
        delayTimer->SetDelayTimer(delayTimer->GetDelayTimer() - elapsed);
    }
    else
    {
        delayTimer->SetDelayTimer(0);
    }

    aDataset.mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

otError DatasetLocal::Get(otOperationalDataset &aDataset) const
{
    Dataset    dataset(mType);
    otError    error;
    const Tlv *cur;
    const Tlv *end;

    memset(&aDataset, 0, sizeof(aDataset));

    dataset.mLength = sizeof(dataset.mTlvs);
    error = otPlatSettingsGet(&GetInstance(), GetSettingsKey(), 0, dataset.mTlvs, &dataset.mLength);
    SuccessOrExit(error);

    cur = reinterpret_cast<const Tlv *>(dataset.mTlvs);
    end = reinterpret_cast<const Tlv *>(dataset.mTlvs + dataset.mLength);

    while (cur < end)
    {
        switch (cur->GetType())
        {

        case Tlv::kActiveTimestamp:
        {
            const ActiveTimestampTlv *tlv = static_cast<const ActiveTimestampTlv *>(cur);
            aDataset.mActiveTimestamp = tlv->GetSeconds();
            aDataset.mIsActiveTimestampSet = true;
            break;
        }

        case Tlv::kChannel:
        {
            const ChannelTlv *tlv = static_cast<const ChannelTlv *>(cur);
            aDataset.mChannel = tlv->GetChannel();
            aDataset.mIsChannelSet = true;
            break;
        }

        case Tlv::kChannelMask:
        {
            uint8_t tlvLength = cur->GetLength();
            const uint8_t *entry = reinterpret_cast<const uint8_t *>(cur) + sizeof(Tlv);
            const uint8_t *entryEnd =  entry + tlvLength;

            while (entry < entryEnd)
            {
                if (reinterpret_cast<const ChannelMaskEntry *>(entry)->GetChannelPage() == 0)
                {
                    uint8_t i = sizeof(ChannelMaskEntry);
                    aDataset.mChannelMaskPage0 = static_cast<uint32_t>(entry[i] | (entry[i + 1] << 8) |
                                                                       (entry[i + 2] << 16) | (entry[i + 3] << 24));
                    aDataset.mIsChannelMaskPage0Set = true;
                    break;
                }

                entry += (reinterpret_cast<const ChannelMaskEntry *>(entry)->GetMaskLength() +
                          sizeof(ChannelMaskEntry));
            }

            break;
        }

        case Tlv::kDelayTimer:
        {
            const DelayTimerTlv *tlv = static_cast<const DelayTimerTlv *>(cur);
            aDataset.mDelay = tlv->GetDelayTimer();
            aDataset.mIsDelaySet = true;
            break;
        }

        case Tlv::kExtendedPanId:
        {
            const ExtendedPanIdTlv *tlv = static_cast<const ExtendedPanIdTlv *>(cur);
            memcpy(aDataset.mExtendedPanId.m8, tlv->GetExtendedPanId(), sizeof(aDataset.mExtendedPanId));
            aDataset.mIsExtendedPanIdSet = true;
            break;
        }

        case Tlv::kMeshLocalPrefix:
        {
            const MeshLocalPrefixTlv *tlv = static_cast<const MeshLocalPrefixTlv *>(cur);
            memcpy(aDataset.mMeshLocalPrefix.m8, tlv->GetMeshLocalPrefix(), sizeof(aDataset.mMeshLocalPrefix));
            aDataset.mIsMeshLocalPrefixSet = true;
            break;
        }

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *tlv = static_cast<const NetworkMasterKeyTlv *>(cur);
            aDataset.mMasterKey = tlv->GetNetworkMasterKey();
            aDataset.mIsMasterKeySet = true;
            break;
        }

        case Tlv::kNetworkName:
        {
            const NetworkNameTlv *tlv = static_cast<const NetworkNameTlv *>(cur);
            memcpy(aDataset.mNetworkName.m8, tlv->GetNetworkName(), tlv->GetLength());
            aDataset.mNetworkName.m8[tlv->GetLength()] = '\0';
            aDataset.mIsNetworkNameSet = true;
            break;
        }

        case Tlv::kPanId:
        {
            const PanIdTlv *panid = static_cast<const PanIdTlv *>(cur);
            aDataset.mPanId = panid->GetPanId();
            aDataset.mIsPanIdSet = true;
            break;
        }

        case Tlv::kPendingTimestamp:
        {
            const PendingTimestampTlv *tlv = static_cast<const PendingTimestampTlv *>(cur);
            aDataset.mPendingTimestamp = tlv->GetSeconds();
            aDataset.mIsPendingTimestampSet = true;
            break;
        }

        case Tlv::kPSKc:
        {
            const PSKcTlv *tlv = static_cast<const PSKcTlv *>(cur);
            memcpy(aDataset.mPSKc.m8, tlv->GetPSKc(), tlv->GetLength());
            aDataset.mIsPSKcSet = true;
            break;
        }

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *tlv = static_cast<const SecurityPolicyTlv *>(cur);
            aDataset.mSecurityPolicy.mRotationTime = tlv->GetRotationTime();
            aDataset.mSecurityPolicy.mFlags = tlv->GetFlags();
            aDataset.mIsSecurityPolicySet = true;
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

#if OPENTHREAD_FTD

otError DatasetLocal::Set(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mType);
    MeshCoP::ActiveTimestampTlv activeTimestampTlv;

    VerifyOrExit(aDataset.mIsActiveTimestampSet, error = OT_ERROR_INVALID_ARGS);

    activeTimestampTlv.Init();
    activeTimestampTlv.SetSeconds(aDataset.mActiveTimestamp);
    activeTimestampTlv.SetTicks(0);
    dataset.Set(activeTimestampTlv);

    if (mType == Tlv::kPendingTimestamp)
    {
        MeshCoP::PendingTimestampTlv pendingTimestampTlv;

        VerifyOrExit(aDataset.mIsPendingTimestampSet, error = OT_ERROR_INVALID_ARGS);

        pendingTimestampTlv.Init();
        pendingTimestampTlv.SetSeconds(aDataset.mPendingTimestamp);
        pendingTimestampTlv.SetTicks(0);
        dataset.Set(pendingTimestampTlv);

        if (aDataset.mIsDelaySet)
        {
            MeshCoP::DelayTimerTlv tlv;
            tlv.Init();
            tlv.SetDelayTimer(aDataset.mDelay);
            dataset.Set(tlv);
        }
    }

    if (aDataset.mIsChannelSet)
    {
        MeshCoP::ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannelPage(0);
        tlv.SetChannel(aDataset.mChannel);
        dataset.Set(tlv);
    }

    if (aDataset.mIsChannelMaskPage0Set)
    {
        MeshCoP::ChannelMask0Tlv tlv;
        tlv.Init();
        tlv.SetMask(aDataset.mChannelMaskPage0);
        dataset.Set(tlv);
    }

    if (aDataset.mIsExtendedPanIdSet)
    {
        MeshCoP::ExtendedPanIdTlv tlv;
        tlv.Init();
        tlv.SetExtendedPanId(aDataset.mExtendedPanId.m8);
        dataset.Set(tlv);
    }

    if (aDataset.mIsMeshLocalPrefixSet)
    {
        MeshCoP::MeshLocalPrefixTlv tlv;
        tlv.Init();
        tlv.SetMeshLocalPrefix(aDataset.mMeshLocalPrefix.m8);
        dataset.Set(tlv);
    }

    if (aDataset.mIsMasterKeySet)
    {
        MeshCoP::NetworkMasterKeyTlv tlv;
        tlv.Init();
        tlv.SetNetworkMasterKey(aDataset.mMasterKey);
        dataset.Set(tlv);
    }

    if (aDataset.mIsNetworkNameSet)
    {
        MeshCoP::NetworkNameTlv tlv;
        tlv.Init();
        tlv.SetNetworkName(aDataset.mNetworkName.m8);
        dataset.Set(tlv);
    }

    if (aDataset.mIsPanIdSet)
    {
        MeshCoP::PanIdTlv tlv;
        tlv.Init();
        tlv.SetPanId(aDataset.mPanId);
        dataset.Set(tlv);
    }

    if (aDataset.mIsPSKcSet)
    {
        MeshCoP::PSKcTlv tlv;
        tlv.Init();
        tlv.SetPSKc(aDataset.mPSKc.m8);
        dataset.Set(tlv);
    }

    if (aDataset.mIsSecurityPolicySet)
    {
        MeshCoP::SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(aDataset.mSecurityPolicy.mRotationTime);
        tlv.SetFlags(aDataset.mSecurityPolicy.mFlags);
        dataset.Set(tlv);
    }

    if (dataset.GetSize() == 0)
    {
        error = otPlatSettingsDelete(&GetInstance(), GetSettingsKey(), 0);
        otLogInfoMeshCoP(GetInstance(), "%s dataset deleted", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }
    else
    {
        error = otPlatSettingsSet(&GetInstance(), GetSettingsKey(), dataset.GetBytes(), dataset.GetSize());
        otLogInfoMeshCoP(GetInstance(), "%s dataset set", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }

exit:
    return error;
}

#endif  // OPENTHREAD_FTD

otError DatasetLocal::Set(const Dataset &aDataset)
{
    Dataset dataset(aDataset);
    otError error;

    if (mType == Tlv::kActiveTimestamp)
    {
        dataset.Remove(Tlv::kPendingTimestamp);
        dataset.Remove(Tlv::kDelayTimer);
    }

    if (dataset.GetSize() == 0)
    {
        error = otPlatSettingsDelete(&GetInstance(), GetSettingsKey(), 0);
        otLogInfoMeshCoP(GetInstance(), "%s dataset deleted", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }
    else
    {
        error = otPlatSettingsSet(&GetInstance(), GetSettingsKey(), dataset.GetBytes(), dataset.GetSize());
        otLogInfoMeshCoP(GetInstance(), "%s dataset set", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }

    SuccessOrExit(error);

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

uint16_t DatasetLocal::GetSettingsKey(void) const
{
    uint16_t rval;

    if (mType == Tlv::kActiveTimestamp)
    {
        rval = static_cast<uint16_t>(Settings::kKeyActiveDataset);
    }
    else
    {
        rval = static_cast<uint16_t>(Settings::kKeyPendingDataset);
    }

    return rval;
}

int DatasetLocal::Compare(const Timestamp *aCompareTimestamp)
{
    const Timestamp *thisTimestamp;
    Dataset dataset(mType);
    int rval = 1;

    SuccessOrExit(Get(dataset));

    thisTimestamp = dataset.GetTimestamp();

    if (aCompareTimestamp == NULL && thisTimestamp == NULL)
    {
        rval = 0;
    }
    else if (aCompareTimestamp == NULL && thisTimestamp != NULL)
    {
        rval = -1;
    }
    else if (aCompareTimestamp != NULL && thisTimestamp == NULL)
    {
        rval = 1;
    }
    else
    {
        rval = thisTimestamp->Compare(*aCompareTimestamp);
    }

exit:
    return rval;
}

}  // namespace MeshCoP
}  // namespace ot
