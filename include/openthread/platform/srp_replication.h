/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes the platform abstraction for SRP Replication Protocol (SRPL).
 */

#ifndef OPENTHREAD_PLATFORM_SRP_REPLICATION_H_
#define OPENTHREAD_PLATFORM_SRP_REPLICATION_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents an SRPL partner info discovered using DNS-SD browse on the service name "_srpl-tls._tcp".
 *
 */
typedef struct otPlatSrplPartnerInfo
{
    /**
     * This boolean flag indicates whether the entry is being removed or added.
     *
     * - TRUE indicates that peer is removed.
     * - FALSE indicates that it is a new entry or an update to an existing entry.
     *
     */
    bool mRemoved;

    /**
     * The TXT record data (encoded as specified by DSN-SD) from the SRV record of the discovered service.
     *
     */
    const uint8_t *mTxtData;

    uint16_t mTxtLength; ///< Number of bytes in @p mTxtData buffer.

    /**
     * The partner socket address (IPv6 address and port number).
     *
     * The port number is determined from the SRV record of the discovered SRPL service instance. The IPv6 address
     * is determined from the DNS-SD query for A/AAAA records on the hostname indicated in the SRV record of the
     * discovered service instance. If multiple host IPv6 addressees are discovered, one with highest scope is used.
     *
     */
    otSockAddr mSockAddr;
} otPlatSrplPartnerInfo;

/**
 * This function starts or stops DNS-SD browse to discover SRPL partners within the local domain.
 *
 * On start platform layer MUST initiate an ongoing DNS-SD browse on the service name "_srpl-tls._tcp" within the local
 * browsing domain to discover SRPL partners. The ongoing browse will produce two different types of events:
 * "add" events and "remove" events.  When the browse is started, it should produce an "add" event for every SRPL
 * partner currently present on the network.  Whenever a partner goes offline, a "remove" event should be produced.
 * remove"  events are not guaranteed, however.
 *
 * When an SRP partner is discovered, a new ongoing DNS-SD query for A/AAAA record MUST be started on the hostname
 * indicated in the SRV record of the discovered entry. If multiple host IPv6 addressees are discovered for a partner,
 * one with highest scope among all addresses MUST be reported (if there are multiple address at same scope, one must
 * be selected randomly).
 *
 * SRPL platform MUST signal back the discovered partner info using `otPlatSrplHandleDnssdBrowseResult()` callback.
 * This callback MUST be invoked when a new partner is added or removed. If there is a change to TXT record of an
 * already discovered/added service on an SRP partner, then platform MUST call `otPlatSrplHandleDnssdBrowseResult()`
 * with new TXT record info. If the IPv6 address of the an already discovered/added service changes, then the platform
 * MUST first call `otPlatSrplHandleDnssdBrowseResult()` to remove the old address, before calling it again to add
 * the new address.
 *
 * SRPL platform MUST NOT invoke `otPlatSrplHandleDnssdBrowseResult()` for the SRPL service instance that is registered
 * by the device itself. This may be realized by checking the service instance's IPv6 address against its own addresses.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aEnable    TRUE to start DNS-SD browse, FALSE to stop it.
 *
 */
void otPlatSrplDnssdBrowse(otInstance *aInstance, bool aEnable);

/**
 * This is a callback function from platform layer to report a discovered SRPL partner info.
 *
 * This callback MUST be called only when DNS-SD browse for SRPL is enabled, `otPlatSrplDnssdBrowse()` is called with
 * `aEnable` being TRUE.  Please see `otPlatSrplDnssdBrowse()` for more details on when/how the callback MUST be
 * invoked on different events.
 *
 * @note The @p aPartnerInfo structure and its content (e.g., the `mTxtData` buffer) does not need to persist after
 * returning from this call. OpenThread code will make a copy of all the info it needs.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aPartnerInfo   The SRPL partner info.
 *
 */
extern void otPlatSrplHandleDnssdBrowseResult(otInstance *aInstance, const otPlatSrplPartnerInfo *aPartnerInfo);

/**
 * This function registers an SRP replication service to be advertised using DNS-SD.
 *
 * The service name is "_srpl-tls._tcp". The platform should choose its own hostname, which when combined with the
 * service name and the local DNS-SD domain name will produce the full service instance name, for example
 * "example-host._srpl-tls._tcp.local.".
 *
 * The domain under which the service instance name appears will be 'local' for mDNS, and will be whatever domain is
 * used for service registration in the case of a non-mDNS local DNS-SD service.
 *
 * SRP replication uses DNS port 853. So the SRV record for advertised SRPL service instance MUST include this port.
 *
 * A subsequent call to this function updates the previous service. For example, it can be used to update the TXT
 * record data.
 *
 * The @p aTxtData buffer is not persisted after returning from this function. The platform layer MUST NOT keep the
 * pointer and instead copy the content if needed.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aTxtData    A pointer to the TXT record data (encoded) to be include in the advertised service.
 * @param[in] aTxtLength  The length of @p aTxtData (number of bytes).
 *
 *
 */
void otPlatSrplRegisterDnssdService(otInstance *aInstance, const uint8_t *aTxtData, uint16_t aTxtLength);

/**
 * This function unregisters a previously registered SRPL service (if any) and stops its advertisement using DNS-SD.
 *
 * @param[in] aInstance    The OpenThread instance.
 *
 */
void otPlatSrplUnregisterDnssdService(otInstance *aInstance);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_SRP_REPLICATION_H_
