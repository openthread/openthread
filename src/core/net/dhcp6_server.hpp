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
 *   This file includes definitions for DHCPv6 Server.
 */

#ifndef DHCP6_SERVER_HPP_
#define DHCP6_SERVER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "net/dhcp6.hpp"
#include "net/udp6.hpp"
#include "thread/network_data_leader.hpp"

namespace ot {

namespace Dhcp6 {

/**
 * @addtogroup core-dhcp6
 *
 * @brief
 *   This module includes definitions for DHCPv6 Server.
 *
 * @{
 *
 */

/**
 * DHCPv6 default constant
 *
 */
#define OT_DHCP6_DEFAULT_IA_NA_T1 0xffffffffU
#define OT_DHCP6_DEFAULT_IA_NA_T2 0xffffffffU
#define OT_DHCP6_DEFAULT_PREFERRED_LIFETIME 0xffffffffU
#define OT_DHCP6_DEFAULT_VALID_LIFETIME 0xffffffffU

/**
 * This class implements prefix agent.
 *
 */

OT_TOOL_PACKED_BEGIN
class PrefixAgent
{
public:
    /**
     * This method returns the reference to the IPv6 prefix.
     *
     * @returns A reference to the IPv6 prefix.
     *
     */
    otIp6Prefix *GetPrefix(void) { return &mIp6Prefix; }

    /**
     * This method sets the IPv6 prefix.
     *
     * @param[in]  aIp6Prefix The reference to the IPv6 prefix to set.
     *
     */
    void SetPrefix(otIp6Prefix &aIp6Prefix) { memcpy(&mIp6Prefix, &aIp6Prefix, sizeof(otIp6Prefix)); }

private:
    otIp6Prefix mIp6Prefix; ///< prefix
} OT_TOOL_PACKED_END;

class Dhcp6Server : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Dhcp6Server(Instance &aInstance);

    /**
     * This method updates DHCP Agents and DHCP Alocs.
     *
     */
    otError UpdateService();

private:
    otError Start(void);
    otError Stop(void);

    otError AddPrefixAgent(otIp6Prefix &aIp6Prefix);
    otError RemovePrefixAgent(const uint8_t *aIp6Address);

    otError AppendHeader(Message &aMessage, uint8_t *aTransactionId);
    otError AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClient);
    otError AppendServerIdentifier(Message &aMessage);
    otError AppendIaNa(Message &aMessage, IaNa &aIaNa);
    otError AppendStatusCode(Message &aMessage, Status aStatusCode);
    otError AppendIaAddress(Message &aMessage, ClientIdentifier &aClient);
    otError AppendRapidCommit(Message &aMessage);
    otError AppendVendorSpecificInformation(Message &aMessage);

    otError AddIaAddress(Message &aMessage, otIp6Prefix &aIp6Prefix, ClientIdentifier &aClient);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void ProcessSolicit(Message &aMessage, otIp6Address &aDst, uint8_t *aTransactionId);

    uint16_t FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode);
    otError  ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClient);
    otError  ProcessIaNa(Message &aMessage, uint16_t aOffset, IaNa &aIaNa);
    otError  ProcessIaAddress(Message &aMessage, uint16_t aOffset);
    otError  ProcessElapsedTime(Message &aMessage, uint16_t aOffset);

    otError SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClientIdentifier, IaNa &aIaNa);

    Ip6::UdpSocket mSocket;

    Ip6::NetifUnicastAddress mAgentsAloc[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
    PrefixAgent              mPrefixAgents[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
    uint8_t                  mPrefixAgentsMask;
    uint8_t                  mPrefixAgentsCount;
};

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_SERVER_HPP_
