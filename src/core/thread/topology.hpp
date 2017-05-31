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

#include <openthread/platform/random.h>

#include "openthread-core-config.h"
#include "common/message.hpp"
#include "mac/mac_frame.hpp"
#include "net/ip6.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {

/**
 * This class represents a Thread neighbor.
 *
 */
class Neighbor
{
public:
    /**
     * Neighbor link states.
     *
     */
    enum State
    {
        kStateInvalid,                   ///< Neighbor link is invalid
        kStateRestored,                  ///< Neighbor is restored from non-volatile memory
        kStateParentRequest,             ///< Received an MLE Parent Request message
        kStateChildIdRequest,            ///< Received an MLE Child ID Request message
        kStateLinkRequest,               ///< Sent an MLE Link Request message
        kStateChildUpdateRequest,        ///< Sent an MLE Child Update Request message (trying to restore the child)
        kStateValid,                     ///< Link is valid
    };

    /**
     * This method returns the current state.
     *
     * @returns The current state.
     *
     */
    State GetState(void) const { return static_cast<State>(mState); }

    /**
     * This method sets the current state.
     *
     * @param[in]  aState  The state value.
     *
     */
    void SetState(State aState) { mState = static_cast<uint8_t>(aState); }

    /**
     * Check if the neighbor/child is being restored.
     *
     * @returns TRUE if the neighbor is being restored, FALSE otherwise.
     *
     */
    bool IsStateRestoring(void) const { return (mState == kStateRestored) || (mState == kStateChildUpdateRequest); }

    /**
     * Check if the neighbor/child is in valid state or if it is being restored.
     * When in these states messages can be sent to and/or received from the neighbor/child.
     *
     * @returns TRUE if the neighbor is in valid, restored, or being restored states, FALSE otherwise.
     *
     */
    bool IsStateValidOrRestoring(void) const { return (mState == kStateValid) || IsStateRestoring(); }

    /**
     * This method gets the device mode flags.
     *
     * @returns The device mode flags.
     *
     */
    uint8_t GetDeviceMode(void) const { return mMode; }

    /**
     * This method sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     *
     */
    void SetDeviceMode(uint8_t aMode) { mMode = aMode; }

    /**
     * This method indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     *
     */
    bool IsRxOnWhenIdle(void) const { return (mMode & Mle::ModeTlv::kModeRxOnWhenIdle) != 0; }

    /**
     * This method indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     *
     */
    bool IsFullThreadDevice(void) const { return (mMode & Mle::ModeTlv::kModeFFD) != 0; }

    /**
     * This method indicates whether or not the device uses secure IEEE 802.15.4 Data Request messages.
     *
     * @returns TRUE if using secure IEEE 802.15.4 Data Request messages, FALSE otherwise.
     *
     */
    bool IsSecureDataRequest(void) const { return (mMode & Mle::ModeTlv::kModeSecureDataRequest) != 0; }

    /**
     * This method indicates whether or not the device requests Full Network Data.
     *
     * @returns TRUE if requests Full Network Data, FALSE otherwise.
     *
     */
    bool IsFullNetworkData(void) const { return (mMode & Mle::ModeTlv::kModeFullNetworkData) != 0; }

    /**
     * This method sets all bytes of the Extended Address to zero.
     *
     */
    void ClearExtAddress(void) { memset(&mMacAddr, 0, sizeof(mMacAddr)); }

    /**
     * This method returns the Extended Address.
     *
     * @returns A reference to the Extended Address.
     *
     */
    Mac::ExtAddress &GetExtAddress(void) { return mMacAddr; }

    /**
     * This method returns the Extended Address.
     *
     * @returns A reference to the Extended Address.
     *
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mMacAddr; }

    /**
     * This method sets the Extended Address.
     *
     * @param[in]  aAddress  The Extended Address value to set.
     *
     */
    void SetExtAddress(const Mac::ExtAddress &aAddress) { mMacAddr = aAddress; }

    /**
     * This method gets the key sequence value.
     *
     * @returns The key sequence value.
     *
     */
    uint32_t GetKeySequence(void) const { return mKeySequence; }

    /**
     * This method sets the key sequence value.
     *
     * @parma[in]  aKeySequence  The key sequence value.
     *
     */
    void SetKeySequence(uint32_t aKeySequence) { mKeySequence = aKeySequence; }

    /**
     * This method returns the last heard time.
     *
     * @returns The last heard time.
     *
     */
    uint32_t GetLastHeard(void) const { return mLastHeard; }

    /**
     * This method sets the last heard time.
     *
     * @param[in]  aLastHeard  The last heard time.
     *
     */
    void SetLastHeard(uint32_t aLastHeard) { mLastHeard = aLastHeard; }

