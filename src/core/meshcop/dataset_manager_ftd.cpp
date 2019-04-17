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
#define WPP_NAME "dataset_manager_ftd.tmh"

#if OPENTHREAD_FTD

#include <stdio.h>

#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/leader.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

otError DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    Dataset dataset(mLocal.GetType());

    mLocal.Read(dataset);

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
    StateTlv::State state                    = StateTlv::kAccept;
    Dataset         dataset(mLocal.GetType());

    ActiveTimestampTlv       activeTimestamp;
    PendingTimestampTlv      pendingTimestamp;
    ChannelTlv               channel;
    CommissionerSessionIdTlv sessionId;
    MeshLocalPrefixTlv       meshLocalPrefix;
    NetworkMasterKeyTlv      masterKey;
    PanIdTlv                 panId;

    activeTimestamp.SetLength(0);
    pendingTimestamp.SetLength(0);
    channel.SetLength(0);
    masterKey.SetLength(0);
    meshLocalPrefix.SetLength(0);
    panId.SetLength(0);
    pendingTimestamp.SetLength(0);
    sessionId.SetLength(0);

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER, state = StateTlv::kReject);

    // verify that TLV data size is less than maximum TLV value size
    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize, state = StateTlv::kReject);
        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize, state = StateTlv::kReject);

    type = (strcmp(mUriSet, OT_URI_PATH_ACTIVE_SET) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) != OT_ERROR_NONE)
    {
        ExitNow(state = StateTlv::kReject);
    }

    VerifyOrExit(activeTimestamp.IsValid(), state = StateTlv::kReject);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), state = StateTlv::kReject);
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    timestamp = (type == Tlv::kActiveTimestamp) ? static_cast<Timestamp *>(&activeTimestamp)
                                                : static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.Compare(timestamp) > 0, state = StateTlv::kReject);

    // check channel
    if (Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel) == OT_ERROR_NONE)
    {
        VerifyOrExit(channel.IsValid(), state = StateTlv::kReject);

        if (channel.GetChannel() != Get<Mac::Mac>().GetPanChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panId), panId) == OT_ERROR_NONE && panId.IsValid() &&
        panId.GetPanId() != Get<Mac::Mac>().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix) == OT_ERROR_NONE &&
        meshLocalPrefix.IsValid() &&
        memcmp(&meshLocalPrefix.GetMeshLocalPrefix(), &Get<Mle::MleRouter>().GetMeshLocalPrefix(),
               meshLocalPrefix.GetLength()))
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey) == OT_ERROR_NONE &&
        masterKey.IsValid() &&
        memcmp(&masterKey.GetNetworkMasterKey(), &Get<KeyManager>().GetMasterKey(), OT_MASTER_KEY_SIZE))
    {
        doesAffectConnectivity = true;
        doesAffectMasterKey    = true;
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp &&
        (masterKey.GetLength() == 0 ||
         memcmp(&masterKey.GetNetworkMasterKey(), &Get<KeyManager>().GetMasterKey(), OT_MASTER_KEY_SIZE) == 0))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = Get<ActiveDataset>().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == NULL || localActiveTimestamp->Compare(activeTimestamp) > 0,
                     state = StateTlv::kReject);
    }

    // check commissioner session id
    if (Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId) == OT_ERROR_NONE)
    {
        CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(sessionId.IsValid() && localId != NULL &&
                         localId->GetCommissionerSessionId() == sessionId.GetCommissionerSessionId(),
                     state = StateTlv::kReject);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity,
                 state = StateTlv::kReject);

    if (isUpdateFromCommissioner)
    {
        // Thread specification allows partial dataset changes for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
        // from Commissioner based on existing active dataset.
        Get<ActiveDataset>().Read(dataset);
    }

    if (type == Tlv::kPendingTimestamp || !doesAffectConnectivity)
    {
        offset = aMessage.GetOffset();

        while (offset < aMessage.GetLength())
        {
            OT_TOOL_PACKED_BEGIN
            struct
            {
                Tlv     tlv;
                uint8_t value[Dataset::kMaxValueSize];
            } OT_TOOL_PACKED_END data;

            aMessage.Read(offset, sizeof(Tlv), &data.tlv);
            VerifyOrExit(data.tlv.GetLength() <= sizeof(data.value), state = StateTlv::kReject);

            aMessage.Read(offset + sizeof(Tlv), data.tlv.GetLength(), data.value);

            switch (data.tlv.GetType())
            {
            case Tlv::kCommissionerSessionId:
                // do not store Commissioner Session ID TLV
                break;

            case Tlv::kDelayTimer:
            {
                DelayTimerTlv *delayTimerTlv = static_cast<DelayTimerTlv *>(&data.tlv);

                if (doesAffectMasterKey && delayTimerTlv->GetDelayTimer() < DelayTimerTlv::kDelayTimerDefault)
                {
                    delayTimerTlv->SetDelayTimer(DelayTimerTlv::kDelayTimerDefault);
                }
                else if (delayTimerTlv->GetDelayTimer() < Get<Leader>().GetDelayTimerMinimal())
                {
                    delayTimerTlv->SetDelayTimer(Get<Leader>().GetDelayTimerMinimal());
                }
            }

                // fall through

            default:
                VerifyOrExit(dataset.Set(data.tlv) == OT_ERROR_NONE, state = StateTlv::kReject);
                break;
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        VerifyOrExit(Save(dataset) == OT_ERROR_NONE, state = StateTlv::kReject);
        Get<NetworkData::Leader>().IncrementVersion();
        Get<NetworkData::Leader>().IncrementStableVersion();
    }
    else
    {
        Get<PendingDataset>().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        BorderAgentLocatorTlv *borderAgentLocator;
        Ip6::Address           destination;

        borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));
        VerifyOrExit(borderAgentLocator != NULL);

        memset(&destination, 0, sizeof(destination));
        destination                = Get<Mle::MleRouter>().GetMeshLocal16();
        destination.mFields.m16[7] = HostSwap16(borderAgentLocator->GetBorderAgentLocator());

        Get<Leader>().SendDatasetChanged(destination);
    }

