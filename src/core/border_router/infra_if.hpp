/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#ifndef INFRA_IF_HPP_
#define INFRA_IF_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <openthread/platform/infra_if.h>

#include "common/data.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/string.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace BorderRouter {

/**
 * This class represents the infrastructure network interface on a border router.
 *
 */
class InfraIf : public InstanceLocator
{
public:
    static constexpr uint16_t kInfoStringSize = 20; ///< Max chars for the info string (`ToString()`).

    typedef String<kInfoStringSize> InfoString;  ///< String type returned from `ToString()`.
    typedef Data<kWithUint16Length> Icmp6Packet; ///< An IMCPv6 packet (data containing the IP payload)

    /**
     * This constructor initializes the `InfraIf`.
     *
     * @param[in]  aInstance  A OpenThread instance.
     *
     */
    explicit InfraIf(Instance &aInstance);

    /**
     * This method initializes the `InfraIf`.
     *
     * @param[in]  aIfIndex        The infrastructure interface index.
     *
     * @retval  kErrorNone         Successfully initialized the `InfraIf`.
     * @retval  kErrorInvalidArgs  The index of the infra interface is not valid.
     * @retval  kErrorInvalidState The `InfraIf` is already initialized.
     *
     */
    Error Init(uint32_t aIfIndex);

    /**
     * This method deinitilaizes the `InfraIf`.
     *
     */
    void Deinit(void);

    /**
     * This method indicates whether or not the `InfraIf` is initialized.
     *
     * @retval TRUE    The `InfraIf` is initialized.
     * @retval FALSE   The `InfraIf` is not initialized.
     *
     */
    bool IsInitialized(void) const { return mInitialized; }

    /**
     * This method indicates whether or not the infra interface is running.
     *
     * @retval TRUE   The infrastructure interface is running.
     * @retval FALSE  The infrastructure interface is not running.
     *
     */
    bool IsRunning(void) const { return mIsRunning; }

    /**
     * This method returns the infrastructure interface index.
     *
     * @returns The interface index or zero if not initialized.
     *
     */
    uint32_t GetIfIndex(void) const { return mIfIndex; }

    /**
     * This method indicates whether or not the infra interface has the given IPv6 address assigned.
     *
     * This method MUST be used when interface is initialized.
     *
     * @param[in]  aAddress       The IPv6 address.
     *
     * @retval TRUE   The infrastructure interface has @p aAddress.
     * @retval FALSE  The infrastructure interface does not have @p aAddress.
     *
     */
    bool HasAddress(const Ip6::Address &aAddress);

    /**
     * This method sends an ICMPv6 Neighbor Discovery packet on the infrastructure interface.
     *
     * This method MUST be used when interface is initialized.
     *
     * @param[in]  aPacket        The ICMPv6 packet to send.
     * @param[in]  aDestination   The destination address.
     *
     * @retval kErrorNone    Successfully sent the ICMPv6 message.
     * @retval kErrorFailed  Failed to send the ICMPv6 message.
     *
     */
    Error Send(const Icmp6Packet &aPacket, const Ip6::Address &aDestination);

    /**
     * This method processes a received ICMPv6 Neighbor Discovery packet from an infrastructure interface.
     *
     * @param[in]  aIfIndex       The infrastructure interface index on which the ICMPv6 message is received.
     * @param[in]  aSource        The IPv6 source address.
     * @param[in]  aPacket        The ICMPv6 packet.
     *
     */
    void HandledReceived(uint32_t aIfIndex, const Ip6::Address &aSource, const Icmp6Packet &aPacket);

    /**
     * This method handles infrastructure interface state changes.
     *
     * @param[in]  aIfIndex         The infrastructure interface index.
     * @param[in]  aIsRunning       A boolean that indicates whether the infrastructure interface is running.
     *
     * @retval  kErrorNone          Successfully updated the infra interface status.
     * @retval  kErrorInvalidState  The `InfraIf` is not initialized.
     * @retval  kErrorInvalidArgs   The @p IfIndex does not match the interface index of `InfraIf`.
     *
     */
    Error HandleStateChanged(uint32_t aIfIndex, bool aIsRunning);

    /**
     * This method converts the `InfraIf` to a human-readable string.
     *
     * @returns The string representation of `InfraIf`.
     *
     */
    InfoString ToString(void) const;

private:
    bool     mInitialized : 1;
    bool     mIsRunning : 1;
    uint32_t mIfIndex;
};

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // INFRA_IF_HPP_
