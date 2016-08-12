/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 * @brief
 *  This file defines the types and structures used in the OpenThread library API.
 */

#ifndef OPENTHREAD_TYPES_H_
#define OPENTHREAD_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration represents error codes used throughout OpenThread.
 */
typedef enum ThreadError
{
    kThreadError_None = 0,
    kThreadError_Failed = 1,
    kThreadError_Drop = 2,
    kThreadError_NoBufs = 3,
    kThreadError_NoRoute = 4,
    kThreadError_Busy = 5,
    kThreadError_Parse = 6,
    kThreadError_InvalidArgs = 7,
    kThreadError_Security = 8,
    kThreadError_AddressQuery = 9,
    kThreadError_NoAddress = 10,
    kThreadError_NotReceiving = 11,
    kThreadError_Abort = 12,
    kThreadError_NotImplemented = 13,
    kThreadError_InvalidState = 14,
    kThreadError_NoTasklets = 15,

    /**
     * No acknowledgment was received after macMaxFrameRetries (IEEE 802.15.4-2006).
     */
    kThreadError_NoAck = 16,

    /**
     * A transmission could not take place due to activity on the channel, i.e., the CSMA-CA mechanism has failed
     * (IEEE 802.15.4-2006).
     */
    kThreadError_ChannelAccessFailure = 17,

    /**
     * Not currently attached to a Thread Partition.
     */
    kThreadError_Detached = 18,

    /**
     * FCS check failure while receiving.
     */
    kThreadError_FcsErr = 19,

    /**
     * No frame received.
     */
    kThreadError_NoFrameReceived = 20,

    /**
     * Received a frame from an unknown neighbor.
     */
    kThreadError_UnknownNeighbor = 21,

    /**
     * Received a frame from an invalid source address.
     */
    kThreadError_InvalidSourceAddress = 22,

    /**
     * Received a frame filtered by the whitelist.
     */
    kThreadError_WhitelistFiltered = 23,

    /**
     * Received a frame filtered by the destination address check.
     */
    kThreadError_DestinationAddressFiltered = 24,

    /**
     * The requested item could not be found.
     */
    kThreadError_NotFound = 25,

    /**
     * The operation is already in progress.
     */
    kThreadError_Already = 26,

    /**
     * Received a frame filtered by the blacklist.
     */
    kThreadError_BlacklistFiltered = 27,

    kThreadError_Error = 255,
} ThreadError;

#define OT_IP6_IID_SIZE            8   ///< Size of an IPv6 Interface Identifier (bytes)

#define OT_MASTER_KEY_SIZE         16  ///< Size of the Thread Master Key (bytes)

/**
 * This structure represents a Thread Master Key.
 *
 */
typedef struct otMasterKey
{
    uint8_t m8[OT_MASTER_KEY_SIZE];
} otMasterKey;

#define OT_NETWORK_NAME_MAX_SIZE   16  ///< Maximum size of the Thread Network Name field (bytes)

/**
 * This structure represents a Network Name.
 *
 */
typedef struct otNetworkName
{
    char m8[OT_NETWORK_NAME_MAX_SIZE + 1];
} otNetworkName;

#define OT_EXT_PAN_ID_SIZE         8   ///< Size of a Thread PAN ID (bytes)

/**
 * This structure represents an Extended PAN ID.
 *
 */
typedef struct otExtendedPanId
{
    uint8_t m8[OT_EXT_PAN_ID_SIZE];
} otExtendedPanId;

#define OT_MESH_LOCAL_PREFIX_SIZE  8  ///< Size of the Mesh Local Prefix (bytes)

/**
 * This structure represents a Mesh Local Prefix.
 *
 */
typedef struct otMeshLocalPrefix
{
    uint8_t m8[OT_MESH_LOCAL_PREFIX_SIZE];
} otMeshLocalPrefix;

/**
 * This type represents the IEEE 802.15.4 PAN ID.
 *
 */
