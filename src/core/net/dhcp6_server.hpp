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

#include "openthread/types.h"

#include <mac/mac_frame.hpp>
#include <mac/mac.hpp>
#include <net/dhcp6.hpp>
#include <net/udp6.hpp>
#include <thread/network_data_leader.hpp>

namespace ot {

class ThreadNetif;
namespace NetworkData { class Leader; }

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
#define OT_DHCP6_DEFAULT_IA_NA_T1            0xffffffffU
#define OT_DHCP6_DEFAULT_IA_NA_T2            0xffffffffU
#define OT_DHCP6_DEFAULT_PREFERRED_LIFETIME  0xffffffffU
#define OT_DHCP6_DEFAULT_VALID_LIFETIME      0xffffffffU

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
    otIp6Prefix mIp6Prefix;                  ///< prefix
} OT_TOOL_PACKED_END;

class Dhcp6Server
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit Dhcp6Server(ThreadNetif &aThreadNetif);

    /**
     * This method updates DHCP Agents and DHCP Alocs.
     *
     */
    ThreadError UpdateService();

private:
    ThreadError Start(void);
    ThreadError Stop(void);

    ThreadError AddPrefixAgent(otIp6Prefix &aIp6Prefix);
    ThreadError RemovePrefixAgent(const uint8_t *aIp6Address);

    ThreadError AppendHeader(Message &aMessage, uint8_t *aTransactionId);
    ThreadError AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClient);
    ThreadError AppendServerIdentifier(Message &aMessage);
    ThreadError AppendIaNa(Message &aMessage, IaNa &aIaNa);
    ThreadError AppendStatusCode(Message &aMessage, Status aStatusCode);
    ThreadError AppendIaAddress(Message &aMessage, ClientIdentifier &aClient);
    ThreadError AppendRapidCommit(Message &aMessage);
    ThreadError AppendVendorSpecificInformation(Message &aMessage);

    ThreadError AddIaAddress(Message &aMessage, otIp6Prefix &aIp6Prefix, ClientIdentifier &aClient);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void ProcessSolicit(Message &aMessage, otIp6Address &aDst, uint8_t *aTransactionId);

    uint16_t FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode);
    ThreadError ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClient);
    ThreadError ProcessIaNa(Message &aMessage, uint16_t aOffset, IaNa &aIaNa);
    ThreadError ProcessIaAddress(Message &aMessage, uint16_t aOffset);
    ThreadError ProcessElapsedTime(Message &aMessage, uint16_t aOffset);

    ThreadError SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClientIdentifier, IaNa &aIaNa);

    Ip6::UdpSocket mSocket;

    ThreadNetif &mNetif;

    Ip6::NetifUnicastAddress mAgentsAloc[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
    PrefixAgent mPrefixAgents[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
    uint8_t mPrefixAgentsMask;
    uint8_t mPrefixAgentsCount;
};

}  // namespace Dhcp6
}  // namespace ot

#endif  // DHCP6_SERVER_HPP_
