/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file defines the platform DNS interface.
 *
 */

#ifndef OPENTHREAD_PLATFORM_DNS_H_
#define OPENTHREAD_PLATFORM_DNS_H_

#include <openthread/instance.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-dns
 *
 * @brief
 *   This module includes the platform abstraction for sending recursive DNS query to upstream DNS servers.
 *
 * @{
 *
 */

/**
 * This opaque type represents an upstream DNS query transaction.
 *
 */
typedef struct otPlatDnsUpstreamQuery otPlatDnsUpstreamQuery;

/**
 * Starts an upstream query transaction.
 *
 * - In success case (and errors represented by DNS protocol messages), the platform is expected to call
 *   `otPlatDnsUpstreamQueryDone`.
 * - The OpenThread core may cancel a (possibly timeout) query transaction by calling
 *   `otPlatDnsCancelUpstreamQuery`, the platform must not call `otPlatDnsUpstreamQueryDone` on a
 *   cancelled transaction.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 * @param[in] aQuery     A message buffer of the DNS payload that should be sent to upstream DNS server.
 *
 */
void otPlatDnsStartUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery);

/**
 * Cancels a transaction of upstream query.
 *
 * The platform must call `otPlatDnsUpstreamQueryDone` to release the resources.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 *
 */
void otPlatDnsCancelUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn);

/**
 * The platform calls this function to finish DNS query.
 *
 * The transaction will be released, so the platform must not call on the same transaction twice. This function passes
 * the ownership of `aResponse` to OpenThread stack.
 *
 * Platform can pass a nullptr to close a transaction without a response.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 * @param[in] aResponse  A message buffer of the DNS response payload or `nullptr` to close a transaction without a
 *                       response.
 *
 */
extern void otPlatDnsUpstreamQueryDone(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, otMessage *aResponse);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif
