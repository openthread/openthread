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
 *   This file includes definitions for IPv6 datagram filtering.
 */

#ifndef IP6_FILTER_HPP_
#define IP6_FILTER_HPP_

#include "openthread-core-config.h"

#include "common/message.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ipv6
 *
 * @brief
 *   This module includes definitions for IPv6 datagram filtering.
 *
 * @{
 *
 */

/**
 * This class implements an IPv6 datagram filter.
 *
 */
class Filter
{
public:
    /**
     * This constructor initializes the Filter object.
     *
     */
    Filter(void);

    /**
     * This method indicates whether or not the IPv6 datagram passes the filter.
     *
     * @param[in]  aMessage  The IPv6 datagram to process.
     *
     * @retval TRUE   Accept the IPv6 datagram.
     * @retval FALSE  Reject the IPv6 datagram.
     *
     */
    bool Accept(Message &aMessage) const;

    /**
     * This method adds a port to the allowed unsecured port list.
     *
     * @param[in]  aPort  The port value.
     *
     * @retval OT_ERROR_NONE     The port was successfully added to the allowed unsecure port list.
     * @retval OT_ERROR_NO_BUFS  The unsecure port list is full.
     *
     */
    otError AddUnsecurePort(uint16_t aPort);

    /**
     * This method removes a port from the allowed unsecure port list.
     *
     * @param[in]  aPort  The port value.
     *
     * @retval OT_ERROR_NONE       The port was successfully removed from the allowed unsecure port list.
     * @retval OT_ERROR_NOT_FOUND  The port was not found in the unsecure port list.
     *
     */
    otError RemoveUnsecurePort(uint16_t aPort);

    /**
     * This method removes all ports from the allowed unsecure port list.
     *
     */
    void RemoveAllUnsecurePorts(void);

    /**
     * This method returns a pointer to the unsecure port list.
     *
     * @note Port value 0 is used to indicate an invalid entry.
     *
     * @param[out]  aNumEntries  The number of entries in the list.
     *
     * @returns A pointer to the unsecure port list.
     *
     */
    const uint16_t *GetUnsecurePorts(uint8_t &aNumEntries) const;

private:
    enum
    {
        kMaxUnsecurePorts = 2,
    };
    uint16_t mUnsecurePorts[kMaxUnsecurePorts];
};

} // namespace Ip6
} // namespace ot

#endif // IP6_FILTER_HPP_
