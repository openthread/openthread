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
#if OPENTHREAD_FTD

#include <stdio.h>

#include <openthread/platform/radio.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_leader.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

otError DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    Dataset dataset(GetType());

    IgnoreError(Read(dataset));

    return dataset.AppendMleDatasetTlv(aMessage);
}

otError DatasetManager::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv             tlv;
    Timestamp *     timestamp;
    uint16_t        offset = aMessage.GetOffset();
    Tlv::Type       type;
    bool            isUpdateFromCommissioner = false;
    bool            doesAffectConnectivity   = false;
    bool            doesAffectMasterKey      = false;
    bool            hasMasterKey             = false;
    StateTlv::State state                    = StateTlv::kReject;
    Dataset         dataset(GetType());

    ActiveTimestampTlv   activeTimestamp;
    PendingTimestampTlv  pendingTimestamp;
    ChannelTlv           channel;
    uint16_t             sessionId;
    Mle::MeshLocalPrefix meshLocalPrefix;
    MasterKey            masterKey;
    uint16_t             panId;

    activeTimestamp.SetLength(0);
    pendingTimestamp.SetLength(0);
    channel.SetLength(0);
    pendingTimestamp.SetLength(0);

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

    type = (GetType() == Dataset::kActive) ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp;

    if (Tlv::FindTlv(aMessage, activeTimestamp) != OT_ERROR_NONE)
    {
        ExitNow();
    }

    VerifyOrExit(activeTimestamp.IsValid());

    if (Tlv::FindTlv(aMessage, pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid());
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    timestamp = (type == Tlv::kActiveTimestamp) ? static_cast<Timestamp *>(&activeTimestamp)
                                                : static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.Compare(timestamp) > 0);

    // check channel
    if (Tlv::FindTlv(aMessage, channel) == OT_ERROR_NONE)
    {
        VerifyOrExit(channel.IsValid());

        if (channel.GetChannel() != Get<Mac::Mac>().GetPanChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::Find<PanIdTlv>(aMessage, panId) == OT_ERROR_NONE && panId != Get<Mac::Mac>().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::Find<MeshLocalPrefixTlv>(aMessage, meshLocalPrefix) == OT_ERROR_NONE &&
        meshLocalPrefix != Get<Mle::MleRouter>().GetMeshLocalPrefix())
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::Find<NetworkMasterKeyTlv>(aMessage, masterKey) == OT_ERROR_NONE)
    {
        hasMasterKey = true;

        if (masterKey != Get<KeyManager>().GetMasterKey())
        {
            doesAffectConnectivity = true;
            doesAffectMasterKey    = true;
        }
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp && (!hasMasterKey || (masterKey == Get<KeyManager>().GetMasterKey())))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = Get<ActiveDataset>().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == nullptr || localActiveTimestamp->Compare(activeTimestamp) > 0);
    }

    // check commissioner session id
    if (Tlv::Find<CommissionerSessionIdTlv>(aMessage, sessionId) == OT_ERROR_NONE)
    {
        const CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<const CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(localId != nullptr && localId->GetCommissionerSessionId() == sessionId);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity);

    if (isUpdateFromCommissioner)
    {
        // Thread specification allows partial dataset changes for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
        // from Commissioner based on existing active dataset.
        IgnoreError(Get<ActiveDataset>().Read(dataset));
    }

    if (type == Tlv::kPendingTimestamp || !doesAffectConnectivity)
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
                DelayTimerTlv &delayTimerTlv = static_cast<DelayTimerTlv &>(static_cast<Tlv &>(datasetTlv));

                if (doesAffectMasterKey && delayTimerTlv.GetDelayTimer() < DelayTimerTlv::kDelayTimerDefault)
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
        Get<PendingDataset>().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    state = StateTlv::kAccept;

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        const CommissionerSessionIdTlv *localSessionId;
        Ip6::Address                    destination;

        localSessionId = static_cast<const CommissionerSessionIdTlv *>(
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

    return (state == StateTlv::kAccept) ? OT_ERROR_NONE : OT_ERROR_DROP;
}

