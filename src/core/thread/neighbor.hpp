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
 *   This file includes definitions for a Thread `Neighbor`.
 */

#ifndef NEIGHBOR_HPP_
#define NEIGHBOR_HPP_

#include "openthread-core-config.h"

#include <openthread/thread_ftd.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/serial_number.hpp"
#include "common/timer.hpp"
#include "common/uptime.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6.hpp"
#include "radio/radio.hpp"
#include "radio/trel_link.hpp"
#include "thread/csl_tx_scheduler.hpp"
#include "thread/indirect_sender.hpp"
#include "thread/link_metrics_types.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/network_data_types.hpp"
#include "thread/radio_selector.hpp"
#include "thread/version.hpp"

namespace ot {

/**
 * Represents a Thread neighbor.
 *
 */
class Neighbor : public InstanceLocatorInit
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ,
                 public RadioSelector::NeighborInfo
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ,
                 public Trel::NeighborInfo
#endif
{
public:
    /**
     * Neighbor link states.
     *
     */
    enum State : uint8_t
    {
        kStateInvalid,            ///< Neighbor link is invalid
        kStateRestored,           ///< Neighbor is restored from non-volatile memory
        kStateParentRequest,      ///< Received an MLE Parent Request message
        kStateParentResponse,     ///< Received an MLE Parent Response message
        kStateChildIdRequest,     ///< Received an MLE Child ID Request message
        kStateLinkRequest,        ///< Sent an MLE Link Request message
        kStateChildUpdateRequest, ///< Sent an MLE Child Update Request message (trying to restore the child)
        kStateValid,              ///< Link is valid
    };

    /**
     * Defines state filters used for finding a neighbor or iterating through the child/neighbor table.
     *
     * Each filter definition accepts a subset of `State` values.
     *
     */
    enum StateFilter : uint8_t
    {
        kInStateValid,                     ///< Accept neighbor only in `kStateValid`.
        kInStateValidOrRestoring,          ///< Accept neighbor with `IsStateValidOrRestoring()` being `true`.
        kInStateChildIdRequest,            ///< Accept neighbor only in `Child:kStateChildIdRequest`.
        kInStateValidOrAttaching,          ///< Accept neighbor with `IsStateValidOrAttaching()` being `true`.
        kInStateInvalid,                   ///< Accept neighbor only in `kStateInvalid`.
        kInStateAnyExceptInvalid,          ///< Accept neighbor in any state except `kStateInvalid`.
        kInStateAnyExceptValidOrRestoring, ///< Accept neighbor in any state except `IsStateValidOrRestoring()`.
        kInStateAny,                       ///< Accept neighbor in any state.
    };

    /**
     * Represents an Address Matcher used to find a neighbor (child/router) with a given MAC address also
     * matching a given state filter.
     *
     */
    class AddressMatcher
    {
    public:
        /**
         * Initializes the `AddressMatcher` with a given MAC short address (RCOC16) and state filter.
         *
         * @param[in]  aShortAddress   A MAC short address (RLOC16).
         * @param[in]  aStateFilter    A state filter.
         *
         */
        AddressMatcher(Mac::ShortAddress aShortAddress, StateFilter aStateFilter)
            : AddressMatcher(aStateFilter, aShortAddress, nullptr)
        {
        }

        /**
         * Initializes the `AddressMatcher` with a given MAC extended address and state filter.
         *
         * @param[in]  aExtAddress     A MAC extended address.
         * @param[in]  aStateFilter    A state filter.
         *
         */
        AddressMatcher(const Mac::ExtAddress &aExtAddress, StateFilter aStateFilter)
            : AddressMatcher(aStateFilter, Mac::kShortAddrInvalid, &aExtAddress)
        {
        }

        /**
         * Initializes the `AddressMatcher` with a given MAC address and state filter.
         *
         * @param[in]  aMacAddress     A MAC address.
         * @param[in]  aStateFilter    A state filter.
         *
         */
        AddressMatcher(const Mac::Address &aMacAddress, StateFilter aStateFilter)
            : AddressMatcher(aStateFilter,
                             aMacAddress.IsShort() ? aMacAddress.GetShort()
                                                   : static_cast<Mac::ShortAddress>(Mac::kShortAddrInvalid),
                             aMacAddress.IsExtended() ? &aMacAddress.GetExtended() : nullptr)
        {
        }

        /**
         * Initializes the `AddressMatcher` with a given state filter (it accepts any address).
         *
         * @param[in]  aStateFilter    A state filter.
         *
         */
        explicit AddressMatcher(StateFilter aStateFilter)
            : AddressMatcher(aStateFilter, Mac::kShortAddrInvalid, nullptr)
        {
        }

        /**
         * Indicates if a given neighbor matches the address and state filter of `AddressMatcher`.
         *
         * @param[in] aNeighbor   A neighbor.
         *
         * @retval TRUE   Neighbor @p aNeighbor matches the address and state filter.
         * @retval FALSE  Neighbor @p aNeighbor does not match the address or state filter.
         *
         */
        bool Matches(const Neighbor &aNeighbor) const;

    private:
        AddressMatcher(StateFilter aStateFilter, Mac::ShortAddress aShortAddress, const Mac::ExtAddress *aExtAddress)
            : mStateFilter(aStateFilter)
            , mShortAddress(aShortAddress)
            , mExtAddress(aExtAddress)
        {
        }

        StateFilter            mStateFilter;
        Mac::ShortAddress      mShortAddress;
        const Mac::ExtAddress *mExtAddress;
    };

    /**
     * Represents diagnostic information for a neighboring node.
     *
     */
    class Info : public otNeighborInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Neighbor`.
         *
         * @param[in] aNeighbor   A neighbor.
         *
         */
        void SetFrom(const Neighbor &aNeighbor);
    };

    /**
     * Returns the current state.
     *
     * @returns The current state.
     *
     */
    State GetState(void) const { return static_cast<State>(mState); }

    /**
     * Sets the current state.
     *
     * @param[in]  aState  The state value.
     *
     */
    void SetState(State aState);

    /**
     * Indicates whether the neighbor is in the Invalid state.
     *
     * @returns TRUE if the neighbor is in the Invalid state, FALSE otherwise.
     *
     */
    bool IsStateInvalid(void) const { return (mState == kStateInvalid); }

    /**
     * Indicates whether the neighbor is in the Child ID Request state.
     *
     * @returns TRUE if the neighbor is in the Child ID Request state, FALSE otherwise.
     *
     */
    bool IsStateChildIdRequest(void) const { return (mState == kStateChildIdRequest); }

    /**
     * Indicates whether the neighbor is in the Link Request state.
     *
     * @returns TRUE if the neighbor is in the Link Request state, FALSE otherwise.
     *
     */
    bool IsStateLinkRequest(void) const { return (mState == kStateLinkRequest); }

    /**
     * Indicates whether the neighbor is in the Parent Response state.
     *
     * @returns TRUE if the neighbor is in the Parent Response state, FALSE otherwise.
     *
     */
    bool IsStateParentResponse(void) const { return (mState == kStateParentResponse); }

    /**
     * Indicates whether the neighbor is being restored.
     *
     * @returns TRUE if the neighbor is being restored, FALSE otherwise.
     *
     */
    bool IsStateRestoring(void) const { return (mState == kStateRestored) || (mState == kStateChildUpdateRequest); }

    /**
     * Indicates whether the neighbor is in the Restored state.
     *
     * @returns TRUE if the neighbor is in the Restored state, FALSE otherwise.
     *
     */
    bool IsStateRestored(void) const { return (mState == kStateRestored); }

    /**
     * Indicates whether the neighbor is valid (frame counters are synchronized).
     *
     * @returns TRUE if the neighbor is valid, FALSE otherwise.
     *
     */
    bool IsStateValid(void) const { return (mState == kStateValid); }

    /**
     * Indicates whether the neighbor is in valid state or if it is being restored.
     *
     * When in these states messages can be sent to and/or received from the neighbor.
     *
     * @returns TRUE if the neighbor is in valid, restored, or being restored states, FALSE otherwise.
     *
     */
    bool IsStateValidOrRestoring(void) const { return (mState == kStateValid) || IsStateRestoring(); }

    /**
     * Indicates if the neighbor state is valid, attaching, or restored.
     *
     * The states `kStateRestored`, `kStateChildIdRequest`, `kStateChildUpdateRequest`, `kStateValid`, and
     * `kStateLinkRequest` are considered as valid, attaching, or restored.
     *
     * @returns TRUE if the neighbor state is valid, attaching, or restored, FALSE otherwise.
     *
     */
    bool IsStateValidOrAttaching(void) const;

    /**
     * Indicates whether neighbor state matches a given state filter.
     *
     * @param[in] aFilter   A state filter (`StateFilter` enumeration) to match against.
     *
     * @returns TRUE if the neighbor state matches the filter, FALSE otherwise.
     *
     */
    bool MatchesFilter(StateFilter aFilter) const;

    /**
     * Indicates whether neighbor matches a given `AddressMatcher`.
     *
     * @param[in]  aMatcher   An `AddressMatcher` to match against.
     *
     * @returns TRUE if the neighbor matches the address and state filter of @p aMatcher, FALSE otherwise.
     *
     */
    bool Matches(const AddressMatcher &aMatcher) const { return aMatcher.Matches(*this); }

    /**
     * Gets the device mode flags.
     *
     * @returns The device mode flags.
     *
     */
    Mle::DeviceMode GetDeviceMode(void) const { return Mle::DeviceMode(mMode); }

    /**
     * Sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     *
     */
    void SetDeviceMode(Mle::DeviceMode aMode) { mMode = aMode.Get(); }

    /**
     * Indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     *
     */
    bool IsRxOnWhenIdle(void) const { return GetDeviceMode().IsRxOnWhenIdle(); }

    /**
     * Indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     *
     */
    bool IsFullThreadDevice(void) const { return GetDeviceMode().IsFullThreadDevice(); }

    /**
     * Gets the Network Data type (full set or stable subset) that the device requests.
     *
     * @returns The Network Data type.
     *
     */
    NetworkData::Type GetNetworkDataType(void) const { return GetDeviceMode().GetNetworkDataType(); }

    /**
     * Returns the Extended Address.
     *
     * @returns A const reference to the Extended Address.
     *
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mMacAddr; }

    /**
     * Returns the Extended Address.
     *
     * @returns A reference to the Extended Address.
     *
     */
    Mac::ExtAddress &GetExtAddress(void) { return mMacAddr; }

    /**
     * Sets the Extended Address.
     *
     * @param[in]  aAddress  The Extended Address value to set.
     *
     */
    void SetExtAddress(const Mac::ExtAddress &aAddress) { mMacAddr = aAddress; }

    /**
     * Gets the key sequence value.
     *
     * @returns The key sequence value.
     *
     */
    uint32_t GetKeySequence(void) const { return mKeySequence; }

    /**
     * Sets the key sequence value.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     */
    void SetKeySequence(uint32_t aKeySequence) { mKeySequence = aKeySequence; }

    /**
     * Returns the last heard time.
     *
     * @returns The last heard time.
     *
     */
    TimeMilli GetLastHeard(void) const { return mLastHeard; }

    /**
     * Sets the last heard time.
     *
     * @param[in]  aLastHeard  The last heard time.
     *
     */
    void SetLastHeard(TimeMilli aLastHeard) { mLastHeard = aLastHeard; }

    /**
     * Gets the link frame counters.
     *
     * @returns A reference to `Mac::LinkFrameCounters` containing link frame counter for all supported radio links.
     *
     */
    Mac::LinkFrameCounters &GetLinkFrameCounters(void) { return mValidPending.mValid.mLinkFrameCounters; }

    /**
     * Gets the link frame counters.
     *
     * @returns A reference to `Mac::LinkFrameCounters` containing link frame counter for all supported radio links.
     *
     */
    const Mac::LinkFrameCounters &GetLinkFrameCounters(void) const { return mValidPending.mValid.mLinkFrameCounters; }

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    /**
     * Gets the link ACK frame counter value.
     *
     * @returns The link ACK frame counter value.
     *
     */
    uint32_t GetLinkAckFrameCounter(void) const { return mValidPending.mValid.mLinkAckFrameCounter; }
#endif

    /**
     * Sets the link ACK frame counter value.
     *
     * @param[in]  aAckFrameCounter  The link ACK frame counter value.
     *
     */
    void SetLinkAckFrameCounter(uint32_t aAckFrameCounter)
    {
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        mValidPending.mValid.mLinkAckFrameCounter = aAckFrameCounter;
#else
        OT_UNUSED_VARIABLE(aAckFrameCounter);
#endif
    }

    /**
     * Gets the MLE frame counter value.
     *
     * @returns The MLE frame counter value.
     *
     */
    uint32_t GetMleFrameCounter(void) const { return mValidPending.mValid.mMleFrameCounter; }

    /**
     * Sets the MLE frame counter value.
     *
     * @param[in]  aFrameCounter  The MLE frame counter value.
     *
     */
    void SetMleFrameCounter(uint32_t aFrameCounter) { mValidPending.mValid.mMleFrameCounter = aFrameCounter; }

    /**
     * Gets the RLOC16 value.
     *
     * @returns The RLOC16 value.
     *
     */
    uint16_t GetRloc16(void) const { return mRloc16; }

    /**
     * Gets the Router ID value.
     *
     * @returns The Router ID value.
     *
     */
    uint8_t GetRouterId(void) const { return mRloc16 >> Mle::kRouterIdOffset; }

    /**
     * Sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = aRloc16; }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    /**
     * Clears the last received fragment tag.
     *
     * The last received fragment tag is used for detect duplicate frames (received over different radios) when
     * multi-radio feature is enabled.
     *
     */
    void ClearLastRxFragmentTag(void) { mLastRxFragmentTag = 0; }

    /**
     * Gets the last received fragment tag.
     *
     * MUST be used only when the tag is set (and not cleared). Otherwise its behavior is undefined.
     *
     * @returns The last received fragment tag.
     *
     */
    uint16_t GetLastRxFragmentTag(void) const { return mLastRxFragmentTag; }

    /**
     * Set the last received fragment tag.
     *
     * @param[in] aTag   The new tag value.
     *
     */
    void SetLastRxFragmentTag(uint16_t aTag);

    /**
     * Indicates whether or not the last received fragment tag is set and valid (i.e., not yet timed out).
     *
     * @returns TRUE if the last received fragment tag is set and valid, FALSE otherwise.
     *
     */
    bool IsLastRxFragmentTagSet(void) const;

    /**
     * Indicates whether the last received fragment tag is strictly after a given tag value.
     *
     * MUST be used only when the tag is set (and not cleared). Otherwise its behavior is undefined.
     *
     * The tag value compassion follows the Serial Number Arithmetic logic from RFC-1982. It is semantically equivalent
     * to `LastRxFragmentTag > aTag`.
     *
     * @param[in] aTag   A tag value to compare against.
     *
     * @returns TRUE if the current last rx fragment tag is strictly after @p aTag, FALSE if they are equal or it is
     * before @p aTag.
     *
     */
    bool IsLastRxFragmentTagAfter(uint16_t aTag) const { return SerialNumber::IsGreater(mLastRxFragmentTag, aTag); }

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Indicates whether or not it is Thread 1.1.
     *
     * @returns TRUE if neighbors is Thread 1.1, FALSE otherwise.
     *
     */
    bool IsThreadVersion1p1(void) const { return mState != kStateInvalid && mVersion == kThreadVersion1p1; }

    /**
     * Indicates whether or not neighbor is Thread 1.2 or higher..
     *
     * @returns TRUE if neighbor is Thread 1.2 or higher, FALSE otherwise.
     *
     */
    bool IsThreadVersion1p2OrHigher(void) const { return mState != kStateInvalid && mVersion >= kThreadVersion1p2; }

    /**
     * Indicates whether Thread version supports CSL.
     *
     * @returns TRUE if CSL is supported, FALSE otherwise.
     *
     */
    bool IsThreadVersionCslCapable(void) const { return IsThreadVersion1p2OrHigher() && !IsRxOnWhenIdle(); }

    /**
     * Indicates whether Enhanced Keep-Alive is supported or not.
     *
     * @returns TRUE if Enhanced Keep-Alive is supported, FALSE otherwise.
     *
     */
    bool IsEnhancedKeepAliveSupported(void) const
    {
        return (mState != kStateInvalid) && (mVersion >= kThreadVersion1p2);
    }

    /**
     * Gets the device MLE version.
     *
     */
    uint16_t GetVersion(void) const { return mVersion; }

    /**
     * Sets the device MLE version.
     *
     * @param[in]  aVersion  The device MLE version.
     *
     */
    void SetVersion(uint16_t aVersion) { mVersion = aVersion; }

    /**
     * Gets the number of consecutive link failures.
     *
     * @returns The number of consecutive link failures.
     *
     */
    uint8_t GetLinkFailures(void) const { return mLinkFailures; }

    /**
     * Increments the number of consecutive link failures.
     *
     */
    void IncrementLinkFailures(void) { mLinkFailures++; }

    /**
     * Resets the number of consecutive link failures to zero.
     *
     */
    void ResetLinkFailures(void) { mLinkFailures = 0; }

    /**
     * Returns the LinkQualityInfo object.
     *
     * @returns The LinkQualityInfo object.
     *
     */
    LinkQualityInfo &GetLinkInfo(void) { return mLinkInfo; }

    /**
     * Returns the LinkQualityInfo object.
     *
     * @returns The LinkQualityInfo object.
     *
     */
    const LinkQualityInfo &GetLinkInfo(void) const { return mLinkInfo; }

    /**
     * Gets the link quality in value.
     *
     * @returns The link quality in value.
     *
     */
    LinkQuality GetLinkQualityIn(void) const { return GetLinkInfo().GetLinkQuality(); }

    /**
     * Generates a new challenge value for MLE Link Request/Response exchanges.
     *
     */
    void GenerateChallenge(void) { mValidPending.mPending.mChallenge.GenerateRandom(); }

    /**
     * Returns the current challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The current challenge value.
     *
     */
    const Mle::TxChallenge &GetChallenge(void) const { return mValidPending.mPending.mChallenge; }

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    /**
     * Returns the connection time (in seconds) of the neighbor (seconds since entering `kStateValid`).
     *
     * @returns The connection time (in seconds), zero if device is not currently in `kStateValid`.
     *
     */
    uint32_t GetConnectionTime(void) const;
#endif

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Indicates whether or not time sync feature is enabled.
     *
     * @returns TRUE if time sync feature is enabled, FALSE otherwise.
     *
     */
    bool IsTimeSyncEnabled(void) const { return mTimeSyncEnabled; }

    /**
     * Sets whether or not time sync feature is enabled.
     *
     * @param[in]  aEnable    TRUE if time sync feature is enabled, FALSE otherwise.
     *
     */
    void SetTimeSyncEnabled(bool aEnabled) { mTimeSyncEnabled = aEnabled; }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    /**
     * Aggregates the Link Metrics data into all the series that is running for this neighbor.
     *
     * If a series wants to account frames of @p aFrameType, it would add count by 1 and aggregate @p aLqi and
     * @p aRss into its averagers.
     *
     * @param[in] aSeriesId     Series ID for Link Probe. Should be `0` if this method is not called by Link Probe.
     * @param[in] aFrameType    Type of the frame that carries Link Metrics data.
     * @param[in] aLqi          The LQI value.
     * @param[in] aRss          The Rss value.
     *
     */
    void AggregateLinkMetrics(uint8_t aSeriesId, uint8_t aFrameType, uint8_t aLqi, int8_t aRss);

    /**
     * Adds a new LinkMetrics::SeriesInfo to the neighbor's list.
     *
     * @param[in]  aSeriesInfo  A reference to the new SeriesInfo.
     *
     */
    void AddForwardTrackingSeriesInfo(LinkMetrics::SeriesInfo &aSeriesInfo);

    /**
     * Finds a specific LinkMetrics::SeriesInfo by Series ID.
     *
     * @param[in] aSeriesId    A reference to the Series ID.
     *
     * @returns The pointer to the LinkMetrics::SeriesInfo. `nullptr` if not found.
     *
     */
    LinkMetrics::SeriesInfo *GetForwardTrackingSeriesInfo(const uint8_t &aSeriesId);

    /**
     * Removes a specific LinkMetrics::SeriesInfo by Series ID.
     *
     * @param[in] aSeriesId    A reference to the Series ID to remove.
     *
     * @returns The pointer to the LinkMetrics::SeriesInfo. `nullptr` if not found.
     *
     */
    LinkMetrics::SeriesInfo *RemoveForwardTrackingSeriesInfo(const uint8_t &aSeriesId);

    /**
     * Removes all the Series and return the data structures to the Pool.
     *
     */
    void RemoveAllForwardTrackingSeriesInfo(void);

    /**
     * Gets the Enh-ACK Probing metrics (this `Neighbor` object is the Probing Subject).
     *
     * @returns Enh-ACK Probing metrics configured.
     *
     */
    const LinkMetrics::Metrics &GetEnhAckProbingMetrics(void) const { return mEnhAckProbingMetrics; }

    /**
     * Sets the Enh-ACK Probing metrics (this `Neighbor` object is the Probing Subject).
     *
     * @param[in]  aEnhAckProbingMetrics  The metrics value to set.
     *
     */
    void SetEnhAckProbingMetrics(const LinkMetrics::Metrics &aEnhAckProbingMetrics)
    {
        mEnhAckProbingMetrics = aEnhAckProbingMetrics;
    }

    /**
     * Indicates if Enh-ACK Probing is configured and active for this `Neighbor` object.
     *
     * @retval TRUE   Enh-ACK Probing is configured and active for this `Neighbor`.
     * @retval FALSE  Otherwise.
     *
     */
    bool IsEnhAckProbingActive(void) const
    {
        return (mEnhAckProbingMetrics.mLqi != 0) || (mEnhAckProbingMetrics.mLinkMargin != 0) ||
               (mEnhAckProbingMetrics.mRssi != 0);
    }
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

    /**
     * Converts a given `State` to a human-readable string.
     *
     * @param[in] aState   A neighbor state.
     *
     * @returns A string representation of given state.
     *
     */
    static const char *StateToString(State aState);

protected:
    /**
     * Initializes the `Neighbor` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance);

private:
    enum : uint32_t
    {
        kLastRxFragmentTagTimeout = OPENTHREAD_CONFIG_MULTI_RADIO_FRAG_TAG_TIMEOUT, ///< Frag tag timeout in msec.
    };

    Mac::ExtAddress mMacAddr;   ///< The IEEE 802.15.4 Extended Address
    TimeMilli       mLastHeard; ///< Time when last heard.
    union
    {
        struct
        {
            Mac::LinkFrameCounters mLinkFrameCounters; ///< The Link Frame Counters
            uint32_t               mMleFrameCounter;   ///< The MLE Frame Counter
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
            uint32_t mLinkAckFrameCounter; ///< The Link Ack Frame Counter
#endif
        } mValid;
        struct
        {
            Mle::TxChallenge mChallenge; ///< The challenge value
        } mPending;
    } mValidPending;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    uint16_t  mLastRxFragmentTag;     ///< Last received fragment tag
    TimeMilli mLastRxFragmentTagTime; ///< The time last fragment tag was received and set.
#endif

    uint32_t mKeySequence; ///< Current key sequence
    uint16_t mRloc16;      ///< The RLOC16
    uint8_t  mState : 4;   ///< The link state
    uint8_t  mMode : 4;    ///< The MLE device mode
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    uint8_t mLinkFailures : 7;    ///< Consecutive link failure count
    bool    mTimeSyncEnabled : 1; ///< Indicates whether or not time sync feature is enabled.
#else
    uint8_t mLinkFailures; ///< Consecutive link failure count
#endif
    uint16_t        mVersion;  ///< The MLE version
    LinkQualityInfo mLinkInfo; ///< Link quality info (contains average RSS, link margin and link quality)
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    // A list of Link Metrics Forward Tracking Series that is being
    // tracked for this neighbor. Note that this device is the
    // Subject and this neighbor is the Initiator.
    LinkedList<LinkMetrics::SeriesInfo> mLinkMetricsSeriesInfoList;

    // Metrics configured for Enh-ACK Based Probing at the Probing
    // Subject (this neighbor). Note that this device is the Initiator
    // and this neighbor is the Subject.
    LinkMetrics::Metrics mEnhAckProbingMetrics;
#endif
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    uint32_t mConnectionStart;
#endif
};

DefineCoreType(otNeighborInfo, Neighbor::Info);

} // namespace ot

#endif // NEIGHBOR_HPP_
