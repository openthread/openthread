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

#include "common/as_core_type.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/notifier.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("DatasetManager");

DatasetManager::DatasetManager(Instance &aInstance, Dataset::Type aType, Timer::Handler aTimerHandler)
    : InstanceLocator(aInstance)
    , mLocal(aInstance, aType)
    , mTimestampValid(false)
    , mMgmtPending(false)
    , mTimer(aInstance, aTimerHandler)
    , mMgmtSetCallback(nullptr)
    , mMgmtSetCallbackContext(nullptr)
{
    mTimestamp.Clear();
}

const Timestamp *DatasetManager::GetTimestamp(void) const
{
    return mTimestampValid ? &mTimestamp : nullptr;
}

Error DatasetManager::Restore(void)
{
    Error   error;
    Dataset dataset;

    mTimer.Stop();

    mTimestampValid = false;

    SuccessOrExit(error = mLocal.Restore(dataset));

    mTimestampValid = (dataset.GetTimestamp(GetType(), mTimestamp) == kErrorNone);

    if (IsActiveDataset())
    {
        IgnoreError(dataset.ApplyConfiguration(GetInstance()));
    }

    SignalDatasetChange();

exit:
    return error;
}

Error DatasetManager::ApplyConfiguration(void) const
{
    Error   error;
    Dataset dataset;

    SuccessOrExit(error = Read(dataset));
    SuccessOrExit(error = dataset.ApplyConfiguration(GetInstance()));

exit:
    return error;
}

void DatasetManager::Clear(void)
{
    mTimestamp.Clear();
    mTimestampValid = false;
    mLocal.Clear();
    mTimer.Stop();
    SignalDatasetChange();
}

void DatasetManager::HandleDetach(void)
{
    IgnoreError(Restore());
}

Error DatasetManager::Save(const Dataset &aDataset)
{
    Error error = kErrorNone;
    int   compare;
    bool  isNetworkkeyUpdated = false;

    if (aDataset.GetTimestamp(GetType(), mTimestamp) == kErrorNone)
    {
        mTimestampValid = true;

        if (IsActiveDataset())
        {
            SuccessOrExit(error = aDataset.ApplyConfiguration(GetInstance(), &isNetworkkeyUpdated));
        }
    }

    compare = Timestamp::Compare(mTimestampValid ? &mTimestamp : nullptr, mLocal.GetTimestamp());

    if (isNetworkkeyUpdated || compare > 0)
    {
        SuccessOrExit(error = mLocal.Save(aDataset));

#if OPENTHREAD_FTD
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
#endif
    }
    else if (compare < 0)
    {
        mTimer.Start(kSendSetDelay);
    }

    SignalDatasetChange();

exit:
    return error;
}

Error DatasetManager::Save(const Dataset::Info &aDatasetInfo)
{
    Error error;

    SuccessOrExit(error = mLocal.Save(aDatasetInfo));
    HandleDatasetUpdated();

exit:
    return error;
}

Error DatasetManager::Save(const otOperationalDatasetTlvs &aDataset)
{
    Error error;

    SuccessOrExit(error = mLocal.Save(aDataset));
    HandleDatasetUpdated();

exit:
    return error;
}

Error DatasetManager::SaveLocal(const Dataset &aDataset)
{
    Error error;

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

    SignalDatasetChange();
}

void DatasetManager::SignalDatasetChange(void) const
{
    Get<Notifier>().Signal(mLocal.GetType() == Dataset::kActive ? kEventActiveDatasetChanged
                                                                : kEventPendingDatasetChanged);
}

Error DatasetManager::GetChannelMask(Mac::ChannelMask &aChannelMask) const
{
    Error                 error;
    const ChannelMaskTlv *channelMaskTlv;
    uint32_t              mask;
    Dataset               dataset;

    SuccessOrExit(error = Read(dataset));

    channelMaskTlv = dataset.GetTlv<ChannelMaskTlv>();
    VerifyOrExit(channelMaskTlv != nullptr, error = kErrorNotFound);
    VerifyOrExit((mask = channelMaskTlv->GetChannelMask()) != 0);

    aChannelMask.SetMask(mask & Get<Mac::Mac>().GetSupportedChannelMask().GetMask());

    VerifyOrExit(!aChannelMask.IsEmpty(), error = kErrorNotFound);

exit:
    return error;
}

void DatasetManager::HandleTimer(void)
{
    SendSet();
}

