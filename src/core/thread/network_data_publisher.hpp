/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes definition of Network Data Publisher.
 */

#ifndef NETWORK_DATA_PUBLISHER_HPP_
#define NETWORK_DATA_PUBLISHER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE

#if !OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE && !OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#error "OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE requires either OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE"\
            "or OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE"
#endif

#include <openthread/netdata_publisher.h>

#include "border_router/routing_manager.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/string.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/network_data_service.hpp"
#include "thread/network_data_types.hpp"

namespace ot {
namespace NetworkData {

/**
 * Implements the Network Data Publisher.
 *
 * It provides mechanisms to limit the number of similar Service and/or Prefix (on-mesh prefix or external route)
 * entries in the Thread Network Data by monitoring the Network Data and managing if or when to add or remove entries.
 */
class Publisher : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    /**
     * Represents the events reported from the Publisher callbacks.
     */
    enum Event : uint8_t
    {
        kEventEntryAdded   = OT_NETDATA_PUBLISHER_EVENT_ENTRY_ADDED,   ///< Entry is added to Network Data.
        kEventEntryRemoved = OT_NETDATA_PUBLISHER_EVENT_ENTRY_REMOVED, ///< Entry is removed from Network Data.
    };

    /**
     * Represents the requester associated with a published prefix.
     */
    enum Requester : uint8_t
    {
        kFromUser,           ///< Requested by user (public OT API).
        kFromRoutingManager, ///< Requested by `RoutingManager` module.
    };

    /**
     * Initializes `Publisher` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Publisher(Instance &aInstance);

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

    /**
     * Represents the callback function pointer used to notify when a "DNS/SRP Service" entry is added to or
     * removed from the Thread Network Data.
     *
     * On remove the callback is invoked independent of whether the entry is removed by `Publisher` (e.g., when there
     * are too many similar entries already present in the Network Data) or through an explicit call to unpublish the
     * entry (i.e., a call to `UnpublishDnsSrpService()`).
     */
    typedef otNetDataDnsSrpServicePublisherCallback DnsSrpServiceCallback;

    /**
     * Sets a callback for notifying when a published "DNS/SRP Service" is actually added to or removed
     * from the Thread Network Data.
     *
     * A subsequent call to this method replaces any previously set callback function.
     *
     * @param[in] aCallback        The callback function pointer (can be NULL if not needed).
     * @param[in] aContext         A pointer to application-specific context (used when @p aCallback is invoked).
     */
    void SetDnsSrpServiceCallback(DnsSrpServiceCallback aCallback, void *aContext)
    {
        mDnsSrpServiceEntry.SetCallback(aCallback, aContext);
    }

    /**
     * Requests "DNS/SRP Service Anycast Address" to be published in the Thread Network Data.
     *
     * A call to this method will remove and replace any previous "DNS/SRP Service" entry that was being published
     * (from earlier call to any of `PublishDnsSrpService{Type}()` methods).
     *
     * @param[in] aSequenceNumber  The sequence number of DNS/SRP Anycast Service.
     */
    void PublishDnsSrpServiceAnycast(uint8_t aSequenceNumber) { mDnsSrpServiceEntry.PublishAnycast(aSequenceNumber); }

    /**
     * Requests "DNS/SRP Service Unicast Address" to be published in the Thread Network Data.
     *
     * A call to this method will remove and replace any previous "DNS/SRP Service" entry that was being published
     * (from earlier call to any of `PublishDnsSrpService{Type}()` methods).
     *
     * Publishes the "DNS/SRP Service Unicast Address" by including the address and port info in the
     * Service TLV data.
     *
     * @param[in] aAddress   The DNS/SRP server address to publish.
     * @param[in] aPort      The SRP server port number to publish.
     */
    void PublishDnsSrpServiceUnicast(const Ip6::Address &aAddress, uint16_t aPort)
    {
        mDnsSrpServiceEntry.PublishUnicast(aAddress, aPort);
    }

