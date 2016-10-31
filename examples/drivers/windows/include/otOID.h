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
 *   This file defines the OID interface between otLwf and it's miniport.
 */

#ifndef __OTOID_H__
#define __OTOID_H__

#pragma once

//
// Macros for defining native OpenThread OIDs
//

#define OT_OPERATIONAL_OID      (0x01U)
#define OT_STATISTICS_OID       (0x02U) 

#define OT_MANDATORY_OID        (0x01U)
#define OT_OPTIONAL_OID         (0x02U)

#define OT_DEFINE_OID(Seq,o,m)  ((0xD0000000U) | ((o) << 16) | ((m) << 8) | (Seq))

//
// OpenThread Status Indication codes (and associated payload types)
//

#define NDIS_STATUS_OT_ENERGY_SCAN_RESULT           ((NDIS_STATUS)0x40050000L)
    typedef struct _OT_ENERGY_SCAN_RESULT
    {
        NDIS_STATUS         Status;
        CHAR                MaxRssi;
    } OT_ENERGY_SCAN_RESULT, * POT_ENERGY_SCAN_RESULT;

//
// General OID Definitions
//

// Used to query initial constants of the miniport
#define OID_OT_CAPABILITIES                         OT_DEFINE_OID(0, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef enum OT_MP_MODE
    {
        OT_MP_MODE_RADIO,  // Supports the physical radio layer
        OT_MP_MODE_THREAD  // Supports the full Thread stack
    } OT_MP_MODE;
    typedef enum OT_RADIO_CAPABILITY
    {
        // Radio supports Ack timeouts internally
        OT_RADIO_CAP_ACK_TIMEOUT                        = 1 << 0,
        // Radio supports MAC retry logic and timers; as well as collision avoidance.
        OT_RADIO_CAP_MAC_RETRY_AND_COLLISION_AVOIDANCE  = 1 << 1,
        // Radio supports sleeping. If the device supports sleeping, it is assumed to
        // default to the sleep state on bring up.
        OT_RADIO_CAP_SLEEP                              = 1 << 2,
    } OT_RADIO_CAPABILITY;
    typedef struct _OT_CAPABILITIES
    {
        #define OT_CAPABILITIES_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        OT_MP_MODE         MiniportMode;
        USHORT             RadioCapabilities;  // OT_RADIO_CAPABILITY flags
    } OT_CAPABILITIES, * POT_CAPABILITIES;

#define SIZEOF_OT_CAPABILITIES_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_CAPABILITIES, RadioCapabilities)

//
// Radio Mode OIDs
//

// Used to query/set sleep mode; only used if RadioCapabilities 
// indicates support for OT_RADIO_CAP_SLEEP.
#define OID_OT_SLEEP_MODE                           OT_DEFINE_OID(100, OT_OPERATIONAL_OID, OT_OPTIONAL_OID)
    typedef struct _OT_SLEEP_MODE
    {
        #define OT_SLEEP_MODE_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        BOOLEAN            InSleepMode;
    } OT_SLEEP_MODE, * POT_SLEEP_MODE;

#define SIZEOF_OT_SLEEP_MODE_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_SLEEP_MODE, InSleepMode)

// Used to query/set promiscuous mode
#define OID_OT_PROMISCUOUS_MODE                     OT_DEFINE_OID(101, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_PROMISCUOUS_MODE
    {
        #define OT_PROMISCUOUS_MODE_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        BOOLEAN            InPromiscuousMode;
    } OT_PROMISCUOUS_MODE, * POT_PROMISCUOUS_MODE;

#define SIZEOF_OT_PROMISCUOUS_MODE_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_PROMISCUOUS_MODE, InPromiscuousMode)

// Used to query the factory Extended Address
#define OID_OT_FACTORY_EXTENDED_ADDRESS             OT_DEFINE_OID(102, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_FACTORY_EXTENDED_ADDRESS
    {
        #define OT_FACTORY_EXTENDED_ADDRESS_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        ULONGLONG          ExtendedAddress;
    } OT_FACTORY_EXTENDED_ADDRESS, * POT_FACTORY_EXTENDED_ADDRESS;

