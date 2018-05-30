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

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
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

otError DatasetManager::Set(const otOperationalDataset &aDataset)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    SuccessOrExit(error = mLocal.Set(aDataset));

    switch (netif.GetMle().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
        Restore();
        break;

    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_ROUTER:
        mTimer.Start(1000);
        break;

    case OT_DEVICE_ROLE_LEADER:
        Restore();
        netif.GetNetworkDataLeader().IncrementVersion();
        netif.GetNetworkDataLeader().IncrementStableVersion();
        break;

    default:
        break;
    }

exit:
    return error;
}

otError DatasetManager::Set(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &   netif = GetNetif();
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

    mLocal.Get(dataset);

    activeTimestamp.SetLength(0);
    pendingTimestamp.SetLength(0);
    channel.SetLength(0);
    masterKey.SetLength(0);
    meshLocalPrefix.SetLength(0);
    panId.SetLength(0);
    pendingTimestamp.SetLength(0);
    sessionId.SetLength(0);

    VerifyOrExit(netif.GetMle().GetRole() == OT_DEVICE_ROLE_LEADER, state = StateTlv::kReject);

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
        VerifyOrExit(channel.IsValid() && channel.GetChannel() >= OT_RADIO_CHANNEL_MIN &&
                         channel.GetChannel() <= OT_RADIO_CHANNEL_MAX,
                     state = StateTlv::kReject);

        if (channel.GetChannel() != netif.GetMac().GetPanChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panId), panId) == OT_ERROR_NONE && panId.IsValid() &&
        panId.GetPanId() != netif.GetMac().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix) == OT_ERROR_NONE &&
        meshLocalPrefix.IsValid() &&
        memcmp(meshLocalPrefix.GetMeshLocalPrefix(), netif.GetMle().GetMeshLocalPrefix(), meshLocalPrefix.GetLength()))
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey) == OT_ERROR_NONE &&
        masterKey.IsValid() &&
        memcmp(&masterKey.GetNetworkMasterKey(), &netif.GetKeyManager().GetMasterKey(), OT_MASTER_KEY_SIZE))
    {
        doesAffectConnectivity = true;
        doesAffectMasterKey    = true;
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp &&
        (masterKey.GetLength() == 0 ||
         memcmp(&masterKey.GetNetworkMasterKey(), &netif.GetKeyManager().GetMasterKey(), OT_MASTER_KEY_SIZE) == 0))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = netif.GetActiveDataset().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == NULL || localActiveTimestamp->Compare(activeTimestamp) > 0,
                     state = StateTlv::kReject);
    }

    // check commissioner session id
    if (Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId) == OT_ERROR_NONE)
    {
        CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<CommissionerSessionIdTlv *>(
            netif.GetNetworkDataLeader().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(sessionId.IsValid() && localId != NULL &&
                         localId->GetCommissionerSessionId() == sessionId.GetCommissionerSessionId(),
                     state = StateTlv::kReject);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity,
                 state = StateTlv::kReject);

    // update dataset
    // Thread specification allows partial dataset changes for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
    // from Commissioner.
    // Updates based on existing active/pending dataset if it is from Commissioner.
    if (isUpdateFromCommissioner)
    {
        // take active dataset as the update base for MGMT_PENDING_SET.req if no existing pending dataset.
        if (type == Tlv::kPendingTimestamp && dataset.GetSize() == 0)
        {
            netif.GetActiveDataset().Get(dataset);
        }
    }

#if 0
    // Interim workaround for certification:
    // Thread specification requires entire dataset for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req from thread device.
    // Not all stack vendors would send entire dataset in MGMT_ACTIVE_SET.req triggered by command as known when
    // testing 9.2.5.
    // So here would accept even if it is not entire, update the Tlvs in the message on existing dataset in
    // order to avoid interop issue for now.
    // TODO: remove '#if 0' condition after all stack vendors reach consensus-MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
    // from thread device triggered by command would include entire dataset as expected.
    else
    {
        dataset.Clear();
    }

#endif

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
                else if (delayTimerTlv->GetDelayTimer() < netif.GetLeader().GetDelayTimerMinimal())
                {
                    delayTimerTlv->SetDelayTimer(netif.GetLeader().GetDelayTimerMinimal());
                }
            }

                // fall through

            default:
                VerifyOrExit(dataset.Set(data.tlv) == OT_ERROR_NONE, state = StateTlv::kReject);
                break;
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        VerifyOrExit(Set(dataset) == OT_ERROR_NONE, state = StateTlv::kReject);
        netif.GetNetworkDataLeader().IncrementVersion();
        netif.GetNetworkDataLeader().IncrementStableVersion();
    }
    else
    {
        netif.GetPendingDataset().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        BorderAgentLocatorTlv *borderAgentLocator;
        Ip6::Address           destination;

        borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(
            netif.GetNetworkDataLeader().GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));
        VerifyOrExit(borderAgentLocator != NULL);

        memset(&destination, 0, sizeof(destination));
        destination                = netif.GetMle().GetMeshLocal16();
        destination.mFields.m16[7] = HostSwap16(borderAgentLocator->GetBorderAgentLocator());

        netif.GetLeader().SendDatasetChanged(destination);
    }

