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

#include <openthread/dhcp6_client.h>

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "common/trickle_timer.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "net/dhcp6.hpp"
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
 * Some constants
 *
 */
enum
{
    kTrickleTimerImin = 1,
    kTrickleTimerImax = 120,
};

/**
 * This class implements IdentityAssociation.
 *
 */
OT_TOOL_PACKED_BEGIN
class IdentityAssociation
{
public:
    /**
     * Status of IdentityAssociation
     *
     */
    typedef enum Status {
        kStatusInvalid,
        kStatusSolicit,
        kStatusSoliciting,
        kStatusSolicitReplied,
    } Status;

    /**
     * This method returns the status of the object.
     *
     * @returns Status.
     *
     */
    Status GetStatus(void) const { return static_cast<Status>(mStatus); }

    /**
     * This method sets the status of the object.
     *
     * @param[in]  aStatus  The Status to set.
     *
     */
    void SetStatus(Status aStatus) { mStatus = static_cast<uint8_t>(aStatus); }

    /**
     * This method returns the rloc of the DHCP Agent.
     *
     * @returns Status.
     *
     */
    uint16_t GetPrefixAgentRloc(void) const { return mPrefixAgentRloc; }

    /**
     * This method sets the rloc of the DHCP Agent.
     *
     * @param[in]  aRloc  The rloc of the DHCP Agent.
     *
     */
    void SetPrefixAgentRloc(uint16_t aRloc16) { mPrefixAgentRloc = aRloc16; }

    /**
     * This method returns the pointer to the IPv6 prefix.
     *
     * @returns A pointer to the IPv6 prefix.
     *
     */
    otIp6Prefix *GetPrefix(void) { return &mIp6Prefix; }

    /**
     * This method sets the IPv6 prefix to specified location.
     *
     * @param[in]  aIp6Prefix The reference to the IPv6 prefix to set.
     *
     */
    void SetPrefix(otIp6Prefix &aIp6Prefix) { memcpy(&mIp6Prefix, &aIp6Prefix, sizeof(otIp6Prefix)); }

    /**
     * This method returns the pointer to the next IdentityAssociation.
     *
     * @returns A pointer to the next IdentityAssociation.
     *
     */
    IdentityAssociation *GetNext(void) { return mNext; }

    /**
     * This method sets the pointer to the next IdentityAssociation.
     *
     */
    void SetNext(IdentityAssociation *aNext) { mNext = aNext; }

private:
    uint8_t              mStatus;          ///< Status of IdentityAssocation
    uint16_t             mPrefixAgentRloc; ///< Rloc of Prefix Agent
    otIp6Prefix          mIp6Prefix;       ///< Prefix
    IdentityAssociation *mNext;            ///< Pointer to next IdentityAssocation
} OT_TOOL_PACKED_END;

/**
 * This class implements DHCPv6 Client.
 *
 */
class Dhcp6Client : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Dhcp6Client(Instance &aInstance);

    /**
     * This method update addresses that shall be automatically created using DHCP.
     *
     * @param[in]     aInstance     A pointer to OpenThread instance.
     * @param[inout]  aAddresses    A pointer to an array containing addresses created by this module.
     * @param[in]     aNumAddresses The number of elements in aAddresses array.
     * @param[in]     aContext      A pointer to IID creator-specific context data.
     *
     */
    void UpdateAddresses(otInstance *aInstance, otDhcpAddress *aAddresses, uint32_t aNumAddresses, void *aContext);

private:
    otError Start(void);
    otError Stop(void);

    otError Solicit(uint16_t aRloc16);

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

    Ip6::UdpSocket mSocket;

    TrickleTimer mTrickleTimer;

    uint8_t        mTransactionId[kTransactionIdSize];
    uint32_t       mStartTime;
    otDhcpAddress *mAddresses;
    uint32_t       mNumAddresses;

    IdentityAssociation  mIdentityAssociations[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
    IdentityAssociation *mIdentityAssociationHead;
    IdentityAssociation *mIdentityAssociationAvail;
};

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_CLIENT_HPP_
