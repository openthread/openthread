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
    StateTlv::State state                    = StateTlv::kReject;
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

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER);

    // verify that TLV data size is less than maximum TLV value size
    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize);
        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize);

    type = (strcmp(mUriSet, OT_URI_PATH_ACTIVE_SET) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) != OT_ERROR_NONE)
    {
        ExitNow();
    }

    VerifyOrExit(activeTimestamp.IsValid());

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid());
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    timestamp = (type == Tlv::kActiveTimestamp) ? static_cast<Timestamp *>(&activeTimestamp)
                                                : static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.Compare(timestamp) > 0);

    // check channel
    if (Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel) == OT_ERROR_NONE)
    {
        VerifyOrExit(channel.IsValid());

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
               meshLocalPrefix.GetMeshLocalPrefixLength()))
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey) == OT_ERROR_NONE &&
        masterKey.IsValid() && (masterKey.GetNetworkMasterKey() != Get<KeyManager>().GetMasterKey()))
    {
        doesAffectConnectivity = true;
        doesAffectMasterKey    = true;
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp &&
        ((masterKey.GetLength() == 0) || (masterKey.GetNetworkMasterKey() == Get<KeyManager>().GetMasterKey())))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = Get<ActiveDataset>().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == NULL || localActiveTimestamp->Compare(activeTimestamp) > 0);
    }

    // check commissioner session id
    if (Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId) == OT_ERROR_NONE)
    {
        CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(sessionId.IsValid() && localId != NULL &&
                     localId->GetCommissionerSessionId() == sessionId.GetCommissionerSessionId());
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity);

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

            VerifyOrExit(aMessage.Read(offset, sizeof(Tlv), &data.tlv) == sizeof(Tlv));
            VerifyOrExit(data.tlv.GetLength() <= sizeof(data.value));

            VerifyOrExit(aMessage.Read(offset + sizeof(Tlv), data.tlv.GetLength(), data.value) == data.tlv.GetLength());

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
                SuccessOrExit(dataset.Set(data.tlv));
                break;
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        SuccessOrExit(Save(dataset));
        Get<NetworkData::Leader>().IncrementVersion();
        Get<NetworkData::Leader>().IncrementStableVersion();
    }
    else
    {
        Get<PendingDataset>().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    state = StateTlv::kAccept;

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        CommissionerSessionIdTlv *localSessionId;
        Ip6::Address              destination;

        localSessionId = static_cast<CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));
        VerifyOrExit(localSessionId != NULL);

        SuccessOrExit(
            Get<Mle::MleRouter>().GetCommissionerAloc(destination, localSessionId->GetCommissionerSessionId()));

        Get<Leader>().SendDatasetChanged(destination);
    }

exit:

    if (Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER)
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
    StateTlv       state;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = state.AppendTo(*message));

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError ActiveDataset::CreateNewNetwork(otOperationalDataset &aDataset)
{
    otError          error             = OT_ERROR_NONE;
    Mac::ChannelMask supportedChannels = Get<Mac::Mac>().GetSupportedChannelMask();
    Mac::ChannelMask preferredChannels(Get<Radio>().GetPreferredChannelMask());

    memset(&aDataset, 0, sizeof(aDataset));

    aDataset.mActiveTimestamp = 1;

    SuccessOrExit(error = Random::Crypto::FillBuffer(aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey)));
    SuccessOrExit(error = static_cast<Pskc &>(aDataset.mPskc).GenerateRandom());
    SuccessOrExit(error = Random::Crypto::FillBuffer(aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId)));

    aDataset.mMeshLocalPrefix.m8[0] = 0xfd;
    SuccessOrExit(error = Random::Crypto::FillBuffer(&aDataset.mMeshLocalPrefix.m8[1], OT_MESH_LOCAL_PREFIX_SIZE - 1));

    aDataset.mSecurityPolicy.mFlags = Get<KeyManager>().GetSecurityPolicyFlags();

    // If the preferred channel mask is not empty, select a random
    // channel from it, otherwise choose one from the supported
    // channel mask.

    preferredChannels.Intersect(supportedChannels);

    if (preferredChannels.IsEmpty())
    {
        preferredChannels = supportedChannels;
    }

    aDataset.mChannel     = preferredChannels.ChooseRandomChannel();
    aDataset.mChannelMask = supportedChannels.GetMask();

    aDataset.mPanId = Mac::GenerateRandomPanId();

    snprintf(aDataset.mNetworkName.m8, sizeof(aDataset.mNetworkName), "OpenThread-%04x", aDataset.mPanId);

    aDataset.mComponents.mIsActiveTimestampPresent = true;
    aDataset.mComponents.mIsMasterKeyPresent       = true;
    aDataset.mComponents.mIsNetworkNamePresent     = true;
    aDataset.mComponents.mIsExtendedPanIdPresent   = true;
    aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    aDataset.mComponents.mIsPanIdPresent           = true;
    aDataset.mComponents.mIsChannelPresent         = true;
    aDataset.mComponents.mIsPskcPresent            = true;
    aDataset.mComponents.mIsSecurityPolicyPresent  = true;
    aDataset.mComponents.mIsChannelMaskPresent     = true;

exit:
    return error;
}

otError ActiveDataset::GenerateLocal(void)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mLocal.IsTimestampPresent(), error = OT_ERROR_ALREADY);

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
        tlv.SetNetworkName(Get<Mac::Mac>().GetNetworkName().GetAsData());
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
    if (dataset.Get(Tlv::kPskc) == NULL)
    {
        PskcTlv tlv;

        tlv.Init();

        if (Get<KeyManager>().IsPskcSet())
        {
            // use configured PSKc
            tlv.SetPskc(Get<KeyManager>().GetPskc());
        }
        else
        {
            // PSKc has not yet been configured, generate new PSKc at random
            Pskc pskc;

            SuccessOrExit(error = pskc.GenerateRandom());
            tlv.SetPskc(pskc);
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
