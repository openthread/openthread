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
 *   This file includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 */

#ifndef NETWORK_DATA_LEADER_HPP_
#define NETWORK_DATA_LEADER_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include "coap/coap.hpp"
#include "common/const_cast.hpp"
#include "common/non_copyable.hpp"
#include "common/numeric_limits.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace NetworkData {

/**
 * @addtogroup core-netdata-leader
 *
 * @brief
 *   This module includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 *
 * @{
 */

/**
 * Implements the Thread Network Data maintained by the Leader.
 */
class Leader : public MutableNetworkData, private NonCopyable
{
    friend class Tmf::Agent;
    friend class Notifier;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Leader(Instance &aInstance);

    /**
     * Reset the Thread Network Data.
     */
    void Reset(void);

    /**
     * Returns the maximum observed Network Data length since OT stack initialization or since the last
     * call to `ResetMaxLength()`.
     *
     * @returns The maximum observed Network Data length (high water mark for Network Data length).
     */
    uint8_t GetMaxLength(void) const { return mMaxLength; }

    /**
     * Resets the tracked maximum Network Data Length.
     *
     * @sa GetMaxLength
     */
    void ResetMaxLength(void) { mMaxLength = GetLength(); }

    /**
     * Returns the Data Version value for a type (full set or stable subset).
     *
     * @param[in] aType   The Network Data type (full set or stable subset).
     *
     * @returns The Data Version value for @p aType.
     */
    uint8_t GetVersion(Type aType) const { return (aType == kFullSet) ? mVersion : mStableVersion; }

