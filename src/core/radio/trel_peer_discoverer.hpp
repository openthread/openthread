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

#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "net/dns_types.hpp"
#include "net/dnssd.hpp"
#include "radio/trel_peer.hpp"

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

#if !(OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE)
#error "OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE requires either the native mDNS or platform DNS-SD APIs"
#endif

#if !OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
#error "OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE requires OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE"
#endif

#endif

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
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    friend class ot::Dnssd;
    friend class Peer;
#else
    friend void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo);
#endif

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
    void HandleExtAddressChange(void) { PostServiceTask(); }

    /**
     * Notifies that device's Extended PAN Identifier has changed.
     */
    void HandleExtPanIdChange(void) { PostServiceTask(); }

    /**
     * Notifies that a TREL packet is received from a peer using a different socket address than the one reported
     * earlier.
     *
     * @param[in] aPeerSockAddr   The previously reported peer sock address.
     * @param[in] aRxSockAddr     The address of received packet from the same peer.
     */
    void NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr, const Ip6::SockAddr &aRxSockAddr);

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    /**
     * Returns the TREL service name (service instance label) used by the device itself when advertising TREL service.
     *
     * @returns The TREL service name.
     */
    const char *GetServiceName(void) { return mServiceName.GetName(); }
#endif

private:
    enum State : uint8_t
    {
        kStateStopped,      // Stopped.
        kStatePendingDnssd, // Started but waiting for `Dnssd` to be ready.
        kStateRunning,      // Started and `Dnssd` is also ready.
    };

    class TxtData
    {
    public:
        struct Info : public Clearable<Info>
        {
            Mac::ExtAddress        mExtAddress;
            MeshCoP::ExtendedPanId mExtPanId;
        };

        void           Init(const uint8_t *aData, uint16_t aLength);
        const uint8_t *GetBytes(void) const { return mData; }
        uint16_t       GetLength(void) const { return mLength; }
        Error          Decode(Info &aInfo);

    protected:
        static const char kExtAddressKey[]; // "xa"
        static const char kExtPanIdKey[];   // "xp"

        const uint8_t *mData;
        uint16_t       mLength;
    };

    class TxtDataEncoder : public InstanceLocator, public TxtData
    {
    public:
        explicit TxtDataEncoder(Instance &aInstance);
        void Encode(void);

    private:
        // TXT data consists of two entries: `xa` for extended address
        // and `xp` for extended PAN ID. Each entry starts with one
        // byte for length, then the two-character key, followed by
        // an `=` character, and then the value. This adds up to
        // (4 + 8 [value]) = 12 bytes total per entry. The value of
        // 32 accommodates these two entries and more.
        static constexpr uint8_t kMaxSize = 32;

        uint8_t mBuffer[kMaxSize];
    };

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

    class ServiceName : public InstanceLocator
    {
        // Tracks the service name (instance label) to use when
        // registering TREL mDNS service

    public:
        explicit ServiceName(Instance &aInstance);

        const char *GetName(void);
        void        GenerateName(void);

    private:
        static const char kNamePrefix[];

        Dns::Name::LabelBuffer mName;
        uint8_t                mSuffixIndex;
    };

    class Browser : public Dnssd::Browser
    {
    public:
        Browser(void);
    };

    class SrvResolver : public Dnssd::SrvResolver
    {
    public:
        explicit SrvResolver(const Peer &aPeer);
    };

    class TxtResolver : public Dnssd::TxtResolver
    {
    public:
        explicit TxtResolver(const Peer &aPeer);
    };

    class AddressResolver : public Dnssd::AddressResolver
    {
    public:
        explicit AddressResolver(const Peer &aPeer);
    };

    struct AddrAndTtl : public Clearable<AddrAndTtl>
    {
        void SetFrom(const Dnssd::AddressAndTtl &aAddrAndTtl);
        bool IsFavoredOver(const Dnssd::AddressAndTtl &aAddrAndTtl) const;
        bool IsEmpty(void) const { return mTtl == 0; }

        Ip6::Address mAddress;
        uint32_t     mTtl;
    };

#endif // OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

#if !OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    struct PeerInfo : public otPlatTrelPeerInfo
    {
        bool                 IsRemoved(void) const { return mRemoved; }
        const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }
    };
#endif

    explicit PeerDiscoverer(Instance &aInstance);
    ~PeerDiscoverer(void);

    bool IsRunning(void) const { return mState == kStateRunning; }
    void PostServiceTask(void);
    void HandleServiceTask(void);
    void RegisterService(void);

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    // Callback from `Dnssd`
    void HandleDnssdPlatformStateChange(void);
    // Callback from `Peer` class.
    void HandlePeerRemoval(Peer &aPeer);

    void RegisterService(uint16_t aPort, const TxtData &aTxtData);
    void UnregisterService(void);
    void HandleRegisterDone(Error aError);
    void HandleBrowseResult(const Dnssd::BrowseResult &aResult);
    void StartServiceResolvers(Peer &aPeer);
    void StopServiceResolvers(Peer &aPeer);
    void HandleSrvResult(const Dnssd::SrvResult &aResult);
    void HandleTxtResult(const Dnssd::TxtResult &aResult);
    void ProcessPeerTxtData(const Dnssd::TxtResult &aResult, Peer &aPeer);
    void StartHostAddressResolver(Peer &aPeer);
    void StopHostAddressResolver(Peer &aPeer);
    void HandleAddressResult(const Dnssd::AddressResult &aResult);
    void UpdatePeerAddresses(Peer &aPeer, const Peer::AddressArray &aSortedAddresses);
    void UpdatePeerState(Peer &aPeer);

    static void HandleRegisterDone(otInstance *aInstance, Dnssd::RequestId aRequestId, otError aError);
    static void HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult);
    static void HandleSrvResult(otInstance *aInstance, const otPlatDnssdSrvResult *aResult);
    static void HandleTxtResult(otInstance *aInstance, const otPlatDnssdTxtResult *aResult);
    static void HandleAddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult);

#else
    void        HandleDiscoveredPeerInfo(const PeerInfo &aInfo);
#endif

    using ServiceTask = TaskletIn<PeerDiscoverer, &PeerDiscoverer::HandleServiceTask>;

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    static const char kTrelServiceType[];
#endif

    State       mState;
    ServiceTask mServiceTask;
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    ServiceName mServiceName;
    bool        mBrowsing;
#endif
};

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // TREL_PEER_DISCOVERER_HPP_
