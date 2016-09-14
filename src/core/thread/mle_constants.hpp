/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

namespace Thread {
namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 */

enum
{
    kMaxChildren                = OPENTHREAD_CONFIG_MAX_CHILDREN,
    kMaxChildKeepAliveAttempts  = 4,    ///< Maximum keep alive attempts before attempting to reattach to a new Parent
};

/**
 * MLE Protocol Constants
 *
 */
enum
{
    kVersion                    = 2,     ///< MLE Version
    kUdpPort                    = 19788, ///< MLE UDP Port
    kParentRequestRouterTimeout = 1000,  ///< Router Request timeout
    kParentRequestChildTimeout  = 2000,  ///< End Device Request timeout
};

enum
{
    kMinChildId                 = 1,     ///< Minimum Child ID
    kMaxChildId                 = 511,   ///< Maximum Child ID
    kRouterIdOffset             = 10,    ///< Bit offset of Router ID in RLOC16
    kRlocPrefixLength           = 14,    ///< Prefix length of RLOC in bytes
};

/**
 * Routing Protocol Constants
 *
 */
enum
{
    kAdvertiseIntervalMin       = 1,    ///< ADVERTISEMENT_I_MIN (seconds)
    kAdvertiseIntervalMax       = 32,   ///< ADVERTISEMENT_I_MAX (seconds)
    kRouterIdReuseDelay         = 100,  ///< ID_REUSE_DELAY (seconds)
    kRouterIdSequencePeriod     = 10,   ///< ID_SEQUENCE_PERIOD (seconds)
    kMaxNeighborAge             = 100,  ///< MAX_NEIGHBOR_AGE (seconds)
    kMaxRouteCost               = 16,   ///< MAX_ROUTE_COST
    kMaxRouterId                = 62,   ///< MAX_ROUTER_ID
    kMaxRouters                 = 32,   ///< MAX_ROUTERS
    kMinDowngradeNeighbors      = 7,    ///< MIN_DOWNGRADE_NEIGHBORS
    kNetworkIdTimeout           = 120,  ///< NETWORK_ID_TIMEOUT (seconds)
    kParentRouteToLeaderTimeout = 20,   ///< PARENT_ROUTE_TO_LEADER_TIMEOUT (seconds)
    kRouterSelectionJitter      = 120,  ///< ROUTER_SELECTION_JITTER (seconds)
    kRouterDowngradeThreshold   = 23,   ///< ROUTER_DOWNGRADE_THRESHOLD (routers)
    kRouterUpgradeThreshold     = 16,   ///< ROUTER_UPGRADE_THRESHOLD (routers)
    kMaxLeaderToRouterTimeout   = 90,   ///< INFINITE_COST_TIMEOUT (seconds)
    kReedAdvertiseInterval      = 570,  ///< REED_ADVERTISEMENT_INTERVAL (seconds)
    kReedAdvertiseJitter        = 60,   ///< REED_ADVERTISEMENT_JITTER (seconds)
    kMleEndDeviceTimeout        = 240,  ///< MLE_END_DEVICE_TIMEOUT (seconds)
    kLeaderWeight               = 64,   ///< Default leader weight for the Thread Network Partition
};

enum
{
    kLqi3LinkCost               = 1,    ///< Link Cost for LQI 3
    kLqi2LinkCost               = 2,    ///< Link Cost for LQI 2
    kLqi1LinkCost               = 4,    ///< Link Cost for LQI 1
    kLqi0LinkCost               = 16,   ///< Link Cost for LQI 0
};

// add for certification testing
enum
{
    kMinAssignedLinkMargin3     = 0x15, ///< minimal link margin for LQI 3 (21 - 255)
    kMinAssignedLinkMargin2     = 0x0b, ///< minimal link margin for LQI 2 (11 - 20)
    kMinAssignedLinkMargin1     = 0x03, ///< minimal link margin for LQI 1 (3 - 9)
    kMinAssignedLinkMargin0     = 0x00, ///< minimal link margin for LQI 0 (0 - 2)
};

}  // namespace Mle

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // MLE_CONSTANTS_HPP_
