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
 *   This file includes definitions for Border Agent Tracker.
 */

#ifndef BORDER_AGENT_TRACKER_HPP_
#define BORDER_AGENT_TRACKER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE

#if !OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE && !OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
#error "BORDER_AGENT_TRACKER_ENABLE requires either the native mDNS or platform DNS-SD APIs"
#endif

#include <openthread/border_agent_tracker.h>

#include "common/as_core_type.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_array.hpp"
#include "common/heap_data.hpp"
#include "common/heap_string.hpp"
#include "common/locator.hpp"
#include "common/owning_list.hpp"
#include "common/retain_ptr.hpp"
#include "net/dnssd.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

/**
 * Implements the Border Agent Tracker.
 */
class Tracker : public InstanceLocator
{
    friend class ot::Dnssd;

    struct Agent;

public:
    typedef otBorderAgentTrackerAgentInfo AgentInfo; ///< Information about a discovered Border Agent.

    /**
     * Represents an entity requesting to start or stop.
     */
    enum Requester : uint8_t
    {
        kRequesterUser,  ///< Requested by user (public OT API).
        kRequesterStack, ///< Requested by stack itself (other OT modules).
    };

    class Iterator : public otBorderAgentTrackerIterator
    {
    public:
        /**
         * Initializes the iterator.
         *
         * An iterator MUST be initialized before being used.
         *
         * @param[in] aInstance The OpenThread instance.
         */
        void Init(Instance &aInstance);

        /**
         * Gets the information for the next discovered Border Agent.
         *
         * @param[out] aInfo  A reference to an `AgentInfo` to populate with the agent's information.
         *
         * @retval kErrorNone      Successfully retrieved the information for the next agent.
         * @retval kErrorNotFound  No more agents were found.
         */
        Error GetNextAgentInfo(AgentInfo &aInfo);

    private:
        const Agent *GetAgentEntry(void) const { return static_cast<const Agent *>(mPtr); }
        void         SetAgentEntry(const Agent *aEntry) { mPtr = aEntry; }
        uint64_t     GetInitUptime(void) const { return mData; }
        void         SetInitUptime(uint64_t aUptime) { mData = aUptime; }
    };

    /**
     * Initializes the Border Agent Tracker.
     *
     * @param[in]  aInstance  The OpenThread instance.
     */
    explicit Tracker(Instance &aInstance);

    /**
     * Enables or disables the Border Agent Tracker.
     *
     * When enabled, the tracker browses for the `_meshcop._udp` mDNS service to discover and track Border Agents on
     * the infra-if network.
     *
     * The Border Agent Tracker can be enabled by multiple requesters (see `Requester`). It remains enabled as long as
     * at least one requester has it enabled. It is disabled only when all requesters have disabled it.
     *
     * @param[in] aEnable      A boolean to enable/disable the Border Agent Tracker.
     * @param[in] aRequester   The entity requesting to enable/disable.
     */
    void SetEnabled(bool aEnable, Requester aRequester);

    /**
     * Indicates whether the tracker is running.
     *
     * The tracker is running if at least one requester (see `Requester`) has enabled it and the underlying DNS-SD
     * platform is ready.
     *
     * @retval TRUE   If the tracker is running.
     * @retval FALSE  If the tracker is not running.
     */
    bool IsRunning(void) const { return mState == kStateRunning; }

private:
    enum State : uint8_t
    {
        kStateStopped,
        kStatePendingDnssd,
        kStateRunning,
    };

    enum MatchType : uint8_t
    {
        kMatchServiceName,
        kMatchHostName,
    };

    struct Host : public InstanceLocator, public RetainCountable, public Heap::Allocatable<Host>
    {
        explicit Host(Instance &aInstance);
        ~Host(void);

        Error SetNameAndStartAddrResolver(const char *aHostName);
        void  SetAddresses(const Dnssd::AddressResult &aResult);

        Heap::String              mName;
        Heap::Array<Ip6::Address> mAddresses;
    };

    struct Agent : public InstanceLocator, public Heap::Allocatable<Agent>, public LinkedListEntry<Agent>
    {
        explicit Agent(Instance &aInstance);
        ~Agent(void);

        Error SetServiceNameAndStartSrvTxtResolvers(const char *aServiceName);
        void  SetHost(const char *aHostName);
        void  ClearHost(void);
        void  SetPort(uint16_t aPort);
        void  SetTxtData(const uint8_t *aData, uint16_t aDataLength);
        void  ClearTxtData(void);
        void  SetUpdateTimeToNow(void);
        bool  Matches(MatchType aType, const char *aName) const;
        void  CopyInfoTo(AgentInfo &aInfo, uint64_t aUptimeNow) const;

        Agent          *mNext;
        Heap::String    mServiceName;
        RetainPtr<Host> mHost;
        Heap::Data      mTxtData;
        uint64_t        mDiscoverUptime;
        uint64_t        mLastUpdateUptime;
        uint16_t        mPort;
    };

    struct Browser : public Dnssd::Browser
    {
        Browser(void);
    };

    struct SrvResolver : public Dnssd::SrvResolver
    {
        explicit SrvResolver(const char *aServiceName);
    };

    struct TxtResolver : public Dnssd::TxtResolver
    {
        explicit TxtResolver(const char *aServiceName);
    };

    struct AddressResolver : public Dnssd::AddressResolver
    {
        explicit AddressResolver(const char *aHostName);
    };

    void UpdateState(void);
    void HandleDnssdPlatformStateChange(void);
    void HandleBrowseResult(const Dnssd::BrowseResult &aResult);
    void HandleSrvResult(const Dnssd::SrvResult &aResult);
    void HandleTxtResult(const Dnssd::TxtResult &aResult);
    void HandleAddressResult(const Dnssd::AddressResult &aResult);

    static void HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult);
    static void HandleSrvResult(otInstance *aInstance, const otPlatDnssdSrvResult *aResult);
    static void HandleTxtResult(otInstance *aInstance, const otPlatDnssdTxtResult *aResult);
    static void HandleAddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult);

    static bool        NameMatch(const Heap::String &aHeapString, const char *aName);
    static void        LogOnError(Error aError, const char *aText, const char *aName);
    static const char *StateToString(State aState);

    static const char kServiceType[];

    State             mState;
    bool              mUserEnabled : 1;
    bool              mStackEnabled : 1;
    OwningList<Agent> mAgents;
};

} // namespace BorderAgent
} // namespace MeshCoP

DefineCoreType(otBorderAgentTrackerIterator, MeshCoP::BorderAgent::Tracker::Iterator);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE

#endif // BORDER_AGENT_TRACKER_HPP_
