/*
 *  Copyright (c) 2019-2021, The OpenThread Authors.
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
 *   This file includes definitions for Thread Radio Encapsulation Link (TREL) interface.
 */

#ifndef TREL_INTERFACE_HPP_
#define TREL_INTERFACE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include <openthread/trel.h>
#include <openthread/platform/trel.h>

#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/pool.hpp"
#include "common/tasklet.hpp"
#include "common/time.hpp"
#include "radio/trel_packet.hpp"
#include "radio/trel_peer.hpp"

namespace ot {
namespace Trel {

class Link;
class Interface;

extern "C" void otPlatTrelHandleReceived(otInstance       *aInstance,
                                         uint8_t          *aBuffer,
                                         uint16_t          aLength,
                                         const otSockAddr *aSenderAddr);
extern "C" void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);

/**
 * Represents a group of TREL counters.
 */
typedef otTrelCounters Counters;

/**
 * Represents a TREL link interface.
 */
class Interface : public InstanceLocator
{
    friend class Link;
    friend void otPlatTrelHandleReceived(otInstance       *aInstance,
                                         uint8_t          *aBuffer,
                                         uint16_t          aLength,
                                         const otSockAddr *aSenderAddr);
    friend void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);

public:
    /**
     * Represents an iterator for iterating over TREL peer table entries.
     */
    typedef otTrelPeerIterator PeerIterator;

    /**
     * Enables or disables the TREL interface.
     *
     * @param[in] aEnable A boolean to enable/disable the TREL interface.
     */
    void SetEnabled(bool aEnable);

    /**
     * Enables the TREL interface.
     *
     * This call initiates an ongoing DNS-SD browse on the service name "_trel._udp" within the local browsing domain
     * to discover other devices supporting TREL. Device also registers a new service to be advertised using DNS-SD,
     * with the service name is "_trel._udp" indicating its support for TREL. Device is ready to receive TREL messages
     * from peers.
     */
    void Enable(void);

    /**
     * Disables the TREL interface.
     *
     * This call stops the DNS-SD browse on the service name "_trel._udp", stops advertising TREL DNS-SD service, and
     * clears the TREL peer table.
     */
    void Disable(void);

    /**
     * Indicates whether the TREL interface is enabled.
     *
     * @retval TRUE if the TREL interface is enabled.
     * @retval FALSE if the TREL interface is disabled.
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * Initializes a peer table iterator.
     *
     * @param[in] aIterator   The iterator to initialize.
     */
    void InitIterator(PeerIterator &aIterator) const { aIterator = mPeerList.GetHead(); }

    /**
     * Iterates over the peer table entries.
     *
     * @param[in] aIterator   The iterator. MUST be initialized.
     *
     * @returns A pointer to the next `Peer` entry or `nullptr` if no more entries in the table.
     */
    const Peer *GetNextPeer(PeerIterator &aIterator) const;

    /**
     * Returns the number of TREL peers.
     *
     * @returns  The number of TREL peers.
     */
    uint16_t GetNumberOfPeers(void) const;

    /**
     * Sets the filter mode (enables/disables filtering).
     *
     * When filtering is enabled, any rx and tx traffic through TREL interface is silently dropped. This is mainly
     * intended for use during testing.
     *
     * Unlike `Enable()/Disable()` which fully start/stop the TREL interface operation, when filter mode is enabled the
     * TREL interface continues to be enabled.
     *
     * @param[in] aFiltered  TRUE to enable filter mode, FALSE to disable filter mode.
     */
    void SetFilterEnabled(bool aEnable) { mFiltered = aEnable; }

    /**
     * Indicates whether or not the filter mode is enabled.
     *
     * @retval TRUE if the TREL filter mode is enabled.
     * @retval FALSE if the TREL filter mode is disabled.
     */
    bool IsFilterEnabled(void) const { return mFiltered; }

    /**
     * Gets the TREL counters.
     *
     * The counters are initialized to zero when the TREL platform is initialized.
     */
    const Counters *GetCounters(void) const;

    /**
     * Resets the TREL counters.
     */
    void ResetCounters(void);

    /**
     * Returns the TREL UDP port.
     *
     * @returns The TREL UDP port.
     */
    uint16_t GetUdpPort(void) const { return mUdpPort; }

    /**
     * Finds the TREL peer associated with a given Extended Address.
     *
     * @param[in] aExtAddress  The extended address.
     *
     * @returns The peer associated with @ aExtAddress, or `nullptr` if not found.
     */
    Peer *FindPeer(const Mac::ExtAddress &aExtAddress);

    /**
     * Notifies platform that a TREL packet is received from a peer using a different socket address than the one
     * reported earlier.
     *
     * @param[in] aPeerSockAddr   The previously reported peer sock addr.
     * @param[in] aRxSockAddr     The address of received packet from the same peer.
     */
    void NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr, const Ip6::SockAddr &aRxSockAddr);

private:
#if OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE != 0
    static constexpr uint16_t kPeerPoolSize = OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE;
#else
    static constexpr uint16_t kExtraEntries = 32;
    static constexpr uint16_t kPeerPoolSize = Mle::kMaxRouters + Mle::kMaxChildren + kExtraEntries;
#endif
    static const char kTxtRecordExtAddressKey[];
    static const char kTxtRecordExtPanIdKey[];

    struct PeerInfo : public otPlatTrelPeerInfo
    {
        bool                 IsRemoved(void) const { return mRemoved; }
        const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }
        Error                ParseTxtData(Mac::ExtAddress &aExtAddress, MeshCoP::ExtendedPanId &aExtPanId) const;
    };

    explicit Interface(Instance &aInstance);

    // Methods used by `Trel::Link`.
    void  Init(void);
    void  HandleExtAddressChange(void);
    void  HandleExtPanIdChange(void);
    Error Send(const Packet &aPacket, bool aIsDiscovery = false);

    // Callbacks from `otPlatTrel`.
    void HandleReceived(uint8_t *aBuffer, uint16_t aLength, const Ip6::SockAddr &aSenderAddr);
    void HandleDiscoveredPeerInfo(const PeerInfo &aInfo);

    void  RegisterService(void);
    Peer *GetNewPeerEntry(void);
    void  RemovePeerEntry(Peer &aEntry);
    void  ClearPeerList(void);

    using RegisterServiceTask = TaskletIn<Interface, &Interface::RegisterService>;

    bool                      mInitialized : 1;
    bool                      mEnabled : 1;
    bool                      mFiltered : 1;
    RegisterServiceTask       mRegisterServiceTask;
    uint16_t                  mUdpPort;
    Packet                    mRxPacket;
    LinkedList<Peer>          mPeerList;
    Pool<Peer, kPeerPoolSize> mPeerPool;
};

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // TREL_INTERFACE_HPP_
