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
 *   This file includes definitions for maintaining Thread network topologies.
 */

#ifndef TOPOLOGY_HPP_
#define TOPOLOGY_HPP_

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
#include "thread/link_metrics.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/network_data_types.hpp"
#include "thread/radio_selector.hpp"
#include "thread/version.hpp"

namespace ot {

/**
 * This class represents a Thread neighbor.
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
     * This class represents an Address Matcher used to find a neighbor (child/router) with a given MAC address also
     * matching a given state filter.
     *
     */
    class AddressMatcher
    {
    public:
        /**
         * This constructor initializes the `AddressMatcher` with a given MAC short address (RCOC16) and state filter.
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
         * This constructor initializes the `AddressMatcher` with a given MAC extended address and state filter.
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
         * This constructor initializes the `AddressMatcher` with a given MAC address and state filter.
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
         * This constructor initializes the `AddressMatcher` with a given state filter (it accepts any address).
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
     * This type represents diagnostic information for a neighboring node.
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
    void GenerateChallenge(void);

    /**
     * Returns the current challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The current challenge value.
     *
     */
    const uint8_t *GetChallenge(void) const { return mValidPending.mPending.mChallenge; }

    /**
     * Returns the size (bytes) of the challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The size (bytes) of the challenge value for MLE Link Request/Response exchanges.
     *
     */
    uint8_t GetChallengeSize(void) const { return sizeof(mValidPending.mPending.mChallenge); }

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
            uint8_t mChallenge[Mle::kMaxChallengeSize]; ///< The challenge value
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

#if OPENTHREAD_FTD

/**
 * This class represents a Thread Child.
 *
 */
class Child : public Neighbor,
              public IndirectSender::ChildInfo,
              public DataPollHandler::ChildInfo
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ,
              public CslTxScheduler::ChildInfo
#endif
{
    class AddressIteratorBuilder;

public:
    static constexpr uint8_t kMaxRequestTlvs = 6;

    /**
     * This class represents diagnostic information for a Thread Child.
     *
     */
    class Info : public otChildInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Child`.
         *
         * @param[in] aChild   A neighbor.
         *
         */
        void SetFrom(const Child &aChild);
    };

    /**
     * This class defines an iterator used to go through IPv6 address entries of a child.
     *
     */
    class AddressIterator : public Unequatable<AddressIterator>
    {
        friend class AddressIteratorBuilder;

    public:
        /**
         * This type represents an index indicating the current IPv6 address entry to which the iterator is pointing.
         *
         */
        typedef otChildIp6AddressIterator Index;

        /**
         * This constructor initializes the iterator associated with a given `Child` starting from beginning of the
         * IPv6 address list.
         *
         * @param[in] aChild    A reference to a child entry.
         * @param[in] aFilter   An IPv6 address type filter restricting iterator to certain type of addresses.
         *
         */
        explicit AddressIterator(const Child &aChild, Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny)
            : AddressIterator(aChild, 0, aFilter)
        {
        }

        /**
         * This constructor initializes the iterator associated with a given `Child` starting from a given index
         *
         * @param[in]  aChild   A reference to the child entry.
         * @param[in]  aIndex   An index (`Index`) with which to initialize the iterator.
         * @param[in]  aFilter  An IPv6 address type filter restricting iterator to certain type of addresses.
         *
         */
        AddressIterator(const Child &aChild, Index aIndex, Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny)
            : mChild(aChild)
            , mFilter(aFilter)
            , mIndex(aIndex)
        {
            Update();
        }

        /**
         * Converts the iterator into an index.
         *
         * @returns An index corresponding to the iterator.
         *
         */
        Index GetAsIndex(void) const { return mIndex; }

        /**
         * Gets the iterator's associated `Child` entry.
         *
         * @returns The associated child entry.
         *
         */
        const Child &GetChild(void) const { return mChild; }

        /**
         * Gets the current `Child` IPv6 Address to which the iterator is pointing.
         *
         * @returns  A pointer to the associated IPv6 Address, or `nullptr` if iterator is done.
         *
         */
        const Ip6::Address *GetAddress(void) const;

        /**
         * Indicates whether the iterator has reached end of the list.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) const { return (mIndex >= kMaxIndex); }

        /**
         * Overloads `++` operator (pre-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Address` entry.  If there are no more `Ip6::Address` entries
         * `IsDone()` returns `true`.
         *
         */
        void operator++(void) { mIndex++, Update(); }

        /**
         * Overloads `++` operator (post-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Address` entry.  If there are no more `Ip6::Address` entries
         *  `IsDone()` returns `true`.
         *
         */
        void operator++(int) { mIndex++, Update(); }

        /**
         * Overloads the `*` dereference operator and gets a reference to `Ip6::Address` to which the
         * iterator is currently pointing.
         *
         * MUST be used when the iterator is not done (i.e., `IsDone()` returns `false`).
         *
         * @returns A reference to the `Ip6::Address` entry currently pointed by the iterator.
         *
         */
        const Ip6::Address &operator*(void) const { return *GetAddress(); }

        /**
         * Overloads operator `==` to evaluate whether or not two `Iterator` instances are equal.
         *
         * MUST be used when the two iterators are associated with the same `Child` entry.
         *
         * @param[in]  aOther  The other `Iterator` to compare with.
         *
         * @retval TRUE   If the two `Iterator` objects are equal.
         * @retval FALSE  If the two `Iterator` objects are not equal.
         *
         */
        bool operator==(const AddressIterator &aOther) const { return (mIndex == aOther.mIndex); }

    private:
        enum IteratorType : uint8_t
        {
            kEndIterator,
        };

        static constexpr uint16_t kMaxIndex = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD;

        AddressIterator(const Child &aChild, IteratorType)
            : mChild(aChild)
            , mIndex(kMaxIndex)
        {
        }

        void Update(void);

        const Child             &mChild;
        Ip6::Address::TypeFilter mFilter;
        Index                    mIndex;
        Ip6::Address             mMeshLocalAddress;
    };

