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
 *   This file includes definitions for DHCPv6 Client.
 */

#ifndef DHCP6_CLIENT_HPP_
#define DHCP6_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "common/trickle_timer.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "net/dhcp6_types.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"

namespace ot {

namespace Dhcp6 {

/**
 * @addtogroup core-dhcp6
 *
 * @brief
 *   This module includes definitions for DHCPv6 Client.
 *
 * @{
 */

/**
 * Implements DHCPv6 Client.
 */
class Client : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Client(Instance &aInstance);

private:
    static constexpr uint16_t kNumPrefixes      = OPENTHREAD_CONFIG_DHCP6_CLIENT_NUM_PREFIXES;
    static constexpr uint32_t kTrickleTimerImin = 1;
    static constexpr uint32_t kTrickleTimerImax = 120;

    enum IaStatus : uint8_t
    {
        kIaStatusInvalid,
        kIaStatusSolicit,
        kIaStatusSoliciting,
        kIaStatusSolicitReplied,
    };

    struct IdentityAssociation
    {
        Ip6::Netif::UnicastAddress mNetifAddress;
        uint32_t                   mPreferredLifetime;
        uint32_t                   mValidLifetime;
        uint16_t                   mPrefixAgentRloc;
        IaStatus                   mStatus;
    };

    void Start(void);
    void Stop(void);

    void Solicit(uint16_t aRloc16);

    bool ProcessNextIdentityAssociation(void);

    Error AppendHeader(Message &aMessage);
    Error AppendClientIdOption(Message &aMessage);
    Error AppendIaNaOption(Message &aMessage, uint16_t aRloc16);
    Error AppendElapsedTimeOption(Message &aMessage);
    Error AppendRapidCommitOption(Message &aMessage) { return RapidCommitOption::AppendTo(aMessage); }

    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void  ProcessReply(const Message &aMessage);
    Error ProcessServerIdOption(const Message &aMessage);
    Error ProcessClientIdOption(const Message &aMessage);
    Error ProcessIaNaOption(const Message &aMessage);
    Error ProcessIaAddressOption(const IaAddressOption &aOption);

    void HandleNotifierEvents(Events aEvents);
    void UpdateAddresses(void);

    static void HandleTrickleTimer(TrickleTimer &aTrickleTimer);
    void        HandleTrickleTimer(void);

    using ClientSocket = Ip6::Udp::SocketIn<Client, &Client::HandleUdpReceive>;

    ClientSocket         mSocket;
    TrickleTimer         mTrickleTimer;
    TransactionId        mTransactionId;
    TimeMilli            mStartTime;
    IdentityAssociation  mIdentityAssociations[kNumPrefixes];
    IdentityAssociation *mIdentityAssociationCurrent;
};

/**
 * @}
 */

} // namespace Dhcp6
} // namespace ot

#else // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#if OPENTHREAD_ENABLE_DHCP6_MULTICAST_SOLICIT
#error "OPENTHREAD_ENABLE_DHCP6_MULTICAST_SOLICIT requires OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE to be also set."
#endif

#endif // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#endif // DHCP6_CLIENT_HPP_
