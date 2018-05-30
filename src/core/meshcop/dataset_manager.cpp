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

#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

DatasetManager::DatasetManager(Instance &      aInstance,
                               const Tlv::Type aType,
                               const char *    aUriGet,
                               const char *    aUriSet,
                               Timer::Handler  aTimerHandler)
    : InstanceLocator(aInstance)
    , mLocal(aInstance, aType)
    , mTimestampValid(false)
    , mTimer(aInstance, aTimerHandler, this)
    , mUriGet(aUriGet)
    , mUriSet(aUriSet)
{
    mTimestamp.Init();
}

const Timestamp *DatasetManager::GetTimestamp(void) const
{
    return mTimestampValid ? &mTimestamp : NULL;
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

otError DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    Dataset dataset(mLocal.GetType());

    mLocal.Get(dataset);

    return dataset.AppendMleDatasetTlv(aMessage);
}

otError DatasetManager::Restore(void)
{
    otError          error;
    Dataset          dataset(mLocal.GetType());
    const Timestamp *timestamp;

    mTimer.Stop();

    SuccessOrExit(error = mLocal.Restore(dataset));

    timestamp = dataset.GetTimestamp();

    if (timestamp != NULL)
    {
        mTimestamp      = *timestamp;
        mTimestampValid = true;

        if (mLocal.GetType() == Tlv::kActiveTimestamp)
        {
            dataset.ApplyConfiguration(GetInstance());
        }
    }

exit:
    return error;
}

otError DatasetManager::ApplyConfiguration(void) const
{
    otError error;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = mLocal.Get(dataset));
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
    Restore();
}

otError DatasetManager::Set(const Dataset &aDataset)
{
    otError          error = OT_ERROR_NONE;
    const Timestamp *timestamp;
    int              compare;

    timestamp = aDataset.GetTimestamp();

    if (timestamp != NULL)
    {
        mTimestamp      = *timestamp;
        mTimestampValid = true;

        if (mLocal.GetType() == Tlv::kActiveTimestamp)
        {
            SuccessOrExit(error = aDataset.ApplyConfiguration(GetInstance()));
        }
    }

    compare = mLocal.Compare(timestamp);

    if (compare > 0)
    {
        ThreadNetif &netif = GetNetif();

        mLocal.Set(aDataset);

        if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
        {
            netif.GetNetworkDataLeader().IncrementVersion();
            netif.GetNetworkDataLeader().IncrementStableVersion();
        }
    }
    else if (compare < 0)
    {
        mTimer.Start(1000);
    }

exit:
    return error;
}

