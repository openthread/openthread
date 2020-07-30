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

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "common/trickle_timer.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "net/dhcp6.hpp"
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
 *
 */

/**
 * This class implements DHCPv6 Client.
 *
 */
class Client : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Client(Instance &aInstance);

    /**
     * This method update addresses that shall be automatically created using DHCP.
     *
     *
     */
    void UpdateAddresses(void);

private:
    enum
    {
        kTrickleTimerImin = 1,
        kTrickleTimerImax = 120,
    };

    enum IaStatus : uint8_t
    {
        kIaStatusInvalid,
        kIaStatusSolicit,
        kIaStatusSoliciting,
        kIaStatusSolicitReplied,
    };

    struct IdentityAssociation
    {
        Ip6::NetifUnicastAddress mNetifAddress;
        uint32_t                 mPreferredLifetime;
        uint32_t                 mValidLifetime;
        uint16_t                 mPrefixAgentRloc;
        IaStatus                 mStatus;
    };

    void Start(void);
    void Stop(void);

    static bool MatchNetifAddressWithPrefix(const Ip6::NetifUnicastAddress &aAddress, const otIp6Prefix &aIp6Prefix);

    void Solicit(uint16_t aRloc16);

    void AddIdentityAssociation(uint16_t aRloc16, otIp6Prefix &aIp6Prefix);
    void RemoveIdentityAssociation(uint16_t aRloc16, otIp6Prefix &aIp6Prefix);

    bool ProcessNextIdentityAssociation(void);

    otError AppendHeader(Message &aMessage);
    otError AppendClientIdentifier(Message &aMessage);
    otError AppendIaNa(Message &aMessage, uint16_t aRloc16);
    otError AppendIaAddress(Message &aMessage, uint16_t aRloc16);
    otError AppendElapsedTime(Message &aMessage);
    otError AppendRapidCommit(Message &aMessage);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void     ProcessReply(Message &aMessage);
    uint16_t FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode);
    otError  ProcessServerIdentifier(Message &aMessage, uint16_t aOffset);
    otError  ProcessClientIdentifier(Message &aMessage, uint16_t aOffset);
    otError  ProcessIaNa(Message &aMessage, uint16_t aOffset);
    otError  ProcessStatusCode(Message &aMessage, uint16_t aOffset);
    otError  ProcessIaAddress(Message &aMessage, uint16_t aOffset);

    static bool HandleTrickleTimer(TrickleTimer &aTrickleTimer);
    bool        HandleTrickleTimer(void);

    Ip6::Udp::Socket mSocket;

    TrickleTimer mTrickleTimer;

    TransactionId mTransactionId;
    TimeMilli     mStartTime;

    IdentityAssociation  mIdentityAssociations[OPENTHREAD_CONFIG_DHCP6_CLIENT_NUM_PREFIXES];
    IdentityAssociation *mIdentityAssociationCurrent;
};

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_CLIENT_HPP_
