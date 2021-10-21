/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes the platform abstraction for Thread Radio Encapsulation Link (TREL) using DNS-SD and UDP/IPv6.
 *
 */

#ifndef OPENTHREAD_PLATFORM_TREL_H_
#define OPENTHREAD_PLATFORM_TREL_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-trel
 *
 * @brief
 *   This module includes the platform abstraction for Thread Radio Encapsulation Link (TREL) using DNS-SD and
 *   UDP/IPv6.
 *
 * @{
 *
 */

/**
 * This function initializes and enables TREL platform layer.
 *
 * Upon this call, the platform layer MUST perform the following:
 *
 * 1) TREL platform layer MUST open a UDP socket to listen for and receive TREL messages from peers. The socket is
 * bound to an ephemeral port number chosen by the platform layer. The port number MUST be returned in @p aUdpPort.
 * The socket is also bound to network interface(s) on which TREL is to be supported. The socket and the chosen port
 * should stay valid while TREL is enabled.
 *
 * 2) Platform layer MUST initiate an ongoing DNS-SD browse on the service name "_trel._udp" within the local browsing
 * domain to discover other devices supporting TREL. The ongoing browse will produce two different types of events:
 * "add" events and "remove" events.  When the browse is started, it should produce an "add" event for every TREL peer
 * currently present on the network.  Whenever a TREL peer goes offline, a "remove" event should be produced. "remove"
 * events are not guaranteed, however. When a TREL service instance is discovered, a new ongoing DNS-SD query for an
 * AAAA record should be started on the hostname indicated in the SRV record of the discovered instance. If multiple
 * host IPv6 addressees are discovered for a peer, one with highest scope among all addresses MUST be reported (if
 * there are multiple address at same scope, one must be selected randomly).
 *
 * TREL platform MUST signal back the discovered peer info using `otPlatTrelHandleDiscoveredPeerInfo()` callback. This
 * callback MUST be invoked when a new peer is discovered, when there is a change in an existing entry (e.g., new
 * TXT record or new port number or new IPv6 address), or when the peer is removed.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aUdpPort   A pointer to return the selected port number by platform layer.
 *
 */
void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort);

/**
 * This function disables TREL platform layer.
 *
 * After this call, the platform layer MUST stop DNS-SD browse on the service name "_trel._udp", stop advertising the
 * TREL DNS-SD service (from `otPlatTrelRegisterService()`) and MUST close the UDP socket used to receive TREL messages.
 *
 * @pram[in]  aInstance  The OpenThread instance.
 *
 */
void otPlatTrelDisable(otInstance *aInstance);

/**
 * This structure represents a TREL peer info discovered using DNS-SD browse on the service name "_trel._udp".
 *
 */
typedef struct otPlatTrelPeerInfo
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
     * The TXT record data (encoded as specified by DNS-SD) from the SRV record of the discovered TREL peer service
     * instance.
     *
     */
    const uint8_t *mTxtData;

    uint16_t mTxtLength; ///< Number of bytes in @p mTxtData buffer.

    /**
     * The TREL peer socket address (IPv6 address and port number).
     *
     * The port number is determined from the SRV record of the discovered TREL peer service instance. The IPv6 address
     * is determined from the DNS-SD query for AAAA records on the hostname indicated in the SRV record of the
     * discovered service instance. If multiple host IPv6 addressees are discovered, one with highest scope is used.
     *
     */
    otSockAddr mSockAddr;
} otPlatTrelPeerInfo;

/**
 * This is a callback function from platform layer to report a discovered TREL peer info.
 *
 * @note The @p aInfo structure and its content (e.g., the `mTxtData` buffer) does not need to persist after returning
 * from this call. OpenThread code will make a copy of all the info it needs.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aInfo       A pointer to the TREL peer info.
 *
 */
extern void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);

/**
 * This function registers a new service to be advertised using DNS-SD [RFC6763].
 *
 * The service name is "_trel._udp". The platform should use its own hostname, which when combined with the service
 * name and the local DNS-SD domain name will produce the full service instance name, for example
 * "example-host._trel._udp.local.".
 *
 * The domain under which the service instance name appears will be 'local' for mDNS, and will be whatever domain is
 * used for service registration in the case of a non-mDNS local DNS-SD service.
 *
 * A subsequent call to this function updates the previous service. It is used to update the TXT record data and/or the
 * port number.
 *
 * The @p aTxtData buffer is not persisted after the return from this function. The platform layer MUST NOT keep the
 * pointer and instead copy the content if needed.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aPort       The port number to include in the SRV record of the advertised service.
 * @param[in] aTxtData    A pointer to the TXT record data (encoded) to be include in the advertised service.
 * @param[in] aTxtLength  The length of @p aTxtData (number of bytes).
 *
 *
 */
void otPlatTrelRegisterService(otInstance *aInstance, uint16_t aPort, const uint8_t *aTxtData, uint8_t aTxtLength);

/**
 * This function requests a TREL UDP packet to be sent to a given destination.
 *
 * @param[in] aInstance        The OpenThread instance structure.
 * @param[in] aUdpPayload      A pointer to UDP payload.
 * @param[in] aUdpPayloadLen   The payload length (number of bytes).
 * @param[in] aDestSockAddr    The destination socket address.
 *
 */
void otPlatTrelSend(otInstance *      aInstance,
                    const uint8_t *   aUdpPayload,
                    uint16_t          aUdpPayloadLen,
                    const otSockAddr *aDestSockAddr);

/**
 * This function is a callback from platform to notify of a received TREL UDP packet.
 *
 * @note The buffer content (up to its specified length) may get changed during processing by OpenThread core (e.g.,
 * decrypted in place), so the platform implementation should expect that after returning from this function the
 * @p aBuffer content may have been altered.
 *
 * @param[in] aInstance        The OpenThread instance structure.
 * @param[in] aBuffer          A buffer containing the received UDP payload.
 * @param[in] aLength          UDP payload length (number of bytes).
 *
 */
extern void otPlatTrelHandleReceived(otInstance *aInstance, uint8_t *aBuffer, uint16_t aLength);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_TREL_H_
