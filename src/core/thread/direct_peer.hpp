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
 *   This file includes definitions for a Thread Direct `DirectPeer`.
 */

#ifndef OT_CORE_THREAD_DIRECT_PEER_HPP_
#define OT_CORE_THREAD_DIRECT_PEER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include "common/random.hpp"
#include "mac/mac_header_ie.hpp"
#include "thread/neighbor.hpp"

namespace ot {

/**
 * Represents a Thread Direct peer and its link state established during the
 * TD handshake.
 */
class DirectPeer : public CslNeighbor
{
public:
    /**
     * Maximum number of re-transmitted TD link teardown frames.
     */
    static constexpr uint8_t kMaxRetransmitLinkTearDowns = 4;

    /**
     * Initializes the `DirectPeer` object.
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
     * Gets the link-local IPv6 address of the peer.
     *
     * @returns The link-local IPv6 address of the peer.
     */
    void GetLinkLocalIp6Address(Ip6::Address &aIp6Address) const
    {
        aIp6Address.InitAsLinkLocalAddress(GetExtAddress());
    }

    /**
     * Generates a new challenge value for the TD handshake.
     */
    void GenerateChallenge(void) { Random::NonCrypto::FillBuffer(mChallenge.mChallenge, Mac::ChallengeLtv::kLength); }

    /**
     * Gets the current challenge value.
     *
     * @returns The current challenge value.
     */
    const Mac::ChallengeLtv &GetChallenge(void) const { return mChallenge; }

    /**
     * Increments the count of re-transmitted link teardown frames.
     */
    void IncrementTearDownCount(void) { mTearDownCount++; }

    /**
     * Resets the teardown re-transmit count to zero.
     */
    void ResetTearDownCount(void) { mTearDownCount = 0; }

    /**
     * Returns the current teardown re-transmit count.
     */
    uint8_t GetTearDownCount(void) const { return mTearDownCount; }

private:
    Mac::ChallengeLtv mChallenge;

    uint8_t mTearDownCount : 3; // Re-transmitted teardown frame count.
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#endif // OT_CORE_THREAD_DIRECT_PEER_HPP_
