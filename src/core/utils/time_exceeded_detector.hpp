/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for the TimeExceededDetector module.
 */

#ifndef TIME_EXCEEDED_DETECTOR_HPP_
#define TIME_EXCEEDED_DETECTOR_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TIME_EXCEEDED_DETECTION_ENABLE

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

#include <openthread/ip6.h>
#include <openthread/mesh_diag.h>
#include <openthread/thread.h>
#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/logging.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace TimeExceededDetector {

/**
 * Implements the TimeExceededDetector functionality.
 */

class TimeExceededDetector : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */

    explicit TimeExceededDetector(Instance &aInstance);

    /**
     * Detect if the hop limit of a IPv6 packet coming from the infrastructure network is insufficient
     * to reach the destination (if it will expire before reaching the destination).
     *
     * @param[in]  aHeader          The IPv6 header of the packet.
     * @param[out] aDyingAtAddress  A pointer to the OMR IPv6 address of the node where the packet will maybe expire.
     *
     * @returns true if the hop limit is insufficient (packet will expire in the network before reaching destination)
     */

    bool IsHopLimitInsufficient(const Ip6::Header &aHeader, Ip6::Address &aDyingAtAddress);

    /**
     * This constant specifies the maximum number of IPv6 addresses to store for each router in the topology table.
     */

    static constexpr uint8_t kMaxIp6Addresses = 5;

    /**
     * This constant specifies the maximum number of child devices to store for each router in the topology table.
     */
    static constexpr uint8_t kMaxChildren = 25;

    /**
     * Represents a child device (end device) connected to a parent router in the Thread network (topology table).
     */

    struct ChildNode
    {
        uint16_t     mRloc16;     ///< RLOC16 identifier of the child device.
        bool         mHasIp;      ///< true if a IP address is known for this child device.
        otIp6Address mIp6Address; ///< OMR IPv6 address of the child device.
    };

    /**
     * Represents a Thread router in the Thread network (topology table).
     */
    struct RouterNode
    {
        bool     mValid;  ///< true if the router is currently present in the network topology.
        uint16_t mRloc16; ///< RLOC16 identifier of the router.
        uint8_t
            mLinkQualities[OT_NETWORK_MAX_ROUTER_ID + 1]; ///< Table storing incomming link quality with other routers.
        uint8_t      mIp6AddressCount;                    ///< Number of IPv6 addresses known for this router.
        otIp6Address mIp6Addresses[kMaxIp6Addresses];     ///< Table of the IPv6 addresses associated with the router.
        uint8_t      mChildCount;                         ///< Number of child devices attached to the router.
        ChildNode    mChildren[kMaxChildren];             ///< Table of the child devices attached to the router.
    };

    /**
     * Returns the topology table containing the information about all routers in the Thread network.
     *
     * @returns A pointer to the topology table.
     */

    const RouterNode *GetTopologyTable(void) { return mTopologyTable; }