    /**
     * This method gets the link frame counter value.
     *
     * @returns The link frame counter value.
     *
     */
    uint32_t GetLinkFrameCounter(void) const { return mValidPending.mValid.mLinkFrameCounter; }

    /**
     * This method sets the link frame counter value.
     *
     * @param[in]  aFrameCounter  The link frame counter value.
     *
     */
    void SetLinkFrameCounter(uint32_t aFrameCounter) { mValidPending.mValid.mLinkFrameCounter = aFrameCounter; }

    /**
     * This method gets the MLE frame counter value.
     *
     * @returns The MLE frame counter value.
     *
     */
    uint32_t GetMleFrameCounter(void) const { return mValidPending.mValid.mMleFrameCounter; }

    /**
     * This method sets the MLE frame counter value.
     *
     * @param[in]  aFrameCounter  The MLE frame counter value.
     *
     */
    void SetMleFrameCounter(uint32_t aFrameCounter) { mValidPending.mValid.mMleFrameCounter = aFrameCounter; }

    /**
     * This method gets the RLOC16 value.
     *
     * @returns The RLOC16 value.
     *
     */
    uint16_t GetRloc16(void) const { return mValidPending.mValid.mRloc16; }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mValidPending.mValid.mRloc16 = aRloc16; }

    /**
     * This method indicates whether an IEEE 802.15.4 Data Request message was received.
     *
     * @returns TRUE if an IEEE 802.15.4 Data Request message was received, FALSE otherwise.
     *
     */
    bool IsDataRequestPending(void) const { return mDataRequest; }

    /**
     * This method sets the indicator for whether an IEEE 802.15.4 Data Request message was received.
     *
     * @param[in]  aPending  TRUE if an IEEE 802.15.4 Data Request message was received, FALSE otherwise.
     *
     */
    void SetDataRequestPending(bool aPending) { mDataRequest = aPending; }

    /**
     * This method gets the number of consecutive link failures.
     *
     * @returns The number of consecutive link failures.
     *
     */
    uint8_t GetLinkFailures(void) const { return mLinkFailures; }

    /**
     * This method increments the number of consecutive link failures.
     *
     */
    void IncrementLinkFailures(void) { mLinkFailures++; }

    /**
     * This method resets the number of consecutive link failures to zero.
     *
     */
    void ResetLinkFailures(void) { mLinkFailures = 0; }

    /**
     * This method returns the LinkQualityInfo object.
     *
     * @returns The LinkQualityInfo object.
     *
     */
    LinkQualityInfo &GetLinkInfo(void) { return mLinkInfo; }

    /**
     * This method generates a new challenge value for MLE Link Request/Response exchanges.
     *
     */
    void GenerateChallenge(void);

    /**
     * This method returns the current challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The current challenge value.
     *
     */
    const uint8_t *GetChallenge(void) const { return mValidPending.mPending.mChallenge; }

    /**
     * This method returns the size (byets) of the challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The size (byets) of the challenge value for MLE Link Request/Response exchanges.
     *
     */
    uint8_t GetChallengeSize(void) const { return sizeof(mValidPending.mPending.mChallenge); }

private:
    Mac::ExtAddress mMacAddr;            ///< The IEEE 802.15.4 Extended Address
    uint32_t        mLastHeard;          ///< Time when last heard.
    union
    {
        struct
        {
            uint32_t mLinkFrameCounter;  ///< The Link Frame Counter
            uint32_t mMleFrameCounter;   ///< The MLE Frame Counter
            uint16_t mRloc16;            ///< The RLOC16
        } mValid;
        struct
        {
            uint8_t mChallenge[Mle::ChallengeTlv::kMaxSize];  ///< The challenge value
            uint8_t mChallengeLength;    ///< The challenge length
        } mPending;
    } mValidPending;

    uint32_t        mKeySequence;        ///< Current key sequence
    uint8_t         mState : 3;          ///< The link state
    uint8_t         mMode : 4;           ///< The MLE device mode
    bool            mDataRequest : 1;    ///< Indicates whether or not a Data Poll was received
    uint8_t         mLinkFailures;       ///< Consecutive link failure count
    LinkQualityInfo mLinkInfo;           ///< Link quality info (contains average RSS, link margin and link quality)
};

/**
 * This class represents a Thread Child.
 *
 */
class Child : public Neighbor
{
public:
    enum
    {
        kMaxIp6AddressPerChild = OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD,
        kMaxRequestTlvs        = 5,
    };

