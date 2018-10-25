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
 * @brief
 *   This file defines the IOCTL interface for otLwf.sys.
 */

#ifndef __OTLWFIOCTL_H__
#define __OTLWFIOCTL_H__

__inline LONG ThreadErrorToNtstatus(otError error) { return (LONG)-((int)error); }

// User-mode IOCTL path for CreateFile
#define OTLWF_IOCLT_PATH      TEXT("\\\\.\\\\otlwf")

//
// IOCLTs and Data Types
//

#define OTLWF_CTL_CODE(request, method, access) \
    CTL_CODE(FILE_DEVICE_NETWORK, request, method, access)

// Different possible notification types
typedef enum _OTLWF_NOTIF_TYPE
{
    OTLWF_NOTIF_UNSPECIFIED,
    OTLWF_NOTIF_DEVICE_AVAILABILITY,
    OTLWF_NOTIF_STATE_CHANGE,
    OTLWF_NOTIF_DISCOVER,
    OTLWF_NOTIF_ACTIVE_SCAN,
    OTLWF_NOTIF_ENERGY_SCAN,
    OTLWF_NOTIF_COMMISSIONER_ENERGY_REPORT,
    OTLWF_NOTIF_COMMISSIONER_PANID_QUERY,
    OTLWF_NOTIF_JOINER_COMPLETE

} OTLWF_NOTIF_TYPE;

#define MAX_ENERGY_REPORT_LENGTH    64

