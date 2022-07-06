/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements Thread Management Framework (TMF) functionalities.
 */

#include "thread/tmf.hpp"

#include "common/locator_getters.hpp"

namespace ot {
namespace Tmf {

//----------------------------------------------------------------------------------------------------------------------
// MessageInfo

void MessageInfo::SetSockAddrToRloc(void)
{
    SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
}

Error MessageInfo::SetSockAddrToRlocPeerAddrToLeaderAloc(void)
{
    SetSockAddrToRloc();
    return Get<Mle::MleRouter>().GetLeaderAloc(GetPeerAddr());
}

Error MessageInfo::SetSockAddrToRlocPeerAddrToLeaderRloc(void)
{
    SetSockAddrToRloc();
    return Get<Mle::MleRouter>().GetLeaderAddress(GetPeerAddr());
}

void MessageInfo::SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast(void)
{
    SetSockAddrToRloc();
    GetPeerAddr().SetToRealmLocalAllRoutersMulticast();
}

void MessageInfo::SetSockAddrToRlocPeerAddrTo(uint16_t aRloc16)
{
    SetSockAddrToRloc();
    SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    GetPeerAddr().GetIid().SetLocator(aRloc16);
}

void MessageInfo::SetSockAddrToRlocPeerAddrTo(const Ip6::Address &aPeerAddress)
{
    SetSockAddrToRloc();
    SetPeerAddr(aPeerAddress);
}

//----------------------------------------------------------------------------------------------------------------------
// Agent

Error Agent::Start(void)
{
    return Coap::Start(kUdpPort, OT_NETIF_THREAD);
}

Error Agent::Filter(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext)
{
    OT_UNUSED_VARIABLE(aMessage);

    return static_cast<Agent *>(aContext)->IsTmfMessage(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(),
                                                        aMessageInfo.GetSockPort())
               ? kErrorNone
               : kErrorNotTmf;
}

bool Agent::IsTmfMessage(const Ip6::Address &aSourceAddress, const Ip6::Address &aDestAddress, uint16_t aDestPort) const
{
    bool isTmf = false;

    VerifyOrExit(aDestPort == kUdpPort);

    if (aSourceAddress.IsLinkLocal())
    {
        isTmf = aDestAddress.IsLinkLocal() || aDestAddress.IsLinkLocalMulticast();
        ExitNow();
    }

    VerifyOrExit(Get<Mle::Mle>().IsMeshLocalAddress(aSourceAddress));
    VerifyOrExit(Get<Mle::Mle>().IsMeshLocalAddress(aDestAddress) || aDestAddress.IsLinkLocalMulticast() ||
                 aDestAddress.IsRealmLocalMulticast());

    isTmf = true;

exit:
    return isTmf;
}

} // namespace Tmf
} // namespace ot
