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
#include "mac/mac_types.hpp"
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

    /**
     * This method applies the Mesh Local Prefix.
     *
     */
    void ApplyMeshLocalPrefix(void);

private:
    class PrefixAgent
    {
    public:
        /**
         * This method indicates whether or not @p aPrefix matches this entry.
         *
         * @param[in]  aPrefix  The prefix to compare.
         *
         * @retval TRUE if the prefix matches.
         * @retval FALSE if the prefix does not match.
         *
         */
        bool IsPrefixMatch(const otIp6Prefix &aPrefix) const
        {
            return (mPrefix.mLength == aPrefix.mLength) &&
                   (otIp6PrefixMatch(&mPrefix.mPrefix, &aPrefix.mPrefix) >= mPrefix.mLength);
        }

        /**
         * This method indicates whether or not @p aAddress has a matching prefix.
         *
         * @param[in]  aAddress  The IPv6 address to compare.
         *
         * @retval TRUE if the address has a matching prefix.
         * @retval FALSE if the address does not have a matching prefix.
         *
         */
        bool IsPrefixMatch(const otIp6Address &aAddress) const
        {
            return (otIp6PrefixMatch(&mPrefix.mPrefix, &aAddress) >= mPrefix.mLength);
        }

        /**
         * This method indicates whether or not this entry is valid.
         *
         * @retval TRUE if this entry is valid.
         * @retval FALSE if this entry is not valid.
         *
         */
        bool IsValid(void) const { return mAloc.mValid; }

        /**
         * This method sets the entry to invalid.
         *
         */
        void Clear(void) { mAloc.mValid = false; }

        /**
         * This method returns the 6LoWPAN context ID.
         *
         * @returns The 6LoWPAN context ID.
         *
         */
        uint8_t GetContextId(void) const { return mAloc.mAddress.mFields.m8[15]; }

        /**
         * This method returns the ALOC.
         *
         * @returns the ALOC.
         *
         */
        Ip6::NetifUnicastAddress &GetAloc(void) { return mAloc; }

        /**
         * This method returns the IPv6 prefix.
         *
         * @returns The IPv6 prefix.
         *
         */
        const Ip6::Address &GetPrefix(void) const { return static_cast<const Ip6::Address &>(mPrefix.mPrefix); }

        /**
         * This method sets the ALOC.
         *
         * @param[in]  aPrefix           The IPv6 prefix.
         * @param[in]  aMeshLocalPrefix  The Mesh Local Prefix.
         * @param[in]  aContextId        The 6LoWPAN Context ID.
         *
         */
        void Set(const otIp6Prefix &aPrefix, const Mle::MeshLocalPrefix &aMeshLocalPrefix, uint8_t aContextId)
        {
            mPrefix = aPrefix;

            mAloc.GetAddress().SetToAnycastLocator(aMeshLocalPrefix, (Ip6::Address::kAloc16Mask << 8) + aContextId);
            mAloc.mPrefixLength  = OT_IP6_PREFIX_BITSIZE;
            mAloc.mAddressOrigin = OT_ADDRESS_ORIGIN_THREAD;
            mAloc.mPreferred     = true;
            mAloc.mValid         = true;
        }

    private:
        Ip6::NetifUnicastAddress mAloc;
        otIp6Prefix              mPrefix;
    };

    void Start(void);
    void Stop(void);

    void AddPrefixAgent(const otIp6Prefix &aIp6Prefix, const Lowpan::Context &aContext);

    otError AppendHeader(Message &aMessage, uint8_t *aTransactionId);
    otError AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClientId);
    otError AppendServerIdentifier(Message &aMessage);
    otError AppendIaNa(Message &aMessage, IaNa &aIaNa);
    otError AppendStatusCode(Message &aMessage, Status aStatusCode);
    otError AppendIaAddress(Message &aMessage, ClientIdentifier &aClientId);
    otError AppendRapidCommit(Message &aMessage);
    otError AppendVendorSpecificInformation(Message &aMessage);

    otError AddIaAddress(Message &aMessage, const Ip6::Address &aPrefix, ClientIdentifier &aClientId);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void ProcessSolicit(Message &aMessage, otIp6Address &aDst, uint8_t *aTransactionId);

    uint16_t FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode);
    otError  ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClientId);
    otError  ProcessIaNa(Message &aMessage, uint16_t aOffset, IaNa &aIaNa);
    otError  ProcessIaAddress(Message &aMessage, uint16_t aOffset);
    otError  ProcessElapsedTime(Message &aMessage, uint16_t aOffset);

    otError SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClientId, IaNa &aIaNa);

    Ip6::Udp::Socket mSocket;

    PrefixAgent mPrefixAgents[OPENTHREAD_CONFIG_DHCP6_SERVER_NUM_PREFIXES];
    uint8_t     mPrefixAgentsCount;
    uint8_t     mPrefixAgentsMask;
};

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_SERVER_HPP_
