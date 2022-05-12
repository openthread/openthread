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

#include "meshcop/dataset_manager.hpp"

#if OPENTHREAD_FTD

#include <stdio.h>

#include <openthread/platform/radio.h>

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_leader.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("DatasetManager");

Error DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    Dataset dataset;

    IgnoreError(Read(dataset));

    return dataset.AppendMleDatasetTlv(GetType(), aMessage);
}

Error DatasetManager::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv                tlv;
    uint16_t           offset                   = aMessage.GetOffset();
    bool               isUpdateFromCommissioner = false;
    bool               doesAffectConnectivity   = false;
    bool               doesAffectNetworkKey     = false;
    bool               hasNetworkKey            = false;
    StateTlv::State    state                    = StateTlv::kReject;
    Dataset            dataset;
    Timestamp          activeTimestamp;
    ChannelTlv         channel;
    uint16_t           sessionId;
    Ip6::NetworkPrefix meshLocalPrefix;
    NetworkKey         networkKey;
    uint16_t           panId;

    VerifyOrExit(Get<Mle::MleRouter>().IsLeader());

    // verify that TLV data size is less than maximum TLV value size
    while (offset < aMessage.GetLength())
    {
        SuccessOrExit(aMessage.Read(offset, tlv));
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize);
        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize);

    // verify the request includes a timestamp that is ahead of the locally stored value
    SuccessOrExit(Tlv::Find<ActiveTimestampTlv>(aMessage, activeTimestamp));

    if (GetType() == Dataset::kPending)
    {
        Timestamp pendingTimestamp;

        SuccessOrExit(Tlv::Find<PendingTimestampTlv>(aMessage, pendingTimestamp));
        VerifyOrExit(Timestamp::Compare(&pendingTimestamp, mLocal.GetTimestamp()) > 0);
    }
    else
    {
        VerifyOrExit(Timestamp::Compare(&activeTimestamp, mLocal.GetTimestamp()) > 0);
    }

    // check channel
    if (Tlv::FindTlv(aMessage, channel) == kErrorNone)
    {
        VerifyOrExit(channel.IsValid());

        if (channel.GetChannel() != Get<Mac::Mac>().GetPanChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::Find<PanIdTlv>(aMessage, panId) == kErrorNone && panId != Get<Mac::Mac>().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::Find<MeshLocalPrefixTlv>(aMessage, meshLocalPrefix) == kErrorNone &&
        meshLocalPrefix != Get<Mle::MleRouter>().GetMeshLocalPrefix())
    {
        doesAffectConnectivity = true;
    }

    // check network key
    if (Tlv::Find<NetworkKeyTlv>(aMessage, networkKey) == kErrorNone)
    {
        NetworkKey localNetworkKey;

        hasNetworkKey = true;
        Get<KeyManager>().GetNetworkKey(localNetworkKey);

        if (networkKey != localNetworkKey)
        {
            doesAffectConnectivity = true;
            doesAffectNetworkKey   = true;
        }
    }

    // check active timestamp rollback
    if (GetType() == Dataset::kPending && (!hasNetworkKey || !doesAffectNetworkKey))
    {
        // no change to network key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = Get<ActiveDatasetManager>().GetTimestamp();

        VerifyOrExit(Timestamp::Compare(&activeTimestamp, localActiveTimestamp) > 0);
    }

    // check commissioner session id
    if (Tlv::Find<CommissionerSessionIdTlv>(aMessage, sessionId) == kErrorNone)
    {
        const CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = As<CommissionerSessionIdTlv>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(localId != nullptr && localId->GetCommissionerSessionId() == sessionId);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || GetType() == Dataset::kPending || !doesAffectConnectivity);

    if (isUpdateFromCommissioner)
    {
        // Thread specification allows partial dataset changes for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
        // from Commissioner based on existing active dataset.
        IgnoreError(Get<ActiveDatasetManager>().Read(dataset));
    }

    if (GetType() == Dataset::kPending || !doesAffectConnectivity)
    {
        offset = aMessage.GetOffset();

        while (offset < aMessage.GetLength())
        {
            DatasetTlv datasetTlv;

            SuccessOrExit(datasetTlv.ReadFromMessage(aMessage, offset));

            switch (datasetTlv.GetType())
            {
            case Tlv::kCommissionerSessionId:
                // do not store Commissioner Session ID TLV
                break;

            case Tlv::kDelayTimer:
            {
                DelayTimerTlv &delayTimerTlv = As<DelayTimerTlv>(datasetTlv);

                if (doesAffectNetworkKey && delayTimerTlv.GetDelayTimer() < DelayTimerTlv::kDelayTimerDefault)
                {
                    delayTimerTlv.SetDelayTimer(DelayTimerTlv::kDelayTimerDefault);
                }
                else if (delayTimerTlv.GetDelayTimer() < Get<Leader>().GetDelayTimerMinimal())
                {
                    delayTimerTlv.SetDelayTimer(Get<Leader>().GetDelayTimerMinimal());
                }
            }

                OT_FALL_THROUGH;

            default:
                SuccessOrExit(dataset.SetTlv(datasetTlv));
                break;
            }

            offset += static_cast<uint16_t>(datasetTlv.GetSize());
        }

        SuccessOrExit(Save(dataset));
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
    }
    else
    {
        Get<PendingDatasetManager>().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    state = StateTlv::kAccept;

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        const CommissionerSessionIdTlv *localSessionId;
        Ip6::Address                    destination;

        localSessionId = As<CommissionerSessionIdTlv>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));
        VerifyOrExit(localSessionId != nullptr);

        SuccessOrExit(
            Get<Mle::MleRouter>().GetCommissionerAloc(destination, localSessionId->GetCommissionerSessionId()));

        Get<Leader>().SendDatasetChanged(destination);
    }

