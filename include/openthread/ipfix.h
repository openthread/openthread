/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *  This file defines the OpenThread IPFIX module functions and APIs.
 */

#ifndef OPENTHREAD_IPFIX_H_
#define OPENTHREAD_IPFIX_H_

#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-ipfix
 *
 * @brief
 *   This module defines types and functions for observing and metering flows for the OTBR (IPFIX Flow Capture).
 *
 *   The functions in this module are available when `OPENTHREAD_CONFIG_IPFIX_ENABLE` is enabled.
 *
 * @{
 */

#define OT_IPFIX_MAX_FLOWS 256  ///< Maximum number of distinct flow entries to register during a time period.
#define OT_IPFIX_NBR_BUCKETS 16 ///< Number of buckets in the hash table.

/**
 * Represents observation points where the flow is observed by the IPFIX module.
 *
 */

typedef enum
{
    OT_IPFIX_OBSERVATION_POINT_WPAN_TO_RCP = 0, ///< Flow observed when going from WPAN interface to RCP.
    OT_IPFIX_OBSERVATION_POINT_RCP_TO_WPAN = 1, ///< Flow observed when going from RCP to WPAN interface.
    OT_IPFIX_OBSERVATION_POINT_OTBR_TO_AIL = 2, ///< Flow observed when going from OTBR to AIL interface.
    OT_IPFIX_OBSERVATION_POINT_AIL_TO_OTBR = 3, ///< Flow observed when going from AIL interface to OTBR.
} otIpfixFlowObservationPoint;

/**
 * Represents the network interfaces associated with a traffic flow (to obtain source and destination networks of a
 * flow).
 *
 */

typedef enum
{
    OT_IPFIX_INTERFACE_OTBR             = 0, ///< OpenThread Border Router (OTBR)
    OT_IPFIX_INTERFACE_THREAD_NETWORK   = 1, ///< Thread Network
    OT_IPFIX_INTERFACE_AIL_NETWORK      = 2, ///< AIL Network (Wi-Fi or Ethernet)
    OT_IPFIX_INTERFACE_WIFI_NETWORK     = 3, ///< Wi-Fi Network
    OT_IPFIX_INTERFACE_ETHERNET_NETWORK = 4, ///< Ethernet Network
} otIpfixFlowInterface;

/**
 * Represents the data structure used for storing IPFIX flow records.
 *
 */

typedef struct otIpfixFlowInfo
{
    otIp6Address         mSourceAddress;           ///< IPv6 source address.
    otIp6Address         mDestinationAddress;      ///< IPv6 destination address.
    uint16_t             mSourcePort;              ///< Source transport layer port.
    uint16_t             mDestinationPort;         ///< Destination transport layer port.
    uint8_t              mIpProto;                 ///< IP Protocol number.
    uint64_t             mPacketsCount;            ///< The number of IP packets.
    uint64_t             mBytesCount;              ///< The number of IP bytes.
    uint8_t              mIcmp6Type;               ///< ICMP6 type if message is ICMP6.
    uint8_t              mIcmp6Code;               ///< ICMP6 code if message is ICMP6.
    otIpfixFlowInterface mSourceNetwork;           ///< Network from which the flow originates (Source Network).
    otIpfixFlowInterface mDestinationNetwork;      ///< Network where the flow terminates (Destination Network).
    uint64_t             mFlowStartTime;           ///< Timestamp of first packet in milliseconds.
    uint64_t             mFlowEndTime;             ///< Timestamp of last packet in milliseconds.
    otExtAddress         mThreadSrcMacAddress;     ///< Source extended MAC address .
    otExtAddress         mThreadDestMacAddress;    ///< Destination extended MAC address.
    uint16_t             mThreadSrcRloc16Address;  ///< Source RLOC16 address.
    uint16_t             mThreadDestRloc16Address; ///< Destination RLOC16 address.
    uint64_t             mThreadFramesCount;       ///< Number of IEEE 802.15.4 frames.
} otIpfixFlowInfo;

/**
 * Returns the number of IPFIX flows records currently stored in the hashtable.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns Number of IPFIX flows records currently stored in the hashtable.
 *
 */

uint16_t otIpfixGetFlowCount(otInstance *aInstance);

/**
 * Returns the entire content of the hashtable (all IPFIX flow records) into the aFlowBuffer (linked list).
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aFlowBuffer  A pointer to the buffer where the IPFIX flow records will be copied.
 *
 */

void otIpfixGetFlowTable(otInstance *aInstance, otIpfixFlowInfo *aFlowBuffer);

/**
 * Resets the IPFIX hashtable (all the IPFIX flow records are reset).
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */

void otIpfixResetFlowTable(otInstance *aInstance);

/**
 * Metering process of the IPFIX exporter that meter the layer3 flows observed on the AIL (IPv6 traffic flow).
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aSrcAddress     IPv6 source address.
 * @param[in]  aDstAddress     IPv6 destination address.
 * @param[in]  aBuffer         Pointer to the payload buffer.
 * @param[in]  aBufferLength   Length of the payload.
 * @param[in]  aLocation       The observation point where the flow is observed.
 *
 */
extern void otIpfixMeterLayer3InfraFlowTraffic(otInstance                 *aInstance,
                                               const otIp6Address         *aSrcAddress,
                                               const otIp6Address         *aDstAddress,
                                               const uint8_t              *aBuffer,
                                               uint16_t                    aBufferLength,
                                               otIpfixFlowObservationPoint aLocation);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_IPFIX_H_
