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
 *   This file implements MeshCoP Datasets manager to process commands.
 *
 */

#include "dataset_manager.hpp"

#include <stdio.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

DatasetManager::DatasetManager(Instance &     aInstance,
                               Dataset::Type  aType,
                               const char *   aUriGet,
                               const char *   aUriSet,
                               Timer::Handler aTimerHandler)
    : InstanceLocator(aInstance)
    , mLocal(aInstance, aType)
    , mTimestampValid(false)
    , mTimer(aInstance, aTimerHandler, this)
    , mUriGet(aUriGet)
    , mUriSet(aUriSet)
    , mCoapPending(false)
{
    mTimestamp.Init();
}

const Timestamp *DatasetManager::GetTimestamp(void) const
{
    return mTimestampValid ? &mTimestamp : nullptr;
}

int DatasetManager::Compare(const Timestamp &aTimestamp) const
{
    const Timestamp *timestamp = GetTimestamp();
    int              rval      = 1;

    if (timestamp)
    {
        rval = timestamp->Compare(aTimestamp);
    }

    return rval;
}

otError DatasetManager::Restore(void)
{
    otError          error;
    Dataset          dataset(mLocal.GetType());
    const Timestamp *timestamp;

    mTimer.Stop();

    mTimestampValid = false;

    SuccessOrExit(error = mLocal.Restore(dataset));

    timestamp = dataset.GetTimestamp();

    if (timestamp != nullptr)
    {
        mTimestamp      = *timestamp;
        mTimestampValid = true;
    }

    if (mLocal.GetType() == Dataset::kActive)
    {
        IgnoreError(dataset.ApplyConfiguration(GetInstance()));
    }

exit:
    return error;
}

otError DatasetManager::ApplyConfiguration(void) const
{
    otError error;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = mLocal.Read(dataset));
    SuccessOrExit(error = dataset.ApplyConfiguration(GetInstance()));

exit:
    return error;
}

void DatasetManager::Clear(void)
{
    mTimestamp.Init();
    mTimestampValid = false;
    mLocal.Clear();
    mTimer.Stop();
}

void DatasetManager::HandleDetach(void)
{
    IgnoreError(Restore());
}

otError DatasetManager::Save(const Dataset &aDataset)
{
    otError          error = OT_ERROR_NONE;
    const Timestamp *timestamp;
    int              compare;
    bool             isMasterkeyUpdated = false;

    timestamp = aDataset.GetTimestamp();

    if (timestamp != nullptr)
    {
        mTimestamp      = *timestamp;
        mTimestampValid = true;

        if (mLocal.GetType() == Dataset::kActive)
        {
            SuccessOrExit(error = aDataset.ApplyConfiguration(GetInstance(), &isMasterkeyUpdated));
        }
    }

    compare = mLocal.Compare(timestamp);

    if (isMasterkeyUpdated || compare > 0)
    {
        IgnoreError(mLocal.Save(aDataset));

#if OPENTHREAD_FTD
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
#endif
    }
    else if (compare < 0)
    {
        VerifyOrExit(!Get<Mle::MleRouter>().IsLeader(), error = OT_ERROR_INVALID_STATE);
        SendSet();
    }

exit:
    return error;
}

otError DatasetManager::Save(const otOperationalDataset &aDataset)
{
    otError error;

    SuccessOrExit(error = mLocal.Save(aDataset));
    HandleDatasetUpdated();

exit:
    return error;
}

otError DatasetManager::Save(const otOperationalDatasetTlvs &aDataset)
{
    otError error;

    SuccessOrExit(error = mLocal.Save(aDataset));
    HandleDatasetUpdated();

exit:
    return error;
}

void DatasetManager::HandleDatasetUpdated(void)
{
    switch (Get<Mle::MleRouter>().GetRole())
    {
    case Mle::kRoleDisabled:
        IgnoreError(Restore());
        break;

    case Mle::kRoleChild:
        SendSet();
        break;
#if OPENTHREAD_FTD
    case Mle::kRoleRouter:
        SendSet();
        break;

    case Mle::kRoleLeader:
        IgnoreError(Restore());
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
        break;
#endif

    default:
        break;
    }
}