private:
    void HandleTimer(void);
    using DetectorTimer = TimerMilliIn<TimeExceededDetector, &TimeExceededDetector::HandleTimer>;

    /**
     * Topology table that is used to store the information about all the routers in the Thread network.
     */
    RouterNode mTopologyTable[OT_NETWORK_MAX_ROUTER_ID + 1];

    /**
     *   Number of routers in the Thread network (stored in the mTopologyTable).
     */

    uint8_t mRouterCount;

    /**
     * Timer used to schedule the topology discovery process using the HandleTimer function.
     */

    DetectorTimer mTimer;

    /**
     * Index used to iterate through the Thread routers in mTopologyTable during phase 2 of the topology discovery
     * process.
     */
    uint8_t mRouterIteratorIndex;

    /**
     * Indicates whether the time exceeded detector is currently in phase 2 of the topology discovery process (retrieval
     * of child IPv6 addresses).
     */
    bool mIsQueryingChildIps;

    /**
     * Indicates whether the time exceeded detector is waiting for the next scheduled topology discovery to start a new
     * topology discovery process.
     */
    bool mWaitingForNextDiscovery;

    /**
     * Indicates whether the network topology discovery process is fully finished and the topology table is ready to be
     * used.
     */
    bool mIsTopologyComputed;

    /**
     * Starts the first phase of the Thread network topology discovery process (Thread routers discovery).
     */

    void StartDiscoveryPhase1(void);

    /**
     * Callback function used for the phase1 network topology discovery (discovery of the Thread routers).
     *
     * @param[in]  aError       Error code associated with the discovery.
     * @param[in]  aRouterInfo  A pointer to the discovered router information.
     * @param[in]  aContext     A pointer to the TimeExceededDetector instance.
     */

    static void HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext);

    /**
     * Function used to process the mesh diagnostic responses received from the Thread routers during phase1 of network
     * topology discovery.
     *
     * @param[in]  aError       The error code associated with the discovery.
     * @param[in]  aRouterInfo  A pointer to the discovered router information.
     */
    void HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo);

    /**
     * Manages the second phase of the Thread network topology discovery process (iterative retrieval of child devices
     * IPv6 addresses).
     */

    void QueryNextRouterForChildIps(void);

    /**
     * Callback function used for the phase2 network topology discovery (retrieval of child devices IPv6 addresses).
     *
     * @param[in]  aError       Error code associated with the query.
     * @param[in]  aRloc16      RLOC16 identifier of the child device corresponding to the discovered IPv6 addresses.
     * @param[in]  aIterator    A pointer to the iterator containing the discovered IPv6 addresses.
     * @param[in]  aContext     A pointer to the TimeExceededDetector instance.
     */
    static void HandleChildIpQueryDone(otError                    aError,
                                       uint16_t                   aRloc16,
                                       otMeshDiagIp6AddrIterator *aIterator,
                                       void                      *aContext);

    /**
     * Function used to process the mesh diagnostic responses (child devices IPv6 addresses) received during phase 2 of
     * network topology discovery.
     *
     * @param[in]  aError       The error code associated with the query.
     * @param[in]  aRloc16      RLOC16 identifier of the child device corresponding to the discovered IPv6 addresses.
     * @param[in]  aIterator    A pointer to the iterator containing the discovered IPv6 addresses.
     */
    void HandleChildIpQueryDone(otError aError, uint16_t aRloc16, otMeshDiagIp6AddrIterator *aIterator);

    /**
     * Debug function used to log the topology of the network (Thread routers and child devices stored in the
     * mTopologyTable).
     */
    void LogTopologyTable(void);

    /**
     * Represents the information about a destination node (Thread router or child end device) in the network topology.
     */
    struct DestinationInfo
    {
        bool    mFound;    ///< true if the destination address node was found in the network topology.
        uint8_t mRouterId; ///< Router ID of the destination router (or parent router if destination is child device).
        bool    mIsChild; ///< true if the destination is a child end device (then mRouterId = router ID of the parent).
    };

    /**
     * Find the destination Thread router or child end device in the network topology and returns
     * corresponding informations for this node (in a DestinationInfo data structure).
     *
     * @param[in] aDestIp   The destination IPv6 address.
     *
     * @returns information about the destination node (stored in a DestinationInfo data structure)
     */

    DestinationInfo FindDestination(const Ip6::Address &aDestIp);

    /**
     * Returns the link cost value corresponding to the input link quality.
     *
     * @param[in] aLinkQuality  The link quality value (from 0 to 3).
     *
     * @returns The link cost value corresponding to the input link quality.
     */
    uint8_t GetLinkCost(uint8_t aLinkQuality);

    /**
     * Computes the least cost path between two routers using Dijkstra's algorithm.
     *
     * @param[in]  aStartRouterId   The router ID of the source router.
     * @param[in]  aEndRouterId     The router ID of the destination router.
     * @param[out] aPathBuffer      A pointer to the buffer where the computed path (sequence of router ID) will be
     * stored.
     * @param[in]  aMaxLen          The maximum number of Router IDs that can be stored in the buffer.
     *
     * @returns The number of hops in the computed least cost path.
     */
    uint8_t ComputeLeastCostPath(uint8_t aStartRouterId, uint8_t aEndRouterId, uint8_t *aPathBuffer, uint8_t aMaxLen);
};

/**
 * @}
 */

} // namespace TimeExceededDetector
} // namespace ot

#endif // OPENTHREAD_CONFIG_TIME_EXCEEDED_DETECTION_ENABLE
#endif // TIME_EXCEEDED_DETECTOR_HPP_