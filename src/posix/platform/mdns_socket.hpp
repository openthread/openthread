/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
#ifndef OT_POSIX_PLATFORM_MDNS_SOCKET_HPP_
#define OT_POSIX_PLATFORM_MDNS_SOCKET_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#include <openthread/nat64.h>
#include <openthread/platform/mdns_socket.h>

#include "logger.hpp"
#include "mainloop.hpp"

#include "core/common/non_copyable.hpp"

namespace ot {
namespace Posix {

/**
 * Implements platform mDNS socket APIs.
 */
class MdnsSocket : public Mainloop::Source, public Logger<MdnsSocket>, private NonCopyable
{
public:
    static const char kLogModuleName[]; ///< Module name used for logging.

    /**
     * Gets the `MdnsSocket` singleton.
     *
     * @returns The singleton object.
     */
    static MdnsSocket &Get(void);

    /**
     * Initializes the `MdnsSocket`.
     *
     * Called before OpenThread instance is created.
     */
    void Init(void);

    /**
     * Sets up the `MdnsSocket`.
     *
     * Called after OpenThread instance is created.
     */
    void SetUp(void);

    /**
     * Tears down the `MdnsSocket`.
     *
     * Called before OpenThread instance is destructed.
     */
    void TearDown(void);

    /**
     * Deinitializes the `MdnsSocket`.
     *
     * Called after OpenThread instance is destructed.
     */
    void Deinit(void);

    /**
     * Updates the fd_set and timeout for mainloop.
     *
     * @param[in,out]   aContext    A reference to the mainloop context.
     */
    void Update(otSysMainloopContext &aContext) override;

    /**
     * Performs `MdnsSocket` processing.
     *
     * @param[in]   aContext   A reference to the mainloop context.
     */
    void Process(const otSysMainloopContext &aContext) override;

    // otPlatMdns APIs
    otError SetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex);
    void    SendMulticast(otMessage *aMessage, uint32_t aInfraIfIndex);
    void    SendUnicast(otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress);

private:
    static constexpr uint16_t kMaxMessageLength = 2000;
    static constexpr uint16_t kMdnsPort         = 5353;

    enum MsgType : uint8_t
    {
        kIp6Msg,
        kIp4Msg,
    };

    struct Metadata
    {
        bool CanFreeMessage(void) const { return (mIp6Port == 0) && (mIp4Port == 0); }

        otIp6Address mIp6Address;
        uint16_t     mIp6Port;
        otIp4Address mIp4Address;
        uint16_t     mIp4Port;
    };

    bool           mEnabled;
    uint32_t       mInfraIfIndex;
    int            mFd4;
    int            mFd6;
    uint32_t       mPendingIp6Tx;
    uint32_t       mPendingIp4Tx;
    otMessageQueue mTxQueue;
    otIp6Address   mMulticastIp6Address;
    otIp4Address   mMulticastIp4Address;
    otInstance    *mInstance;

    otError Enable(uint32_t aInfraIfIndex);
    void    Disable(uint32_t aInfraIfIndex);
    void    ClearTxQueue(void);
    void    SendQueuedMessages(MsgType aMsgType);
    void    ReceiveMessage(MsgType aMsgType);

    otError OpenIp4Socket(uint32_t aInfraIfIndex);
    otError JoinOrLeaveIp4MulticastGroup(bool aJoin, uint32_t aInfraIfIndex);
    void    CloseIp4Socket(void);
    otError OpenIp6Socket(uint32_t aInfraIfIndex);
    otError JoinOrLeaveIp6MulticastGroup(bool aJoin, uint32_t aInfraIfIndex);
    void    CloseIp6Socket(void);

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

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#endif // OT_POSIX_PLATFORM_MDNS_SOCKET_HPP_
