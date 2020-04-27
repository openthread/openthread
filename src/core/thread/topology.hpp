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

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6.hpp"
#include "thread/indirect_sender.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {

/**
 * This class represents a Thread neighbor.
 *
 */
class Neighbor : public InstanceLocatorInit
{
public:
    /**
     * Neighbor link states.
     *
     */
    enum State
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
     * This enumeration defines state filters used for finding a neighbor or iterating through the child/neighbor table.
     *
     * Each filter definition accepts a subset of `State` values.
     *
     */
    enum StateFilter
    {
        kInStateValid,                     ///< Accept child only in `kStateValid`.
        kInStateValidOrRestoring,          ///< Accept child with `IsStateValidOrRestoring()` being `true`.
        kInStateChildIdRequest,            ///< Accept child only in `Child:kStateChildIdRequest`.
        kInStateValidOrAttaching,          ///< Accept child with `IsStateValidOrAttaching()` being `true`.
        kInStateAnyExceptInvalid,          ///< Accept child in any state except `kStateInvalid`.
        kInStateAnyExceptValidOrRestoring, ///< Accept child in any state except `IsStateValidOrRestoring()`.
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
     * This method indicates whether the neighbor is in the Invalid state.
     *
     * @returns TRUE if the neighbor is in the Invalid state, FALSE otherwise.
     *
     */
    bool IsStateInvalid(void) const { return (mState == kStateInvalid); }

    /**
     * This method indicates whether the neighbor is in the Child ID Request state.
     *
     * @returns TRUE if the neighbor is in the Child ID Request state, FALSE otherwise.
     *
     */
    bool IsStateChildIdRequest(void) const { return (mState == kStateChildIdRequest); }

    /**
     * This method indicates whether the neighbor is in the Link Request state.
     *
     * @returns TRUE if the neighbor is in the Link Request state, FALSE otherwise.
     *
     */
    bool IsStateLinkRequest(void) const { return (mState == kStateLinkRequest); }

    /**
     * This method indicates whether the neighbor is in the Parent Response state.
     *
     * @returns TRUE if the neighbor is in the Parent Response state, FALSE otherwise.
     *
     */
    bool IsStateParentResponse(void) const { return (mState == kStateParentResponse); }

    /**
     * This method indicates whether the neighbor is being restored.
     *
     * @returns TRUE if the neighbor is being restored, FALSE otherwise.
     *
     */
    bool IsStateRestoring(void) const { return (mState == kStateRestored) || (mState == kStateChildUpdateRequest); }

    /**
     * This method indicates whether the neighbor is in the Restored state.
     *
     * @returns TRUE if the neighbor is in the Restored state, FALSE otherwise.
     *
     */
    bool IsStateRestored(void) const { return (mState == kStateRestored); }

    /**
     * This method indicates whether the neighbor is valid (frame counters are synchronized).
     *
     * @returns TRUE if the neighbor is valid, FALSE otherwise.
     *
     */
    bool IsStateValid(void) const { return (mState == kStateValid); }

    /**
     * This method indicates whether the neighbor is in valid state or if it is being restored.
     *
     * When in these states messages can be sent to and/or received from the neighbor.
     *
     * @returns TRUE if the neighbor is in valid, restored, or being restored states, FALSE otherwise.
     *
     */
    bool IsStateValidOrRestoring(void) const { return (mState == kStateValid) || IsStateRestoring(); }

    /**
     * This method indicates if the neighbor state is valid, attaching, or restored.
     *
     * The states `kStateRestored`, `kStateChildIdRequest`, `kStateChildUpdateRequest`, `kStateValid`, and
     * `kStateLinkRequest` are considered as valid, attaching, or restored.
     *
     * @returns TRUE if the neighbor state is valid, attaching, or restored, FALSE otherwise.
     *
     */
    bool IsStateValidOrAttaching(void) const;

    /**
     * This method indicates whether neighbor state matches a given state filter.
     *
     * @param[in] aFilter   A state filter (`StateFilter` enumeration) to match against.
     *
     * @returns TRUE if the neighbor state matches the filter, FALSE otherwise.
     *
     */
    bool MatchesFilter(StateFilter aFilter) const;

    /**
     * This method gets the device mode flags.
     *
     * @returns The device mode flags.
     *
     */
    Mle::DeviceMode GetDeviceMode(void) const { return Mle::DeviceMode(mMode); }

    /**
     * This method sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     *
     */
    void SetDeviceMode(Mle::DeviceMode aMode) { mMode = aMode.Get(); }

    /**
     * This method indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     *
     */
    bool IsRxOnWhenIdle(void) const { return GetDeviceMode().IsRxOnWhenIdle(); }

    /**
     * This method indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     *
     */
    bool IsFullThreadDevice(void) const { return GetDeviceMode().IsFullThreadDevice(); }

    /**
     * This method indicates whether or not the device uses secure IEEE 802.15.4 Data Request messages.
     *
     * @returns TRUE if using secure IEEE 802.15.4 Data Request messages, FALSE otherwise.
     *
     */
    bool IsSecureDataRequest(void) const { return GetDeviceMode().IsSecureDataRequest(); }

