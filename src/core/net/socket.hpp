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
 *   This file includes definitions for IPv6 sockets.
 */

#ifndef NET_SOCKET_HPP_
#define NET_SOCKET_HPP_

#include "openthread-core-config.h"

#include "net/ip6_address.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-ip6
 *
 * @{
 *
 */

/**
 * This class implements message information for an IPv6 message.
 *
 */
class MessageInfo : public otMessageInfo
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    MessageInfo(void) { memset(this, 0, sizeof(*this)); }

    /**
     * This method returns a reference to the local socket address.
     *
     * @returns A reference to the local socket address.
     *
     */
    const Address &GetSockAddr(void) const { return *static_cast<const Address *>(&mSockAddr); }

    /**
     * This method sets the local socket address.
     *
     * @param[in]  aAddress  The IPv6 address.
     *
     */
    void SetSockAddr(const Address &aAddress) { mSockAddr = aAddress; }

    /**
     * This method gets the local socket port.
     *
     * @returns The local socket port.
     *
     */
    uint16_t GetSockPort(void) const { return mSockPort; }

    /**
     * This method gets the local socket port.
     *
     * @param[in]  aPort  The port value.
     *
     */
    void SetSockPort(uint16_t aPort) { mSockPort = aPort; }

    /**
     * This method returns a reference to the peer socket address.
     *
     * @returns A reference to the peer socket address.
     *
     */
    Address &GetPeerAddr(void) { return *static_cast<Address *>(&mPeerAddr); }

    /**
     * This method returns a reference to the peer socket address.
     *
     * @returns A reference to the peer socket address.
     *
     */
    const Address &GetPeerAddr(void) const { return *static_cast<const Address *>(&mPeerAddr); }

    /**
     * This method sets the peer's socket address.
     *
     * @param[in]  aAddress  The IPv6 address.
     *
     */
    void SetPeerAddr(const Address &aAddress) { mPeerAddr = aAddress; }

    /**
     * This method gets the peer socket port.
     *
     * @returns The peer socket port.
     *
     */
    uint16_t GetPeerPort(void) const { return mPeerPort; }

    /**
     * This method gets the peer socket port.
     *
     * @param[in]  aPort  The port value.
     *
     */
    void SetPeerPort(uint16_t aPort) { mPeerPort = aPort; }

    /**
     * This method gets the Interface ID.
     *
     * @returns The Interface ID.
     *
     */
    int8_t GetInterfaceId(void) const { return mInterfaceId; }

    /**
     * This method sets the Interface ID.
     *
     * @param[in]  aInterfaceId  The Interface ID.
     *
     */
    void SetInterfaceId(int8_t aInterfaceId) { mInterfaceId = aInterfaceId; }

    /**
     * This method gets the Hop Limit.
     *
     * @returns The Hop Limit.
     *
     */
    uint8_t GetHopLimit(void) const { return mHopLimit; }

    /**
     * This method sets the Hop Limit.
     *
     * @param[in]  aHopLimit  The Hop Limit.
     *
     */
    void SetHopLimit(uint8_t aHopLimit) { mHopLimit = aHopLimit; }

    /**
     * This method returns a pointer to the Link Info.
     *
     * @returns A pointer to the Link Info.
     *
     */
    const void *GetLinkInfo(void) const { return mLinkInfo; }

    /**
     * This method sets the pointer to the Link Info.
     *
     * @param[in]  aLinkInfo  A pointer to the Link Info.
     *
     */
    void SetLinkInfo(const void *aLinkInfo) { mLinkInfo = aLinkInfo; }
};

/**
 * This class implements a socket address.
 *
 */
class SockAddr : public otSockAddr
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    SockAddr(void) { memset(this, 0, sizeof(*this)); }

    /**
     * This method returns a reference to the IPv6 address.
     *
     * @returns A reference to the IPv6 address.
     *
     */
    Address &GetAddress(void) { return *static_cast<Address *>(&mAddress); }

    /**
     * This method returns a reference to the IPv6 address.
     *
     * @returns A reference to the IPv6 address.
     *
     */
    const Address &GetAddress(void) const { return *static_cast<const Address *>(&mAddress); }
};

/**
 * @}
 */

} // namespace Ip6
} // namespace ot

#endif // NET_SOCKET_HPP_