    /**
     * Retrieves the 6LoWPAN Context information based on a given IPv6 address.
     *
     * @param[in]   aAddress  A reference to an IPv6 address.
     * @param[out]  aContext  A reference to 6LoWPAN Context information.
     *
     * @retval kErrorNone       Successfully retrieved 6LoWPAN Context information.
     * @retval kErrorNotFound   Could not find the 6LoWPAN Context information.
     */
    Error GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext) const;

    /**
     * Retrieves the 6LoWPAN Context information based on a given Context ID.
     *
     * @param[in]   aContextId  The Context ID value.
     * @param[out]  aContext    A reference to the 6LoWPAN Context information.
     *
     * @retval kErrorNone       Successfully retrieved 6LoWPAN Context information.
     * @retval kErrorNotFound   Could not find the 6LoWPAN Context information.
     */
    Error GetContext(uint8_t aContextId, Lowpan::Context &aContext) const;

    /**
     * Indicates whether or not the given IPv6 address is on-mesh.
     *
     * @param[in]  aAddress  A reference to an IPv6 address.
     *
     * @retval TRUE   If @p aAddress is on-link.
     * @retval FALSE  If @p aAddress if not on-link.
     */
    bool IsOnMesh(const Ip6::Address &aAddress) const;

    /**
     * Performs a route lookup using the Network Data.
     *
     * @param[in]   aSource             A reference to the IPv6 source address.
     * @param[in]   aDestination        A reference to the IPv6 destination address.
     * @param[out]  aRloc16             A reference to return the RLOC16 for the selected route.
     *
     * @retval kErrorNone      Successfully found a route. @p aRloc16 is updated.
     * @retval kErrorNoRoute   No valid route was found.
     */
    Error RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint16_t &aRloc16) const;

    /**
     * Is used by non-Leader devices to set Network Data by reading it from a message from Leader.
     *
     * @param[in]  aVersion        The Version value.
     * @param[in]  aStableVersion  The Stable Version value.
     * @param[in]  aType           The Network Data type to set, the full set or stable subset.
     * @param[in]  aMessage        A reference to the message.
     * @param[in]  aOffsetRange    The offset range in @p aMessage to read from.
     *
     * @retval kErrorNone   Successfully set the network data.
     * @retval kErrorParse  Network Data in @p aMessage is not valid.
     */
    Error SetNetworkData(uint8_t            aVersion,
                         uint8_t            aStableVersion,
                         Type               aType,
                         const Message     &aMessage,
                         const OffsetRange &aOffsetRange);

    /**
     * Gets the Commissioning Dataset from Network Data.
     *
     * @param[out] aDataset    A reference to a `MeshCoP::CommissioningDataset` to populate.
     */
    void GetCommissioningDataset(MeshCoP::CommissioningDataset &aDataset) const;

    /**
     * Processes a MGMT_COMMISSIONER_GET request message and prepares the response.
     *
     * @param[in] aRequest   The MGMT_COMMISSIONER_GET request message.
     *
     * @returns The prepared response, or `nullptr` if fails to parse the request or cannot allocate message.
     */
    Coap::Message *ProcessCommissionerGetRequest(const Coap::Message &aMessage) const;

    /**
     * Searches for given sub-TLV in Commissioning Data TLV.
     *
     * @tparam SubTlvType    The sub-TLV type to search for.
     *
     * @returns A pointer to the Commissioning Data Sub-TLV or `nullptr` if no such sub-TLV exists.
     */
    template <typename SubTlvType> const SubTlvType *FindInCommissioningData(void) const
    {
        return As<SubTlvType>(FindCommissioningDataSubTlv(SubTlvType::kType));
    }

    /**
     * Searches for given sub-TLV in Commissioning Data TLV.
     *
     * @tparam SubTlvType    The sub-TLV type to search for.
     *
     * @returns A pointer to the Commissioning Data Sub-TLV or `nullptr` if no such sub-TLV exists.
     */
    template <typename SubTlvType> SubTlvType *FindInCommissioningData(void)
    {
        return As<SubTlvType>(FindCommissioningDataSubTlv(SubTlvType::kType));
    }

    /**
     * Finds and reads the Commissioning Session ID in Commissioning Data TLV.
     *
     * @param[out] aSessionId  A reference to return the read session ID.
     *
     * @retval kErrorNone       Successfully read the session ID, @p aSessionId is updated.
     * @retval kErrorNotFound   Did not find Session ID sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     */
    Error FindCommissioningSessionId(uint16_t &aSessionId) const;

    /**
     * Finds and reads the Border Agent RLOC16 in Commissioning Data TLV.
     *
     * @param[out] aRloc16  A reference to return the read RLOC16.
     *
     * @retval kErrorNone       Successfully read the Border Agent RLOC16, @p aRloc16 is updated.
     * @retval kErrorNotFound   Did not find Border Agent RLOC16 sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     */
    Error FindBorderAgentRloc(uint16_t &aRloc16) const;

    /**
     * Finds and reads the Joiner UDP Port in Commissioning Data TLV.
     *
     * @param[out] aPort  A reference to return the read port number.
     *
     * @retval kErrorNone       Successfully read the Joiner UDP port, @p aPort is updated.
     * @retval kErrorNotFound   Did not find Joiner UDP Port sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     */
    Error FindJoinerUdpPort(uint16_t &aPort) const;

    /**
     * Finds and read the Steering Data in Commissioning Data TLV.
     *
     * @param[out] aSteeringData  A reference to return the read Steering Data.
     *
     * @retval kErrorNone       Successfully read the Steering Data, @p aSteeringData is updated.
     * @retval kErrorNotFound   Did not find Steering Data sub-TLV.
     */
    Error FindSteeringData(MeshCoP::SteeringData &aSteeringData) const;

    /**
     * Indicates whether or not the Commissioning Data TLV indicates Joining is allowed.
     *
     * Joining is allowed if a Border Agent Locator TLV exist and the Steering Data TLV is non-zero.
     *
     * @retval TRUE    If joining is allowed.
     * @retval FALSE   If joining is not allowed.
     */
    bool IsJoiningAllowed(void) const;

    /**
     * Checks if the steering data includes a Joiner.
     *
     * @param[in]  aEui64             A reference to the Joiner's IEEE EUI-64.
     *
     * @retval kErrorNone          @p aEui64 is in the bloom filter.
     * @retval kErrorInvalidState  No steering data present.
     * @retval kErrorNotFound      @p aEui64 is not in the bloom filter.
     */
    Error SteeringDataCheckJoiner(const Mac::ExtAddress &aEui64) const;

    /**
     * Checks if the steering data includes a Joiner with a given discerner value.
     *
     * @param[in]  aDiscerner         A reference to the Joiner Discerner.
     *
     * @retval kErrorNone          @p aDiscerner is in the bloom filter.
     * @retval kErrorInvalidState  No steering data present.
     * @retval kErrorNotFound      @p aDiscerner is not in the bloom filter.
     */
    Error SteeringDataCheckJoiner(const MeshCoP::JoinerDiscerner &aDiscerner) const;

    /**
     * Gets the Service ID for the specified service.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number (IANA-assigned) for Service TLV
     * @param[in]  aServiceData       The Service Data.
     * @param[in]  aServerStable      The Stable flag value for Server TLV.
     * @param[out] aServiceId         A reference where to put the Service ID.
     *
     * @retval kErrorNone       Successfully got the Service ID.
     * @retval kErrorNotFound   The specified service was not found.
     */
    Error GetServiceId(uint32_t           aEnterpriseNumber,
                       const ServiceData &aServiceData,
                       bool               aServerStable,
                       uint8_t           &aServiceId) const;

    /**
     * Gets the preferred NAT64 prefix from network data.
     *
     * The returned prefix is the highest preference external route entry in Network Data with NAT64 flag set. If there
     * are multiple such entries the first one is returned.
     *
     * @param[out] aConfig      A reference to an `ExternalRouteConfig` to return the prefix.
     *
     * @retval kErrorNone       Found the NAT64 prefix and updated @p aConfig.
     * @retval kErrorNotFound   Could not find any NAT64 entry.
     */
    Error GetPreferredNat64Prefix(ExternalRouteConfig &aConfig) const;

    /**
     * Indicates whether or not the given IPv6 address matches any NAT64 prefixes.
     *
     * @param[in]  aAddress  An IPv6 address to check.
     *
     * @retval TRUE   If @p aAddress matches a NAT64 prefix.
     * @retval FALSE  If @p aAddress does not match a NAT64 prefix.
     */
    bool IsNat64(const Ip6::Address &aAddress) const;

