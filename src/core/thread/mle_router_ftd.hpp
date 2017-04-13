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
 *   This file includes definitions for MLE functionality required by the Thread Router and Leader roles.
 */

#ifndef MLE_ROUTER_HPP_
#define MLE_ROUTER_HPP_

#include <string.h>

#include <coap/coap_header.hpp>
#include <coap/coap_server.hpp>
#include <coap/coap_client.hpp>
#include <common/timer.hpp>
#include <common/trickle_timer.hpp>
#include <mac/mac_frame.hpp>
#include <net/icmp6.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/topology.hpp>

namespace Thread {
namespace Mle {

class MeshForwarder;
class NetworkDataLeader;

/**
 * @addtogroup core-mle-router
 *
 * @brief
 *   This module includes definitions for MLE functionality required by the Thread Router and Leader roles.
 *
 * @{
 */

/**
* This structure represents the child information for persistent storage.
*
*/
struct ChildInfo
{
    Mac::ExtAddress  mExtAddress;    ///< Extended Address
    uint32_t         mTimeout;       ///< Timeout
    uint16_t         mRloc16;        ///< RLOC16
    uint8_t          mMode;          ///< The MLE device mode
};

/**
 * This class implements MLE functionality required by the Thread Router and Leader roles.
 *
 */
class MleRouter: public Mle
{
    friend class Mle;

public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit MleRouter(ThreadNetif &aThreadNetif);

    /**
     * This method indicates whether or not the Router Role is enabled.
     *
     * @retval true   If the Router Role is enabled.
     * @retval false  If the Router Role is not enabled.
     *
     */
    bool IsRouterRoleEnabled(void) const;

    /**
     * This method sets whether or not the Router Role is enabled.
     *
     * If @p aEnable is false and the device is currently operating as a router, this call will cause the device to
     * detach and attempt to reattach as a child.
     *
     * @param[in]  aEnabled  TRUE to enable the Router Role, FALSE otherwise.
     *
     */
    void SetRouterRoleEnabled(bool aEnabled);

    /**
     * This method indicates whether a node is the only router on the network.
     *
     * @retval TRUE   It is the only router in the network.
     * @retval FALSE  It is a child or is not a single router in the network.
     *
     */
    bool IsSingleton(void);

    /**
     * This method generates an Address Solicit request for a Router ID.
     *
     * @param[in]  aStatus  The reason for requesting a Router ID.
     *
     * @retval kThreadError_None          Successfully generated an Address Solicit message.
     * @retval kThreadError_NotCapable    Device is not capable of becoming a router
     * @retval kThreadError_InvalidState  Thread is not enabled
     *
     */
    ThreadError BecomeRouter(ThreadStatusTlv::Status aStatus);

    /**
     * This method causes the Thread interface to become a Leader and start a new partition.
     *
     * @retval kThreadError_None          Successfully become a Leader and started a new partition.
     * @retval kThreadError_NotCapable    Device is not capable of becoming a leader
     * @retval kThreadError_InvalidState  Thread is not enabled
     *
     */
    ThreadError BecomeLeader(void);

    /**
     * This method returns the number of active routers.
     *
     * @returns The number of active routers.
     *
     */
    uint8_t GetActiveRouterCount(void) const;

    /**
     * This method returns the time in seconds since the last Router ID Sequence update.
     *
     * @returns The time in seconds since the last Router ID Sequence update.
     *
     */
    uint32_t GetLeaderAge(void) const;

    /**
     * This method returns the Leader Weighting value for this Thread interface.
     *
     * @returns The Leader Weighting value for this Thread interface.
     *
     */
    uint8_t GetLeaderWeight(void) const { return mLeaderWeight; }

    /**
     * This method sets the Leader Weighting value for this Thread interface.
     *
     * @param[in]  aWeight  The Leader Weighting value.
     *
     */
    void SetLeaderWeight(uint8_t aWeight) { mLeaderWeight = aWeight; }

    /**
     * This method returns the fixed Partition Id of Thread network partition for certification testing.
     *
     * @returns The Partition Id for this Thread network partition.
     *
     */
    uint32_t GetLeaderPartitionId(void) const { return mFixedLeaderPartitionId; }

    /**
     * This method sets the fixed Partition Id for Thread network partition for certification testing.
     *
     * @param[in]  aPartitionId  The Leader Partition Id.
     *
     */
    void SetLeaderPartitionId(uint32_t aPartitionId) { mFixedLeaderPartitionId = aPartitionId; }

