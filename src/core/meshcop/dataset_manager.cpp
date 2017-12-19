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

#define WPP_NAME "dataset_manager.tmh"

#include "dataset_manager.hpp"

#include <stdio.h>

#include <openthread/types.h>
#include <openthread/platform/random.h>
#include <openthread/platform/radio.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/timer.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/leader.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

DatasetManager::DatasetManager(Instance &aInstance, const Tlv::Type aType, const char *aUriSet,
                               const char *aUriGet, Timer::Handler aTimerHandler):
    InstanceLocator(aInstance),
    mNetwork(aType),
    mLocal(aInstance, aType),
    mTimer(aInstance, aTimerHandler, this),
    mUriSet(aUriSet),
    mUriGet(aUriGet)
{
}

const Timestamp *DatasetManager::GetTimestamp(void) const
{
    return mNetwork.GetTimestamp();
}

int DatasetManager::Compare(const Timestamp &aTimestamp) const
{
    const Timestamp *timestamp = mNetwork.GetTimestamp();
    int rval = 1;

    if (timestamp)
    {
        rval = timestamp->Compare(aTimestamp);
    }

    return rval;
}

otError DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    return mNetwork.AppendMleDatasetTlv(aMessage);
}

const Tlv *DatasetManager::GetTlv(Tlv::Type aType) const
{
    return mNetwork.Get(aType);
}

otError DatasetManager::ApplyConfiguration(void)
{
    ThreadNetif &netif = GetNetif();
    Mac::Mac &mac = netif.GetMac();
    otError error = OT_ERROR_NONE;
    Dataset datasetLocal(mLocal.GetType());
    Dataset *dataset;
    const Tlv *cur;
    const Tlv *end;

    if (netif.GetMle().IsAttached())
    {
        dataset = &mNetwork;
    }
    else
    {
        mLocal.Get(datasetLocal);
        dataset = &datasetLocal;
    }

    cur = reinterpret_cast<const Tlv *>(dataset->GetBytes());
    end = reinterpret_cast<const Tlv *>(dataset->GetBytes() + dataset->GetSize());

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case Tlv::kChannel:
        {
            uint8_t channel = static_cast<uint8_t>(static_cast<const ChannelTlv *>(cur)->GetChannel());

            if (mac.GetChannel() != channel)
            {
                error = mac.SetChannel(channel);

                if (error != OT_ERROR_NONE)
                {
                    otLogWarnMeshCoP(GetInstance(),
                                     "DatasetManager::ApplyConfiguration() Failed to set channel to %d (%s)",
                                     channel, otThreadErrorToString(error));
                    ExitNow();
                }

                GetNotifier().SetFlags(OT_CHANGED_THREAD_CHANNEL);
            }

            break;
        }

        case Tlv::kPanId:
        {
            uint16_t panid = static_cast<const PanIdTlv *>(cur)->GetPanId();

            if (mac.GetPanId() != panid)
            {
                mac.SetPanId(panid);
                GetNotifier().SetFlags(OT_CHANGED_THREAD_PANID);
            }

            break;
        }

        case Tlv::kExtendedPanId:
        {
            const ExtendedPanIdTlv *extpanid = static_cast<const ExtendedPanIdTlv *>(cur);

            if (memcmp(mac.GetExtendedPanId(), extpanid->GetExtendedPanId(), OT_EXT_PAN_ID_SIZE) != 0)
            {
                mac.SetExtendedPanId(extpanid->GetExtendedPanId());
                GetNotifier().SetFlags(OT_CHANGED_THREAD_EXT_PANID);
            }

            break;
        }

        case Tlv::kNetworkName:
        {
            const NetworkNameTlv *name = static_cast<const NetworkNameTlv *>(cur);
            otNetworkName networkName;
            memcpy(networkName.m8, name->GetNetworkName(), name->GetLength());
            networkName.m8[name->GetLength()] = '\0';

            if (strcmp(networkName.m8, mac.GetNetworkName()) != 0)
            {
                mac.SetNetworkName(networkName.m8);
                GetNotifier().SetFlags(OT_CHANGED_THREAD_NETWORK_NAME);
            }

            break;
        }

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *key = static_cast<const NetworkMasterKeyTlv *>(cur);
            netif.GetKeyManager().SetMasterKey(key->GetNetworkMasterKey());
            break;
        }

