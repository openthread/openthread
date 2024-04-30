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
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/notifier.hpp"
#include "instance/instance.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("DatasetManager");

//---------------------------------------------------------------------------------------------------------------------
// DatasetManager

DatasetManager::DatasetManager(Instance &aInstance, Dataset::Type aType, Timer::Handler aTimerHandler)
    : InstanceLocator(aInstance)
    , mLocal(aInstance, aType)
    , mTimestampValid(false)
    , mMgmtPending(false)
    , mTimer(aInstance, aTimerHandler)
{
    mTimestamp.Clear();
}

const Timestamp *DatasetManager::GetTimestamp(void) const { return mTimestampValid ? &mTimestamp : nullptr; }

Error DatasetManager::Restore(void)
{
    Error   error;
    Dataset dataset;

    mTimer.Stop();

    mTimestampValid = false;

    SuccessOrExit(error = mLocal.Restore(dataset));

    mTimestampValid = (dataset.ReadTimestamp(GetType(), mTimestamp) == kErrorNone);

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

void DatasetManager::HandleDetach(void) { IgnoreError(Restore()); }

Error DatasetManager::Save(const Dataset &aDataset)
{
    Error error = kErrorNone;
    int   compare;
    bool  isNetworkKeyUpdated = false;

    if (aDataset.ReadTimestamp(GetType(), mTimestamp) == kErrorNone)
    {
        mTimestampValid = true;

        if (IsActiveDataset())
        {
            SuccessOrExit(error = aDataset.ApplyConfiguration(GetInstance(), isNetworkKeyUpdated));
        }
    }

    compare = Timestamp::Compare(mTimestampValid ? &mTimestamp : nullptr, mLocal.GetTimestamp());

    if (isNetworkKeyUpdated || compare > 0)
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

Error DatasetManager::Save(const Dataset::Tlvs &aDatasetTlvs)
{
    Error error;

    SuccessOrExit(error = mLocal.Save(aDatasetTlvs));
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
    Get<Notifier>().Signal(IsActiveDataset() ? kEventActiveDatasetChanged : kEventPendingDatasetChanged);
}

Error DatasetManager::GetChannelMask(Mac::ChannelMask &aChannelMask) const
{
    Error                 error;
    const ChannelMaskTlv *channelMaskTlv;
    uint32_t              mask;
    Dataset               dataset;

    SuccessOrExit(error = Read(dataset));

    channelMaskTlv = As<ChannelMaskTlv>(dataset.FindTlv(Tlv::kChannelMask));
    VerifyOrExit(channelMaskTlv != nullptr, error = kErrorNotFound);
    SuccessOrExit(channelMaskTlv->ReadChannelMask(mask));

    aChannelMask.SetMask(mask & Get<Mac::Mac>().GetSupportedChannelMask().GetMask());

    VerifyOrExit(!aChannelMask.IsEmpty(), error = kErrorNotFound);

exit:
    return error;
}

void DatasetManager::HandleTimer(void) { SendSet(); }

void DatasetManager::SendSet(void)
{
    Error   error = kErrorNone;
    Dataset dataset;

    VerifyOrExit(!mMgmtPending, error = kErrorBusy);
    VerifyOrExit(Get<Mle::MleRouter>().IsChild() || Get<Mle::MleRouter>().IsRouter(), error = kErrorInvalidState);

    VerifyOrExit(Timestamp::Compare(GetTimestamp(), mLocal.GetTimestamp()) < 0, error = kErrorAlready);

    if (IsActiveDataset())
    {
        Dataset   pendingDataset;
        Timestamp timestamp;

        IgnoreError(Get<PendingDatasetManager>().Read(pendingDataset));

        if ((pendingDataset.Read<ActiveTimestampTlv>(timestamp) == kErrorNone) &&
            (Timestamp::Compare(&timestamp, mLocal.GetTimestamp()) == 0))
        {
            // Stop registration attempts during dataset transition
            ExitNow(error = kErrorInvalidState);
        }
    }

    IgnoreError(Read(dataset));

    error = SendSetRequest(dataset);

exit:
    if (error == kErrorNoBufs)
    {
        mTimer.Start(kSendSetDelay);
    }

    if (error != kErrorAlready)
    {
        LogWarnOnError(error, "send Dataset set to leader");
    }
}

Error DatasetManager::SendSetRequest(const Dataset &aDataset)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit(!mMgmtPending, error = kErrorAlready);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(IsActiveDataset() ? kUriActiveSet : kUriPendingSet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->AppendBytes(aDataset.GetBytes(), aDataset.GetLength()));
    IgnoreError(messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleMgmtSetResponse, this));
    mMgmtPending = true;

    LogInfo("Sent dataset set request to leader");

exit:
    FreeMessageOnError(message, error);
    return error;
}

void DatasetManager::HandleMgmtSetResponse(void                *aContext,
                                           otMessage           *aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           Error                aError)
{
    static_cast<DatasetManager *>(aContext)->HandleMgmtSetResponse(AsCoapMessagePtr(aMessage),
                                                                   AsCoreTypePtr(aMessageInfo), aError);
}

