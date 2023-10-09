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
 *   This file includes definitions for Backbone Router management.
 */

#ifndef BACKBONE_ROUTER_MANAGER_HPP_
#define BACKBONE_ROUTER_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include <openthread/backbone_router.h>
#include <openthread/backbone_router_ftd.h>

#include "backbone_router/backbone_tmf.hpp"
#include "backbone_router/bbr_leader.hpp"
#include "backbone_router/multicast_listeners_table.hpp"
#include "backbone_router/ndproxy_table.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "net/netif.hpp"
#include "thread/network_data.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace BackboneRouter {

/**
 * Implements the definitions for Backbone Router management.
 *
 */
class Manager : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class Tmf::Agent;
    friend class BackboneTmfAgent;

public:
    /**
     * Initializes the Backbone Router manager.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Manager(Instance &aInstance);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    /**
     * Returns the NdProxy Table.
     *
     * @returns The NdProxy Table.
     *
     */
    NdProxyTable &GetNdProxyTable(void);
#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Configures response status for next DUA registration.
     *
     * Note: available only when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     *       Only used for test and certification.
     *
     * @param[in] aMlIid    A pointer to the Mesh Local IID. If `nullptr`, respond with @p aStatus for any
     *                      coming DUA.req, otherwise only respond the one with matching @p aMlIid.
     * @param[in] aStatus   The status to respond.
     *
     */
    void ConfigNextDuaRegistrationResponse(const Ip6::InterfaceIdentifier *aMlIid, uint8_t aStatus);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    /**
     * Configures response status for next Multicast Listener Registration.
     *
     * Note: available only when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     *       Only used for test and certification.
     *
     * @param[in] aStatus  The status to respond.
     *
     */
    void ConfigNextMulticastListenerRegistrationResponse(ThreadStatusTlv::MlrStatus aStatus);
#endif
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    /**
     * Gets the Multicast Listeners Table.
     *
     * @returns The Multicast Listeners Table.
     *
     */
    MulticastListenersTable &GetMulticastListenersTable(void) { return mMulticastListenersTable; }
#endif

    /**
     * Returns if messages destined to a given Domain Unicast Address should be forwarded to the Backbone
     * link.
     *
     * @param aAddress The Domain Unicast Address.
     *
     * @retval TRUE   If messages destined to the Domain Unicast Address should be forwarded to the Backbone link.
     * @retval FALSE  If messages destined to the Domain Unicast Address should not be forwarded to the Backbone link.
     *
     */
    bool ShouldForwardDuaToBackbone(const Ip6::Address &aAddress);

    /**
     * Returns a reference to the Backbone TMF agent.
     *
     * @returns A reference to the Backbone TMF agent.
     *
     */
    BackboneTmfAgent &GetBackboneTmfAgent(void) { return mBackboneTmfAgent; }

    /**
     * Sends BB.qry on the Backbone link.
     *
     * @param[in]  aDua     The Domain Unicast Address to query.
     * @param[in]  aRloc16  The short address of the address resolution initiator or `Mac::kShortAddrInvalid` for
     *                      DUA DAD.
     *
     * @retval kErrorNone          Successfully sent BB.qry on backbone link.
     * @retval kErrorInvalidState  If the Backbone Router is not primary, or not enabled.
     * @retval kErrorNoBufs        If insufficient message buffers available.
     *
     */
    Error SendBackboneQuery(const Ip6::Address &aDua, uint16_t aRloc16 = Mac::kShortAddrInvalid);

    /**
     * Send a Proactive Backbone Notification (PRO_BB.ntf) on the Backbone link.
     *
     * @param[in] aDua                          The Domain Unicast Address to notify.
     * @param[in] aMeshLocalIid                 The Mesh-Local IID to notify.
     * @param[in] aTimeSinceLastTransaction     Time since last transaction (in seconds).
     *
     * @retval kErrorNone          Successfully sent PRO_BB.ntf on backbone link.
     * @retval kErrorNoBufs        If insufficient message buffers available.
     *
     */
    Error SendProactiveBackboneNotification(const Ip6::Address             &aDua,
                                            const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                            uint32_t                        aTimeSinceLastTransaction);

private:
    static constexpr uint8_t  kDefaultHoplimit = 1;
    static constexpr uint32_t kTimerInterval   = 1000;

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    void HandleMulticastListenerRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendMulticastListenerRegistrationResponse(const Coap::Message       &aMessage,
                                                   const Ip6::MessageInfo    &aMessageInfo,
                                                   ThreadStatusTlv::MlrStatus aStatus,
                                                   Ip6::Address              *aFailedAddresses,
                                                   uint8_t                    aFailedAddressNum);
    void SendBackboneMulticastListenerRegistration(const Ip6::Address *aAddresses,
                                                   uint8_t             aAddressNum,
                                                   uint32_t            aTimeout);
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    void  HandleDuaRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error SendBackboneAnswer(const Ip6::MessageInfo      &aQueryMessageInfo,
                             const Ip6::Address          &aDua,
                             uint16_t                     aSrcRloc16,
                             const NdProxyTable::NdProxy &aNdProxy);
    Error SendBackboneAnswer(const Ip6::Address             &aDstAddr,
                             const Ip6::Address             &aDua,
                             const Ip6::InterfaceIdentifier &aMeshLocalIid,
                             uint32_t                        aTimeSinceLastTransaction,
                             uint16_t                        aSrcRloc16);
    void  HandleDadBackboneAnswer(const Ip6::Address &aDua, const Ip6::InterfaceIdentifier &aMeshLocalIid);
    void  HandleExtendedBackboneAnswer(const Ip6::Address             &aDua,
                                       const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                       uint32_t                        aTimeSinceLastTransaction,
                                       uint16_t                        aSrcRloc16);
    void  HandleProactiveBackboneNotification(const Ip6::Address             &aDua,
                                              const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                              uint32_t                        aTimeSinceLastTransaction);
    void  SendDuaRegistrationResponse(const Coap::Message       &aMessage,
                                      const Ip6::MessageInfo    &aMessageInfo,
                                      const Ip6::Address        &aTarget,
                                      ThreadStatusTlv::DuaStatus aStatus);
#endif
    void HandleNotifierEvents(Events aEvents);

    void HandleTimer(void);

    void LogError(const char *aText, Error aError) const;

    using BbrTimer = TimerMilliIn<Manager, &Manager::HandleTimer>;

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    NdProxyTable mNdProxyTable;
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    MulticastListenersTable mMulticastListenersTable;
#endif
    BbrTimer mTimer;

    BackboneTmfAgent mBackboneTmfAgent;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    Ip6::InterfaceIdentifier mDuaResponseTargetMlIid;
    uint8_t                  mDuaResponseStatus;
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    ThreadStatusTlv::MlrStatus mMlrResponseStatus;
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    bool mDuaResponseIsSpecified : 1;
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    bool mMlrResponseIsSpecified : 1;
#endif
#endif
};

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
DeclareTmfHandler(Manager, kUriMlr);
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
DeclareTmfHandler(Manager, kUriDuaRegistrationRequest);
DeclareTmfHandler(Manager, kUriBackboneQuery);
DeclareTmfHandler(Manager, kUriBackboneAnswer);
#endif

} // namespace BackboneRouter

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // BACKBONE_ROUTER_MANAGER_HPP_