typedef uint16_t otPanId;

/**
 * This type represents the IEEE 802.15.4 Short Address.
 *
 */
typedef uint16_t otShortAddress;

#define OT_EXT_ADDRESS_SIZE        8   ///< Size of an IEEE 802.15.4 Extended Address (bytes)

/**
 * This type represents the IEEE 802.15.4 Extended Address.
 *
 */
typedef struct otExtAddress
{
    uint8_t m8[OT_EXT_ADDRESS_SIZE];  ///< IEEE 802.15.4 Extended Address bytes
} otExtAddress;

#define OT_IP6_ADDRESS_SIZE        16  ///< Size of an IPv6 address (bytes)

/**
 * This structure represents an IPv6 address.
 */
typedef OT_TOOL_PACKED_BEGIN struct otIp6Address
{
    union
    {
        uint8_t  m8[OT_IP6_ADDRESS_SIZE];                      ///< 8-bit fields
        uint16_t m16[OT_IP6_ADDRESS_SIZE / sizeof(uint16_t)];  ///< 16-bit fields
        uint32_t m32[OT_IP6_ADDRESS_SIZE / sizeof(uint32_t)];  ///< 32-bit fields
    } mFields;                                                 ///< IPv6 accessor fields
} OT_TOOL_PACKED_END otIp6Address;

/**
 * @addtogroup commands  Commands
 *
 * @{
 *
 */

#define OT_PANID_BROADCAST   0xffff      ///< IEEE 802.15.4 Broadcast PAN ID

#define OT_CHANNEL_11_MASK   (1 << 11)   ///< Channel 11
#define OT_CHANNEL_12_MASK   (1 << 12)   ///< Channel 12
#define OT_CHANNEL_13_MASK   (1 << 13)   ///< Channel 13
#define OT_CHANNEL_14_MASK   (1 << 14)   ///< Channel 14
#define OT_CHANNEL_15_MASK   (1 << 15)   ///< Channel 15
#define OT_CHANNEL_16_MASK   (1 << 16)   ///< Channel 16
#define OT_CHANNEL_17_MASK   (1 << 17)   ///< Channel 17
#define OT_CHANNEL_18_MASK   (1 << 18)   ///< Channel 18
#define OT_CHANNEL_19_MASK   (1 << 19)   ///< Channel 19
#define OT_CHANNEL_20_MASK   (1 << 20)   ///< Channel 20
#define OT_CHANNEL_21_MASK   (1 << 21)   ///< Channel 21
#define OT_CHANNEL_22_MASK   (1 << 22)   ///< Channel 22
#define OT_CHANNEL_23_MASK   (1 << 23)   ///< Channel 23
#define OT_CHANNEL_24_MASK   (1 << 24)   ///< Channel 24
#define OT_CHANNEL_25_MASK   (1 << 25)   ///< Channel 25
#define OT_CHANNEL_26_MASK   (1 << 26)   ///< Channel 26

#define OT_CHANNEL_ALL       0xffffffff  ///< All channels

/**
 * This struct represents a received IEEE 802.15.4 Beacon.
 *
 */
typedef struct otActiveScanResult
{
    otExtAddress    mExtAddress;      ///< IEEE 802.15.4 Extended Address
    otNetworkName   mNetworkName;     ///< Thread Network Name
    otExtendedPanId mExtendedPanId;   ///< Thread Extended PAN ID
    uint16_t        mPanId;           ///< IEEE 802.15.4 PAN ID
    uint8_t         mChannel;         ///< IEEE 802.15.4 Channel
    int8_t          mRssi;            ///< RSSI (dBm)
    uint8_t         mLqi;             ///< LQI
    unsigned int    mVersion : 4;     ///< Version
    bool            mIsNative : 1;    ///< Native Commissioner flag
    bool            mIsJoinable : 1;  ///< Joining Permitted flag
} otActiveScanResult;

