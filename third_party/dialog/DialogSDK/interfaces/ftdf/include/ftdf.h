/**
 \addtogroup INTERFACES
 \{
 \addtogroup FTDF
 \{
 \brief IEEE 802.15.4 Wireless
 */
/**
 ****************************************************************************************
 *
 * @file ftdf.h
 *
 * @brief FTDF UMAC API header file
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */

#ifndef FTDF_H_
#define FTDF_H_

#include <stdint.h>
#ifdef FTDF_PHY_API
#include <ftdf_config_phy_api.h>
#else
#include <ftdf_config_mac_api.h>
#endif

#define __private1                            request
#define __private2                            requestASN
#define __private3                            requestSN

#define FTDF_CAPABILITY_IS_FFD                0x02
#define FTDF_CAPABILITY_AC_POWER              0x04
#define FTDF_CAPABILITY_RECEIVER_ON_WHEN_IDLE 0x08
#define FTDF_CAPABILITY_FAST_ASSOCIATION      0x10
#define FTDF_CAPABILITY_SUPPORTS_SECURITY     0x40
#define FTDF_CAPABILITY_WANTS_SHORT_ADDRESS   0x80

#define FTDF_TRANSPARENT_USE_WAIT_FOR_ACK       1

// Transparency options
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_0      0x00000001
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_1      0x00000002
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_2      0x00000004
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_3      0x00000008
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_4      0x00000010
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_5      0x00000020
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_6      0x00000040
#define FTDF_TRANSPARENT_PASS_FRAME_TYPE_7      0x00000080
#define FTDF_TRANSPARENT_PASS_ALL_FRAME_TYPES   0x000000ff
#define FTDF_TRANSPARENT_AUTO_ACK               0x00000100
#define FTDF_TRANSPARENT_AUTO_FP_ACK            0x00000200
#define FTDF_TRANSPARENT_PASS_CRC_ERROR         0x00000400
#define FTDF_TRANSPARENT_PASS_ALL_FRAME_VERSION 0x00001000
#define FTDF_TRANSPARENT_PASS_ALL_PAN_ID        0x00002000
#define FTDF_TRANSPARENT_PASS_ALL_ADDR          0x00004000
#define FTDF_TRANSPARENT_PASS_ALL_BEACON        0x00008000
#define FTDF_TRANSPARENT_PASS_ALL_NO_DEST_ADDR  0x00010000
#define FTDF_TRANSPARENT_ENABLE_CSMA_CA         0x01000000
#define FTDF_TRANSPARENT_ENABLE_FCS_GENERATION  0x02000000
#define FTDF_TRANSPARENT_ENABLE_SECURITY        0x04000000
#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
#define FTDF_TRANSPARENT_WAIT_FOR_ACK           0x08000000
#endif

// Transparent TX statuses
#define FTDF_TRANSPARENT_SEND_SUCCESSFUL 0x00000000
#define FTDF_TRANSPARENT_CSMACA_FAILURE  0x00000001
#define FTDF_TRANSPARENT_OVERFLOW        0x00000002
#if FTDF_TRANSPARENT_WAIT_FOR_ACK
#define FTDF_TRANSPARENT_NO_ACK          0x00000004
#endif

// Transparent RX statuses
#define FTDF_TRANSPARENT_RCV_SUCCESSFUL         0x00000000
#define FTDF_TRANSPARENT_RCV_CRC_ERROR          0x00000001
#define FTDF_TRANSPARENT_RCV_SECURITY_ERROR     0x00000002
#define FTDF_TRANSPARENT_RCV_RES_FRAMETYPE      0x00000004
#define FTDF_TRANSPARENT_RCV_RES_FRAME_VERSION  0x00000008
#define FTDF_TRANSPARENT_RCV_UNEXP_DST_ADDR     0x00000010
#define FTDF_TRANSPARENT_RCV_UNEXP_DST_PAN_ID   0x00000020
#define FTDF_TRANSPARENT_RCV_UNEXP_BEACON       0x00000040
#define FTDF_TRANSPARENT_RCV_UNEXP_NO_DEST_ADDR 0x00000080

typedef uint8_t ftdf_msg_id_t;
#define FTDF_DATA_REQUEST               1
#define FTDF_DATA_INDICATION            2
#define FTDF_DATA_CONFIRM               3
#define FTDF_PURGE_REQUEST              4
#define FTDF_PURGE_CONFIRM              5
#define FTDF_ASSOCIATE_REQUEST          6
#define FTDF_ASSOCIATE_INDICATION       7
#define FTDF_ASSOCIATE_RESPONSE         8
#define FTDF_ASSOCIATE_CONFIRM          9
#define FTDF_DISASSOCIATE_REQUEST       10
#define FTDF_DISASSOCIATE_INDICATION    11
#define FTDF_DISASSOCIATE_CONFIRM       12
#define FTDF_BEACON_NOTIFY_INDICATION   13
#define FTDF_COMM_STATUS_INDICATION     14
#define FTDF_GET_REQUEST                15
#define FTDF_GET_CONFIRM                16
#define FTDF_SET_REQUEST                17
#define FTDF_SET_CONFIRM                18
#define FTDF_GTS_REQUEST                19
#define FTDF_GTS_CONFIRM                20
#define FTDF_GTS_INDICATION             21
#define FTDF_ORPHAN_INDICATION          22
#define FTDF_ORPHAN_RESPONSE            23
#define FTDF_RESET_REQUEST              24
#define FTDF_RESET_CONFIRM              25
#define FTDF_RX_ENABLE_REQUEST          26
#define FTDF_RX_ENABLE_CONFIRM          27
#define FTDF_SCAN_REQUEST               28
#define FTDF_SCAN_CONFIRM               29
#define FTDF_START_REQUEST              30
#define FTDF_START_CONFIRM              31
#define FTDF_SYNC_REQUEST               32
#define FTDF_SYNC_LOSS_INDICATION       33
#define FTDF_POLL_REQUEST               34
#define FTDF_POLL_CONFIRM               35
#define FTDF_SET_SLOTFRAME_REQUEST      36
#define FTDF_SET_SLOTFRAME_CONFIRM      37
#define FTDF_SET_LINK_REQUEST           38
#define FTDF_SET_LINK_CONFIRM           39
#define FTDF_TSCH_MODE_REQUEST          40
#define FTDF_TSCH_MODE_CONFIRM          41
#define FTDF_KEEP_ALIVE_REQUEST         42
#define FTDF_KEEP_ALIVE_CONFIRM         43
#define FTDF_BEACON_REQUEST             44
#define FTDF_BEACON_CONFIRM             45
#define FTDF_BEACON_REQUEST_INDICATION  46
#define FTDF_TRANSPARENT_CONFIRM        47
#define FTDF_TRANSPARENT_INDICATION     48
#define FTDF_TRANSPARENT_REQUEST        49
#define FTDF_TRANSPARENT_ENABLE_REQUEST 50
#define FTDF_SLEEP_REQUEST              51
#define FTDF_EXPLICIT_WAKE_UP           52
#define FTDF_REMOTE_REQUEST             53
#if FTDF_DBG_BUS_ENABLE
#define FTDF_DBG_MODE_SET_REQUEST       54
#endif /* FTDF_DBG_BUS_ENABLE */
#define FTDF_FPPR_MODE_SET_REQUEST      55

/**
 * \brief  Request status
 * \remark Valid statuses:
 *         - FTDF_SUCCESS: Successful
 *         - FTDF_CHANNEL_ACCESS_FAILURE: CCA failure
 *         - FTDF_NO_ACK: No ack received after macMaxFrameRetries
 *         - FTDF_NO_DATA: No data frames pending
 *         - FTDF_COUNTER_ERROR: Security frame counter error
 *         - FTDF_FRAME_TOO_LONG: Too long frame
 *         - FTDF_IMPROPER_KEY_TYPE: Improper key type
 *         - FTDF_IMPROPER_SECURITY_LEVEL: Improper security level
 *         - FTDF_SECURITY_ERROR: Security authentication error
 *         - FTDF_UNAVAILABLE_KEY: Unavailable key
 *         - FTDF_UNAVAILABLE_DEVICE: Unavailable device
 *         - FTDF_UNAVAILABLE_SECURITY_LEVEL: Unavailable security level
 *         - FTDF_UNSUPPORTED_LEGACY: Unsupported legacy
 *         - FTDF_UNSUPPORTED_SECURITY: Unsupporteds security
 *         - FTDF_INVALID_PARAMETER: Invalid parameter
 *         - FTDF_TRANSACTION_OVERFLOW: Transaction overflow
 *         - FTDF_TRANSACTION_EXPIRED: Transaction expired
 *         - FTDF_NO_BEACON: No beacon received
 *         - FTDF_SCAN_IN_PROGRESS: Scan in progress
 *         - FTDF_NO_SHORT_ADDRESS: FTDF does not have a short address
 *         - FTDF_SLOTFRAME_NOT_FOUND: Slotframe not found
 *         - FTDF_MAX_SLOTFRAMES_EXCEEDED: Max slotframes exceeded
 *         - FTDF_UNKNOWN_LINK: Unknown link
 *         - FTDF_MAX_LINKS_EXCEEDED: Max links exceeded
 *         - FTDF_UNSUPPORTED_ATTRIBUTE: Unsupported PIB attribute
 *         - FTDF_READ_ONLY: Read onli PIB attribute
 *         - FTDF_INVALID_HANDLE: Invalid handle
 *         - FTDF_PAN_AT_CAPACITY: PAN at capacity
 *         - FTDF_PAN_ACCESS_DENIED: PAN access denied
 *         - FTDF_HOPPING_SEQUENCE_OFFSET_DUPLICATION: Hopping sequence offset duplication
 */
typedef uint8_t ftdf_status_t;
#define FTDF_SUCCESS                             0
#define FTDF_CHANNEL_ACCESS_FAILURE              1
#define FTDF_NO_ACK                              2
#define FTDF_NO_DATA                             3
#define FTDF_COUNTER_ERROR                       4
#define FTDF_FRAME_TOO_LONG                      5
#define FTDF_IMPROPER_KEY_TYPE                   6
#define FTDF_IMPROPER_SECURITY_LEVEL             7
#define FTDF_SECURITY_ERROR                      8
#define FTDF_UNAVAILABLE_KEY                     9
#define FTDF_UNAVAILABLE_DEVICE                  10
#define FTDF_UNAVAILABLE_SECURITY_LEVEL          11
#define FTDF_UNSUPPORTED_LEGACY                  12
#define FTDF_UNSUPPORTED_SECURITY                13
#define FTDF_INVALID_PARAMETER                   14
#define FTDF_TRANSACTION_OVERFLOW                15
#define FTDF_TRANSACTION_EXPIRED                 16
#define FTDF_ON_TIME_TOO_LONG                    17
#define FTDF_LIMIT_REACHED                       18
#define FTDF_NO_BEACON                           19
#define FTDF_SCAN_IN_PROGRESS                    20
#define FTDF_INVALID_INDEX                       21
#define FTDF_NO_SHORT_ADDRESS                    22
#define FTDF_SUPERFRAME_OVERLAP                  23
#define FTDF_TRACKING_OFF                        24
#define FTDF_SLOTFRAME_NOT_FOUND                 25
#define FTDF_MAX_SLOTFRAMES_EXCEEDED             26
#define FTDF_UNKNOWN_LINK                        27
#define FTDF_MAX_LINKS_EXCEEDED                  28
#define FTDF_UNSUPPORTED_ATTRIBUTE               29
#define FTDF_READ_ONLY                           30
#define FTDF_INVALID_HANDLE                      31
#define FTDF_PAN_AT_CAPACITY                     32
#define FTDF_PAN_ACCESS_DENIED                   33
#define FTDF_HOPPING_SEQUENCE_OFFSET_DUPLICATION 34

/**
 * \brief Association status
 * \remark Supported statuses:
 *         - FTDF_ASSOCIATION_SUCCESSFUL: Associate request granted
 *         - FTDF_ASSOCIATION_PAN_AT_CAPACITY: Asscociate denied, PAN at capacity
 *         - FTDF_ASSOCIATION_PAN_ACCESS_DENIED: Associate denied
 *         - FTDF_FAST_ASSOCIATION_SUCCESSFUL: Associate request granted (fast)
 */
typedef uint8_t ftdf_association_status_t;
#define FTDF_ASSOCIATION_SUCCESSFUL                          0x00
#define FTDF_ASSOCIATION_PAN_AT_CAPACITY                     0x01
#define FTDF_ASSOCIATION_PAN_ACCESS_DENIED                   0x02
#define FTDF_ASSOCIATION_HOPPING_SEQUENCE_OFFSET_DUPLICATION 0x03
#define FTDF_FAST_ASSOCIATION_SUCCESSFUL                     0x80

/**
 * \brief Address mode
 * \remark Supported address modes:
 *         - FTDF_NO_ADDRESS
 *         - FTDF_SHORT_ADDRESS
 *         - FTDF_EXTENDED_ADDRESS
 */
typedef uint8_t ftdf_address_mode_t;
#define FTDF_NO_ADDRESS       0
#define FTDF_SIMPLE_ADDRESS   1
#define FTDF_SHORT_ADDRESS    2
#define FTDF_EXTENDED_ADDRESS 3

/**
 * \brief  PAN id
 * \remark Range 0.. 65535
 */
typedef uint16_t ftdf_pan_id_t;

typedef uint8_t  ftdf_simple_address_t;

/**
 * \brief  Short address
 * \remark Range 00:00 - ff:ff
 */
typedef uint16_t ftdf_short_address_t;

/**
 * \brief  Extended address
 * \remark Range 00:00:00:00:00:00:00:00 - ff:ff:ff:ff:ff:ff:ff:ff
 */
typedef uint64_t ftdf_ext_address_t;

typedef union {
        /** \brief DO NOT USE */
        ftdf_simple_address_t simple_address;
        /** \brief Short address */
        ftdf_short_address_t  short_address;
        /** \brief Extended address */
        ftdf_ext_address_t    ext_address;
} ftdf_address_t;

#ifdef SIMULATOR
typedef uint16_t ftdf_data_length_t;
#else
typedef uint8_t  ftdf_data_length_t;
#endif

/**
 * \brief  Handle
 * \remark Range 0..255
 */
typedef uint8_t ftdf_handle_t;

/**
 * \brief  Boolean
 * \remark Valid values:
 *         - FTDF_FALSE
 *         - FTDF_TRUE
 */
typedef uint8_t ftdf_boolean_t;

/** \brief FTDF boolean FALSE */
#define FTDF_FALSE 0
/** \brief FTDF boolean TRUE */
#define FTDF_TRUE  1

/**
 * \brief  Security level
 * \remark Valid range 0..7
 */
typedef uint8_t  ftdf_security_level_t;

/**
 * \brief  Security key id mode
 * \remark Valid range 0..3
 */
typedef uint8_t  ftdf_key_id_mode_t;

/**
 * \brief  Security key index
 * \remark Valid range 0..255
 */
typedef uint8_t  ftdf_key_index_t;

/**
 * \brief  Sequence number
 * \remark Valid range 0..255
 */
typedef uint8_t  ftdf_sn_t;

/**
 * \brief  Sequence number
 * \remark Valid range 0..0xffffffffff (5 octets)
 */
typedef uint64_t ftdf_asn_t;

/**
 * \brief  Order
 */
typedef uint8_t  ftdf_order_t;

/**
 * \brief  Time
 * \remark Timestamp in symbols which wraps around every 0x100000000 symbols
 */
typedef uint32_t ftdf_time_t;

/**
 * \brief  Real time
 * \remark Timestamp in symbols which does not wrap (in the lifetime of the device)
 */
typedef uint64_t ftdf_time64_t;

typedef uint32_t ftdf_usec_t;

typedef uint64_t ftdf_psec_t;

typedef uint32_t ftdf_nr_low_power_clock_cycles_t;

typedef uint8_t ftdf_nr_backoff_periods_t;

/**
 * \brief  Link Quality mode
 * \remark 0: RSSI, 1: LQI
 */
typedef uint8_t ftdf_link_quality_mode_t;
#define FTDF_LINK_QUALITY_MODE_RSSI 0
#define FTDF_LINK_QUALITY_MODE_LQI  1

/**
 * \brief  Period of time
 * \remark Range 0..65535
 */
typedef uint16_t ftdf_period_t;

/**
 * \brief  Size type
 */
typedef uint8_t  ftdf_size_t;

/**
 * \brief  Length type
 */
typedef uint8_t  ftdf_length_t;

/**
 * \brief  Priority type
 */
typedef uint8_t  ftdf_priority_t;

/**
 * \brief  Performance counter
 * \remark Range 0..0xffffffff
 */
typedef uint32_t ftdf_count_t;

/**
 * \brief Disassociate reason
 * \remark Allowed values:
 *         - FTDF_COORD_WISH_DEVICE_LEAVE_PAN: Coordinator wishes that device leaves PAN
 *         - FTDF_DEVICE_WISH_LEAVE_PAN: Device wishes to leave PAN
 */
typedef uint8_t  ftdf_disassociate_reason_t;
#define FTDF_COORD_WISH_DEVICE_LEAVE_PAN 1
#define FTDF_DEVICE_WISH_LEAVE_PAN       2

/**
 * \brief Beacon type
 * \remark Allowed values:
 *         - FTDF_NORMAL_BEACON: Normal beacon
 *         - FTDF_ENHANCED_BEACON: Enhanced beacon
 */
typedef uint8_t ftdf_beacon_type_t;
#define FTDF_NORMAL_BEACON   0
#define FTDF_ENHANCED_BEACON 1

// MAC constants
/** \brief aBaseSlotDuration */
#define FTDF_BASE_SLOT_DURATION          60
/** \brief aNumSuperframeSlots */
#define FTDF_NUM_SUPERFRAME_SLOTS        16
/** \brief aBaseSuperframeDuration */
#define FTDF_BASE_SUPERFRAME_DURATION    (FTDF_BASE_SLOT_DURATION * FTDF_NUM_SUPERFRAME_SLOTS)
/** \brief NOT USED */
#define FTDF_GTS_PERSISTENCE_TIME        4
/** \brief aMaxBeaconOverhead */
#define FTDF_MAX_BEACON_OVERHEAD         75
/** \brief aMaxBeaconOverhead */
#define FTDF_MAX_PHY_PACKET_SIZE         127
/** \brief aMaxPHYPacketSize */
#define FTDF_MAX_BEACON_PAYLOAD_LENGTH   (FTDF_MAX_PHY_PACKET_SIZE - FTDF_MAX_BEACON_OVERHEAD)
/** \brief NOT USED  */
#define FTDF_MAX_LOST_BEACONS            4
/** \brief aMaxMPDUUnsecuredOverhead */
#define FTDF_MAX_MPDU_UNSECURED_OVERHEAD 25
/** \brief aMinMPDUOverhead */
#define FTDF_MIN_MPDU_OVERHEAD           9
/** \brief aMaxMACSafePayloadSize */
#define FTDF_MAX_MAC_SAFE_PAYLOAD_SIZE   (FTDF_MAX_PHY_PACKET_SIZE - FTDF_MAX_MPDU_UNSECURED_OVERHEAD)
/** \brief aMaxMACSafePayloadSize */
#define FTDF_MAX_PAYLOAD_SIZE            (FTDF_MAX_PHY_PACKET_SIZE - FTDF_MIN_MPDU_OVERHEAD)
/** \brief aMaxMACPayloadSize */
#define FTDF_MAX_SIFS_FRAME_SIZE         18
/** \brief NOT USED  */
#define FTDF_MIN_CAP_LENGTH              440
/** \brief aUnitBackoffPeriod */
#define FTDF_UNIT_BACKOFF_PERIOD         20

