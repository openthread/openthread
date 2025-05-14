/*
 *  Copyright (c) 2019-2025, The OpenThread Authors.
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
 *   This file includes definitions for Thread Radio Encapsulation Link (TREL) peer discovery.
 */

#ifndef TREL_PEER_DISCOVERER_HPP_
#define TREL_PEER_DISCOVERER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include <openthread/platform/trel.h>

#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "radio/trel_peer.hpp"

namespace ot {
namespace Trel {

class Link;

extern "C" void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);

/**
 * Represents a TREL module responsible for peer discovery and mDNS service registration.
 */
class PeerDiscoverer : public InstanceLocator
{
    friend class Link;
    friend void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);

public:
    /**
     * Starts the peer discovery.
     */
    void Start(void);

    /**
     * Stops the peer discovery and clears the peer table.
     */
    void Stop(void);

    /**
     * Notifies that device's Extended MAC Address has changed.
     */
    void HandleExtAddressChange(void) { PostRegisterServiceTask(); }

    /**
     * Notifies that device's Extended PAN Identifier has changed.
     */
    void HandleExtPanIdChange(void) { PostRegisterServiceTask(); }

    /**
     * Notifies that a TREL packet is received from a peer using a different socket address than the one reported
     * earlier.
     *
     * @param[in] aPeerSockAddr   The previously reported peer sock address.
     * @param[in] aRxSockAddr     The address of received packet from the same peer.
     */
    void NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr, const Ip6::SockAddr &aRxSockAddr);

private:
    static const char kTxtRecordExtAddressKey[];
    static const char kTxtRecordExtPanIdKey[];

    struct PeerInfo : public otPlatTrelPeerInfo
    {
        bool                 IsRemoved(void) const { return mRemoved; }
        const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }
        Error                ParseTxtData(Mac::ExtAddress &aExtAddress, MeshCoP::ExtendedPanId &aExtPanId) const;
    };

    explicit PeerDiscoverer(Instance &aInstance);

    void HandleDiscoveredPeerInfo(const PeerInfo &aInfo);
    void PostRegisterServiceTask(void);
    void RegisterService(void);

    using RegisterServiceTask = TaskletIn<PeerDiscoverer, &PeerDiscoverer::RegisterService>;

    bool                mIsRunning;
    RegisterServiceTask mRegisterServiceTask;
};

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // TREL_PEER_DISCOVERER_HPP_