void DatasetManager::SendSet(void)
{
    Error            error;
    Coap::Message *  message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    Dataset          dataset;

    VerifyOrExit(!mMgmtPending, error = kErrorBusy);
    VerifyOrExit(Get<Mle::MleRouter>().IsChild() || Get<Mle::MleRouter>().IsRouter(), error = kErrorInvalidState);

    VerifyOrExit(Timestamp::Compare(GetTimestamp(), mLocal.GetTimestamp()) < 0, error = kErrorAlready);

    if (IsActiveDataset())
    {
        Dataset   pendingDataset;
        Timestamp timestamp;

        IgnoreError(Get<PendingDatasetManager>().Read(pendingDataset));

        if ((pendingDataset.GetTimestamp(Dataset::kActive, timestamp) == kErrorNone) &&
            (Timestamp::Compare(&timestamp, mLocal.GetTimestamp()) == 0))
        {
            // stop registration attempts during dataset transition
            ExitNow(error = kErrorInvalidState);
        }
    }

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(IsActiveDataset() ? UriPath::kActiveSet
                                                                                    : UriPath::kPendingSet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    IgnoreError(Read(dataset));
    SuccessOrExit(error = message->AppendBytes(dataset.GetBytes(), dataset.GetSize()));

    IgnoreError(messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());
    SuccessOrExit(
        error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, &DatasetManager::HandleMgmtSetResponse, this));

    LogInfo("Sent %s set to leader", Dataset::TypeToString(GetType()));

exit:

    switch (error)
    {
    case kErrorNone:
        mMgmtPending = true;
        break;

    case kErrorNoBufs:
        mTimer.Start(kSendSetDelay);
        OT_FALL_THROUGH;

    default:
        LogError("send Dataset set to leader", error);
        FreeMessage(message);
        break;
    }
}

void DatasetManager::HandleMgmtSetResponse(void *               aContext,
                                           otMessage *          aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           Error                aError)
{
    static_cast<DatasetManager *>(aContext)->HandleMgmtSetResponse(AsCoapMessagePtr(aMessage),
                                                                   AsCoreTypePtr(aMessageInfo), aError);
}

void DatasetManager::HandleMgmtSetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error    error;
    StateTlv stateTlv;

    SuccessOrExit(error = aError);
    VerifyOrExit(Tlv::FindTlv(*aMessage, stateTlv) == kErrorNone, error = kErrorParse);

    switch (stateTlv.GetState())
    {
    case StateTlv::kReject:
        error = kErrorRejected;
        break;
    case StateTlv::kAccept:
        error = kErrorNone;
        break;
    default:
        error = kErrorParse;
        break;
    }

exit:
    LogInfo("MGMT_SET finished: %s", ErrorToString(error));

    mMgmtPending = false;

    if (mMgmtSetCallback != nullptr)
    {
        otDatasetMgmtSetCallback callback = mMgmtSetCallback;
        void *                   context  = mMgmtSetCallbackContext;

        mMgmtSetCallback        = nullptr;
        mMgmtSetCallbackContext = nullptr;

        callback(error, context);
    }

    mTimer.Start(kSendSetDelay);
}