    /**
     * Requests "DNS/SRP Service Unicast Address" to be published in the Thread Network Data.
     *
     * A call to this method will remove and replace any previous "DNS/SRP Service" entry that was being published
     * (from earlier call to any of `PublishDnsSrpService{Type}()` methods).
     *
     * Unlike the `PublishDnsSrpServiceUnicast(aAddress, aPort)` which requires the published address to be given and
     * includes the info in the Service TLV data, this method uses the device's mesh-local EID and includes the info
     * in the Server TLV data.
     *
     * @param[in] aPort      The SRP server port number to publish.
     */
    void PublishDnsSrpServiceUnicast(uint16_t aPort) { mDnsSrpServiceEntry.PublishUnicast(aPort); }

    /**
     * Indicates whether or not currently the "DNS/SRP Service" entry is added to the Thread Network Data.
     *
     * @retval TRUE    The published DNS/SRP Service entry is added to the Thread Network Data.
     * @retval FALSE   The entry is not added to Thread Network Data or there is no entry to publish.
     */
    bool IsDnsSrpServiceAdded(void) const { return mDnsSrpServiceEntry.IsAdded(); }

    /**
     * Unpublishes any previously added "DNS/SRP (Anycast or Unicast) Service" entry from the Thread
     * Network Data.
     */
    void UnpublishDnsSrpService(void) { mDnsSrpServiceEntry.Unpublish(); }

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * Represents the callback function pointer used to notify when a prefix (on-mesh or external route)
     * entry is added to or removed from the Thread Network Data.
     *
     * On remove the callback is invoked independent of whether the entry is removed by `Publisher` (e.g., when there
     * are too many similar entries already present in the Network Data) or through an explicit call to unpublish the
     * entry.
     */
    typedef otNetDataPrefixPublisherCallback PrefixCallback;

    /**
     * Sets a callback for notifying when a published prefix entry is actually added to or removed from
     * the Thread Network Data.
     *
     * A subsequent call to this method replaces any previously set callback function.
     *
     * @param[in] aCallback        The callback function pointer (can be NULL if not needed).
     * @param[in] aContext         A pointer to application-specific context (used when @p aCallback is invoked).
     */
    void SetPrefixCallback(PrefixCallback aCallback, void *aContext) { mPrefixCallback.Set(aCallback, aContext); }

    /**
     * Requests an on-mesh prefix to be published in the Thread Network Data.
     *
     * Only stable entries can be published (i.e.,`aConfig.mStable` MUST be `true`).
     *
     * A subsequent call to this method will replace a previous request for the same prefix. In particular if the
     * new call only changes the flags (e.g., preference level) and the prefix is already added in the Network Data,
     * the change to flags is immediately reflected in the Network Data. This ensures that existing entries in the
     * Network Data are not abruptly removed. Note that a change in the preference level can potentially later cause
     * the entry to be removed from the Network Data after determining there are other nodes that are publishing the
     * same prefix with the same or higher preference.
     *
     * @param[in] aConfig         The on-mesh prefix config to publish.
     * @param[in] aRequester      The requester (`kFromUser` or `kFromRoutingManager` module).
     *
     * @retval kErrorNone         The on-mesh prefix is published successfully.
     * @retval kErrorInvalidArgs  The @p aConfig is not valid (bad prefix, invalid flag combinations, or not stable).
     * @retval kErrorAlready      An entry with the same prefix is already in the published list.
     * @retval kErrorNoBufs       Could not allocate an entry for the new request. Publisher supports a limited number
     *                            of entries (shared between on-mesh prefix and external route) determined by config
     *                            `OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_PREFIX_ENTRIES`.
     */
    Error PublishOnMeshPrefix(const OnMeshPrefixConfig &aConfig, Requester aRequester);

