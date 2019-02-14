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
 *   This file includes definitions for the Thread network interface.
 */

#ifndef THREAD_NETIF_HPP_
#define THREAD_NETIF_HPP_

#include "openthread-core-config.h"

#include "coap/coap.hpp"
#include "coap/coap_secure.hpp"
#include "mac/mac.hpp"

#if OPENTHREAD_ENABLE_BORDER_AGENT
#include "meshcop/border_agent.hpp"
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#include "meshcop/commissioner.hpp"
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

#include "meshcop/dataset_manager.hpp"

#if OPENTHREAD_ENABLE_DTLS
#include "meshcop/dtls.hpp"
#endif // OPENTHREAD_ENABLE_DTLS

#if OPENTHREAD_ENABLE_JOINER
#include "meshcop/joiner.hpp"
#endif // OPENTHREAD_ENABLE_JOINER

#include "meshcop/joiner_router.hpp"
#include "meshcop/leader.hpp"
#include "net/dhcp6.hpp"
#include "net/dhcp6_client.hpp"
#include "net/dhcp6_server.hpp"
#include "net/dns_client.hpp"
#include "net/ip6_filter.hpp"
#include "net/netif.hpp"
#include "net/sntp_client.hpp"
#include "thread/address_resolver.hpp"
#include "thread/announce_begin_server.hpp"
#include "thread/energy_scan_server.hpp"
#include "thread/key_manager.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_diagnostic.hpp"
#include "thread/panid_query_server.hpp"
#include "thread/time_sync_service.hpp"
#include "utils/child_supervision.hpp"

#if OPENTHREAD_ENABLE_JAM_DETECTION
#include "utils/jam_detector.hpp"
#endif // OPENTHREAD_ENABLE_JAM_DETECTION

namespace ot {

/**
 * @addtogroup core-netif
 *
 * @brief
 *   This module includes definitions for the Thread network interface.
 *
 * @{
 */

class ThreadNetif : public Ip6::Netif
{
public:
    /**
     * This constructor initializes the Thread network interface.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit ThreadNetif(Instance &aInstance);

    /**
     * This method enables the Thread network interface.
     *
     */
    void Up(void);

    /**
     * This method disables the Thread network interface.
     *
     */
    void Down(void);

    /**
     * This method indicates whether or not the Thread network interface is enabled.
     *
     * @retval TRUE   If the Thread network interface is enabled.
     * @retval FALSE  If the Thread network interface is not enabled.
     *
     */
    bool IsUp(void) const { return mIsUp; }

    /**
     * This method retrieves the link address.
     *
     * @param[out]  aAddress  A reference to the link address.
     *
     */
    virtual otError GetLinkAddress(Ip6::LinkAddress &aAddress) const;

    /**
     * This method submits a message to the network interface.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE  Successfully submitted the message to the interface.
     *
     */
    virtual otError SendMessage(Message &aMessage) { return mMeshForwarder.SendMessage(aMessage); }

    /**
     * This method performs a route lookup.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  A pointer where the number of prefix match bits for the chosen route is stored.
     *
     * @retval OT_ERROR_NONE      Successfully found a route.
     * @retval OT_ERROR_NO_ROUTE  Could not find a valid route.
     *
     */
    virtual otError RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint8_t *aPrefixMatch);

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    /**
     * This method returns a reference to the network diagnostic object.
     *
     * @returns A reference to the address resolver object.
     *
     */
    NetworkDiagnostic::NetworkDiagnostic &GetNetworkDiagnostic(void) { return mNetworkDiagnostic; }
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    /**
     * This method returns a reference to the dhcp client object.
     *
     * @returns A reference to the dhcp client object.
     *
     */
    Dhcp6::Dhcp6Client &GetDhcp6Client(void) { return mDhcp6Client; }
#endif // OPENTHREAD_ENABLE_DHCP6_CLIENT

#if OPENTHREAD_ENABLE_DHCP6_SERVER
    /**
     * This method returns a reference to the dhcp server object.
     *
     * @returns A reference to the dhcp server object.
     *
     */
    Dhcp6::Dhcp6Server &GetDhcp6Server(void) { return mDhcp6Server; }
#endif // OPENTHREAD_ENABLE_DHCP6_SERVER

#if OPENTHREAD_ENABLE_DNS_CLIENT
    /**
     * This method returns a reference to the dns client object.
     *
     * @returns A reference to the dns client object.
     *
     */
    Dns::Client &GetDnsClient(void) { return mDnsClient; }
#endif // OPENTHREAD_ENABLE_DNS_CLIENT

    /**
     * This method returns a reference to the CoAP object.
     *
     * @returns A reference to the CoAP object.
     *
     */
    Coap::Coap &GetCoap(void) { return mCoap; }