/**
 * \brief PIB attribute id
 * \remark List of supported PIB attributes, their types/structures and description
 * - FTDF_PIB_EXTENDED_ADDRESS, \link ftdf_ext_address_t \endlink,
 *         The extended address of the device
 * - FTDF_PIB_ACK_WAIT_DURATION, \link ftdf_period_t \endlink, The maximum time in symbols that is
     waited for an ack, Read only
 * - FTDF_PIB_ASSOCIATION_PAN_COORD, \link ftdf_boolean_t \endlink, Indication whether the device
     is associated with a PAN coordinator
 * - FTDF_PIB_ASSOCIATION_PERMIT, \link  ftdf_boolean_t \endlink, Indication whether the PAN
     coordinator grats association requests
 * - FTDF_PIB_AUTO_REQUEST, \link ftdf_boolean_t \endlink, Indication whether beacon received while
     scanning are forwarded to the application or not
 * - FTDF_PIB_BEACON_PAYLOAD, ftdf_octet_t*, Pointer to the data to be included in a beacon as
     payload
 * - FTDF_PIB_BEACON_PAYLOAD_LENGTH, \link ftdf_size_t \endlink, Size of the data to be included
     in a beacon as payload
 * - FTDF_PIB_BEACON_ORDER, \link ftdf_order_t \endlink, Fixed to 15 indicating beaconless mode
 * - FTDF_PIB_BSN, \link ftdf_sn_t \endlink, The current beacon sequence number
 * - FTDF_PIB_COORD_EXTENDED_ADDRESS, \link ftdf_ext_address_t \endlink, The extended address of
     the coordinator
 * - FTDF_PIB_COORD_SHORT_ADDRESS, \link ftdf_short_address_t \endlink, The short address of the
     coordinator
 * - FTDF_PIB_DSN, \link ftdf_sn_t \endlink, The current data sequence number
 * - FTDF_PIB_MAX_BE, \link ftdf_be_exponent_t \endlink, The maximum backoff exponent
 * - FTDF_PIB_MAX_CSMA_BACKOFFS, \link ftdf_size_t \endlink, The maximum number of backoffs
 * - FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME, \link ftdf_period_t \endlink, The maximum time in symbols
     that RX is ON after that a frame is received with FP set.
 * - FTDF_PIB_MAX_FRAME_RETRIES, \link ftdf_size_t \endlink, The maximum number of frame retries
 * - FTDF_PIB_MIN_BE, \link ftdf_be_exponent_t \endlink, The minimum backoff exponent
 * - FTDF_PIB_LIFS_PERIOD, \link ftdf_period_t \endlink, The long IFS period in number of symbols,
     Read oonly
 * - FTDF_PIB_SIFS_PERIOD, \link ftdf_period_t \endlink, The short IFS period in number of symbols,
     Read oonly
 * - FTDF_PIB_PAN_ID, \link ftdf_pan_id_t \endlink, The PAN id of the device
 * - FTDF_PIB_PROMISCUOUS_MODE, \link ftdf_boolean_t \endlink, Indication whether the promiscuous
     is enable or not
 * - FTDF_PIB_RESPONSE_WAIT_TIME, \link ftdf_size_t \endlink, The maximum time in
     aBaseSuperFramePeriod's (960 symbols) that is waited for a command frame response
 * - FTDF_PIB_RX_ON_WHEN_IDLE, \link ftdf_boolean_t \endlink, Indication whether the receiver
     must be on when idle
 * - FTDF_PIB_SECURITY_ENABLED, \link ftdf_boolean_t \endlink, Indication whether the security
     is enabled
 * - FTDF_PIB_SHORT_ADDRESS, \link ftdf_short_address_t \endlink, The short address of the device
 * - FTDF_PIB_SYNC_SYMBOL_OFFSET, \link ftdf_period_t \endlink, The offset in symbols between the
     start of frame and that the timestamp is taken
 * - FTDF_PIB_TIMESTAMP_SUPPORTED, \link ftdf_boolean_t \endlink, Indication whether
     timestamping is supported
 * - FTDF_PIB_TRANSACTION_PERSISTENCE_TIME, \link ftdf_period_t \endlink, The maximum time in
     aBaseSuperFramePeriod's (960 symbols) that indirect data requests are queued
 * - FTDF_PIB_ENH_ACK_WAIT_DURATION, \link ftdf_period_t \endlink, The maximum time in symbols
     that is waited for an enhanced ack
 * - FTDF_PIB_IMPLICIT_BROADCAST, \link ftdf_boolean_t \endlink, Indication whether frames without
     a destination PAN are treated as broadcasts
 * - FTDF_PIB_DISCONNECT_TIME, \link ftdf_period_t \endlink, The time in timeslots that disassoctate
     frames are send before disconnecting
 * - FTDF_PIB_JOIN_PRIORITY, \link ftdf_priority_t \endlink, The join priority
 * - FTDF_PIB_ASN, \link ftdf_asn_t \endlink, The current ASN
 * - FTDF_PIB_SLOTFRAME_TABLE, \link ftdf_slotframe_table_t \endlink, The slotframe table, Read only
 * - FTDF_PIB_LINK_TABLE, \link FTDF_LinkTable \endlink, The link table, Read only
 * - FTDF_PIB_TIMESLOT_TEMPLATE, \link ftdf_timeslot_template_t \endlink, The current timeslot
     template
 * - FTDF_PIB_HOPPINGSEQUENCE_ID, \link ftdf_hopping_sequence_id_t \endlink, The id of the current
     hopping sequence
 * - FTDF_PIB_CHANNEL_PAGE, \link ftdf_channel_page_t \endlink, The current channel page
 * - FTDF_PIB_HOPPING_SEQUENCE_LENGTH, \link ftdf_length_t \endlink, Length of the current hopping
     sequence
 * - FTDF_PIB_HOPPING_SEQUENCE_LIST, \link ftdf_channel_number_t \endlink, Hopping sequence
 * - FTDF_PIB_CURRENT_HOP, \link ftdf_length_t \endlink, The current hop
 * - FTDF_PIB_CSL_PERIOD, \link ftdf_period_t \endlink, The CSL sample period in units of 10 symbols
 * - FTDF_PIB_CSL_MAX_PERIOD, \link ftdf_period_t \endlink, The maximum CSL sample period of devices
     in the PAN in units of 10 symbols
 * - FTDF_PIB_CSL_CHANNEL_MASK, \link ftdf_bitmap32_t \endlink, Bitmapped list of channels to be
     sample in CSL mode
 * - FTDF_PIB_CSL_FRAME_PENDING_WAIT_T, \link ftdf_period_t \endlink, The maximum time in symbols
     that RX is ON after that a frame is received with FP set in CSL mode.
 * - FTDF_PIB_PERFORMANCE_METRICS, \link ftdf_performance_metrics_t \endlink,
     The performance metrics, Read only
 * - FTDF_PIB_EB_IE_LIST, \link ftdf_ie_list_t \endlink, Payload IE list to be added to an
     enhanced beacon
 * - FTDF_PIB_EBSN, \link ftdf_sn_t \endlink, Current enhanced beacon sequence number
 * - FTDF_PIB_EB_AUTO_SA, \link ftdf_auto_sa_t \endlink, Source address mode of auto generated
     enhanced beacons
 * - FTDF_PIB_EACK_IE_LIST, \link ftdf_ie_list_t \endlink, Payload IE list to be added
     to an enhanced ack
 * - FTDF_PIB_KEY_TABLE, \link ftdf_key_table_t \endlink, Security key table
 * - FTDF_PIB_DEVICE_TABLE, \link ftdf_device_table_t \endlink, Security device table
 * - FTDF_PIB_SECURITY_LEVEL_TABLE, \link ftdf_security_level_table_t \endlink, Security level table
 * - FTDF_PIB_FRAME_COUNTER, \link ftdf_frame_counter_t \endlink, Current security frame counter
 * - FTDF_PIB_MT_DATA_SECURITY_LEVEL, \link ftdf_security_level_t \endlink, Security level for
     auto generated data frames
 * - FTDF_PIB_MT_DATA_KEY_ID_MODE, \link ftdf_key_id_mode_t \endlink, Security key id mode for
     auto generated data frames
 * - FTDF_PIB_MT_DATA_KEY_SOURCE, ftdf_octet_t[8], Security key source for auto generated
     data frames
 * - FTDF_PIB_MT_DATA_KEY_INDEX, \link ftdf_key_index_t \endlink, Security key index for auto
     generated data frames
 * - FTDF_PIB_DEFAULT_KEY_SOURCE, ftdf_octet_t[8], Default key source
 * - FTDF_PIB_FRAME_COUNTER_MODE, \link ftdf_frame_counter_mode_t \endlink, Security frame counter
     mode
 * - FTDF_PIB_CSL_SYNC_TX_MARGIN, \link ftdf_period_t \endlink, The margin in unit of 10 symbols
     used by FTDF in CSL mode in case of a synchronised transmission
 * - FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO, \link ftdf_period_t \endlink, The time in unit of 10 symbols
     after which FTDF discard the remote synchronisation info.
 * - FTDF_PIB_TSCH_ENABLED, \link ftdf_boolean_t \endlink, Indicates whether the TSCH mode is
     enabled or not, Read only
 * - FTDF_PIB_LE_ENABLED, \link ftdf_boolean_t \endlink, Indicates whether the CSL mode is
     enabled or not
 * - FTDF_PIB_CURRENT_CHANNEL, \link ftdf_channel_number_t \endlink, The current channel used
     by FTDF
 * - FTDF_PIB_CHANNELS_SUPPORTED, \link ftdf_channel_descriptor_list_t \endlink, List of channels
     supported by FTDF
 * - FTDF_PIB_TX_POWER_TOLERANCE, \link ftdf_tx_power_tolerance_t \endlink, TX power tolerance
 * - FTDF_PIB_TX_POWER, \link ftdf_dbm \endlink, TX power
 * - FTDF_PIB_CCA_MODE, \link ftdf_cca_mode_t \endlink, CCA mode
 * - FTDF_PIB_CURRENT_PAGE, \link ftdf_channel_page_t \endlink, Current channel page
 * - FTDF_PIB_MAX_FRAME_DURATION, \link ftdf_period_t \endlink, The maximum number of symbols in
     frame, Read only
 * - FTDF_PIB_SHR_DURATION, \link ftdf_period_t \endlink, Synchronisation header length in
     symbols, Read only
 * - FTDF_PIB_TRAFFIC_COUNTERS, \link ftdf_traffic_counters_t \endlink, Miscelaneous
     traffic counters, Read only
 * - FTDF_PIB_LE_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports LE (CSL),
     Read only
 * - FTDF_PIB_LL_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports LL,
     Read only
 * - FTDF_PIB_DSME_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports DSME,
     Read only
 * - FTDF_PIB_RFID_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports RFID,
     Read only
 * - FTDF_PIB_AMCA_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports AMCA,
     Read only
 * - FTDF_PIB_TSCH_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports FTDF,
     Read only
 * - FTDF_PIB_METRICS_CAPABLE, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports
     metrics, Read only
 * - FTDF_PIB_RANGING_SUPPORTED, \link ftdf_boolean_t \endlink, Indicates whether FTDF supports
     ranging, Read only
 * - FTDF_PIB_KEEP_PHY_ENABLED, \link ftdf_boolean_t \endlink, Indicates whether the PHY must
     be kept enabled
 * - FTDF_PIB_METRICS_ENABLED, \link ftdf_boolean_t \endlink, Indicates whether metrics
     are enabled or not
 * - FTDF_PIB_BEACON_AUTO_RESPOND, \link ftdf_boolean_t \endlink, Indicates whether FTDF must
     send automatically a beacon at a BECAON_REQUEST command frame or must forward the request
     to the application using FTDF_BEACON_REQUEST_INDICATION.
 * - FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD, \link ftdf_period_t \endlink, The minimum TSCH slot sync
     offset in microseconds before resyncing
 */
typedef uint8_t ftdf_pib_attribute_t;
// See Table 52 "MAC PIB attributes" of IEEE 802.15.4-2011 and IEEE 802.15.4e-2012 for more details

// NOTE: Be careful with changing the order of these PIB attributes because initialization of related
//       registers will be done in the order defined here.
#define FTDF_PIB_EXTENDED_ADDRESS                    1
#define FTDF_PIB_ACK_WAIT_DURATION                   2
#define FTDF_PIB_ASSOCIATION_PAN_COORD               3
#define FTDF_PIB_ASSOCIATION_PERMIT                  4
#define FTDF_PIB_AUTO_REQUEST                        5
#define FTDF_PIB_BATT_LIFE_EXT                       6
#define FTDF_PIB_BATT_LIFE_EXT_PERIODS               7
#define FTDF_PIB_BEACON_PAYLOAD                      8
#define FTDF_PIB_BEACON_PAYLOAD_LENGTH               9
#define FTDF_PIB_BEACON_ORDER                        10
#define FTDF_PIB_BEACON_TX_TIME                      11
#define FTDF_PIB_BSN                                 12
#define FTDF_PIB_COORD_EXTENDED_ADDRESS              13
#define FTDF_PIB_COORD_SHORT_ADDRESS                 14
#define FTDF_PIB_DSN                                 15
#define FTDF_PIB_GTS_PERMIT                          16
#define FTDF_PIB_MAX_BE                              17
#define FTDF_PIB_MAX_CSMA_BACKOFFS                   18
#define FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME           19
#define FTDF_PIB_MAX_FRAME_RETRIES                   20
#define FTDF_PIB_MIN_BE                              21
#define FTDF_PIB_LIFS_PERIOD                         22
#define FTDF_PIB_SIFS_PERIOD                         23
#define FTDF_PIB_PAN_ID                              24
#define FTDF_PIB_PROMISCUOUS_MODE                    25
#define FTDF_PIB_RESPONSE_WAIT_TIME                  26
#define FTDF_PIB_RX_ON_WHEN_IDLE                     27
#define FTDF_PIB_SECURITY_ENABLED                    28
#define FTDF_PIB_SHORT_ADDRESS                       29
#define FTDF_PIB_SUPERFRAME_ORDER                    30
#define FTDF_PIB_SYNC_SYMBOL_OFFSET                  31
#define FTDF_PIB_TIMESTAMP_SUPPORTED                 32
#define FTDF_PIB_TRANSACTION_PERSISTENCE_TIME        33
#define FTDF_PIB_TX_CONTROL_ACTIVE_DURATION          34
#define FTDF_PIB_TX_CONTROL_PAUSE_DURATION           35
#define FTDF_PIB_ENH_ACK_WAIT_DURATION               36
#define FTDF_PIB_IMPLICIT_BROADCAST                  37
#define FTDF_PIB_SIMPLE_ADDRESS                      38
#define FTDF_PIB_DISCONNECT_TIME                     39
#define FTDF_PIB_JOIN_PRIORITY                       40
#define FTDF_PIB_ASN                                 41
#define FTDF_PIB_NO_HL_BUFFERS                       42
#define FTDF_PIB_SLOTFRAME_TABLE                     43
#define FTDF_PIB_LINK_TABLE                          44
#define FTDF_PIB_TIMESLOT_TEMPLATE                   45
#define FTDF_PIB_HOPPINGSEQUENCE_ID                  46
#define FTDF_PIB_CHANNEL_PAGE                        47
#define FTDF_PIB_NUMBER_OF_CHANNELS                  48
#define FTDF_PIB_PHY_CONFIGURATION                   49
#define FTDF_PIB_EXTENTED_BITMAP                     50
#define FTDF_PIB_HOPPING_SEQUENCE_LENGTH             51
#define FTDF_PIB_HOPPING_SEQUENCE_LIST               52
#define FTDF_PIB_CURRENT_HOP                         53
#define FTDF_PIB_DWELL_TIME                          54
#define FTDF_PIB_CSL_PERIOD                          55
#define FTDF_PIB_CSL_MAX_PERIOD                      56
#define FTDF_PIB_CSL_CHANNEL_MASK                    57
#define FTDF_PIB_CSL_FRAME_PENDING_WAIT_T            58
#define FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED     59
#define FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL 60
#define FTDF_PIB_PERFORMANCE_METRICS                 61
#define FTDF_PIB_USE_ENHANCED_BEACON                 62
#define FTDF_PIB_EB_IE_LIST                          63
#define FTDF_PIB_EB_FILTERING_ENABLED                64
#define FTDF_PIB_EBSN                                65
#define FTDF_PIB_EB_AUTO_SA                          66
#define FTDF_PIB_EACK_IE_LIST                        67
#define FTDF_PIB_KEY_TABLE                           68
#define FTDF_PIB_DEVICE_TABLE                        69
#define FTDF_PIB_SECURITY_LEVEL_TABLE                70
#define FTDF_PIB_FRAME_COUNTER                       71
#define FTDF_PIB_MT_DATA_SECURITY_LEVEL              72
#define FTDF_PIB_MT_DATA_KEY_ID_MODE                 73
#define FTDF_PIB_MT_DATA_KEY_SOURCE                  74
#define FTDF_PIB_MT_DATA_KEY_INDEX                   75
#define FTDF_PIB_DEFAULT_KEY_SOURCE                  76
#define FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS          77
#define FTDF_PIB_PAN_COORD_SHORT_ADDRESS             78
#define FTDF_PIB_FRAME_COUNTER_MODE                  79
#define FTDF_PIB_CSL_SYNC_TX_MARGIN                  80
#define FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO             81
#define FTDF_PIB_TSCH_ENABLED                        82
#define FTDF_PIB_LE_ENABLED                          83

// See Table 71 "PHY PIB attributes" of IEEE 802.15.4-2011 for more details
#define FTDF_PIB_CURRENT_CHANNEL           84
#define FTDF_PIB_CHANNELS_SUPPORTED        85
#define FTDF_PIB_TX_POWER_TOLERANCE        86
#define FTDF_PIB_TX_POWER                  87
#define FTDF_PIB_CCA_MODE                  88
#define FTDF_PIB_CURRENT_PAGE              89
#define FTDF_PIB_MAX_FRAME_DURATION        90
#define FTDF_PIB_SHR_DURATION              91