    /**
     * Initializes the `Child` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * Clears the child entry.
     *
     */
    void Clear(void);

    /**
     * Clears the IPv6 address list for the child.
     *
     */
    void ClearIp6Addresses(void);

    /**
     * Sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     *
     */
    void SetDeviceMode(Mle::DeviceMode aMode);

    /**
     * Gets the mesh-local IPv6 address.
     *
     * @param[out]   aAddress            A reference to an IPv6 address to provide address (if any).
     *
     * @retval kErrorNone      Successfully found the mesh-local address and updated @p aAddress.
     * @retval kErrorNotFound  No mesh-local IPv6 address in the IPv6 address list.
     *
     */
    Error GetMeshLocalIp6Address(Ip6::Address &aAddress) const;

    /**
     * Returns the Mesh Local Interface Identifier.
     *
     * @returns The Mesh Local Interface Identifier.
     *
     */
    const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mMeshLocalIid; }

    /**
     * Enables range-based `for` loop iteration over all (or a subset of) IPv6 addresses.
     *
     * Should be used as follows: to iterate over all addresses
     *
     *     for (const Ip6::Address &address : child.IterateIp6Addresses()) { ... }
     *
     * or to iterate over a subset of IPv6 addresses determined by a given address type filter
     *
     *     for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticast)) { ... }
     *
     * @param[in] aFilter  An IPv6 address type filter restricting iteration to certain type of addresses (default is
     *                     to accept any address type).
     *
     * @returns An IteratorBuilder instance.
     *
     */
    AddressIteratorBuilder IterateIp6Addresses(Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny) const
    {
        return AddressIteratorBuilder(*this, aFilter);
    }