    /**
     * This method clears the IPv6 addresses for the child.
     *
     */
    void ClearIp6Addresses(void) { memset(mIp6Address, 0, sizeof(mIp6Address)); }

    /**
     * This method gets the IPv6 address at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the IPv6 address list.
     *
     * @returns A reference to the IPv6 address entry at index @p aIndex.
     *
     */
    Ip6::Address &GetIp6Address(uint8_t aIndex) { return mIp6Address[aIndex]; }

    /**
     * This method searches for a given IPv6 address in the child's IPv6 address list and provides the index of the
     * address in the list if it is found.
     *
     * @param[in]  aAddress           The IPv6 address to search for in the IPv6 address list.
     * @param[out] aIndex             Pointer to variable where the index of address is provided if address is found in
     *                                the list. @p aIndex can be set NULL if index is not required.
     *
     * @retval OT_ERROR_NONE          Successfully found the address in IPv6 address list and updated @p aIndex.
     * @retval OT_ERROR_NOT_FOUND     Could not find the address in the list.
     *
     */
    otError FindIp6Address(const Ip6::Address &aAddress, uint8_t *aIndex) const;

    /**
     * This method removes the address at index @p aIndex.
     *
     * @param[in] aIndex   The index into the IPv6 address list.
     *
     */
    void RemoveIp6Address(uint8_t aIndex);

    /**
     * This method gets the child timeout.
     *
     * @returns The child timeout.
     *
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * This method sets the child timeout.
     *
     * @param[in]  aTimeout  The child timeout.
     *
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = aTimeout; }

    /**
     * This method gets the network data version.
     *
     * @returns The network data version.
     *
     */
    uint8_t GetNetworkDataVersion(void) const { return mNetworkDataVersion; }

    /**
     * This method sets the network data version.
     *
     * @param[in]  aVersion  The network data version.
     *
     */
    void SetNetworkDataVersion(uint8_t aVersion) { mNetworkDataVersion = aVersion; }

    /**
     * This method generates a new challenge value to use during a child attach.
     *
     */
    void GenerateChallenge(void);

    /**
     * This method gets the current challenge value used during attach.
     *
     * @returns The current challenge value.
     *
     */
    const uint8_t *GetChallenge(void) const { return mAttachChallenge; }

    /**
     * This method gets the challenge size (bytes) used during attach.
     *
     * @returns The challenge size (bytes).
     *
     */
    uint8_t GetChallengeSize(void) const { return sizeof(mAttachChallenge); }

    /**
     * This method gets the IEEE 802.15.4 Frame Counter used during indirect retransmissions.
     *
     * @returns The IEEE 802.15.4 Frame Counter value.
     *
     */
    uint32_t GetIndirectFrameCounter(void) const { return mIndirectFrameCounter; }

    /**
     * This method sets the IEEE 802.15.4 Frame Counter to use during indirect retransmissions.
     *
     * @param[in]  aFrameCounter  The IEEE 802.15.4 Frame Counter value.
     *
     */
    void SetIndirectFrameCounter(uint32_t aFrameCounter) { mIndirectFrameCounter = aFrameCounter; }

    /**
     * This method gets the message buffer to use for indirect transmissions.
     *
     * @returns The message buffer.
     *
     */
    Message *GetIndirectMessage(void) { return mIndirectMessage; }

    /**
     * This method sets the message buffer to use for indirect transmissions.
     *
     * @param[in]  aMessage  The message buffer.
     *
     */
    void SetIndirectMessage(Message *aMessage) { mIndirectMessage = aMessage; }

    /**
     * This method gets the 6LoWPAN Fragment Offset to use for indirect transmissions.
     *
     * @returns The 6LoWPAN Fragment Offset value.
     *
     */
    uint16_t GetIndirectFragmentOffset(void) const { return mIndirectFragmentOffset; }

    /**
     * This method sets the 6LoWPAN Fragment Offset to use for indirect transmissions.
     *
     * @param[in]  aFragmentOffset  The 6LoWPAN Fragment Offset value to use.
     *
     */
    void SetIndirectFragmentOffset(uint16_t aFragmentOffset) { mIndirectFragmentOffset = aFragmentOffset; }

    /**
     * This method gets the IEEE 802.15.4 Key ID to use for indirect retransmissions.
     *
     * @returns The IEEE 802.15.4 Key ID value.
     *
     */
    uint8_t GetIndirectKeyId(void) const { return mIndirectKeyId; }

    /**
     * This method sets the IEEE 802.15.4 Key ID value to use for indirect retransmissions.
     *
     * @param[in]  aKeyId  The IEEE 802.15.4 Key ID value.
     *
     */
    void SetIndirectKeyId(uint8_t aKeyId) { mIndirectKeyId = aKeyId; }