otError DatasetManager::GetChannelMask(Mac::ChannelMask &aChannelMask) const
{
    otError                        error;
    const MeshCoP::ChannelMaskTlv *channelMaskTlv;
    uint32_t                       mask;
    Dataset                        dataset(mLocal.GetType());

    SuccessOrExit(error = mLocal.Read(dataset));

    channelMaskTlv = dataset.GetTlv<ChannelMaskTlv>();
    VerifyOrExit(channelMaskTlv != nullptr, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit((mask = channelMaskTlv->GetChannelMask()) != 0, OT_NOOP);

    aChannelMask.SetMask(mask & Get<Mac::Mac>().GetSupportedChannelMask().GetMask());

    VerifyOrExit(!aChannelMask.IsEmpty(), error = OT_ERROR_NOT_FOUND);

exit:
    return error;
}

void DatasetManager::HandleTimer(void)
{
    SendSet();
}

void DatasetManager::SendSet(void)
{
    otError          error;
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    Dataset          dataset(mLocal.GetType());

    VerifyOrExit(!mCoapPending, error = OT_ERROR_BUSY);
    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mLocal.Compare(GetTimestamp()) < 0, error = OT_ERROR_INVALID_STATE);

    if (mLocal.GetType() == Dataset::kActive)
    {
        Dataset pendingDataset(Dataset::kPending);
        IgnoreError(Get<PendingDataset>().Read(pendingDataset));

        const ActiveTimestampTlv *tlv                    = pendingDataset.GetTlv<ActiveTimestampTlv>();
        const Timestamp *         pendingActiveTimestamp = static_cast<const Timestamp *>(tlv);

        if (pendingActiveTimestamp != nullptr && mLocal.Compare(pendingActiveTimestamp) == 0)
        {
            // stop registration attempts during dataset transition
            ExitNow(error = OT_ERROR_INVALID_STATE);
        }
    }

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsConfirmablePost(mUriSet));
    SuccessOrExit(error = message->SetPayloadMarker());

    IgnoreError(mLocal.Read(dataset));
    SuccessOrExit(error = message->Append(dataset.GetBytes(), dataset.GetSize()));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    IgnoreError(Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    SuccessOrExit(
        error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo, &DatasetManager::HandleCoapResponse, this));

    otLogInfoMeshCoP("Sent %s to leader", mUriSet);

exit:

    switch (error)
    {
    case OT_ERROR_NONE:
        mCoapPending = true;
        break;

    case OT_ERROR_NO_BUFS:
        mTimer.Start(kDelayNoBufs);
        // fall through

    default:
        otLogWarnMeshCoP("Failed to send %s to leader: %s", mUriSet, otThreadErrorToString(error));
        FreeMessage(message);
        break;
    }
}

void DatasetManager::HandleCoapResponse(void *               aContext,
                                        otMessage *          aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        otError              aError)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);
    OT_UNUSED_VARIABLE(aError);

    static_cast<DatasetManager *>(aContext)->HandleCoapResponse();
}

void DatasetManager::HandleCoapResponse(void)
{
    mCoapPending = false;
    SendSet();
}

void DatasetManager::HandleGet(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    Tlv      tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t  tlvs[Dataset::kMaxGetTypes];
    uint8_t  length = 0;

    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == Tlv::kGet)
        {
            length = tlv.GetLength();

            if (length > (sizeof(tlvs) - 1))
            {
                // leave space for potential DelayTimer type below
                length = sizeof(tlvs) - 1;
            }

            aMessage.Read(offset + sizeof(Tlv), length, tlvs);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    // MGMT_PENDING_GET.rsp must include Delay Timer TLV (Thread 1.1.1 Section 8.7.5.4)
    VerifyOrExit(length > 0 && strcmp(mUriGet, UriPath::kPendingGet) == 0, OT_NOOP);

    for (uint8_t i = 0; i < length; i++)
    {
        if (tlvs[i] == Tlv::kDelayTimer)
        {
            ExitNow();
        }
    }

    tlvs[length++] = Tlv::kDelayTimer;

exit:
    SendGetResponse(aMessage, aMessageInfo, tlvs, length);
}

