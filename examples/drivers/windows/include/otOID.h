/*
 *  Copyright (c) 2016, Microsoft Corporation.
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
        OT_RADIO_CAP_ACK_TIMEOUT    = 1 << 0,  // Radio supports Ack timeouts internally
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

// Used to query/set promiscuous mode
#define OID_OT_PROMISCUOUS_MODE                     OT_DEFINE_OID(100, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_PROMISCUOUS_MODE
    {
        #define OT_PROMISCUOUS_MODE_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        BOOLEAN            InPromiscuousMode;
    } OT_PROMISCUOUS_MODE, * POT_PROMISCUOUS_MODE;

#define SIZEOF_OT_PROMISCUOUS_MODE_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_PROMISCUOUS_MODE, InPromiscuousMode)

// Used to query/set the Pan ID
#define OID_OT_PAND_ID                              OT_DEFINE_OID(101, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_PAND_ID
    {
        #define OT_PAND_ID_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        USHORT             PanID;
    } OT_PAND_ID, * POT_PAND_ID;

#define SIZEOF_OT_PAND_ID_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_PAND_ID, PanID)

// Used to query/set the Short Address
#define OID_OT_SHORT_ADDRESS                        OT_DEFINE_OID(102, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_SHORT_ADDRESS
    {
        #define OT_SHORT_ADDRESS_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        USHORT             ShortAddress;
    } OT_SHORT_ADDRESS, * POT_SHORT_ADDRESS;

#define SIZEOF_OT_SHORT_ADDRESS_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_SHORT_ADDRESS, ShortAddress)

// Used to query/set the Extended Address
#define OID_OT_EXTENDED_ADDRESS                     OT_DEFINE_OID(103, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_EXTENDED_ADDRESS
    {
        #define OT_EXTENDED_ADDRESS_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        UCHAR              ExtendedAddress[8];
    } OT_EXTENDED_ADDRESS, * POT_EXTENDED_ADDRESS;

#define SIZEOF_OT_EXTENDED_ADDRESS_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_EXTENDED_ADDRESS, ExtendedAddress)

// Used to query/set the current listening channel
#define OID_OT_CURRENT_CHANNEL                      OT_DEFINE_OID(104, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_CURRENT_CHANNEL
    {
        #define OT_CURRENT_CHANNEL_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        UCHAR              Channel;
    } OT_CURRENT_CHANNEL, * POT_CURRENT_CHANNEL;

#define SIZEOF_OT_CURRENT_CHANNEL_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_CURRENT_CHANNEL, Channel)

// Used to query the noise floor (not currently used?)
#define OID_OT_NOISE_FLOOR                          OT_DEFINE_OID(105, OT_OPERATIONAL_OID, OT_MANDATORY_OID)
    typedef struct _OT_NOISE_FLOOR
    {
        #define OT_NOISE_FLOOR_REVISION_1 1
        NDIS_OBJECT_HEADER Header;
        CHAR               NoiseFloor;
    } OT_NOISE_FLOOR, * POT_NOISE_FLOOR;

#define SIZEOF_OT_NOISE_FLOOR_REVISION_1 \
    RTL_SIZEOF_THROUGH_FIELD(OT_NOISE_FLOOR, NoiseFloor)

//
// Thread Mode OIDs
//

// TODO ...

#endif //__OTOID_H__