    /**
     * Adds an IPv6 address to the list.
     *
     * @param[in]  aAddress           A reference to IPv6 address to be added.
     *
     * @retval kErrorNone          Successfully added the new address.
     * @retval kErrorAlready       Address is already in the list.
     * @retval kErrorNoBufs        Already at maximum number of addresses. No entry available to add the new address.
     * @retval kErrorInvalidArgs   Address is invalid (it is the Unspecified Address).
     *
     */
    Error AddIp6Address(const Ip6::Address &aAddress);

    /**
     * Removes an IPv6 address from the list.
     *
     * @param[in]  aAddress               A reference to IPv6 address to be removed.
     *
     * @retval kErrorNone             Successfully removed the address.
     * @retval kErrorNotFound         Address was not found in the list.
     * @retval kErrorInvalidArgs      Address is invalid (it is the Unspecified Address).
     *
     */
    Error RemoveIp6Address(const Ip6::Address &aAddress);

    /**
     * Indicates whether an IPv6 address is in the list of IPv6 addresses of the child.
     *
     * @param[in]  aAddress   A reference to IPv6 address.
     *
     * @retval TRUE           The address exists on the list.
     * @retval FALSE          Address was not found in the list.
     *
     */
    bool HasIp6Address(const Ip6::Address &aAddress) const;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    /**
     * Retrieves the Domain Unicast Address registered by the child.
     *
     * @returns A pointer to Domain Unicast Address registered by the child if there is.
     *
     */
    const Ip6::Address *GetDomainUnicastAddress(void) const;
#endif

    /**
     * Gets the child timeout.
     *
     * @returns The child timeout.
     *
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * Sets the child timeout.
     *
     * @param[in]  aTimeout  The child timeout.
     *
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = aTimeout; }

    /**
     * Gets the network data version.
     *
     * @returns The network data version.
     *
     */
    uint8_t GetNetworkDataVersion(void) const { return mNetworkDataVersion; }

    /**
     * Sets the network data version.
     *
     * @param[in]  aVersion  The network data version.
     *
     */
    void SetNetworkDataVersion(uint8_t aVersion) { mNetworkDataVersion = aVersion; }

    /**
     * Generates a new challenge value to use during a child attach.
     *
     */
    void GenerateChallenge(void);

    /**
     * Gets the current challenge value used during attach.
     *
     * @returns The current challenge value.
     *
     */
    const uint8_t *GetChallenge(void) const { return mAttachChallenge; }

    /**
     * Gets the challenge size (bytes) used during attach.
     *
     * @returns The challenge size (bytes).
     *
     */
    uint8_t GetChallengeSize(void) const { return sizeof(mAttachChallenge); }

    /**
     * Clears the requested TLV list.
     *
     */
    void ClearRequestTlvs(void) { memset(mRequestTlvs, Mle::Tlv::kInvalid, sizeof(mRequestTlvs)); }

    /**
     * Returns the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     *
     * @returns The requested TLV at index @p aIndex.
     *
     */
    uint8_t GetRequestTlv(uint8_t aIndex) const { return mRequestTlvs[aIndex]; }

    /**
     * Sets the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     * @param[in]  aType   The TLV type.
     *
     */
    void SetRequestTlv(uint8_t aIndex, uint8_t aType) { mRequestTlvs[aIndex] = aType; }

    /**
     * Returns the supervision interval (in seconds).
     *
     * @returns The supervision interval (in seconds).
     *
     */
    uint16_t GetSupervisionInterval(void) const { return mSupervisionInterval; }

    /**
     * Sets the supervision interval.
     *
     * @param[in] aInterval  The supervision interval (in seconds).
     *
     */
    void SetSupervisionInterval(uint16_t aInterval) { mSupervisionInterval = aInterval; }

    /**
     * Increments the number of seconds since last supervision of the child.
     *
     */
    void IncrementSecondsSinceLastSupervision(void) { mSecondsSinceSupervision++; }

    /**
     * Returns the number of seconds since last supervision of the child (last message to the child)
     *
     * @returns Number of seconds since last supervision of the child.
     *
     */
    uint16_t GetSecondsSinceLastSupervision(void) const { return mSecondsSinceSupervision; }