void DatasetManager::HandleMgmtSetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error   error;
    uint8_t state = StateTlv::kPending;

    SuccessOrExit(error = aError);
    VerifyOrExit(Tlv::Find<StateTlv>(*aMessage, state) == kErrorNone && state != StateTlv::kPending,
                 error = kErrorParse);

    if (state == StateTlv::kReject)
    {
        error = kErrorRejected;
    }

exit:
    LogInfo("MGMT_SET finished: %s", error == kErrorNone ? "Accepted" : ErrorToString(error));

    mMgmtPending = false;

    mMgmtSetCallback.InvokeAndClearIfSet(error);

    mTimer.Start(kSendSetDelay);
}

void DatasetManager::HandleGet(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    TlvList  tlvList;
    uint8_t  tlvType;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(Tlv::FindTlvValueOffset(aMessage, Tlv::kGet, offset, length));

    for (; length > 0; length--, offset++)
    {
        IgnoreError(aMessage.Read(offset, tlvType));
        tlvList.Add(tlvType);
    }

    // MGMT_PENDING_GET.rsp must include Delay Timer TLV (Thread 1.1.1
    // Section 8.7.5.4).

    if (!tlvList.IsEmpty() && IsPendingDataset())
    {
        tlvList.Add(Tlv::kDelayTimer);
    }

exit:
    SendGetResponse(aMessage, aMessageInfo, tlvList);
}