exit:

    if (Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        SendSetResponse(aMessage, aMessageInfo, state);
    }

    return state == StateTlv::kAccept ? OT_ERROR_NONE : OT_ERROR_DROP;
}

void DatasetManager::SendSetResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;
    StateTlv       state;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    message->SetDefaultResponseHeader(aRequest);
    message->SetPayloadMarker();

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->AppendTlv(state));

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError ActiveDataset::GenerateLocal(void)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);

    mLocal.Read(dataset);

    // Active Timestamp
    if (dataset.Get(Tlv::kActiveTimestamp) == NULL)
    {
        ActiveTimestampTlv activeTimestampTlv;
        activeTimestampTlv.Init();
        activeTimestampTlv.SetSeconds(0);
        activeTimestampTlv.SetTicks(0);
        dataset.Set(activeTimestampTlv);
    }

    // Channel
    if (dataset.Get(Tlv::kChannel) == NULL)
    {
        ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannel(Get<Mac::Mac>().GetPanChannel());
        dataset.Set(tlv);
    }

    // channelMask
    if (dataset.Get(Tlv::kChannelMask) == NULL)
    {
        ChannelMaskTlv tlv;
        tlv.Init();
        tlv.SetChannelMask(Get<Mac::Mac>().GetSupportedChannelMask().GetMask());
        dataset.Set(tlv);
    }

    // Extended PAN ID
    if (dataset.Get(Tlv::kExtendedPanId) == NULL)
    {
        ExtendedPanIdTlv tlv;
        tlv.Init();
        tlv.SetExtendedPanId(Get<Mac::Mac>().GetExtendedPanId());
        dataset.Set(tlv);
    }

    // Mesh-Local Prefix
    if (dataset.Get(Tlv::kMeshLocalPrefix) == NULL)
    {
        MeshLocalPrefixTlv tlv;
        tlv.Init();
        tlv.SetMeshLocalPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        dataset.Set(tlv);
    }

    // Master Key
    if (dataset.Get(Tlv::kNetworkMasterKey) == NULL)
    {
        NetworkMasterKeyTlv tlv;
        tlv.Init();
        tlv.SetNetworkMasterKey(Get<KeyManager>().GetMasterKey());
        dataset.Set(tlv);
    }

    // Network Name
    if (dataset.Get(Tlv::kNetworkName) == NULL)
    {
        NetworkNameTlv tlv;
        tlv.Init();
        tlv.SetNetworkName(Get<Mac::Mac>().GetNetworkName());
        dataset.Set(tlv);
    }

    // Pan ID
    if (dataset.Get(Tlv::kPanId) == NULL)
    {
        PanIdTlv tlv;
        tlv.Init();
        tlv.SetPanId(Get<Mac::Mac>().GetPanId());
        dataset.Set(tlv);
    }

    // PSKc
    if (dataset.Get(Tlv::kPSKc) == NULL)
    {
        PSKcTlv tlv;

        tlv.Init();

        if (Get<KeyManager>().IsPSKcSet())
        {
            // use configured PSKc
            tlv.SetPSKc(Get<KeyManager>().GetPSKc());
        }
        else
        {
            // PSKc has not yet been configured, generate new PSKc at random
            otPSKc pskc;
            SuccessOrExit(error = otPlatRandomGetTrue(pskc.m8, sizeof(pskc)));
            tlv.SetPSKc(pskc);
        }

        dataset.Set(tlv);
    }

    // Security Policy
    if (dataset.Get(Tlv::kSecurityPolicy) == NULL)
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(static_cast<uint16_t>(Get<KeyManager>().GetKeyRotation()));
        tlv.SetFlags(Get<KeyManager>().GetSecurityPolicyFlags());
        dataset.Set(tlv);
    }

    SuccessOrExit(error = mLocal.Save(dataset));
    Restore();

    otLogInfoMeshCoP("Generated local dataset");

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    GenerateLocal();
    Get<Coap::Coap>().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    Get<Coap::Coap>().RemoveResource(mResourceSet);
}

void ActiveDataset::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(*static_cast<Coap::Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    ApplyConfiguration();

exit:
    return;
}

void PendingDataset::StartLeader(void)
{
    StartDelayTimer();
    Get<Coap::Coap>().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    Get<Coap::Coap>().RemoveResource(mResourceSet);
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
    uint16_t      offset = aMessage.GetOffset();
    Dataset       dataset(mLocal.GetType());
    DelayTimerTlv delayTimer;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached());

    while (offset < aMessage.GetLength())
    {
        OT_TOOL_PACKED_BEGIN
        struct
        {
            Tlv     tlv;
            uint8_t value[Dataset::kMaxValueSize];
        } OT_TOOL_PACKED_END data;

        aMessage.Read(offset, sizeof(Tlv), &data.tlv);
        aMessage.Read(offset + sizeof(Tlv), data.tlv.GetLength(), data.value);
        dataset.Set(data.tlv);
        offset += sizeof(Tlv) + data.tlv.GetLength();
    }

    // add delay timer tlv
    delayTimer.Init();
    delayTimer.SetDelayTimer(Get<Leader>().GetDelayTimerMinimal());
    dataset.Set(delayTimer);

    // add pending timestamp tlv
    dataset.SetTimestamp(aTimestamp);
    DatasetManager::Save(dataset);

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