#define FTDF_PIB_TRAFFIC_COUNTERS          92
#define FTDF_PIB_LE_CAPABLE                93
#define FTDF_PIB_LL_CAPABLE                94
#define FTDF_PIB_DSME_CAPABLE              95
#define FTDF_PIB_RFID_CAPABLE              96
#define FTDF_PIB_AMCA_CAPABLE              97
#define FTDF_PIB_METRICS_CAPABLE           98
#define FTDF_PIB_RANGING_SUPPORTED         99
#define FTDF_PIB_KEEP_PHY_ENABLED          100
#define FTDF_PIB_METRICS_ENABLED           101
#define FTDF_PIB_BEACON_AUTO_RESPOND       102
#define FTDF_PIB_TSCH_CAPABLE              103
#define FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD 104

/* Proprietary PIB. */
#define FTDF_PIB_BO_IRQ_THRESHOLD          105
#define FTDF_PIB_PTI_CONFIG                106
#define FTDF_PIB_LINK_QUALITY_MODE         107

// Total number of PIB attributes
#define FTDF_NR_OF_PIB_ATTRIBUTES          107

/* Default values */
#ifndef FTDF_BO_IRQ_THRESHOLD
#define FTDF_BO_IRQ_THRESHOLD               0xff
#endif

#if FTDF_DBG_BUS_ENABLE
/**
 * \brief Debug bus mode.
 *
 */
typedef uint8_t ftdf_dbg_mode_t;

/**
 * \brief Disable debug signals
 */
#define FTDF_DBG_DISABLE                        0x00

/**
 * \brief Legacy debug mode
 *              diagnose bus bit 7 = ed_request
 *              diagnose bus bit 6 = gen_irq
 *              diagnose bus bit 5 = tx_en
 *              diagnose bus bit 4 = rx_en
 *              diagnose bus bit 3 = phy_en
 *              diagnose bus bit 2 = tx_valid
 *              diagnose bus bit 1 = rx_sof
 *              diagnose bus bit 0 = rx_eof
 */
#define FTDF_DBG_LEGACY                         0x01

/**
 * \brief Clocks and reset
 *              diagnose bus bit 7-3 = "00000"
 *              diagnose bus bit 2   = lp_clk divide by 2
 *              diagnose bus bit 1   = mac_clk divide by 2
 *              diagnose bus bit 0   = hclk divided by 2
 */
#define FTDF_DBG_CLK_RESET                      0x02

/**
 * \brief Rx phy signals
 *              diagnose_bus bit 7   = rx_enable towards the rx_pipeline
 *              diagnose bus bit 6   = rx_sof from phy
 *              diagnose bus bit 5-2 = rx_data from phy
 *              diagnose bus bit 1   = rx_eof to phy
 *              diagnose bus bit 0   = rx_sof event from rx_mac_timing block
 */
#define FTDF_DBG_RX_PHY_SIGNALS                 0x10

/**
 * \brief Rx sof and length
 *              diagnose_bus bit 7   = rx_sof
 *              diagnose bus bit 6-0 = rx_mac_length from rx_mac_timing block
 */
#define FTDF_DBG_RX_SOF_AND_LENGTH              0x11

/**
 * \brief Rx mac timing output
 *              diagnose_bus bit 7   = Enable from rx_mac_timing block
 *              diagnose bus bit 6   = Sop    from rx_mac_timing block
 *              diagnose bus bit 5   = Eop    from rx_mac_timing block
 *              diagnose bus bit 4   = Crc    from rx_mac_timing block
 *              diagnose bus bit 3   = Err    from rx_mac_timing block
 *              diagnose bus bit 2   = Drop   from rx_mac_timing block
 *              diagnose bus bit 1   = Forward from rx_mac_timing block
 *              diagnose bus bit 0   = rx_sof
 */
#define FTDF_DBG_RX_MAC_TIMING_OUTPUT           0x12

/**
 * \brief Rx mac frame parser output
 *              diagnose_bus bit 7   = Enable from mac_frame_parser block
 *              diagnose bus bit 6   = Sop    from  mac_frame_parser block
 *              diagnose bus bit 5   = Eop    from  mac_frame_parser block
 *              diagnose bus bit 4   = Crc    from  mac_frame_parser block
 *              diagnose bus bit 3   = Err    from  mac_frame_parser block
 *              diagnose bus bit 2   = Drop   from  mac_frame_parser block
 *              diagnose bus bit 1   = Forward from mac_frame_parser block
 *              diagnose bus bit 0   = rx_sof
 */
#define FTDF_DBG_RX_MAC_FRAME_PARSER_OUTPUT     0x13

/**
 * \brief Rx status to Umac
 *              diagnose_bus bit 7   = rx_frame_stat_umac.crc16_error
 *              diagnose bus bit 6   = rx_frame_stat_umac.res_frm_type_error
 *              diagnose bus bit 5   = rx_frame_stat_umac.res_frm_version_error
 *              diagnose bus bit 4   = rx_frame_stat_umac.dpanid_error
 *              diagnose bus bit 3   = rx_frame_stat_umac.daddr_error
 *              diagnose bus bit 2   = rx_frame_stat_umac.spanid_error
 *              diagnose bus bit 1   = rx_frame_stat_umac.ispan_coord_error
 *              diagnose bus bit 0   = rx_sof
 */
#define FTDF_DBG_RX_STATUS_TO_UMAC              0x14

/**
 * \brief Rx status to lmac
 *              diagnose_bus bit 7   = rx_frame_stat.packet_valid
 *              diagnose bus bit 6   = rx_frame_stat.rx_sof_e
 *              diagnose bus bit 5   = rx_frame_stat.rx_eof_e
 *              diagnose bus bit 4   = rx_frame_stat.wakeup_frame
 *              diagnose bus bit 3-1 = rx_frame_stat.frame_type
 *              diagnose bus bit 0   = rx_frame_stat.rx_sqnr_valid
 */
#define FTDF_DBG_RX_STATUS_TO_LMAC              0x15

/**
 * \brief Rx buffer control
 *              diagnose_bus bit 7-4 = prov_from_swio.rx_read_buf_ptr
 *              diagnose bus bit 3-0 = stat_to_swio.rx_write_buf_ptr
 */
#define FTDF_DBG_RX_BUFFER_CONTROL              0x16

/**
 * \brief Rx interrupts
 *              diagnose_bus bit 7   = stat_to_swio.rx_buf_avail_e
 *              diagnose bus bit 6   = stat_to_swio.rx_buf_full
 *              diagnose bus bit 5   = stat_to_swio.rx_buf_overflow_e
 *              diagnose bus bit 4   = stat_to_swio.rx_sof_e
 *              diagnose bus bit 3   = stat_to_swio.rxbyte_e
 *              diagnose bus bit 2   = rx_frame_ctrl.rx_enable
 *              diagnose bus bit 1   = rx_eof
 *              diagnose bus bit 0   = rx_sof
 */
#define FTDF_DBG_RX_INTERRUPTS                  0x17

/**
 * \brief MAC frame parser events
 *              diagnose_bus bit 7-3 =
 *                      Prt statements mac frame parser
 *                      0 Reset
 *                      1 Multipurpose frame with no destination PAN id and macImplicitBroadcast is
 *                        false and not PAN coordinator dropped
 *                      2 Multipurpose frame with no destination address and macImplicitBroadcast is
 *                        false and not PAN coordinator dropped
 *                      3 RX unsupported frame type detected
 *                      4 RX non Beacon frame detected and dropped in RxBeaconOnly mode
 *                      5 RX frame passed due to macAlwaysPassFrmType set
 *                      6 RX unsupported da_mode = 01 for frame_version 0x0- detected
 *                      7 RX unsupported sa_mode = 01 for frame_version 0x0- detected
 *                      8 Data or command frame with no destination PAN id and macImplicitBroadcast
 *                        is false and not PAN coordinator dropped
 *                      9 Data or command frame with no destination address and macImplicitBroadcast
 *                        is false and not PAN coordinator dropped
 *                      10 RX unsupported frame_version detected
 *                      11 Multipurpose frame with no destination PAN id and macImplicitBroadcast is
 *                         false and not PAN coordinator dropped
 *                      12 Multipurpose frame with no destination address and macImplicitBroadcast
 *                         is false and not PAN coordinator dropped
 *                      13 RX unsupported frame_version detected
 *                      14 RX Destination PAN Id check failed
 *                      15 RX Destination address (1 byte) check failed
 *                      16 RX Destination address (2 byte) check failed
 *                      17 RX Destination address (8 byte) check failed
 *                      18 RX Source PAN Id check for Beacon frames failed
 *                      19 RX Source PAN Id check failed
 *                      20 Auxiliary Security Header security control word received
 *                      21 Auxiliary Security Header unsupported_legacy received frame_version 00
 *                      22 Auxiliary Security Header unsupported_legacy security frame_version 11
 *                      23 Auxiliary Security Header frame counter field received
 *                      24 Auxiliary Security Header Key identifier field received
 *                      25 Errored dDestination Wakeup frame found send to UMAC, not indicated
 *                         towards LMAC controller
 *                      26 MacAlwaysPassWakeUpFrames = 1, Wakeup frame found send to UMAC, not
 *                         indicated towards LMAC controller
 *                      27 Wakeup frame found send to LMAC controller and UMAC
 *                      28 Wakeup frame found send to LMAC controller, dropped towards UMAC
 *                      29 Command frame with data_request command received
 *                      30 Coordinator realignment frame received
 *                      31 Not Used
 *              diagnose bus bit 2-0 = frame_type
 */
#define FTDF_DBG_RX_MAC_FRAME_PARSER_EVENTS     0x18

/**
 * \brief MAC frame parser states
 *              diagnose_bus bit 7-4 =
 *                      Framestates mac frame parser
 *                      0 idle_state
 *                      1 fr_ctrl_state
 *                      2 fr_ctrl_long_state
 *                      3 seq_nr_state
 *                      4 dest_pan_id_state
 *                      5 dest_addr_state
 *                      6 src_pan_id_state
 *                      7 src_addr_state
 *                      8 aux_sec_hdr_state
 *                      9 hdr_ie_state
 *                      10 pyld_ie_state
 *                      11 ignore_state
 *                      12-16  Not used
 *              diagnose bus bit 3   = rx_mac_sof_e
 *              diagnose bus bit 2-0 = frame_type
 */
#define FTDF_DBG_RX_MAC_FRAME_PARSER_STATES     0x19

/**
 * \brief Top and bottom interfaces tx datapath
 *              diagnose_bus bit 7   = tx_mac_ld_strb
 *              diagnose bus bit 6   = tx_mac_drdy
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 */
#define FTDF_DBG_TX_TOP_BOTTOM_INTERFACES       0x20

/**
 * \brief Control signals for tx datapath
 *              diagnose_bus bit 7   = TxTransparentMode
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.CRC16_ena
 *              diagnose bus bit 5   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 3   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 2-1 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose bus bit 0   = tx_valid
 */
#define FTDF_DBG_TX_CONTROL_SIGNALS             0x21

/**
 * \brief Control signals for wakeup frames
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5-1 = ctrl_lmac_to_tx.gen_wu_RZ(4 DOWNTO 0)
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 */
#define FTDF_DBG_TX_WAKEUP_CONTROL_SIGNALS      0x22

/**
 * \brief Data signals for wakeup frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 */
#define FTDF_DBG_TX_WAKUP_DATA_SIGNALS          0x23

/**
 * \brief Data and control ack frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 */
#define FTDF_DBG_TX_ACK_                        0x24


/**
 * \brief PHY signals
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6   = tx_en
 *              diagnose bus bit 5   = rx_en
 *              diagnose bus bit 4-1 = phyattr(3 downto 0)
 *              diagnose bus bit 0   = ed_request
 */
#define FTDF_DBG_LMAC_PHY_SIGNALS               0x30

/**
 * \brief PHY enable and detailed state machine info
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6-0 = prt statements detailed state machine info
 *                      0 Reset
 *                      1 MAIN_STATE_IDLE TX_ACKnowledgement frame skip CSMA_CA
 *                      2 MAIN_STATE_IDLE : Wake-Up frame resides in buffer no
 *                      3 MAIN_STATE_IDLE : TX frame in TSCH mode
 *                      4 MAIN_STATE_IDLE : TX frame
 *                      5 MAIN_STATE_IDLE : goto SINGLE_CCA
 *                      6 MAIN_STATE_IDLE : goto Energy Detection
 *                      7 MAIN_STATE_IDLE : goto RX
 *                      8 MAIN_STATE_IDLE de-assert phy_en wait for
 *                      9 MAIN_STATE_IDLE Start phy_en and assert phy_en wait for :
 *                      10 MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA framenr :
 *                      11 MAIN_STATE_CSMA_CA framenr :
 *                      12 MAIN_STATE_CSMA_CA start CSMA_CA
 *                      14 MAIN_STATE_CSMA_CA skip CSMA_CA
 *                      15 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed for CSL mode
 *                      16 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed
 *                      17 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Success
 *                      18 MAIN_STATE_SINGLE_CCA CCA failed
 *                      19 MAIN_STATE_SINGLE_CCA CCA ok
 *                      20 MAIN_STATE_WAIT_ACK_DELAY wait time elapsed
 *                      21 MAIN_STATE_WAIT ACK_DELAY : Send Ack in TSCH mode
 *                      22 MAIN_STATE_WAIT ACK_DELAY : Ack not scheduled in time in TSCH mode
 *                      23 MAIN_STATE_TSCH_TX_FRAME macTSRxTx time elapsed
 *                      24 MAIN_STATE_TX_FRAME it is CSL mode, start the WU seq for macWUPeriod
 *                      25 MAIN_STATE_TX_FRAME1
 *                      26 MAIN_STATE_TX_FRAME_W1 exit, waited for :
 *                      27 MAIN_STATE_TX_FRAME_W2 exit, waited for :
 *                      28 MAIN_STATE_TX_WAIT_LAST_SYMBOL exit
 *                      29 MAIN_STATE_TX_RAMPDOWN_W exit, waited for :
 *                      30 MAIN_STATE_TX_PHYTRXWAIT exit, waited for :
 *                      31 MAIN_STATE_GEN_IFS , Ack with frame pending received
 *                      32 MAIN_STATE_GEN_IFS , Ack requested but DisRXackReceivedca is set
 *                      33 MAIN_STATE_GEN_IFS , instead of generating an IFS, switch the rx-on for
 *                         an ACK
 *                      34 MAIN_STATE_GEN_IFS , generating an SIFS (Short Inter Frame Space)
 *                      35 MAIN_STATE_GEN_IFS , generating an LIFS (Long Inter Frame Space)
 *                      36 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'cslperiod_timer_ready' is ready
 *                         sent the data frame
 *                      37 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'Rendezvous zero time not yet
 *                         transmitted sent another WU frame
 *                      38 MAIN_STATE_GEN_IFS_FINISH exit, corrected for :
 *                      39 MAIN_STATE_GEN_IFS_FINISH in CSL mode, rx_resume_after_tx is set
 *                      40 MAIN_STATE_SENT_STATUS CSLMode wu_sequence_done_flag :
 *                      41 MAIN_STATE_ADD_DELAY exit, goto RX for frame pending time
 *                      42 MAIN_STATE_ADD_DELAY exit, goto IDLE
 *                      43 MAIN_STATE_RX switch the RX from off to on
 *                      44 MAIN_STATE_RX_WAIT_PHYRXSTART waited for
 *                      45 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the MacAckDuration timer
 *                      46 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the ack wait timer
 *                      47 MAIN_STATE_RX_WAIT_PHYRXLATENCY NORMAL mode : start rx_duration
 *                      48 MAIN_STATE_RX_WAIT_PHYRXLATENCY TSCH mode : start macTSRxWait
 *                         (macTSRxWait) timer
 *                      49 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration
 *                         (macCSLsamplePeriod) timer
 *                      50 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration
 *                         (macCSLdataPeriod) timer
 *                      51 MAIN_STATE_RX_CHK_EXPIRED 1 ackwait_timer_ready or ack_received_ind
 *                      52 MAIN_STATE_RX_CHK_EXPIRED 2 normal_mode. ACK with FP bit set, start frame
 *                         pending timer
 *                      54 MAIN_STATE_RX_CHK_EXPIRED 4 RxAlwaysOn is set to 0
 *                      55 MAIN_STATE_RX_CHK_EXPIRED 5 TSCH mode valid frame rxed
 *                      56 MAIN_STATE_RX_CHK_EXPIRED 6 TSCH mode macTSRxwait expired
 *                      57 MAIN_STATE_RX_CHK_EXPIRED 7 macCSLsamplePeriod timer ready
 *                      58 MAIN_STATE_RX_CHK_EXPIRED 8 csl_mode. Data frame recieved with FP bit
 *                         set, increment RX duration time
 *                      59 MAIN_STATE_RX_CHK_EXPIRED 9 CSL mode : Received wu frame in data
 *                         listening period restart macCSLdataPeriod timer
 *                      60 MAIN_STATE_RX_CHK_EXPIRED 10 csl_rx_duration_ready or csl data frame
 *                         received:
 *                      61 MAIN_STATE_RX_CHK_EXPIRED 11 normal_mode. FP bit is set, start frame
 *                         pending timer
 *                      62 MAIN_STATE_RX_CHK_EXPIRED 12 normal mode rx_duration_timer_ready and
 *                         frame_pending_timer_ready
 *                      63 MAIN_STATE_RX_CHK_EXPIRED 13 acknowledge frame requested
 *                      64 MAIN_STATE_RX_CHK_EXPIRED 14 Interrupt to sent a frame. found_tx_packet
 *                      65 MAIN_STATE_CSL_RX_IDLE_END RZ time is far away, switch receiver Off and
 *                         wait for csl_rx_rz_time before switching on
 *                      66 MAIN_STATE_CSL_RX_IDLE_END RZ time is not so far away, keep receiver On
 *                         and wait for
 *                      67 MAIN_STATE_RX_START_PHYRXSTOP ack with fp received keep rx on
 *                      68 MAIN_STATE_RX_START_PHYRXSTOP switch the RX from on to off
 *                      69 MAIN_STATE_RX_WAIT_PHYRXSTOP acknowledge done : switch back to finish TX
 *                      70 MAIN_STATE_RX_WAIT_PHYRXSTOP switch the RX off
 *                      71 MAIN_STATE_ED switch the RX from off to on
 *                      72 MAIN_STATE_ED_WAIT_PHYRXSTART waited for phyRxStartup
 *                      73 MAIN_STATE_ED_WAIT_EDSCANDURATION waited for EdScanDuration
 *                      74 MAIN_STATE_ED_WAIT_PHYRXSTOP end of Energy Detection, got IDLE
 *                         75 - 127 Not used
 */
#define FTDF_DBG_LMAC_ENABLE_AND_SM             0x31

/**
 * \brief RX enable and detailed state machine info
 *              diagnose_bus bit 7   = rx_en
 *              diagnose bus bit 6-0 = detailed state machine info see FTDF_DBG_LMAC_ENABLE_AND_SM
 */
#define FTDF_DBG_LMAC_RX_ENABLE_AND_SM          0x32