void DatasetManager::HandleGet(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    Tlv      tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t  tlvs[Dataset::kMaxGetTypes];
    uint8_t  length = 0;

    while (offset < aMessage.GetLength())
    {
        SuccessOrExit(aMessage.Read(offset, tlv));

        if (tlv.GetType() == Tlv::kGet)
        {
            length = tlv.GetLength();

            if (length > (sizeof(tlvs) - 1))
            {
                // leave space for potential DelayTimer type below
                length = sizeof(tlvs) - 1;
            }

            aMessage.ReadBytes(offset + sizeof(Tlv), tlvs, length);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    // MGMT_PENDING_GET.rsp must include Delay Timer TLV (Thread 1.1.1 Section 8.7.5.4)
    VerifyOrExit(length > 0 && IsPendingDataset());

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
    Error          error = kErrorNone;
    Coap::Message *message;
    Dataset        dataset;

    IgnoreError(Read(dataset));

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aLength == 0)
    {
        for (const Tlv *cur = dataset.GetTlvsStart(); cur < dataset.GetTlvsEnd(); cur = cur->GetNext())
        {
            if (cur->GetType() != Tlv::kNetworkKey || Get<KeyManager>().GetSecurityPolicy().mObtainNetworkKeyEnabled)
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

            if (aTlvs[index] == Tlv::kNetworkKey && !Get<KeyManager>().GetSecurityPolicy().mObtainNetworkKeyEnabled)
            {
                continue;
            }

            if ((tlv = dataset.GetTlv(static_cast<Tlv::Type>(aTlvs[index]))) != nullptr)
            {
                SuccessOrExit(error = tlv->AppendTo(*message));
            }
        }
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

    LogInfo("sent %s dataset get response to %s", (GetType() == Dataset::kActive ? "active" : "pending"),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
}

Error DatasetManager::AppendDatasetToMessage(const Dataset::Info &aDatasetInfo, Message &aMessage) const
{
    Error   error;
    Dataset dataset;

    SuccessOrExit(error = dataset.SetFrom(aDatasetInfo));
    error = aMessage.AppendBytes(dataset.GetBytes(), dataset.GetSize());

exit:
    return error;
}

Error DatasetManager::SendSetRequest(const Dataset::Info &    aDatasetInfo,
                                     const uint8_t *          aTlvs,
                                     uint8_t                  aLength,
                                     otDatasetMgmtSetCallback aCallback,
                                     void *                   aContext)
{
    Error            error   = kErrorNone;
    Coap::Message *  message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit(!mMgmtPending, error = kErrorBusy);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(IsActiveDataset() ? UriPath::kActiveSet
                                                                                    : UriPath::kPendingSet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

    if (Get<Commissioner>().IsActive())
    {
        const Tlv *end          = reinterpret_cast<const Tlv *>(aTlvs + aLength);
        bool       hasSessionId = false;

        for (const Tlv *cur = reinterpret_cast<const Tlv *>(aTlvs); cur < end; cur = cur->GetNext())
        {
            VerifyOrExit((cur + 1) <= end, error = kErrorInvalidArgs);

            if (cur->GetType() == Tlv::kCommissionerSessionId)
            {
                hasSessionId = true;
                break;
            }
        }

        if (!hasSessionId)
        {
            SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, Get<Commissioner>().GetSessionId()));
        }
    }

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

    SuccessOrExit(error = AppendDatasetToMessage(aDatasetInfo, *message));

    if (aLength > 0)
    {
        SuccessOrExit(error = message->AppendBytes(aTlvs, aLength));
    }

    IgnoreError(messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleMgmtSetResponse, this));
    mMgmtSetCallback        = aCallback;
    mMgmtSetCallbackContext = aContext;
    mMgmtPending            = true;

    LogInfo("sent dataset set request to leader");

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error DatasetManager::SendGetRequest(const Dataset::Components &aDatasetComponents,
                                     const uint8_t *            aTlvTypes,
                                     uint8_t                    aLength,
                                     const otIp6Address *       aAddress) const
{
    Error            error = kErrorNone;
    Coap::Message *  message;
    Tmf::MessageInfo messageInfo(GetInstance());
    Tlv              tlv;
    uint8_t          datasetTlvs[kMaxDatasetTlvs];
    uint8_t          length;

    length = 0;

    if (aDatasetComponents.IsActiveTimestampPresent())
    {
        datasetTlvs[length++] = Tlv::kActiveTimestamp;
    }

    if (aDatasetComponents.IsPendingTimestampPresent())
    {
        datasetTlvs[length++] = Tlv::kPendingTimestamp;
    }

    if (aDatasetComponents.IsNetworkKeyPresent())
    {
        datasetTlvs[length++] = Tlv::kNetworkKey;
    }

    if (aDatasetComponents.IsNetworkNamePresent())
    {
        datasetTlvs[length++] = Tlv::kNetworkName;
    }

    if (aDatasetComponents.IsExtendedPanIdPresent())
    {
        datasetTlvs[length++] = Tlv::kExtendedPanId;
    }

    if (aDatasetComponents.IsMeshLocalPrefixPresent())
    {
        datasetTlvs[length++] = Tlv::kMeshLocalPrefix;
    }

    if (aDatasetComponents.IsDelayPresent())
    {
        datasetTlvs[length++] = Tlv::kDelayTimer;
    }

    if (aDatasetComponents.IsPanIdPresent())
    {
        datasetTlvs[length++] = Tlv::kPanId;
    }

    if (aDatasetComponents.IsChannelPresent())
    {
        datasetTlvs[length++] = Tlv::kChannel;
    }

    if (aDatasetComponents.IsPskcPresent())
    {
        datasetTlvs[length++] = Tlv::kPskc;
    }

    if (aDatasetComponents.IsSecurityPolicyPresent())
    {
        datasetTlvs[length++] = Tlv::kSecurityPolicy;
    }

    if (aDatasetComponents.IsChannelMaskPresent())
    {
        datasetTlvs[length++] = Tlv::kChannelMask;
    }

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(IsActiveDataset() ? UriPath::kActiveGet
                                                                                    : UriPath::kPendingGet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aLength + length > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength + length);
        SuccessOrExit(error = message->Append(tlv));

        if (length > 0)
        {
            SuccessOrExit(error = message->AppendBytes(datasetTlvs, length));
        }

        if (aLength > 0)
        {
            SuccessOrExit(error = message->AppendBytes(aTlvTypes, aLength));
        }
    }

    IgnoreError(messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());

    if (aAddress != nullptr)
    {
        // Use leader ALOC if `aAddress` is `nullptr`.
        messageInfo.SetPeerAddr(AsCoreType(aAddress));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("sent dataset get request");

exit:
    FreeMessageOnError(message, error);
    return error;
}

ActiveDatasetManager::ActiveDatasetManager(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kActive, ActiveDatasetManager::HandleTimer)
    , mResourceGet(UriPath::kActiveGet, &ActiveDatasetManager::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(UriPath::kActiveSet, &ActiveDatasetManager::HandleSet, this)
#endif
{
    Get<Tmf::Agent>().AddResource(mResourceGet);
}

bool ActiveDatasetManager::IsPartiallyComplete(void) const
{
    return mLocal.IsSaved() && !mTimestampValid;
}

bool ActiveDatasetManager::IsCommissioned(void) const
{
    Dataset::Info datasetInfo;
    bool          isValid = false;

    SuccessOrExit(Read(datasetInfo));

    isValid = (datasetInfo.IsNetworkKeyPresent() && datasetInfo.IsNetworkNamePresent() &&
               datasetInfo.IsExtendedPanIdPresent() && datasetInfo.IsPanIdPresent() && datasetInfo.IsChannelPresent());

exit:
    return isValid;
}

Error ActiveDatasetManager::Save(const Timestamp &aTimestamp,
                                 const Message &  aMessage,
                                 uint16_t         aOffset,
                                 uint8_t          aLength)
{
    Error   error = kErrorNone;
    Dataset dataset;

    SuccessOrExit(error = dataset.ReadFromMessage(aMessage, aOffset, aLength));
    dataset.SetTimestamp(Dataset::kActive, aTimestamp);
    error = DatasetManager::Save(dataset);

exit:
    return error;
}

void ActiveDatasetManager::HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDatasetManager *>(aContext)->HandleGet(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void ActiveDatasetManager::HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void ActiveDatasetManager::HandleTimer(Timer &aTimer)
{
    aTimer.Get<ActiveDatasetManager>().HandleTimer();
}

PendingDatasetManager::PendingDatasetManager(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kPending, PendingDatasetManager::HandleTimer)
    , mDelayTimer(aInstance, PendingDatasetManager::HandleDelayTimer)
    , mResourceGet(UriPath::kPendingGet, &PendingDatasetManager::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(UriPath::kPendingSet, &PendingDatasetManager::HandleSet, this)
#endif
{
    Get<Tmf::Agent>().AddResource(mResourceGet);
}

void PendingDatasetManager::Clear(void)
{
    DatasetManager::Clear();
    mDelayTimer.Stop();
}

void PendingDatasetManager::ClearNetwork(void)
{
    Dataset dataset;

    mTimestamp.Clear();
    mTimestampValid = false;
    IgnoreError(DatasetManager::Save(dataset));
}

Error PendingDatasetManager::Save(const Dataset::Info &aDatasetInfo)
{
    Error error;

    SuccessOrExit(error = DatasetManager::Save(aDatasetInfo));
    StartDelayTimer();

exit:
    return error;
}

Error PendingDatasetManager::Save(const otOperationalDatasetTlvs &aDataset)
{
    Error error;

    SuccessOrExit(error = DatasetManager::Save(aDataset));
    StartDelayTimer();

exit:
    return error;
}

Error PendingDatasetManager::Save(const Dataset &aDataset)
{
    Error error;

    SuccessOrExit(error = DatasetManager::SaveLocal(aDataset));
    StartDelayTimer();

exit:
    return error;
}

Error PendingDatasetManager::Save(const Timestamp &aTimestamp,
                                  const Message &  aMessage,
                                  uint16_t         aOffset,
                                  uint8_t          aLength)
{
    Error   error = kErrorNone;
    Dataset dataset;

    SuccessOrExit(error = dataset.ReadFromMessage(aMessage, aOffset, aLength));
    dataset.SetTimestamp(Dataset::kPending, aTimestamp);
    SuccessOrExit(error = DatasetManager::Save(dataset));
    StartDelayTimer();

exit:
    return error;
}

void PendingDatasetManager::StartDelayTimer(void)
{
    DelayTimerTlv *delayTimer;
    Dataset        dataset;

    IgnoreError(Read(dataset));

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
        LogInfo("delay timer started %d", delay);
    }
}

void PendingDatasetManager::HandleDelayTimer(Timer &aTimer)
{
    aTimer.Get<PendingDatasetManager>().HandleDelayTimer();
}

void PendingDatasetManager::HandleDelayTimer(void)
{
    DelayTimerTlv *delayTimer;
    Dataset        dataset;

    IgnoreError(Read(dataset));

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

    LogInfo("pending delay timer expired");

    dataset.ConvertToActive();

    Get<ActiveDatasetManager>().Save(dataset);

    Clear();

exit:
    return;
}

void PendingDatasetManager::HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDatasetManager *>(aContext)->HandleGet(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void PendingDatasetManager::HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void PendingDatasetManager::HandleTimer(Timer &aTimer)
{
    aTimer.Get<PendingDatasetManager>().HandleTimer();
}

} // namespace MeshCoP
} // namespace ot
