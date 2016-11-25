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
 * This class implements functionality required for delaying MLE responses.
 *
 */
OT_TOOL_PACKED_BEGIN
class DelayedResponseHeader
{
public:
    /**
     * Default constructor for the object.
     *
     */
    DelayedResponseHeader(void) { memset(this, 0, sizeof(*this)); };

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aSendTime     Time when the message shall be sent.
     * @param[in]  aDestination  IPv6 address of the message destination.
     *
     */
    DelayedResponseHeader(uint32_t aSendTime, const Ip6::Address &aDestination) {
        mSendTime = aSendTime;
        mDestination = aDestination;
    };

    /**
     * This method appends delayed response header to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the bytes.
     * @retval kThreadError_NoBufs  Insufficient available buffers to grow the message.
     *
     */
    ThreadError AppendTo(Message &aMessage) {
        return aMessage.Append(this, sizeof(*this));
    };

    /**
     * This method reads delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(Message &aMessage) {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    };

    /**
     * This method removes delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None  Successfully removed the header.
     *
     */
    static ThreadError RemoveFrom(Message &aMessage) {
        return aMessage.SetLength(aMessage.GetLength() - sizeof(DelayedResponseHeader));
    };

    /**
     * This method returns a time when the message shall be sent.
     *
     * @returns  A time when the message shall be sent.
     *
     */
    uint32_t GetSendTime(void) const { return mSendTime; };

    /**
     * This method returns a destination of the delayed message.
     *
     * @returns  A destination of the delayed message.
     *
     */
    const Ip6::Address &GetDestination(void) const { return mDestination; };

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) > 0); };

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) < 0); };

private:
    Ip6::Address mDestination;  ///< IPv6 address of the message destination.
    uint32_t mSendTime;         ///< Time when the message shall be sent.
} OT_TOOL_PACKED_END;

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
    uint8_t GetLeaderWeight(void) const;

    /**
     * This method sets the Leader Weighting value for this Thread interface.
     *
     * @param[in]  aWeight  The Leader Weighting value.
     *
     */
    void SetLeaderWeight(uint8_t aWeight);

    /**
     * This method returns the fixed Partition Id of Thread network partition for certification testing.
     *
     * @returns The Partition Id for this Thread network partition.
     *
     */
    uint32_t GetLeaderPartitionId(void) const;

    /**
     * This method sets the fixed Partition Id for Thread network partition for certification testing.
     *
     * @param[in]  aPartitionId  The Leader Partition Id.
     *
     */
    void SetLeaderPartitionId(uint32_t aPartitionId);

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
     * This method sets the Router Id from the stored network information.
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
    uint16_t GetNextHop(uint16_t aDestination) const;

    /**
     * This method returns the NETWORK_ID_TIMEOUT value.
     *
     * @returns The NETWORK_ID_TIMEOUT value.
     *
     */
    uint8_t GetNetworkIdTimeout(void) const;

    /**
     * This method sets the NETWORK_ID_TIMEOUT value.
     *
     * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
     *
     */
    void SetNetworkIdTimeout(uint8_t aTimeout);

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
    uint8_t GetRouterIdSequence(void) const;

    /**
     * This method returns the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     *
     */
    uint8_t GetRouterUpgradeThreshold(void) const;

    /**
     * This method sets the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     *
     */
    void SetRouterUpgradeThreshold(uint8_t aThreshold);

    /**
     * This method returns the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     *
     */
    uint8_t GetRouterDowngradeThreshold(void) const;

    /**
     * This method sets the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     *
     */
    void SetRouterDowngradeThreshold(uint8_t aThreshold);

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
     * @retval  kThreadErrorNone           Successfully set the max.
     * @retval  kThreadError_InvalidArgs   If @p aMaxChildren is not in the range [1, kMaxChildren].
     * @retval  kThreadError_InvalidState  If MLE has already been started.
     *
     */
    ThreadError SetMaxAllowedChildren(uint8_t aMaxChildren);

    /**
     * This method restores children information from non-volatile memory.
     *
     * @retval  kThreadErrorNone      Successfully restores children information.
     * @retval  kThreadError_NoBufs   Insufficient available buffers to restore all children information.
     *
     */
    ThreadError RestoreChildren(void);

    /**
     * This method remove a stored child information from non-volatile memory.
     *
     * @param[in]  aChildRloc16   The child RLOC16 to remove.
     *
     * @retval  kThreadErrorNone        Successfully remove child.
     * @retval  kThreadError_NotFound   There is no specified child stored in non-volatile memory.
     *
     */
    ThreadError RemoveStoredChild(uint16_t aChildRloc16);

    /**
     * This method store a child information into non-volatile memory.
     *
     * @param[in]  aChildRloc16   The child RLOC16 to store.
     *
     * @retval  kThreadErrorNone      Successfully store child.
     * @retval  kThreadError_NoBufs   Insufficient available buffers to store child.
     *
     */
    ThreadError StoreChild(uint16_t aChildRloc16);

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

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const MessageQueue &GetMessageQueue(void) const { return mDelayedResponses; }