    /**
     * This method indicates whether or not the device requests Full Network Data.
     *
     * @returns TRUE if requests Full Network Data, FALSE otherwise.
     *
     */
    bool IsFullNetworkData(void) const { return GetDeviceMode().IsFullNetworkData(); }

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
     * @param[in]  aKeySequence  The key sequence value.
     *
     */
    void SetKeySequence(uint32_t aKeySequence) { mKeySequence = aKeySequence; }

    /**
     * This method returns the last heard time.
     *
     * @returns The last heard time.
     *
     */
    TimeMilli GetLastHeard(void) const { return mLastHeard; }

    /**
     * This method sets the last heard time.
     *
     * @param[in]  aLastHeard  The last heard time.
     *
     */
    void SetLastHeard(TimeMilli aLastHeard) { mLastHeard = aLastHeard; }

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
    uint16_t GetRloc16(void) const { return mRloc16; }

    /**
     * This method gets the Router ID value.
     *
     * @returns The Router ID value.
     *
     */
    uint8_t GetRouterId(void) const { return mRloc16 >> Mle::kRouterIdOffset; }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = aRloc16; }

    /**
     * This method indicates whether or not it is a valid Thread 1.1 neighbor.
     *
     * @returns TRUE if it is a valid Thread 1.1 neighbor, FALSE otherwise.
     *
     */
    bool IsThreadVersion1p1(void) const { return mState != kStateInvalid && mVersion == OT_THREAD_VERSION_1_1; }

    /**
     * This method indicates whether Enhanced Keep-Alive is supported or not.
     *
     * @returns TRUE if Enhanced Keep-Alive is supported, FALSE otherwise.
     *
     */
    bool IsEnhancedKeepAliveSupported(void) const
    {
        return mState != kStateInvalid && mVersion >= OT_THREAD_VERSION_1_2;
    }

    /**
     * This method gets the device MLE version.
     *
     */
    uint8_t GetVersion(void) const { return mVersion; }

    /**
     * This method sets the device MLE version.
     *
     * @param[in]  aVersion  The device MLE version.
     *
     */
    void SetVersion(uint8_t aVersion) { mVersion = aVersion; }

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
     * This method returns the size (bytes) of the challenge value for MLE Link Request/Response exchanges.
     *
     * @returns The size (bytes) of the challenge value for MLE Link Request/Response exchanges.
     *
     */
    uint8_t GetChallengeSize(void) const { return sizeof(mValidPending.mPending.mChallenge); }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * This method indicates whether or not time sync feature is enabled.
     *
     * @returns TRUE if time sync feature is enabled, FALSE otherwise.
     *
     */
    bool IsTimeSyncEnabled(void) const { return mTimeSyncEnabled; }

    /**
     * This method sets whether or not time sync feature is enabled.
     *
     * @param[in]  aEnable    TRUE if time sync feature is enabled, FALSE otherwise.
     *
     */
    void SetTimeSyncEnabled(bool aEnabled) { mTimeSyncEnabled = aEnabled; }
#endif

protected:
    /**
     * This method initializes the `Neighbor` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance);

private:
    Mac::ExtAddress mMacAddr;   ///< The IEEE 802.15.4 Extended Address
    TimeMilli       mLastHeard; ///< Time when last heard.
    union
    {
        struct
        {
            uint32_t mLinkFrameCounter; ///< The Link Frame Counter
            uint32_t mMleFrameCounter;  ///< The MLE Frame Counter
        } mValid;
        struct
        {
            uint8_t mChallenge[Mle::kMaxChallengeSize]; ///< The challenge value
        } mPending;
    } mValidPending;

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
    uint8_t         mVersion;  ///< The MLE version
    LinkQualityInfo mLinkInfo; ///< Link quality info (contains average RSS, link margin and link quality)
};

/**
 * This class represents a Thread Child.
 *
 */
class Child : public Neighbor, public IndirectSender::ChildInfo, public DataPollHandler::ChildInfo
{
public:
    enum
    {
        kMaxRequestTlvs = 5,
    };

    /**
     * This class defines an iterator used by `GetNextIp6Address()` to go through IPv6 address entries of a child.
     *
     */
    class Ip6AddressIterator
    {
        friend class Child;

    public:
        /**
         * This constructor initializes the iterator object.
         *
         * After initialization a call to `GetNextIp6Address()` would start at the first IPv6 address entry in the list.
         *
         */
        Ip6AddressIterator(void)
            : mIndex(0)
        {
        }

        /**
         * This method resets the iterator.
         *
         * After reset the next call to `GetNextIp6Address()` would start at the first IPv6 address entry in the list.
         *
         */
        void Reset(void) { mIndex = 0; }

        /**
         * This method sets the iterator from an `otChildIp6AddressIterator`
         *
         * @param[in]   aChildAddressIterator  A child address iterator
         *
         */
        void Set(otChildIp6AddressIterator aChildAddressIterator) { mIndex = aChildAddressIterator; }