#if OPENTHREAD_FTD

        case Tlv::kPSKc:
        {
            const PSKcTlv *pskc = static_cast<const PSKcTlv *>(cur);
            netif.GetKeyManager().SetPSKc(pskc->GetPSKc());
            break;
        }

#endif

        case Tlv::kMeshLocalPrefix:
        {
            const MeshLocalPrefixTlv *prefix = static_cast<const MeshLocalPrefixTlv *>(cur);
            netif.GetMle().SetMeshLocalPrefix(prefix->GetMeshLocalPrefix());
            break;
        }

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *securityPolicy = static_cast<const SecurityPolicyTlv *>(cur);
            netif.GetKeyManager().SetKeyRotation(securityPolicy->GetRotationTime());
            netif.GetKeyManager().SetSecurityPolicyFlags(securityPolicy->GetFlags());
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

otError DatasetManager::Restore(void)
{
    return mLocal.Get(mNetwork);
}

void DatasetManager::Clear(void)
{
    mNetwork.Clear();
    mLocal.Clear();
    mTimer.Stop();
}

void DatasetManager::HandleDetach(void)
{
    mLocal.Get(mNetwork);
    mTimer.Stop();
}

void DatasetManager::Set(const Dataset &aDataset)
{
    mNetwork.Set(aDataset);
    mLocal.Set(aDataset);
}

otError DatasetManager::Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mNetwork.Set(aMessage, aOffset, aLength));
    mNetwork.SetTimestamp(aTimestamp);
    HandleNetworkUpdate();

exit:
    return error;
}

#if OPENTHREAD_FTD

otError DatasetManager::Set(const otOperationalDataset &aDataset)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mLocal.Set(aDataset));

    switch (netif.GetMle().GetRole())
    {
    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_ROUTER:
        mTimer.Start(1000);
        break;

    case OT_DEVICE_ROLE_LEADER:
        mLocal.Get(mNetwork);
        netif.GetNetworkDataLeader().IncrementVersion();
        netif.GetNetworkDataLeader().IncrementStableVersion();
        break;

    default:
        break;
    }

exit:
    return error;
}

#endif // OPENTHREAD_FTD

void DatasetManager::HandleNetworkUpdate(void)
{
    int compare = mLocal.Compare(mNetwork.GetTimestamp());

    if (compare > 0)
    {
        mLocal.Set(mNetwork);
    }
    else if (compare < 0)
    {
        mTimer.Start(1000);
    }
}

void DatasetManager::HandleTimer(void)
{
    ThreadNetif &netif = GetNetif();

    VerifyOrExit(netif.GetActiveDataset().GetLocal().IsPresent());

    VerifyOrExit(netif.GetMle().IsAttached());

    VerifyOrExit(mLocal.Compare(mNetwork.GetTimestamp()) < 0);

    if (mLocal.GetType() == Tlv::kActiveTimestamp)
    {
        const ActiveTimestampTlv *tlv = static_cast<const ActiveTimestampTlv *>(netif.GetPendingDataset().GetTlv(
                                                                                    Tlv::kActiveTimestamp));
        const Timestamp *pendingActiveTimestamp = static_cast<const Timestamp *>(tlv);

        if (pendingActiveTimestamp != NULL &&
            netif.GetActiveDataset().GetLocal().Compare(pendingActiveTimestamp) >= 0)
        {
            // stop registration attempts during dataset transition
            ExitNow();
        }
    }

    Register();
    mTimer.Start(1000);

exit:
    return;
}