/**
 * \brief TX enable and detailed state machine info
 *              diagnose_bus bit 7   = tx_en
 *              diagnose bus bit 6-0 = detailed state machine info see FTDF_DBG_LMAC_ENABLE_AND_SM
 */
#define FTDF_DBG_LMAC_TX_ENABLE_AND_SM          0x33

/**
 * \brief PHY enable, TX enable and Rx enable and state machine states
 *              diagnose_bus bit 7   = phy_en
 *              diagnose_bus bit 6   = tx_en
 *              diagnose_bus bit 5   = rx_en
 *              diagnose bus bit 4-0 = State machine states
 *                       1  MAIN_STATE_IDLE
 *                       2  MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA
 *                       3  MAIN_STATE_CSMA_CA
 *                       4  MAIN_STATE_WAIT_FOR_CSMA_CA_0
 *                       5  MAIN_STATE_WAIT_FOR_CSMA_CA
 *                       6  MAIN_STATE_SINGLE_CCA
 *                       7  MAIN_STATE_WAIT_ACK_DELAY
 *                       8  MAIN_STATE_TSCH_TX_FRAME
 *                       9  MAIN_STATE_TX_FRAME
 *                       10 MAIN_STATE_TX_FRAME1
 *                       11 MAIN_STATE_TX_FRAME_W1
 *                       12 MAIN_STATE_TX_FRAME_W2
 *                       13 MAIN_STATE_TX_WAIT_LAST_SYMBOL
 *                       14 MAIN_STATE_TX_RAMPDOWN
 *                       15 MAIN_STATE_TX_RAMPDOWN_W
 *                       16 MAIN_STATE_TX_PHYTRXWAIT
 *                       17 MAIN_STATE_GEN_IFS
 *                       18 MAIN_STATE_GEN_IFS_FINISH
 *                       19 MAIN_STATE_SENT_STATUS
 *                       20 MAIN_STATE_ADD_DELAY
 *                       21 MAIN_STATE_TSCH_RX_ACKDELAY
 *                       22 MAIN_STATE_RX
 *                       23 MAIN_STATE_RX_WAIT_PHYRXSTART
 *                       24 MAIN_STATE_RX_WAIT_PHYRXLATENCY
 *                       25 MAIN_STATE_RX_CHK_EXPIRED
 *                       26 MAIN_STATE_CSL_RX_IDLE_END
 *                       27 MAIN_STATE_RX_START_PHYRXSTOP
 *                       28 MAIN_STATE_RX_WAIT_PHYRXSTOP
 *                       29 MAIN_STATE_ED
 *                       30 MAIN_STATE_ED_WAIT_PHYRXSTART
 *                       31 MAIN_STATE_ED_WAIT_EDSCANDURATION
 */
#define FTDF_DBG_LMAC_RX_TX_ENABLE_AND_STATES   0x34

/**
 * \brief LMAC controller to tx interface with wakeup
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen_wu
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 */
#define FTDF_DBG_LMAC_TO_TX_AND_WAKEUP          0x35

/**
 * \brief LMAC controller to tx interface with frame pending
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen.fp_bit
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 */
#define FTDF_DBG_LMAC_TO_TX_AND_FRAME_PENDING   0x36

/**
 * \brief Rx pipepline to LMAC controller interface
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_rx_to_lmac.rx_sof_e
 *              diagnose_bus bit 4   = ctrl_rx_to_lmac.rx_eof_e
 *              diagnose_bus bit 3   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 2   = ack_received_ind
 *              diagnose_bus bit 1   = ack_request_ind
 *              diagnose_bus bit 0   = ctrl_rx_to_lmac.data_request_frame
 */
#define FTDF_DBG_LMAC_RX_TO_LMAC                0x37

/**
 * \brief CSL start
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = csl_start
 *              diagnose_bus bit 4-1 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 0   = found_wu_packet;
 */
#define FTDF_DBG_LMAC_CSL_START                 0x38

/**
 * \brief TSCH TX ACK timing
 *              diagnose_bus bit 7-4 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 3   = tx_en
 *              diagnose_bus bit 2   = rx_en
 *              diagnose_bus bit 1   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 0   = one_us_timer_ready
 */
#define FTDF_DBG_LMAC_TSCH_ACK_TIMING           0x39


/**
 * \brief Security control
 *              diagnose_bus bit 7   = swio_to_security.secStart
 *              diagnose_bus bit 6   = swio_to_security.secTxRxn
 *              diagnose_bus bit 5   = swio_to_security.secAbort
 *              diagnose_bus bit 4   = swio_to_security.secEncDecn
 *              diagnose_bus bit 3   = swio_from_security.secBusy
 *              diagnose_bus bit 2   = swio_from_security.secAuthFail
 *              diagnose_bus bit 1   = swio_from_security.secReady_e
 *              diagnose_bus bit 0   = '0'
 */
#define FTDF_DBG_SEC_CONTROL                    0x40

/**
 * \brief Security CTLID access Rx and Tx
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 2   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 1   = rx_sec_ci.selectp
 *              diagnose_bus bit 0   = rx_sec_co.d_rdy
 */
#define FTDF_DBG_SEC_CTLID_ACCESS_RX_TX         0x41

/**
 * \brief Security ctlid access tx with sec_wr_data_valid
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 */
#define FTDF_DBG_SEC_CTLID_ACCESS_TX_AND_SEC_WR_DATA_VALID      0x42

/**
 * \brief Security ctlid access rx with sec_wr_data_valid
 *              diagnose_bus bit 7   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = rx_sec_ci.selectp
 *              diagnose_bus bit 4   = rx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 */
#define FTDF_DBG_SEC_CTLID_ACCESS_RX_AND_SEC_WR_DATA_VALID     0x43

/**
 * \brief AHB bus without hsize
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3   = hready_in
 *              diagnose_bus bit 2   = hready_out
 *              diagnose_bus bit 1-0 = hresp
 */
#define FTDF_DBG_AHB_WO_HSIZE                   0x50

/**
 * \brief AHB bus without hwready_in and hresp(1)
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3-2 = hhsize(1 downto 0)
 *              diagnose_bus bit 1   = hready_out
 *              diagnose_bus bit 0   = hresp(0)
 */
#define FTDF_DBG_AHB_WO_HW_READY_HRESP          0x51

/**
 * \brief AMBA Tx signals
 *              diagnose_bus bit 7   = ai_tx.penable
 *              diagnose_bus bit 6   = ai_tx.psel
 *              diagnose_bus bit 5   = ai_tx.pwrite
 *              diagnose_bus bit 4   = ao_tx.pready
 *              diagnose_bus bit 3   = ao_tx.pslverr
 *              diagnose_bus bit 2-1 = psize_tx
 *              diagnose_bus bit 0   = hready_out
 */
#define FTDF_DBG_AHB_AMBA_TX_SIGNALS            0x52

/**
 * \brief AMBA Rx signals
 *              diagnose_bus bit 7   = ai_rx.penable
 *              diagnose_bus bit 6   = ai_rx.psel
 *              diagnose_bus bit 5   = ai_rx.pwrite
 *              diagnose_bus bit 4   = ao_rx.pready
 *              diagnose_bus bit 3   = ao_rx.pslverr
 *              diagnose_bus bit 2-1 = psize_rx
 *              diagnose_bus bit 0   = hready_out
 */
#define FTDF_DBG_AHB_AMBA_RX_SIGNALS            0x53

/**
 * \brief AMBA register map signals
 *              diagnose_bus bit 7   = ai_reg.penable
 *              diagnose_bus bit 6   = ai_reg.psel
 *              diagnose_bus bit 5   = ai_reg.pwrite
 *              diagnose_bus bit 4   = ao_reg.pready
 *              diagnose_bus bit 3   = ao_reg.pslverr
 *              diagnose_bus bit 2-1 = "00"
 *              diagnose_bus bit 0   = hready_out
 */
#define FTDF_DBG_AHB_AMBA_REG_MAP_SIGNALS       0x54

#endif /* FTDF_DBG_BUS_ENABLE */

#define FTDF_MAX_HOPPING_SEQUENCE_LENGTH 16

typedef void     ftdf_pib_attribute_value_t;

/**
 * \brief  Link quality
 * \remark Range: 0..255
 * \remark A higher number is a higher quality
 */
typedef uint8_t  ftdf_link_quality_t;

/**
 * \brief Hopping sequence id
 */
typedef uint16_t ftdf_hopping_sequence_id_t;

/**
 * \brief  Channel number
 * \remark Supported range: 11..27
 */
typedef uint8_t  ftdf_channel_number_t;

/**
 * \brief  Packet Traffic Information (PTI) used to classify a transaction in the radio arbiter.
 */
typedef uint8_t  ftdf_pti_t;

enum {
        /* PTI index for explicit Rx. */
        FTDF_PTI_CONFIG_RX,
        /* PTI index for Tx. */
        FTDF_PTI_CONFIG_TX,
        FTDF_PTI_CONFIG_RESERVED0,
        FTDF_PTI_CONFIG_RESERVED1,
        FTDF_PTI_CONFIG_RESERVED2,
        FTDF_PTI_CONFIG_RESERVED3,
        FTDF_PTI_CONFIG_RESERVED4,
        FTDF_PTI_CONFIG_RESERVED5,
        /* Enumeration end. */
        FTDF_PTIS,
};

typedef struct {
        ftdf_pti_t ptis[FTDF_PTIS];
} ftdf_pti_config_t;

/**
 * \brief  Channel page
 * \remark Supported values: 0
 */
typedef uint8_t  ftdf_channel_page_t;

typedef uint8_t  ftdf_channel_offset_t;

typedef uint8_t  ftdf_octet_t;

/**
 * \brief 8-bit type bitmap
 */
typedef uint8_t  ftdf_bitmap8_t;

/**
 * \brief 16-bit type bitmap
 */
typedef uint16_t ftdf_bitmap16_t;

/**
 * \brief 32-bit type bitmap
 */
typedef uint32_t ftdf_bitmap32_t;

typedef uint8_t  ftdf_num_of_backoffs_t;

typedef uint8_t  ftdf_gts_charateristics_t;

/**
 * \brief Scan type
 * \remark Allowed values:
 *         - FTDF_ED_SCAN: Energy detect scan
 *         - FTDF_ACTIVE_SCAN: Active scan
 *         - FTDF_PASSIVE_SCAN: Passive scan
 *         - FTDF_ORPHAN_SCAN: Orphan scan
 *         - FTDF_ENHANCED_ACTIVE_SCAN: Enhanced active scan
 */
typedef uint8_t  ftdf_scan_type_t;
#define FTDF_ED_SCAN              1
#define FTDF_ACTIVE_SCAN          2
#define FTDF_PASSIVE_SCAN         3
#define FTDF_ORPHAN_SCAN          4
#define FTDF_ENHANCED_ACTIVE_SCAN 7

typedef uint8_t ftdf_scan_duration_t;

/**
 * \brief Loss operation
 * \remark Possible values:
 *         - FTDF_PAN_ID_CONFLICT
 *         - FTDF_REALIGNMENT
 *         - FTDF_BEACON_LOST (not supported because only beaconless mode is supported)
 */
typedef uint8_t ftdf_loss_reason_t;
#define FTDF_PAN_ID_CONFLICT 1
#define FTDF_REALIGNMENT     2
#define FTDF_BEACON_LOST     3

typedef uint16_t ftdf_slotframe_size_t;

/**
 * \brief Set operation
 * \remark Allowed values:
 *         - FTDF_ADD: Add an entry
 *         - FTDF_DELETE: Delete an entry
 *         - FTDF_MODIFY: Modify an entry
 */
typedef uint8_t  ftdf_operation_t;
#define FTDF_ADD    0
#define FTDF_DELETE 2
#define FTDF_MODIFY 3

typedef uint16_t ftdf_timeslot_t;

/**
 * \brief Set operation
 * \remark Allowed values:
 *         - FTDF_NORMAL_LINK
 *         - FTDF_ADVERTISING_LINK
 */
typedef uint8_t  ftdf_link_type_t;
#define FTDF_NORMAL_LINK      0
#define FTDF_ADVERTISING_LINK 1

/**
 * \brief TSCH mode status
 * \remark Allowed values:
 *         - FTDF_TSCH_OFF
 *         - FTDF_TSCH_ON
 */
typedef uint8_t ftdf_tsch_mode_t;
#define FTDF_TSCH_OFF 0
#define FTDF_TSCH_ON  1

typedef uint16_t ftdf_keep_alive_period_t;

/**
 * \brief BE exponent
 */
typedef uint8_t  ftdf_be_exponent_t;

#define FTDF_MAX_SLOTFRAMES 4

typedef struct {
        /** \brief Handle of this slotframe */
        ftdf_handle_t         slotframe_handle;
        /** \brief The number of timeslots in this slotframe */
        ftdf_slotframe_size_t slotframe_size;
} ftdf_slotframe_entry_t;

typedef struct {
        /** \brief Number of slotframes in the slotframe table */
        ftdf_size_t             nr_of_slotframes;
        /** \brief Pointer to the first slotframe entry */
        ftdf_slotframe_entry_t* slotframe_entries;
} ftdf_slotframe_table_t;

#define FTDF_MAX_LINKS                16

#define FTDF_LINK_OPTION_TRANSMIT     0x01
#define FTDF_LINK_OPTION_RECEIVE      0x02
#define FTDF_LINK_OPTION_SHARED       0x04
#define FTDF_LINK_OPTION_TIME_KEEPING 0x08

typedef struct {
        /** \brief Handle of this link, used for delete and modify operations */
        ftdf_handle_t         link_handle;
        /**
         * \brief Bit mapped link options, the following options are supported:<br>
         *        FTDF_LINK_OPTION_TRANSMIT: Lik can be used for transmitting<br>
         *        FTDF_LINK_OPTION_RECEIVE: Link can be used for receiving<br>
         *        FTDF_LINK_OPTION_SHARED: Shared link, multiple device can transmit in this link<br>
         *        FTDF_LINK_OPTION_TIME_KEEPING: Link will be used to synchronise timeslots<br>
         *        Multiple options can be defined by or-ing them.
         */
        ftdf_bitmap8_t        link_options;
        /** \brief Link type */
        ftdf_link_type_t      link_type;
        /** \brief Handle of the slotframe of this link */
        ftdf_handle_t         slotframe_handle;
        /** \brief Short address of the peer node, only valid if is a transmit link */
        ftdf_short_address_t  node_address;
        /** \brief Timeslot in the slotframe */
        ftdf_timeslot_t       timeslot;
        /** \brief Channel offset */
        ftdf_channel_offset_t channel_offset;
        /** \brief DO NOT USE */
        void*                 __private1;
        /** \brief DO NOT USE */
        uint64_t              __private2;
} ftdf_link_entry_t;

typedef struct {
        /** \brief Number of links in the link table */
        ftdf_size_t        nr_of_links;
        /** \brief Pointer to the first link entry */
        ftdf_link_entry_t* link_entries;
} FTDF_LinkTable;

typedef struct {
        ftdf_handle_t timeslot_template_id;
        ftdf_period_t ts_cca_offset;
        ftdf_period_t ts_cca;
        ftdf_period_t ts_tx_offset;
        ftdf_period_t ts_rx_offset;
        ftdf_period_t ts_rx_ack_delay;
        ftdf_period_t ts_tx_ack_delay;
        ftdf_period_t ts_rx_wait;
        ftdf_period_t ts_ack_wait;
        ftdf_period_t ts_rx_tx;
        ftdf_period_t ts_max_ack;
        ftdf_period_t ts_max_ts;
        ftdf_period_t ts_timeslot_length;
} ftdf_timeslot_template_t;

typedef struct {
        ftdf_size_t  counter_octets;
        ftdf_count_t retry_count;
        ftdf_count_t multiple_retry_count;
        ftdf_count_t tx_fail_count;
        ftdf_count_t tx_success_count;
        ftdf_count_t fcs_error_count;
        ftdf_count_t security_failure_count;
        ftdf_count_t duplicate_frame_count;
        ftdf_count_t rx_success_count;
        ftdf_count_t nack_count;
        ftdf_count_t rx_expired_count;
        ftdf_count_t bo_irq_count;
} ftdf_performance_metrics_t;

typedef struct {
        ftdf_count_t tx_data_frm_cnt;
        ftdf_count_t tx_cmd_frm_cnt;
        ftdf_count_t tx_std_ack_frm_cnt;
        ftdf_count_t tx_enh_ack_frm_cnt;
        ftdf_count_t tx_ceacon_frm_cnt;
        ftdf_count_t tx_multi_purp_frm_cnt;
        ftdf_count_t rx_data_frm_ok_cnt;
        ftdf_count_t rx_cmd_frm_ok_cnt;
        ftdf_count_t rx_std_ack_frm_ok_cnt;
        ftdf_count_t rx_enh_ack_frm_ok_cnt;
        ftdf_count_t rx_beacon_frm_ok_cnt;
        ftdf_count_t rx_multi_purp_frm_ok_cnt;
} ftdf_traffic_counters_t;

typedef uint8_t ftdf_energy_t;

typedef struct {
        /** \brief Coordinator address mode */
        ftdf_address_mode_t   coord_addr_mode;
        /** \brief Coordinator PAN id */
        ftdf_pan_id_t         coord_pan_id;
        /** \brief Coordinator address */
        ftdf_address_t        coord_addr;
        /** \brief Channel number */
        ftdf_channel_number_t channel_number;
        /** \brief Channnel page */
        ftdf_channel_page_t   channel_page;
        /** \brief NOT USED */
        ftdf_bitmap16_t       superframe_spec;
        /** \brief NOT USED */
        ftdf_boolean_t        gts_permit;
        /** \brief Link quality */
        ftdf_link_quality_t   link_quality;
        /** \brief */
        ftdf_time_t           timestamp;
} ftdf_pan_descriptor_t;

typedef struct {
        /** \brief Coordinator PAN id */
        ftdf_pan_id_t         coord_pan_id;
        /** \brief Coordinator short address */
        ftdf_short_address_t  coord_short_addr;
        /** \brief Channel number */
        ftdf_channel_number_t channel_number;
        /** \brief Device short address */
        ftdf_short_address_t  short_addr;
        /** \brief Channnel page */
        ftdf_channel_page_t   channel_page;
} ftdf_coord_realign_descriptor_t;

/**
 * \brief  Auto frame source address mode
 * \remark Valid values:
 * - FTDF_AUTO_NONE: No address
 * - FTDF_AUTO_SHORT: Short address
 * - FTDF_AUTO_FULL: Extended address
 */
typedef uint8_t ftdf_auto_sa_t;
#define FTDF_AUTO_NONE  1
#define FTDF_AUTO_SHORT 2
#define FTDF_AUTO_FULL  3

/**
 * \brief  TX Power tolerance
 * \remark Valid values:
 * - FTDF_POWER_TOLERANCE_1_DB
 * - FTDF_POWER_TOLERANCE_3_DB
 * - FTDF_POWER_TOLERANCE_6_DB
 */