    /**
     * Resets the number of seconds since last supervision of the child to zero.
     *
     */
    void ResetSecondsSinceLastSupervision(void) { mSecondsSinceSupervision = 0; }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    /**
     * Returns MLR state of an IPv6 multicast address.
     *
     * @note The @p aAddress reference MUST be from `IterateIp6Addresses()` or `AddressIterator`.
     *
     * @param[in] aAddress  The IPv6 multicast address.
     *
     * @returns MLR state of the IPv6 multicast address.
     *
     */
    MlrState GetAddressMlrState(const Ip6::Address &aAddress) const;

    /**
     * Sets MLR state of an IPv6 multicast address.
     *
     * @note The @p aAddress reference MUST be from `IterateIp6Addresses()` or `AddressIterator`.
     *
     * @param[in] aAddress  The IPv6 multicast address.
     * @param[in] aState    The target MLR state.
     *
     */
    void SetAddressMlrState(const Ip6::Address &aAddress, MlrState aState);

    /**
     * Returns if the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     *
     * @param[in] aAddress  The IPv6 address.
     *
     * @retval true   If the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     *
     */
    bool HasMlrRegisteredAddress(const Ip6::Address &aAddress) const;

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateRegistered`.
     *
     */
    bool HasAnyMlrRegisteredAddress(void) const { return mMlrRegisteredMask.HasAny(); }

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateToRegister`.
     *
     */
    bool HasAnyMlrToRegisterAddress(void) const { return mMlrToRegisterMask.HasAny(); }
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

private:
#if OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD < 2
#error OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD should be at least set to 2.
#endif

    static constexpr uint16_t kNumIp6Addresses = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1;

    typedef BitVector<kNumIp6Addresses> ChildIp6AddressMask;

    class AddressIteratorBuilder
    {
    public:
        AddressIteratorBuilder(const Child &aChild, Ip6::Address::TypeFilter aFilter)
            : mChild(aChild)
            , mFilter(aFilter)
        {
        }

        AddressIterator begin(void) { return AddressIterator(mChild, mFilter); }
        AddressIterator end(void) { return AddressIterator(mChild, AddressIterator::kEndIterator); }

    private:
        const Child             &mChild;
        Ip6::Address::TypeFilter mFilter;
    };

    Ip6::InterfaceIdentifier mMeshLocalIid;                 ///< IPv6 address IID for mesh-local address
    Ip6::Address             mIp6Address[kNumIp6Addresses]; ///< Registered IPv6 addresses
    uint32_t                 mTimeout;                      ///< Child timeout

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    ChildIp6AddressMask mMlrToRegisterMask;
    ChildIp6AddressMask mMlrRegisteredMask;
#endif

    uint8_t mNetworkDataVersion; ///< Current Network Data version

    union
    {
        uint8_t mRequestTlvs[kMaxRequestTlvs];            ///< Requested MLE TLVs
        uint8_t mAttachChallenge[Mle::kMaxChallengeSize]; ///< The challenge value
    };

    uint16_t mSupervisionInterval;     // Supervision interval for the child (in sec).
    uint16_t mSecondsSinceSupervision; // Number of seconds since last supervision of the child.

    static_assert(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < 8192, "mQueuedMessageCount cannot fit max required!");
};

#endif // OPENTHREAD_FTD

class Parent;

/**
 * This class represents a Thread Router
 *
 */
