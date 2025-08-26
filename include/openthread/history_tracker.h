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
 */

#ifndef OPENTHREAD_HISTORY_TRACKER_H_
#define OPENTHREAD_HISTORY_TRACKER_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/border_routing.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/netdata.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-history-tracker
 *
 * @brief
 *   Records the history of different events, for example RX and TX messages or network info changes. All tracked
 *   entries are timestamped.
 *
 * The functions in this module are available when `OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE` is enabled.
 *
 * @{
 */

/**
 * This constant specifies the maximum age of entries which is 49 days (in msec).
 *
 * Entries older than the max age will give this value as their age.
 */
#define OT_HISTORY_TRACKER_MAX_AGE (49 * 24 * 60 * 60 * 1000u)

#define OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE 21 ///< Recommended size for string representation of an entry age.

/**
 * Represents an iterator to iterate through a history list.
 *
 * The fields in this type are opaque (intended for use by OpenThread core) and therefore should not be accessed/used
 * by caller.
 *
 * Before using an iterator, it MUST be initialized using `otHistoryTrackerInitIterator()`,
 */
typedef struct otHistoryTrackerIterator
{
    uint32_t mData32;
    uint16_t mData16;
} otHistoryTrackerIterator;

/**
 * Represents Thread network info.
 */
typedef struct otHistoryTrackerNetworkInfo
{
    otDeviceRole     mRole;        ///< Device Role.
    otLinkModeConfig mMode;        ///< Device Mode.
    uint16_t         mRloc16;      ///< Device RLOC16.
    uint32_t         mPartitionId; ///< Partition ID (valid when attached).
} otHistoryTrackerNetworkInfo;

/**
 * Defines the events for an IPv6 (unicast or multicast) address info (i.e., whether address is added
 * or removed).
 */
typedef enum
{
    OT_HISTORY_TRACKER_ADDRESS_EVENT_ADDED   = 0, ///< Address is added.
    OT_HISTORY_TRACKER_ADDRESS_EVENT_REMOVED = 1, ///< Address is removed.
} otHistoryTrackerAddressEvent;

/**
 * Represent a unicast IPv6 address info.
 */
typedef struct otHistoryTrackerUnicastAddressInfo
{
    otIp6Address                 mAddress;       ///< The unicast IPv6 address.
    uint8_t                      mPrefixLength;  ///< The Prefix length (in bits).
    uint8_t                      mAddressOrigin; ///< The address origin (`OT_ADDRESS_ORIGIN_*` constants).
    otHistoryTrackerAddressEvent mEvent;         ///< Indicates the event (address is added/removed).
    uint8_t                      mScope : 4;     ///< The IPv6 scope.
    bool                         mPreferred : 1; ///< If the address is preferred.
    bool                         mValid : 1;     ///< If the address is valid.
    bool                         mRloc : 1;      ///< If the address is an RLOC.
} otHistoryTrackerUnicastAddressInfo;

/**
 * Represent an IPv6 multicast address info.
 */
typedef struct otHistoryTrackerMulticastAddressInfo
{
    otIp6Address                 mAddress;       ///< The IPv6 multicast address.
    uint8_t                      mAddressOrigin; ///< The address origin (`OT_ADDRESS_ORIGIN_*` constants).
    otHistoryTrackerAddressEvent mEvent;         ///< Indicates the event (address is added/removed).
} otHistoryTrackerMulticastAddressInfo;

/**
 * Constants representing message priority used in `otHistoryTrackerMessageInfo` struct.
 */
enum
{
    OT_HISTORY_TRACKER_MSG_PRIORITY_LOW    = OT_MESSAGE_PRIORITY_LOW,      ///< Low priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL = OT_MESSAGE_PRIORITY_NORMAL,   ///< Normal priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_HIGH   = OT_MESSAGE_PRIORITY_HIGH,     ///< High priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_NET    = OT_MESSAGE_PRIORITY_HIGH + 1, ///< Network Control priority level.
};

/**
 * Represents a RX/TX IPv6 message info.
 *
 * Some of the fields in this struct are applicable to a RX message or a TX message only, e.g., `mAveRxRss` is the
 * average RSS of all fragment frames that form a received message and is only applicable for a RX message.
 */
typedef struct otHistoryTrackerMessageInfo
{
    uint16_t   mPayloadLength;       ///< IPv6 payload length (exclude IP6 header itself).
    uint16_t   mNeighborRloc16;      ///< RLOC16 of neighbor which sent/received the msg (`0xfffe` if no RLOC16).
    otSockAddr mSource;              ///< Source IPv6 address and port (if UDP/TCP)
    otSockAddr mDestination;         ///< Destination IPv6 address and port (if UDP/TCP).
    uint16_t   mChecksum;            ///< Message checksum (valid only for UDP/TCP/ICMP6).
    uint8_t    mIpProto;             ///< IP Protocol number (`OT_IP6_PROTO_*` enumeration).
    uint8_t    mIcmp6Type;           ///< ICMP6 type if msg is ICMP6, zero otherwise (`OT_ICMP6_TYPE_*` enumeration).
    int8_t     mAveRxRss;            ///< RSS of received message or OT_RADIO_INVALID_RSSI if not known.
    bool       mLinkSecurity : 1;    ///< Indicates whether msg used link security.
    bool       mTxSuccess : 1;       ///< Indicates TX success (e.g., ack received). Applicable for TX msg only.
    uint8_t    mPriority : 2;        ///< Message priority (`OT_HISTORY_TRACKER_MSG_PRIORITY_*` enumeration).
    bool       mRadioIeee802154 : 1; ///< Indicates whether msg was sent/received over a 15.4 radio link.
    bool       mRadioTrelUdp6 : 1;   ///< Indicates whether msg was sent/received over a TREL radio link.
} otHistoryTrackerMessageInfo;

/**
 * Defines the events in a neighbor info (i.e. whether neighbor is added, removed, or changed).
 *
 * Event `OT_HISTORY_TRACKER_NEIGHBOR_EVENT_RESTORING` is applicable to child neighbors only. It is triggered after
 * the device (re)starts and when the previous children list is retrieved from non-volatile settings and the device
 * tries to restore connection to them.
 */
typedef enum
{
    OT_HISTORY_TRACKER_NEIGHBOR_EVENT_ADDED     = 0, ///< Neighbor is added.
    OT_HISTORY_TRACKER_NEIGHBOR_EVENT_REMOVED   = 1, ///< Neighbor is removed.
    OT_HISTORY_TRACKER_NEIGHBOR_EVENT_CHANGED   = 2, ///< Neighbor changed (e.g., device mode flags changed).
    OT_HISTORY_TRACKER_NEIGHBOR_EVENT_RESTORING = 3, ///< Neighbor is being restored (applicable to child only).
} otHistoryTrackerNeighborEvent;

/**
 * Represents a neighbor info.
 */
typedef struct otHistoryTrackerNeighborInfo
{
    otExtAddress mExtAddress;           ///< Neighbor's Extended Address.
    uint16_t     mRloc16;               ///< Neighbor's RLOC16.
    int8_t       mAverageRssi;          ///< Average RSSI of rx frames from neighbor at the time of recording entry.
    uint8_t      mEvent : 2;            ///< Indicates the event (`OT_HISTORY_TRACKER_NEIGHBOR_EVENT_*` enumeration).
    bool         mRxOnWhenIdle : 1;     ///< Rx-on-when-idle.
    bool         mFullThreadDevice : 1; ///< Full Thread Device.
    bool         mFullNetworkData : 1;  ///< Full Network Data.
    bool         mIsChild : 1;          ///< Indicates whether or not the neighbor is a child.
} otHistoryTrackerNeighborInfo;

/**
 * Defines the events in a router info (i.e. whether router is added, removed, or changed).
 */
typedef enum
{
    OT_HISTORY_TRACKER_ROUTER_EVENT_ADDED            = 0, ///< Router is added (router ID allocated).
    OT_HISTORY_TRACKER_ROUTER_EVENT_REMOVED          = 1, ///< Router entry is removed (router ID released).
    OT_HISTORY_TRACKER_ROUTER_EVENT_NEXT_HOP_CHANGED = 2, ///< Router entry next hop and cost changed.
    OT_HISTORY_TRACKER_ROUTER_EVENT_COST_CHANGED     = 3, ///< Router entry path cost changed (next hop as before).
} otHistoryTrackerRouterEvent;

#define OT_HISTORY_TRACKER_NO_NEXT_HOP 63 ///< No next hop - For `mNextHop` in `otHistoryTrackerRouterInfo`.

#define OT_HISTORY_TRACKER_INFINITE_PATH_COST 0 ///< Infinite path cost - used in `otHistoryTrackerRouterInfo`.

/**
 * Represents a router table entry event.
 */
typedef struct otHistoryTrackerRouterInfo
{
    uint8_t mEvent : 2;       ///< Router entry event (`OT_HISTORY_TRACKER_ROUTER_EVENT_*` enumeration).
    uint8_t mRouterId : 6;    ///< Router ID.
    uint8_t mNextHop;         ///< Next Hop Router ID - `OT_HISTORY_TRACKER_NO_NEXT_HOP` if no next hop.
    uint8_t mOldPathCost : 4; ///< Old path cost - `OT_HISTORY_TRACKER_INFINITE_PATH_COST` if infinite or unknown.
    uint8_t mPathCost : 4;    ///< New path cost - `OT_HISTORY_TRACKER_INFINITE_PATH_COST` if infinite or unknown.
} otHistoryTrackerRouterInfo;

/**
 * Defines the events for a Network Data entry (i.e., whether an entry is added or removed).
 */
typedef enum
{
    OT_HISTORY_TRACKER_NET_DATA_ENTRY_ADDED   = 0, ///< Network data entry is added.
    OT_HISTORY_TRACKER_NET_DATA_ENTRY_REMOVED = 1, ///< Network data entry is removed.
} otHistoryTrackerNetDataEvent;

/**
 * Represent a Network Data on mesh prefix info.
 */
typedef struct otHistoryTrackerOnMeshPrefixInfo
{
    otBorderRouterConfig         mPrefix; ///< The on mesh prefix entry.
    otHistoryTrackerNetDataEvent mEvent;  ///< Indicates the event (added/removed).
} otHistoryTrackerOnMeshPrefixInfo;

/**
 * Represent a Network Data extern route info.
 */
typedef struct otHistoryTrackerExternalRouteInfo
{
    otExternalRouteConfig        mRoute; ///< The external route entry.
    otHistoryTrackerNetDataEvent mEvent; ///< Indicates the event (added/removed).
} otHistoryTrackerExternalRouteInfo;

/**
 * Represents the DNS/SRP server address type parsed from Network Data service entries.
 */
typedef enum
{
    OT_HISTORY_TRACKER_DNS_SRP_ADDR_TYPE_UNICAST_LOCAL, ///< Unicast address type local (in server data).
    OT_HISTORY_TRACKER_DNS_SRP_ADDR_TYPE_UNICAST_INFRA, ///< Unicast address type infrastructure (in service data).
    OT_HISTORY_TRACKER_DNS_SRP_ADDR_TYPE_ANYCAST,       ///< Anycast address type.
} otHistoryTrackerDnsSrpAddrType;

/**
 * Represents DNS/SRP server address information parsed from a Network Data service entry.
 *
 * The `mType` field specifies the entry type. Some fields are only applicable to specific types.
 * - The `mPort` field is only applicable for `OT_HISTORY_TRACKER_DNS_SRP_ADDR_TYPE_UNICAST_*` types.
 * - The `mSequenceNumber` field is only applicable for the `OT_HISTORY_TRACKER_DNS_SRP_ADDR_TYPE_ANYCAST` type.
 * - Other fields are common and used for all address types.
 */
typedef struct otHistoryTrackerDnsSrpAddrInfo
{
    otIp6Address                   mAddress;        ///< The server address.
    uint16_t                       mRloc16;         ///< The RLOC16 of the Border Router adding/removing the entry.
    uint16_t                       mPort;           ///< Port number.
    uint8_t                        mSequenceNumber; ///< Anycast sequence number.
    uint8_t                        mVersion;        ///< Version number.
    otHistoryTrackerDnsSrpAddrType mType;           ///< Address type.
    otHistoryTrackerNetDataEvent   mEvent;          ///< Indicates the event (added/removed).
} otHistoryTrackerDnsSrpAddrInfo;

/**
 * Represents events during the Border Agent's ePSKc journey.
 */
typedef enum
{
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED,                   ///< ePSKc mode is activated.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED,                   ///< Secure session is connected.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED,                  ///< Commissioner petition is received.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_ACTIVE_DATASET,    ///< Active dataset is retrieved.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_PENDING_DATASET,   ///< Pending dataset is retrieved.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_KEEP_ALIVE,                  ///< Keep alive message is received.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_LOCAL_CLOSE,     ///< Deactivated by a call to the API.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_REMOTE_CLOSE,    ///< Disconnected by the peer.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_SESSION_ERROR,   ///< Disconnected due to some error.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_SESSION_TIMEOUT, ///< Disconnected due to timeout.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_MAX_ATTEMPTS,    ///< Max allowed attempts reached.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_EPSKC_TIMEOUT,   ///< ePSKc mode timed out.
    OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_UNKNOWN,         ///< Deactivated for an unknown reason.
} otHistoryTrackerBorderAgentEpskcEvent;

/**
 * Represents a favored OMR prefix tracked by a device acting as a Border Router (BR).
 *
 * The `mIsLocal` field indicates whether the favored OMR prefix is the same as the local one maintained by this BR.
 * The local OMR prefix can be either based on (random) ULA or a prefix delegated via DHCPv6-PD.
 */
typedef struct otHistoryTrackerFavoredOmrPrefix
{
    otIp6Prefix mOmrPrefix;      ///< The OMR prefix.
    signed int  mPreference : 2; ///< The 2-bit signed preference (`OT_ROUTE_PREFERENCE_*` values).
    bool        mIsLocal : 1;    ///< `true` if the prefix is the local OMR prefix; `false` otherwise.
} otHistoryTrackerFavoredOmrPrefix;

/**
 * Represents a favored on-link prefix on AIL tracked by a device acting as a Border Router (BR).
 *
 * The `mIsLocal` field indicates whether the favored on-link prefix is the same as the local one maintained by this
 * BR.
 */
typedef struct otHistoryTrackerFavoredOnLinkPrefix
{
    otIp6Prefix mOnLinkPrefix; ///< The on-link prefix.
    bool        mIsLocal : 1;  ///< `true` if the prefix is the local on-link prefix; `false` otherwise.
} otHistoryTrackerFavoredOnLinkPrefix;

/**
 * Represents the DHCPv6-PD state and delegated prefix (if any) by a device acting as Border Router (BR).
 */
typedef struct otHistoryTrackerDhcp6PdInfo
{
    otIp6Prefix                 mPrefix; ///< The delegated prefix if any. If none, it is set to `::/0`.
    otBorderRoutingDhcp6PdState mState;  ///< The DHCPv6 state.
} otHistoryTrackerDhcp6PdInfo;

/**
 * Defines events for discovered routers on an Adjacent Infrastructure Link (AIL).
 *
 * This applies when a device is acting as a Border Router, processing received Router Advertisements and tracking
 * AIL routers.
 *
 * `OT_HISTORY_TRACKER_AIL_ROUTER_EVENT_CHANGED` is used if any of the properties in the `otHistoryTrackerAilRouter`
 * structure associated with a specific router changes.
 */
typedef enum
{
    OT_HISTORY_TRACKER_AIL_ROUTER_EVENT_ADDED   = 0, ///< A new AIL router is discovered.
    OT_HISTORY_TRACKER_AIL_ROUTER_EVENT_CHANGED = 1, ///< A property in the router's information has changed.
    OT_HISTORY_TRACKER_AIL_ROUTER_EVENT_REMOVED = 2, ///< The AIL router is removed and no longer tracked.
} otHistoryTrackerAilRouterEvent;

/**
 * Represents information about a discovered router on an Adjacent Infrastructure Link (AIL).
 *
 * This applies when a device is acting as a Border Router, processing received Router Advertisements and tracking
 * information about discovered AIL routers.
 *
 * `mProvidesDefaultRoute` indicates whether the router provides a default route. If it does, `mDefRoutePreference`
 * specifies the route preference.
 *
 * `mFavoredOnLinkPrefix` indicates the favored on-link prefix advertised by the router. If there is no on-link prefix,
 * this will be an empty prefix (i.e., its length will be zero).
 */
typedef struct otHistoryTrackerAilRouter
{
    otHistoryTrackerAilRouterEvent mEvent;                        ///< The event type (e.g., added, changed, removed).
    int8_t                         mDefRoutePreference;           ///< Def route preference.
    otIp6Address                   mAddress;                      ///< The IPv6 address of the AIL router.
    otIp6Prefix                    mFavoredOnLinkPrefix;          ///< The favored on-link prefix, if any.
    bool                           mProvidesDefaultRoute : 1;     ///< Whether the router provides a default route.
    bool                           mManagedAddressConfigFlag : 1; ///< The Managed Address Config flag (`M` flag).
    bool                           mOtherConfigFlag : 1;          ///< The Other Config flag (`O` flag).
    bool                           mSnacRouterFlag : 1;           ///< The SNAC Router flag (`S` flag).
    bool                           mIsLocalDevice : 1;            ///< This router is the local device (this BR).
    bool                           mIsReachable : 1;              ///< This router is reachable.
    bool                           mIsPeerBr : 1;                 ///< This router is (likely) a peer BR.
} otHistoryTrackerAilRouter;

/**
 * Initializes an `otHistoryTrackerIterator`.
 *
 * An iterator MUST be initialized before it is used.
 *
 * An iterator can be initialized again to start from the beginning of the list.
 *
 * When iterating over entries in a list, to ensure the entry ages are consistent, the age is given relative to the
 * time the iterator was initialized, i.e., the entry age is provided as the duration (in milliseconds) from the event
 * (when entry was recorded) to the iterator initialization time.
 *
 * @param[in] aIterator  A pointer to the iterator to initialize (MUST NOT be NULL).
 */
void otHistoryTrackerInitIterator(otHistoryTrackerIterator *aIterator);

/**
 * Iterates over the entries in the network info history list.
 *
 * @param[in]     aInstance   A pointer to the OpenThread instance.
 * @param[in,out] aIterator   A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge   A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                            Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                            @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                            older than max age.
 *
 * @returns A pointer to `otHistoryTrackerNetworkInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerNetworkInfo *otHistoryTrackerIterateNetInfoHistory(otInstance               *aInstance,
                                                                         otHistoryTrackerIterator *aIterator,
                                                                         uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the unicast address history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns A pointer to `otHistoryTrackerUnicastAddressInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerUnicastAddressInfo *otHistoryTrackerIterateUnicastAddressHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the multicast address history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns A pointer to `otHistoryTrackerMulticastAddressInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerMulticastAddressInfo *otHistoryTrackerIterateMulticastAddressHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the RX message history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerMessageInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerMessageInfo *otHistoryTrackerIterateRxHistory(otInstance               *aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the TX message history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerMessageInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerMessageInfo *otHistoryTrackerIterateTxHistory(otInstance               *aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the neighbor history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerNeighborInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerNeighborInfo *otHistoryTrackerIterateNeighborHistory(otInstance               *aInstance,
                                                                           otHistoryTrackerIterator *aIterator,
                                                                           uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the router history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerRouterInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerRouterInfo *otHistoryTrackerIterateRouterHistory(otInstance               *aInstance,
                                                                       otHistoryTrackerIterator *aIterator,
                                                                       uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the Network Data on mesh prefix entry history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerOnMeshPrefixInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerOnMeshPrefixInfo *otHistoryTrackerIterateOnMeshPrefixHistory(otInstance               *aInstance,
                                                                                   otHistoryTrackerIterator *aIterator,
                                                                                   uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the Network Data external route entry history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerExternalRouteInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerExternalRouteInfo *otHistoryTrackerIterateExternalRouteHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the Network Data SRP/DNS address history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerDnsSrpAddrInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerDnsSrpAddrInfo *otHistoryTrackerIterateDnsSrpAddrHistory(otInstance               *aInstance,
                                                                               otHistoryTrackerIterator *aIterator,
                                                                               uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the Border Agent ePSKc history list.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerBorderAgentEpskcEvent` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerBorderAgentEpskcEvent *otHistoryTrackerIterateBorderAgentEpskcEventHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the favored OMR prefix history list.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` (device acting as Border Router).
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerFavoredOmrPrefix` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerFavoredOmrPrefix *otHistoryTrackerIterateFavoredOmrPrefixHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the favored on-link prefix history list.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` (device acting as Border Router).
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when the entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than the max age.
 *
 * @returns The `otHistoryTrackerFavoredOnLinkPrefix` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerFavoredOnLinkPrefix *otHistoryTrackerIterateFavoredOnLinkPrefixHistory(
    otInstance               *aInstance,
    otHistoryTrackerIterator *aIterator,
    uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the DHCPv6-PD history list.
 *
 * Requires both `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` and `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE`.
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerDhcp6PdInfo` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerDhcp6PdInfo *otHistoryTrackerIterateDhcp6PdHistory(otInstance               *aInstance,
                                                                         otHistoryTrackerIterator *aIterator,
                                                                         uint32_t                 *aEntryAge);

/**
 * Iterates over the entries in the BR AIL routers history list.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` (device acting as Border Router).
 *
 * @param[in]     aInstance  A pointer to the OpenThread instance.
 * @param[in,out] aIterator  A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]    aEntryAge  A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when the entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than the max age.
 *
 * @returns The `otHistoryTrackerAilRouter` entry or `NULL` if no more entries in the list.
 */
const otHistoryTrackerAilRouter *otHistoryTrackerIterateAilRoutersHistory(otInstance               *aInstance,
                                                                          otHistoryTrackerIterator *aIterator,
                                                                          uint32_t                 *aEntryAge);

/**
 * Converts a given entry age to a human-readable string.
 *
 * The entry age string follows the format `hours:minutes:seconds:milliseconds` (if
 * shorter than one day) or `days:hours:minutes:seconds`(if longer than one day).
 *
 * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be truncated
 * but the outputted string is always null-terminated.
 *
 * @param[in]  aEntryAge The entry age (duration in msec).
 * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be NULL).
 * @param[in]  aSize     The size of @p aBuffer. Recommended to use `OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE`.
 */
void otHistoryTrackerEntryAgeToString(uint32_t aEntryAge, char *aBuffer, uint16_t aSize);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_HISTORY_TRACKER_H_