void DatasetManager::SendSetResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response");

exit:
    FreeMessageOnError(message, error);
}

otError DatasetManager::DatasetTlv::ReadFromMessage(const Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aMessage.Read(aOffset, this, sizeof(Tlv)));
    VerifyOrExit(GetLength() <= kMaxValueSize, error = OT_ERROR_PARSE);
    SuccessOrExit(error = aMessage.Read(aOffset + sizeof(Tlv), mValue, GetLength()));
    VerifyOrExit(Tlv::IsValid(*this), error = OT_ERROR_PARSE);

exit:
    return error;
}

otError ActiveDataset::GenerateLocal(void)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(GetType());

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mLocal.IsTimestampPresent(), error = OT_ERROR_ALREADY);

    IgnoreError(Read(dataset));

    if (dataset.GetTlv<ActiveTimestampTlv>() == nullptr)
    {
        ActiveTimestampTlv activeTimestampTlv;
        activeTimestampTlv.Init();
        activeTimestampTlv.SetSeconds(0);
        activeTimestampTlv.SetTicks(0);
        IgnoreError(dataset.SetTlv(activeTimestampTlv));
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
        IgnoreError(dataset.SetTlv(Tlv::kExtendedPanId, Get<Mac::Mac>().GetExtendedPanId()));
    }

    if (dataset.GetTlv<MeshLocalPrefixTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kMeshLocalPrefix, Get<Mle::MleRouter>().GetMeshLocalPrefix()));
    }

    if (dataset.GetTlv<NetworkMasterKeyTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kNetworkMasterKey, Get<KeyManager>().GetMasterKey()));
    }

    if (dataset.GetTlv<NetworkNameTlv>() == nullptr)
    {
        Mac::NameData nameData = Get<Mac::Mac>().GetNetworkName().GetAsData();

        IgnoreError(dataset.SetTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (dataset.GetTlv<PanIdTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kPanId, Get<Mac::Mac>().GetPanId()));
    }

    if (dataset.GetTlv<PskcTlv>() == nullptr)
    {
        if (Get<KeyManager>().IsPskcSet())
        {
            IgnoreError(dataset.SetTlv(Tlv::kPskc, Get<KeyManager>().GetPskc()));
        }
        else
        {
            // PSKc has not yet been configured, generate new PSKc at random
            Pskc pskc;

            SuccessOrExit(error = pskc.GenerateRandom());
            IgnoreError(dataset.SetTlv(Tlv::kPskc, pskc));
        }
    }

    if (dataset.GetTlv<SecurityPolicyTlv>() == nullptr)
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(static_cast<uint16_t>(Get<KeyManager>().GetKeyRotation()));
        tlv.SetFlags(Get<KeyManager>().GetSecurityPolicyFlags());
        IgnoreError(dataset.SetTlv(tlv));
    }

    SuccessOrExit(error = mLocal.Save(dataset));
    IgnoreError(Restore());

    otLogInfoMeshCoP("Generated local dataset");

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    IgnoreError(GenerateLocal());
    Get<Tmf::TmfAgent>().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    Get<Tmf::TmfAgent>().RemoveResource(mResourceSet);
}

void ActiveDataset::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(*static_cast<Coap::Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    IgnoreError(ApplyConfiguration());

exit:
    return;
}

void PendingDataset::StartLeader(void)
{
    StartDelayTimer();
    Get<Tmf::TmfAgent>().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    Get<Tmf::TmfAgent>().RemoveResource(mResourceSet);
}

void PendingDataset::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleSet(*static_cast<Coap::Message *>(aMessage),
                                                       *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    StartDelayTimer();

exit:
    return;
}

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Coap::Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    Dataset  dataset(GetType());

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
    dataset.SetTimestamp(aTimestamp);
    IgnoreError(DatasetManager::Save(dataset));

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
