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

#ifndef IP6_ROUTES_HPP_
#define IP6_ROUTES_HPP_

/**
 * @file
 *   This file includes definitions for manipulating IPv6 routing tables.
 */

#include <openthread-types.h>
#include <common/message.hpp>
#include <net/ip6_address.hpp>

namespace Thread {
namespace Ip6 {

/**
 * @addtogroup core-ip6-ip6
 *
 * @{
 *
 */

/**
 * This structure represents an IPv6 route.
 *
 */
struct Route
{
    Address       mPrefix;        ///< The IPv6 prefix.
    uint8_t       mPrefixLength;  ///< The IPv6 prefix length.
    int8_t        mInterfaceId;   ///< The interface identifier.
    struct Route *mNext;          ///< A pointer to the next IPv6 route.
};

/**
 * This class implements IPv6 route management.
 *
 */
class Routes
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aIp6  A reference to the IPv6 network object.
     *
     */
    Routes(Ip6 &aIp6);

    /**
     * This method adds an IPv6 route.
     *
     * @param[in]  aRoute  A reference to the IPv6 route.
     *
     * @retval kThreadError_None  Successfully added the route.
     * @retval kThreadError_Busy  The route was already added.
     *
     */
    ThreadError Add(Route &aRoute);

    /**
     * This method removes an IPv6 route.
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     * @param[in]  aRoute     A reference to the IPv6 route.
     *
     * @retval kThreadError_None         Successfully removed the route.
     * @retval kThreadError_InvalidArgs  The route was not added.
     *
     */
    ThreadError Remove(Route &aRoute);

    /**
     * This method performs source-destination route lookup.
     *
     * @param[in]  aSource       The IPv6 source address.
     * @param[in]  aDestination  The IPv6 destination address.
     *
     * @returns The interface identifier for the best route or -1 if no route is available.
     *
     */
    int8_t Lookup(const Address &aSource, const Address &aDestination);

private:
    Route *mRoutes;
    Ip6 &mIp6;
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_IP6_ROUTES_HPP_
