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

#include <stdio.h>

#include <openthread-types.h>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <coap/coap_header.hpp>
#include <common/debug.hpp>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <common/timer.hpp>
#include <meshcop/dataset.hpp>
#include <meshcop/dataset_manager.hpp>
#include <meshcop/tlvs.hpp>
#include <platform/random.h>
#include <platform/radio.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <meshcop/leader.hpp>

namespace Thread {
namespace MeshCoP {

ActiveDataset::ActiveDataset(ThreadNetif &aThreadNetif):
    ActiveDatasetBase(aThreadNetif),
    mResourceGet(OPENTHREAD_URI_ACTIVE_GET, &ActiveDataset::HandleGet, this),
    mResourceSet(OPENTHREAD_URI_ACTIVE_SET, &ActiveDataset::HandleSet, this)
{
    mCoapServer.AddResource(mResourceGet);
}

bool ActiveDataset::IsTlvInitialized(Tlv::Type aType)
{
    bool rval = false;
    const Tlv *cur = reinterpret_cast<const Tlv *>(mLocal.GetBytes());
    const Tlv *end = reinterpret_cast<const Tlv *>(mLocal.GetBytes() + mLocal.GetSize());

    while (cur < end)
    {
        if (aType == cur->GetType())
        {
            ExitNow(rval = true);
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

ThreadError ActiveDataset::GenerateLocal(void)
{
    ThreadError error = kThreadError_None;
    otOperationalDataset dataset;

    VerifyOrExit(mNetif.GetMle().IsAttached(), error = kThreadError_InvalidState);

    memset(&dataset, 0, sizeof(dataset));

    // Active Timestamp
    if (!IsTlvInitialized(Tlv::kActiveTimestamp))
    {
        ActiveTimestampTlv activeTimestampTlv;
        activeTimestampTlv.Init();
        activeTimestampTlv.SetSeconds(0);
        activeTimestampTlv.SetTicks(0);
        mLocal.Set(activeTimestampTlv);
    }

    // Channel
    if (!IsTlvInitialized(Tlv::kChannel))
    {
        ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannelPage(0);
        tlv.SetChannel(mNetif.GetMac().GetChannel());
        mLocal.Set(tlv);
    }

    // channelMask
    if (!IsTlvInitialized(Tlv::kChannelMask))
    {
        ChannelMask0Tlv tlv;
        tlv.Init();
        tlv.SetMask(kPhySupportedChannelMask);
        mLocal.Set(tlv);
    }

    // Extended PAN ID
    if (!IsTlvInitialized(Tlv::kExtendedPanId))
    {
        ExtendedPanIdTlv tlv;
        tlv.Init();
        tlv.SetExtendedPanId(mNetif.GetMac().GetExtendedPanId());
        mLocal.Set(tlv);
    }

    // Mesh-Local Prefix
    if (!IsTlvInitialized(Tlv::kMeshLocalPrefix))
    {
        MeshLocalPrefixTlv tlv;
        tlv.Init();
        tlv.SetMeshLocalPrefix(mNetif.GetMle().GetMeshLocalPrefix());
        mLocal.Set(tlv);
    }

    // Master Key
    if (!IsTlvInitialized(Tlv::kNetworkMasterKey))
    {
        NetworkMasterKeyTlv tlv;
        tlv.Init();
        tlv.SetNetworkMasterKey(mNetif.GetKeyManager().GetMasterKey(NULL));
        mLocal.Set(tlv);
    }

    // Network Name
    if (!IsTlvInitialized(Tlv::kNetworkMasterKey))
    {
        NetworkNameTlv tlv;
        tlv.Init();
        tlv.SetNetworkName(mNetif.GetMac().GetNetworkName());
        mLocal.Set(tlv);
    }

    // Pan ID
    if (!IsTlvInitialized(Tlv::kPanId))
    {
        PanIdTlv tlv;
        tlv.Init();
        tlv.SetPanId(mNetif.GetMac().GetPanId());
        mLocal.Set(tlv);
    }

    // PSKc
    if (!IsTlvInitialized(Tlv::kPSKc))
    {
        const uint8_t PSKc[16] = {0};
        PSKcTlv tlv;
        tlv.Init();
        tlv.SetPSKc(PSKc);
        mLocal.Set(tlv);
    }

    // Security Policy
    if (!IsTlvInitialized(Tlv::kSecurityPolicy))
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(static_cast<uint16_t>(mNetif.GetKeyManager().GetKeyRotation()));
        tlv.SetFlags(mNetif.GetKeyManager().GetSecurityPolicyFlags());
        mLocal.Set(tlv);
    }

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    GenerateLocal();

    mLocal.Store();
    mNetwork = mLocal;
    mCoapServer.AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    mCoapServer.RemoveResource(mResourceSet);
}

void ActiveDataset::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void ActiveDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    ApplyConfiguration();

exit:
    return;
}

PendingDataset::PendingDataset(ThreadNetif &aThreadNetif):
    PendingDatasetBase(aThreadNetif),
    mResourceGet(OPENTHREAD_URI_PENDING_GET, &PendingDataset::HandleGet, this),
    mResourceSet(OPENTHREAD_URI_PENDING_SET, &PendingDataset::HandleSet, this)
{
    mCoapServer.AddResource(mResourceGet);
}

void PendingDataset::StartLeader(void)
{
    UpdateDelayTimer(mLocal, mLocalTime);
    mLocal.Store();
    mNetwork = mLocal;
    ResetDelayTimer(kFlagNetworkUpdated);

    mCoapServer.AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    mCoapServer.RemoveResource(mResourceSet);
}

void PendingDataset::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void PendingDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    ResetDelayTimer(kFlagLocalUpdated | kFlagNetworkUpdated);

exit:
    return;
}

}  // namespace MeshCoP
}  // namespace Thread