/**
 * @}
 *
 */

/**
 * @addtogroup config  Configuration
 *
 * @brief
 *   This module includes functions for configuration.
 *
 * @{
 *
 */

/**
 * @defgroup config-general  General
 *
 * @brief
 *   This module includes functions that manage configuration parameters for the Thread Child, Router, and Leader roles.
 *
 * @{
 *
 */

/**
 * This structure represents an Active or Pending Operational Dataset.
 *
 */
typedef struct otOperationalDataset
{
    uint64_t          mActiveTimestamp;            ///< Active Timestamp
    uint64_t          mPendingTimestamp;           ///< Pending Timestamp
    otMasterKey       mMasterKey;                  ///< Network Master Key
    otNetworkName     mNetworkName;                ///< Network Name
    otExtendedPanId   mExtendedPanId;              ///< Extended PAN ID
    otMeshLocalPrefix mMeshLocalPrefix;            ///< Mesh Local Prefix
    uint32_t          mDelay;                      ///< Delay Timer
    otPanId           mPanId;                      ///< PAN ID
    uint16_t          mChannel;                    ///< Channel

    bool              mIsActiveTimestampSet : 1;   ///< TRUE if Active Timestamp is set, FALSE otherwise.
    bool              mIsPendingTimestampSet : 1;  ///< TRUE if Pending Timestamp is set, FALSE otherwise.
    bool              mIsMasterKeySet : 1;         ///< TRUE if Network Master Key is set, FALSE otherwise.
    bool              mIsNetworkNameSet : 1;       ///< TRUE if Network Name is set, FALSE otherwise.
    bool              mIsExtendedPanIdSet : 1;     ///< TRUE if Extended PAN ID is set, FALSE otherwise.
    bool              mIsMeshLocalPrefixSet : 1;   ///< TRUE if Mesh Local Prefix is set, FALSE otherwise.
    bool              mIsDelaySet : 1;             ///< TRUE if Delay Timer is set, FALSE otherwise.
    bool              mIsPanIdSet : 1;             ///< TRUE if PAN ID is set, FALSE otherwise.
    bool              mIsChannelSet : 1;           ///< TRUE if Channel is set, FALSE otherwise.
} otOperationalDataset;

/**
 * This structure represents an MLE Link Mode configuration.
 */
typedef struct otLinkModeConfig
{
    /**
     * 1, if the sender has its receiver on when not transmitting.  0, otherwise.
     */
    bool mRxOnWhenIdle : 1;

    /**
     * 1, if the sender will use IEEE 802.15.4 to secure all data requests.  0, otherwise.
     */
    bool mSecureDataRequests : 1;

    /**
     * 1, if the sender is an FFD.  0, otherwise.
     */
    bool mDeviceType : 1;

    /**
     * 1, if the sender requires the full Network Data.  0, otherwise.
     */
    bool mNetworkData : 1;
} otLinkModeConfig;

/**
 * This enumeration represents flags that indicate what configuration or state has changed within OpenThread.
 *
 */
enum
{
    OT_IP6_ADDRESS_ADDED    = 1 << 0,  ///< IPv6 address was added
    OT_IP6_ADDRESS_REMOVED  = 1 << 1,  ///< IPv6 address was removed

    OT_NET_STATE            = 1 << 2,  ///< Device state (offline, detached, attached) changed
    OT_NET_ROLE             = 1 << 3,  ///< Device role (disabled, detached, child, router, leader) changed
    OT_NET_PARTITION_ID     = 1 << 4,  ///< Partition ID changed
    OT_NET_KEY_SEQUENCE     = 1 << 5,  ///< Thread Key Sequence changed

    OT_THREAD_CHILD_ADDED   = 1 << 6,  ///< Child was added
    OT_THREAD_CHILD_REMOVED = 1 << 7,  ///< Child was removed