#if OPENTHREAD_FTD
    /**
     * Defines the match mode constants to compare two RLOC16 values.
     */
    enum MatchMode : uint8_t
    {
        kMatchModeRloc16,   ///< Perform exact RLOC16 match.
        kMatchModeRouterId, ///< Perform Router ID match (match the router and any of its children).
    };

    /**
     * Starts the Leader services.
     *
     * The start mode indicates whether device is starting normally as leader or restoring its role as leader after
     * reset. In the latter case, we do not accept any new registrations (`HandleServerData()`) and wait for
     * `HandleNetworkDataRestoredAfterReset()` to indicate that the leader has successfully recovered the Network Data
     * before allowing new Network Data registrations.
     *
     * @param[in] aStartMode   The start mode.
     */
    void Start(Mle::LeaderStartMode aStartMode);

    /**
     * Increments the Thread Network Data version.
     */
    void IncrementVersion(void);

    /**
     * Increments both the Thread Network Data version and stable version.
     */
    void IncrementVersionAndStableVersion(void);

    /**
     * Performs anycast ALOC route lookup using the Network Data.
     *
     * @param[in]   aAloc16     The ALOC16 destination to lookup.
     * @param[out]  aRloc16     A reference to return the RLOC16 for the selected route.
     *
     * @retval kErrorNone      Successfully lookup best option for @p aAloc16. @p aRloc16 is updated.
     * @retval kErrorNoRoute   No valid route was found.
     * @retval kErrorDrop      The @p aAloc16 is not valid.
     */
    Error AnycastLookup(uint16_t aAloc16, uint16_t &aRloc16) const;

    /**
     * Returns CONTEXT_ID_RESUSE_DELAY value.
     *
     * @returns The CONTEXT_ID_REUSE_DELAY value (in seconds).
     */
    uint32_t GetContextIdReuseDelay(void) const { return mContextIds.GetReuseDelay(); }

    /**
     * Sets CONTEXT_ID_RESUSE_DELAY value.
     *
     * @warning This method should only be used for testing.
     *
     * @param[in]  aDelay  The CONTEXT_ID_REUSE_DELAY value (in seconds).
     */
    void SetContextIdReuseDelay(uint32_t aDelay) { mContextIds.SetReuseDelay(aDelay); }

    /**
     * Removes Network Data entries matching with a given RLOC16.
     *
     * @param[in]  aRloc16    A RLOC16 value.
     * @param[in]  aMatchMode A match mode (@sa MatchMode).
     */
    void RemoveBorderRouter(uint16_t aRloc16, MatchMode aMatchMode);

    /**
     * Updates Commissioning Data in Network Data.
     *
     * @param[in]  aData        A pointer to the Commissioning Data.
     * @param[in]  aDataLength  The length of @p aData.
     *
     * @retval kErrorNone     Successfully updated the Commissioning Data.
     * @retval kErrorNoBufs   Insufficient space to add the Commissioning Data.
     */
    Error SetCommissioningData(const void *aData, uint8_t aDataLength);

    /**
     * Synchronizes internal 6LoWPAN Context ID Set with recently obtained Thread Network Data.
     *
     * Note that this method should be called only by the Leader once after reset.
     */
    void HandleNetworkDataRestoredAfterReset(void);

