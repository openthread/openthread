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

#ifndef NETWORK_DATA_LEADER_FTD_HPP_
#define NETWORK_DATA_LEADER_FTD_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include <stdint.h>

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
 *
 */

/**
 * This class implements the Thread Network Data maintained by the Leader.
 *
 */
class Leader : public LeaderBase, private NonCopyable
{
    friend class Tmf::Agent;

public:
    /**
     * This enumeration defines the match mode constants to compare two RLOC16 values.
     *
     */
    enum MatchMode : uint8_t
    {
        kMatchModeRloc16,   ///< Perform exact RLOC16 match.
        kMatchModeRouterId, ///< Perform Router ID match (match the router and any of its children).
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Leader(Instance &aInstance);

    /**
     * This method reset the Thread Network Data.
     *
     */
    void Reset(void);

    /**
     * This method starts the Leader services.
     *
     * The start mode indicates whether device is starting normally as leader or restoring its role as leader after
     * reset. In the latter case, we do not accept any new registrations (`HandleServerData()`) and wait for
     * `HandleNetworkDataRestoredAfterReset()` to indicate that the leader has successfully recovered the Network Data
     * before allowing new Network Data registrations.
     *
     * @param[in] aStartMode   The start mode.
     *
     */
    void Start(Mle::LeaderStartMode aStartMode);

    /**
     * This method increments the Thread Network Data version.
     *
     */
    void IncrementVersion(void);

    /**
     * This method increments both the Thread Network Data version and stable version.
     *
     */
    void IncrementVersionAndStableVersion(void);

    /**
     * This method returns CONTEXT_ID_RESUSE_DELAY value.
     *
     * @returns The CONTEXT_ID_REUSE_DELAY value (in seconds).
     *
     */
    uint32_t GetContextIdReuseDelay(void) const { return mContextIds.GetReuseDelay(); }

    /**
     * This method sets CONTEXT_ID_RESUSE_DELAY value.
     *
     * @warning This method should only be used for testing.
     *
     * @param[in]  aDelay  The CONTEXT_ID_REUSE_DELAY value (in seconds).
     *
     */
    void SetContextIdReuseDelay(uint32_t aDelay) { mContextIds.SetReuseDelay(aDelay); }

    /**
     * This method removes Network Data entries matching with a given RLOC16.
     *
     * @param[in]  aRloc16    A RLOC16 value.
     * @param[in]  aMatchMode A match mode (@sa MatchMode).
     *
     */
    void RemoveBorderRouter(uint16_t aRloc16, MatchMode aMatchMode);

    /**
     * This method synchronizes internal 6LoWPAN Context ID Set with recently obtained Thread Network Data.
     *
     * Note that this method should be called only by the Leader once after reset.
     *
     */
    void HandleNetworkDataRestoredAfterReset(void);

    /**
     * This method scans network data for given Service ID and returns pointer to the respective TLV, if present.
     *
     * @param aServiceId Service ID to look for.
     * @return Pointer to the Service TLV for given Service ID, or nullptr if not present.
     *
     */
    const ServiceTlv *FindServiceById(uint8_t aServiceId) const;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    /**
     * This method indicates whether a given Prefix can act as a valid OMR prefix and exists in the network data.
     *
     * @param[in]  aPrefix   The OMR prefix to check.
     *
     * @retval TRUE  If @p aPrefix is a valid OMR prefix and Network Data contains @p aPrefix.
     * @retval FALSE Otherwise.
     *
     */
    bool ContainsOmrPrefix(const Ip6::Prefix &aPrefix);
#endif

private:
    static constexpr uint32_t kMaxNetDataSyncWait = 60 * 1000; // Maximum time to wait for netdata sync in msec.

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

        explicit ContextIds(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mReuseDelay(kReuseDelay)
        {
        }

        void     Clear(void);
        Error    GetUnallocatedId(uint8_t &aId);
        void     MarkAsInUse(uint8_t aId) { mRemoveTimes[aId - kMinId].SetValue(kInUse); }
        void     ScheduleToRemove(uint8_t aId);
        uint32_t GetReuseDelay(void) const { return mReuseDelay; }
        void     SetReuseDelay(uint32_t aDelay) { mReuseDelay = aDelay; }
        void     HandleTimer(void);

    private:
        static constexpr uint32_t kReuseDelay = 48 * 60 * 60; // in seconds

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
    };

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleTimer(void);

    void RegisterNetworkData(uint16_t aRloc16, const NetworkData &aNetworkData);

    Error AddPrefix(const PrefixTlv &aPrefix, ChangedFlags &aChangedFlags);
    Error AddHasRoute(const HasRouteTlv &aHasRoute, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags);
    Error AddBorderRouter(const BorderRouterTlv &aBorderRouter, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags);
    Error AddService(const ServiceTlv &aService, ChangedFlags &aChangedFlags);
    Error AddServer(const ServerTlv &aServer, ServiceTlv &aDstService, ChangedFlags &aChangedFlags);

    Error AllocateServiceId(uint8_t &aServiceId) const;

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

    void SendCommissioningGetResponse(const Coap::Message    &aRequest,
                                      uint16_t                aLength,
                                      const Ip6::MessageInfo &aMessageInfo);
    void SendCommissioningSetResponse(const Coap::Message     &aRequest,
                                      const Ip6::MessageInfo  &aMessageInfo,
                                      MeshCoP::StateTlv::State aState);
    void IncrementVersions(bool aIncludeStable);
    void IncrementVersions(const ChangedFlags &aFlags);

    using UpdateTimer = TimerMilliIn<Leader, &Leader::HandleTimer>;

    bool        mWaitingForNetDataSync;
    ContextIds  mContextIds;
    UpdateTimer mTimer;
};

DeclareTmfHandler(Leader, kUriServerData);
DeclareTmfHandler(Leader, kUriCommissionerGet);
DeclareTmfHandler(Leader, kUriCommissionerSet);

/**
 * @}
 */

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD

#endif // NETWORK_DATA_LEADER_FTD_HPP_