    /**
     * This method gets the number of indirect transmission attempts for the current message.
     *
     * @returns The number of indirect transmission attempts.
     *
     */
    uint8_t GetIndirectTxAttempts(void) const { return mIndirectTxAttempts; }

    /**
     * This method resets the number of indirect transmission attempts to zero.
     *
     */
    void ResetIndirectTxAttempts(void) { mIndirectTxAttempts = 0; }

    /**
     * This method increments the number of indirect transmission attempts.
     *
     */
    void IncrementIndirectTxAttempts(void) { mIndirectTxAttempts++; }

    /**
     * This method gets the IEEE 802.15.4 Data Sequence Number to use during indirect retransmissions.
     *
     * @returns The IEEE 802.15.4 Data Sequence Number value.
     *
     */
    uint8_t GetIndirectDataSequenceNumber(void) const { return mIndirectDsn; }

    /**
     * This method sets the IEEE 802.15.4 Data Sequence Number to use during indirect retransmissions.
     *
     * @param[in]  aDsn  The IEEE 802.15.4 Data Sequence Number value.
     *
     */
    void SetIndirectDataSequenceNumber(uint8_t aDsn) { mIndirectDsn = aDsn; }

    /**
     * This method indicates whether or not to source match on the short address.
     *
     * @returns TRUE if using the short address, FALSE if using the extended address.
     *
     */
    bool IsIndirectSourceMatchShort(void) const { return mUseShortAddress; }

    /**
     * This method sets whether or not to source match on the short address.
     *
     * @param[in]  aShort  TRUE if using the short address, FALSE if using the extended address.
     *
     */
    void SetIndirectSourceMatchShort(bool aShort) { mUseShortAddress = aShort; }

    /**
     * This method indicates whether or not the child needs to be added to the source match table.
     *
     * @returns TRUE if the child needs to be added to the source match table, FALSE otherwise.
     *
     */
    bool IsIndirectSourceMatchPending(void) const { return mSourceMatchPending; }

    /**
     * This method sets whether or not the child needs to be added to the source match table.
     *
     * @param[in]  aPending  TRUE if the child needs to be added to the source match table, FALSE otherwise.
     *
     */
    void SetIndirectSourceMatchPending(bool aPending) { mSourceMatchPending = aPending; }

    /**
     * This method returns the number of queued message(s) for the child
     *
     * @returns Number of queues message(s).
     *
     */
    uint16_t GetIndirectMessageCount(void) const { return mQueuedMessageCount; }

    /**
     * This method increments the indirect message count.
     *
     */
    void IncrementIndirectMessageCount(void) { mQueuedMessageCount++; }

    /**
     * This method decrements the indirect message count.
     *
     */
    void DecrementIndirectMessageCount(void) { mQueuedMessageCount--; }

    /**
     * This method resets the indirect message count to zero.
     *
     */
    void ResetIndirectMessageCount(void) { mQueuedMessageCount = 0; }

    /**
     * This method clears the requested TLV list.
     *
     */
    void ClearRequestTlvs(void) { memset(mRequestTlvs, Mle::Tlv::kInvalid, sizeof(mRequestTlvs)); }

    /**
     * This method returns the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     *
     * @returns The requested TLV at index @p aIndex.
     *
     */
    uint8_t GetRequestTlv(uint8_t aIndex) const { return mRequestTlvs[aIndex]; }

    /**
     * This method sets the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     * @param[in]  aType   The TLV type.
     *
     */
    void SetRequestTlv(uint8_t aIndex, uint8_t aType) { mRequestTlvs[aIndex] = aType; }

    /**
     * This method gets the mac address of child (either rloc16 or extended address depending on `UseShortAddress` flag).
     *
     * @param[out] aMacAddress A reference to a mac address object to which the child's address is copied.
     *
     * @returns A (const) reference to the mac address @a aMacAddress.
     *
     */
    const Mac::Address &GetMacAddress(Mac::Address &aMacAddress) const;

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION

    /**
     * This method increments the number of seconds since last supervision of the child.
     *
     */
    void IncrementSecondsSinceLastSupervision(void) { mSecondsSinceSupervision++; }

    /**
     * This method returns the number of seconds since last supervision of the child (last message to the child)
     *
     * @returns Number of seconds since last supervision of the child.
     *
     */
    uint16_t GetSecondsSinceLastSupervision(void) const { return mSecondsSinceSupervision; }