    OT_IP6_LL_ADDR_CHANGED  = 1 << 8,  ///< The link-local address has changed
    OT_IP6_ML_ADDR_CHANGED  = 1 << 9,  ///< The mesh-local address has changed
};

/**
 * @}
 */

/**
 * @defgroup config-br  Border Router
 *
 * @brief
 *   This module includes functions that manage configuration parameters that apply to the Thread Border Router role.
 *
 * @{
 *
 */

/**
 * This structure represents an IPv6 prefix.
 */
typedef struct otIp6Prefix
{
    otIp6Address  mPrefix;  ///< The IPv6 prefix.
    uint8_t       mLength;  ///< The IPv6 prefix length.
} otIp6Prefix;

/**
 * This structure represents a Border Router configuration.
 */
typedef struct otBorderRouterConfig
{
    /**
     * The IPv6 prefix.
     */
    otIp6Prefix mPrefix;

    /**
     * A 2-bit signed integer indicating router preference as defined in RFC 4291.
     */
    int mPreference : 2;

    /**
     * TRUE, if @p mPrefix is preferred.  FALSE, otherwise.
     */
    bool mPreferred : 1;

    /**
     * TRUE, if @p mPrefix should be used for address autoconfiguration.  FALSE, otherwise.
     */
    bool mSlaac : 1;

    /**
     * TRUE, if this border router is a DHCPv6 Agent that supplies IPv6 address configuration.  FALSE, otherwise.
     */
    bool mDhcp : 1;

    /**
     * TRUE, if this border router is a DHCPv6 Agent that supplies other configuration data.  FALSE, otherwise.
     */
    bool mConfigure : 1;

    /**
     * TRUE, if this border router is a default route for @p mPrefix.  FALSE, otherwise.
     */
    bool mDefaultRoute : 1;

    /**
     * TRUE, if this prefix is considered on-mesh.  FALSE, otherwise.
     */
    bool mOnMesh : 1;

    /**
     * TRUE, if this configuration is considered Stable Network Data.  FALSE, otherwise.
     */
    bool mStable : 1;
} otBorderRouterConfig;

/**
 * This structure represents an External Route configuration.
 */
typedef struct otExternalRouteConfig
{
    /**
     * The prefix for the off-mesh route.
     */
    otIp6Prefix mPrefix;

    /**
     * A 2-bit signed integer indicating router preference as defined in RFC 4291.
     */
    int mPreference : 2;

    /**
     * TRUE, if this configuration is considered Stable Network Data.  FALSE, otherwise.
     */
    bool mStable : 1;
} otExternalRouteConfig;

/**
 * @}
 *
 */

/**
 * @defgroup config-test  Test
 *
 * @brief
 *   This module includes functions that manage configuration parameters required for Thread Certification testing.
 *
 * @{
 *
 */

/**
 * Represents any restrictions on the attach process.
 */
typedef enum otMleAttachFilter
{
    kMleAttachAnyPartition    = 0,  ///< Attach to any Thread partition.
    kMleAttachSamePartition   = 1,  ///< Attach to the same Thread partition.
    kMleAttachBetterPartition = 2,  ///< Attach to a better (i.e. higher weight/partition id) Thread partition.
} otMleAttachFilter;

/**
 * This structure represents a whitelist entry.
 *
 */
typedef struct otMacWhitelistEntry
{
    otExtAddress mExtAddress;       ///< IEEE 802.15.4 Extended Address
    int8_t       mRssi;             ///< RSSI value
    bool         mValid : 1;        ///< Indicates whether or not the whitelist entry is vaild
    bool         mFixedRssi : 1;    ///< Indicates whether or not the RSSI value is fixed.
} otMacWhitelistEntry;

/**
 * This structure represents a blacklist entry.
 *
 */
typedef struct otMacBlacklistEntry
{
    otExtAddress mExtAddress;       ///< IEEE 802.15.4 Extended Address
    bool         mValid;            ///< Indicates whether or not the blacklist entry is vaild
} otMacBlacklistEntry;

