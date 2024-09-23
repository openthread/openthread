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
 *   This file implements the infrastructure interface for posix.
 */

#ifndef OT_POSIX_PLATFORM_INFRA_IF_HPP_
#define OT_POSIX_PLATFORM_INFRA_IF_HPP_

#include "openthread-posix-config.h"

#include <net/if.h>
#include <openthread/nat64.h>
#include <openthread/openthread-system.h>

#include "core/common/non_copyable.hpp"

#include "logger.hpp"
#include "mainloop.hpp"
#include "multicast_routing.hpp"

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE

namespace ot {
namespace Posix {

/**
 * Manages infrastructure network interface.
 */
class InfraNetif : public Mainloop::Source, public Logger<InfraNetif>, private NonCopyable
{
public:
    static const char kLogModuleName[]; ///< Module name used for logging.

    /**
     * Updates the fd_set and timeout for mainloop.
     *
     * @param[in,out]   aContext    A reference to the mainloop context.
     */
    void Update(otSysMainloopContext &aContext) override;

    /**
     * Performs infrastructure network interface processing.
     *
     * @param[in]   aContext   A reference to the mainloop context.
     */
    void Process(const otSysMainloopContext &aContext) override;

    /**
     * Initializes the infrastructure network interface.
     *
     * To specify the infrastructure network interface, you need to call SetInfraNetif() after Init().
     *
     * @note This method is called before OpenThread instance is created.
     */
    void Init(void);

    /**
     * Sets the infrastructure network interface.
     *
     * @param[in]  aIfName       A pointer to infrastructure network interface name.
     * @param[in]  aIcmp6Socket  A SOCK_RAW socket for sending/receiving ICMPv6 messages. If you don't need border
     *                           routing feature, you can pass in -1.
     */
    void SetInfraNetif(const char *aIfName, int aIcmp6Socket);

    /**
     * Sets up the infrastructure network interface.
     *
     * @note This method is called after OpenThread instance is created.
     */
    void SetUp(void);

    /**
     * Tears down the infrastructure network interface.
     *
     * @note This method is called before OpenThread instance is destructed.
     */
    void TearDown(void);

    /**
     * Deinitializes the infrastructure network interface.
     *
     * @note This method is called after OpenThread instance is destructed.
     */
    void Deinit(void);

    /**
     * Checks whether the infrastructure network interface is running.
     */
    bool IsRunning(void) const;

    /**
     * Returns the ifr_flags of the infrastructure network interface.
     *
     * @returns The ifr_flags of the infrastructure network interface.
     */
    uint32_t GetFlags(void) const;

    /**
     * This functions counts the number of addresses on the infrastructure network interface.
     *
     * @param[out] aAddressCounters  The counters of addresses on infrastructure network interface.
     */
    void CountAddresses(otSysInfraNetIfAddressCounters &aAddressCounters) const;

    /**
     * Handles the backbone state change events.
     *
     * @param[in] aInstance  A pointer to the OpenThread instance.
     * @param[in] aFlags     Flags that denote the state change events.
     */
    void HandleBackboneStateChange(otInstance *aInstance, otChangedFlags aFlags);

    /**
     * Sends an ICMPv6 Neighbor Discovery message on given infrastructure interface.
     *
     * See RFC 4861: https://tools.ietf.org/html/rfc4861.
     *
     * @param[in]  aInfraIfIndex  The index of the infrastructure interface this message is sent to.
     * @param[in]  aDestAddress   The destination address this message is sent to.
     * @param[in]  aBuffer        The ICMPv6 message buffer. The ICMPv6 checksum is left zero and the
     *                            platform should do the checksum calculate.
     * @param[in]  aBufferLength  The length of the message buffer.
     *
     * @note  Per RFC 4861, the implementation should send the message with IPv6 link-local source address
     *        of interface @p aInfraIfIndex and IP Hop Limit 255.
     *
     * @retval OT_ERROR_NONE    Successfully sent the ICMPv6 message.
     * @retval OT_ERROR_FAILED  Failed to send the ICMPv6 message.
     */
    otError SendIcmp6Nd(uint32_t            aInfraIfIndex,
                        const otIp6Address &aDestAddress,
                        const uint8_t      *aBuffer,
                        uint16_t            aBufferLength);

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE && OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
    /**
     * Sends an asynchronous address lookup for the well-known host name "ipv4only.arpa"
     * to discover the NAT64 prefix.
     *
     * @param[in]  aInfraIfIndex  The index of the infrastructure interface the address look-up is sent to.
     *
     * @retval  OT_ERROR_NONE    Successfully request address look-up.
     * @retval  OT_ERROR_FAILED  Failed to request address look-up.
     */
    otError DiscoverNat64Prefix(uint32_t aInfraIfIndex);
#endif

    /**
     * Gets the infrastructure network interface name.
     *
     * @returns The infrastructure network interface name, or `nullptr` if not specified.
     */
    const char *GetNetifName(void) const { return (mInfraIfIndex != 0) ? mInfraIfName : nullptr; }

    /**
     * Gets the infrastructure network interface index.
     *
     * @returns The infrastructure network interface index.
     */
    uint32_t GetNetifIndex(void) const { return mInfraIfIndex; }

    /**
     * Gets the infrastructure network interface singleton.
     *
     * @returns The singleton object.
     */
    static InfraNetif &Get(void);

    /**
     * Creates a socket for sending/receiving ICMPv6 messages.
     *
     * @param[in] aInfraIfName  The infrastructure network interface name.
     *
     * @returns The file descriptor of the socket.
     */
    static int CreateIcmp6Socket(const char *aInfraIfName);

private:
    static const char         kWellKnownIpv4OnlyName[];   // "ipv4only.arpa"
    static const otIp4Address kWellKnownIpv4OnlyAddress1; // 192.0.0.170
    static const otIp4Address kWellKnownIpv4OnlyAddress2; // 192.0.0.171
    static const uint8_t      kValidNat64PrefixLength[];

    char     mInfraIfName[IFNAMSIZ];
    uint32_t mInfraIfIndex = 0;

#ifdef __linux__
    int mNetLinkSocket = -1;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    int mInfraIfIcmp6Socket = -1;
#endif
#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    MulticastRoutingManager mMulticastRoutingManager;
#endif

    bool HasLinkLocalAddress(void) const;

#ifdef __linux__
    void ReceiveNetLinkMessage(void);
#endif

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE && OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
#ifdef __linux__
    static void DiscoverNat64PrefixDone(union sigval sv);
#endif // #ifdef __linux__
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    void SetInfraNetifIcmp6SocketForBorderRouting(int aIcmp6Socket);
    void ReceiveIcmp6Message(void);
#endif
};

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE

#endif // OT_POSIX_PLATFORM_INFRA_IF_HPP_