typedef uint8_t ftdf_tx_power_tolerance_t;
#define FTDF_POWER_TOLERANCE_1_DB 1
#define FTDF_POWER_TOLERANCE_3_DB 2
#define FTDF_POWER_TOLERANCE_6_DB 3

/**
 * \brief  CCA mode
 * \remark Valid values:
 * - FTDF_CCA_MODE_1
 * - FTDF_CCA_MODE_2
 * - FTDF_CCA_MODE_3
 */
typedef uint8_t ftdf_cca_mode_t;
#define FTDF_CCA_MODE_1 1
#define FTDF_CCA_MODE_2 2
#define FTDF_CCA_MODE_3 3

/**
 * \brief  Frame control options
 * \remark Supported options:
 * - FTDF_PAN_ID_PRESENT:    Controls the PAN id compression/present bit in multipurpose and
     version 2 frames
 * - FTDF_IES_INCLUDED:      Indicates that ie must be included
 * - FTDF_SEQ_NR_SUPPRESSED: Controls suppression of the sequence number
 * \remark Multiple options can specified by bit wise or-ing them
 */
typedef uint8_t ftdf_frame_control_options_t;
#define FTDF_PAN_ID_PRESENT    0x01
#define FTDF_IES_INCLUDED      0x02
#define FTDF_SEQ_NR_SUPPRESSED 0x04

typedef struct {
        ftdf_channel_page_t    channel_page;
        ftdf_size_t            nr_of_channels;
        ftdf_channel_number_t* channels;
} ftdf_channel_descriptor_t;

typedef struct {
        ftdf_size_t                nr_of_channel_descriptors;
        ftdf_channel_descriptor_t* channel_descriptors;
} ftdf_channel_descriptor_list_t;

/**
 * \brief Dbm data type
 */
typedef int8_t  ftdf_dbm;

typedef uint8_t ftdf_frame_type_t;
#define FTDF_BEACON_FRAME          0
#define FTDF_DATA_FRAME            1
#define FTDF_ACKNOWLEDGEMENT_FRAME 2
#define FTDF_MAC_COMMAND_FRAME     3
#define FTDF_LLDN_FRAME            4
#define FTDF_MULTIPURPOSE_FRAME    5

/**
 * \brief  Command frame id
 * \remark Supported command frame IDs:
 *         - FTDF_COMMAND_ASSOCIATION_REQUEST
 *         - FTDF_COMMAND_ASSOCIATION_RESPONSE
 *         - FTDF_COMMAND_DISASSOCIATION_NOTIFICATION
 *         - FTDF_COMMAND_DATA_REQUEST
 *         - FTDF_COMMAND_PAN_ID_CONFLICT_NOTIFICATION
 *         - FTDF_COMMAND_ORPHAN_NOTIFICATION
 *         - FTDF_COMMAND_BEACON_REQUEST
 *         - FTDF_COMMAND_COORDINATOR_REALIGNMENT
 *         - FTDF_COMMAND_GTS_REQUEST
 */
typedef uint8_t ftdf_command_frame_id_t;
#define FTDF_COMMAND_ASSOCIATION_REQUEST          1
#define FTDF_COMMAND_ASSOCIATION_RESPONSE         2
#define FTDF_COMMAND_DISASSOCIATION_NOTIFICATION  3
#define FTDF_COMMAND_DATA_REQUEST                 4
#define FTDF_COMMAND_PAN_ID_CONFLICT_NOTIFICATION 5
#define FTDF_COMMAND_ORPHAN_NOTIFICATION          6
#define FTDF_COMMAND_BEACON_REQUEST               7
#define FTDF_COMMAND_COORDINATOR_REALIGNMENT      8
#define FTDF_COMMAND_GTS_REQUEST                  9

/**
 * \brief Security frame counter
 * \remark Valid range 0..0xffffffff (4 octets) if frame counter mode equals 4
 * \remark Valid range 0..0xffffffffff (5 octets) if frame counter mode equals 5
 */
typedef uint64_t ftdf_frame_counter_t;

/**
 * \brief Security frame counter mode
 * \remark Valid range 4..5
 */
typedef uint8_t  ftdf_frame_counter_mode_t;

typedef struct {
        ftdf_key_id_mode_t  key_id_mode;
        ftdf_octet_t        key_source[8];
        ftdf_key_index_t    key_index;
        ftdf_address_mode_t device_addr_mode;
        ftdf_pan_id_t       device_pan_id;
        ftdf_address_t      device_address;
} ftdf_key_id_lookup_descriptor_t;

typedef struct {
        ftdf_pan_id_t        pan_id;
        ftdf_short_address_t short_address;
        ftdf_ext_address_t   ext_address;
        ftdf_frame_counter_t frame_counter;
        ftdf_boolean_t       exempt;
} ftdf_device_descriptor_t;

/**
 * \brief  Device descriptor handle
 * \remark This is the index of a device in \link ftdf_device_table_t \endlink
 */
typedef uint8_t ftdf_device_descriptor_handle_t;

typedef struct {
        /** \brief Frame type */
        ftdf_frame_type_t       frame_type;
        /** \brief Command frame id */
        ftdf_command_frame_id_t command_frame_id;
} ftdf_key_usage_descriptor_t;

typedef struct {
        ftdf_size_t                     nr_of_key_id_lookup_descriptors;
        ftdf_key_id_lookup_descriptor_t *key_id_lookup_descriptors;
        ftdf_size_t                     nr_of_device_descriptor_handles;
        ftdf_device_descriptor_handle_t *device_descriptor_handles;
        ftdf_size_t                     nr_of_key_usage_descriptors;
        ftdf_key_usage_descriptor_t     *key_usage_descriptors;
        ftdf_octet_t                    key[16];
} ftdf_key_descriptor_t;

typedef struct {
        ftdf_frame_type_t       frame_type;
        ftdf_command_frame_id_t command_frame_id;
        ftdf_security_level_t   security_minimum;
        ftdf_boolean_t          device_override_security_minimum;
        ftdf_bitmap8_t          allowed_security_levels;
} ftdf_security_level_descriptor_t;

/**
 * \brief  Key table
 * \remark As followes an example how to initialize a key table for usage with FTDF_SET_REQUEST:
 * \code
 * ftdf_key_id_lookup_descriptor_t  lookupDescriptorsKey1[] =
 *          { { 2, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }, 7, 0, 0 0 },
 *            { 0, { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 2, 0x1234, 0x0001 } };
 * ftdf_key_id_lookup_descriptor_t  lookupDescriptorsKey2[] =
 *          { { 1, { 0, 0, 0, 0, 0, 0, 0, 0 }, 5, 0, 0, 0 } };
 * ftdf_device_descriptor_handle_t deviceDescriptorHandlesKey1[] = { 0, 2 };
 * ftdf_device_descriptor_handle_t deviceDescriptorHandlesKey2[] = { 1 };
 * ftdf_key_usage_descriptor_t     keyUsageDescriptorKey1[] = { { 1, 0 }, { 3, 4 } };
 * ftdf_key_usage_descriptor_t     keyUsageDescriptorKey2[] = { { 1, 0 } };
 * ftdf_key_descriptor_t          key_descriptors[] =
 *          { { sizeof(lookupDescriptorsKey1) / sizeof(ftdf_key_id_lookup_descriptor_t),
 *              lookupDescriptorsKey1,
 *              sizeof(deviceDescriptorHandlesKey1) / sizeof(ftdf_device_descriptor_handle_t),
 *              deviceDescriptorHandlesKey1,
 *              sizeof(keyUsageDescriptorKey1) / sizeof(ftdf_key_usage_descriptor_t),
 *              keyUsageDescriptorKey1,
 *              { 0x7e, 0x37, 0x2d, 0x41, 0xdf, 0x74, 0x72, 0x3f, 0x4c, 0x3a, 0xa7, 0x53, 0xa1, 0x11, 0x46, 0x8b } },
 *            { sizeof(lookupDescriptorsKey2) / sizeof(ftdf_key_id_lookup_descriptor_t),
 *              lookupDescriptorsKey2,
 *              sizeof(deviceDescriptorHandlesKey2) / sizeof(ftdf_device_descriptor_handle_t),
 *              deviceDescriptorHandlesKey2,
 *              sizeof(keyUsageDescriptorKey2) / sizeof(ftdf_key_usage_descriptor_t),
 *              keyUsageDescriptorKey1,
 *              { 0xdf, 0x74, 0x72, 0x3f, 0x7e, 0x37, 0x2d, 0x41, 0x4c, 0x3a, 0xa1, 0x11, 0x46, 0x8, 0xa7, 0x53b } } };
 * ftdf_key_table_t               keyTable =
 *           { sizeof(key_descriptors) / sizeof(ftdf_key_descriptor_t), key_descriptors };
 * \endcode
 */
typedef struct {
        /** \brief Number of key descriptors in the key table */
        ftdf_size_t           nr_of_key_descriptors;
        /** \brief Pointer to the first key descriptor entry */
        ftdf_key_descriptor_t *key_descriptors;
} ftdf_key_table_t;

typedef struct {
        /** \brief Number of device descriptors in the key table */
        ftdf_size_t              nr_of_device_descriptors;
        /** \brief Pointer to the first device descriptor entry */
        ftdf_device_descriptor_t *device_descriptors;
} ftdf_device_table_t;

typedef struct {
        /** \brief Number of security level descriptors in the key table */
        ftdf_size_t                      nr_of_security_level_descriptors;
        /** \brief Pointer to the first security level descriptor entry */
        ftdf_security_level_descriptor_t *security_level_descriptors;
} ftdf_security_level_table_t;

/**
 * \brief  IE type
 * \remark Supported IE types:
 *         - FTDF_SHORT_IE
 *         - FTDF_LONG_IE
 */
typedef uint8_t ftdf_ie_type_t;
#define FTDF_SHORT_IE 0
#define FTDF_LONG_IE  1

typedef uint8_t ftdf_ie_id_t;

typedef uint8_t ftdf_ie_length_t;

typedef struct {
        /** \brief IE type */
        ftdf_ie_type_t   type;
        /** \brief Sub IE id*/
        ftdf_ie_id_t     sub_id;
        /** \brief Sub IE length */
        ftdf_ie_length_t length;
        /** \brief Pointer to sub IE content */
        ftdf_octet_t     *sub_content;
} ftdf_sub_ie_descriptor_t;

typedef struct {
        /** \brief Number of sub IE descriptors in an MLME payload IE (id = 1) */
        ftdf_size_t              nr_of_sub_ie;
        /** \brief Pointer to the first sub IE descriptor */
        ftdf_sub_ie_descriptor_t *sub_ie;
} ftdf_sub_ie_list_t;

typedef union {
        /** \brief IE content if not a MLME payload IE (id = 1) */
        ftdf_octet_t       *raw;
        /** \brief IE content if a MLME payload IE (id = 1) */
        ftdf_sub_ie_list_t *nested;
} ftdf_ie_content_t;

typedef struct {
        /** \brief IE id */
        ftdf_ie_id_t      id;
        /** \brief Content length of the IE, undefined/unused in case of a MLME payload IE (id = 1) */
        ftdf_ie_length_t  length;
        /** \brief Content of the IE */
        ftdf_ie_content_t content;
} ftdf_ie_descriptor_t;

typedef struct {
        /** \brief Number of IE descriptors */
        ftdf_size_t          nr_of_ie;
        /** \brief Pointer to the first IE descriptor */
        ftdf_ie_descriptor_t *ie;
} ftdf_ie_list_t;

typedef struct {
        /** \brief Message id */
        ftdf_msg_id_t msg_id;
} ftdf_msg_buffer_t;

typedef struct {
        /** \brief Message id = FTDF_DATA_REQUEST */
        ftdf_msg_id_t                msg_id;
        /** \brief Source address mode */
        ftdf_address_mode_t          src_addr_mode;
        /** \brief Destination address mode */
        ftdf_address_mode_t          dst_addr_mode;
        /** \brief Destination PAN id */
        ftdf_pan_id_t                dst_pan_id;
        /** \brief Destination address */
        ftdf_address_t               dst_addr;
        /** \brief Length of MSDU payload */
        ftdf_data_length_t           msdu_length;
        /**
         * \brief MSDU payload data buffer. Must be freed by the application using FTDF_REL_DATA_BUFFER
         *        when it receives the FTDF_DATA_CONFIRM message
         */
        ftdf_octet_t                 *msdu;
        /** \brief Handle */
        ftdf_handle_t                msdu_handle;
        /** \brief Whether the data frame needs to be acknowledged or */
        ftdf_boolean_t               ack_tx;
        /** \brief NOT USED */
        ftdf_boolean_t               gts_tx;
        /**
         * \brief Indication whether the frame must be send directly or be queued until a
         *  DATA_REQUEST COMMAND frame is received
         */
        ftdf_boolean_t               indirect_tx;
        /** \brief Security level */
        ftdf_security_level_t        security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t           key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t                 key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t             key_index;
        /** \brief Frame control option bitmap */
        ftdf_frame_control_options_t frame_control_options;
        /**
         * \brief  Header IE list to be inserted.
         *         Ignored if the FTDF_IES_INCLUDED frame control option is NOT set.
         *         The IE list must be located in rentention RAM
         */
        ftdf_ie_list_t               *header_ie_list;
        /**
         * \brief  Payload IE list to be inserted.
         *         Ignored if the FTDF_IES_INCLUDED frame control option is NOT set.
         *         The IE list must be located in rentention RAM
         */
        ftdf_ie_list_t               *payload_ie_list;
        /** Indication whether a DATA or MULTIPURPOSE frame must be set */
        ftdf_boolean_t               send_multi_purpose;
        /** \brief DO NOT USE */
        uint8_t                      __private3;
} ftdf_data_request_t;

typedef struct {
        /** \brief Message id = FTDF_DATA_INDICATION */
        ftdf_msg_id_t         msg_id;
        /** \brief Source address mode */
        ftdf_address_mode_t   src_addr_mode;
        /** \brief Source PAN id */
        ftdf_pan_id_t         src_pan_id;
        /** \brief Source address */
        ftdf_address_t        src_addr;
        /** \brief Destination address mode */
        ftdf_address_mode_t   dst_addr_mode;
        /** \brief Destination PAN id */
        ftdf_pan_id_t         dst_pan_id;
        /** \brief Destination address */
        ftdf_address_t        dst_addr;
        /** \brief Length of MSDU payload */
        ftdf_data_length_t    msdu_length;
        /** \brief MSDU payload data buffer Must be freed by the application using FTDF_REL_DATA_BUFFER */
        ftdf_octet_t          *msdu;
        /** \brief MPDU link quality */
        ftdf_link_quality_t   mpdu_link_quality;
        /** \brief Data sequence number */
        ftdf_sn_t             dsn;
        /** \brief Timestamp of the time the frame was received */
        ftdf_time_t           timestamp;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
        /** /brief Payload IE list<br>
         *         The whole payload IE list (including IE descriptors, Sub IE descriptors and content)
         *         is allocated in one data buffer that must be freed by the application using
         *         FTDF_REL_DATA_BUFFER
         *         with the ie_list pointer as parameter
         */
        ftdf_ie_list_t        *payload_ie_list;
} ftdf_data_indication_t;

typedef struct {
        /** \brief Message id = FTDF_DATA_CONFIRM */
        ftdf_msg_id_t          msg_id;
        /** /brief The handle given by the application in the correspoding FTDF_DATA_REQUEST */
        ftdf_handle_t          msdu_handle;
        /** /brief Timestmap of the time the frame has been sent */
        ftdf_time_t            timestamp;
        /** /brief Status of the data request */
        ftdf_status_t          status;
        /** /brief Number of backoffs, undefined in case status not equal to FTDF_SUCCESS or TSCH enabled */
        ftdf_num_of_backoffs_t num_of_backoffs;
        /** /brief Data sequence number of the sent frame */
        ftdf_sn_t              dsn;
        /** /brief Acknowledgement payload IE list<br>
         *         The whole payload IE list (including IE descriptors, Sub IE descriptors and content)
         *         is allocated in one data buffer that must be freed by the application using
         *         FTDF_REL_DATA_BUFFER with the ie_list pointer as parameter
         */
        ftdf_ie_list_t         *ack_payload;
} ftdf_data_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_PURGE_REQUEST */
        ftdf_msg_id_t msg_id;
        /** \brief Handle the data request to be purged */
        ftdf_handle_t msdu_handle;
} ftdf_purge_request_t;

typedef struct {
        /** \brief Message id = FTDF_PURGE_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Handle of the purged data request */
        ftdf_handle_t msdu_handle;
        /** /brief Status of the purge request */
        ftdf_status_t status;
} ftdf_purge_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_ASSOCIATE_REQUEST */
        ftdf_msg_id_t              msg_id;
        /** \brief Channel number to be used for the associate request. Ignored in TSCH mode */
        ftdf_channel_number_t      channel_number;
        /** \brief Channel page to be used for the associate request. Ignored in TSCH mode */
        ftdf_channel_page_t        channel_page;
        /** \brief Coordinator address mode */
        ftdf_address_mode_t        coord_addr_mode;
        /** \brief Coordinator PAN id */
        ftdf_pan_id_t              coord_pan_id;
        /** \brief Coordinator address */
        ftdf_address_t             coord_addr;
        /**
         * \brief Capabillity information bitmap. The following capabilities are defined:<br>
         *        FTDF_CAPABILITY_IS_FFD: Device is a full function device<br>
         *        FTDF_CAPABILITY_AC_POWER: Device has AC power<br>
         *        FTDF_CAPABILITY_RECEIVER_ON_WHEN_IDLE: Device receiver is on when idle<br>
         *        FTDF_CAPABILITY_FAST_ASSOCIATION: Device wants a fast association<br>
         *        FTDF_CAPABILITY_SUPPORTS_SECURITY: Device supports security<br>
         *        FTDF_CAPABILITY_WANTS_SHORT_ADDRESS: Device wants short address<br>
         */
        ftdf_bitmap8_t             capability_information;
        /** \brief Security level */
        ftdf_security_level_t      security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t         key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t               key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t           key_index;
        /** \brief NOT USED */
        ftdf_channel_offset_t      channel_offset;
        /** \brief NOT USED */
        ftdf_hopping_sequence_id_t hopping_sequence_id;
        /** \brief DO NOT USE */
        uint8_t                    __private3;
} ftdf_associate_request_t;