exit:

    if (Get<Mle::MleRouter>().IsLeader())
    {
        SendSetResponse(aMessage, aMessageInfo, state);
    }

    return (state == StateTlv::kAccept) ? kErrorNone : kErrorDrop;
}

void DatasetManager::SendSetResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

    LogInfo("sent dataset set response");

exit:
    FreeMessageOnError(message, error);
}

Error DatasetManager::DatasetTlv::ReadFromMessage(const Message &aMessage, uint16_t aOffset)
{
    Error error = kErrorNone;

    SuccessOrExit(error = aMessage.Read(aOffset, this, sizeof(Tlv)));
    VerifyOrExit(GetLength() <= Dataset::kMaxValueSize, error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aOffset + sizeof(Tlv), mValue, GetLength()));
    VerifyOrExit(Tlv::IsValid(*this), error = kErrorParse);

exit:
    return error;
}

Error ActiveDatasetManager::GenerateLocal(void)
{
    Error   error = kErrorNone;
    Dataset dataset;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(!mLocal.IsTimestampPresent(), error = kErrorAlready);

    IgnoreError(Read(dataset));

    if (dataset.GetTlv<ActiveTimestampTlv>() == nullptr)
    {
        Timestamp timestamp;

        timestamp.Clear();
        IgnoreError(dataset.SetTlv(Tlv::kActiveTimestamp, timestamp));
    }

    if (dataset.GetTlv<ChannelTlv>() == nullptr)
    {
        ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannel(Get<Mac::Mac>().GetPanChannel());
        IgnoreError(dataset.SetTlv(tlv));
    }

    if (dataset.GetTlv<ChannelMaskTlv>() == nullptr)
    {
        ChannelMaskTlv tlv;
        tlv.Init();
        tlv.SetChannelMask(Get<Mac::Mac>().GetSupportedChannelMask().GetMask());
        IgnoreError(dataset.SetTlv(tlv));
    }

    if (dataset.GetTlv<ExtendedPanIdTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kExtendedPanId, Get<ExtendedPanIdManager>().GetExtPanId()));
    }

    if (dataset.GetTlv<MeshLocalPrefixTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kMeshLocalPrefix, Get<Mle::MleRouter>().GetMeshLocalPrefix()));
    }

    if (dataset.GetTlv<NetworkKeyTlv>() == nullptr)
    {
        NetworkKey networkKey;

        Get<KeyManager>().GetNetworkKey(networkKey);
        IgnoreError(dataset.SetTlv(Tlv::kNetworkKey, networkKey));
    }

    if (dataset.GetTlv<NetworkNameTlv>() == nullptr)
    {
        NameData nameData = Get<NetworkNameManager>().GetNetworkName().GetAsData();

        IgnoreError(dataset.SetTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (dataset.GetTlv<PanIdTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kPanId, Get<Mac::Mac>().GetPanId()));
    }

    if (dataset.GetTlv<PskcTlv>() == nullptr)
    {
        Pskc pskc;

        if (Get<KeyManager>().IsPskcSet())
        {
            Get<KeyManager>().GetPskc(pskc);
        }
        else
        {
            SuccessOrExit(error = pskc.GenerateRandom());
        }

        IgnoreError(dataset.SetTlv(Tlv::kPskc, pskc));
    }

    if (dataset.GetTlv<SecurityPolicyTlv>() == nullptr)
    {
        SecurityPolicyTlv tlv;

        tlv.Init();
        tlv.SetSecurityPolicy(Get<KeyManager>().GetSecurityPolicy());
        IgnoreError(dataset.SetTlv(tlv));
    }

    SuccessOrExit(error = mLocal.Save(dataset));
    IgnoreError(Restore());

    LogInfo("Generated local dataset");

exit:
    return error;
}

void ActiveDatasetManager::StartLeader(void)
{
    IgnoreError(GenerateLocal());
    Get<Tmf::Agent>().AddResource(mResourceSet);
}

void ActiveDatasetManager::StopLeader(void)
{
    Get<Tmf::Agent>().RemoveResource(mResourceSet);
}

void ActiveDatasetManager::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDatasetManager *>(aContext)->HandleSet(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void ActiveDatasetManager::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    IgnoreError(ApplyConfiguration());

exit:
    return;
}

void PendingDatasetManager::StartLeader(void)
{
    StartDelayTimer();
    Get<Tmf::Agent>().AddResource(mResourceSet);
}

void PendingDatasetManager::StopLeader(void)
{
    Get<Tmf::Agent>().RemoveResource(mResourceSet);
}

void PendingDatasetManager::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDatasetManager *>(aContext)->HandleSet(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void PendingDatasetManager::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    StartDelayTimer();

exit:
    return;
}

void PendingDatasetManager::ApplyActiveDataset(const Timestamp &aTimestamp, Coap::Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    Dataset  dataset;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached());

    while (offset < aMessage.GetLength())
    {
        DatasetTlv datasetTlv;

        SuccessOrExit(datasetTlv.ReadFromMessage(aMessage, offset));
        offset += static_cast<uint16_t>(datasetTlv.GetSize());
        IgnoreError(dataset.SetTlv(datasetTlv));
    }

    // add delay timer tlv
    IgnoreError(dataset.SetTlv(Tlv::kDelayTimer, Get<Leader>().GetDelayTimerMinimal()));

    // add pending timestamp tlv
    dataset.SetTimestamp(Dataset::kPending, aTimestamp);
    IgnoreError(DatasetManager::Save(dataset));

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
