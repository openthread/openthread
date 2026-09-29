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
    : mNode(nullptr)
    , mEnabled(false)
{
}

void Mdns::Init(Node &aNode) { mNode = &aNode; }

void Mdns::Reset(void) { mEnabled = false; }

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
    Ip6::Address multicastAddress;

    if (aInfraIfIndex != kInfraIfIndex)
    {
        aMessage.Free();
        ExitNow();
    }

    GetMulticastAddress(multicastAddress);
    mNode->mInfraIf.SendUdp(mNode->mInfraIf.GetLinkLocalAddress(), multicastAddress, kUdpPort, kUdpPort, aMessage);

exit:
    return;
}

void Mdns::SendUnicast(Message &aMessage, const AddressInfo &aAddress)
{
    if (aAddress.mInfraIfIndex != kInfraIfIndex)
    {
        aMessage.Free();
        ExitNow();
    }

    mNode->mInfraIf.SendUdp(mNode->mInfraIf.GetLinkLocalAddress(), AsCoreType(&aAddress.mAddress), kUdpPort,
                            aAddress.mPort, aMessage);

exit:
    return;
}

void Mdns::SignalIfAddresses(Instance &aInstance)
{
    otPlatMdnsHandleHostAddressRemoveAll(&aInstance, kInfraIfIndex);

    for (const Ip6::Address &address : mNode->mInfraIf.GetAddresses())
    {
        otPlatMdnsHandleHostAddressEvent(&aInstance, &address, /* aAdded */ true, kInfraIfIndex);
    }
}

void Mdns::Receive(Instance &aInstance, Message &aMessage, bool aIsUnicast, const AddressInfo &aSenderAddress)
{
    Message *message;

    VerifyOrExit(mEnabled);

    if (aIsUnicast)
    {
        VerifyOrExit(aSenderAddress.mInfraIfIndex == kInfraIfIndex);
        VerifyOrExit(aSenderAddress.mPort == kUdpPort);
    }

    message = aMessage.Clone<kNoReservedHeader>();
    VerifyOrQuit(message != nullptr);

    otPlatMdnsHandleReceive(&aInstance, message, aIsUnicast, &aSenderAddress);

exit:
    return;
}

void Mdns::GetAddress(AddressInfo &aAddress) const
{
    ClearAllBytes(aAddress);
    aAddress.mAddress      = mNode->mInfraIf.GetLinkLocalAddress();
    aAddress.mPort         = kUdpPort;
    aAddress.mInfraIfIndex = kInfraIfIndex;
}

void Mdns::HandleHostAddressEvent(const Ip6::Address &aAddress, bool aAdded)
{
    if (mEnabled)
    {
        otPlatMdnsHandleHostAddressEvent(&mNode->GetInstance(), &aAddress, aAdded, kInfraIfIndex);
    }
}

void Mdns::HandleHostAddressRemoveAll(void)
{
    if (mEnabled)
    {
        otPlatMdnsHandleHostAddressRemoveAll(&mNode->GetInstance(), kInfraIfIndex);
    }
}

void Mdns::GetMulticastAddress(Ip6::Address &aAddress) { SuccessOrQuit(aAddress.FromString("ff02::fb")); }

} // namespace Nexus
} // namespace ot