typedef struct {
        /** \brief Message id = FTDF_ASSOCIATE_INDICATION */
        ftdf_msg_id_t              msg_id;
        /** \brief Extended address of the device that did the association request */
        ftdf_ext_address_t         device_address;
        /**
         * \brief Capabillity information bitmap. The following capabilities are defined:<br>
         *        FTDF_CAPABILITY_IS_FFD: Device is a full function device<br>
         *        FTDF_CAPABILITY_AC_POWER: Device has AC power<br>
         *        FTDF_CAPABILITY_RECEIVER_ON_WHEN_IDLE: Device receiver is on when idle<br>
         *        FTDF_CAPABILITY_FAST_ASSOCIATION: Device wants a fast association<br>
         *        FTDF_CAPABILITY_SUPPORTS_SECURITY: Device supports security<br>
         *        FTDF_CAPABILITY_WANTS_SHORT_ADDRESS: Device wants short address<br>
         */
        ftdf_bitmap8_t             capability_information;
        /** \brief Security level */
        ftdf_security_level_t      security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t         key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t               key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t           key_index;
        /** \brief NOT USED */
        ftdf_channel_offset_t      channel_offset;
        /** \brief NOT USED */
        ftdf_hopping_sequence_id_t hopping_sequence_id;
} ftdf_associate_indication_t;

typedef struct {
        /** \brief Message id = FTDF_ASSOCIATE_RESPONSE */
        ftdf_msg_id_t          msg_id;
        /** \brief Extended address of the device that did association request and to which the response needs to be send */
        ftdf_ext_address_t     device_address;
        /** \brief The short address for the device that did the associate request */
        ftdf_short_address_t   assoc_short_address;
        /** \brief The association status of the associate request */
        ftdf_association_status_t status;
        /** \brief Indication whether this a response to fast or normal association request */
        ftdf_boolean_t         fast_a;
        /** \brief Security level */
        ftdf_security_level_t  security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t     key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t           key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t       key_index;
        /** \brief NOT USED */
        ftdf_channel_offset_t  channel_offset;
        /** \brief NOT USED */
        ftdf_length_t          hopping_sequence_length;
        /** \brief NOT USED */
        ftdf_octet_t*          hopping_sequence;
        /** \brief DO NOT USE */
        uint8_t                __private3;
} ftdf_associate_response_t;

typedef struct {
        /** \brief Message id = FTDF_ASSOCIATE_CONFIRM */
        ftdf_msg_id_t         msg_id;
        /** \brief The short adddress assigned by the coordinator */
        ftdf_short_address_t  assoc_short_address;
        /** \brief Status of the associate request */
        ftdf_status_t         status;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
        /** \brief NOT USED */
        ftdf_channel_offset_t channel_offset;
        /** \brief NOT USED */
        ftdf_length_t         hopping_sequence_length;
        /** \brief NOT USED */
        ftdf_octet_t          *hopping_sequence;
} ftdf_associate_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_DISASSOCIATE_REQUEST */
        ftdf_msg_id_t              msg_id;
        /** \brief Device address mode */
        ftdf_address_mode_t        device_addr_mode;
        /** \brief Device PAN id */
        ftdf_pan_id_t              device_pan_id;
        /** \brief Device address */
        ftdf_address_t             device_address;
        /** \brief Disassociate reason */
        ftdf_disassociate_reason_t disassociate_reason;
        /**
         * \brief Indication whether the disassociate request must be send directly or be queued until a
         *        DATA_REQUEST command frame is received
         */
        ftdf_boolean_t             tx_indirect;
        /** \brief Security level */
        ftdf_security_level_t      security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t         key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t               key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t           key_index;
        /** \brief DO NOT USE */
        uint8_t                    __private3;
} ftdf_disassociate_request_t;

typedef struct {
        /** \brief Message id = FTDF_DISASSOCIATE_INDICATION */
        ftdf_msg_id_t              msg_id;
        /** \brief Extended address of the device that did disassociate request */
        ftdf_ext_address_t         device_address;
        /** \brief Disassociate reason */
        ftdf_disassociate_reason_t disassociate_reason;
        /** \brief Security level */
        ftdf_security_level_t      security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t         key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t               key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t           key_index;
} ftdf_disassociate_indication_t;

typedef struct {
        /** \brief Message id = FTDF_DISASSOCIATE_CONFIRM */
        ftdf_msg_id_t       msg_id;
        /** \brief Disassociate request status */
        ftdf_status_t       status;
        /** \brief Address mode of the device that want/must disassociate */
        ftdf_address_mode_t device_addr_mode;
        /** \brief PAN id of the device that want/must disassociate */
        ftdf_pan_id_t       device_pan_id;
        /** \brief address of the device that want/must disassociate */
        ftdf_address_t      device_address;
} ftdf_disassociate_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_BEACON_NOTIFY_INDCATION */
        ftdf_msg_id_t         msg_id;
        /** \brief Beacon sequence number, valid only when beacon type is FTDF_NORMAL_BEACON */
        ftdf_sn_t             bsn;
        /** \brief PAN descriptor*/
        ftdf_pan_descriptor_t *pan_descriptor;
        /** \brief NOT USED */
        ftdf_bitmap8_t        pend_addr_spec;
        /** \brief NOT USED */
        ftdf_address_t        *addr_list;
        /** \brief Length of the beacon payload */
        ftdf_data_length_t    sdu_length;
        /** \brief Beacon payload */
        ftdf_octet_t          *sdu;
        /** \brief Enhanced beacon sequence number, valid only when beacon type is FTDF_ENHANCED_BEACON */
        ftdf_sn_t             eb_sn;
        /** \brief Beacon type */
        ftdf_beacon_type_t    beacon_type;
        /**
         * \brief Payload IE list<br>
         *        The whole Payload IE list (including IE descriptors, Sub IE descriptors and content)
         *        is allocate in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
         *        with the ie_list pointer as parameter.
         */
        ftdf_ie_list_t        *ie_list;
        /**
         * \brief Timestamp of the time the beacon frame was received<br>
         *        In TSCH mode this timestamp should be used to calculate the TSCH timeslot start time
         *        required in a FTDF_TSCH_MODE_REQUEST message
         */
        ftdf_time64_t         timestamp;
} ftdf_beacon_notify_indication_t;

typedef struct {
        /** \brief Message id = FTDF_COMM_STATUS_INDICATION */
        ftdf_msg_id_t         msg_id;
        /** \brief PAN id of the frame causing the error */
        ftdf_pan_id_t         pan_id;
        /** \brief Source address mode of the frame causing the error  */
        ftdf_address_mode_t   src_addr_mode;
        /** \brief Source address of the frame causing the error */
        ftdf_address_t        src_addr;
        /** \brief Destination address mode of the frame causing the error */
        ftdf_address_mode_t   dst_addr_mode;
        /** \brief Destination address of the frame causing the error */
        ftdf_address_t        dst_addr;
        /** \brief Error status */
        ftdf_status_t         status;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_comm_status_indication_t;

typedef struct {
        /** \brief Message id = FTDF_GET_REQUEST */
        ftdf_msg_id_t        msg_id;
        ftdf_pib_attribute_t pib_attribute;
} ftdf_get_request_t;

typedef struct {
        /** \brief Message id = FTDF_GET_CONFIRM */
        ftdf_msg_id_t                    msg_id;
        /** \brief Status of the get request */
        ftdf_status_t                    status;
        /** \brief PIB attribute id of PIB attribute gotten */
        ftdf_pib_attribute_t             pib_attribute;
        /**
         * \brief Pointer to the PIB attribute value to be set<br>
         *        Each PIB attribute has its own type, see \link ftdf_pib_attribute_t \endlink<br>
         *        In order to access the attribute value the application must cast this pinter
         *        to the correct type.
         */
        const ftdf_pib_attribute_value_t *pib_attribute_value;
} ftdf_get_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_SET_REQUEST */
        ftdf_msg_id_t                    msg_id;
        /** \brief PIB attribute id of attribute to be set */
        ftdf_pib_attribute_t             pib_attribute;
        /**
         * \brief Pointer to the PIB attribute value to be set<br>
         *        Each PIB attribute has its own type, see \link ftdf_pib_attribute_t \endlink<br>
         *        FTDF does a shallow copy of the PIB attribute. So if the PIB attribute contains pointers to
         *        other data this data is NOT copied by FTDF. The application must make sure that data referred
         *        from a PIB attribute is statically allocated in retention RAM.
         */
        const ftdf_pib_attribute_value_t *pib_attribute_value;
} ftdf_set_request_t;

typedef struct {
        /** \brief Message id = FTDF_GET_CONFIRM */
        ftdf_msg_id_t        msg_id;
        /** \brief Set request status */
        ftdf_status_t        status;
        /** \brief PIB Attribute id of PIB attribute that has been set */
        ftdf_pib_attribute_t pib_attribute;
} ftdf_set_confirm_t;

typedef struct {
        ftdf_msg_id_t             msg_id;
        ftdf_gts_charateristics_t gts_charateristics;
        ftdf_security_level_t     security_level;
        ftdf_key_id_mode_t        key_id_mode;
        ftdf_octet_t              key_source[8];
        ftdf_key_index_t          key_index;
} ftdf_gts_request_t;

typedef struct {
        ftdf_msg_id_t             msg_id;
        ftdf_gts_charateristics_t gts_charateristics;
        ftdf_status_t             status;
} ftdf_gts_confirm_t;

typedef struct {
        ftdf_msg_id_t             msg_id;
        ftdf_short_address_t      device_address;
        ftdf_gts_charateristics_t gts_charateristics;
        ftdf_security_level_t     security_level;
        ftdf_key_id_mode_t        key_id_mode;
        ftdf_octet_t              key_source[8];
        ftdf_key_index_t          key_index;
} ftdf_gts_indication_t;

typedef struct {
        /** \brief Message id = FTDF_ORPHAN_INDICATION */
        ftdf_msg_id_t         msg_id;
        /** \brief Extended address of orphaned device */
        ftdf_ext_address_t    orphan_address;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_orphan_indication_t;

typedef struct {
        /** \brief Message id = FTDF_ORPHAN_RESPONSE */
        ftdf_msg_id_t         msg_id;
        /** \brief Extended address of orphaned device */
        ftdf_ext_address_t    orphan_address;
        /** \brief Short address of orphaned device */
        ftdf_short_address_t  short_address;
        /** \brief Indication whether the device is associated with this coordinator or not */
        ftdf_boolean_t        associated_member;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_orphan_response_t;

typedef struct {
        /** \brief Message id = FTDF_RESET_REQUEST */
        ftdf_msg_id_t  msg_id;
        /** \brief Indication whether the PIB attributes must be reset to their default values or not */
        ftdf_boolean_t set_default_pib;
} ftdf_reset_request_t;

typedef struct {
        /** \brief Message id = FTDF_RESET_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the reset request */
        ftdf_status_t status;
} ftdf_reset_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_RX_ENABLE_REQUEST */
        ftdf_msg_id_t  msg_id;
        /** \brief NOT USED */
        ftdf_boolean_t defer_permit;
        /** \brief NOT USED */
        ftdf_time_t    rx_on_time;
        /** \brief The time in aBaseSuperFramePeriod's (960 symbols) that the receiver will be on */
        ftdf_time_t    rx_on_duration;
} ftdf_rx_enable_request_t;

typedef struct {
        /** \brief Message id = FTDF_RX_ENABLE_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the RX enable request */
        ftdf_status_t status;
} ftdf_rx_enable_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_SCAN_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Scan Type */
        ftdf_scan_type_t      scan_type;
        /** \brief Bitmap of the channels to be scanned  (only channel 11 - 27 are supported) */
        ftdf_bitmap32_t       scan_channels;
        /** \brief Channel page of the channels to be scanned (only page 0 is supported) */
        ftdf_channel_page_t   channel_page;
        /** \brief Scan duration per channel, duration = ((scan_duration ^ 2) - 1) * aBaseSuperFramePeriod symbols */
        ftdf_scan_duration_t  scan_duration;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_scan_request_t;

typedef struct {
        /** \brief Message id = FTDF_SCAN_CONFIRM */
        ftdf_msg_id_t                   msg_id;
        /** \brief Status of the scan request */
        ftdf_status_t                   status;
        /** \brief Scan Type */
        ftdf_scan_type_t                scan_type;
        /** \brief Channel page of the scanned channels */
        ftdf_channel_page_t             channel_page;
        /** \brief Bitmap of the channels not scanned but were requested to be scanned */
        ftdf_bitmap32_t                 unscanned_channels;
        /** \brief Number of entries in energy_detect_list or pan_descriptor_list */
        ftdf_size_t                     result_list_size;
        /** \brief Pointer to the first energy detect result */
        ftdf_energy_t                   *energy_detect_list;
        /** \brief Pointer to the first PAN descriptor */
        ftdf_pan_descriptor_t           *pan_descriptor_list;
        /** \brief */
        ftdf_coord_realign_descriptor_t *coord_realign_descriptor;
} ftdf_scan_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_START_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief PAN id that will be used by the coordinator */
        ftdf_pan_id_t         pan_id;
        /** \brief Channel number that will be used by the coordinator */
        ftdf_channel_number_t channel_number;
        /** \brief Channel page that will be used by the coordinator */
        ftdf_channel_page_t   channel_page;
        /** \brief NOT USED */
        ftdf_time_t           start_time;
        /** \brief Beacon order, only beacon order 15 (beaconless) is supported  */
        ftdf_order_t          beacon_order;
        /** \brief NOT USED */
        ftdf_order_t          superframe_order;
        /** \brief Indication whether the device must be a coordinator or a PAN coordinator */
        ftdf_boolean_t        pan_coordinator;
        /** \brief NOT USED */
        ftdf_boolean_t        battery_life_extension;
        /** \brief NOT USED */
        ftdf_boolean_t        coord_realignment;
        /** \brief NOT USED */
        ftdf_security_level_t coord_realign_security_level;
        /** \brief NOT USED */
        ftdf_key_id_mode_t    coord_realign_key_id_mode;
        /** \brief NOT USED */
        ftdf_octet_t          coord_realign_key_source[8];
        /** \brief NOT USED */
        ftdf_key_index_t      coord_realign_key_index;
        /** \brief NOT USED */
        ftdf_security_level_t beacon_security_level;
        /** \brief NOT USED */
        ftdf_key_id_mode_t    beacon_key_id_mode;
        /** \brief NOT USED */
        ftdf_octet_t          beacon_key_source[8];
        /** \brief NOT USED */
        ftdf_key_index_t      beacon_key_index;
} ftdf_start_request_t;

typedef struct {
        /** \brief Message id = FTDF_START_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the start request */
        ftdf_status_t status;
} ftdf_start_confirm_t;

typedef struct {
        ftdf_msg_id_t         msg_id;
        ftdf_channel_number_t channnel_number;
        ftdf_channel_page_t   channel_page;
        ftdf_boolean_t        trackbeacon;
} ftdf_sync_request_t;

typedef struct {
        /** \brief Message id = FTDF_SYNC_LOSS_INDICATION */
        ftdf_msg_id_t         msg_id;
        /** \brief Loss Reason */
        ftdf_loss_reason_t    loss_reason;
        /** \brief PAN id */
        ftdf_pan_id_t         pan_id;
        /** \brief Channel number */
        ftdf_channel_number_t channel_number;
        /** \brief Channel page */
        ftdf_channel_page_t   channel_page;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_sync_loss_indication_t;

typedef struct {
        /** \brief Message id = FTDF_POLL_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Coordinator address mode */
        ftdf_address_mode_t   coord_addr_mode;
        /** \brief coordinator PAN id */
        ftdf_pan_id_t         coord_pan_id;
        /** \brief Coordinator address */
        ftdf_address_t        coord_addr;
        /** \brief Security level */
        ftdf_security_level_t security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      key_index;
} ftdf_poll_request_t;

typedef struct {
        /** \brief Message id = FTDF_POLL_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the poll request */
        ftdf_status_t status;
} ftdf_poll_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_SET_SLOTFRAME_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Slotframe handle */
        ftdf_handle_t         handle;
        /** \brief Set operation */
        ftdf_operation_t      operation;
        /** \brief Number of timeslots in this slotframe */
        ftdf_slotframe_size_t size;
} ftdf_set_slotframe_request_t;

typedef struct {
        /** \brief Message id = FTDF_SET_SLOTFRAME_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Slotframe handle */
        ftdf_handle_t handle;
        /** \brief Status of the set sloytframe request */
        ftdf_status_t status;
} ftdf_set_slotframe_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_SET_LINK_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Set operation */
        ftdf_operation_t      operation;
        /** \brief Handle of this link, used for delete and modify operations */
        ftdf_handle_t         link_handle;
        /** \brief Handle of the slotframe of this link */
        ftdf_handle_t         slotframe_handle;
        /** \brief Timeslot in the slotframe */
        ftdf_timeslot_t       timeslot;
        /** \brief Channel offset */
        ftdf_channel_offset_t channel_offset;
        /**
         * \brief Bit mapped link options, the following options are supported:<br>
         *        FTDF_LINK_OPTION_TRANSMIT: Lik can be used for transmitting<br>
         *        FTDF_LINK_OPTION_RECEIVE: Link can be used for receiving<br>
         *        FTDF_LINK_OPTION_SHARED: Shared link, multiple device can transmit in this link<br>
         *        FTDF_LINK_OPTION_TIME_KEEPING: Link will be used to synchronise timeslots<br>
         *        Multiple options can be defined by or-ing them.
         */
        ftdf_bitmap8_t        link_options;
        /** \brief Link type */
        ftdf_link_type_t      link_type;
        /** \brief Short address of the peer node, only valid if is a transmit link */
        ftdf_short_address_t  node_address;
} ftdf_set_link_request_t;

typedef struct {
        /** \brief Message id = FTDF_SET_LINK_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the set link request */
        ftdf_status_t status;
        /** \brief Link handle */
        ftdf_handle_t link_handle;
        /** \brief Slotframe handle */
        ftdf_handle_t slotframe_handle;
} ftdf_set_link_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_TSCH_MODE_REQUEST */
        ftdf_msg_id_t    msg_id;
        /** \brief The new TSCH mode */
        ftdf_tsch_mode_t tschMode;
        /**
         * \brief The timestamp in symbols of the start of the timeslot with ASN equal to the PIB attribute
         *        FTDF_PIB_ASN<br>
         *        The timestamp should be calculated from the timestamp of the BEACON_NOTIFY_INDICATION message<br>
         *        Before sending the FTDF_TSCH_MODE_REQUEST message the application should set FTDF_PIB_ASN with
         *        the ASN of the TSCH synchronization IE of the ie_list of the BEACON_NOTIFY_INDICATION message
         */
        ftdf_time_t      timeslotStartTime;
} ftdf_tsch_mode_request_t;

typedef struct {
        /** \brief Message id = FTDF_TSCH_MODE_CONFIRM */
        ftdf_msg_id_t    msg_id;
        /** \brief The new TSCH mode */
        ftdf_tsch_mode_t tschMode;
        /** \brief Status of the TSCH mode request */
        ftdf_status_t    status;
} ftdf_tsch_mode_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_KEEP_ALIVE_REQUEST */
        ftdf_msg_id_t            msg_id;
        /** \brief Short address to which keep alive message should be send */
        ftdf_short_address_t     dst_address;
        /** \brief */
        ftdf_keep_alive_period_t keep_alive_period;
} ftdf_keep_alive_request_t;