/**
 * @}
 *
 */

/**
 * @}
 *
 */

/**
 * @addtogroup diags  Diagnostics
 *
 * @brief
 *   This module includes functions that expose internal state.
 *
 * @{
 *
 */

/**
 * Represents a Thread device role.
 */
typedef enum
{
    kDeviceRoleDisabled,  ///< The Thread stack is disabled.
    kDeviceRoleDetached,  ///< Not currently participating in a Thread network/partition.
    kDeviceRoleChild,     ///< The Thread Child role.
    kDeviceRoleRouter,    ///< The Thread Router role.
    kDeviceRoleLeader,    ///< The Thread Leader role.
} otDeviceRole;

/**
 * This structure holds diagnostic information for a Thread Child
 *
 */
typedef struct
{
    otExtAddress   mExtAddress;            ///< IEEE 802.15.4 Extended Address
    uint32_t       mTimeout;               ///< Timeout
    uint32_t       mAge;                   ///< Time last heard
    uint16_t       mRloc16;                ///< RLOC16
    uint16_t       mChildId;               ///< Child ID
    uint8_t        mNetworkDataVersion;    ///< Network Data Version
    uint8_t        mLinkQualityIn;         ///< Link Quality In
    int8_t         mAverageRssi;           ///< Average RSSI
    bool           mRxOnWhenIdle : 1;      ///< rx-on-when-idle
    bool           mSecureDataRequest : 1; ///< Secure Data Requests
    bool           mFullFunction : 1;      ///< Full Function Device
    bool           mFullNetworkData : 1;   ///< Full Network Data
} otChildInfo;

/**
 * This structure holds diagnostic information for a Thread Router
 *
 */
typedef struct
{
    otExtAddress   mExtAddress;            ///< IEEE 802.15.4 Extended Address
    uint16_t       mRloc16;                ///< RLOC16
    uint8_t        mRouterId;              ///< Router ID
    uint8_t        mNextHop;               ///< Next hop to router
    uint8_t        mPathCost;              ///< Path cost to router
    uint8_t        mLinkQualityIn;         ///< Link Quality In
    uint8_t        mLinkQualityOut;        ///< Link Quality Out
    uint8_t        mAge;                   ///< Time last heard
    bool           mAllocated : 1;         ///< Router ID allocated or not
    bool           mLinkEstablished : 1;   ///< Link established with Router ID or not
} otRouterInfo;

/**
 * This structure represents an EID cache entry.
 *
 */
typedef struct otEidCacheEntry
{
    otIp6Address    mTarget;          ///< Target
    otShortAddress  mRloc16;          ///< RLOC16
    bool            mValid : 1;       ///< Indicates whether or not the cache entry is valid
} otEidCacheEntry;

/**
 * This structure represents the Thread Leader Data.
 *
 */
typedef struct otLeaderData
{
    uint32_t mPartitionId;            ///< Partition ID
    uint8_t mWeighting;               ///< Leader Weight
    uint8_t mDataVersion;             ///< Full Network Data Version
    uint8_t mStableDataVersion;       ///< Stable Network Data Version
    uint8_t mLeaderRouterId;          ///< Leader Router ID
} otLeaderData;

/**
 * This structure represents the MAC layer counters.
 */
