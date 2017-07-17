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

#define WPP_NAME "dataset_manager_ftd.tmh"

#include <openthread/config.h>

#include <stdio.h>

#include <openthread/platform/random.h>
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "coap/coap_header.hpp"
#include "common/debug.hpp"
#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/leader.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

ActiveDataset::ActiveDataset(ThreadNetif &aThreadNetif):
    ActiveDatasetBase(aThreadNetif),
    mResourceSet(OT_URI_PATH_ACTIVE_SET, &ActiveDataset::HandleSet, this)
{
}

otError ActiveDataset::GenerateLocal(void)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

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
        tlv.SetChannel(netif.GetMac().GetChannel());
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

    otLogInfoMeshCoP(GetInstance(), "Generated local dataset");

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    GenerateLocal();

    mLocal.Get(mNetwork);
    GetNetif().GetCoap().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    GetNetif().GetCoap().RemoveResource(mResourceSet);
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
    mResourceSet(OT_URI_PATH_PENDING_SET, &PendingDataset::HandleSet, this)
{
}

void PendingDataset::StartLeader(void)
{
    mLocal.Get(mNetwork);
    StartDelayTimer();

    GetNetif().GetCoap().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    GetNetif().GetCoap().RemoveResource(mResourceSet);
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
    StartDelayTimer();

exit:
    return;
}

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage)
{
    ThreadNetif &netif = GetNetif();
    uint16_t offset = aMessage.GetOffset();
    DelayTimerTlv delayTimer;

    VerifyOrExit(netif.GetMle().IsAttached());

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
    delayTimer.SetDelayTimer(netif.GetLeader().GetDelayTimerMinimal());
    mNetwork.Set(delayTimer);

    // add pending timestamp tlv
    mNetwork.SetTimestamp(aTimestamp);
    DatasetManager::HandleNetworkUpdate();

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

}  // namespace MeshCoP
}  // namespace ot

#endif // OPENTHREAD_FTD