typedef struct {
        /** \brief Message id = FTDF_KEEP_ALIVE_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the keep alive request */
        ftdf_status_t status;
} ftdf_keep_alive_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_BEACON_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Beacon type */
        ftdf_beacon_type_t    beacon_type;
        /** \brief Channel number at which the beacon must be sent, ignored in TSCH mode */
        ftdf_channel_number_t channel;
        /** \brief Channel page, ignored in TSCH mode */
        ftdf_channel_page_t   channel_page;
        /** \brief NOT USED */
        ftdf_order_t          superframe_order;
        /** \brief Security level */
        ftdf_security_level_t beacon_security_level;
        /** \brief Security key id mode */
        ftdf_key_id_mode_t    beacon_key_id_mode;
        /** \brief Security key source */
        ftdf_octet_t          beacon_key_source[8];
        /** \brief Security key index */
        ftdf_key_index_t      beacon_key_index;
        /** \brief Beacon destination address mode */
        ftdf_address_mode_t   dst_addr_mode;
        /** \brief Beacon destination address */
        ftdf_address_t        dst_addr;
        /** \brief Indication whether the bsn (or eb_sn) in the beacon frame must suppressed */
        ftdf_boolean_t        bsn_suppression;
        /** \brief DO NOT USE */
        uint8_t               __private3;
} ftdf_beacon_request_t;

typedef struct {
        /** \brief Message id = FTDF_BEACON_CONFIRM */
        ftdf_msg_id_t msg_id;
        /** \brief Status of the beacon request */
        ftdf_status_t status;
} ftdf_beacon_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_BEACON_REQUEST_INDICATION */
        ftdf_msg_id_t       msg_id;
        /** \brief Beacon type */
        ftdf_beacon_type_t  beacon_type;
        /** \brief Source address mode */
        ftdf_address_mode_t src_addr_mode;
        /** \brief Source address */
        ftdf_address_t      src_addr;
        /** \brief Source PAN id */
        ftdf_pan_id_t       dst_pan_id;
        /**
         * \brief Payload IE list<br>
         *        The whole Payload IE list (including IE descriptors, Sub IE descriptors and content)
         *        is allocate in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
         *        with the ie_list pointer as parameter.
         */
        ftdf_ie_list_t      *ie_list;
} ftdf_beacon_request_indication_t;

typedef struct {
        /** \brief Message id = FTDF_TRANSPARENT_ENABLE_REQUEST */
        ftdf_msg_id_t   msg_id;
        /** \brief Enable or disable of transparent mode */
        ftdf_boolean_t  enable;
        /** \brief Bitmapped transparent options, see ftdf_enable_transparent_mode */
        ftdf_bitmap32_t options;
} ftdf_transparent_enable_request_t;

typedef struct {
        /** \brief Message id = FTDF_TRANSPARENT_REQUEST */
        ftdf_msg_id_t         msg_id;
        /** \brief Frame length */
        ftdf_data_length_t    frame_length;
        /** \brief Pointer to frame, must be allocated with FTDF_GET_DATA_BUFFER */
        ftdf_octet_t          *frame;
        /** \brief Channel number */
        ftdf_channel_number_t channel;
        /** \brief Packet Traffic Information (PTI) used for transmitting the frame. */
        ftdf_pti_t            pti;
        /** \brief CSMA suppression */
        ftdf_boolean_t        cmsa_suppress;
        /** \brief Application provided handle, will be returned in the transparent confirm call */
        void                  *handle;
} ftdf_transparent_request_t;

typedef struct {
        /** \brief Message id = FTDF_TRANSPARENT_CONFIRM */
        ftdf_msg_id_t   msg_id;
        /** \brief Application provided handle of the corresponding FTDF_SEND_FRAME_TRANSPARENT call */
        void            *handle;
        /** \brief Bit mapped status */
        ftdf_bitmap32_t status;
} ftdf_transparent_confirm_t;

typedef struct {
        /** \brief Message id = FTDF_TRANSPARENT_INDICATION */
        ftdf_msg_id_t      msg_id;
        /** \brief Frame length */
        ftdf_data_length_t frame_length;
        /** \brief Pointer to frame, must be freed by the application using FTDF_REL_DATA_BUFFER */
        ftdf_octet_t       *frame;
        /** \brief Bit mapped receive status */
        ftdf_bitmap32_t    status;
} ftdf_transparent_indication_t;

typedef struct {
        /** \brief Message id = FTDF_SLEEP_REQUEST */
        ftdf_msg_id_t msg_id;
        /** \brief Sleep time */
        ftdf_usec_t   sleep_time;
} ftdf_sleep_request_t;

#if FTDF_DBG_BUS_ENABLE
typedef struct {
        /** \brief Message id = FTDF_DBG_MODE_SET_REQUEST */
        ftdf_msg_id_t   msg_id;
        /** \brief Debug bus mode */
        ftdf_dbg_mode_t dbg_mode;
} ftdf_dbg_mode_set_request_t;
#endif /* FTDF_DBG_BUS_ENABLE */

typedef struct
{
        /** \brief Message ID = FTDF_FPPR_MODE_SET_REQUEST */
        ftdf_msg_id_t  msg_id;
        /** \brief fp bit value to set when src address matches*/
        ftdf_bitmap8_t match_fp;
        /** \brief When set, fp_force_value will be applied to fp bit for all src addresses*/
        ftdf_bitmap8_t fp_override;
        /** \brief The value will be set to fp bit for all src addresses when fp_override is set*/
        ftdf_bitmap8_t fp_force;
} ftdf_fppr_mode_set_request_t;

/**
 * \brief       ftdf_get_release_info - gets the LMAC (FPGA) and UMAC (FTDF SW driver) release info
 * \param[out]  lmac_rel_name:     LMAC release name
 * \param[out]  lmac_build_time:   LMAC build time
 * \param[out]  umac_rel_name:     UMAC release name
 * \param[out]  umac_build_time:   UMAC build time
 * \remark      Sets the application character strings pointers to character strings containing the release
 *              info. The character string space is statically allocated by FTDF and does not need
 *              to be released by the application.
 */
void ftdf_get_release_info(char **lmac_rel_name, char **lmac_build_time, char **umac_rel_name,
                           char **umac_build_time);

/**
 * \brief       ftdf_confirm_lmac_interrupt - Confirmation that the LMAC interrupt has been received
 *              by the application
 * \remark      This function disables the LMAC interrupt. The application must wait for any other
 *              FTDF API function to be completed and call the function ftdf_event_handler. This function
 *              will enable the LMAC interrupt again the function ftdf_event_handler.
 */
void ftdf_confirm_lmac_interrupt(void);

/**
 * \brief       ftdf_event_handler - Handles an event signalled by the LMAC interrupt
 * \remark      This function handles the event signalled by the LMAC interrupt and enables the LMAC
 *              interrupt when finished.
 * \warning     FTDF is strictly NON reentrant, so ftdf_event_handler is NOT allowed to be called when another FTDF
 *              is being executed
 */
void ftdf_event_handler(void);

/**
 * \brief       ftdf_snd_msg - Sends a request (response) message to FTDF driver
 * \param[in]   msg_buf:     The message to be send. To application must set the msg_id field and the request
 *                           specific parameters by casting it to the appropriate request message structure.
 * \remark      The message must be allocated by the application using the function ftdf_get_msg_buffer
 *              and will be released by FTDF using ftdf_rel_msg_buffer. When finished FTDF releases the message
 *              buffer and sends a confirm (indication) to the application using the function ftdf_rcv_msg.
 * \remark      As follows a table of message IDs accepted by ftdf_snd_msg, the message structure corresponding
 *              to this message id and the confirm send back to the application when the request has been
 *              completed by FTDF:
 * \remark      <br>
 * \remark      Request message id, Request structure, Request confirm
 * \remark      ---------------------------------------------------------------------------------
 * \remark      FTDF_DATA_REQUEST, ftdf_data_request_t, FTDF_DATA_CONFIRM
 * \remark      FTDF_PURGE_REQUEST, ftdf_purge_request_t, FTDF_DATA_CONFIRM
 * \remark      FTDF_ASSOCIATE_REQUEST, ftdf_associate_request_t, FTDF_ASSOCIATE_CONFIRM
 * \remark      FTDF_ASSOCIATE_RESPONSE, ftdf_associate_response_t, FTDF_COMM_STATUS_INDICATION
 * \remark      FTDF_DISASSOCIATE_REQUEST, ftdf_disassociate_request_t, FTDF_DISASSOCIATE_CONFIRM
 * \remark      FTDF_GET_REQUEST, ftdf_get_request_t, FTDF_GET_CONFIRM
 * \remark      FTDF_SET_REQUEST, ftdf_set_request_t, FTDF_SET_CONFIRM
 * \remark      FTDF_ORPHAN_RESPONSE, ftdf_orphan_response_t,  FTDF_COMM_STATUS_INDICATION
 * \remark      FTDF_RESET_REQUEST, ftdf_reset_request_t, FTDF_RESET_CONFIRM
 * \remark      FTDF_RX_ENABLE_REQUEST, ftdf_scan_request_t, FTDF_RX_ENABLE_CONFIRM
 * \remark      FTDF_SCAN_REQUEST, ftdf_scan_request_t, FTDF_SCAN_CONFIRM
 * \remark      FTDF_START_REQUEST, ftdf_start_request_t, FTDF_START_CONFIRM
 * \remark      FTDF_POLL_REQUEST, ftdf_poll_request_t, FTDF_POLL_CONFIRM
 * \remark      FTDF_SET_SLOTFRAME_REQUEST, ftdf_set_slotframe_request_t, FTDF_SET_SLOTFRAME_CONFIRM
 * \remark      FTDF_SET_LINK_REQUEST, ftdf_set_link_request_t, FTDF_SET_LINK_CONFIRM
 * \remark      FTDF_TSCH_MODE_REQUEST, ftdf_tsch_mode_request_t, FTDF_TSCH_MODE_CONFIRM
 * \remark      FTDF_KEEP_ALIVE_REQUEST, ftdf_keep_alive_request_t, FTDF_KEEP_ALIVE_CONFIRM
 * \remark      FTDF_BEACON_REQUEST, ftdf_beacon_request_t, FTDF_BEACON_CONFIRM
 * \warning     FTDF is strictly NON reentrant, so it is NOT allowed to call any other FTDF function before this API has been completed. This including the ftdf_event_handler function.
 */
void ftdf_snd_msg(ftdf_msg_buffer_t *msg_buf);

/**
 * \brief       FTDF_RCV_MSG - Receives a confir or indication message from the FTDF driver
 * \param[in]   msg_buf:     The received message. The msg_id field indicates the received message
 * \remark      This function must be implemented by the application and is called by FTDF to pass
 *              a confirm or indication message to the application. It must be configured in the
 *              configuration file ftdf_config.h.
 * \remark      The message buffer is allocated by FTDF using FTDF_GET_MSG_BUFFER and must be released
 *              by the application using ftdf_rel_msg_buffer
 * \remark      As follows a table of the confirm and indications messages and their message structures.
 * \remark      <br>
 * \remark      Confirm/indication message id, Confirm/indication structure
 * \remark      ---------------------------------------------------------------------------------
 * \remark      FTDF_DATA_INDICATION, ftdf_data_indication_t
 * \remark      FTDF_DATA_CONFIRM, ftdf_data_confirm_t
 * \remark      FTDF_PURGE_CONFIRM, ftdf_purge_confirm_t
 * \remark      FTDF_ASSOCIATE_INDICATION, ftdf_associate_indication_t
 * \remark      FTDF_ASSOCIATE_CONFIRM, ftdf_associate_confirm_t
 * \remark      FTDF_DISASSOCIATE_INDICATION, ftdf_disassociate_indication_t
 * \remark      FTDF_DISASSOCIATE_CONFIRM,ftdf_disassociate_confirm_t
 * \remark      FTDF_BEACON_NOTIFY_INDICATION, ftdf_beacon_notify_indication_t
 * \remark      FTDF_COMM_STATUS_INDICATION, ftdf_comm_status_indication_t
 * \remark      FTDF_GET_CONFIRM, ftdf_get_confirm_t
 * \remark      FTDF_SET_CONFIRM, ftdf_set_confirm_t
 * \remark      FTDF_ORPHAN_INDICATION, ftdf_orphan_indication_t
 * \remark      FTDF_RESET_CONFIRM, ftdf_reset_confirm_t
 * \remark      FTDF_RX_ENABLE_CONFIRM, ftdf_rx_enable_confirm_t
 * \remark      FTDF_SCAN_CONFIRM, ftdf_scan_confirm_t
 * \remark      FTDF_START_CONFIRM, ftdf_start_confirm_t
 * \remark      FTDF_SYNC_LOSS_INDICATION, ftdf_sync_loss_indication_t
 * \remark      FTDF_POLL_CONFIRM, ftdf_poll_confirm_t
 * \remark      FTDF_SET_SLOTFRAME_CONFIRM, ftdf_set_slotframe_confirm_t
 * \remark      FTDF_SET_LINK_CONFIRM, ftdf_set_link_confirm_t
 * \remark      FTDF_TSCH_MODE_CONFIRM, ftdf_tsch_mode_confirm_t
 * \remark      FTDF_KEEP_ALIVE_CONFIRM, ftdf_keep_alive_confirm_t
 * \remark      FTDF_BEACON_CONFIRM, ftdf_beacon_confirm_t
 * \remark      FTDF_BEACON_REQUEST_INDICATION, ftdf_beacon_request_indication_t
 * \warning     FTDF is strictly NON reentrant. Because ftdf_rcv_msg can be called by ftdf_snd_msg or ftdf_event_handler
 *              it is not allowed to call other FTDF functions within ftdf_rcv_msg
 */
void FTDF_RCV_MSG(ftdf_msg_buffer_t *msg_buf);

/**
 * \brief       FTDF_GET_MSG_BUFFER - Allocates a message buffer
 * \param[in]   len:      The length of the message buffer to be allocated
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      The allocated message buffer must be freed using ftdf_rel_msg_buffer
 * \remark      The message buffer must be located in retention RAM
 * \return      pointer to the allocated message buffer.
 */
ftdf_msg_buffer_t *FTDF_GET_MSG_BUFFER(ftdf_size_t len);

/**
 * \brief       FTDF_REL_MSG_BUFFER - Frees a message buffer
 * \param[in]   msg_buf:      Pointer to the message buffer to be released
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      It will free message buffers allocated with ftdf_get_msg_buffer
 */
void FTDF_REL_MSG_BUFFER(ftdf_msg_buffer_t *msg_buf);

/**
 * \brief       FTDF_GET_DATA_BUFFER - Allocates a data buffer
 * \param[in]   len:      The length of the data buffer to be allocated
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      The allocated data buffer must be freed using FTDF_REL_DATA_BUFFER
 * \remark      The data buffer must located in retention RAM
 * \return      pointer to the allocated data buffer.
 */
ftdf_octet_t *FTDF_GET_DATA_BUFFER(ftdf_data_length_t len);

/**
 * \brief       FTDF_REL_DATA_BUFFER - Frees a data buffer
 * \param[in]   data_buf:      Pointer to the data buffer to be released
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      It will free data buffers allocated with FTDF_GET_DATA_BUFFER
 */
void FTDF_REL_DATA_BUFFER(ftdf_octet_t *data_buf);

/**
 * \brief       FTDF_GET_EXT_ADDRESS - Gets the extended address
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      After a reset it is used by FTDF to get the extended address of the device
 * \return      An extended address.
 */
ftdf_ext_address_t FTDF_GET_EXT_ADDRESS(void);

/**
 * \brief       ftdf_enable_transparent_mode - Enables or disables the transparent mode
 * \param[in]   enable: If FTDF_TRUE transparent mode is enabled else it will be disabled
 * \param[in]   options: A bitmap of transparent mode options. Not used if enable equals to FTDF_FALSE.
 * \remark      If the transparent mode is enabled received frame are passed to the application using
 *              FTDF_RCV_FRAME_TRANSPARENT. The whole MAC frame including frame headers, security headers
 *              and FCS are passed to the application. Depending on the defined options some filtering
 *              is done.
 * \remark      When the transparent mode is enabled frames must be send using the function FTDF_SEND_FRAME_TRANSPARENT
 * \remark      When the transparent mode is enabled only the following request messages are supported:
 * \remark      <br>
 * \remark      FTDF_GET_REQUEST
 * \remark      FTDF_SET_REQUEST
 * \remark      FTDF_RESET_REQUEST
 * \remark      <br>
 * \remark      The behavior of FTDF when it receives request not supported in transparent mode is NOT defined!
 * \remark      <br>
 * \remark      Transparent mode cannot be enabled if FTDF is in CSL or TSCH mode.
 * \remark      <br>
 * \remark      The following options are supported:
 * \remark      <br>
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_0: Frames of type 0 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_1: Frames of type 1 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_2: Frames of type 2 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_3: Frames of type 3 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_4: Frames of type 4 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_5: Frames of type 5 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_6: Frames of type 6 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_FRAME_TYPE_7: Frames of type 7 are always passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_FRAME_TYPES: All frame types are always passed to the application
 * \remark      FTDF_TRANSPARENT_AUTO_ACK: FTDF sends an automatically and ACK if it receives an version 0 or 1
 *              frame with AR set in the fram control field
 * \remark      FTDF_TRANSPARENT_AUTO_FP_ACK:  FTDF sends an automatically and ACK frame with FP set if it
 *              receives an version 0 or 1 DATA_REQUEST command frame with AR set in the fram control field
 * \remark      FTDF_TRANSPARENT_PASS_CRC_ERROR: Frames with a CRC (FCS) error are passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_FRAME_VERSION: Frames with any frame version are passed to the
 *              application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_PAN_ID: Frames with any PAN id (even when it does not match its own
 *              PAN id) are passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_PAN_ID: Frames with any address (even when it does not match its own
 *              extended or short address) are passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_BEACON: Any beacon frame is passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_NO_DEST_ADDR: Frames with no destination address are always passed to
 *              the application
 * \remark      FTDF_TRANSPARENT_ENABLE_FCS_GENERATION: FTDF adds an FCS to frames send with FTDF_SEND_FRAME_TRANSPARENT
 */
void ftdf_enable_transparent_mode(ftdf_boolean_t  enable, ftdf_bitmap32_t options);

/**
 * \brief       ftdf_send_frame_transparent - Sends a frame transparent
 * \param[in]   frame_length:  Length of the MAC frame to be send
 * \param[in]   frame:         Data buffer with the frame to be sent. Must be allocate using FTDF_GET_DATA_BUFFER
 * \param[in]   channel:       The channel used to send the frame
 * \param[in]   pti:           Packet Traffic Information used for transmitting the packet
 * \param[in]   cmsa_suppress: If FTDF_TRUE no CSMA-CA is performed before sending the frame
 * \param[in]   handle:        A user defined handle.
 * \remark      FTDF sends the frame tarnsparently. That is the application has to compose the MAC frame header,
 *              security header, etc and make sure that it complies with the IEEE802.15.4 standard. Before sending
 *              it FTDF adds the  preamble, frame start, phy header octets and optionally an FCS to the frame provide
 *              by the application.
 * \remark      When FTDF has send the frame or failed to send the frame it will call FTDF_SEND_FRAME_TRANSPARENT_CONFIRM
 *              with the handle provided by the application and the send status.
 */
