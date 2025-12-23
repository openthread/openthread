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

#include "common/bit_set.hpp"
#include "thread/neighbor.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE

/**
 * Represents a Thread Peer.
 */
class Peer : public CslNeighbor
{
public:
    /**
     * Initializes the `Peer` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

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
     * Sets whether the peer is a local SRP server.
     *
     * @param[in]  aIsLocalSrpServer   TRUE to set the peer s a local SRP server, FALSE otherwise.
     */
    void SetLocalSrpServer(bool aIsLocalSrpServer) { mIsLocalSrpServer = aIsLocalSrpServer; }

    /**
     * Indicates whether the peer is a local SRP server.
     *
     * @retval TRUE   The peer is a SRP server.
     * @retval FALSE  The peer is not a local SRP server.
     */
    bool IsLocalSrpServer(void) const { return mIsLocalSrpServer; }

private:
    bool mIsLocalSrpServer : 1;
};

#endif // OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE

} // namespace ot

#endif // PEER_HPP_