    /**
     * Requests an external route prefix to be published in the Thread Network Data.
     *
     * Only stable entries can be published (i.e.,`aConfig.mStable` MUST be `true`).
     *
     * A subsequent call to this method will replace a previous request for the same prefix. In particular if the
     * new call only changes the flags (e.g., preference level) and the prefix is already added in the Network Data,
     * the change to flags is immediately reflected in the Network Data. This ensures that existing entries in the
     * Network Data are not abruptly removed. Note that a change in the preference level can potentially later cause
     * the entry to be removed from the Network Data after determining there are other nodes that are publishing the
     * same prefix with the same or higher preference.
     *
     * @param[in] aConfig         The external route config to publish.
     * @param[in] aRequester      The requester (`kFromUser` or `kFromRoutingManager` module).
     *
     * @retval kErrorNone         The external route is published successfully.
     * @retval kErrorInvalidArgs  The @p aConfig is not valid (bad prefix, invalid flag combinations, or not stable).
     * @retval kErrorNoBufs       Could not allocate an entry for the new request. Publisher supports a limited number
     *                            of entries (shared between on-mesh prefix and external route) determined by config
     *                            `OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_PREFIX_ENTRIES`.
     */
    Error PublishExternalRoute(const ExternalRouteConfig &aConfig, Requester aRequester);

    /**
     * Replaces a previously published external route.
     *
     * Only stable entries can be published (i.e.,`aConfig.mStable` MUST be `true`).
     *
     * If there is no previously published external route matching @p aPrefix, this method behaves similarly to
     * `PublishExternalRoute()`, i.e., it will start the process of publishing @a aConfig as an external route in the
     * Thread Network Data.
     *
     * If there is a previously published route entry matching @p aPrefix, it will be replaced with the new prefix from
     * @p aConfig.
     *
     * - If the @p aPrefix was already added in the Network Data, the change to the new prefix in @p aConfig is
     *   immediately reflected in the Network Data. This ensures that route entries in the Network Data are not
     *   abruptly removed and the transition from aPrefix to the new prefix is smooth.
     *
     * - If the old published @p aPrefix was not added in the Network Data, it will be replaced with the new @p aConfig
     *   prefix but it will not be immediately added. Instead, it will start the process of publishing it in the
     *   Network Data (monitoring the Network Data to determine when/if to add the prefix, depending on the number of
     *   similar prefixes present in the Network Data).
     *
     * @param[in] aPrefix         The previously published external route prefix to replace.
     * @param[in] aConfig         The external route config to publish.
     * @param[in] aRequester      The requester (`kFromUser` or `kFromRoutingManager` module).
     *
     * @retval kErrorNone         The external route is published successfully.
     * @retval kErrorInvalidArgs  The @p aConfig is not valid (bad prefix, invalid flag combinations, or not stable).
     * @retval kErrorNoBufs       Could not allocate an entry for the new request. Publisher supports a limited number
     *                            of entries (shared between on-mesh prefix and external route) determined by config
     *                            `OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_PREFIX_ENTRIES`.
     */
    Error ReplacePublishedExternalRoute(const Ip6::Prefix         &aPrefix,
                                        const ExternalRouteConfig &aConfig,
                                        Requester                  aRequester);

    /**
     * Indicates whether or not currently a published prefix entry (on-mesh or external route) is added to
     * the Thread Network Data.
     *
     * @param[in] aPrefix   The prefix to check.
     *
     * @retval TRUE    The published prefix entry is added to the Thread Network Data.
     * @retval FALSE   The entry is not added to Thread Network Data or there is no matching entry to publish.
     */
    bool IsPrefixAdded(const Ip6::Prefix &aPrefix) const;

    /**
     * Unpublishes a previously published prefix (on-mesh or external route).
     *
     * @param[in] aPrefix       The prefix to unpublish.
     *
     * @retval kErrorNone       The prefix was unpublished successfully.
     * @retval kErrorNotFound   Could not find the prefix in the published list.
     */
    Error UnpublishPrefix(const Ip6::Prefix &aPrefix);
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

private:
    class Entry : public InstanceLocatorInit
    {
    protected:
        enum State : uint8_t
        {
            kNoEntry,  // Entry is unused (there is no entry).
            kToAdd,    // Entry is ready to be added, monitoring network data to decide if/when to add it.
            kAdding,   // Entry is being added in network data (random wait interval before add).
            kAdded,    // Entry is added in network data, monitoring to determine if/when to remove.
            kRemoving, // Entry is being removed from network data (random wait interval before remove).
        };