otError DatasetManager::Register(void)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;
    Dataset dataset(mLocal.GetType());

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriSet);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    mLocal.Get(dataset);
    SuccessOrExit(error = message->Append(dataset.GetBytes(), dataset.GetSize()));

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void DatasetManager::Get(const Coap::Header &aHeader, const Message &aMessage,
                         const Ip6::MessageInfo &aMessageInfo) const
{
    Tlv tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t tlvs[Dataset::kMaxSize];
    uint8_t length = 0;

    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == Tlv::kGet)
        {
            length = tlv.GetLength();
            aMessage.Read(offset + sizeof(Tlv), length, tlvs);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    // MGMT_PENDING_GET.rsp must include Delay Timer TLV (Thread 1.1.1 Section 8.7.5.4)
    if (length != 0 && strcmp(mUriGet, OT_URI_PATH_PENDING_GET) == 0)
    {
        uint16_t i;

        for (i = 0; i < length; i++)
        {
            if (tlvs[i] == Tlv::kDelayTimer)
            {
                break;
            }
        }

        if (i == length && (i + 1u) <= sizeof(tlvs))
        {
            tlvs[length++] = Tlv::kDelayTimer;
        }
    }

    SendGetResponse(aHeader, aMessageInfo, tlvs, length);
}

#if OPENTHREAD_FTD
otError DatasetManager::Set(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    Tlv tlv;
    Timestamp *timestamp;
    uint16_t offset = aMessage.GetOffset();
    Tlv::Type type;
    bool isUpdateFromCommissioner = false;
    bool doesAffectConnectivity = false;
    bool doesAffectMasterKey = false;
    StateTlv::State state = StateTlv::kAccept;

    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    ChannelTlv channel;
    CommissionerSessionIdTlv sessionId;
    MeshLocalPrefixTlv meshLocalPrefix;
    NetworkMasterKeyTlv masterKey;
    PanIdTlv panId;

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
    timestamp = (type == Tlv::kActiveTimestamp) ?
                static_cast<Timestamp *>(&activeTimestamp) :
                static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.Compare(timestamp) > 0, state = StateTlv::kReject);

    // check channel
    if (Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel) == OT_ERROR_NONE)
    {
        VerifyOrExit(channel.IsValid() &&
                     channel.GetChannel() >= OT_RADIO_CHANNEL_MIN &&
                     channel.GetChannel() <= OT_RADIO_CHANNEL_MAX,
                     state = StateTlv::kReject);

        if (channel.GetChannel() != netif.GetMac().GetChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panId), panId) == OT_ERROR_NONE &&
        panId.IsValid() &&
        panId.GetPanId() != netif.GetMac().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix) == OT_ERROR_NONE &&
        meshLocalPrefix.IsValid() &&
        memcmp(meshLocalPrefix.GetMeshLocalPrefix(), netif.GetMle().GetMeshLocalPrefix(),
               meshLocalPrefix.GetLength()))
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey) == OT_ERROR_NONE &&
        masterKey.IsValid() &&
        memcmp(&masterKey.GetNetworkMasterKey(), &netif.GetKeyManager().GetMasterKey(), OT_MASTER_KEY_SIZE))
    {
        doesAffectConnectivity = true;
        doesAffectMasterKey = true;
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

        localId = static_cast<CommissionerSessionIdTlv *>(netif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                              Tlv::kCommissionerSessionId));

        VerifyOrExit(sessionId.IsValid() &&
                     localId != NULL &&
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
        if (type == Tlv::kPendingTimestamp && mNetwork.GetSize() == 0)
        {
            mNetwork.Set(netif.GetActiveDataset().GetNetwork());
        }
    }

