/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include <openthread/platform/mdns_socket.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

//---------------------------------------------------------------------------------------------------------------------
// otPlatMdns APIs

extern "C" {

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    return AsNode(aInstance).mMdns.SetListeningEnabled(AsCoreType(aInstance), aEnable, aInfraIfIndex);
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    AsNode(aInstance).mMdns.SendMulticast(AsCoreType(aMessage), aInfraIfIndex);
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    AsNode(aInstance).mMdns.SendUnicast(AsCoreType(aMessage), *aAddress);
}

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Mdns

Mdns::Mdns(void)
    : mEnabled(false)
{
    Ip6::Address             address;
    Ip6::InterfaceIdentifier iid;

    iid.GenerateRandom();
    address.SetToLinkLocalAddress(iid);

    SuccessOrQuit(mIfAddresses.PushBack(address));
}

Error Mdns::SetListeningEnabled(Instance &aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    Error error = kErrorNone;

    VerifyOrExit(aInfraIfIndex == kInfraIfIndex, error = kErrorFailed);
    VerifyOrExit(mEnabled != aEnable);
    mEnabled = aEnable;

    if (mEnabled)
    {
        SignalIfAddresses(aInstance);
    }

exit:
    return error;
}

void Mdns::SendMulticast(Message &aMessage, uint32_t aInfraIfIndex)
{
    PendingTx *pendingTx;

    if (aInfraIfIndex != kInfraIfIndex)
    {
        aMessage.Free();
        ExitNow();
    }

    pendingTx = PendingTx::Allocate();
    VerifyOrQuit(pendingTx != nullptr);

    pendingTx->mMessage.Reset(&aMessage);
    pendingTx->mIsUnicast = false;

    mPendingTxList.PushAfterTail(*pendingTx);

exit:
    return;
}

void Mdns::SendUnicast(Message &aMessage, const AddressInfo &aAddress)
{
    PendingTx *pendingTx;

    if (aAddress.mInfraIfIndex != kInfraIfIndex)
    {
        aMessage.Free();
        ExitNow();
    }

    pendingTx = PendingTx::Allocate();
    VerifyOrQuit(pendingTx != nullptr);

    pendingTx->mMessage.Reset(&aMessage);
    pendingTx->mIsUnicast = true;
    pendingTx->mAddress   = aAddress;

    mPendingTxList.PushAfterTail(*pendingTx);

exit:
    return;
}

void Mdns::SignalIfAddresses(Instance &aInstance)
{
    otPlatMdnsHandleHostAddressRemoveAll(&aInstance, kInfraIfIndex);

    for (const Ip6::Address &address : mIfAddresses)
    {
        otPlatMdnsHandleHostAddressEvent(&aInstance, &address, /* aAdded */ true, kInfraIfIndex);
    }
}

void Mdns::Receive(Instance &aInstance, const PendingTx &aPendingTx, const AddressInfo &aSenderAddress)
{
    Message *message;

    VerifyOrExit(mEnabled);

    if (aPendingTx.mIsUnicast)
    {
        VerifyOrExit(aPendingTx.mAddress.mInfraIfIndex == kInfraIfIndex);
        VerifyOrExit(aPendingTx.mAddress.mPort == kUdpPort);
        VerifyOrExit(mIfAddresses.Contains(AsCoreType(&aPendingTx.mAddress.mAddress)));
    }

    message = aPendingTx.mMessage->Clone();
    VerifyOrQuit(message != nullptr);

    otPlatMdnsHandleReceive(&aInstance, message, aPendingTx.mIsUnicast, &aSenderAddress);

exit:
    return;
}

void Mdns::GetAddress(AddressInfo &aAddress) const
{
    ClearAllBytes(aAddress);
    aAddress.mAddress      = mIfAddresses[0];
    aAddress.mPort         = kUdpPort;
    aAddress.mInfraIfIndex = kInfraIfIndex;
}

} // namespace Nexus
} // namespace ot
