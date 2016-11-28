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
 *   This file includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 */

#ifndef NETWORK_DATA_LEADER_HPP_
#define NETWORK_DATA_LEADER_HPP_

#include <stdint.h>

#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <net/ip6_address.hpp>
#include <thread/mle_router.hpp>
#include <thread/network_data.hpp>

namespace Thread {

class ThreadNetif;

namespace NetworkData {

/**
 * @addtogroup core-netdata-leader
 *
 * @brief
 *   This module includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 *
 * @{
 *
 */

/**
 * This class implements the Thread Network Data maintained by the Leader.
 *
 */
class LeaderBase: public NetworkData
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit LeaderBase(ThreadNetif &aThreadNetif);

    /**
     * This method reset the Thread Network Data.
     *
     */
    void Reset(void);

    /**
     * This method returns the Thread Network Data version.
     *
     * @returns The Thread Network Data version.
     *
     */
    uint8_t GetVersion(void) const;

    /**
     * This method returns the Thread Network Data stable version.
     *
     * @returns The Thread Network Data stable version.
     *
     */
    uint8_t GetStableVersion(void) const;

    /**
     * This method retrieves the 6LoWPAN Context information based on a given IPv6 address.
     *
     * @param[in]   aAddress  A reference to an IPv6 address.
     * @param[out]  aContext  A reference to 6LoWPAN Context information.
     *
     * @retval kThreadError_None      Successfully retrieved 6LoWPAN Context information.
     * @retval kThreadError_NotFound  Could not find the 6LoWPAN Context information.
     *
     */
    ThreadError GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext);

    /**
     * This method retrieves the 6LoWPAN Context information based on a given Context ID.
     *
     * @param[in]   aContextId  The Context ID value.
     * @param[out]  aContext    A reference to the 6LoWPAN Context information.
     *
     * @retval kThreadError_None      Successfully retrieved 6LoWPAN Context information.
     * @retval kThreadError_NotFound  Could not find the 6LoWPAN Context information.
     *
     */
    ThreadError GetContext(uint8_t aContextId, Lowpan::Context &aContext);

    /**
     * This method indicates whether or not the given IPv6 address is on-mesh.
     *
     * @param[in]  aAddress  A reference to an IPv6 address.
     *
     * @retval TRUE   If @p aAddress is on-link.
     * @retval FALSE  If @p aAddress if not on-link.
     *
     */
    bool IsOnMesh(const Ip6::Address &aAddress);

    /**
     * This method performs a route lookup using the Network Data.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  A pointer to the longest prefix match length in bits.
     * @param[out]  aRloc16       A pointer to the RLOC16 for the selected route.
     *
     * @retval kThreadError_None     Successfully found a route.
     * @retval kThreadError_NoRoute  No valid route was found.
     *
     */
    ThreadError RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination,
                            uint8_t *aPrefixMatch, uint16_t *aRloc16);

    /**
     * This method is used by non-Leader devices to set newly received Network Data from the Leader.
     *
     * @param[in]  aVersion        The Version value.
     * @param[in]  aStableVersion  The Stable Version value.
     * @param[in]  aStableOnly     TRUE if storing only the stable data, FALSE otherwise.
     * @param[in]  aData           A pointer to the Network Data.
     * @param[in]  aDataLength     The length of the Network Data in bytes.
     *
     */
    void SetNetworkData(uint8_t aVersion, uint8_t aStableVersion, bool aStableOnly, const uint8_t *aData,
                        uint8_t aDataLength);

    /**
     * This method sends a Server Data Notification message to the Leader indicating an invalid RLOC16.
     *
     * @param[in]  aRloc16  The invalid RLOC16 to notify.
     *
     * @retval kThreadError_None    Successfully enqueued the notification message.
     * @retval kThreadError_NoBufs  Insufficient message buffers to generate the notification message.
     *
     */
    ThreadError SendServerDataNotification(uint16_t aRloc16);

    /**
     * This method returns a pointer to the Commissioning Data.
     *
     * @param[out]  aLength  The length of the Commissioning Data in bytes.
     *
     * @returns A pointer to the Commissioning Data or NULL if no Commissioning Data exists.
     *
     */
    uint8_t *GetCommissioningData(uint8_t &aLength);

    /**
     * This method adds Commissioning Data to the Thread Network Data.
     *
     * @param[in]  aValue        A pointer to the Commissioning Data value.
     * @param[in]  aValueLength  The length of @p aValue.
     *
     * @retval kThreadError_None    Successfully added the Commissioning Data.
     * @retval kThreadError_NoBufs  Insufficient space to add the Commissioning Data.
     *
     */
    ThreadError SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength);

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
    /**
     * This method gets the Rloc of Dhcp Agent of specified contextId.
     *
     * @param[in]  aContextId      A pointer to the Commissioning Data value.
     * @param[out] aRloc16         The reference of which for output the Rloc16.
     *
     * @retval kThreadError_None      Successfully get the Rloc of Dhcp Agent.
     * @retval kThreadError_NotFound  The specified @p aContextId could not be found.
     *
     */
    ThreadError GetRlocByContextId(uint8_t aContextId, uint16_t &aRloc16);
#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT

protected:
    uint8_t         mStableVersion;
    uint8_t         mVersion;
    ThreadNetif    &mNetif;

private:
    ThreadError RemoveCommissioningData(void);

    ThreadError ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &destination,
                                    uint8_t *aPrefixMatch, uint16_t *aRloc16);
    ThreadError DefaultRouteLookup(PrefixTlv &aPrefix, uint16_t *aRloc16);
};

/**
 * @}
 */

}  // namespace NetworkData
}  // namespace Thread

#ifdef OPENTHREAD_MTD
#include "network_data_leader_mtd.hpp"
#else
#include "network_data_leader_ftd.hpp"
#endif

#endif  // NETWORK_DATA_LEADER_HPP_