#if 0
    // Interim workaround for certification:
    // Thread specification requires entire dataset for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req from thread device.
    // Not all stack vendors would send entire dataset in MGMT_ACTIVE_SET.req triggered by command as known when
    // testing 9.2.5.
    // So here would accept even if it is not entire, update the Tlvs in the message on existing mNetwork in
    // order to avoid interop issue for now.
    // TODO: remove '#if 0' condition after all stack vendors reach consensus-MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
    // from thread device triggered by command would include entire dataset as expected.
    else
    {
        mNetwork.Clear();
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
                Tlv tlv;
                uint8_t value[Dataset::kMaxValueSize];
            } OT_TOOL_PACKED_END data;

            aMessage.Read(offset, sizeof(Tlv), &data.tlv);
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
                mNetwork.Set(data.tlv);
                break;
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        mLocal.Set(mNetwork);
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
        Ip6::Address destination;

        borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(netif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                                      Tlv::kBorderAgentLocator));
        VerifyOrExit(borderAgentLocator != NULL);

        memset(&destination, 0, sizeof(destination));
        destination = netif.GetMle().GetMeshLocal16();
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
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriSet);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    if (netif.GetCommissioner().IsActive())
    {
        const uint8_t *cur = aTlvs;
        const uint8_t *end = aTlvs + aLength;
        bool hasSessionId = false;

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

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

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

otError DatasetManager::SendGetRequest(const uint8_t *aTlvTypes, uint8_t aLength, const otIp6Address *aAddress)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;
    Tlv tlv;

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

void DatasetManager::SendSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State aState)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *message;
    StateTlv state;

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
#endif // OPENTHREAD_FTD

void DatasetManager::SendGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     uint8_t *aTlvs, uint8_t aLength) const
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *message;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    if (aLength == 0)
    {
        const Tlv *cur = reinterpret_cast<const Tlv *>(mNetwork.GetBytes());
        const Tlv *end = reinterpret_cast<const Tlv *>(mNetwork.GetBytes() + mNetwork.GetSize());

        while (cur < end)
        {
            if (cur->GetType() != Tlv::kNetworkMasterKey ||
                (netif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY))
            {
                SuccessOrExit(error = message->Append(cur, sizeof(Tlv) + cur->GetLength()));
            }

            cur = cur->GetNext();
        }
    }
    else
    {
        for (uint8_t index = 0; index < aLength; index++)
        {
            const Tlv *tlv;

            if (aTlvs[index] == Tlv::kNetworkMasterKey &&
                !(netif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY))
            {
                continue;
            }

            if ((tlv = mNetwork.Get(static_cast<Tlv::Type>(aTlvs[index]))) != NULL)
            {
                SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
            }
        }
    }

    if (message->GetLength() == responseHeader.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset get response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

ActiveDatasetBase::ActiveDatasetBase(Instance &aInstance):
    DatasetManager(aInstance, Tlv::kActiveTimestamp, OT_URI_PATH_ACTIVE_SET, OT_URI_PATH_ACTIVE_GET,
                   &ActiveDatasetBase::HandleTimer),
    mResourceGet(OT_URI_PATH_ACTIVE_GET, &ActiveDatasetBase::HandleGet, this)
{
    GetNetif().GetCoap().AddResource(mResourceGet);
}

otError ActiveDatasetBase::Restore(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Restore());
    SuccessOrExit(error = DatasetManager::ApplyConfiguration());

exit:
    return error;
}

void ActiveDatasetBase::Clear(void)
{
    DatasetManager::Clear();
}

void ActiveDatasetBase::HandleDetach(void)
{
    DatasetManager::HandleDetach();
    DatasetManager::ApplyConfiguration();
}

#if OPENTHREAD_FTD
otError ActiveDatasetBase::Set(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}
#endif // OPENTHREAD_FTD

void ActiveDatasetBase::Set(const Dataset &aDataset)
{
    ThreadNetif &netif = GetNetif();

    DatasetManager::Set(aDataset);
    DatasetManager::ApplyConfiguration();

    if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        netif.GetNetworkDataLeader().IncrementVersion();
        netif.GetNetworkDataLeader().IncrementStableVersion();
    }
}