    /**
     * This method resets the number of seconds since last supervision of the child to zero.
     *
     */
    void ResetSecondsSinceLastSupervision(void) { mSecondsSinceSupervision = 0; }

#endif // #if OPENTHREAD_ENABLE_CHILD_SUPERVISION

private:
    Ip6::Address mIp6Address[kMaxIp6AddressPerChild];  ///< Registered IPv6 addresses
    uint32_t     mTimeout;                             ///< Child timeout

    union
    {
        uint8_t mRequestTlvs[kMaxRequestTlvs];                 ///< Requested MLE TLVs
        uint8_t mAttachChallenge[Mle::ChallengeTlv::kMaxSize]; ///< The challenge value
    };

    uint32_t     mIndirectFrameCounter;                ///< Frame counter for current indirect message (used fore retx).
    Message     *mIndirectMessage;                     ///< Current indirect message.
    uint16_t     mIndirectFragmentOffset;              ///< 6LoWPAN fragment offset for the indirect message.
    uint8_t      mIndirectKeyId;                       ///< Key Id for current indirect message (used for retx).
    uint8_t      mIndirectTxAttempts;                  ///< Number of data poll triggered tx attempts.
    uint8_t      mIndirectDsn;                         ///< MAC level Data Sequence Number (DSN) for retx attempts.
    uint8_t      mNetworkDataVersion;                  ///< Current Network Data version
    uint16_t     mQueuedMessageCount : 13;             ///< Number of queued indirect messages for the child.
    bool         mUseShortAddress : 1;                 ///< Indicates whether to use short or extended address.
    bool         mSourceMatchPending : 1;              ///< Indicates whether or not pending to add to src match table.

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    uint16_t     mSecondsSinceSupervision;             ///< Number of seconds since last supervision of the child.
#endif // OPENTHREAD_ENABLE_CHILD_SUPERVISION

};

/**
 * This class represents a Thread Router
 *
 */
class Router : public Neighbor
{
public:
    /**
     * This method gets the router ID of the next hop to this router.
     *
     * @returns The router ID of the next hop to this router.
     *
     */
    uint8_t GetNextHop(void) const { return mNextHop; }

    /**
     * This method sets the router ID of the next hop to this router.
     *
     * @param[in]  aRouterId  The router ID of the next hop to this router.
     *
     */
    void SetNextHop(uint8_t aRouterId) { mNextHop = aRouterId; }

    /**
     * This method gets the link quality out value for this router.
     *
     * @returns The link quality out value for this router.
     *
     */
    uint8_t GetLinkQualityOut(void) const { return mLinkQualityOut; }

    /**
     * This method sets the link quality out value for this router.
     *
     * @param[in]  aLinkQuality  The link quality out value for this router.
     *
     */
    void SetLinkQualityOut(uint8_t aLinkQuality) { mLinkQualityOut = aLinkQuality; }

    /**
     * This method get the route cost to this router.
     *
     * @returns The route cost to this router.
     *
     */
    uint8_t GetCost(void) const { return mCost; }

    /**
     * This method sets the router cost to this router.
     *
     * @param[in]  aCost  The router cost to this router.
     *
     */
    void SetCost(uint8_t aCost) { mCost = aCost; }

    /**
     * This method indicates whether or not this router ID has been allocated.
     *
     * @returns TRUE if this router ID has been allocated, FALSE otherwise.
     *
     */
    bool IsAllocated(void) const { return mAllocated; }

    /**
     * This method sets whether or not this router ID has been allocated.
     *
     * @param[in]  aAllocated  TRUE if this router ID has been allocated, FALSE otherwise.
     *
     */
    void SetAllocated(bool aAllocated) { mAllocated = aAllocated; }

    /**
     * This method indicates whether the reclaim delay is in effect for this router ID.
     *
     * @returns TRUE if the reclaim delay is in effect, FALSE otherwise.
     *
     */
    bool IsReclaimDelay(void) const { return mReclaimDelay; }

    /**
     * This method sets whether the reclaim delay is in effect for this router ID.
     *
     * @param[in]  aReclaimDelay  TRUE if the reclaim delay is in effect, FALSE otherwise.
     *
     */
    void SetReclaimDelay(bool aReclaimDelay) { mReclaimDelay = aReclaimDelay; }

private:
    uint8_t mNextHop;             ///< The next hop towards this router
    uint8_t mLinkQualityOut : 2;  ///< The link quality out for this router
    uint8_t mCost : 4;            ///< The cost to this router via neighbor router
    bool    mAllocated : 1;       ///< Indicates whether or not this entry is allocated
    bool    mReclaimDelay : 1;    ///< Indicates whether or not this entry is waiting to be reclaimed
};

}  // namespace ot

#endif  // TOPOLOGY_HPP_