        // All intervals are in milliseconds.
        static constexpr uint32_t kMaxDelayToAdd    = OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_DELAY_TO_ADD;
        static constexpr uint32_t kMaxDelayToRemove = OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_DELAY_TO_REMOVE;
        static constexpr uint32_t kExtraDelayToRemovePreferred =
            OPENTHREAD_CONFIG_NETDATA_PUBLISHER_EXTRA_DELAY_TIME_TO_REMOVE_PREFERRED;

        static constexpr uint16_t kInfoStringSize = 60;

        typedef String<kInfoStringSize> InfoString;

        Entry(void)
            : mState(kNoEntry)
        {
        }

        void             Init(Instance &aInstance) { InstanceLocatorInit::Init(aInstance); }
        State            GetState(void) const { return mState; }
        void             SetState(State aState);
        const TimeMilli &GetUpdateTime(void) const { return mUpdateTime; }
        bool             IsPreferred(uint16_t aRloc16) const;
        void             UpdateState(uint8_t aNumEntries, uint8_t aNumPreferredEntries, uint8_t aDesiredNumEntries);
        void             HandleTimer(void);
        InfoString       ToString(bool aIncludeState = true) const;

    public:
        bool IsAdded(void) const { return (mState == kAdded); }

    private:
        void               Add(void);
        void               Remove(State aNextState);
        void               LogUpdateTime(void) const;
        static const char *StateToString(State aState);

        TimeMilli mUpdateTime;
        State     mState;
    };

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    class DnsSrpServiceEntry : public Entry, private NonCopyable
    {
        friend class Entry;

    public:
        explicit DnsSrpServiceEntry(Instance &aInstance);
        void SetCallback(DnsSrpServiceCallback aCallback, void *aContext) { mCallback.Set(aCallback, aContext); }
        void PublishAnycast(uint8_t aSequenceNumber);
        void PublishUnicast(const Ip6::Address &aAddress, uint16_t aPort);
        void PublishUnicast(uint16_t aPort);
        void Unpublish(void);
        void HandleTimer(void) { Entry::HandleTimer(); }
        void HandleNotifierEvents(Events aEvents);

    private:
        static constexpr uint8_t kDesiredNumAnycast =
            OPENTHREAD_CONFIG_NETDATA_PUBLISHER_DESIRED_NUM_ANYCAST_DNS_SRP_SERVICE_ENTRIES;

        static constexpr uint8_t kDesiredNumUnicast =
            OPENTHREAD_CONFIG_NETDATA_PUBLISHER_DESIRED_NUM_UNICAST_DNS_SRP_SERVICE_ENTRIES;

        enum Type : uint8_t
        {
            kTypeAnycast,
            kTypeUnicast,
            kTypeUnicastMeshLocalEid,
        };

        class Info : public Clearable<Info>, public Equatable<Info>
        {
        public:
            Info(void) { Clear(); }
            Type                GetType(void) const { return mType; }
            uint8_t             GetSequenceNumber(void) const { return static_cast<uint8_t>(mPortOrSeqNumber); }
            uint16_t            GetPort(void) const { return mPortOrSeqNumber; }
            const Ip6::Address &GetAddress(void) const { return mAddress; }
            void                SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }

            static Info InfoAnycast(uint8_t aSequenceNumber) { return Info(kTypeAnycast, aSequenceNumber); }
            static Info InfoUnicast(Type aType, const Ip6::Address &aAddress, uint16_t aPort)
            {
                return Info(aType, aPort, &aAddress);
            }

        private:
            Info(Type aType, uint16_t aPortOrSeqNumber, const Ip6::Address *aAddress = nullptr);

            Ip6::Address mAddress;
            uint16_t     mPortOrSeqNumber;
            Type         mType;
        };

        Type GetType(void) const { return mInfo.GetType(); }
        void Publish(const Info &aInfo);
        void Add(void);
        void Remove(State aNextState);
        void Notify(Event aEvent) const;
        void Process(void);
        void CountAnycastEntries(uint8_t &aNumEntries, uint8_t &aNumPreferredEntries) const;
        bool HasAnyAnycastEntry(void) const;
        void CountUnicastEntries(Service::DnsSrpUnicastType aType,
                                 uint8_t                   &aNumEntries,
                                 uint8_t                   &aNumPreferredEntries) const;
        bool HasAnyServiceDataUnicastEntry(void) const;