void ftdf_send_frame_transparent(ftdf_data_length_t    frame_length,
                                 ftdf_octet_t          *frame,
                                 ftdf_channel_number_t channel,
                                 ftdf_pti_t            pti,
                                 ftdf_boolean_t        cmsa_suppress,
                                 void                  *handle);

/**
 * \brief       FTDF_SEND_FRAME_TRANSPARENT_CONFIRM - Confirms a transparent send
 * \param[in]   handle:   The application provided handle of the corresponding ftdf_send_frame_transparent call call
 * \param[in]   status:   Bit mapped status
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when a frame has send successfully or failed to send. The application can
 *              now free the data buffer of the corresponding ftdf_send_frame_transparent call using ftdf_rel_data_buffer
 * \remark      If the frame is send successfully status is equal to 0
 * \remark      If sending the frame failed one of more of the following status bits is set:
 *              - FTDF_TRANSPARENT_CSMACA_FAILURE: Sending failed because CSMA-CA failed
 *              - FTDF_TRANSPARENT_OVERFLOW: Sending failed because of internal resource overflow
 */
void FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(void *handle, ftdf_bitmap32_t status);

/**
 * \brief       ftdf_send_frame_transparent_confirm - Converts the FTDF_SEND_FRAME_TRANSPARENT_CONFIRM to a msg buffer
 *              and then calls ftdf_rcv_msg.
 * \param[in]   handle:   The application provided handle of the corresponding ftdf_send_frame_transparent call
 * \param[in]   status:   Bit mapped status
 * \remark      This function converts the input parameters to a ftdf_transparent_confirm_t structure
 *              and then calls ftdf_rcv_msg.
 * \remark      FTDF_SEND_FRAME_TRANSPARENT_CONFIRM can be defined to be this function.
 */
void ftdf_send_frame_transparent_confirm(void *handle, ftdf_bitmap32_t status);

/**
 * \brief       FTDF_RCV_FRAME_TRANSPARENT - Receive frame transparent
 * \param[in]   frame_length: Length of the received frame
 * \param[in]   frame:       Pointer to received frame.
 * \param[in]   status:      Bitmapped receive status
 * \param[in]   link_quality: LQI/RSSI value for this packet. Depends on FTDF_PIB_LINK_QUALITY_MODE
 *                            PIB parameter.
 *
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when a frame is received when transparent mode is enabled
 * \remark      The buffer with the frame must be copied by the application before this function returns
 * \remark      The following status bits can be set:
 *              - FTDF_TRANSPARENT_RCV_CRC_ERROR: The CRC (FCS) of the received frame is not correct
 *              - FTDF_TRANSPARENT_RCV_RES_FRAMETYPE: Received a reserved frame type
 *              - FTDF_TRANSPARENT_RCV_RES_FRAME_VERSION: Received a reserved frame version
 *              - FTDF_TRANSPARENT_RCV_UNEXP_DST_ADDR: Unexpected destination address
 *              - FTDF_TRANSPARENT_RCV_UNEXP_DST_PAN_ID: Unexpected destionation PAN id
 *              - FTDF_TRANSPARENT_RCV_UNEXP_BEACON: Unexpected beacon frame
 *              - FTDF_TRANSPARENT_RCV_UNEXP_NO_DEST_ADDR: Unexpected frame without destination adres.
 */
void FTDF_RCV_FRAME_TRANSPARENT(ftdf_data_length_t frame_length, ftdf_octet_t *frame,
                                ftdf_bitmap32_t status, ftdf_link_quality_t link_quality);

/**
 * \brief       ftdf_rcv_frame_transparent - Converts the FTDF_RCV_FRAME_TRANSPARENT to a msg buffer
 *              and then calls ftdf_rcv_msg.
 * \param[in]   frame_length: Length of the received frame
 * \param[in]   frame:       Pointer to received frame.
 * \param[in]   status:      Bitmapped receive status
 * \param[in]   link_quality: Received packet link quality (RSSI/LQI)
 * \remark      This function convers the input parameters to a ftdf_transparent_indication_t structure
 *              and then calls ftdf_rcv_msg.
 * \remark      FTDF_RCV_FRAME_TRANSPARENT can be defined to be this function.
 */
void ftdf_rcv_frame_transparent(ftdf_data_length_t frame_length, ftdf_octet_t *frame,
                                ftdf_bitmap32_t status, ftdf_link_quality_t link_quality);

/**
 * \brief       ftdf_set_sleep_attributes - Set sleep atributes
 * \param[in]   low_power_clock_cycle: The LP clock cycle in picoseconds
 * \param[in]   wakeup_latency:      The wakep up latency time in low power clock cycles
 * \remark      This functions sets some attributes required by FTDF to go into a deep sleep and wake up from it.
 * \remark      The wake up latency is the time between the wakeup signal send by FTDF and the moment that the
 *              clocks are stable. It will be used by FTDF to determine when it must must send the wakeup signal.
 * \remark      The low power clock cycle will be used to determine how long the system has slept and recover
 *              the symbol counter internally used by FTDF.
 */
void ftdf_set_sleep_attributes(ftdf_psec_t low_power_clock_cycle,
                               ftdf_nr_low_power_clock_cycles_t wakeup_latency);

/**
 * \brief       ftdf_can_sleep - Indicates whether FTDF can go in a deep sleep
 * \remark      This function returns the time in microseconds that the FTDF can deep sleep.
 * \remark      It does NOT take into account the wakeup latency.
 * \return      The number of microseconds that the FTDF can deep sleep. If equal to 0 the FTDF is still active
 *              and cannot deep sleep
 */
ftdf_usec_t ftdf_can_sleep(void);

/**
 * \brief       ftdf_prepare_for_sleep - Prepare the FTDF for a deep sleep
 * \param[in]   sleep_time: The sleep time. After the sleep time the FTDF must be ready to do next scheduled activity.
 * \remark      When this function is completed the application can safely power off the FTDF and put the uP in
 *              a deep sleep. At least wake up latency time microseconds before sleep time is expired the FTDF
 *              will send a wakeup signal. FTDF expect that the application calls ftdf_wakeup as it has been
 *              woken up.
 * \return      FTDF_TRUE if the FTDF can be put in deep sleep, FTDF_FALSE if the sleep_time is too short to go to
 *              deep sleep and wake up again after sleep_time microseconds, the FTDF cannot be put in deep sleep.
 */
ftdf_boolean_t ftdf_prepare_for_sleep(ftdf_usec_t sleep_time);

/**
 * \brief       ftdf_wakeup - Wake up the FTDF after a sleep
 * \remark      This function will restores the FTDF to state before the deep sleep. It will does this in 2 steps
 *              1. It initialises it internal states and data structures and all the non timing specific HW. After
 *              that it waits until the wake up latency time has been expired. When expired FTDF assumes that the
 *              slocks are stable.
 *              2. It restores timers in HW and calls the function FTDF_WAKE_UP_READY to indicate FTDF is up again,
 *              ready to perform scheduled activities and ready to process new requests from the application.
 */
void ftdf_wakeup(void);

/**
 * \brief       FTDF_WAKE_UP_READY - Indication that FTDF has woke up
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when it is up again, ready to perform scheduled activities and ready to
 *              process new requests from the application.
 */
void FTDF_WAKE_UP_READY(void);

/**
 * \brief       FTDF_SLEEP_CALLBACK - Callback from FTDF_SLEEP_REQUEST
 * \remark      This function must be implemented by the application, it will be called by FTDF.
 *              It must be configured in the configuration file ftdf_config.h
 * \remark      FTDF calls this function when an FTDF_SLEEP_REQUEST message is sent through the ftdf_snd_msg function.
 */
void FTDF_SLEEP_CALLBACK(ftdf_usec_t sleep_time);

void ftdf_reset(int set_default_pib);

ftdf_pti_t ftdf_get_rx_pti(void);


#if dg_configCOEX_ENABLE_CONFIG
void ftdf_set_rx_pti(ftdf_pti_t rx_pti);
#if FTDF_USE_AUTO_PTI
void ftdf_restore_rx_pti(void);
#endif
#endif


#ifdef FTDF_PHY_API
ftdf_status_t ftdf_send_frame_simple(ftdf_data_length_t    frame_length,
                                     ftdf_octet_t          *frame,
                                     ftdf_channel_number_t channel,
                                     ftdf_pti_t            pti,
                                     ftdf_boolean_t        csmaSuppress);

ftdf_pib_attribute_value_t *ftdf_get_value(ftdf_pib_attribute_t pib_attribute);
ftdf_status_t ftdf_set_value(ftdf_pib_attribute_t             pib_attribute,
                             const ftdf_pib_attribute_value_t *pib_attribute_value);

void ftdf_rx_enable(ftdf_time_t rx_on_duration);
#endif /* FTDF_PHY_API */

#if FTDF_DBG_BUS_ENABLE
/**
 * \brief Checks current debug mode and makes appropriate hardware configurations.
 */
void ftdf_check_dbg_mode(void);

/**
 * \brief Sets debug bus mode.
 */
void ftdf_set_dbg_mode(ftdf_dbg_mode_t dbg_mode);

/**
 * \brief Debug bus GPIO configuration callback. Makes appropriate GPIO configurations for the debug
 * bus.
 */
void FTDF_DBG_BUS_GPIO_CONFIG(void);
#endif /* FTDF_DBG_BUS_ENABLE */

#if dg_configUSE_FTDF_DDPHY == 1
void ftdf_ddphy_set(uint16_t cca_reg);
#endif /* dg_configUSE_FTDF_DDPHY == 1 */

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
#ifndef FTDF_LITE
/**
 * \brief Controls when the FPPR address invalidation will be performed.
 *
 * If set to 0 (recommended), address invalidation in FPPR occurs right after the frame is passed to
 * LMAC for direct transmission. If set to 1, address invalidation occurs after the associated
 * confirm message is received.
 */
#define FTDF_FPPR_DEFER_INVALIDATION                    0

/**
 * \brief Adds a new short address to FPPR, initializing the corresponding FSM.
 *
 * A short address FP FSM accepts the following notifications (events):
 *
 * - New address (initialize) by calling @ref ftdf_fp_fsm_short_address_new()
 * - Last frame pending (passed to direct sending) by calling @ref
 * ftdf_fp_fsm_short_address_last_frame_pending()
 * - Last pending frame sent by calling @ref ftdf_fp_fsm_clear_pending()
 *
 * A new address may be rejected if there is no adequate space left in FPPR.
 *
 * \param [in] pan_id Destination PAN ID.
 * \param [in] short_address Destination short address.
 *
 * \return \ref FTDF_TRUE on success.
 */
ftdf_boolean_t ftdf_fp_fsm_short_address_new(ftdf_pan_id_t pan_id, ftdf_short_address_t short_address);

/**
 * \brief Adds a new extended address to FPPR, initializing the corresponding FSM.
 *
 * An extended address FP FSM accepts the following notifications (events):
 *
 * - New address (initialize) by calling @ref ftdf_fp_fsm_rxt_address_new()
 * - Last frame pending (passed to direct sending) by calling @ref
 * ftdf_fp_fsm_ext_address_last_frame_pending()
 * - Last pending frame sent by calling @ref ftdf_fp_fsm_clear_pending()
 *
 * A new address may be rejected if there is no adequate space left in FPPR.
 *
 * \param [in] pan_id Destination PAN ID.
 * \param [in] ext_address Destination extended address.
 *
 * \return \ref FTDF_TRUE on success.
 */
ftdf_boolean_t ftdf_fp_fsm_rxt_address_new(ftdf_pan_id_t pan_id, ftdf_ext_address_t ext_address);

/**
 * \brief Notifies FP FSM that the last packet for the associated destination has been passed for
 * direct transmission.
 *
 * \param [in] pan_id Destination PAN ID.
 * \param [in] short_address Destination short address.
 */
void ftdf_fp_fsm_short_address_last_frame_pending(ftdf_pan_id_t pan_id, ftdf_short_address_t short_address);

/**
 * \brief Notifies FP FSM that the last packet for the associated destination has been passed for
 * direct transmission.
 *
 * \param [in] pan_id Destination PAN ID.
 * \param [in] ext_address Destination extended address.
 */
void ftdf_fp_fsm_ext_address_last_frame_pending(ftdf_pan_id_t pan_id, ftdf_ext_address_t ext_address);

/**
 * \brief Notifies FP FSM that the last pending packet has been transmitted (UMAC state machine
 * closed).
 */
void ftdf_fp_fsm_clear_pending(void);

#endif /* #ifndef FTDF_LITE */

/**
 * \brief Number of FPPR table entries.
 */
#define FTDF_FPPR_TABLE_ENTRIES                         24

/**
 * \brief Number of extended address table entries.
 */
#define FTDF_FPPR_TABLE_EXT_ADDRESS_ENTRIES             FTDF_FPPR_TABLE_ENTRIES

/**
 * \brief Number of short address table entries.
 */
#define FTDF_FPPR_TABLE_SHORT_ADDRESS_ENTRIES           (FTDF_FPPR_TABLE_ENTRIES * 4)

/**
 * \brief Resets FPPR table, invalidating all addresses.
 */
void ftdf_fppr_reset(void);

/**
 * \brief Gets the short address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] short_addr_idx Short address position within the entry (0 to 3).
 *
 * \return Requested short address
 */
ftdf_short_address_t ftdf_fppr_get_short_address(uint8_t entry, uint8_t short_addr_idx);

/**
 * \brief Programs a short address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] short_addr_idx Short address position within the entry (0 to 3).
 * \param [in] short_address short address.
 */
void ftdf_fppr_set_short_address(uint8_t entry, uint8_t short_addr_idx,
                                 ftdf_short_address_t short_address);

/**
 * \brief Determines validity of a short address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] short_addr_idx Short address position within the entry (0 to 3).
 *
 * \return FTDF_TRUE if the address is valid.
 */
ftdf_boolean_t ftdf_fppr_get_short_address_valid(uint8_t entry, uint8_t short_addr_idx);

/**
 * \brief Sets validity of a short address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] short_addr_idx Short address position within the entry (0 to 3).
 * \param [in] valid Associated validity.
 */
void ftdf_fppr_set_short_address_valid(uint8_t entry, uint8_t short_addr_idx,
                                       ftdf_boolean_t valid);

/**
 * \brief Gets the extended address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 *
 * \return Requested extended address
 */
ftdf_ext_address_t ftdf_fppr_get_ext_address(uint8_t entry);

/**
 * \brief Programs an extended address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] ext_address Associated extended address.
 */
void ftdf_fppr_set_ext_address(uint8_t entry, ftdf_ext_address_t ext_address);

/**
 * \brief Determines validity of an extended address programmed at the specified FPPR table
 * position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 *
 * \return \ref FTDF_TRUE if the address is valid.
 */
ftdf_boolean_t ftdf_fppr_get_ext_address_valid(uint8_t entry);

/**
 * \brief Sets validity of an extended address programmed at the specified FPPR table position.
 *
 * \param [in] entry FPPR table entry (0 to \ref FTDF_FPPR_TABLE_ENTRIES - 1).
 * \param [in] valid Associated validity.
 */
void ftdf_fppr_set_ext_address_valid(uint8_t entry, ftdf_boolean_t valid);

/**
 * \brief Gets a free (invalid) short address position in the FPPR table.
 *
 * \param [out] entry Reference to the FPPR table entry.
 * \param [out] short_addr_idx Reference to the short address position within the entry.
 *
 * \return \ref FTDF_TRUE if a free position was found.
 */
ftdf_boolean_t ftdf_fppr_get_free_short_address(uint8_t * entry, uint8_t *short_addr_idx);

/**
 * \brief Gets a free (invalid) extended address position in the FPPR table.
 *
 * \param [out] entry Reference to the FPPR table entry.
 *
 * \return \ref FTDF_TRUE if a free position was found.
 */
ftdf_boolean_t ftdf_fppr_get_free_ext_address(uint8_t * entry);

/**
 * \brief Looks up a short address within the FPPR table.
 *
 * \param [in] shortAddr Associated short address.
 * \param [out] entry Reference to the FPPR table entry.
 * \param [out] short_addr_idx Reference to the short address position within the entry.
 *
 *  * \return \ref FTDF_TRUE if the address was found.
 */
ftdf_boolean_t ftdf_fppr_lookup_short_address(ftdf_short_address_t shortAddr, uint8_t *entry,
                                           uint8_t *short_addr_idx);

/**
 * \brief Looks up an extended address within the FPPR table.
 *
 * \param [in] ext_addr Associated extended address.
 * \param [out] entry Reference to the FPPR table entry.
 *
 *  * \return \ref FTDF_TRUE if the address was found.
 */
ftdf_boolean_t ftdf_fppr_lookup_ext_address(ftdf_ext_address_t ext_addr, uint8_t *entry);

#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

/**
 * \brief Sets FPPR mode.
 *
 * \param [in] match_fp Specifies the value of the FP bit when acknowledging a received frame with a
 * source address that matches a valid a address in the FPPR table. The opposite value is set if the
 * source address does not match. Ignored if \a fp_override is \ref FTDF_TRUE.
 *
 * \param [in] fp_override If \ref FTDF_TRUE, the value of \a fp_force is used for setting the FP bit,
 * regardless of the address table matching result.
 *
 * \param [in] fp_force FP bit value when fp_override is \ref FTDF_TRUE. Ignored otherwise.
 */
void ftdf_fppr_set_mode(ftdf_boolean_t match_fp, ftdf_boolean_t fp_override, ftdf_boolean_t fp_force);

#if FTDF_FP_BIT_TEST_MODE
/**
 * \brief Gets FPPR mode.
 *
 * \param [out] match_fp Reference to the returned value. \see FTDF_fpprRamSetMode
 *
 * \param [out] fp_override Reference to the returned value. \see FTDF_fpprRamSetMode
 *
 * \param [out] fp_force FP Reference to the returned value. \see FTDF_fpprRamSetMode
 */
void ftdf_fppr_get_mode(ftdf_boolean_t *match_fp, ftdf_boolean_t *fp_override, ftdf_boolean_t *fp_force);
#endif //FTDF_FP_BIT_TEST_MODE

#if FTDF_USE_LPDP == 1

/**
 * \brief Enable or disable LPDP.
 *
 * \param [in] enable if FTDF_TRUE, LPDP functionality is enabled.
 */
void ftdf_lpdp_enable(ftdf_boolean_t enable);

#if FTDF_FP_BIT_TEST_MODE
/**
 * \brief Checks whether LPDP is enabled.
 *
 * \return FTDF_TRUE, if LPDP functionality is enabled.
 */
ftdf_boolean_t ftdf_lpdp_is_enabled(void);
#endif

#endif /* #if FTDF_USE_LPDP == 1 */

#endif /* FTDF_H_ */

/**
 * \}
 * \}
 * \}
 */