    /**
     * This method returns a reference to the IPv6 filter object.
     *
     * @returns A reference to the IPv6 filter object.
     *
     */
    Ip6::Filter &GetIp6Filter(void) { return mIp6Filter; }

    /**
     * This method returns a reference to the key manager object.
     *
     * @returns A reference to the key manager object.
     *
     */
    KeyManager &GetKeyManager(void) { return mKeyManager; }

    /**
     * This method returns a reference to the lowpan object.
     *
     * @returns A reference to the lowpan object.
     *
     */
    Lowpan::Lowpan &GetLowpan(void) { return mLowpan; }

    /**
     * This method returns a reference to the mac object.
     *
     * @returns A reference to the mac object.
     *
     */
    Mac::Mac &GetMac(void) { return mMac; }

    /**
     * This method returns a reference to the mle object.
     *
     * @returns A reference to the mle object.
     *
     */
    Mle::MleRouter &GetMle(void) { return mMleRouter; }

    /**
     * This method returns a reference to the mesh forwarder object.
     *
     * @returns A reference to the mesh forwarder object.
     *
     */
    MeshForwarder &GetMeshForwarder(void) { return mMeshForwarder; }

#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    /**
     * This method returns a reference to the network data local object.
     *
     * @returns A reference to the network data local object.
     *
     */
    NetworkData::Local &GetNetworkDataLocal(void) { return mNetworkDataLocal; }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE

    /**
     * This method returns a reference to the network data leader object.
     *
     * @returns A reference to the network data leader object.
     *
     */
    NetworkData::Leader &GetNetworkDataLeader(void) { return mNetworkDataLeader; }

    /**
     * This method returns a reference to the active dataset object.
     *
     * @returns A reference to the active dataset object.
     *
     */
    MeshCoP::ActiveDataset &GetActiveDataset(void) { return mActiveDataset; }

    /**
     * This method returns a reference to the pending dataset object.
     *
     * @returns A reference to the pending dataset object.
     *
     */
    MeshCoP::PendingDataset &GetPendingDataset(void) { return mPendingDataset; }

#if OPENTHREAD_FTD
    /**
     * This method returns a reference to the joiner router object.
     *
     * @returns A reference to the joiner router object.
     *
     */
    MeshCoP::JoinerRouter &GetJoinerRouter(void) { return mJoinerRouter; }

    /**
     * This method returns a reference to the MeshCoP leader object.
     *
     * @returns A reference to the MeshCoP leader object.
     *
     */
    MeshCoP::Leader &GetLeader(void) { return mLeader; }

    /**
     * This method returns a reference to the address resolver object.
     *
     * @returns A reference to the address resolver object.
     *
     */
    AddressResolver &GetAddressResolver(void) { return mAddressResolver; }
#endif // OPENTHREAD_FTD

    /**
     * This method returns a reference to the announce begin server object.
     *
     * @returns A reference to the announce begin server object.
     *
     */
    AnnounceBeginServer &GetAnnounceBeginServer(void) { return mAnnounceBegin; }

#if OPENTHREAD_ENABLE_BORDER_AGENT
    /**
     * This method returns a reference to the border agent object.
     *
     * @returns A reference to the border agent object.
     *
     */
    MeshCoP::BorderAgent &GetBorderAgent(void) { return mBorderAgent; }
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    /**
     * This method returns a reference to the commissioner object.
     *
     * @returns A reference to the commissioner object.
     *
     */
    MeshCoP::Commissioner &GetCommissioner(void) { return mCommissioner; }
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_DTLS
    /**
     * This method returns a reference to the Dtls object.
     *
     * @returns A reference to the Dtls object.
     *
     */
    MeshCoP::Dtls &GetDtls(void) { return mDtls; }

    /**
     * This method returns a reference to the secure CoAP object.
     *
     * @returns A reference to the secure CoAP object.
     *
     */
    Coap::CoapSecure &GetCoapSecure(void) { return mCoapSecure; }
#endif // OPENTHREAD_ENABLE_DTLS

#if OPENTHREAD_ENABLE_JOINER
    /**
     * This method returns a reference to the joiner object.
     *
     * @returns A reference to the joiner object.
     *
     */
    MeshCoP::Joiner &GetJoiner(void) { return mJoiner; }
#endif // OPENTHREAD_ENABLE_JOINER

#if OPENTHREAD_ENABLE_JAM_DETECTION
    /**
     * This method returns the jam detector instance.
     *
     * @returns Reference to the JamDetector instance.
     *
     */
    Utils::JamDetector &GetJamDetector(void) { return mJamDetector; }
#endif // OPENTHREAD_ENABLE_JAM_DETECTION

    /**
     * This method returns a reference to the child supervisor object.
     *
     * @returns A reference to the child supervisor object.
     *
     */
    Utils::ChildSupervisor &GetChildSupervisor(void) { return mChildSupervisor; }

