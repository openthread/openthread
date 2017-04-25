
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

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <openthread-instance.h>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

static const uint8_t kThreadMasterKey[] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

ThreadNetif::ThreadNetif(Ip6::Ip6 &aIp6):
    Netif(aIp6, OT_NETIF_INTERFACE_ID_THREAD),
    mCoapServer(*this, kCoapUdpPort),
    mCoapClient(*this),
    mAddressResolver(*this),
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
    mNetworkDataLocal(*this),
    mNetworkDataLeader(*this),
    mNetworkDiagnostic(*this),
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    mSecureCoapServer(*this, OPENTHREAD_CONFIG_JOINER_UDP_PORT),
    mCommissioner(*this),
#endif  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_DTLS
    mDtls(*this),
#endif
#if OPENTHREAD_ENABLE_JOINER
    mSecureCoapClient(*this),
    mJoiner(*this),
#endif  // OPENTHREAD_ENABLE_JOINER
#if OPENTHREAD_ENABLE_JAM_DETECTION
    mJamDetector(*this),
#endif // OPENTHREAD_ENABLE_JAM_DETECTTION
    mJoinerRouter(*this),
    mLeader(*this),
    mAnnounceBegin(*this),
    mPanIdQuery(*this),
    mEnergyScan(*this)

{
    mKeyManager.SetMasterKey(kThreadMasterKey, sizeof(kThreadMasterKey));
    mCoapServer.SetInterceptor(&ThreadNetif::TmfFilter);
}

ThreadError ThreadNetif::Up(void)
{
    if (!mIsUp)
    {
        mIp6.AddNetif(*this);
        mMeshForwarder.Start();
        mCoapServer.Start();
        mCoapClient.Start();
#if OPENTHREAD_ENABLE_JOINER
        mSecureCoapClient.Start();
#endif
#if OPENTHREAD_ENABLE_DNS_CLIENT
        mDnsClient.Start();
#endif
        mMleRouter.Enable();
        mIsUp = true;
    }

    return kThreadError_None;
}

ThreadError ThreadNetif::Down(void)
{
    mCoapServer.Stop();
    mCoapClient.Stop();
#if OPENTHREAD_ENABLE_JOINER
    mSecureCoapClient.Stop();
#endif
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mDnsClient.Stop();
#endif
    mMleRouter.Disable();
    mMeshForwarder.Stop();
    mIp6.RemoveNetif(*this);
    RemoveAllExternalUnicastAddresses();
    UnsubscribeAllExternalMulticastAddresses();
    mIsUp = false;

#if OPENTHREAD_ENABLE_DTLS
    mDtls.Stop();
#endif

    return kThreadError_None;
}

ThreadError ThreadNetif::GetLinkAddress(Ip6::LinkAddress &address) const
{
    address.mType = Ip6::LinkAddress::kEui64;
    address.mLength = sizeof(address.mExtAddress);
    memcpy(&address.mExtAddress, mMac.GetExtAddress(), address.mLength);
    return kThreadError_None;
}

ThreadError ThreadNetif::RouteLookup(const Ip6::Address &source, const Ip6::Address &destination, uint8_t *prefixMatch)
{
    ThreadError error;
    uint16_t rloc;

    SuccessOrExit(error = mNetworkDataLeader.RouteLookup(source, destination, prefixMatch, &rloc));

    if (rloc == mMleRouter.GetRloc16())
    {
        error = kThreadError_NoRoute;
    }

exit:
    return error;
}

ThreadError ThreadNetif::TmfFilter(const otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    const Ip6::MessageInfo &messageInfo = *static_cast<const Ip6::MessageInfo *>(aMessageInfo);
    ThreadError error = kThreadError_None;

    VerifyOrExit(messageInfo.GetSockAddr().IsRoutingLocator() ||
                 messageInfo.GetPeerAddr().IsRoutingLocator() ||
                 messageInfo.GetSockAddr().IsLinkLocal() ||
                 messageInfo.GetSockAddr().IsAnycastRoutingLocator() ||
                 messageInfo.GetPeerAddr().IsAnycastRoutingLocator(),
                 error = kThreadError_Security);

exit:
    (void)aMessage;
    return error;
}

otInstance *ThreadNetif::GetInstance(void)
{
    return otInstanceFromThreadNetif(this);
}

}  // namespace Thread