private:
    enum
    {
        kStateUpdatePeriod = 1000u,  ///< State update period in milliseconds.
    };

    ThreadError AppendConnectivity(Message &aMessage);
    ThreadError AppendChildAddresses(Message &aMessage, Child &aChild);
    ThreadError AppendRoute(Message &aMessage);
    ThreadError AppendActiveDataset(Message &aMessage);
    ThreadError AppendPendingDataset(Message &aMessage);
    void GetChildInfo(Child &aChild, otChildInfo &aChildInfo);
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
    void ResetAdvertiseInterval(void);
    ThreadError SendAddressSolicit(ThreadStatusTlv::Status aStatus);
    ThreadError SendAddressRelease(void);
    void SendAddressSolicitResponse(const Coap::Header &aRequest, uint8_t aRouterId,
                                    const Ip6::MessageInfo &aMessageInfo);
    void SendAddressReleaseResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo);
    ThreadError SendAdvertisement(void);
    ThreadError SendLinkRequest(Neighbor *aNeighbor);
    ThreadError SendLinkAccept(const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor,
                               const TlvRequestTlv &aTlvRequest, const ChallengeTlv &aChallenge);
    ThreadError SendParentResponse(Child *aChild, const ChallengeTlv &aChallenge, bool aRoutersOnlyRequest);
    ThreadError SendChildIdResponse(Child *aChild);
    ThreadError SendChildUpdateRequest(Child *aChild);
    ThreadError SendChildUpdateResponse(Child *aChild, const Ip6::MessageInfo &aMessageInfo,
                                        const uint8_t *aTlvs, uint8_t aTlvsLength,  const ChallengeTlv *challenge);
    ThreadError SendDataResponse(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength);
    ThreadError SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId);

    ThreadError SetStateRouter(uint16_t aRloc16);
    ThreadError SetStateLeader(uint16_t aRloc16);
    void StopLeader(void);
    ThreadError UpdateChildAddresses(const AddressRegistrationTlv &aTlv, Child &aChild);
    void UpdateRoutes(const RouteTlv &aTlv, uint8_t aRouterId);

    static void HandleAddressSolicitResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                             ThreadError result);
    void HandleAddressSolicitResponse(Coap::Header *aHeader, Message *aMessage, ThreadError result);
    static void HandleAddressRelease(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleAddressRelease(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleAddressSolicit(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleAddressSolicit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static uint8_t LqiToCost(uint8_t aLqi);

    Child *NewChild(void);
    Child *FindChild(uint16_t aChildId);
    Child *FindChild(const Mac::ExtAddress &aMacAddr);

    bool HasChildren(void);
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
    static void HandleDelayedResponseTimer(void *aContext);
    void HandleDelayedResponseTimer(void);
    static void HandleChildUpdateRequestTimer(void *aContext);
    void HandleChildUpdateRequestTimer(void);

    ThreadError AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay);

    MessageQueue mDelayedResponses;

    TrickleTimer mAdvertiseTimer;
    Timer mStateUpdateTimer;
    Timer mDelayedResponseTimer;
    Timer mChildUpdateRequestTimer;

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

    uint8_t mRouterId;
    uint8_t mPreviousRouterId;

    Coap::Server &mCoapServer;
    Coap::Client &mCoapClient;
};

}  // namespace Mle

/**
 * @}
 */

}  // namespace Thread

#endif  // MLE_ROUTER_HPP_