        /**
         * This method returns the iterator as an `otChildIp6AddressIterator`
         *
         * @returns The iterator as an `otChildIp6AddressIterator`
         *
         */
        otChildIp6AddressIterator Get(void) const { return mIndex; }

    private:
        void Increment(void) { mIndex++; }

        otChildIp6AddressIterator mIndex;
    };

    /**
     * This method initializes the `Child` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * This method clears the child entry.
     *
     */
    void Clear(void);

    /**
     * This method clears the IPv6 address list for the child.
     *
     */
    void ClearIp6Addresses(void);

    /**
     * This method gets the mesh-local IPv6 address.
     *
     * @param[out]   aAddress            A reference to an IPv6 address to provide address (if any).
     *
     * @retval       OT_ERROR_NONE       Successfully found the mesh-local address and updated @p aAddress.
     * @retval       OT_ERROR_NOT_FOUND  No mesh-local IPv6 address in the IPv6 address list.
     *
     */
    otError GetMeshLocalIp6Address(Ip6::Address &aAddress) const;

    /**
     * This method returns a pointer to the Mesh Local Interface Identifier.
     *
     * @returns a pointer to the Mesh Local Interface Identifier.
     *
     */
    const uint8_t *GetMeshLocalIid(void) const { return mMeshLocalIid; }

    /**
     * This method gets the next IPv6 address in the list.
     *
     * @param[inout] aIterator           A reference to an IPv6 address iterator.
     * @param[out]   aAddress            A reference to an IPv6 address to provide the next address (if any).
     *
     * @retval       OT_ERROR_NONE       Successfully found the next address and updated @p aAddress and @p aIterator.
     * @retval       OT_ERROR_NOT_FOUND  No subsequent IPv6 address exists in the IPv6 address list.
     *
     */
    otError GetNextIp6Address(Ip6AddressIterator &aIterator, Ip6::Address &aAddress) const;

    /**
     * This method adds an IPv6 address to the list.
     *
     * @param[in]  aAddress           A reference to IPv6 address to be added.
     *
     * @retval OT_ERROR_NONE          Successfully added the new address.
     * @retval OT_ERROR_ALREADY       Address is already in the list.
     * @retval OT_ERROR_NO_BUFS       Already at maximum number of addresses. No entry available to add the new address.
     * @retval OT_ERROR_INVALID_ARGS  Address is invalid (it is the Unspecified Address).
     *
     */
    otError AddIp6Address(const Ip6::Address &aAddress);

    /**
     * This method removes an IPv6 address from the list.
     *
     * @param[in]  aAddress               A reference to IPv6 address to be removed.
     *
     * @retval OT_ERROR_NONE              Successfully removed the address.
     * @retval OT_ERROR_NOT_FOUND         Address was not found in the list.
     * @retval OT_ERROR_INVALID_ARGS      Address is invalid (it is the Unspecified Address).
     *
     */
    otError RemoveIp6Address(const Ip6::Address &aAddress);

    /**
     * This method indicates whether an IPv6 address is in the list of IPv6 addresses of the child.
     *
     * @param[in]  aAddress   A reference to IPv6 address.
     *
     * @retval TRUE           The address exists on the list.
     * @retval FALSE          Address was not found in the list.
     *
     */
    bool HasIp6Address(const Ip6::Address &aAddress) const;

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

#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE

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

#endif // #if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE

private:
#if OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD < 2
#error OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD should be at least set to 2.
#endif

    enum
    {
        kNumIp6Addresses = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1,
    };

    uint8_t      mNetworkDataVersion;                                   ///< Current Network Data version
    uint8_t      mMeshLocalIid[Ip6::Address::kInterfaceIdentifierSize]; ///< IPv6 address IID for mesh-local address
    Ip6::Address mIp6Address[kNumIp6Addresses];                         ///< Registered IPv6 addresses

    uint32_t mTimeout; ///< Child timeout

    union
    {
        uint8_t mRequestTlvs[kMaxRequestTlvs];            ///< Requested MLE TLVs
        uint8_t mAttachChallenge[Mle::kMaxChallengeSize]; ///< The challenge value
    };

#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
    uint16_t mSecondsSinceSupervision; ///< Number of seconds since last supervision of the child.
#endif

    OT_STATIC_ASSERT(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < 8192, "mQueuedMessageCount cannot fit max required!");
};

/**
 * This class represents a Thread Router
 *
 */
class Router : public Neighbor
{
public:
    /**
     * This method initializes the `Router` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * This method clears the router entry.
     *
     */
    void Clear(void);

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

private:
    uint8_t mNextHop;            ///< The next hop towards this router
    uint8_t mLinkQualityOut : 2; ///< The link quality out for this router

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    uint8_t mCost; ///< The cost to this router via neighbor router
#else
    uint8_t mCost : 4;     ///< The cost to this router via neighbor router
#endif
};

} // namespace ot

#endif // TOPOLOGY_HPP_