    /**
     * This method returns a reference to the supervision listener object.
     *
     * @returns A reference to the supervision listener object.
     *
     */
    Utils::SupervisionListener &GetSupervisionListener(void) { return mSupervisionListener; }

    /**
     * This method returns a reference to the energy scan server object.
     *
     * @returns A reference to the energy scan server object.
     *
     */
    EnergyScanServer &GetEnergyScanServer(void) { return mEnergyScan; }

    /**
     * This method returns a reference to the PAN ID query server object.
     *
     * @returns A reference to the PAN ID query server object.
     *
     */
    PanIdQueryServer &GetPanIdQueryServer(void) { return mPanIdQuery; }

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    /**
     * This method returns a reference to the Time Synchronization Service object.
     *
     * @returns A reference to the Time Synchronization Service object.
     *
     */
    TimeSync &GetTimeSync(void) { return mTimeSync; }
#endif

#if OPENTHREAD_ENABLE_SNTP_CLIENT
    /**
     * This method returns a reference to the SNTP client object.
     *
     * @returns A reference to the SNTP client object.
     *
     */
    Sntp::Client &GetSntpClient(void) { return mSntpClient; }
#endif // OPENTHREAD_ENABLE_SNTP_CLIENT

    /**
     * This method returns whether Thread Management Framework Addressing Rules are met.
     *
     * @retval TRUE   if Thread Management Framework Addressing Rules are met.
     * @retval FALSE  if Thread Management Framework Addressing Rules are not met.
     *
     */
    bool IsTmfMessage(const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_ENABLE_SLAAC
    /**
     * This method updates addresses that shall be automatically created using SLAAC.
     *
     */
    void UpdateSlaac(void);
#endif

private:
    static otError TmfFilter(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);

    Coap::Coap mCoap;
#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    Dhcp6::Dhcp6Client mDhcp6Client;
#endif // OPENTHREAD_ENABLE_DHCP6_CLIENT
#if OPENTHREAD_ENABLE_DHCP6_SERVER
    Dhcp6::Dhcp6Server mDhcp6Server;
#endif // OPENTHREAD_ENABLE_DHCP6_SERVER
#if OPENTHREAD_CONFIG_ENABLE_SLAAC
    Ip6::NetifUnicastAddress mSlaacAddresses[OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES];
#endif
#if OPENTHREAD_ENABLE_DNS_CLIENT
    Dns::Client mDnsClient;
#endif // OPENTHREAD_ENABLE_DNS_CLIENT
#if OPENTHREAD_ENABLE_SNTP_CLIENT
    Sntp::Client mSntpClient;
#endif // OPENTHREAD_ENABLE_SNTP_CLIENT
    MeshCoP::ActiveDataset  mActiveDataset;
    MeshCoP::PendingDataset mPendingDataset;
    Ip6::Filter             mIp6Filter;
    KeyManager              mKeyManager;
    Lowpan::Lowpan          mLowpan;
    Mac::Mac                mMac;
    MeshForwarder           mMeshForwarder;
    Mle::MleRouter          mMleRouter;
#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    NetworkData::Local mNetworkDataLocal;
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    NetworkData::Leader mNetworkDataLeader;
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    NetworkDiagnostic::NetworkDiagnostic mNetworkDiagnostic;
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    bool mIsUp;

#if OPENTHREAD_ENABLE_BORDER_AGENT
    MeshCoP::BorderAgent mBorderAgent;
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    MeshCoP::Commissioner mCommissioner;
#endif // OPENTHREAD_ENABLE_COMMISSIONER

#if OPENTHREAD_ENABLE_DTLS
    MeshCoP::Dtls    mDtls;
    Coap::CoapSecure mCoapSecure;
#endif // OPENTHREAD_ENABLE_DTLS

#if OPENTHREAD_ENABLE_JOINER
    MeshCoP::Joiner mJoiner;
#endif // OPENTHREAD_ENABLE_JOINER

#if OPENTHREAD_ENABLE_JAM_DETECTION
    Utils::JamDetector mJamDetector;
#endif // OPENTHREAD_ENABLE_JAM_DETECTION

#if OPENTHREAD_FTD
    MeshCoP::JoinerRouter mJoinerRouter;
    MeshCoP::Leader       mLeader;
    AddressResolver       mAddressResolver;
#endif // OPENTHREAD_FTD

    Utils::ChildSupervisor     mChildSupervisor;
    Utils::SupervisionListener mSupervisionListener;
    AnnounceBeginServer        mAnnounceBegin;
    PanIdQueryServer           mPanIdQuery;
    EnergyScanServer           mEnergyScan;

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    TimeSync mTimeSync;
#endif
};

/**
 * @}
 */

} // namespace ot

#endif // THREAD_NETIF_HPP_