exit:

    if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        SendSetResponse(aHeader, aMessageInfo, state);
    }

    return state == StateTlv::kAccept ? OT_ERROR_NONE : OT_ERROR_DROP;
}

otError DatasetManager::SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Header     header;
    Message *        message;
    Ip6::MessageInfo messageInfo;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriSet);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

#if OPENTHREAD_ENABLE_COMMISSIONER

    if (netif.GetCommissioner().IsActive())
    {
        const uint8_t *cur          = aTlvs;
        const uint8_t *end          = aTlvs + aLength;
        bool           hasSessionId = false;

        while (cur < end)
        {
            const Tlv *data = reinterpret_cast<const Tlv *>(cur);

            if (data->GetType() == Tlv::kCommissionerSessionId)
            {
                hasSessionId = true;
                break;
            }

            cur += sizeof(Tlv) + data->GetLength();
        }

        if (!hasSessionId)
        {
            CommissionerSessionIdTlv sessionId;
            sessionId.Init();
            sessionId.SetCommissionerSessionId(netif.GetCommissioner().GetSessionId());
            SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));
        }
    }

#endif // OPENTHREAD_ENABLE_COMMISSIONER

    if (aDataset.mIsActiveTimestampSet)
    {
        ActiveTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds(aDataset.mActiveTimestamp);
        static_cast<Timestamp *>(&timestamp)->SetTicks(0);
        SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    }

    if (aDataset.mIsPendingTimestampSet)
    {
        PendingTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds(aDataset.mPendingTimestamp);
        static_cast<Timestamp *>(&timestamp)->SetTicks(0);
        SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    }

    if (aDataset.mIsMasterKeySet)
    {
        NetworkMasterKeyTlv masterkey;
        masterkey.Init();
        masterkey.SetNetworkMasterKey(aDataset.mMasterKey);
        SuccessOrExit(error = message->Append(&masterkey, sizeof(masterkey)));
    }

    if (aDataset.mIsNetworkNameSet)
    {
        NetworkNameTlv networkname;
        networkname.Init();
        networkname.SetNetworkName(aDataset.mNetworkName.m8);
        SuccessOrExit(error = message->Append(&networkname, sizeof(Tlv) + networkname.GetLength()));
    }

    if (aDataset.mIsExtendedPanIdSet)
    {
        ExtendedPanIdTlv extpanid;
        extpanid.Init();
        extpanid.SetExtendedPanId(aDataset.mExtendedPanId.m8);
        SuccessOrExit(error = message->Append(&extpanid, sizeof(extpanid)));
    }

    if (aDataset.mIsMeshLocalPrefixSet)
    {
        MeshLocalPrefixTlv localprefix;
        localprefix.Init();
        localprefix.SetMeshLocalPrefix(aDataset.mMeshLocalPrefix.m8);
        SuccessOrExit(error = message->Append(&localprefix, sizeof(localprefix)));
    }

    if (aDataset.mIsDelaySet)
    {
        DelayTimerTlv delaytimer;
        delaytimer.Init();
        delaytimer.SetDelayTimer(aDataset.mDelay);
        SuccessOrExit(error = message->Append(&delaytimer, sizeof(delaytimer)));
    }

    if (aDataset.mIsPanIdSet)
    {
        PanIdTlv panid;
        panid.Init();
        panid.SetPanId(aDataset.mPanId);
        SuccessOrExit(error = message->Append(&panid, sizeof(panid)));
    }

    if (aDataset.mIsChannelSet)
    {
        ChannelTlv channel;
        channel.Init();
        channel.SetChannelPage(0);
        channel.SetChannel(aDataset.mChannel);
        SuccessOrExit(error = message->Append(&channel, sizeof(channel)));
    }

    if (aDataset.mIsChannelMaskPage0Set)
    {
        ChannelMask0Tlv channelMask;
        channelMask.Init();
        channelMask.SetMask(aDataset.mChannelMaskPage0);
        SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    if (message->GetLength() == header.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset set request to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError DatasetManager::SendGetRequest(const uint8_t *aTlvTypes, uint8_t aLength, const otIp6Address *aAddress) const
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Header     header;
    Message *        message;
    Ip6::MessageInfo messageInfo;
    Tlv              tlv;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriGet);

    if (aLength > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    if (aLength > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvTypes, aLength));
    }

    if (aAddress != NULL)
    {
        messageInfo.SetPeerAddr(*static_cast<const Ip6::Address *>(aAddress));
    }
    else
    {
        netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    }

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset get request");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void DatasetManager::SendSetResponse(const Coap::Header &    aRequestHeader,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *    message;
    StateTlv     state;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset set response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError ActiveDataset::Set(const otOperationalDataset &aDataset)
{
    return DatasetManager::Set(aDataset);
}

