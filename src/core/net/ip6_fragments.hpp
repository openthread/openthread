/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for IPv6 packet fragmentation and reassembly.
 */

#ifndef OT_CORE_NET_IP6_FRAGMENTS_HPP_
#define OT_CORE_NET_IP6_FRAGMENTS_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "net/icmp6.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-fragments
 *
 * @brief
 *   IPv6 fragmentation (TX) and reassembly (RX).
 *
 * @{
 */

/**
 * Handles IPv6-level packet fragmentation and reassembly.
 */
class Fragments : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the `Fragments` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Fragments(Instance &aInstance);

    /**
     * Fragments an IPv6 datagram that exceeds `kMaxDatagramLength`.
     *
     * The parent datagram is retained until all IP fragments are transmitted or
     * a transmission error occurs. The TX done callback registered on the parent
     * message is invoked once with the final status.
     *
     * @param[in,out] aMessage  Full datagram used as the fragmentation parent.
     * @param[in]     aIpProto  Upper-layer protocol (e.g. `kProtoUdp`).
     *
     * @retval kErrorNone    The first fragment was enqueued.
     * @retval kErrorNoBufs  Could not allocate a fragment buffer.
     */
    Error FragmentDatagram(Message &aMessage, uint8_t aIpProto);

    /**
     * Handles direct transmission completion of an IPv6 fragment message.
     *
     * @param[in] aFragment  The transmitted IPv6 fragment message.
     * @param[in] aError     The transmission result from the mesh forwarder.
     */
    void HandleIpFragmentTxDone(Message &aFragment, Error aError);

    /**
     * Processes an IPv6 Fragment extension header.
     *
     * @param[in,out] aMessage  Datagram with cursor at FragmentHeader.
     *
     * @retval kErrorNone   Atomic fragment stripped; continue extension processing.
     * @retval kErrorDrop   Fragment consumed (buffered or dropped).
     * @retval kErrorNoBufs Reassembly buffer too small or allocation failed.
     */
    Error HandleFragment(Message &aMessage);

    /**
     * Periodic tick handler for reassembly timeouts.
     */
    void HandleTimeTick(void);

    /**
     * Clears all pending reassembly buffers.
     */
    void CleanupFragmentationBuffer(void);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Returns the reassembly queue (for diagnostics / testing).
     *
     * @returns A reference to the reassembly queue.
     */
    const MessageQueue &GetReassemblyQueue(void) const;
#endif

private:
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    static constexpr uint16_t kMinimalMtu           = 1280;
    static constexpr uint8_t  kIp6ReassemblyTimeout = OPENTHREAD_CONFIG_IP6_REASSEMBLY_TIMEOUT;

    Error SendIpFragment(Message &aParent);
    void  CompleteIp6FragTx(Message &aParent, Error aError);
    void  UpdateReassemblyList(void);
    void  SendIcmpError(Message &aMessage, Icmp6Header::Type aIcmpType, Icmp6Header::Code aIcmpCode);

    MessageQueue mReassemblyList;
#endif
};

/** @} */

} // namespace Ip6
} // namespace ot

#endif // OT_CORE_NET_IP6_FRAGMENTS_HPP_
