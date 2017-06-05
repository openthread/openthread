
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
 *   This file implements the Thread network interface.
 */

#include <openthread/openthread_enable_defines.h>

#include "thread_netif.hpp"

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

static const otMasterKey kThreadMasterKey =
{
    {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    }
};

ThreadNetif::ThreadNetif(Ip6::Ip6 &aIp6):
    Netif(aIp6, OT_NETIF_INTERFACE_ID_THREAD),
    mCoap(*this),
#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    mDhcp6Client(*this),
#endif  // OPENTHREAD_ENABLE_DHCP6_CLIENT
#if OPENTHREAD_ENABLE_DHCP6_SERVER
    mDhcp6Server(*this),
#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mDnsClient(*this),
#endif  // OPENTHREAD_ENABLE_DNS_CLIENT
    mActiveDataset(*this),
    mPendingDataset(*this),
    mKeyManager(*this),
    mLowpan(*this),
    mMac(*this),
    mMeshForwarder(*this),
    mMleRouter(*this),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    mNetworkDataLocal(*this),
#endif
    mNetworkDataLeader(*this),
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    mNetworkDiagnostic(*this),
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    mCommissioner(*this),
#endif  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_DTLS
    mDtls(*this),
    mCoapSecure(*this),
#endif
#if OPENTHREAD_ENABLE_JOINER
    mJoiner(*this),
#endif  // OPENTHREAD_ENABLE_JOINER
#if OPENTHREAD_ENABLE_JAM_DETECTION
    mJamDetector(*this),
#endif // OPENTHREAD_ENABLE_JAM_DETECTTION
#if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_BORDER_AGENT_PROXY
    mBorderAgentProxy(mMleRouter.GetMeshLocal16(), mCoap),
#endif // OPENTHREAD_ENABLE_BORDER_AGENT_PROXY
    mJoinerRouter(*this),
    mLeader(*this),
    mAddressResolver(*this),
#endif  // OPENTHREAD_FTD
    mChildSupervisor(*this),
    mSupervisionListener(*this),
    mAnnounceBegin(*this),
    mPanIdQuery(*this),
    mEnergyScan(*this)

{
    mKeyManager.SetMasterKey(kThreadMasterKey);
    mCoap.SetInterceptor(&ThreadNetif::TmfFilter, this);
}

otError ThreadNetif::Up(void)
{
    if (!mIsUp)
    {
        mIp6.AddNetif(*this);
        mMeshForwarder.Start();
        mCoap.Start(kCoapUdpPort);
#if OPENTHREAD_ENABLE_DNS_CLIENT
        mDnsClient.Start();
#endif
        mChildSupervisor.Start();
        mMleRouter.Enable();
        mIsUp = true;
    }

    return OT_ERROR_NONE;
}

otError ThreadNetif::Down(void)
{
    mCoap.Stop();
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mDnsClient.Stop();
#endif
    mChildSupervisor.Stop();
    mMleRouter.Disable();
    mMeshForwarder.Stop();
    mIp6.RemoveNetif(*this);
    RemoveAllExternalUnicastAddresses();
    UnsubscribeAllExternalMulticastAddresses();
    mIsUp = false;

#if OPENTHREAD_ENABLE_DTLS
    mDtls.Stop();
#endif

    return OT_ERROR_NONE;
}

otError ThreadNetif::GetLinkAddress(Ip6::LinkAddress &address) const
{
    address.mType = Ip6::LinkAddress::kEui64;
    address.mLength = sizeof(address.mExtAddress);
    memcpy(&address.mExtAddress, mMac.GetExtAddress(), address.mLength);
    return OT_ERROR_NONE;
}

otError ThreadNetif::RouteLookup(const Ip6::Address &source, const Ip6::Address &destination, uint8_t *prefixMatch)
{
    otError error;
    uint16_t rloc;

    SuccessOrExit(error = mNetworkDataLeader.RouteLookup(source, destination, prefixMatch, &rloc));

    if (rloc == mMleRouter.GetRloc16())
    {
        error = OT_ERROR_NO_ROUTE;
    }

exit:
    return error;
}

otError ThreadNetif::TmfFilter(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext)
{
    otError error = OT_ERROR_NONE;

    // A TMF message must comply with following rules:
    // 1. The destination is a Mesh Local Address or a Link-Local Multicast Address or a Realm-Local Multicast Address,
    //    and the source is a Mesh Local Address.
    // 2. Both the destination and the source are Link-Local Addresses.
    VerifyOrExit(((static_cast<ThreadNetif *>(aContext)->mMleRouter.IsMeshLocalAddress(aMessageInfo.GetSockAddr()) ||
                   aMessageInfo.GetSockAddr().IsLinkLocalMulticast() ||
                   aMessageInfo.GetSockAddr().IsRealmLocalMulticast()) &&
                  static_cast<ThreadNetif *>(aContext)->mMleRouter.IsMeshLocalAddress(aMessageInfo.GetPeerAddr())) ||
                 (aMessageInfo.GetSockAddr().IsLinkLocal() && aMessageInfo.GetPeerAddr().IsLinkLocal()),
                 error = OT_ERROR_NOT_TMF);
exit:
    (void)aMessage;
    return error;
}

otInstance *ThreadNetif::GetInstance(void)
{
    return otInstanceFromThreadNetif(this);
}

}  // namespace ot
