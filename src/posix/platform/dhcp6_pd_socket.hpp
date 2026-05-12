/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
#ifndef OT_POSIX_PLATFORM_DHCP6_PD_SOCKET_HPP_
#define OT_POSIX_PLATFORM_DHCP6_PD_SOCKET_HPP_

#include "openthread-posix-config.h"

#ifdef OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE
#error "OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE                                                      \
    (OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE && \
     OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE)

#if OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE

#include <openthread/ip6.h>

#include "logger.hpp"
#include "mainloop.hpp"

#include "core/common/non_copyable.hpp"

namespace ot {
namespace Posix {

/**
 * Implements platform infra-if DHCPv6 Prefix Delegation (PD) socket APIs.
 *
 * This is sub-component of `ot::Posix::InfraNetif` class.
 */
class Dhcp6PdSocket : public Logger<Dhcp6PdSocket>, private NonCopyable
{
public:
    static const char kLogModuleName[]; ///< Module name used for logging.

    /**
     * Initializes the `Dhcp6PdSocket`.
     *
     * Called before OpenThread instance is created.
     */
    void Init(void);

    /**
     * Sets up the `Dhcp6PdSocket`.
     *
     * Called after OpenThread instance is created.
     */
    void SetUp(void);

    /**
     * Tears down the `Dhcp6PdSocket`.
     *
     * Called before OpenThread instance is destructed.
     */
    void TearDown(void);

    /**
     * Deinitializes the `Dhcp6PdSocket`.
     *
     * Called after OpenThread instance is destructed.
     */
    void Deinit(void);

    /**
     * Updates the fd_set and timeout for mainloop.
     *
     * @param[in,out]   aContext    A reference to the mainloop context.
     */
    void Update(otSysMainloopContext &aContext);

    /**
     * Performs `Dhcp6PdSocket` processing.
     *
     * @param[in]   aContext   A reference to the mainloop context.
     */
    void Process(const otSysMainloopContext &aContext);

    // otPlatInfraIfDhcp6PdClient APIs
    void SetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex);
    void Send(otMessage *aMessage, const otIp6Address *aDstAddress, uint32_t aInfraIfIndex);

private:
    static constexpr uint16_t kMaxMessageLength = 2000;
    static constexpr uint16_t kClientPort       = 546;
    static constexpr uint16_t kServerPort       = 547;

    struct Metadata
    {
        otIp6Address mAddress;
        uint16_t     mPort;
    };

    bool           mEnabled;
    bool           mPendingTx;
    uint32_t       mInfraIfIndex;
    int            mFd6;
    otMessageQueue mTxQueue;
    otIp6Address   mMulticastAddress;
    otInstance    *mInstance;

    void Enable(uint32_t aInfraIfIndex);
    void Disable(uint32_t aInfraIfIndex);
    void ClearTxQueue(void);
    void SendQueuedMessages(void);
    void ReceiveMessage(void);

    otError OpenSocket(uint32_t aInfraIfIndex);
    otError JoinOrLeaveMulticastGroup(bool aJoin, uint32_t aInfraIfIndex);
    void    CloseSocket(void);

    static otError SetReuseAddrPortOptions(int aFd);

    template <typename ValueType>
    static otError SetSocketOption(int aFd, int aLevel, int aOption, const ValueType &aValue, const char *aOptionName)
    {
        return SetSocketOptionValue(aFd, aLevel, aOption, &aValue, sizeof(ValueType), aOptionName);
    }

    static otError SetSocketOptionValue(int         aFd,
                                        int         aLevel,
                                        int         aOption,
                                        const void *aValue,
                                        uint32_t    aValueLength,
                                        const char *aOptionName);
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE

#endif // OT_POSIX_PLATFORM_DHCP6_PD_SOCKET_HPP_