    /**
     * This method sets the preferred Router Id. Upon becoming a router/leader the node
     * attempts to use this Router Id. If the preferred Router Id is not set or if it
     * can not be used, a randomly generated router Id is picked.
     * This property can be set when he device role is detached or disabled.
     *
     * @param[in]  aRouterId             The preferred Router Id.
     *
     * @retval kThreadError_None         Successfully set the preferred Router Id.
     * @retval kThreadError_InvalidState Could not set (role is other than detached and disabled)
     *
     */
    ThreadError SetPreferredRouterId(uint8_t aRouterId);

    /**
     * This method gets the Partition Id which the device joined successfully once.
     *
     */
    uint32_t GetPreviousPartitionId(void) const { return mPreviousPartitionId; }

    /**
     * This method sets the Partition Id which the device joins successfully.
     *
     * @param[in]  aPartitionId   The Partition Id.
     *
     */
    void SetPreviousPartitionId(uint32_t aPartitionId) { mPreviousPartitionId = aPartitionId; }

    /**
     * This method sets the Router Id.
     *
     * @param[in]  aRouterId   The Router Id.
     *
     */
    void SetRouterId(uint8_t aRouterId);

    /**
     * This method returns the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestination  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, kInvalidRloc16 otherwise.
     *
     */
    uint16_t GetNextHop(uint16_t aDestination);

    /**
     * This method returns the NETWORK_ID_TIMEOUT value.
     *
     * @returns The NETWORK_ID_TIMEOUT value.
     *
     */
    uint8_t GetNetworkIdTimeout(void) const { return mNetworkIdTimeout; }

    /**
     * This method sets the NETWORK_ID_TIMEOUT value.
     *
     * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
     *
     */
    void SetNetworkIdTimeout(uint8_t aTimeout) { mNetworkIdTimeout = aTimeout; }

    /**
     * This method returns the route cost to a RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 of the destination.
     *
     * @returns The route cost to a RLOC16.
     *
     */
    uint8_t GetRouteCost(uint16_t aRloc16) const;

    /**
     * This method returns the link cost to the given Router.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @returns The link cost to the Router.
     *
     */
    uint8_t GetLinkCost(uint8_t aRouterId);

    /**
     * This method returns the current Router ID Sequence value.
     *
     * @returns The current Router ID Sequence value.
     *
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * This method returns the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     *
     */
    uint8_t GetRouterUpgradeThreshold(void) const { return mRouterUpgradeThreshold; }

    /**
     * This method sets the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     *
     */
    void SetRouterUpgradeThreshold(uint8_t aThreshold) { mRouterUpgradeThreshold = aThreshold; }

    /**
     * This method returns the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     *
     */
    uint8_t GetRouterDowngradeThreshold(void) const { return mRouterDowngradeThreshold; }

    /**
     * This method sets the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     *
     */
    void SetRouterDowngradeThreshold(uint8_t aThreshold) { mRouterDowngradeThreshold = aThreshold; }

    /**
     * This method release a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to release.
     *
     * @retval kThreadError_None          Successfully released the Router ID.
     * @retval kThreadError_InvalidState  The Router ID was not allocated.
     *
     */
    ThreadError ReleaseRouterId(uint8_t aRouterId);

    /**
     * This method removes a link to a neighbor.
     *
     * @param[in]  aAddress  The link address of the neighbor.
     *
     * @retval kThreadError_None      Successfully removed the neighbor.
     * @retval kThreadError_NotFound  Could not find the neighbor.
     *
     */
    ThreadError RemoveNeighbor(const Mac::Address &aAddress);

    /**
     * This method removes a link to a neighbor.
     *
     * @param[in]  aNeighbor  A reference to the neighbor object.
     *
     * @retval kThreadError_None      Successfully removed the neighbor.
     *
     */
    ThreadError RemoveNeighbor(Neighbor &aNeighbor);

    /**
     * This method returns a pointer to a Child object.
     *
     * @param[in]  aAddress  The address of the Child.
     *
     * @returns A pointer to the Child object.
     *
     */
    Child *GetChild(uint16_t aAddress);

    /**
     * This method returns a pointer to a Child object.
     *
     * @param[in]  aAddress  A reference to the address of the Child.
     *
     * @returns A pointer to the Child object.
     *
     */
    Child *GetChild(const Mac::ExtAddress &aAddress);

    /**
     * This method returns a pointer to a Child object.
     *
     * @param[in]  aAddress  A reference to the address of the Child.
     *
     * @returns A pointer to the Child corresponding to @p aAddress, NULL otherwise.
     *
     */
    Child *GetChild(const Mac::Address &aAddress);