void DatasetManager::SendGetResponse(const Coap::Message    &aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     const TlvList          &aTlvList) const
{
    Error          error = kErrorNone;
    Coap::Message *message;
    Dataset        dataset;

    IgnoreError(Read(dataset));

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    for (const Tlv *tlv = dataset.GetTlvsStart(); tlv < dataset.GetTlvsEnd(); tlv = tlv->GetNext())
    {
        bool shouldAppend = true;

        if (!aTlvList.IsEmpty())
        {
            shouldAppend = aTlvList.Contains(tlv->GetType());
        }

        if ((tlv->GetType() == Tlv::kNetworkKey) && !Get<KeyManager>().GetSecurityPolicy().mObtainNetworkKeyEnabled)
        {
            shouldAppend = false;
        }

        if (shouldAppend)
        {
            SuccessOrExit(error = tlv->AppendTo(*message));
        }
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

    LogInfo("sent %s dataset get response to %s", IsActiveDataset() ? "active" : "pending",
            aMessageInfo.GetPeerAddr().ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
}

Error DatasetManager::SendSetRequest(const Dataset::Info &aDatasetInfo,
                                     const uint8_t       *aTlvs,
                                     uint8_t              aLength,
                                     MgmtSetCallback      aCallback,
                                     void                *aContext)
{
    Error   error = kErrorNone;
    Dataset dataset;

    dataset.SetFrom(aDatasetInfo);
    SuccessOrExit(error = dataset.AppendTlvsFrom(aTlvs, aLength));

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    if (Get<Commissioner>().IsActive() && !dataset.ContainsTlv(Tlv::kCommissionerSessionId))
    {
        SuccessOrExit(error = dataset.Write<CommissionerSessionIdTlv>(Get<Commissioner>().GetSessionId()));
    }
#endif

    SuccessOrExit(error = SendSetRequest(dataset));
    mMgmtSetCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error DatasetManager::SendGetRequest(const Dataset::Components &aDatasetComponents,
                                     const uint8_t             *aTlvTypes,
                                     uint8_t                    aLength,
                                     const otIp6Address        *aAddress) const
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());
    TlvList          tlvList;

    if (aDatasetComponents.IsPresent<Dataset::kActiveTimestamp>())
    {
        tlvList.Add(Tlv::kActiveTimestamp);
    }

    if (aDatasetComponents.IsPresent<Dataset::kPendingTimestamp>())
    {
        tlvList.Add(Tlv::kPendingTimestamp);
    }

    if (aDatasetComponents.IsPresent<Dataset::kNetworkKey>())
    {
        tlvList.Add(Tlv::kNetworkKey);
    }

    if (aDatasetComponents.IsPresent<Dataset::kNetworkName>())
    {
        tlvList.Add(Tlv::kNetworkName);
    }

    if (aDatasetComponents.IsPresent<Dataset::kExtendedPanId>())
    {
        tlvList.Add(Tlv::kExtendedPanId);
    }

    if (aDatasetComponents.IsPresent<Dataset::kMeshLocalPrefix>())
    {
        tlvList.Add(Tlv::kMeshLocalPrefix);
    }

    if (aDatasetComponents.IsPresent<Dataset::kDelay>())
    {
        tlvList.Add(Tlv::kDelayTimer);
    }

    if (aDatasetComponents.IsPresent<Dataset::kPanId>())
    {
        tlvList.Add(Tlv::kPanId);
    }

    if (aDatasetComponents.IsPresent<Dataset::kChannel>())
    {
        tlvList.Add(Tlv::kChannel);
    }

    if (aDatasetComponents.IsPresent<Dataset::kPskc>())
    {
        tlvList.Add(Tlv::kPskc);
    }

    if (aDatasetComponents.IsPresent<Dataset::kSecurityPolicy>())
    {
        tlvList.Add(Tlv::kSecurityPolicy);
    }

    if (aDatasetComponents.IsPresent<Dataset::kChannelMask>())
    {
        tlvList.Add(Tlv::kChannelMask);
    }

    for (uint8_t index = 0; index < aLength; index++)
    {
        tlvList.Add(aTlvTypes[index]);
    }

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(IsActiveDataset() ? kUriActiveGet : kUriPendingGet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (!tlvList.IsEmpty())
    {
        SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kGet, tlvList.GetArrayBuffer(), tlvList.GetLength()));
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

void DatasetManager::TlvList::Add(uint8_t aTlvType)
{
    if (!Contains(aTlvType))
    {
        IgnoreError(PushBack(aTlvType));
    }
}

//---------------------------------------------------------------------------------------------------------------------
// ActiveDatasetManager

ActiveDatasetManager::ActiveDatasetManager(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kActive, ActiveDatasetManager::HandleTimer)
{
}

bool ActiveDatasetManager::IsPartiallyComplete(void) const { return mLocal.IsSaved() && !mTimestampValid; }

bool ActiveDatasetManager::IsComplete(void) const { return mLocal.IsSaved() && mTimestampValid; }

bool ActiveDatasetManager::IsCommissioned(void) const
{
    Dataset::Info datasetInfo;
    bool          isValid = false;

    SuccessOrExit(Read(datasetInfo));

    isValid = (datasetInfo.IsPresent<Dataset::kNetworkKey>() && datasetInfo.IsPresent<Dataset::kNetworkName>() &&
               datasetInfo.IsPresent<Dataset::kExtendedPanId>() && datasetInfo.IsPresent<Dataset::kPanId>() &&
               datasetInfo.IsPresent<Dataset::kChannel>());

exit:
    return isValid;
}

Error ActiveDatasetManager::Save(const Timestamp &aTimestamp,
                                 const Message   &aMessage,
                                 uint16_t         aOffset,
                                 uint16_t         aLength)
{
    Error   error = kErrorNone;
    Dataset dataset;

    SuccessOrExit(error = dataset.SetFrom(aMessage, aOffset, aLength));
    SuccessOrExit(error = dataset.ValidateTlvs());
    SuccessOrExit(error = dataset.Write<ActiveTimestampTlv>(aTimestamp));
    error = DatasetManager::Save(dataset);

exit:
    return error;
}

template <>
void ActiveDatasetManager::HandleTmf<kUriActiveGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void ActiveDatasetManager::HandleTimer(Timer &aTimer) { aTimer.Get<ActiveDatasetManager>().HandleTimer(); }

//---------------------------------------------------------------------------------------------------------------------
// PendingDatasetManager

PendingDatasetManager::PendingDatasetManager(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kPending, PendingDatasetManager::HandleTimer)
    , mDelayTimer(aInstance)
{
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

Error PendingDatasetManager::Save(const Dataset::Tlvs &aDatasetTlvs)
{
    Error error;

    SuccessOrExit(error = DatasetManager::Save(aDatasetTlvs));
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
                                  const Message   &aMessage,
                                  uint16_t         aOffset,
                                  uint16_t         aLength)
{
    Error   error = kErrorNone;
    Dataset dataset;

    SuccessOrExit(error = dataset.SetFrom(aMessage, aOffset, aLength));
    SuccessOrExit(error = dataset.ValidateTlvs());
    SuccessOrExit(dataset.Write<PendingTimestampTlv>(aTimestamp));
    SuccessOrExit(error = DatasetManager::Save(dataset));
    StartDelayTimer();

exit:
    return error;
}

void PendingDatasetManager::StartDelayTimer(void)
{
    Tlv     *tlv;
    uint32_t delay;
    Dataset  dataset;

    IgnoreError(Read(dataset));

    mDelayTimer.Stop();

    tlv = dataset.FindTlv(Tlv::kDelayTimer);
    VerifyOrExit(tlv != nullptr);

    delay = Min(tlv->ReadValueAs<DelayTimerTlv>(), DelayTimerTlv::kMaxDelay);

    mDelayTimer.StartAt(dataset.GetUpdateTime(), delay);
    LogInfo("delay timer started %lu", ToUlong(delay));

exit:
    return;
}

void PendingDatasetManager::HandleDelayTimer(void)
{
    Dataset dataset;

    IgnoreError(Read(dataset));
    LogInfo("pending delay timer expired");

    dataset.ConvertToActive();

    Get<ActiveDatasetManager>().Save(dataset);

    Clear();
}

template <>
void PendingDatasetManager::HandleTmf<kUriPendingGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::HandleGet(aMessage, aMessageInfo);
}

void PendingDatasetManager::HandleTimer(Timer &aTimer) { aTimer.Get<PendingDatasetManager>().HandleTimer(); }

} // namespace MeshCoP
} // namespace ot