void DatasetManager::SendGetResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     uint8_t *               aTlvs,
                                     uint8_t                 aLength) const
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;
    Dataset        dataset(mLocal.GetType());

    IgnoreError(mLocal.Read(dataset));

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    if (aLength == 0)
    {
        for (const Tlv *cur = dataset.GetTlvsStart(); cur < dataset.GetTlvsEnd(); cur = cur->GetNext())
        {
            if (cur->GetType() != Tlv::kNetworkMasterKey || Get<KeyManager>().IsObtainMasterKeyEnabled())
            {
                SuccessOrExit(error = cur->AppendTo(*message));
            }
        }
    }
    else
    {
        for (uint8_t index = 0; index < aLength; index++)
        {
            const Tlv *tlv;

            if (aTlvs[index] == Tlv::kNetworkMasterKey && !Get<KeyManager>().IsObtainMasterKeyEnabled())
            {
                continue;
            }

            if ((tlv = dataset.GetTlv(static_cast<Tlv::Type>(aTlvs[index]))) != nullptr)
            {
                SuccessOrExit(error = tlv->AppendTo(*message));
            }
        }
    }

    if (message->GetLength() == message->GetOffset())
    {
        // no payload, remove coap payload marker
        IgnoreError(message->SetLength(message->GetLength() - 1));
    }

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset get response");

exit:
    FreeMessageOnError(message, error);
}

