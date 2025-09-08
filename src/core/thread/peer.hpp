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

/**
 * @file
 *   This file includes definitions for a Thread P2P `Peer`.
 */

#ifndef PEER_HPP_
#define PEER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_P2P_ENABLE

#include "thread/neighbor.hpp"

namespace ot {

/**
 * Represents a P2P Peer.
 */
class Peer : public CslNeighbor
{
public:
    /**
     * Max number of re-transmitted the P2P link tear down messages.
     */
    static constexpr uint8_t kMaxRetransmitLinkTearDowns = 4;

    /**
     * Initializes the `Peer` object.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    void Init(Instance &aInstance)
    {
        Neighbor::Init(aInstance);
        mTearDownCount = 0;
    }

    /**
     * Clears the peer entry.
     */
    void Clear(void);

    /**
     * Sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     */
    void SetDeviceMode(Mle::DeviceMode aMode);

    /**
     * Gets the link local IPv6 address of the peer.
     *
     * @returns The link local IPv6 address of the peer.
     */
    void GetLinkLocalIp6Address(Ip6::Address &aIp6Address) const { aIp6Address.SetToLinkLocalAddress(GetExtAddress()); }

    /**
     * Generates a new challenge value to use during a child attach.
     */
    void GenerateChallenge(void) { mAttachChallenge.GenerateRandom(); }

    /**
     * Gets the current challenge value used during attach.
     *
     * @returns The current challenge value.
     */
    const Mle::TxChallenge &GetChallenge(void) const { return mAttachChallenge; }

    /**
     * Increments the count of re-transmitted link tear down messages.
     */
    void IncrementTearDownCount(void) { mTearDownCount++; }

    /**
     * Resets the count of re-transmitted link tear down messages to zero.
     */
    void ResetTearDownCount(void) { mTearDownCount = 0; }

    /**
     * Returns the count of re-transmitted link tear down messages.
     */
    uint8_t GetTearDownCount(void) const { return mTearDownCount; }

private:
    Mle::TxChallenge mAttachChallenge;

    uint8_t mTearDownCount : 3; // The count of re-transmitted link tear down messages.
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_P2P_ENABLE

#endif // PEER_HPP_
