/*
 *    Copyright (c) 2016, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This module defines constants that describe physical characteristics and
 *   limits of the underlying hardware.
 */

#pragma once

#ifndef IF_TYPE_IEEE802154
#define IF_TYPE_IEEE802154                  259 // IEEE 802.15.4 WPAN interface
#define NdisPhysicalMediumNative802_15_4    (NDIS_PHYSICAL_MEDIUM)20
#endif

//
// Link layer addressing
// -----------------------------------------------------------------------------
//

// Number of bytes in a hardware address.  802.15.4 uses 8 byte addresses.
#define NIC_MACADDR_SIZE                   8

//
// Frames
// -----------------------------------------------------------------------------
//

#define HW_MAX_FRAME_SIZE                  1280

//
// Medium properties
// -----------------------------------------------------------------------------
//

#define NIC_MEDIUM_TYPE                    NdisMediumIP

// Claim to be 250kbps duplex
#define KILOBITS_PER_SECOND                1000ULL
#define MEGABITS_PER_SECOND                1000000ULL
#define NIC_RECV_XMIT_SPEED                (250ULL*KILOBITS_PER_SECOND)

//
// Hardware limits
// -----------------------------------------------------------------------------
//

// Max number of multicast addresses supported in hardware
#define NIC_MAX_MCAST_LIST                 32

// Maximum number of uncompleted sends that a single adapter will permit
#define NIC_MAX_BUSY_SENDS                 1024

// Maximum number of received packets that can be in the OS at any time
// (Also known as the receive pool size)
#define NIC_MAX_OUTSTANDING_RECEIVES       32


//
// Physical adapter properties
// -----------------------------------------------------------------------------
//

// Change to your company name instead of using OpenThread
#define NIC_VENDOR_DESC                    "OpenThread"

// Highest byte is the NIC byte plus three vendor bytes. This is normally
// obtained from the NIC.
#define NIC_VENDOR_ID                      0x00FFFFFF

#ifdef OTTMP_LEGACY
#define NIC_SUPPORTED_FILTERS ( \
                NDIS_PACKET_TYPE_DIRECTED    | \
                NDIS_PACKET_TYPE_MULTICAST   | \
                NDIS_PACKET_TYPE_BROADCAST   | \
                NDIS_PACKET_TYPE_PROMISCUOUS | \
                NDIS_PACKET_TYPE_ALL_MULTICAST)
#else
#define NIC_SUPPORTED_FILTERS (NET_PACKET_FILTER_TYPES_FLAGS) ( \
                NET_PACKET_FILTER_TYPE_DIRECTED   | \
                NET_PACKET_FILTER_TYPE_MULTICAST  | \
                NET_PACKET_FILTER_TYPE_BROADCAST  | \
                NET_PACKET_FILTER_TYPE_PROMISCUOUS | \
                NET_PACKET_FILTER_TYPE_ALL_MULTICAST)
#endif

//
// Specify a bitmask that defines optional properties of the NIC.
// This miniport indicates receive with NdisMIndicateReceiveNetBufferLists
// function.  Such a driver should set this NDIS_MAC_OPTION_TRANSFERS_NOT_PEND
// flag.
//
// NDIS_MAC_OPTION_NO_LOOPBACK tells NDIS that NIC has no internal
// loopback support so NDIS will manage loopbacks on behalf of
// this driver.
//
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA tells the protocol that
// our receive buffer is not on a device-specific card. If
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA is not set, multi-buffer
// indications are copied to a single flat buffer.
//
#define NIC_MAC_OPTIONS (\
                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
                NDIS_MAC_OPTION_NO_LOOPBACK         | \
                NDIS_MAC_OPTION_8021P_PRIORITY      | \
                NDIS_MAC_OPTION_8021Q_VLAN)

// NDIS 6.x miniports must support all counters in OID_GEN_STATISTICS.
#define NIC_SUPPORTED_STATISTICS (\
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV              | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS           | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR              | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS          | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV     | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_XMIT)