otError ActiveDataset::GenerateLocal(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Dataset      dataset(mLocal.GetType());

    VerifyOrExit(netif.GetMle().IsAttached(), error = OT_ERROR_INVALID_STATE);

    mLocal.Get(dataset);

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
        tlv.SetChannelPage(0);
        tlv.SetChannel(netif.GetMac().GetPanChannel());
        dataset.Set(tlv);
    }

    // channelMask
    if (dataset.Get(Tlv::kChannelMask) == NULL)
    {
        ChannelMask0Tlv tlv;
        tlv.Init();
        tlv.SetMask(OT_RADIO_SUPPORTED_CHANNELS);
        dataset.Set(tlv);
    }

    // Extended PAN ID
    if (dataset.Get(Tlv::kExtendedPanId) == NULL)
    {
        ExtendedPanIdTlv tlv;
        tlv.Init();
        tlv.SetExtendedPanId(netif.GetMac().GetExtendedPanId());
        dataset.Set(tlv);
    }

    // Mesh-Local Prefix
    if (dataset.Get(Tlv::kMeshLocalPrefix) == NULL)
    {
        MeshLocalPrefixTlv tlv;
        tlv.Init();
        tlv.SetMeshLocalPrefix(netif.GetMle().GetMeshLocalPrefix());
        dataset.Set(tlv);
    }

    // Master Key
    if (dataset.Get(Tlv::kNetworkMasterKey) == NULL)
    {
        NetworkMasterKeyTlv tlv;
        tlv.Init();
        tlv.SetNetworkMasterKey(netif.GetKeyManager().GetMasterKey());
        dataset.Set(tlv);
    }

    // Network Name
    if (dataset.Get(Tlv::kNetworkName) == NULL)
    {
        NetworkNameTlv tlv;
        tlv.Init();
        tlv.SetNetworkName(netif.GetMac().GetNetworkName());
        dataset.Set(tlv);
    }

    // Pan ID
    if (dataset.Get(Tlv::kPanId) == NULL)
    {
        PanIdTlv tlv;
        tlv.Init();
        tlv.SetPanId(netif.GetMac().GetPanId());
        dataset.Set(tlv);
    }

    // PSKc
    if (dataset.Get(Tlv::kPSKc) == NULL)
    {
        PSKcTlv tlv;
        tlv.Init();
        tlv.SetPSKc(netif.GetKeyManager().GetPSKc());
        dataset.Set(tlv);
    }

    // Security Policy
    if (dataset.Get(Tlv::kSecurityPolicy) == NULL)
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(static_cast<uint16_t>(netif.GetKeyManager().GetKeyRotation()));
        tlv.SetFlags(netif.GetKeyManager().GetSecurityPolicyFlags());
        dataset.Set(tlv);
    }

    SuccessOrExit(error = mLocal.Set(dataset));
    Restore();

    otLogInfoMeshCoP(GetInstance(), "Generated local dataset");

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    GenerateLocal();
    GetNetif().GetCoap().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    GetNetif().GetCoap().RemoveResource(mResourceSet);
}

void ActiveDataset::HandleSet(void *               aContext,
                              otCoapHeader *       aHeader,
                              otMessage *          aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(*static_cast<Coap::Header *>(aHeader),
                                                      *static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    ApplyConfiguration();

exit:
    return;
}

otError PendingDataset::Set(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    StartDelayTimer();

exit:
    return error;
}

void PendingDataset::StartLeader(void)
{
    StartDelayTimer();
    GetNetif().GetCoap().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    GetNetif().GetCoap().RemoveResource(mResourceSet);
}

void PendingDataset::HandleSet(void *               aContext,
                               otCoapHeader *       aHeader,
                               otMessage *          aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleSet(*static_cast<Coap::Header *>(aHeader),
                                                       *static_cast<Message *>(aMessage),
                                                       *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    StartDelayTimer();

exit:
    return;
}

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage)
{
    ThreadNetif & netif  = GetNetif();
    uint16_t      offset = aMessage.GetOffset();
    Dataset       dataset(mLocal.GetType());
    DelayTimerTlv delayTimer;

    VerifyOrExit(netif.GetMle().IsAttached());

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
    delayTimer.SetDelayTimer(netif.GetLeader().GetDelayTimerMinimal());
    dataset.Set(delayTimer);

    // add pending timestamp tlv
    dataset.SetTimestamp(aTimestamp);
    DatasetManager::Set(dataset);

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