otError ActiveDatasetBase::Set(const Timestamp &aTimestamp, const Message &aMessage,
                               uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}

void ActiveDatasetBase::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                  const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDatasetBase *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDatasetBase::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void ActiveDatasetBase::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ActiveDatasetBase>().HandleTimer();
}

PendingDatasetBase::PendingDatasetBase(Instance &aInstance):
    DatasetManager(aInstance, Tlv::kPendingTimestamp, OT_URI_PATH_PENDING_SET, OT_URI_PATH_PENDING_GET,
                   &PendingDatasetBase::HandleTimer),
    mDelayTimer(aInstance, &PendingDatasetBase::HandleDelayTimer, this),
    mResourceGet(OT_URI_PATH_PENDING_GET, &PendingDatasetBase::HandleGet, this)
{
    GetNetif().GetCoap().AddResource(mResourceGet);
}

otError PendingDatasetBase::Restore(void)
{
    return DatasetManager::Restore();
}

void PendingDatasetBase::Clear(void)
{
    DatasetManager::Clear();
    mDelayTimer.Stop();
}

void PendingDatasetBase::ClearNetwork(void)
{
    mNetwork.Clear();
    mDelayTimer.Stop();
    DatasetManager::HandleNetworkUpdate();
}

void PendingDatasetBase::HandleDetach(void)
{
    DatasetManager::HandleDetach();
    StartDelayTimer();
}

#if OPENTHREAD_FTD
otError PendingDatasetBase::Set(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    StartDelayTimer();

exit:
    return error;
}
#endif // OPENTHREAD_FTD

void PendingDatasetBase::Set(const Dataset &aDataset)
{
    DatasetManager::Set(aDataset);
    StartDelayTimer();
}

otError PendingDatasetBase::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength));
    StartDelayTimer();

exit:
    return error;
}

void PendingDatasetBase::StartDelayTimer(void)
{
    DelayTimerTlv *delayTimer;

    mDelayTimer.Stop();

    if ((delayTimer = static_cast<DelayTimerTlv *>(mNetwork.Get(Tlv::kDelayTimer))) != NULL)
    {
        uint32_t delay = delayTimer->GetDelayTimer();

        // the Timer implementation does not support the full 32 bit range
        if (delay > Timer::kMaxDt)
        {
            delay = Timer::kMaxDt;
        }

        mDelayTimer.StartAt(mNetwork.GetUpdateTime(), delay);
        otLogInfoMeshCoP(GetInstance(), "delay timer started");
    }
}

void PendingDatasetBase::HandleDelayTimer(Timer &aTimer)
{
    aTimer.GetOwner<PendingDatasetBase>().HandleDelayTimer();
}

void PendingDatasetBase::HandleDelayTimer(void)
{
    DelayTimerTlv *delayTimer;

    // if the Delay Timer value is larger than what our Timer implementation can handle, we have to compute
    // the remainder and wait some more.
    if ((delayTimer = static_cast<DelayTimerTlv *>(mNetwork.Get(Tlv::kDelayTimer))) != NULL)
    {
        uint32_t elapsed = mDelayTimer.GetFireTime() - mNetwork.GetUpdateTime();
        uint32_t delay = delayTimer->GetDelayTimer();

        if (elapsed < delay)
        {
            mDelayTimer.StartAt(mDelayTimer.GetFireTime(), delay - elapsed);
            ExitNow();
        }
    }

    otLogInfoMeshCoP(GetInstance(), "pending delay timer expired");

    GetNetif().GetActiveDataset().Set(mNetwork);

    Clear();

exit:
    return;
}

void PendingDatasetBase::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDatasetBase *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDatasetBase::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void PendingDatasetBase::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<PendingDatasetBase>().HandleTimer();
}

}  // namespace MeshCoP
}  // namespace ot
