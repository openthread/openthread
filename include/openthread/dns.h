/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *  This file defines the top-level dns functions for the OpenThread library.
 */

#ifndef OPENTHREAD_DNS_H_
#define OPENTHREAD_DNS_H_

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-dns
 *
 * @brief
 *   This module includes functions that control DNS communication.
 *
 * @{
 *
 */

#define OT_DNS_MAX_HOSTNAME_LENGTH 62 ///< Maximum allowed hostname length (maximum label size - 1 for compression).

#define OT_DNS_DEFAULT_SERVER_IP "2001:4860:4860::8888" ///< Defines default DNS Server address - Google DNS.
#define OT_DNS_DEFAULT_SERVER_PORT 53                   ///< Defines default DNS Server port.

/**
 * This structure implements DNS Query parameters.
 *
 */
typedef struct otDnsQuery
{
    const char *         mHostname;    ///< Identifies hostname to be found. It shall not change during resolving.
    const otMessageInfo *mMessageInfo; ///< A reference to the message info related with DNS Server.
    bool                 mNoRecursion; ///< If cleared, it directs name server to pursue the query recursively.
} otDnsQuery;

/**
 * This function pointer is called when a DNS response is received.
 *
 * @param[in]  aContext   A pointer to application-specific context.
 * @param[in]  aHostname  Identifies hostname related with DNS response.
 * @param[in]  aAddress   A pointer to the IPv6 address received in DNS response. May be null.
 * @param[in]  aTtl       Specifies the maximum time in seconds that the resource record may be cached.
 * @param[in]  aResult    A result of the DNS transaction.
 *
 * @retval  OT_ERROR_NONE              A response was received successfully and IPv6 address is provided
 *                                     in @p aAddress.
 * @retval  OT_ERROR_ABORT             A DNS transaction was aborted by stack.
 * @retval  OT_ERROR_RESPONSE_TIMEOUT  No DNS response has been received within timeout.
 * @retval  OT_ERROR_NOT_FOUND         A response was received but no IPv6 address has been found.
 * @retval  OT_ERROR_FAILED            A response was received but status code is different than success.
 *
 */
typedef void (*otDnsResponseHandler)(void *        aContext,
                                     const char *  aHostname,
                                     otIp6Address *aAddress,
                                     uint32_t      aTtl,
                                     otError       aResult);

/**
 * This function sends a DNS query for AAAA (IPv6) record.
 *
 * This function is available only if feature `OPENTHREAD_ENABLE_DNS_CLIENT` is enabled.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aQuery      A pointer to specify DNS query parameters.
 * @param[in]  aHandler    A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext    A pointer to arbitrary context information.
 *
 */
otError otDnsClientQuery(otInstance *         aInstance,
                         const otDnsQuery *   aQuery,
                         otDnsResponseHandler aHandler,
                         void *               aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DNS_H_