class Router : public Neighbor
{
public:
    /**
     * This class represents diagnostic information for a Thread Router.
     *
     */
    class Info : public otRouterInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Router`.
         *
         * @param[in] aRouter   A router.
         *
         */
        void SetFrom(const Router &aRouter);

        /**
         * Sets the `Info` instance from a given `Parent`.
         *
         * @param[in] aParent   A parent.
         *
         */
        void SetFrom(const Parent &aParent);
    };

    /**
     * Initializes the `Router` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * Clears the router entry.
     *
     */
    void Clear(void);

    /**
     * Sets the `Router` entry from a `Parent`
     *
     */
    void SetFrom(const Parent &aParent);

    /**
     * Gets the router ID of the next hop to this router.
     *
     * @returns The router ID of the next hop to this router.
     *
     */
    uint8_t GetNextHop(void) const { return mNextHop; }

    /**
     * Gets the link quality out value for this router.
     *
     * @returns The link quality out value for this router.
     *
     */
    LinkQuality GetLinkQualityOut(void) const { return static_cast<LinkQuality>(mLinkQualityOut); }

    /**
     * Sets the link quality out value for this router.
     *
     * @param[in]  aLinkQuality  The link quality out value for this router.
     *
     */
    void SetLinkQualityOut(LinkQuality aLinkQuality) { mLinkQualityOut = aLinkQuality; }

    /**
     * Gets the two-way link quality value (minimum of link quality in and out).
     *
     * @returns The two-way link quality value.
     *
     */
    LinkQuality GetTwoWayLinkQuality(void) const;

    /**
     * Get the route cost to this router.
     *
     * @returns The route cost to this router.
     *
     */
    uint8_t GetCost(void) const { return mCost; }

    /**
     * Sets the next hop and cost to this router.
     *
     * @param[in]  aNextHop  The Router ID of the next hop to this router.
     * @param[in]  aCost     The cost to this router.
     *
     * @retval TRUE   If there was a change, i.e., @p aNextHop or @p aCost were different from their previous values.
     * @retval FALSE  If no change to next hop and cost values (new values are the same as before).
     *
     */
    bool SetNextHopAndCost(uint8_t aNextHop, uint8_t aCost);

    /**
     * Sets the next hop to this router as invalid and clears the cost.
     *
     * @retval TRUE   If there was a change (next hop was valid before).
     * @retval FALSE  No change to next hop (next hop was invalid before).
     *
     */
    bool SetNextHopToInvalid(void);

private:
    uint8_t mNextHop;            ///< The next hop towards this router
    uint8_t mLinkQualityOut : 2; ///< The link quality out for this router

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    uint8_t mCost; ///< The cost to this router via neighbor router
#else
    uint8_t mCost : 4;     ///< The cost to this router via neighbor router
#endif
};

/**
 * This class represent parent of a child node.
 *
 */
class Parent : public Router
{
public:
    /**
     * Initializes the `Parent`.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance)
    {
        Neighbor::Init(aInstance);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        mCslAccuracy.Init();
#endif
    }

    /**
     * Clears the parent entry.
     *
     */
    void Clear(void);

    /**
     * Gets route cost from parent to leader.
     *
     * @returns The route cost from parent to leader
     *
     */
    uint8_t GetLeaderCost(void) const { return mLeaderCost; }

    /**
     * Sets route cost from parent to leader.
     *
     * @param[in] aLaderConst  The route cost.
     *
     */
    void SetLeaderCost(uint8_t aLeaderCost) { mLeaderCost = aLeaderCost; }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Gets the CSL accuracy (clock accuracy and uncertainty).
     *
     * @returns The CSL accuracy.
     *
     */
    const Mac::CslAccuracy &GetCslAccuracy(void) const { return mCslAccuracy; }

    /**
     * Sets CSL accuracy.
     *
     * @param[in] aCslAccuracy  The CSL accuracy.
     *
     */
    void SetCslAccuracy(const Mac::CslAccuracy &aCslAccuracy) { mCslAccuracy = aCslAccuracy; }
#endif

private:
    uint8_t mLeaderCost;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Mac::CslAccuracy mCslAccuracy; // CSL accuracy (clock accuracy in ppm and uncertainty).
#endif
};

DefineCoreType(otNeighborInfo, Neighbor::Info);
#if OPENTHREAD_FTD
DefineCoreType(otChildInfo, Child::Info);
#endif
DefineCoreType(otRouterInfo, Router::Info);

} // namespace ot

#endif // TOPOLOGY_HPP_