#define SIZEOF_OT_FACTORY_EXTENDED_ADDRESS_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_FACTORY_EXTENDED_ADDRESS, ExtendedAddress)

// Used to query/set the Pan ID
#define OID_OT_PAND_ID                              OT_DEFINE_OID(103, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_PAND_ID
    {
        #define OT_PAND_ID_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        USHORT             PanID;
    } OT_PAND_ID, * POT_PAND_ID;

#define SIZEOF_OT_PAND_ID_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_PAND_ID, PanID)

// Used to query/set the Short Address
#define OID_OT_SHORT_ADDRESS                        OT_DEFINE_OID(104, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_SHORT_ADDRESS
    {
        #define OT_SHORT_ADDRESS_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        USHORT             ShortAddress;
    } OT_SHORT_ADDRESS, * POT_SHORT_ADDRESS;

#define SIZEOF_OT_SHORT_ADDRESS_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_SHORT_ADDRESS, ShortAddress)

// Used to query/set the Extended Address
#define OID_OT_EXTENDED_ADDRESS                     OT_DEFINE_OID(105, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_EXTENDED_ADDRESS
    {
        #define OT_EXTENDED_ADDRESS_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        ULONGLONG          ExtendedAddress;
    } OT_EXTENDED_ADDRESS, * POT_EXTENDED_ADDRESS;

#define SIZEOF_OT_EXTENDED_ADDRESS_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_EXTENDED_ADDRESS, ExtendedAddress)

// Used to query/set the current listening channel
#define OID_OT_CURRENT_CHANNEL                      OT_DEFINE_OID(106, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_CURRENT_CHANNEL
    {
        #define OT_CURRENT_CHANNEL_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        UCHAR              Channel;
    } OT_CURRENT_CHANNEL, * POT_CURRENT_CHANNEL;

#define SIZEOF_OT_CURRENT_CHANNEL_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_CURRENT_CHANNEL, Channel)

// Used to query the current RSSI for the current channel
#define OID_OT_RSSI                                 OT_DEFINE_OID(107, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_RSSI
    {
        #define OT_RSSI_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        CHAR               Rssi;
    } OT_RSSI, * POT_RSSI;

#define SIZEOF_OT_RSSI_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_RSSI, Rssi)

// The maximum of each type (short or extended) of MAC address to pend
#define MAX_PENDING_MAC_SIZE    32

// Used to set the list of MAC addresses for SEDs we currently have packets pending
#define OID_OT_PENDING_MAC_OFFLOAD                  OT_DEFINE_OID(108, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_PENDING_MAC_OFFLOAD
    {
        #define OT_PENDING_MAC_OFFLOAD_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        UCHAR              ShortAddressCount;
        UCHAR              ExtendedAddressCount;
        // Dynamic array of USHORT ShortAddresses of count ShortAddressCount
        // Dynamic array of ULONGLONG ExtendedAddresses of count ExtendedAddressCount
    } OT_PENDING_MAC_OFFLOAD, * POT_PENDING_MAC_OFFLOAD;

#define SIZEOF_OT_PENDING_MAC_OFFLOAD_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_PENDING_MAC_OFFLOAD, ExtendedAddressCount)

#define COMPLETE_SIZEOF_OT_PENDING_MAC_OFFLOAD_REVISION_1(ShortAddressCount, ExtendedAddressCount) \
    (SIZEOF_OT_PENDING_MAC_OFFLOAD_REVISION_1 + sizeof(USHORT) * ShortAddressCount + sizeof(ULONGLONG) * ExtendedAddressCount)

// Used to issue an energy scan request for the given channel
#define OID_OT_ENERGY_SCAN                          OT_DEFINE_OID(109, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_ENERGY_SCAN
    {
        #define OT_ENERGY_SCAN_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        UCHAR              Channel;
        USHORT             DurationMs;
    } OT_ENERGY_SCAN, * POT_ENERGY_SCAN;

#define SIZEOF_OT_ENERGY_SCAN_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_ENERGY_SCAN, DurationMs)

//
// Thread Mode OIDs
//

// TODO ...

#endif //__OTOID_H__