//
// Queries (async) the next notification in the queue
//
#define IOCTL_OTLWF_QUERY_NOTIFICATION \
    OTLWF_CTL_CODE(0, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_NOTIFICATION {
        GUID                InterfaceGuid;
        OTLWF_NOTIF_TYPE    NotifType;
        union
        {
            // Payload for OTLWF_NOTIF_DEVICE_AVAILABILITY
            struct
            {
                BOOLEAN                 Available;
            } DeviceAvailabilityPayload;

            // Payload for OTLWF_NOTIF_STATE_CHANGE
            struct
            {
                uint32_t                Flags;
            } StateChangePayload;

            // Payload for OTLWF_NOTIF_DISCOVER
            struct
            {
                BOOLEAN                 Valid;
                otActiveScanResult      Results;
            } DiscoverPayload;

            // Payload for OTLWF_NOTIF_ACTIVE_SCAN
            struct
            {
                BOOLEAN                 Valid;
                otActiveScanResult      Results;
            } ActiveScanPayload;

            // Payload for OTLWF_NOTIF_ENERGY_SCAN
            struct
            {
                BOOLEAN                 Valid;
                otEnergyScanResult      Results;
            } EnergyScanPayload;

            // Payload for OTLWF_NOTIF_COMMISSIONER_ENERGY_REPORT
            struct
            {
                uint32_t                ChannelMask;
                uint8_t                 EnergyListLength;
                uint8_t                 EnergyList[MAX_ENERGY_REPORT_LENGTH];

            } CommissionerEnergyReportPayload;

            // Payload for OTLWF_NOTIF_COMMISSIONER_PANID_QUERY
            struct
            {
                uint16_t                PanId;
                uint32_t                ChannelMask;
            } CommissionerPanIdQueryPayload;

            // Payload for OTLWF_NOTIF_JOINER_COMPLETE
            struct
            {
                otError             Error;
            } JoinerCompletePayload;
        };
    } OTLWF_NOTIFICATION, *POTLWF_NOTIFICATION;

//
// Enumerates all the Thread interfaces guids
//
#define IOCTL_OTLWF_ENUMERATE_DEVICES \
    OTLWF_CTL_CODE(1, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_INTERFACE_LIST
    {
        uint16_t cInterfaceGuids;
        GUID     InterfaceGuids[1];
    } OTLWF_INTERFACE_LIST, *POTLWF_INTERFACE_LIST;

//
// Queries the detials of a given device Thread interfaces
//
#define IOCTL_OTLWF_QUERY_DEVICE \
    OTLWF_CTL_CODE(2, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid (in)
    typedef struct _OTLWF_DEVICE {
        ULONG           CompartmentID;
    } OTLWF_DEVICE, *POTLWF_DEVICE;

//
// Proxies to ot* APIs in otLwf.sys
//

/* REMOVED
#define IOCTL_OTLWF_OT_ENABLED \
    OTLWF_CTL_CODE(100, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aEnabled
*/
#define IOCTL_OTLWF_OT_INTERFACE \
    OTLWF_CTL_CODE(101, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aUp

#define IOCTL_OTLWF_OT_THREAD \
    OTLWF_CTL_CODE(102, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aStarted

#define IOCTL_OTLWF_OT_ACTIVE_SCAN \
    OTLWF_CTL_CODE(103, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aScanChannels
    // uint16_t - aScanDuration

#define IOCTL_OTLWF_OT_DISCOVER \
    OTLWF_CTL_CODE(104, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aScanChannels
    // uint16_t - aScanDuration
    // uint16_t - aPanid

#define IOCTL_OTLWF_OT_CHANNEL \
    OTLWF_CTL_CODE(105, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aChannel

#define IOCTL_OTLWF_OT_CHILD_TIMEOUT \
    OTLWF_CTL_CODE(106, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aTimeout

#define IOCTL_OTLWF_OT_EXTENDED_ADDRESS \
    OTLWF_CTL_CODE(107, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddress

#define IOCTL_OTLWF_OT_EXTENDED_PANID \
    OTLWF_CTL_CODE(108, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtendedPanId - aExtendedPanId

#define IOCTL_OTLWF_OT_LEADER_RLOC \
    OTLWF_CTL_CODE(109, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otIp6Address - aLeaderRloc

#define IOCTL_OTLWF_OT_LINK_MODE \
    OTLWF_CTL_CODE(110, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otLinkModeConfig - aConfig

#define IOCTL_OTLWF_OT_MASTER_KEY \
    OTLWF_CTL_CODE(111, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otMasterKey - aKey
    // uint8_t - aKeyLength

#define IOCTL_OTLWF_OT_MESH_LOCAL_EID \
    OTLWF_CTL_CODE(112, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otIp6Address - aMeshLocalEid

#define IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX \
    OTLWF_CTL_CODE(113, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otMeshLocalPrefix - aPrefix

#define IOCTL_OTLWF_OT_NETWORK_DATA_LEADER \
    OTLWF_CTL_CODE(114, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t[] - aData

#define IOCTL_OTLWF_OT_NETWORK_DATA_LOCAL \
    OTLWF_CTL_CODE(115, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t[] - aData

#define IOCTL_OTLWF_OT_NETWORK_NAME \
    OTLWF_CTL_CODE(116, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otNetworkName - aNetworkName

#define IOCTL_OTLWF_OT_PAN_ID \
    OTLWF_CTL_CODE(117, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otPanId - aPanId

#define IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED \
    OTLWF_CTL_CODE(118, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aEnabled

#define IOCTL_OTLWF_OT_SHORT_ADDRESS \
    OTLWF_CTL_CODE(119, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otShortAddress - aShortAddress

/* NOT USED
#define IOCTL_OTLWF_OT_UNICAST_ADDRESSES \
    OTLWF_CTL_CODE(120, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otNetifAddress[] - aAddresses
*/

#define IOCTL_OTLWF_OT_ACTIVE_DATASET \
    OTLWF_CTL_CODE(121, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otOperationalDataset - aDataset

#define IOCTL_OTLWF_OT_PENDING_DATASET \
    OTLWF_CTL_CODE(122, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otOperationalDataset - aDataset

#define IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT \
    OTLWF_CTL_CODE(123, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aWeight

#define IOCTL_OTLWF_OT_ADD_BORDER_ROUTER \
    OTLWF_CTL_CODE(124, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otBorderRouterConfig - aConfig

#define IOCTL_OTLWF_OT_REMOVE_BORDER_ROUTER \
    OTLWF_CTL_CODE(125, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otIp6Prefix - aPrefix

#define IOCTL_OTLWF_OT_ADD_EXTERNAL_ROUTE \
    OTLWF_CTL_CODE(126, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExternalRouteConfig - aConfig

#define IOCTL_OTLWF_OT_REMOVE_EXTERNAL_ROUTE \
    OTLWF_CTL_CODE(127, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otIp6Prefix - aPrefix

#define IOCTL_OTLWF_OT_SEND_SERVER_DATA \
    OTLWF_CTL_CODE(128, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY \
    OTLWF_CTL_CODE(129, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aDelay

#define IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER \
    OTLWF_CTL_CODE(130, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aKeySequenceCounter

#define IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT \
    OTLWF_CTL_CODE(131, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aTimeout

#define IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD \
    OTLWF_CTL_CODE(132, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aThreshold

#define IOCTL_OTLWF_OT_RELEASE_ROUTER_ID \
    OTLWF_CTL_CODE(133, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aRouterId

#define IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED \
    OTLWF_CTL_CODE(134, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aEnabled

#define IOCTL_OTLWF_OT_ADD_MAC_WHITELIST \
    OTLWF_CTL_CODE(135, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr
    // int8_t - aRssi (optional)

#define IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST \
    OTLWF_CTL_CODE(136, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr

#define IOCTL_OTLWF_OT_NEXT_MAC_WHITELIST \
    OTLWF_CTL_CODE(137, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aIterator (input)
    // uint8_t - aNewIterator (output)
    // otMacFilterEntry - aEntry (output)

#define IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST \
    OTLWF_CTL_CODE(138, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_DEVICE_ROLE \
    OTLWF_CTL_CODE(139, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otDeviceRole - aRole
    // otMleAttachFilter - aFilter (only for OT_DEVICE_ROLE_CHILD)

#define IOCTL_OTLWF_OT_CHILD_INFO_BY_ID \
    OTLWF_CTL_CODE(140, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint16_t - aChildId (input)
    // otChildInfo - aChildInfo (output)

#define IOCTL_OTLWF_OT_CHILD_INFO_BY_INDEX \
    OTLWF_CTL_CODE(141, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aChildIndex (input)
    // otChildInfo - aChildInfo (output)

#define IOCTL_OTLWF_OT_EID_CACHE_ENTRY \
    OTLWF_CTL_CODE(142, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aIndex (input)
    // otEidCacheEntry - aEntry (output)

#define IOCTL_OTLWF_OT_LEADER_DATA \
    OTLWF_CTL_CODE(143, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otLeaderData - aLeaderData

#define IOCTL_OTLWF_OT_LEADER_ROUTER_ID \
    OTLWF_CTL_CODE(144, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aRouterID

#define IOCTL_OTLWF_OT_LEADER_WEIGHT \
    OTLWF_CTL_CODE(145, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aWeight

#define IOCTL_OTLWF_OT_NETWORK_DATA_VERSION \
    OTLWF_CTL_CODE(146, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aVersion

#define IOCTL_OTLWF_OT_PARTITION_ID \
    OTLWF_CTL_CODE(147, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aPartition

#define IOCTL_OTLWF_OT_RLOC16 \
    OTLWF_CTL_CODE(148, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint16_t - aRloc16

#define IOCTL_OTLWF_OT_ROUTER_ID_SEQUENCE \
    OTLWF_CTL_CODE(149, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aIdSequence

#define IOCTL_OTLWF_OT_ROUTER_INFO \
    OTLWF_CTL_CODE(150, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint16_t - aRouterId (input)
    // otRouterInfo - aRouterInfo (output)

#define IOCTL_OTLWF_OT_STABLE_NETWORK_DATA_VERSION \
    OTLWF_CTL_CODE(151, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aVersion

#define IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED \
    OTLWF_CTL_CODE(152, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aEnabled

#define IOCTL_OTLWF_OT_ADD_MAC_BLACKLIST \
    OTLWF_CTL_CODE(153, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr

#define IOCTL_OTLWF_OT_REMOVE_MAC_BLACKLIST \
    OTLWF_CTL_CODE(154, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr

#define IOCTL_OTLWF_OT_NEXT_MAC_BLACKLIST \
    OTLWF_CTL_CODE(155, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aIterator (input)
    // uint8_t - aNewIterator (output)
    // otMacFilterEntry - aEntry (output)

#define IOCTL_OTLWF_OT_CLEAR_MAC_BLACKLIST \
    OTLWF_CTL_CODE(156, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

//#define IOCTL_OTLWF_OT_TRANSMIT_POWER                                 \
//    OTLWF_CTL_CODE(157, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // int8_t - aPower

#define IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX \
    OTLWF_CTL_CODE(158, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aLocal (input)
    // uint8_t - aIterator (input)
    // uint8_t - aNewIterator (output)
    // otBorderRouterConfig - aConfig (output)

#define IOCTL_OTLWF_OT_POLL_PERIOD \
    OTLWF_CTL_CODE(159, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aPollPeriod

#define IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID \
    OTLWF_CTL_CODE(160, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aPartitionId

#define IOCTL_OTLWF_OT_PLATFORM_RESET \
    OTLWF_CTL_CODE(162, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_PARENT_INFO \
    OTLWF_CTL_CODE(163, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otRouterInfo - aParentInfo

#define IOCTL_OTLWF_OT_SINGLETON \
    OTLWF_CTL_CODE(164, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aSingleton

#define IOCTL_OTLWF_OT_MAC_COUNTERS \
    OTLWF_CTL_CODE(165, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otMacCounters - aCounters

#define IOCTL_OTLWF_OT_MAX_CHILDREN \
    OTLWF_CTL_CODE(166, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aMaxChildren

#define IOCTL_OTLWF_OT_COMMISIONER_START \
    OTLWF_CTL_CODE(167, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_COMMISIONER_STOP \
    OTLWF_CTL_CODE(168, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define OPENTHREAD_PSK_MAX_LENGTH                32
#define OPENTHREAD_PROV_URL_MAX_LENGTH           64
#define OPENTHREAD_VENDOR_NAME_MAX_LENGTH        32
#define OPENTHREAD_VENDOR_MODEL_MAX_LENGTH       32
#define OPENTHREAD_VENDOR_SW_VERSION_MAX_LENGTH  16
#define OPENTHREAD_VENDOR_DATA_MAX_LENGTH        64
typedef struct otCommissionConfig
{
    char PSKd[OPENTHREAD_PSK_MAX_LENGTH + 1];
    char ProvisioningUrl[OPENTHREAD_PROV_URL_MAX_LENGTH + 1];
    char VendorName[OPENTHREAD_VENDOR_NAME_MAX_LENGTH + 1];
    char VendorModel[OPENTHREAD_VENDOR_MODEL_MAX_LENGTH + 1];
    char VendorSwVersion[OPENTHREAD_VENDOR_SW_VERSION_MAX_LENGTH + 1];
    char VendorData[OPENTHREAD_VENDOR_DATA_MAX_LENGTH + 1];
} otCommissionConfig;

#define IOCTL_OTLWF_OT_JOINER_START \
    OTLWF_CTL_CODE(169, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otCommissionConfig - aConfig

#define IOCTL_OTLWF_OT_JOINER_STOP \
    OTLWF_CTL_CODE(170, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_FACTORY_EUI64 \
    OTLWF_CTL_CODE(171, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aEui64

#define IOCTL_OTLWF_OT_JOINER_ID \
    OTLWF_CTL_CODE(172, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aEui64

#define IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD \
    OTLWF_CTL_CODE(173, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aThreshold

#define IOCTL_OTLWF_OT_COMMISSIONER_PANID_QUERY \
    OTLWF_CTL_CODE(174, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint16_t - aPanId
    // uint32_t - aChannekMask
    // otIp6Address - aAddress

#define IOCTL_OTLWF_OT_COMMISSIONER_ENERGY_SCAN \
    OTLWF_CTL_CODE(175, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aChannekMask
    // uint8_t - aCount
    // uint16_t - aPeriod
    // uint16_t - aScanDuration
    // otIp6Address - aAddress

#define IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER \
    OTLWF_CTL_CODE(176, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aRouterJitter

#define IOCTL_OTLWF_OT_JOINER_UDP_PORT \
    OTLWF_CTL_CODE(177, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint16_t - aJoinerUdpPort

#define IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_GET \
    OTLWF_CTL_CODE(178, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otIp6Address - aDestination
    // uint8_t - aCount
    // uint8_t[aCount] - aTlvTypes

#define IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_RESET \
    OTLWF_CTL_CODE(179, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otIp6Address - aDestination
    // uint8_t - aCount
    // uint8_t[aCount] - aTlvTypes

#define IOCTL_OTLWF_OT_COMMISIONER_ADD_JOINER \
    OTLWF_CTL_CODE(180, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aExtAddressValid
    // otExtAddress - aExtAddress (optional)
    // char[OPENTHREAD_PSK_MAX_LENGTH + 1] - aPSKd
    // uint32_t - aTimeout

#define IOCTL_OTLWF_OT_COMMISIONER_REMOVE_JOINER \
    OTLWF_CTL_CODE(181, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aExtAddressValid
    // otExtAddress - aExtAddress (optional)

#define IOCTL_OTLWF_OT_COMMISIONER_PROVISIONING_URL \
    OTLWF_CTL_CODE(182, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // char[OPENTHREAD_PROV_URL_MAX_LENGTH + 1] - aProvisioningUrl (optional)

#define IOCTL_OTLWF_OT_COMMISIONER_ANNOUNCE_BEGIN \
    OTLWF_CTL_CODE(183, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aChannelMask
    // uint8_t - aCount
    // uint16_t - aPeriod
    // otIp6Address - aAddress

#define IOCTL_OTLWF_OT_ENERGY_SCAN \
    OTLWF_CTL_CODE(184, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aScanChannels
    // uint16_t - aScanDuration

#define IOCTL_OTLWF_OT_SEND_ACTIVE_GET \
    OTLWF_CTL_CODE(185, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvTypes
    // otIp6Address - aAddress (optional)

#define IOCTL_OTLWF_OT_SEND_ACTIVE_SET \
    OTLWF_CTL_CODE(186, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otOperationalDataset - aDataset
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvTypes

#define IOCTL_OTLWF_OT_SEND_PENDING_GET \
    OTLWF_CTL_CODE(187, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvTypes
    // otIp6Address - aAddress (optional)

#define IOCTL_OTLWF_OT_SEND_PENDING_SET \
    OTLWF_CTL_CODE(188, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otOperationalDataset - aDataset
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvTypes

#define IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_GET \
    OTLWF_CTL_CODE(189, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvs

#define IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_SET \
    OTLWF_CTL_CODE(190, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otOperationalDataset - aDataset
    // uint8_t - aLength
    // uint8_t[aLength] - aTlvs

#define IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME \
    OTLWF_CTL_CODE(191, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint32_t - aKeySwitchGuardTime

#define IOCTL_OTLWF_OT_FACTORY_RESET \
    OTLWF_CTL_CODE(192, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_THREAD_AUTO_START \
    OTLWF_CTL_CODE(193, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aAutoStart

#define IOCTL_OTLWF_OT_PREFERRED_ROUTER_ID \
    OTLWF_CTL_CODE(194, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aRouterId

#define IOCTL_OTLWF_OT_PSKC \
    OTLWF_CTL_CODE(195, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otPSKc - aPSKc

#define IOCTL_OTLWF_OT_PARENT_PRIORITY \
    OTLWF_CTL_CODE(196, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // int8_t - aParentPriority

#define IOCTL_OTLWF_OT_ADD_MAC_FIXED_RSS \
    OTLWF_CTL_CODE(197, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr (optional)
    // int8_t - aRssi

#define IOCTL_OTLWF_OT_REMOVE_MAC_FIXED_RSS \
    OTLWF_CTL_CODE(198, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // otExtAddress - aExtAddr (optional)

#define IOCTL_OTLWF_OT_NEXT_MAC_FIXED_RSS \
    OTLWF_CTL_CODE(199, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aIterator (input)
    // uint8_t - aNewIterator (output)
    // otMacFilterEntry - aEntry (output)

#define IOCTL_OTLWF_OT_CLEAR_MAC_FIXED_RSS \
    OTLWF_CTL_CODE(200, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

#define IOCTL_OTLWF_OT_NEXT_ROUTE \
    OTLWF_CTL_CODE(201, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // BOOLEAN - aLocal (input)
    // uint8_t - aIterator (input)
    // uint8_t - aNewIterator (output)
    // otExternalRouteConfig - aConfig (output)

#define IOCTL_OTLWF_OT_MAX_ROUTER_ID \
    OTLWF_CTL_CODE(202, METHOD_BUFFERED, FILE_READ_DATA)
    // GUID - InterfaceGuid
    // uint8_t - aMaxRouterId

// OpenThread function IOCTL codes
#define MIN_OTLWF_IOCTL_FUNC_CODE 100
#define MAX_OTLWF_IOCTL_FUNC_CODE 202

#endif //__OTLWFIOCTL_H__
