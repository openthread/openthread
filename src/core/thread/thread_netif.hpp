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
 *   This file includes definitions for the Thread network interface.
 */

#ifndef THREAD_NETIF_HPP_
#define THREAD_NETIF_HPP_

#include "openthread-core-config.h"

#include "net/ip6_address.hpp"
#include "net/netif.hpp"

namespace ot {

/**
 * @addtogroup core-netif
 *
 * @brief
 *   This module includes definitions for the Thread network interface.
 *
 * @{
 */

class ThreadNetif : public Ip6::Netif
{
public:
    /**
     * This constructor initializes the Thread network interface.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit ThreadNetif(Instance &aInstance);

    /**
     * This method enables the Thread network interface.
     *
     */
    void Up(void);

    /**
     * This method disables the Thread network interface.
     *
     */
    void Down(void);

    /**
     * This method indicates whether or not the Thread network interface is enabled.
     *
     * @retval TRUE   If the Thread network interface is enabled.
     * @retval FALSE  If the Thread network interface is not enabled.
     *
     */
    bool IsUp(void) const { return mIsUp; }

    /**
     * This method submits a message to the network interface.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kErrorNone  Successfully submitted the message to the interface.
     *
     */
    Error SendMessage(Message &aMessage);

    /**
     * This method performs a route lookup.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  A pointer where the number of prefix match bits for the chosen route is stored.
     *
     * @retval kErrorNone      Successfully found a route.
     * @retval kErrorNoRoute   Could not find a valid route.
     *
     */
    Error RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint8_t *aPrefixMatch);

    /**
     * This method indicates whether @p aAddress matches an on-mesh prefix.
     *
     * @param[in]  aAddress  The IPv6 address.
     *
     * @retval TRUE   If @p aAddress matches an on-mesh prefix.
     * @retval FALSE  If @p aAddress does not match an on-mesh prefix.
     *
     */
    bool IsOnMesh(const Ip6::Address &aAddress) const;

private:
    bool mIsUp;
};

/**
 * @}
 */

} // namespace ot

#endif // THREAD_NETIF_HPP_