otError DatasetManager::SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsConfirmablePost(mUriSet));
    SuccessOrExit(error = message->SetPayloadMarker());

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

    if (Get<Commissioner>().IsActive())
    {
        const Tlv *end          = reinterpret_cast<const Tlv *>(aTlvs + aLength);
        bool       hasSessionId = false;

        for (const Tlv *cur = reinterpret_cast<const Tlv *>(aTlvs); cur < end; cur = cur->GetNext())
        {
            VerifyOrExit((cur + 1) <= end, error = OT_ERROR_INVALID_ARGS);

            if (cur->GetType() == Tlv::kCommissionerSessionId)
            {
                hasSessionId = true;
                break;
            }
        }

        if (!hasSessionId)
        {
            SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kCommissionerSessionId,
                                                       Get<Commissioner>().GetSessionId()));
        }
    }

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

    if (aDataset.mComponents.mIsActiveTimestampPresent)
    {
        ActiveTimestampTlv timestamp;
        timestamp.Init();
        timestamp.SetSeconds(aDataset.mActiveTimestamp);
        timestamp.SetTicks(0);
        SuccessOrExit(error = timestamp.AppendTo(*message));
    }

    if (aDataset.mComponents.mIsPendingTimestampPresent)
    {
        PendingTimestampTlv timestamp;
        timestamp.Init();
        timestamp.SetSeconds(aDataset.mPendingTimestamp);
        timestamp.SetTicks(0);
        SuccessOrExit(error = timestamp.AppendTo(*message));
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        SuccessOrExit(error =
                          Tlv::AppendTlv(*message, Tlv::kNetworkMasterKey, &aDataset.mMasterKey, sizeof(MasterKey)));
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        NetworkNameTlv networkname;
        networkname.Init();
        networkname.SetNetworkName(static_cast<const Mac::NetworkName &>(aDataset.mNetworkName).GetAsData());
        SuccessOrExit(error = networkname.AppendTo(*message));
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kExtendedPanId, &aDataset.mExtendedPanId,
                                             sizeof(Mac::ExtendedPanId)));
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kMeshLocalPrefix, &aDataset.mMeshLocalPrefix,
                                             sizeof(otMeshLocalPrefix)));
    }

    if (aDataset.mComponents.mIsDelayPresent)
    {
        SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, Tlv::kDelayTimer, aDataset.mDelay));
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kPanId, aDataset.mPanId));
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        ChannelTlv channel;
        channel.Init();
        channel.SetChannel(aDataset.mChannel);
        SuccessOrExit(error = channel.AppendTo(*message));
    }

    if (aDataset.mComponents.mIsChannelMaskPresent)
    {
        ChannelMaskTlv channelMask;
        channelMask.Init();
        channelMask.SetChannelMask(aDataset.mChannelMask);
        SuccessOrExit(error = channelMask.AppendTo(*message));
    }

    if (aDataset.mComponents.mIsPskcPresent)
    {
        SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kPskc, aDataset.mPskc.m8, sizeof(Pskc)));
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        SecurityPolicyTlv securityPolicy;
        securityPolicy.Init();
        securityPolicy.SetRotationTime(aDataset.mSecurityPolicy.mRotationTime);
        securityPolicy.SetFlags(aDataset.mSecurityPolicy.mFlags);
        SuccessOrExit(error = securityPolicy.AppendTo(*message));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    if (message->GetLength() == message->GetOffset())
    {
        // no payload, remove coap payload marker
        IgnoreError(message->SetLength(message->GetLength() - 1));
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    IgnoreError(Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset set request to leader");

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError DatasetManager::SendGetRequest(const otOperationalDatasetComponents &aDatasetComponents,
                                       const uint8_t *                       aTlvTypes,
                                       uint8_t                               aLength,
                                       const otIp6Address *                  aAddress) const
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;
    Tlv              tlv;
    uint8_t          datasetTlvs[kMaxDatasetTlvs];
    uint8_t          length;

    length = 0;

    if (aDatasetComponents.mIsActiveTimestampPresent)
    {
        datasetTlvs[length++] = Tlv::kActiveTimestamp;
    }

    if (aDatasetComponents.mIsPendingTimestampPresent)
    {
        datasetTlvs[length++] = Tlv::kPendingTimestamp;
    }

    if (aDatasetComponents.mIsMasterKeyPresent)
    {
        datasetTlvs[length++] = Tlv::kNetworkMasterKey;
    }

    if (aDatasetComponents.mIsNetworkNamePresent)
    {
        datasetTlvs[length++] = Tlv::kNetworkName;
    }

    if (aDatasetComponents.mIsExtendedPanIdPresent)
    {
        datasetTlvs[length++] = Tlv::kExtendedPanId;
    }

    if (aDatasetComponents.mIsMeshLocalPrefixPresent)
    {
        datasetTlvs[length++] = Tlv::kMeshLocalPrefix;
    }

    if (aDatasetComponents.mIsDelayPresent)
    {
        datasetTlvs[length++] = Tlv::kDelayTimer;
    }

    if (aDatasetComponents.mIsPanIdPresent)
    {
        datasetTlvs[length++] = Tlv::kPanId;
    }

    if (aDatasetComponents.mIsChannelPresent)
    {
        datasetTlvs[length++] = Tlv::kChannel;
    }

    if (aDatasetComponents.mIsPskcPresent)
    {
        datasetTlvs[length++] = Tlv::kPskc;
    }

    if (aDatasetComponents.mIsSecurityPolicyPresent)
    {
        datasetTlvs[length++] = Tlv::kSecurityPolicy;
    }

    if (aDatasetComponents.mIsChannelMaskPresent)
    {
        datasetTlvs[length++] = Tlv::kChannelMask;
    }

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsConfirmablePost(mUriGet));

    if (aLength + length > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    if (aLength + length > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength + length);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

        if (length > 0)
        {
            SuccessOrExit(error = message->Append(datasetTlvs, length));
        }

        if (aLength > 0)
        {
            SuccessOrExit(error = message->Append(aTlvTypes, aLength));
        }
    }

    if (aAddress != nullptr)
    {
        messageInfo.SetPeerAddr(*static_cast<const Ip6::Address *>(aAddress));
    }
    else
    {
        IgnoreError(Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset get request");

exit:
    FreeMessageOnError(message, error);
    return error;
}

ActiveDataset::ActiveDataset(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kActive, UriPath::kActiveGet, UriPath::kActiveSet, ActiveDataset::HandleTimer)
    , mResourceGet(UriPath::kActiveGet, &ActiveDataset::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(UriPath::kActiveSet, &ActiveDataset::HandleSet, this)
#endif
{
    Get<Tmf::TmfAgent>().AddResource(mResourceGet);
}

bool ActiveDataset::IsPartiallyComplete(void) const
{
    return mLocal.IsSaved() && !mTimestampValid;
}

otError ActiveDataset::Save(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = dataset.Set(aMessage, aOffset, aLength));
    dataset.SetTimestamp(aTimestamp);
    IgnoreError(DatasetManager::Save(dataset));

exit:
    return error;
}

void ActiveDataset::HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleGet(*static_cast<Coap::Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void ActiveDataset::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ActiveDataset>().HandleTimer();
}

PendingDataset::PendingDataset(Instance &aInstance)
    : DatasetManager(aInstance,
                     Dataset::kPending,
                     UriPath::kPendingGet,
                     UriPath::kPendingSet,
                     PendingDataset::HandleTimer)
    , mDelayTimer(aInstance, PendingDataset::HandleDelayTimer, this)
    , mResourceGet(UriPath::kPendingGet, &PendingDataset::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(UriPath::kPendingSet, &PendingDataset::HandleSet, this)
#endif
{
    Get<Tmf::TmfAgent>().AddResource(mResourceGet);
}

void PendingDataset::Clear(void)
{
    DatasetManager::Clear();
    mDelayTimer.Stop();
}

void PendingDataset::ClearNetwork(void)
{
    Dataset dataset(mLocal.GetType());

    mTimestamp.Init();
    mTimestampValid = false;
    IgnoreError(DatasetManager::Save(dataset));
}

otError PendingDataset::Save(const otOperationalDataset &aDataset)
{
    otError error;

    SuccessOrExit(error = DatasetManager::Save(aDataset));
    StartDelayTimer();

exit:
    return error;
}

otError PendingDataset::Save(const otOperationalDatasetTlvs &aDataset)
{
    otError error;

    SuccessOrExit(error = DatasetManager::Save(aDataset));
    StartDelayTimer();

exit:
    return error;
}

otError PendingDataset::Save(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = dataset.Set(aMessage, aOffset, aLength));
    dataset.SetTimestamp(aTimestamp);
    IgnoreError(DatasetManager::Save(dataset));
    StartDelayTimer();

exit:
    return error;
}

void PendingDataset::StartDelayTimer(void)
{
    DelayTimerTlv *delayTimer;
    Dataset        dataset(mLocal.GetType());

    IgnoreError(mLocal.Read(dataset));

    mDelayTimer.Stop();

    if ((delayTimer = dataset.GetTlv<DelayTimerTlv>()) != nullptr)
    {
        uint32_t delay = delayTimer->GetDelayTimer();

        // the Timer implementation does not support the full 32 bit range
        if (delay > Timer::kMaxDelay)
        {
            delay = Timer::kMaxDelay;
        }

        mDelayTimer.StartAt(dataset.GetUpdateTime(), delay);
        otLogInfoMeshCoP("delay timer started %d", delay);
    }
}

void PendingDataset::HandleDelayTimer(Timer &aTimer)
{
    aTimer.GetOwner<PendingDataset>().HandleDelayTimer();
}

void PendingDataset::HandleDelayTimer(void)
{
    DelayTimerTlv *delayTimer;
    Dataset        dataset(mLocal.GetType());

    IgnoreError(mLocal.Read(dataset));

    // if the Delay Timer value is larger than what our Timer implementation can handle, we have to compute
    // the remainder and wait some more.
    if ((delayTimer = dataset.GetTlv<DelayTimerTlv>()) != nullptr)
    {
        uint32_t elapsed = mDelayTimer.GetFireTime() - dataset.GetUpdateTime();
        uint32_t delay   = delayTimer->GetDelayTimer();

        if (elapsed < delay)
        {
            mDelayTimer.StartAt(mDelayTimer.GetFireTime(), delay - elapsed);
            ExitNow();
        }
    }

    otLogInfoMeshCoP("pending delay timer expired");

    dataset.ConvertToActive();

    Get<ActiveDataset>().Save(dataset);

    Clear();

exit:
    return;
}

void PendingDataset::HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleGet(*static_cast<Coap::Message *>(aMessage),
                                                       *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void PendingDataset::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<PendingDataset>().HandleTimer();
}

} // namespace MeshCoP
} // namespace ot