#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    /**
     * Indicates whether Network Data contains a valid OMR prefix.
     *
     * If the given @p aPrefix is itself not a valid OMR prefix, this method will return `false`, regardless of
     * whether the prefix is present in the Network Data.
     *
     * @param[in]  aPrefix   The OMR prefix to check.
     *
     * @retval TRUE   Network Data contains a valid OMR prefix entry matching @p aPrefix.
     * @retval FALSE  Network Data does not contain a valid OMR prefix entry matching @p aPrefix.
     */
    bool ContainsOmrPrefix(const Ip6::Prefix &aPrefix) const;
#endif

private:
    using FilterIndexes = MeshCoP::SteeringData::HashBitIndexes;

    typedef bool (&EntryChecker)(const BorderRouterEntry &aEntry);

    const PrefixTlv *FindNextMatchingPrefixTlv(const Ip6::Address &aAddress, const PrefixTlv *aPrevTlv) const;
    const PrefixTlv *FindPrefixTlvForContextId(uint8_t aContextId, const ContextTlv *&aContextTlv) const;

    int CompareRouteEntries(const BorderRouterEntry &aFirst, const BorderRouterEntry &aSecond) const;
    int CompareRouteEntries(const HasRouteEntry &aFirst, const HasRouteEntry &aSecond) const;
    int CompareRouteEntries(const ServerTlv &aFirst, const ServerTlv &aSecond) const;
    int CompareRouteEntries(int8_t   aFirstPreference,
                            uint16_t aFirstRloc,
                            int8_t   aSecondPreference,
                            uint16_t aSecondRloc) const;

    static bool IsEntryDefaultRoute(const BorderRouterEntry &aEntry);

    Error ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination, uint16_t &aRloc16) const;
    Error DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t &aRloc16) const;
    Error LookupRouteIn(const PrefixTlv &aPrefixTlv, EntryChecker aEntryChecker, uint16_t &aRloc16) const;
    Error SteeringDataCheck(const FilterIndexes &aFilterIndexes) const;
    void  GetContextForMeshLocalPrefix(Lowpan::Context &aContext) const;
    Error ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::Type aType, uint16_t &aValue) const;
    void  SignalNetDataChanged(void);
    const CommissioningDataTlv *FindCommissioningData(void) const;
    CommissioningDataTlv *FindCommissioningData(void) { return AsNonConst(AsConst(this)->FindCommissioningData()); }
    const MeshCoP::Tlv   *FindCommissioningDataSubTlv(uint8_t aType) const;
    MeshCoP::Tlv         *FindCommissioningDataSubTlv(uint8_t aType)
    {
        return AsNonConst(AsConst(this)->FindCommissioningDataSubTlv(aType));
    }

