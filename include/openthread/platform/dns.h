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
 * This opaque type represents a upstream DNS query transaction.
 *
 */
typedef void otPlatDnsUpstreamQuery;

/**
 * Starts a transaction of upstream query.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 * @param[in] aQuery     A message buffer of the DNS payload that should be sent to upstream DNS server.
 *
 */
void otPlatDnsQueryUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery);

/**
 * Cancels a transaction of upstream query.
 *
 * The platform must not call `otPlatDnsOnUpstreamQueryResponse` on the same transaction after this function is called.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 *
 */
void otPlatDnsCancelUpstreamQueryTransaction(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn);

/**
 * The radio driver calls this method to notify OpenThread of a completed DNS query.
 *
 * The transaction will be released, so the platform must not call on the same transaction twice. This function takes
 * the ownership of `aResponse`.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aTxn       A pointer to the opaque DNS query transaction object.
 * @param[in] aResponse  A message buffer of the DNS response payload.
 *
 */
extern void otPlatDnsOnUpstreamQueryResponse(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, otMessage *aResponse);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif