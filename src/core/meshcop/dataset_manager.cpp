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

DatasetManager::DatasetManager(Instance &aInstance, Type aType, Timer::Handler aTimerHandler)
    : InstanceLocator(aInstance)
    , mType(aType)
    , mLocalSaved(false)
    , mLocalTimestampValid(false)
    , mNetworkTimestampValid(false)
    , mMgmtPending(false)
    , mLocalUpdateTime(0)
    , mTimer(aInstance, aTimerHandler)
{
    mLocalTimestamp.Clear();
    mNetworkTimestamp.Clear();
}

const Timestamp *DatasetManager::GetTimestamp(void) const
{
    return mNetworkTimestampValid ? &mNetworkTimestamp : nullptr;
}

Error DatasetManager::Restore(void)
{
    Error   error;
    Dataset dataset;

    mTimer.Stop();

    mNetworkTimestampValid = false;
    mLocalTimestampValid   = false;

    SuccessOrExit(error = Read(dataset));

    mLocalSaved = true;

    if (dataset.ReadTimestamp(mType, mLocalTimestamp) == kErrorNone)
    {
        mLocalTimestampValid   = true;
        mNetworkTimestampValid = true;
        mNetworkTimestamp      = mLocalTimestamp;
    }

    if (IsActiveDataset())
    {
        IgnoreError(ApplyConfiguration(dataset));
    }

    SignalDatasetChange();

exit:
    return error;
}

Error DatasetManager::Read(Dataset &aDataset) const
{
    Error error;

    aDataset.Clear();

    SuccessOrExit(error = Get<Settings>().ReadOperationalDataset(mType, aDataset));

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    EmplaceSecurelyStoredKeys(aDataset);
#endif

    if (mType == Dataset::kActive)
    {
        aDataset.RemoveTlv(Tlv::kPendingTimestamp);
        aDataset.RemoveTlv(Tlv::kDelayTimer);
    }
    else
    {
        Tlv *tlv = aDataset.FindTlv(Tlv::kDelayTimer);

        VerifyOrExit(tlv != nullptr);
        tlv->WriteValueAs<DelayTimerTlv>(DelayTimerTlv::CalculateRemainingDelay(*tlv, mLocalUpdateTime));
    }

    aDataset.mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

Error DatasetManager::Read(Dataset::Info &aDatasetInfo) const
{
    Dataset dataset;
    Error   error;

    aDatasetInfo.Clear();

    SuccessOrExit(error = Read(dataset));
    dataset.ConvertTo(aDatasetInfo);

exit:
    return error;
}

Error DatasetManager::Read(Dataset::Tlvs &aDatasetTlvs) const
{
    Dataset dataset;
    Error   error;

    ClearAllBytes(aDatasetTlvs);

    SuccessOrExit(error = Read(dataset));
    dataset.ConvertTo(aDatasetTlvs);

exit:
    return error;
}

Error DatasetManager::ApplyConfiguration(void) const
{
    Error   error;
    Dataset dataset;

    SuccessOrExit(error = Read(dataset));
    error = ApplyConfiguration(dataset);

exit:
    return error;
}

Error DatasetManager::ApplyConfiguration(const Dataset &aDataset) const
{
    Error error = kErrorNone;

    SuccessOrExit(error = aDataset.ValidateTlvs());

    for (const Tlv *cur = aDataset.GetTlvsStart(); cur < aDataset.GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case Tlv::kChannel:
        {
            uint8_t channel = static_cast<uint8_t>(cur->ReadValueAs<ChannelTlv>().GetChannel());

            error = Get<Mac::Mac>().SetPanChannel(channel);

            if (error != kErrorNone)
            {
                LogWarn("ApplyConfiguration() Failed to set channel to %d (%s)", channel, ErrorToString(error));
                ExitNow();
            }

            break;
        }

        case Tlv::kPanId:
            Get<Mac::Mac>().SetPanId(cur->ReadValueAs<PanIdTlv>());
            break;

        case Tlv::kExtendedPanId:
            Get<ExtendedPanIdManager>().SetExtPanId(cur->ReadValueAs<ExtendedPanIdTlv>());
            break;

        case Tlv::kNetworkName:
            IgnoreError(Get<NetworkNameManager>().SetNetworkName(As<NetworkNameTlv>(cur)->GetNetworkName()));
            break;

        case Tlv::kNetworkKey:
            Get<KeyManager>().SetNetworkKey(cur->ReadValueAs<NetworkKeyTlv>());
            break;

#if OPENTHREAD_FTD
        case Tlv::kPskc:
            Get<KeyManager>().SetPskc(cur->ReadValueAs<PskcTlv>());
            break;
#endif

        case Tlv::kMeshLocalPrefix:
            Get<Mle::MleRouter>().SetMeshLocalPrefix(cur->ReadValueAs<MeshLocalPrefixTlv>());
            break;

        case Tlv::kSecurityPolicy:
            Get<KeyManager>().SetSecurityPolicy(As<SecurityPolicyTlv>(cur)->GetSecurityPolicy());
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

void DatasetManager::Clear(void)
{
    mNetworkTimestamp.Clear();
    mLocalTimestamp.Clear();

    mNetworkTimestampValid = false;
    mLocalTimestampValid   = false;
    mLocalSaved            = false;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    DestroySecurelyStoredKeys();
#endif
    Get<Settings>().DeleteOperationalDataset(mType);

    mTimer.Stop();

    if (IsPendingDataset())
    {
        Get<PendingDatasetManager>().mDelayTimer.Stop();
    }

    SignalDatasetChange();
}

void DatasetManager::HandleDetach(void) { IgnoreError(Restore()); }

Error DatasetManager::Save(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error   error = kErrorNone;
    Dataset dataset;

    SuccessOrExit(error = dataset.SetFrom(aMessage, aOffset, aLength));
    SuccessOrExit(error = dataset.ValidateTlvs());

    if (IsActiveDataset())
    {
        SuccessOrExit(error = dataset.Write<ActiveTimestampTlv>(aTimestamp));
    }
    else
    {
        SuccessOrExit(error = dataset.Write<PendingTimestampTlv>(aTimestamp));
    }

    SuccessOrExit(error = Save(dataset));

    if (IsPendingDataset())
    {
        Get<PendingDatasetManager>().StartDelayTimer(dataset);
    }

exit:
    return error;
}

Error DatasetManager::Save(const Dataset &aDataset, bool aAllowOlderTimestamp)
{
    Error error = kErrorNone;
    int   compare;

    if (aDataset.ReadTimestamp(mType, mNetworkTimestamp) == kErrorNone)
    {
        mNetworkTimestampValid = true;

        if (IsActiveDataset())
        {
            SuccessOrExit(error = ApplyConfiguration(aDataset));
        }
    }

    compare = Timestamp::Compare(GetTimestamp(), GetLocalTimestamp());

    if ((compare > 0) || aAllowOlderTimestamp)
    {
        LocalSave(aDataset);

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

void DatasetManager::SaveLocal(const Dataset::Info &aDatasetInfo)
{
    Dataset dataset;

    dataset.SetFrom(aDatasetInfo);
    SaveLocal(dataset);
}

Error DatasetManager::SaveLocal(const Dataset::Tlvs &aDatasetTlvs)
{
    Error   error = kErrorInvalidArgs;
    Dataset dataset;

    SuccessOrExit(dataset.SetFrom(aDatasetTlvs));
    SuccessOrExit(dataset.ValidateTlvs());
    SaveLocal(dataset);
    error = kErrorNone;

exit:
    return error;
}

void DatasetManager::SaveLocal(const Dataset &aDataset)
{
    LocalSave(aDataset);

    if (IsPendingDataset())
    {
        Get<PendingDatasetManager>().StartDelayTimer(aDataset);
    }

    switch (Get<Mle::MleRouter>().GetRole())
    {
    case Mle::kRoleDisabled:
        IgnoreError(Restore());
        break;

    case Mle::kRoleChild:
        SyncLocalWithLeader(aDataset);
        break;
#if OPENTHREAD_FTD
    case Mle::kRoleRouter:
        SyncLocalWithLeader(aDataset);
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

void DatasetManager::LocalSave(const Dataset &aDataset)
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    DestroySecurelyStoredKeys();
#endif

    if (aDataset.GetLength() == 0)
    {
        Get<Settings>().DeleteOperationalDataset(mType);
        mLocalSaved = false;
        LogInfo("%s dataset deleted", Dataset::TypeToString(mType));
    }
    else
    {
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        // Store the network key and PSKC in the secure storage instead of settings.
        Dataset dataset;

        dataset.SetFrom(aDataset);
        MoveKeysToSecureStorage(dataset);
        Get<Settings>().SaveOperationalDataset(mType, dataset);
#else
        Get<Settings>().SaveOperationalDataset(mType, aDataset);
#endif

        mLocalSaved = true;
        LogInfo("%s dataset set", Dataset::TypeToString(mType));
    }

    mLocalTimestampValid = (aDataset.ReadTimestamp(mType, mLocalTimestamp) == kErrorNone);
    mLocalUpdateTime     = TimerMilli::GetNow();
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

void DatasetManager::HandleTimer(void)
{
    Dataset dataset;

    SuccessOrExit(Read(dataset));
    SyncLocalWithLeader(dataset);

exit:
    return;
}

void DatasetManager::SyncLocalWithLeader(const Dataset &aDataset)
{
    // Attempts to synchronize the local Dataset with the leader by
    // sending `MGMT_SET` command if the local Dataset's timestamp is
    // newer.

    Error error = kErrorNone;

    VerifyOrExit(!mMgmtPending, error = kErrorBusy);
    VerifyOrExit(Get<Mle::MleRouter>().IsChild() || Get<Mle::MleRouter>().IsRouter(), error = kErrorInvalidState);

    VerifyOrExit(Timestamp::Compare(GetTimestamp(), GetLocalTimestamp()) < 0, error = kErrorAlready);

    if (IsActiveDataset())
    {
        Dataset   pendingDataset;
        Timestamp timestamp;

        IgnoreError(Get<PendingDatasetManager>().Read(pendingDataset));

        if ((pendingDataset.Read<ActiveTimestampTlv>(timestamp) == kErrorNone) &&
            (Timestamp::Compare(&timestamp, GetLocalTimestamp()) == 0))
        {
            // Stop registration attempts during dataset transition
            ExitNow(error = kErrorInvalidState);
        }
    }

    error = SendSetRequest(aDataset);

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

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

const DatasetManager::SecurelyStoredTlv DatasetManager::kSecurelyStoredTlvs[] = {
    {
        Tlv::kNetworkKey,
        Crypto::Storage::kActiveDatasetNetworkKeyRef,
        Crypto::Storage::kPendingDatasetNetworkKeyRef,
    },
    {
        Tlv::kPskc,
        Crypto::Storage::kActiveDatasetPskcRef,
        Crypto::Storage::kPendingDatasetPskcRef,
    },
};

void DatasetManager::DestroySecurelyStoredKeys(void) const
{
    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        Crypto::Storage::DestroyKey(entry.GetKeyRef(mType));
    }
}

void DatasetManager::MoveKeysToSecureStorage(Dataset &aDataset) const
{
    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        aDataset.SaveTlvInSecureStorageAndClearValue(entry.mTlvType, entry.GetKeyRef(mType));
    }
}

void DatasetManager::EmplaceSecurelyStoredKeys(Dataset &aDataset) const
{
    bool moveKeys = false;

    // If reading any of the TLVs fails, it indicates they are not yet
    // stored in secure storage and are still contained in the `Dataset`
    // read from `Settings`. In this case, we move the keys to secure
    // storage and then clear them from 'Settings'.

    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        if (aDataset.ReadTlvFromSecureStorage(entry.mTlvType, entry.GetKeyRef(mType)) != kErrorNone)
        {
            moveKeys = true;
        }
    }

    if (moveKeys)
    {
        Dataset dataset;

        dataset.SetFrom(aDataset);
        MoveKeysToSecureStorage(dataset);
        Get<Settings>().SaveOperationalDataset(mType, dataset);
    }
}

#endif // OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// ActiveDatasetManager

ActiveDatasetManager::ActiveDatasetManager(Instance &aInstance)
    : DatasetManager(aInstance, Dataset::kActive, ActiveDatasetManager::HandleTimer)
{
}

bool ActiveDatasetManager::IsPartiallyComplete(void) const { return mLocalSaved && !mNetworkTimestampValid; }

bool ActiveDatasetManager::IsComplete(void) const { return mLocalSaved && mNetworkTimestampValid; }

bool ActiveDatasetManager::IsCommissioned(void) const
{
    static const Tlv::Type kRequiredTlvs[] = {
        Tlv::kNetworkKey, Tlv::kNetworkName, Tlv::kExtendedPanId, Tlv::kPanId, Tlv::kChannel,
    };

    Dataset dataset;
    bool    isValid = false;

    SuccessOrExit(Read(dataset));
    isValid = dataset.ContainsAllTlvs(kRequiredTlvs, sizeof(kRequiredTlvs));

exit:
    return isValid;
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

void PendingDatasetManager::StartDelayTimer(void)
{
    Dataset dataset;

    mDelayTimer.Stop();

    SuccessOrExit(Read(dataset));
    StartDelayTimer(dataset);

exit:
    return;
}

void PendingDatasetManager::StartDelayTimer(const Dataset &aDataset)
{
    uint32_t delay;

    mDelayTimer.Stop();

    SuccessOrExit(aDataset.Read<DelayTimerTlv>(delay));

    delay = Min(delay, DelayTimerTlv::kMaxDelay);

    mDelayTimer.StartAt(aDataset.GetUpdateTime(), delay);
    LogInfo("delay timer started %lu", ToUlong(delay));

exit:
    return;
}

void PendingDatasetManager::HandleDelayTimer(void)
{
    Dataset   dataset;
    Timestamp activeTimestamp;
    bool      shouldReplaceActive = false;

    SuccessOrExit(Read(dataset));

    LogInfo("Pending delay timer expired");

    // Determine whether the Pending Dataset should replace the
    // current Active Dataset. This is allowed if the Pending
    // Dataset's Active Timestamp is newer, or the Pending Dataset
    // contains a different key.

    SuccessOrExit(dataset.Read<ActiveTimestampTlv>(activeTimestamp));

    if (Timestamp::Compare(&activeTimestamp, Get<ActiveDatasetManager>().GetTimestamp()) > 0)
    {
        shouldReplaceActive = true;
    }
    else
    {
        NetworkKey newKey;
        NetworkKey currentKey;

        SuccessOrExit(dataset.Read<NetworkKeyTlv>(newKey));
        Get<KeyManager>().GetNetworkKey(currentKey);
        shouldReplaceActive = (currentKey != newKey);
    }

    VerifyOrExit(shouldReplaceActive);

    // Convert Pending Dataset to Active by removing the Pending
    // Timestamp and the Delay Timer TLVs.

    dataset.RemoveTlv(Tlv::kPendingTimestamp);
    dataset.RemoveTlv(Tlv::kDelayTimer);

    IgnoreError(Get<ActiveDatasetManager>().Save(dataset, /* aAllowOlderTimestamp */ true));

exit:
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