        Info                            mInfo;
        Callback<DnsSrpServiceCallback> mCallback;
    };
#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

    // Max number of prefix (on-mesh or external route) entries.
    static constexpr uint16_t kMaxUserPrefixEntries = OPENTHREAD_CONFIG_NETDATA_PUBLISHER_MAX_PREFIX_ENTRIES;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    static constexpr uint16_t kMaxRoutingManagerPrefixEntries = BorderRouter::RoutingManager::kMaxPublishedPrefixes;
#else
    static constexpr uint16_t kMaxRoutingManagerPrefixEntries = 0;
#endif

    class PrefixEntry : public Entry, private NonCopyable
    {
        friend class Entry;

    public:
        void      Init(Instance &aInstance) { Entry::Init(aInstance); }
        bool      IsInUse(void) const { return GetState() != kNoEntry; }
        bool      Matches(const Ip6::Prefix &aPrefix) const { return mPrefix == aPrefix; }
        void      Publish(const OnMeshPrefixConfig &aConfig, Requester aRequester);
        void      Publish(const ExternalRouteConfig &aConfig, Requester aRequester);
        Requester GetRequester(void) const { return mRequester; }
        void      Unpublish(void);
        void      HandleTimer(void) { Entry::HandleTimer(); }
        void      HandleNotifierEvents(Events aEvents);

    private:
        static constexpr uint8_t kDesiredNumOnMeshPrefix =
            OPENTHREAD_CONFIG_NETDATA_PUBLISHER_DESIRED_NUM_ON_MESH_PREFIX_ENTRIES;

        static constexpr uint8_t kDesiredNumExternalRoute =
            OPENTHREAD_CONFIG_NETDATA_PUBLISHER_DESIRED_NUM_EXTERNAL_ROUTE_ENTRIES;

        enum Type : uint8_t
        {
            kTypeOnMeshPrefix,
            kTypeExternalRoute,
        };

        void  Publish(const Ip6::Prefix &aPrefix, uint16_t aNewFlags, Type aNewType, Requester aRequester);
        void  Add(void);
        Error AddOnMeshPrefix(void);
        Error AddExternalRoute(void);
        void  Remove(State aNextState);
        void  Process(void);
        void  CountOnMeshPrefixEntries(uint8_t &aNumEntries, uint8_t &aNumPreferredEntries) const;
        void  CountExternalRouteEntries(uint8_t &aNumEntries, uint8_t &aNumPreferredEntries) const;

        Type        mType;
        Requester   mRequester;
        Ip6::Prefix mPrefix;
        uint16_t    mFlags;
    };
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    bool IsADnsSrpServiceEntry(const Entry &aEntry) const { return (&aEntry == &mDnsSrpServiceEntry); }
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    PrefixEntry       *FindOrAllocatePrefixEntry(const Ip6::Prefix &aPrefix, Requester aRequester);
    PrefixEntry       *FindMatchingPrefixEntry(const Ip6::Prefix &aPrefix);
    const PrefixEntry *FindMatchingPrefixEntry(const Ip6::Prefix &aPrefix) const;
    bool               IsAPrefixEntry(const Entry &aEntry) const;
    void               NotifyPrefixEntryChange(Event aEvent, const Ip6::Prefix &aPrefix) const;
#endif

    TimerMilli &GetTimer(void) { return mTimer; }
    void        HandleNotifierEvents(Events aEvents);
    void        HandleTimer(void);

    using PublisherTimer = TimerMilliIn<Publisher, &Publisher::HandleTimer>;

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    DnsSrpServiceEntry mDnsSrpServiceEntry;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    PrefixEntry              mPrefixEntries[kMaxUserPrefixEntries + kMaxRoutingManagerPrefixEntries];
    Callback<PrefixCallback> mPrefixCallback;
#endif

    PublisherTimer mTimer;
};

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE

#endif // NETWORK_DATA_PUBLISHER_HPP_
