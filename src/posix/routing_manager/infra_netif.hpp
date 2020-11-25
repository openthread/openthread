/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for infrastructure network interface.
 *
 */

#ifndef POSIX_INFRA_NETIF_HPP_
#define POSIX_INFRA_NETIF_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <stdint.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openthread/error.h>
#include <openthread/ip6.h>
#include <openthread/openthread-system.h>

namespace ot {

namespace Posix {

/**
 * This class represents an infrastructure network interface (e.g. wlan0).
 *
 */
class InfraNetif
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    InfraNetif();

    /**
     * This method initializes the network interface with given interface name.
     *
     * @param[in]  aName  An interface name.
     *
     */
    void Init(const char *aName);

    /**
     * This method deinitializes the network interface.
     *
     */
    void Deinit();

    /**
     * This method updates a mainloop context.
     *
     * @param[in]  aMainloop  A mainloop context to be updated.
     *
     */
    void Update(otSysMainloopContext *aMainloop);

    /**
     * This method process events in a mainloop context.
     *
     * @param[in]  aMainloop  A mainloop context to be processed.
     *
     */
    void Process(const otSysMainloopContext *aMainloop);

    /**
     * This method checks if the network interface is UP.
     *
     */
    bool IsUp() const;

    /**
     * This method returns the name of the interface.
     *
     * @return  The name of the interface.
     *
     */
    const char *GetName() const { return mName; }

    /**
     * This method returns the index of the interface.
     *
     * @return  The index of the interface.
     *
     */
    unsigned int GetIndex() const { return mIndex; }

    /**
     * This method adds a gateway address with given on-link prefix.
     *
     * @param[in]  aOnLinkPrefix  An on-link prefix.
     *
     */
    void AddGatewayAddress(const otIp6Prefix &aOnLinkPrefix)
    {
        UpdateGatewayAddress(aOnLinkPrefix, /* aIsAdded */ true);
    }

    /**
     * This method removes the gateway address with given on-link prefix.
     *
     * @param[in]  aOnLinkPrefix  An on-link prefix.
     *
     */
    void RemoveGatewayAddress(const otIp6Prefix &aOnLinkPrefix)
    {
        UpdateGatewayAddress(aOnLinkPrefix, /* aIsAdded */ false);
    }

private:
    /**
     * This method initializes the netlink fd.
     *
     */
    void InitNetlink();

    /**
     * This method adds/removes the gateway address with given on-link prefix.
     *
     * @param[in]  aOnLinkPrefix  An on-link prefix.
     * @param[in]  aIsAdded       A boolean indicates whether to add or remove the gateway address.
     *
     */
    void UpdateGatewayAddress(const otIp6Prefix &aOnLinkPrefix, bool aIsAdded);

    /**
     * This method adds/removes an unicast address.
     *
     * @param[in]  aAddressInfo  The unicast address to be added/removed.
     * @param[in]  aIsAdded      A boolean indicates whether to add or remove the address.
     *
     */
    void UpdateUnicastAddress(const otIp6AddressInfo &aAddressInfo, bool aIsAdded);

    char         mName[IFNAMSIZ]; ///< The interface name.
    unsigned int mIndex;          ///< the interface index.

    int      mNetlinkFd;       ///< The netlink fd.
    uint32_t mNetlinkSequence; ///< The netlink message sequence.
};

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // POSIX_INFRA_NETIF_HPP_