typedef struct otMacCounters
{
    uint32_t mTxTotal;                ///< The total number of transmissions.
    uint32_t mTxAckRequested;         ///< The number of transmissions with ack request.
    uint32_t mTxAcked;                ///< The number of transmissions that were acked.
    uint32_t mTxNoAckRequested;       ///< The number of transmissions without ack request.
    uint32_t mTxData;                 ///< The number of transmitted data.
    uint32_t mTxDataPoll;             ///< The number of transmitted data poll.
    uint32_t mTxBeacon;               ///< The number of transmitted beacon.
    uint32_t mTxBeaconRequest;        ///< The number of transmitted beacon request.
    uint32_t mTxOther;                ///< The number of transmitted other types of frames.
    uint32_t mTxRetry;                ///< The number of retransmission times.
    uint32_t mTxErrCca;               ///< The number of CCA failure times.
    uint32_t mRxTotal;                ///< The total number of received packets.
    uint32_t mRxData;                 ///< The number of received data.
    uint32_t mRxDataPoll;             ///< The number of received data poll.
    uint32_t mRxBeacon;               ///< The number of received beacon.
    uint32_t mRxBeaconRequest;        ///< The number of received beacon request.
    uint32_t mRxOther;                ///< The number of received other types of frames.
    uint32_t mRxWhitelistFiltered;    ///< The number of received packets filtered by whitelist.
    uint32_t mRxDestAddrFiltered;     ///< The number of received packets filtered by destination check.
    uint32_t mRxErrNoFrame;           ///< The number of received packets that do not contain contents.
    uint32_t mRxErrUnknownNeighbor;   ///< The number of received packets from unknown neighbor.
    uint32_t mRxErrInvalidSrcAddr;    ///< The number of received packets whose source address is invalid.
    uint32_t mRxErrSec;               ///< The number of received packets with security error.
    uint32_t mRxErrFcs;               ///< The number of received packets with FCS error.
    uint32_t mRxErrOther;             ///< The number of received packets with other error.
} otMacCounters;

/**
 * @}
 *
 */

/**
 * This structure represents an IPv6 network interface address.
 *
 */
typedef struct otNetifAddress
{
    otIp6Address           mAddress;            ///< The IPv6 address.
    uint32_t               mPreferredLifetime;  ///< The Preferred Lifetime.
    uint32_t               mValidLifetime;      ///< The Valid lifetime.
    uint8_t                mPrefixLength;       ///< The Prefix length.
    struct otNetifAddress *mNext;               ///< A pointer to the next network interface address.
} otNetifAddress;

/**
 * @addtogroup messages  Message Buffers
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers
 *
 * @{
 *
 */

/**
 * This type points to an OpenThread message buffer.
 */
typedef void *otMessage;

/**
 * @}
 *
 */

/**
 * @addtogroup udp  UDP
 *
 * @brief
 *   This module includes functions that control UDP communication.
 *
 * @{
 *
 */

/**
 * This structure represents an IPv6 socket address.
 */
typedef struct otSockAddr
{
    otIp6Address mAddress;  ///< An IPv6 address.
    uint16_t     mPort;     ///< A transport-layer port.
    int8_t       mScopeId;  ///< An IPv6 scope identifier.
} otSockAddr;

/**
 * This structure represents the local and peer IPv6 socket addresses.
 */
typedef struct otMessageInfo
{
    otIp6Address mSockAddr;     ///< The local IPv6 address.
    otIp6Address mPeerAddr;     ///< The peer IPv6 address.
    uint16_t     mSockPort;     ///< The local transport-layer port.
    uint16_t     mPeerPort;     ///< The peer transport-layer port.
    int8_t       mInterfaceId;  ///< An IPv6 interface identifier.
    uint8_t      mHopLimit;     ///< The IPv6 Hop Limit.
    const void  *mLinkInfo;     ///< A pointer to link-specific information.
} otMessageInfo;

/**
 * This callback allows OpenThread to inform the application of a received UDP message.
 */
typedef void (*otUdpReceive)(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);

/**
 * This structure represents a UDP socket.
 */
typedef struct otUdpSocket
{
    otSockAddr           mSockName;  ///< The local IPv6 socket address.
    otSockAddr           mPeerName;  ///< The peer IPv6 socket address.
    otUdpReceive         mHandler;   ///< A function pointer to the application callback.
    void                *mContext;   ///< A pointer to application-specific context.
    struct otUdpSocket *mNext;       ///< A pointer to the next UDP socket.
} otUdpSocket;

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_TYPES_H_
