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
 * @brief
 *   This file includes the platform abstraction for the Thread DHCPv6 client.
 */

#ifndef OPENTHREAD_DHCP6_CLIENT_H_
#define OPENTHREAD_DHCP6_CLIENT_H_

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-dhcp6
 *
 * @{
 *
 */

/**
 * This structure represents a DHCPv6 address.
 *
 */
typedef struct otDhcpAddress
{
    otNetifAddress mAddress;           ///< The network interface address.
    uint32_t       mPreferredLifetime; ///< The preferred lifetime.
    uint32_t       mValidLifetime;     ///< The valid lifetime.
} otDhcpAddress;

/**
 * Update all automatically created IPv6 addresses for prefixes from current Network Data with DHCP procedure.
 *
 * @param[in]     aInstance      A pointer to an OpenThread instance.
 * @param[inout]  aAddresses     A pointer to an array of automatically created IPv6 addresses.
 * @param[in]     aNumAddresses  The number of slots in aAddresses array.
 * @param[in]     aContext       A pointer to data passed to aIidCreate function.
 *
 */
void otDhcp6ClientUpdate(otInstance *aInstance, otDhcpAddress *aAddresses, uint32_t aNumAddresses, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_DHCP6_CLIENT_H_
