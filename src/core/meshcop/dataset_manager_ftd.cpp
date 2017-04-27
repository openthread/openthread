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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>

#include "openthread/platform/random.h"
#include "openthread/platform/radio.h"

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
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <meshcop/leader.hpp>

#if OPENTHREAD_FTD

namespace Thread {
namespace MeshCoP {

ActiveDataset::ActiveDataset(ThreadNetif &aThreadNetif):
    ActiveDatasetBase(aThreadNetif),
    mResourceSet(OPENTHREAD_URI_ACTIVE_SET, &ActiveDataset::HandleSet, this)
{
}

bool ActiveDataset::IsTlvInitialized(Tlv::Type aType)
{
    return mLocal.Get(aType) != NULL;
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
    if (!IsTlvInitialized(Tlv::kNetworkName))
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
        PSKcTlv tlv;
        tlv.Init();
        tlv.SetPSKc(mNetif.GetKeyManager().GetPSKc());
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
    mNetif.GetCoapServer().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    mNetif.GetCoapServer().RemoveResource(mResourceSet);
}

void ActiveDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
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
    mResourceSet(OPENTHREAD_URI_PENDING_SET, &PendingDataset::HandleSet, this)
{
}

void PendingDataset::StartLeader(void)
{
    UpdateDelayTimer(mLocal, mLocalTime);
    mLocal.Store();
    mNetwork = mLocal;
    ResetDelayTimer(kFlagNetworkUpdated);

    mNetif.GetCoapServer().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    mNetif.GetCoapServer().RemoveResource(mResourceSet);
}

void PendingDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
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

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    DelayTimerTlv delayTimer;
    uint8_t flags;

    VerifyOrExit(mNetif.GetMle().IsAttached());

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
        mNetwork.Set(data.tlv);
        offset += sizeof(Tlv) + data.tlv.GetLength();
    }

    // add delay timer tlv
    delayTimer.Init();
    delayTimer.SetDelayTimer(mNetif.GetLeader().GetDelayTimerMinimal());
    mNetwork.Set(delayTimer);

    // add pending timestamp tlv
    mNetwork.SetTimestamp(aTimestamp);
    HandleNetworkUpdate(flags);

    // reset delay timer
    ResetDelayTimer(kFlagNetworkUpdated);

exit:
    return;
}

}  // namespace MeshCoP
}  // namespace Thread

#endif // OPENTHREAD_FTD