#if OPENTHREAD_FTD
    static constexpr uint32_t kMaxNetDataSyncWait = 60 * 1000; // Maximum time to wait for netdata sync in msec.
    static constexpr uint8_t  kMinServiceId       = 0x00;
    static constexpr uint8_t  kMaxServiceId       = 0x0f;

    class ChangedFlags
    {
    public:
        ChangedFlags(void)
            : mChanged(false)
            , mStableChanged(false)
        {
        }

        void Update(const NetworkDataTlv &aTlv)
        {
            mChanged       = true;
            mStableChanged = (mStableChanged || aTlv.IsStable());
        }

        bool DidChange(void) const { return mChanged; }
        bool DidStableChange(void) const { return mStableChanged; }

    private:
        bool mChanged;       // Any (stable or not) network data change (add/remove).
        bool mStableChanged; // Stable network data change (add/remove).
    };

    enum UpdateStatus : uint8_t
    {
        kTlvRemoved, // TLV contained no sub TLVs and therefore is removed.
        kTlvUpdated, // TLV stable flag is updated based on its sub TLVs.
    };

    class ContextIds : public InstanceLocator
    {
    public:
        // This class tracks Context IDs. A Context ID can be in one
        // of the 3 states: It is unallocated, or it is allocated
        // and in-use, or it scheduled to be removed (after reuse delay
        // interval is passed).

        static constexpr uint8_t kInvalidId = NumericLimits<uint8_t>::kMax;

        explicit ContextIds(Instance &aInstance);

        void     Clear(void);
        Error    GetUnallocatedId(uint8_t &aId);
        void     MarkAsInUse(uint8_t aId) { mRemoveTimes[aId - kMinId].SetValue(kInUse); }
        void     ScheduleToRemove(uint8_t aId);
        uint32_t GetReuseDelay(void) const { return mReuseDelay; }
        void     SetReuseDelay(uint32_t aDelay) { mReuseDelay = aDelay; }
        void     HandleTimer(void);
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
        void MarkAsClone(void) { mIsClone = true; }
#endif

    private:
        static constexpr uint32_t kReuseDelay = 5 * 60; // 5 minutes (in seconds).

        static constexpr uint8_t kMinId = 1;
        static constexpr uint8_t kMaxId = 15;

        // The `mRemoveTimes[id]` is used to track the state of a
        // Context ID and its remove time. Two specific values
        // `kUnallocated` and `kInUse` are used to indicate ID is in
        // unallocated or in-use states. Other values indicate we
        // are in remove state waiting to remove it at `mRemoveTime`.

        static constexpr uint32_t kUnallocated = 0;
        static constexpr uint32_t kInUse       = 1;

        bool      IsUnallocated(uint8_t aId) const { return mRemoveTimes[aId - kMinId].GetValue() == kUnallocated; }
        bool      IsInUse(uint8_t aId) const { return mRemoveTimes[aId - kMinId].GetValue() == kInUse; }
        TimeMilli GetRemoveTime(uint8_t aId) const { return mRemoveTimes[aId - kMinId]; }
        void      SetRemoveTime(uint8_t aId, TimeMilli aTime);
        void      MarkAsUnallocated(uint8_t aId) { mRemoveTimes[aId - kMinId].SetValue(kUnallocated); }

        TimeMilli mRemoveTimes[kMaxId - kMinId + 1];
        uint32_t  mReuseDelay;
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
        bool mIsClone;
#endif
    };

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleTimer(void);

    static bool IsEntryForDhcp6Agent(const BorderRouterEntry &aEntry);
    static bool IsEntryForNdAgent(const BorderRouterEntry &aEntry);

    Error LookupRouteForServiceAloc(uint16_t aAloc16, uint16_t &aRloc16) const;
    Error LookupRouteForAgentAloc(uint8_t aContextId, EntryChecker aEntryChecker, uint16_t &aRloc16) const;

    void RegisterNetworkData(uint16_t aRloc16, const NetworkData &aNetworkData);

    Error AddPrefix(const PrefixTlv &aPrefix, ChangedFlags &aChangedFlags);
    Error AddHasRoute(const HasRouteTlv &aHasRoute, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags);
    Error AddBorderRouter(const BorderRouterTlv &aBorderRouter, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags);
    Error AddService(const ServiceTlv &aService, ChangedFlags &aChangedFlags);
    Error AddServer(const ServerTlv &aServer, ServiceTlv &aDstService, ChangedFlags &aChangedFlags);

    Error             AllocateServiceId(uint8_t &aServiceId) const;
    const ServiceTlv *FindServiceById(uint8_t aServiceId) const;

    void RemoveContext(uint8_t aContextId);
    void RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId);

    void RemoveCommissioningData(void);

    void RemoveRloc(uint16_t aRloc16, MatchMode aMatchMode, ChangedFlags &aChangedFlags);
    void RemoveRloc(uint16_t           aRloc16,
                    MatchMode          aMatchMode,
                    const NetworkData &aExcludeNetworkData,
                    ChangedFlags      &aChangedFlags);
    void RemoveRlocInPrefix(PrefixTlv       &aPrefix,
                            uint16_t         aRloc16,
                            MatchMode        aMatchMode,
                            const PrefixTlv *aExcludePrefix,
                            ChangedFlags    &aChangedFlags);
    void RemoveRlocInService(ServiceTlv       &aService,
                             uint16_t          aRloc16,
                             MatchMode         aMatchMode,
                             const ServiceTlv *aExcludeService,
                             ChangedFlags     &aChangedFlags);
    void RemoveRlocInHasRoute(PrefixTlv       &aPrefix,
                              HasRouteTlv     &aHasRoute,
                              uint16_t         aRloc16,
                              MatchMode        aMatchMode,
                              const PrefixTlv *aExcludePrefix,
                              ChangedFlags    &aChangedFlags);
    void RemoveRlocInBorderRouter(PrefixTlv       &aPrefix,
                                  BorderRouterTlv &aBorderRouter,
                                  uint16_t         aRloc16,
                                  MatchMode        aMatchMode,
                                  const PrefixTlv *aExcludePrefix,
                                  ChangedFlags    &aChangedFlags);

    static bool RlocMatch(uint16_t aFirstRloc16, uint16_t aSecondRloc16, MatchMode aMatchMode);

    static Error Validate(const NetworkData &aNetworkData, uint16_t aRloc16);
    static Error ValidatePrefix(const PrefixTlv &aPrefix, uint16_t aRloc16);
    static Error ValidateService(const ServiceTlv &aService, uint16_t aRloc16);

    static bool ContainsMatchingEntry(const PrefixTlv *aPrefix, bool aStable, const HasRouteEntry &aEntry);
    static bool ContainsMatchingEntry(const HasRouteTlv *aHasRoute, const HasRouteEntry &aEntry);
    static bool ContainsMatchingEntry(const PrefixTlv *aPrefix, bool aStable, const BorderRouterEntry &aEntry);
    static bool ContainsMatchingEntry(const BorderRouterTlv *aBorderRouter, const BorderRouterEntry &aEntry);
    static bool ContainsMatchingServer(const ServiceTlv *aService, const ServerTlv &aServer);

    UpdateStatus UpdatePrefix(PrefixTlv &aPrefix);
    UpdateStatus UpdateService(ServiceTlv &aService);
    UpdateStatus UpdateTlv(NetworkDataTlv &aTlv, const NetworkDataTlv *aSubTlvs);

    Error UpdateCommissioningData(uint16_t aDataLength, CommissioningDataTlv *&aDataTlv);
    Error SetCommissioningData(const Message &aMessage);

    void SendCommissioningSetResponse(const Coap::Message     &aRequest,
                                      const Ip6::MessageInfo  &aMessageInfo,
                                      MeshCoP::StateTlv::State aState);
    void IncrementVersions(bool aIncludeStable);
    void IncrementVersions(const ChangedFlags &aFlags);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    void CheckForNetDataGettingFull(const NetworkData &aNetworkData, uint16_t aOldRloc16);
    void MarkAsClone(void);
#endif

    using UpdateTimer = TimerMilliIn<Leader, &Leader::HandleTimer>;
#endif // OPENTHREAD_FTD

    uint8_t mStableVersion;
    uint8_t mVersion;
    uint8_t mTlvBuffer[kMaxSize];
    uint8_t mMaxLength;

#if OPENTHREAD_FTD
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    bool mIsClone;
#endif
    bool        mWaitingForNetDataSync;
    ContextIds  mContextIds;
    UpdateTimer mTimer;
#endif
};

#if OPENTHREAD_FTD
DeclareTmfHandler(Leader, kUriServerData);
DeclareTmfHandler(Leader, kUriCommissionerGet);
DeclareTmfHandler(Leader, kUriCommissionerSet);
#endif

/**
 * @}
 */

} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_LEADER_HPP_
