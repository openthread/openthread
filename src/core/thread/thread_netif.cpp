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

#include "thread_netif.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"
#if OPENTHREAD_CONFIG_ENABLE_SLAAC
#include "utils/slaac_address.hpp"
#endif

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

ThreadNetif::ThreadNetif(Instance &aInstance)
    : Netif(aInstance, OT_NETIF_INTERFACE_ID_THREAD)
    , mCoap(aInstance)
#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    , mDhcp6Client(aInstance)
#endif // OPENTHREAD_ENABLE_DHCP6_CLIENT
#if OPENTHREAD_ENABLE_DHCP6_SERVER
    , mDhcp6Server(aInstance)
#endif // OPENTHREAD_ENABLE_DHCP6_SERVER
#if OPENTHREAD_ENABLE_DNS_CLIENT
    , mDnsClient(aInstance.GetThreadNetif())
#endif // OPENTHREAD_ENABLE_DNS_CLIENT
#if OPENTHREAD_ENABLE_SNTP_CLIENT
    , mSntpClient(aInstance.GetThreadNetif())
#endif // OPENTHREAD_ENABLE_SNTP_CLIENT
    , mActiveDataset(aInstance)
    , mPendingDataset(aInstance)
    , mKeyManager(aInstance)
    , mLowpan(aInstance)
    , mMac(aInstance)
    , mMeshForwarder(aInstance)
    , mMleRouter(aInstance)
#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    , mNetworkDataLocal(aInstance)
#endif
    , mNetworkDataLeader(aInstance)
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    , mNetworkDiagnostic(aInstance)
#endif
    , mIsUp(false)
#if OPENTHREAD_ENABLE_BORDER_AGENT
    , mBorderAgent(aInstance)
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    , mCommissioner(aInstance)
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_DTLS
    , mDtls(aInstance)
    , mCoapSecure(aInstance)
#endif
#if OPENTHREAD_ENABLE_JOINER
    , mJoiner(aInstance)
#endif // OPENTHREAD_ENABLE_JOINER
#if OPENTHREAD_ENABLE_JAM_DETECTION
    , mJamDetector(aInstance)
#endif // OPENTHREAD_ENABLE_JAM_DETECTION
#if OPENTHREAD_FTD
    , mJoinerRouter(aInstance)
    , mLeader(aInstance)
    , mAddressResolver(aInstance)
#endif // OPENTHREAD_FTD
    , mChildSupervisor(aInstance)
    , mSupervisionListener(aInstance)
    , mAnnounceBegin(aInstance)
    , mPanIdQuery(aInstance)
    , mEnergyScan(aInstance)
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    , mTimeSync(aInstance)
#endif
{
    mCoap.SetInterceptor(&ThreadNetif::TmfFilter, this);
#if OPENTHREAD_CONFIG_ENABLE_SLAAC
    memset(mSlaacAddresses, 0, sizeof(mSlaacAddresses));
#endif
}

void ThreadNetif::Up(void)
{
    VerifyOrExit(!mIsUp);

    // Enable the MAC just in case it was disabled while the Interface was down.
    mMac.SetEnabled(true);
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    GetInstance().GetChannelMonitor().Start();
#endif
    mMeshForwarder.Start();
    GetIp6().AddNetif(*this);

    mIsUp = true;

    SubscribeAllNodesMulticast();
    mMleRouter.Enable();
    mCoap.Start(kCoapUdpPort);
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mDnsClient.Start();
#endif
#if OPENTHREAD_ENABLE_SNTP_CLIENT
    mSntpClient.Start();
#endif
    GetNotifier().Signal(OT_CHANGED_THREAD_NETIF_STATE);

exit:
    return;
}

void ThreadNetif::Down(void)
{
    VerifyOrExit(mIsUp);

#if OPENTHREAD_ENABLE_DTLS
    mDtls.Stop();
#endif
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mDnsClient.Stop();
#endif
#if OPENTHREAD_ENABLE_SNTP_CLIENT
    mSntpClient.Stop();
#endif
    mCoap.Stop();
    mMleRouter.Disable();
    RemoveAllExternalUnicastAddresses();
    UnsubscribeAllExternalMulticastAddresses();
    UnsubscribeAllRoutersMulticast();
    UnsubscribeAllNodesMulticast();

    mIsUp = false;
    GetIp6().RemoveNetif(*this);
    mMeshForwarder.Stop();
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    GetInstance().GetChannelMonitor().Stop();
#endif
    GetNotifier().Signal(OT_CHANGED_THREAD_NETIF_STATE);

exit:
    return;
}

otError ThreadNetif::GetLinkAddress(Ip6::LinkAddress &address) const
{
    address.mType       = Ip6::LinkAddress::kEui64;
    address.mLength     = sizeof(address.mExtAddress);
    address.mExtAddress = mMac.GetExtAddress();
    return OT_ERROR_NONE;
}

otError ThreadNetif::RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint8_t *aPrefixMatch)
{
    otError  error;
    uint16_t rloc;

    SuccessOrExit(error = mNetworkDataLeader.RouteLookup(aSource, aDestination, aPrefixMatch, &rloc));

    if (rloc == mMleRouter.GetRloc16())
    {
        error = OT_ERROR_NO_ROUTE;
    }

exit:
    return error;
}

otError ThreadNetif::TmfFilter(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext)
{
    OT_UNUSED_VARIABLE(aMessage);

    return static_cast<ThreadNetif *>(aContext)->IsTmfMessage(aMessageInfo) ? OT_ERROR_NONE : OT_ERROR_NOT_TMF;
}

bool ThreadNetif::IsTmfMessage(const Ip6::MessageInfo &aMessageInfo)
{
    bool rval = true;

    // A TMF message must comply with following rules:
    // 1. The destination is a Mesh Local Address or a Link-Local Multicast Address or a Realm-Local Multicast Address,
    //    and the source is a Mesh Local Address. Or
    // 2. Both the destination and the source are Link-Local Addresses.
    VerifyOrExit(
        ((mMleRouter.IsMeshLocalAddress(aMessageInfo.GetSockAddr()) ||
          aMessageInfo.GetSockAddr().IsLinkLocalMulticast() || aMessageInfo.GetSockAddr().IsRealmLocalMulticast()) &&
         mMleRouter.IsMeshLocalAddress(aMessageInfo.GetPeerAddr())) ||
            ((aMessageInfo.GetSockAddr().IsLinkLocal() || aMessageInfo.GetSockAddr().IsLinkLocalMulticast()) &&
             aMessageInfo.GetPeerAddr().IsLinkLocal()),
        rval = false);
exit:
    return rval;
}

#if OPENTHREAD_CONFIG_ENABLE_SLAAC
void ThreadNetif::UpdateSlaac(void)
{
    Utils::Slaac::UpdateAddresses(&GetInstance(), mSlaacAddresses, OT_ARRAY_LENGTH(mSlaacAddresses),
                                  otIp6CreateRandomIid, NULL);
}
#endif

} // namespace ot