void DatasetManager::HandleTimer(void)
{
    ThreadNetif &netif = GetNetif();

    VerifyOrExit(netif.GetMle().IsAttached());

    VerifyOrExit(mLocal.Compare(GetTimestamp()) < 0);

    if (mLocal.GetType() == Tlv::kActiveTimestamp)
    {
        Dataset dataset(Tlv::kPendingTimestamp);
        netif.GetPendingDataset().Get(dataset);

        const ActiveTimestampTlv *tlv = static_cast<const ActiveTimestampTlv *>(dataset.Get(Tlv::kActiveTimestamp));
        const Timestamp *         pendingActiveTimestamp = static_cast<const Timestamp *>(tlv);

        if (pendingActiveTimestamp != NULL && mLocal.Compare(pendingActiveTimestamp) >= 0)
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
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Header     header;
    Message *        message;
    Ip6::MessageInfo messageInfo;
    Dataset          dataset(mLocal.GetType());

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

void DatasetManager::Get(const Coap::Header &    aHeader,
                         const Message &         aMessage,
                         const Ip6::MessageInfo &aMessageInfo) const
{
    Tlv      tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t  tlvs[Dataset::kMaxSize];
    uint8_t  length = 0;

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

void DatasetManager::SendGetResponse(const Coap::Header &    aRequestHeader,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     uint8_t *               aTlvs,
                                     uint8_t                 aLength) const
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *    message;
    Dataset      dataset(mLocal.GetType());

    mLocal.Get(dataset);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    if (aLength == 0)
    {
        const Tlv *cur = reinterpret_cast<const Tlv *>(dataset.GetBytes());
        const Tlv *end = reinterpret_cast<const Tlv *>(dataset.GetBytes() + dataset.GetSize());

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

            if ((tlv = dataset.Get(static_cast<Tlv::Type>(aTlvs[index]))) != NULL)
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

ActiveDataset::ActiveDataset(Instance &aInstance)
    : DatasetManager(aInstance,
                     Tlv::kActiveTimestamp,
                     OT_URI_PATH_ACTIVE_GET,
                     OT_URI_PATH_ACTIVE_SET,
                     &ActiveDataset::HandleTimer)
    , mResourceGet(OT_URI_PATH_ACTIVE_GET, &ActiveDataset::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(OT_URI_PATH_ACTIVE_SET, &ActiveDataset::HandleSet, this)
#endif
{
    GetNetif().GetCoap().AddResource(mResourceGet);
}

void ActiveDataset::Clear(void)
{
    DatasetManager::Clear();
}

void ActiveDataset::Set(const Dataset &aDataset)
{
    DatasetManager::Set(aDataset);
}

otError ActiveDataset::Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = dataset.Set(aMessage, aOffset, aLength));
    dataset.SetTimestamp(aTimestamp);
    DatasetManager::Set(dataset);

exit:
    return error;
}

void ActiveDataset::HandleGet(void *               aContext,
                              otCoapHeader *       aHeader,
                              otMessage *          aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleGet(*static_cast<Coap::Header *>(aHeader),
                                                      *static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void ActiveDataset::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ActiveDataset>().HandleTimer();
}

PendingDataset::PendingDataset(Instance &aInstance)
    : DatasetManager(aInstance,
                     Tlv::kPendingTimestamp,
                     OT_URI_PATH_PENDING_GET,
                     OT_URI_PATH_PENDING_SET,
                     &PendingDataset::HandleTimer)
    , mDelayTimer(aInstance, &PendingDataset::HandleDelayTimer, this)
    , mResourceGet(OT_URI_PATH_PENDING_GET, &PendingDataset::HandleGet, this)
#if OPENTHREAD_FTD
    , mResourceSet(OT_URI_PATH_PENDING_SET, &PendingDataset::HandleSet, this)
#endif
{
    GetNetif().GetCoap().AddResource(mResourceGet);
}

void PendingDataset::Clear(void)
{
    DatasetManager::Clear();
    mDelayTimer.Stop();
}

void PendingDataset::ClearNetwork(void)
{
    Restore();
}

otError PendingDataset::Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    SuccessOrExit(error = dataset.Set(aMessage, aOffset, aLength));
    dataset.SetTimestamp(aTimestamp);
    DatasetManager::Set(dataset);
    StartDelayTimer();

exit:
    return error;
}

void PendingDataset::StartDelayTimer(void)
{
    DelayTimerTlv *delayTimer;
    Dataset        dataset(mLocal.GetType());

    mLocal.Get(dataset);

    mDelayTimer.Stop();

    if ((delayTimer = static_cast<DelayTimerTlv *>(dataset.Get(Tlv::kDelayTimer))) != NULL)
    {
        uint32_t delay = delayTimer->GetDelayTimer();

        // the Timer implementation does not support the full 32 bit range
        if (delay > Timer::kMaxDt)
        {
            delay = Timer::kMaxDt;
        }

        mDelayTimer.StartAt(dataset.GetUpdateTime(), delay);
        otLogInfoMeshCoP(GetInstance(), "delay timer started %d", delay);
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

    mLocal.Get(dataset);

    // if the Delay Timer value is larger than what our Timer implementation can handle, we have to compute
    // the remainder and wait some more.
    if ((delayTimer = static_cast<DelayTimerTlv *>(dataset.Get(Tlv::kDelayTimer))) != NULL)
    {
        uint32_t elapsed = mDelayTimer.GetFireTime() - dataset.GetUpdateTime();
        uint32_t delay   = delayTimer->GetDelayTimer();

        if (elapsed < delay)
        {
            mDelayTimer.StartAt(mDelayTimer.GetFireTime(), delay - elapsed);
            ExitNow();
        }
    }

    otLogInfoMeshCoP(GetInstance(), "pending delay timer expired");

    dataset.ConvertToActive();
    GetNetif().GetActiveDataset().Set(dataset);

    Clear();

exit:
    return;
}

void PendingDataset::HandleGet(void *               aContext,
                               otCoapHeader *       aHeader,
                               otMessage *          aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleGet(*static_cast<Coap::Header *>(aHeader),
                                                       *static_cast<Message *>(aMessage),
                                                       *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void PendingDataset::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<PendingDataset>().HandleTimer();
}

} // namespace MeshCoP
} // namespace ot