    /**
     * This method returns a child index for the Child object.
     *
     * @param[in]  aChild  A reference to the Child object.
     *
     * @returns The index for the Child corresponding to @p aChild.
     *
     */
    uint8_t GetChildIndex(const Child &aChild);

    /**
     * This method returns a pointer to a Child array.
     *
     * @param[out]  aNumChildren  A pointer to output the number of children.
     *
     * @returns A pointer to the Child array.
     *
     */
    Child *GetChildren(uint8_t *aNumChildren);

    /**
     * This method sets the max children allowed value for this Thread interface.
     *
     * @param[in]  aMaxChildren  The max children allowed value.
     *
     * @retval  kThreadError_None          Successfully set the max.
     * @retval  kThreadError_InvalidArgs   If @p aMaxChildren is not in the range [1, kMaxChildren].
     * @retval  kThreadError_InvalidState  If MLE has already been started.
     *
     */
    ThreadError SetMaxAllowedChildren(uint8_t aMaxChildren);

    /**
     * This method restores children information from non-volatile memory.
     *
     * @retval  kThreadError_None     Successfully restored children information.
     * @retval  kThreadError_Failed   The saved child info in non-volatile memory is invalid.
     * @retval  kThreadError_NoBufs   More children in settings than max children.
     *
     */
    ThreadError RestoreChildren(void);

    /**
     * This method remove a stored child information from non-volatile memory.
     *
     * @param[in]  aChildRloc16   The child RLOC16 to remove.
     *
     * @retval  kThreadError_None       Successfully remove child.
     * @retval  kThreadError_NotFound   There is no specified child stored in non-volatile memory.
     *
     */
    ThreadError RemoveStoredChild(uint16_t aChildRloc16);

    /**
     * This method store a child information into non-volatile memory.
     *
     * @param[in]  aChildRloc16   The child RLOC16 to store.
     *
     * @retval  kThreadError_None     Successfully store child.
     * @retval  kThreadError_NoBufs   Insufficient available buffers to store child.
     *
     */
    ThreadError StoreChild(uint16_t aChildRloc16);

    /**
     * This method refreshes all the saved children information in non-volatile memory by first erasing any saved
     * child information in non-volatile memory and then saving all children info.
     *
     * @retval  kThreadError_None     Successfully refreshed all children info in non-volatile memory
     * @retval  kThreadError_NoBufs   Insufficient available buffers to store child.
     *
     */
    ThreadError RefreshStoredChildren(void);

    /**
     * This method returns a pointer to a Neighbor object.
     *
     * @param[in]  aAddress  The address of the Neighbor.
     *
     * @returns A pointer to the Neighbor corresponding to @p aAddress, NULL otherwise.
     *
     */
    Neighbor *GetNeighbor(uint16_t aAddress);

    /**
     * This method returns a pointer to a Neighbor object.
     *
     * @param[in]  aAddress  The address of the Neighbor.
     *
     * @returns A pointer to the Neighbor corresponding to @p aAddress, NULL otherwise.
     *
     */
    Neighbor *GetNeighbor(const Mac::ExtAddress &aAddress);

    /**
     * This method returns a pointer to a Neighbor object.
     *
     * @param[in]  aAddress  The address of the Neighbor.
     *
     * @returns A pointer to the Neighbor corresponding to @p aAddress, NULL otherwise.
     *
     */
    Neighbor *GetNeighbor(const Mac::Address &aAddress);

    /**
     * This method returns a pointer to a Neighbor object.
     *
     * @param[in]  aAddress  The address of the Neighbor.
     *
     * @returns A pointer to the Neighbor corresponding to @p aAddress, NULL otherwise.
     *
     */
    Neighbor *GetNeighbor(const Ip6::Address &aAddress);

    /**
     * This method retains diagnostic information for an attached child by Child ID or RLOC16.
     *
     * @param[in]   aChildId    The Child ID or RLOC16 for an attached child.
     * @param[out]  aChildInfo  The child information.
     *
     */
    ThreadError GetChildInfoById(uint16_t aChildId, otChildInfo &aChildInfo);

    /**
     * This method retains diagnostic information for an attached child by the internal table index.
     *
     * @param[in]   aChildIndex  The table index.
     * @param[out]  aChildInfo   The child information.
     *
     */
    ThreadError GetChildInfoByIndex(uint8_t aChildIndex, otChildInfo &aChildInfo);

    /**
     * This method gets the next neighbor information. It is used to iterate through the entries of
     * the neighbor table.
     *
     * @param[inout]  aIterator  A reference to the iterator context. To get the first neighbor entry
                                 it should be set to OT_NEIGHBOR_INFO_ITERATOR_INIT.
     * @param[out]    aNeighInfo The neighbor information.
     *
     * @retval kThreadError_None         Successfully found the next neighbor entry in table.
     * @retval kThreadError_NotFound     No subsequent neighbor entry exists in the table.
     *
     */
    ThreadError GetNextNeighborInfo(otNeighborInfoIterator &aIterator, otNeighborInfo &aNeighInfo);

