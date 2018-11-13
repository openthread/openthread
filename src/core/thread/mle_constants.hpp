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
 *   This file includes definitions for MLE functionality required by the Thread Child, Router, and Leader roles.
 */

#ifndef MLE_CONSTANTS_HPP_
#define MLE_CONSTANTS_HPP_

#include "openthread-core-config.h"

namespace ot {
namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 */

enum
{
    kMaxChildren               = OPENTHREAD_CONFIG_MAX_CHILDREN,
    kMaxChildKeepAliveAttempts = 4, ///< Maximum keep alive attempts before attempting to reattach to a new Parent
    kFailedChildTransmissions  = OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS, ///< FAILED_CHILD_TRANSMISSIONS
};

/**
 * MLE Protocol Constants
 *
 */
enum
{
    kThreadVersion                  = 2,     ///< Thread Version
    kUdpPort                        = 19788, ///< MLE UDP Port
    kParentRequestRouterTimeout     = 750,   ///< Router Parent Request timeout
    kParentRequestReedTimeout       = 1250,  ///< Router and REEDs Parent Request timeout
    kAttachStartJitter              = 50,    ///< Maximum jitter time added to start of attach.
    kAnnounceProcessTimeout         = 250,   ///< Timeout after receiving Announcement before channel/pan-id change
    kAnnounceTimeout                = 1400,  ///< Total timeout used for sending Announcement messages
    kMinAnnounceDelay               = 80,    ///< Minimum delay between Announcement messages
    kParentResponseMaxDelayRouters  = 500,   ///< Maximum delay for response for Parent Request sent to routers only
    kParentResponseMaxDelayAll      = 1000,  ///< Maximum delay for response for Parent Request sent to all devices
    kUnicastRetransmissionDelay     = 1000,  ///< Base delay before retransmitting an MLE unicast.
    kChildUpdateRequestPendingDelay = 100,   ///< Delay (in ms) for aggregating Child Update Request.
    kMaxTransmissionCount           = 3,     ///< Maximum number of times an MLE message may be transmitted.
    kMaxResponseDelay               = 1000,  ///< Maximum delay before responding to a multicast request
    kMaxChildIdRequestTimeout       = 5000,  ///< Maximum delay for receiving a Child ID Request
    kMaxChildUpdateResponseTimeout  = 2000,  ///< Maximum delay for receiving a Child Update Response
    kMaxLinkRequestTimeout          = 2000,  ///< Maximum delay for receiving a Link Accept
    kMinTimeoutKeepAlive            = (((kMaxChildKeepAliveAttempts + 1) * kUnicastRetransmissionDelay) /
                            1000), ///< Minimum timeout(s) for keep alive
    kMinTimeoutDataPoll             = (OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD +
                           OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS * OPENTHREAD_CONFIG_RETX_POLL_PERIOD) /
                          1000, ///< Minimum timeout(s) for data poll
    kMinTimeout = (kMinTimeoutKeepAlive >= kMinTimeoutDataPoll ? kMinTimeoutKeepAlive
                                                               : kMinTimeoutDataPoll), ///< Minimum timeout(s)
};

enum
{
    kMinChildId       = 1,   ///< Minimum Child ID
    kMaxChildId       = 511, ///< Maximum Child ID
    kRouterIdOffset   = 10,  ///< Bit offset of Router ID in RLOC16
    kRlocPrefixLength = 14,  ///< Prefix length of RLOC in bytes
};

/**
 * Routing Protocol Constants
 *
 */
enum
{
    kAdvertiseIntervalMin = 1, ///< ADVERTISEMENT_I_MIN (sec)
#if OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES
    kAdvertiseIntervalMax = 5, ///< ADVERTISEMENT_I_MAX (sec) proposal
#else
    kAdvertiseIntervalMax = 32, ///< ADVERTISEMENT_I_MAX (sec)
#endif
    kFailedRouterTransmissions = 4,   ///< FAILED_ROUTER_TRANSMISSIONS
    kRouterIdReuseDelay        = 100, ///< ID_REUSE_DELAY (sec)
    kRouterIdSequencePeriod    = 10,  ///< ID_SEQUENCE_PERIOD (sec)
    kMaxNeighborAge            = 100, ///< MAX_NEIGHBOR_AGE (sec)
#if OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES
    kMaxRouteCost = 127, ///< MAX_ROUTE_COST proposal
#else
    kMaxRouteCost         = 16, ///< MAX_ROUTE_COST
#endif
    kMaxRouterId                = 62,                                      ///< MAX_ROUTER_ID
    kInvalidRouterId            = kMaxRouterId + 1,                        ///< Value indicating incorrect Router Id
    kMaxRouters                 = OPENTHREAD_CONFIG_MAX_ROUTERS,           ///< MAX_ROUTERS
    kMinDowngradeNeighbors      = 7,                                       ///< MIN_DOWNGRADE_NEIGHBORS
    kNetworkIdTimeout           = 120,                                     ///< NETWORK_ID_TIMEOUT (sec)
    kParentRouteToLeaderTimeout = 20,                                      ///< PARENT_ROUTE_TO_LEADER_TIMEOUT (sec)
    kRouterSelectionJitter      = 120,                                     ///< ROUTER_SELECTION_JITTER (sec)
    kRouterDowngradeThreshold   = 23,                                      ///< ROUTER_DOWNGRADE_THRESHOLD (routers)
    kRouterUpgradeThreshold     = 16,                                      ///< ROUTER_UPGRADE_THRESHOLD (routers)
    kMaxLeaderToRouterTimeout   = 90,                                      ///< INFINITE_COST_TIMEOUT (sec)
    kReedAdvertiseInterval      = 570,                                     ///< REED_ADVERTISEMENT_INTERVAL (sec)
    kReedAdvertiseJitter        = 60,                                      ///< REED_ADVERTISEMENT_JITTER (sec)
    kLeaderWeight               = 64,                                      ///< Default leader weight
    kMleEndDeviceTimeout        = OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT, ///< MLE_END_DEVICE_TIMEOUT (sec)
};

/**
 * Parent Priority values
 *
 */
enum
{
    kParentPriorityHigh        = 1,  // Parent Priority High
    kParentPriorityMedium      = 0,  // Parent Priority Medium (default)
    kParentPriorityLow         = -1, // Parent Priority Low
    kParentPriorityUnspecified = -2, // Parent Priority Unspecified
};

enum
{
    kLinkQuality3LinkCost = 1,             ///< Link Cost for Link Quality 3
    kLinkQuality2LinkCost = 2,             ///< Link Cost for Link Quality 2
    kLinkQuality1LinkCost = 4,             ///< Link Cost for Link Quality 1
    kLinkQuality0LinkCost = kMaxRouteCost, ///< Link Cost for Link Quality 0
};

/**
 * Multicast Forwarding Constants
 *
 */
enum
{
    kMplChildDataMessageTimerExpirations  = 0, ///< Number of MPL retransmissions for Children.
    kMplRouterDataMessageTimerExpirations = 2, ///< Number of MPL retransmissions for Routers.
};

} // namespace Mle

/**
 * @}
 *
 */

} // namespace ot

#endif // MLE_CONSTANTS_HPP_