    /**
     * This method returns a pointer to a Router array.
     *
     * @param[out]  aNumRouters  A pointer to output the number of routers.
     *
     * @returns A pointer to the Router array.
     *
     */
    Router *GetRouters(uint8_t *aNumRouters);

    /**
     * This method returns a pointer to a Router entry.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @returns A pointer to a Router entry or NULL if @p aRouterId is out-of-range.
     *
     */
    Router *GetRouter(uint8_t aRouterId);

    /**
     * This method returns a pointer to a Router entry.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @returns A pointer to a Router entry or NULL if @p aRouterId is out-of-range.
     *
     */
    const Router *GetRouter(uint8_t aRouterId) const;

    /**
     * This method retains diagnostic information for a given router.
     *
     * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
     * @param[out]  aRouterInfo  The router information.
     *
     */
    ThreadError GetRouterInfo(uint16_t aRouterId, otRouterInfo &aRouterInfo);

    /**
     * This method indicates whether or not the given Thread partition attributes are preferred.
     *
     * @param[in]  aSingletonA   Whether or not the Thread Partition A has a single router.
     * @param[in]  aLeaderDataA  A reference to Thread Partition A's Leader Data.
     * @param[in]  aSingletonB   Whether or not the Thread Partition B has a single router.
     * @param[in]  aLeaderDataB  A reference to Thread Partition B's Leader Data.
     *
     * @retval 1   If partition A is preferred.
     * @retval 0   If partition A and B have equal preference.
     * @retval -1  If partition B is preferred.
     *
     */
    static int ComparePartitions(bool aSingletonA, const LeaderDataTlv &aLeaderDataA,
                                 bool aSingletonB, const LeaderDataTlv &aleaderDataB);

    /**
     * This method checks if the destination is reachable.
     *
     * @param[in]  aMeshSource  The RLOC16 of the source.
     * @param[in]  aMeshDest    The RLOC16 of the destination.
     * @param[in]  aIp6Header   A reference to the IPv6 header of the message.
     *
     * @retval kThreadError_None  The destination is reachable.
     * @retval kThreadError_Drop  The destination is not reachable and the message should be dropped.
     *
     */
    ThreadError CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header);

    /**
     * This method resolves 2-hop routing loops.
     *
     * @param[in]  aSourceMac   The RLOC16 of the previous hop.
     * @param[in]  aDestRloc16  The RLOC16 of the final destination.
     *
     */
    void ResolveRoutingLoops(uint16_t aSourceMac, uint16_t aDestRloc16);

    /**
     * This method checks if a given Router ID has correct value.
     *
     * @param[in]  aRouterId  The Router ID value.
     *
     * @retval TRUE   If @p aRouterId is in correct range [0..62].
     * @retval FALSE  If @p aRouterId is not a valid Router ID.
     *
     */
    static bool IsRouterIdValid(uint8_t aRouterId) { return aRouterId <= kMaxRouterId; }

    /**
     * This method fills an ConnectivityTlv.
     *
     * @param[out]  aTlv  A reference to the tlv to be filled.
     *
     */
    void FillConnectivityTlv(ConnectivityTlv &aTlv);

    /**
     * This method fills an RouteTlv.
     *
     * @param[out]  aTlv  A reference to the tlv to be filled.
     *
     */
    void FillRouteTlv(RouteTlv &aTlv);

private:
    enum
    {
        kDiscoveryMaxJitter = 250u,  ///< Maximum jitter time used to delay Discovery Responses in milliseconds.
        kStateUpdatePeriod = 1000u,  ///< State update period in milliseconds.
        kUnsolicitedDataResponseJitter = 500u,  ///< Maximum delay before unsolicited Data Response in milliseconds.
    };

    ThreadError AppendConnectivity(Message &aMessage);
    ThreadError AppendChildAddresses(Message &aMessage, Child &aChild);
    ThreadError AppendRoute(Message &aMessage);
    ThreadError AppendActiveDataset(Message &aMessage);
    ThreadError AppendPendingDataset(Message &aMessage);
    ThreadError GetChildInfo(Child &aChild, otChildInfo &aChildInfo);
    ThreadError HandleDetachStart(void);
    ThreadError HandleChildStart(otMleAttachFilter aFilter);
    ThreadError HandleLinkRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence);
    ThreadError HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence,
                                 bool request);
    ThreadError HandleLinkAcceptAndRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                           uint32_t aKeySequence);
    ThreadError HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleParentRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildIdRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                     uint32_t aKeySequence);
    ThreadError HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                          uint32_t aKeySequence);
    ThreadError HandleDataRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleNetworkDataUpdateRouter(void);
    ThreadError HandleDiscoveryRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    ThreadError ProcessRouteTlv(const RouteTlv &aRoute);
    void StopAdvertiseTimer(void);
    void ResetAdvertiseInterval(void);
    ThreadError SendAddressSolicit(ThreadStatusTlv::Status aStatus);
    ThreadError SendAddressRelease(void);
    void SendAddressSolicitResponse(const Coap::Header &aRequest, uint8_t aRouterId,
                                    const Ip6::MessageInfo &aMessageInfo);
    ThreadError SendAdvertisement(void);
    ThreadError SendLinkRequest(Neighbor *aNeighbor);
    ThreadError SendLinkAccept(const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor,
                               const TlvRequestTlv &aTlvRequest, const ChallengeTlv &aChallenge);
    ThreadError SendParentResponse(Child *aChild, const ChallengeTlv &aChallenge, bool aRoutersOnlyRequest);
    ThreadError SendChildIdResponse(Child *aChild);
    ThreadError SendChildUpdateRequest(Child *aChild);
    ThreadError SendChildUpdateResponse(Child *aChild, const Ip6::MessageInfo &aMessageInfo,
                                        const uint8_t *aTlvs, uint8_t aTlvsLength,  const ChallengeTlv *challenge);
    ThreadError SendDataResponse(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength,
                                 uint16_t aDelay);
    ThreadError SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId);

    ThreadError SetStateRouter(uint16_t aRloc16);
    ThreadError SetStateLeader(uint16_t aRloc16);
    void StopLeader(void);
    ThreadError UpdateChildAddresses(const AddressRegistrationTlv &aTlv, Child &aChild);
    void UpdateRoutes(const RouteTlv &aTlv, uint8_t aRouterId);

    static void HandleAddressSolicitResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                             const otMessageInfo *aMessageInfo, ThreadError result);
    void HandleAddressSolicitResponse(Coap::Header *aHeader, Message *aMessage,
                                      const Ip6::MessageInfo *aMessageInfo, ThreadError result);
    static void HandleAddressRelease(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleAddressRelease(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleAddressSolicit(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleAddressSolicit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static uint8_t LqiToCost(uint8_t aLqi);

    Child *NewChild(void);
    Child *FindChild(uint16_t aChildId);
    Child *FindChild(const Mac::ExtAddress &aMacAddr);

    void SetChildStateToValid(Child *aChild);
    bool HasChildren(void);
    void RemoveChildren(void);
    bool HasMinDowngradeNeighborRouters(void);
    bool HasOneNeighborwithComparableConnectivity(const RouteTlv &aRoute, uint8_t aRouterId);
    bool HasSmallNumberOfChildren(void);
    uint8_t GetMinDowngradeNeighborRouters(void);

    uint8_t AllocateRouterId(void);
    uint8_t AllocateRouterId(uint8_t aRouterId);
    bool InRouterIdMask(uint8_t aRouterId);

    static bool HandleAdvertiseTimer(void *aContext);
    bool HandleAdvertiseTimer(void);
    static void HandleStateUpdateTimer(void *aContext);
    void HandleStateUpdateTimer(void);

    TrickleTimer mAdvertiseTimer;
    Timer mStateUpdateTimer;

    Coap::Resource mAddressSolicit;
    Coap::Resource mAddressRelease;

    uint8_t mRouterIdSequence;
    uint32_t mRouterIdSequenceLastUpdated;
    Router mRouters[kMaxRouterId + 1];
    uint8_t mMaxChildrenAllowed;
    Child mChildren[kMaxChildren];

    uint8_t mChallengeTimeout;
    uint8_t mChallenge[8];
    uint16_t mNextChildId;
    uint8_t mNetworkIdTimeout;
    uint8_t mRouterUpgradeThreshold;
    uint8_t mRouterDowngradeThreshold;
    uint8_t mLeaderWeight;
    uint32_t mFixedLeaderPartitionId;  ///< only for certification testing
    bool mRouterRoleEnabled;
    bool mIsRouterRestoringChildren;

    uint8_t mRouterId;
    uint8_t mPreviousRouterId;
    uint32_t mPreviousPartitionId;
};

}  // namespace Mle

/**
 * @}
 */

}  // namespace Thread

#endif  // MLE_ROUTER_HPP_
