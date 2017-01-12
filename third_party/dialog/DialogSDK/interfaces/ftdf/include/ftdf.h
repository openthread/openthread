/**
 \addtogroup INTERFACES
 \{
 \addtogroup RADIO
 \{
 \addtogroup FTDF
 \{
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

typedef uint8_t FTDF_MsgId;
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
typedef uint8_t FTDF_Status;
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
typedef uint8_t FTDF_AssociationStatus;
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
typedef uint8_t FTDF_AddressMode;
#define FTDF_NO_ADDRESS       0
#define FTDF_SIMPLE_ADDRESS   1
#define FTDF_SHORT_ADDRESS    2
#define FTDF_EXTENDED_ADDRESS 3

/**
 * \brief  PAN ID
 * \remark Range 0.. 65535
 */
typedef uint16_t FTDF_PANId;

typedef uint8_t  FTDF_SimpleAddress;

/**
 * \brief  Short address
 * \remark Range 00:00 - ff:ff
 */
typedef uint16_t FTDF_ShortAddress;

/**
 * \brief  Extended address
 * \remark Range 00:00:00:00:00:00:00:00 - ff:ff:ff:ff:ff:ff:ff:ff
 */
typedef uint64_t FTDF_ExtAddress;

typedef union
{
    /** \brief DO NOT USE */
    FTDF_SimpleAddress simpleAddress;
    /** \brief Short address */
    FTDF_ShortAddress  shortAddress;
    /** \brief Extended address */
    FTDF_ExtAddress    extAddress;
} FTDF_Address;

#ifdef SIMULATOR
typedef uint16_t FTDF_DataLength;
#else
typedef uint8_t  FTDF_DataLength;
#endif

/**
 * \brief  Handle
 * \remark Range 0..255
 */
typedef uint8_t FTDF_Handle;

/**
 * \brief  Boolean
 * \remark Valid values:
 *         - FTDF_FALSE
 *         - FTDF_TRUE
 */
typedef uint8_t FTDF_Boolean;
#define FTDF_FALSE 0
#define FTDF_TRUE  1

/**
 * \brief  Security level
 * \remark Valid range 0..7
 */
typedef uint8_t  FTDF_SecurityLevel;

/**
 * \brief  Security key ID mode
 * \remark Valid range 0..3
 */
typedef uint8_t  FTDF_KeyIdMode;

/**
 * \brief  Security key index
 * \remark Valid range 0..255
 */
typedef uint8_t  FTDF_KeyIndex;

/**
 * \brief  Sequence number
 * \remark Valid range 0..255
 */
typedef uint8_t  FTDF_SN;

/**
 * \brief  Sequence number
 * \remark Valid range 0..0xffffffffff (5 octets)
 */
typedef uint64_t FTDF_ASN;

/**
 * \brief  Order
 */
typedef uint8_t  FTDF_Order;

/**
 * \brief  Time
 * \remark Timestamp in symbols which wraps around every 0x100000000 symbols
 */
typedef uint32_t FTDF_Time;

/**
 * \brief  Real time
 * \remark Timestamp in symbols which does not wrap (in the lifetime of the device)
 */
typedef uint64_t FTDF_Time64;

typedef uint32_t FTDF_USec;

typedef uint64_t FTDF_PSec;

typedef uint32_t FTDF_NrLowPowerClockCycles;

/**
 * \brief  Period of time
 * \remark Range 0..65535
 */
typedef uint16_t FTDF_Period;

/**
 * \brief  Size type
 */
typedef uint8_t  FTDF_Size;

/**
 * \brief  Length type
 */
typedef uint8_t  FTDF_Length;

/**
 * \brief  Priority type
 */
typedef uint8_t  FTDF_Priority;

/**
 * \brief  Performance counter
 * \remark Range 0..0xffffffff
 */
typedef uint32_t FTDF_Count;

/**
 * \brief Disassociate reason
 * \remark Allowed values:
 *         - FTDF_COORD_WISH_DEVICE_LEAVE_PAN: Coordinator wishes that device leaves PAN
 *         - FTDF_DEVICE_WISH_LEAVE_PAN: Device wishes to leave PAN
 */
typedef uint8_t  FTDF_DisassociateReason;
#define FTDF_COORD_WISH_DEVICE_LEAVE_PAN 1
#define FTDF_DEVICE_WISH_LEAVE_PAN       2

/**
 * \brief Beacon type
 * \remark Allowed values:
 *         - FTDF_NORMAL_BEACON: Normal beacon
 *         - FTDF_ENHANCED_BEACON: Enhanced beacon
 */
typedef uint8_t FTDF_BeaconType;
#define FTDF_NORMAL_BEACON   0
#define FTDF_ENHANCED_BEACON 1

// MAC constants
/** \brief aBaseSlotDuration */
#define FTDF_BASE_SLOT_DURATION          60
/** \brief aNumSuperframeSlots */
#define FTDF_NUM_SUPERFRAME_SLOTS        16
/** \brief aBaseSuperframeDuration */
#define FTDF_BASE_SUPERFRAME_DURATION    ( FTDF_BASE_SLOT_DURATION * FTDF_NUM_SUPERFRAME_SLOTS )
/** \brief NOT USED */
#define FTDF_GTS_PERSISTENCE_TIME        4
/** \brief aMaxBeaconOverhead */
#define FTDF_MAX_BEACON_OVERHEAD         75
/** \brief aMaxBeaconOverhead */
#define FTDF_MAX_PHY_PACKET_SIZE         127
/** \brief aMaxPHYPacketSize */
#define FTDF_MAX_BEACON_PAYLOAD_LENGTH   ( FTDF_MAX_PHY_PACKET_SIZE - FTDF_MAX_BEACON_OVERHEAD )
/** \brief NOT USED  */
#define FTDF_MAX_LOST_BEACONS            4
/** \brief aMaxMPDUUnsecuredOverhead */
#define FTDF_MAX_MPDU_UNSECURED_OVERHEAD 25
/** \brief aMinMPDUOverhead */
#define FTDF_MIN_MPDU_OVERHEAD           9
/** \brief aMaxMACSafePayloadSize */
#define FTDF_MAX_MAC_SAFE_PAYLOAD_SIZE   ( FTDF_MAX_PHY_PACKET_SIZE - FTDF_MAX_MPDU_UNSECURED_OVERHEAD )
/** \brief aMaxMACSafePayloadSize */
#define FTDF_MAX_PAYLOAD_SIZE            ( FTDF_MAX_PHY_PACKET_SIZE - FTDF_MIN_MPDU_OVERHEAD )
/** \brief aMaxMACPayloadSize */
#define FTDF_MAX_SIFS_FRAME_SIZE         18
/** \brief NOT USED  */
#define FTDF_MIN_CAP_LENGTH              440
/** \brief aUnitBackoffPeriod */
#define FTDF_UNIT_BACKOFF_PERIOD         20

/**
 * \brief PIB attribute ID
 * \remark List of supported PIB attributes, their types/structures and description
 * - FTDF_PIB_EXTENDED_ADDRESS, \link FTDF_ExtAddress \endlink,
 *         The extended address of the device
 * - FTDF_PIB_ACK_WAIT_DURATION, \link FTDF_Period \endlink, The maximum time in symbols that is waited for an ack, Read only
 * - FTDF_PIB_ASSOCIATION_PAN_COORD, \link FTDF_Boolean \endlink, Indication whether the device is associated with a PAN coordinator
 * - FTDF_PIB_ASSOCIATION_PERMIT, \link  FTDF_Boolean \endlink, Indication whether the PAN coordinator grats association requests
 * - FTDF_PIB_AUTO_REQUEST, \link FTDF_Boolean \endlink, Indication whether beacon received while scanning are forwarded to the application or not
 * - FTDF_PIB_BEACON_PAYLOAD, FTDF_Octet*, Pointer to the data to be included in a beacon as payload
 * - FTDF_PIB_BEACON_PAYLOAD_LENGTH, \link FTDF_Size \endlink, Size of the data to be included in a beacon as payload
 * - FTDF_PIB_BEACON_ORDER, \link FTDF_Order \endlink, Fixed to 15 indicating beaconless mode
 * - FTDF_PIB_BSN, \link FTDF_SN \endlink, The current beacon sequence number
 * - FTDF_PIB_COORD_EXTENDED_ADDRESS, \link FTDF_ExtAddress \endlink, The extended address of the coordinator
 * - FTDF_PIB_COORD_SHORT_ADDRESS, \link FTDF_ShortAddress \endlink, The short address of the coordinator
 * - FTDF_PIB_DSN, \link FTDF_SN \endlink, The current data sequence number
 * - FTDF_PIB_MAX_BE, \link FTDF_BEExponent \endlink, The maximum backoff exponent
 * - FTDF_PIB_MAX_CSMA_BACKOFFS, \link FTDF_Size \endlink, The maximum number of backoffs
 * - FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME, \link FTDF_Period \endlink, The maximum time in symbols that RX is ON after that a frame is received with FP set.
 * - FTDF_PIB_MAX_FRAME_RETRIES, \link FTDF_Size \endlink, The maximum number of frame retries
 * - FTDF_PIB_MIN_BE, \link FTDF_BEExponent \endlink, The minimum backoff exponent
 * - FTDF_PIB_LIFS_PERIOD, \link FTDF_Period \endlink, The long IFS period in number of symbols, Read oonly
 * - FTDF_PIB_SIFS_PERIOD, \link FTDF_Period \endlink, The short IFS period in number of symbols, Read oonly
 * - FTDF_PIB_PAN_ID, \link FTDF_PANId \endlink, The PAN ID of the device
 * - FTDF_PIB_PROMISCUOUS_MODE, \link FTDF_Boolean \endlink, Indication whether the promiscuous is enable or not
 * - FTDF_PIB_RESPONSE_WAIT_TIME, \link FTDF_Size \endlink, The maximum time in aBaseSuperFramePeriod's (960 symbols) that is waited for a command frame response
 * - FTDF_PIB_RX_ON_WHEN_IDLE, \link FTDF_Boolean \endlink, Indication whether the receiver must be on when idle
 * - FTDF_PIB_SECURITY_ENABLED, \link FTDF_Boolean \endlink, Indication whether the security is enabled
 * - FTDF_PIB_SHORT_ADDRESS, \link FTDF_ShortAddress \endlink, The short address of the device
 * - FTDF_PIB_SYNC_SYMBOL_OFFSET, \link FTDF_Period \endlink, The offset in symbols between the start of frame and that the timestamp is taken
 * - FTDF_PIB_TIMESTAMP_SUPPORTED, \link FTDF_Boolean \endlink, Indication whether timestamping is supported
 * - FTDF_PIB_TRANSACTION_PERSISTENCE_TIME, \link FTDF_Period \endlink, The maximum time in aBaseSuperFramePeriod's (960 symbols) that indirect data requests are queued
 * - FTDF_PIB_ENH_ACK_WAIT_DURATION, \link FTDF_Period \endlink, The maximum time in symbols that is waited for an enhanced ack
 * - FTDF_PIB_IMPLICIT_BROADCAST, \link FTDF_Boolean \endlink, Indication whether frames without a destination PAN are treated as broadcasts
 * - FTDF_PIB_DISCONNECT_TIME, \link FTDF_Period \endlink, The time in timeslots that disassoctate frames are send before disconnecting
 * - FTDF_PIB_JOIN_PRIORITY, \link FTDF_Priority \endlink, The join priority
 * - FTDF_PIB_ASN, \link FTDF_ASN \endlink, The current ASN
 * - FTDF_PIB_SLOTFRAME_TABLE, \link FTDF_SlotframeTable \endlink, The slotframe table, Read only
 * - FTDF_PIB_LINK_TABLE, \link FTDF_LinkTable \endlink, The link table, Read only
 * - FTDF_PIB_TIMESLOT_TEMPLATE, \link FTDF_TimeslotTemplate \endlink, The current timeslot template
 * - FTDF_PIB_HOPPINGSEQUENCE_ID, \link FTDF_HoppingSequenceId \endlink, The ID of the current hopping sequence
 * - FTDF_PIB_CHANNEL_PAGE, \link FTDF_ChannelPage \endlink, The current channel page
 * - FTDF_PIB_HOPPING_SEQUENCE_LENGTH, \link FTDF_Length \endlink, Length of the current hopping sequence
 * - FTDF_PIB_HOPPING_SEQUENCE_LIST, \link FTDF_ChannelNumber \endlink, Hopping sequence
 * - FTDF_PIB_CURRENT_HOP, \link FTDF_Length \endlink, The current hop
 * - FTDF_PIB_CSL_PERIOD, \link FTDF_Period \endlink, The CSL sample period in units of 10 symbols
 * - FTDF_PIB_CSL_MAX_PERIOD, \link FTDF_Period \endlink, The maximum CSL sample period of devices in the PAN in units of 10 symbols
 * - FTDF_PIB_CSL_CHANNEL_MASK, \link FTDF_Bitmap32 \endlink, Bitmapped list of channels to be sample in CSL mode
 * - FTDF_PIB_CSL_FRAME_PENDING_WAIT_T, \link FTDF_Period \endlink, The maximum time in symbols that RX is ON after that a frame is received with FP set in CSL mode.
 * - FTDF_PIB_PERFORMANCE_METRICS, \link FTDF_PerformanceMetrics \endlink, The performance metrics, Read only
 * - FTDF_PIB_EB_IE_LIST, \link FTDF_IEList \endlink, Payload IE list to be added to an enhanced beacon
 * - FTDF_PIB_EBSN, \link FTDF_SN \endlink, Current enhanced beacon sequence number
 * - FTDF_PIB_EB_AUTO_SA, \link FTDF_AutoSA \endlink, Source address mode of auto generated enhanced beacons
 * - FTDF_PIB_EACK_IE_LIST, \link FTDF_IEList \endlink, Payload IE list to be added to an enhanced ack
 * - FTDF_PIB_KEY_TABLE, \link FTDF_KeyTable \endlink, Security key table
 * - FTDF_PIB_DEVICE_TABLE, \link FTDF_DeviceTable \endlink, Security device table
 * - FTDF_PIB_SECURITY_LEVEL_TABLE, \link FTDF_SecurityLevelTable \endlink, Security level table
 * - FTDF_PIB_FRAME_COUNTER, \link FTDF_FrameCounter \endlink, Current security frame counter
 * - FTDF_PIB_MT_DATA_SECURITY_LEVEL, \link FTDF_SecurityLevel \endlink, Security level for auto generated data frames
 * - FTDF_PIB_MT_DATA_KEY_ID_MODE, \link FTDF_KeyIdMode \endlink, Security key ID mode for auto generated data frames
 * - FTDF_PIB_MT_DATA_KEY_SOURCE, FTDF_Octet[ 8 ], Security key source for auto generated data frames
 * - FTDF_PIB_MT_DATA_KEY_INDEX, \link FTDF_KeyIndex \endlink, Security key index for auto generated data frames
 * - FTDF_PIB_DEFAULT_KEY_SOURCE, FTDF_Octet[ 8 ], Default key source
 * - FTDF_PIB_FRAME_COUNTER_MODE, \link FTDF_FrameCounterMode \endlink, Security frame counter mode
 * - FTDF_PIB_CSL_SYNC_TX_MARGIN, \link FTDF_Period \endlink, The margin in unit of 10 symbols used by FTDF in CSL mode in case of a synchronised transmission
 * - FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO, \link FTDF_Period \endlink, The time in unit of 10 symbols after which FTDF discard the remote synchronisation info.
 * - FTDF_PIB_TSCH_ENABLED, \link FTDF_Boolean \endlink, Indicates whether the TSCH mode is enabled or not, Read only
 * - FTDF_PIB_LE_ENABLED, \link FTDF_Boolean \endlink, Indicates whether the CSL mode is enabled or not
 * - FTDF_PIB_CURRENT_CHANNEL, \link FTDF_ChannelNumber \endlink, The current channel used by FTDF
 * - FTDF_PIB_CHANNELS_SUPPORTED, \link FTDF_ChannelDescriptorList \endlink, List of channels supported by FTDF
 * - FTDF_PIB_TX_POWER_TOLERANCE, \link FTDF_TXPowerTolerance \endlink, TX power tolerance
 * - FTDF_PIB_TX_POWER, \link FTDF_DBm \endlink, TX power
 * - FTDF_PIB_CCA_MODE, \link FTDF_CCAMode \endlink, CCA mode
 * - FTDF_PIB_CURRENT_PAGE, \link FTDF_ChannelPage \endlink, Current channel page
 * - FTDF_PIB_MAX_FRAME_DURATION, \link FTDF_Period \endlink, The maximum number of symbols in frame, Read only
 * - FTDF_PIB_SHR_DURATION, \link FTDF_Period \endlink, Synchronisation header length in symbols, Read only
 * - FTDF_PIB_TRAFFIC_COUNTERS, \link FTDF_TrafficCounters \endlink, Miscelaneous traffic counters, Read only
 * - FTDF_PIB_LE_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports LE (CSL), Read only
 * - FTDF_PIB_LL_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports LL, Read only
 * - FTDF_PIB_DSME_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports DSME, Read only
 * - FTDF_PIB_RFID_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports RFID, Read only
 * - FTDF_PIB_AMCA_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports AMCA, Read only
 * - FTDF_PIB_TSCH_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports FTDF, Read only
 * - FTDF_PIB_METRICS_CAPABLE, \link FTDF_Boolean \endlink, Indicates whether FTDF supports metrics, Read only
 * - FTDF_PIB_RANGING_SUPPORTED, \link FTDF_Boolean \endlink, Indicates whether FTDF supports ranging, Read only
 * - FTDF_PIB_KEEP_PHY_ENABLED, \link FTDF_Boolean \endlink, Indicates whether the PHY must be kept enabled
 * - FTDF_PIB_METRICS_ENABLED, \link FTDF_Boolean \endlink, Indicates whether metrics are enabled or not
 * - FTDF_PIB_BEACON_AUTO_RESPOND, \link FTDF_Boolean \endlink, Indicates whether FTDF must send automatically a beacon at a BECAON_REQUEST command frame or must forward the request to the application using FTDF_BEACON_REQUEST_INDICATION.
 * - FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD, \link FTDF_Period \endlink, The minimum TSCH slot sync offset in microseconds before resyncing
 */
typedef uint8_t FTDF_PIBAttribute;
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

// Total number of PIB attributes
#define FTDF_NR_OF_PIB_ATTRIBUTES        104

#if FTDF_DBG_BUS_ENABLE
/**
 * \brief Debug bus mode.
 *
 */
typedef uint8_t FTDF_DbgMode;

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
 *                      1 Multipurpose frame with no destination PAN ID and macImplicitBroadcast is
 *                        false and not PAN coordinator dropped
 *                      2 Multipurpose frame with no destination address and macImplicitBroadcast is
 *                        false and not PAN coordinator dropped
 *                      3 RX unsupported frame type detected
 *                      4 RX non Beacon frame detected and dropped in RxBeaconOnly mode
 *                      5 RX frame passed due to macAlwaysPassFrmType set
 *                      6 RX unsupported da_mode = 01 for frame_version 0x0- detected
 *                      7 RX unsupported sa_mode = 01 for frame_version 0x0- detected
 *                      8 Data or command frame with no destination PAN ID and macImplicitBroadcast
 *                        is false and not PAN coordinator dropped
 *                      9 Data or command frame with no destination address and macImplicitBroadcast
 *                        is false and not PAN coordinator dropped
 *                      10 RX unsupported frame_version detected
 *                      11 Multipurpose frame with no destination PAN ID and macImplicitBroadcast is
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

typedef void     FTDF_PIBAttributeValue;

/**
 * \brief  Link quality
 * \remark Range: 0..255
 * \remark A higher number is a higher quality
 */
typedef uint8_t  FTDF_LinkQuality;

/**
 * \brief Hopping sequence ID
 */
typedef uint16_t FTDF_HoppingSequenceId;

/**
 * \brief  Channel number
 * \remark Supported range: 11..27
 */
typedef uint8_t  FTDF_ChannelNumber;

/**
 * \brief  Packet Traffic Information (PTI) used to classify a transaction in the radio arbiter.
 */
typedef uint8_t  FTDF_PTI;

/**
 * \brief  Channel page
 * \remark Supported values: 0
 */
typedef uint8_t  FTDF_ChannelPage;

typedef uint8_t  FTDF_ChannelOffset;

typedef uint8_t  FTDF_Octet;

/**
 * \brief 8-bit type bitmap
 */
typedef uint8_t  FTDF_Bitmap8;

/**
 * \brief 16-bit type bitmap
 */
typedef uint16_t FTDF_Bitmap16;

/**
 * \brief 32-bit type bitmap
 */
typedef uint32_t FTDF_Bitmap32;

typedef uint8_t  FTDF_NumOfBackoffs;

typedef uint8_t  FTDF_GTSCharateristics;

/**
 * \brief Scan type
 * \remark Allowed values:
 *         - FTDF_ED_SCAN: Energy detect scan
 *         - FTDF_ACTIVE_SCAN: Active scan
 *         - FTDF_PASSIVE_SCAN: Passive scan
 *         - FTDF_ORPHAN_SCAN: Orphan scan
 *         - FTDF_ENHANCED_ACTIVE_SCAN: Enhanced active scan
 */
typedef uint8_t  FTDF_ScanType;
#define FTDF_ED_SCAN              1
#define FTDF_ACTIVE_SCAN          2
#define FTDF_PASSIVE_SCAN         3
#define FTDF_ORPHAN_SCAN          4
#define FTDF_ENHANCED_ACTIVE_SCAN 7

typedef uint8_t FTDF_ScanDuration;

/**
 * \brief Loss operation
 * \remark Possible values:
 *         - FTDF_PAN_ID_CONFLICT
 *         - FTDF_REALIGNMENT
 *         - FTDF_BEACON_LOST (not supported because only beaconless mode is supported)
 */
typedef uint8_t FTDF_LossReason;
#define FTDF_PAN_ID_CONFLICT 1
#define FTDF_REALIGNMENT     2
#define FTDF_BEACON_LOST     3

typedef uint16_t FTDF_SlotframeSize;

/**
 * \brief Set operation
 * \remark Allowed values:
 *         - FTDF_ADD: Add an entry
 *         - FTDF_DELETE: Delete an entry
 *         - FTDF_MODIFY: Modify an entry
 */
typedef uint8_t  FTDF_Operation;
#define FTDF_ADD    0
#define FTDF_DELETE 2
#define FTDF_MODIFY 3

typedef uint16_t FTDF_Timeslot;

/**
 * \brief Set operation
 * \remark Allowed values:
 *         - FTDF_NORMAL_LINK
 *         - FTDF_ADVERTISING_LINK
 */
typedef uint8_t  FTDF_LinkType;
#define FTDF_NORMAL_LINK      0
#define FTDF_ADVERTISING_LINK 1

/**
 * \brief TSCH mode status
 * \remark Allowed values:
 *         - FTDF_TSCH_OFF
 *         - FTDF_TSCH_ON
 */
typedef uint8_t FTDF_TSCHMode;
#define FTDF_TSCH_OFF 0
#define FTDF_TSCH_ON  1

typedef uint16_t FTDF_KeepAlivePeriod;

/**
 * \brief BE exponent
 */
typedef uint8_t  FTDF_BEExponent;

#define FTDF_MAX_SLOTFRAMES 4

typedef struct
{
    /** \brief Handle of this slotframe */
    FTDF_Handle        slotframeHandle;
    /** \brief The number of timeslots in this slotframe */
    FTDF_SlotframeSize slotframeSize;
} FTDF_SlotframeEntry;

typedef struct
{
    /** \brief Number of slotframes in the slotframe table */
    FTDF_Size            nrOfSlotframes;
    /** \brief Pointer to the first slotframe entry */
    FTDF_SlotframeEntry* slotframeEntries;
} FTDF_SlotframeTable;

#define FTDF_MAX_LINKS                16

#define FTDF_LINK_OPTION_TRANSMIT     0x01
#define FTDF_LINK_OPTION_RECEIVE      0x02
#define FTDF_LINK_OPTION_SHARED       0x04
#define FTDF_LINK_OPTION_TIME_KEEPING 0x08

typedef struct
{
    /** \brief Handle of this link, used for delete and modify operations */
    FTDF_Handle        linkHandle;
    /**
     * \brief Bit mapped link options, the following options are supported:<br>
     *        FTDF_LINK_OPTION_TRANSMIT: Lik can be used for transmitting<br>
     *        FTDF_LINK_OPTION_RECEIVE: Link can be used for receiving<br>
     *        FTDF_LINK_OPTION_SHARED: Shared link, multiple device can transmit in this link<br>
     *        FTDF_LINK_OPTION_TIME_KEEPING: Link will be used to synchronise timeslots<br>
     *        Multiple options can be defined by or-ing them.
     */
    FTDF_Bitmap8       linkOptions;
    /** \brief Link type */
    FTDF_LinkType      linkType;
    /** \brief Handle of the slotframe of this link */
    FTDF_Handle        slotframeHandle;
    /** \brief Short address of the peer node, only valid if is a transmit link */
    FTDF_ShortAddress  nodeAddress;
    /** \brief Timeslot in the slotframe */
    FTDF_Timeslot      timeslot;
    /** \brief Channel offset */
    FTDF_ChannelOffset channelOffset;
    /** \brief DO NOT USE */
    void*              __private1;
    /** \brief DO NOT USE */
    uint64_t           __private2;
} FTDF_LinkEntry;

typedef struct
{
    /** \brief Number of links in the link table */
    FTDF_Size       nrOfLinks;
    /** \brief Pointer to the first link entry */
    FTDF_LinkEntry* linkEntries;
} FTDF_LinkTable;

typedef struct
{
    FTDF_Handle timeslotTemplateId;
    FTDF_Period tsCCAOffset;
    FTDF_Period tsCCA;
    FTDF_Period tsTxOffset;
    FTDF_Period tsRxOffset;
    FTDF_Period tsRxAckDelay;
    FTDF_Period tsTxAckDelay;
    FTDF_Period tsRxWait;
    FTDF_Period tsAckWait;
    FTDF_Period tsRxTx;
    FTDF_Period tsMaxAck;
    FTDF_Period tsMaxTs;
    FTDF_Period tsTimeslotLength;
} FTDF_TimeslotTemplate;

typedef struct
{
    FTDF_Size  counterOctets;
    FTDF_Count retryCount;
    FTDF_Count multipleRetryCount;
    FTDF_Count TXFailCount;
    FTDF_Count TXSuccessCount;
    FTDF_Count FCSErrorCount;
    FTDF_Count securityFailureCount;
    FTDF_Count duplicateFrameCount;
    FTDF_Count RXSuccessCount;
    FTDF_Count NACKCount;
} FTDF_PerformanceMetrics;

typedef struct
{
    FTDF_Count txDataFrmCnt;
    FTDF_Count txCmdFrmCnt;
    FTDF_Count txStdAckFrmCnt;
    FTDF_Count txEnhAckFrmCnt;
    FTDF_Count txBeaconFrmCnt;
    FTDF_Count txMultiPurpFrmCnt;
    FTDF_Count rxDataFrmOkCnt;
    FTDF_Count rxCmdFrmOkCnt;
    FTDF_Count rxStdAckFrmOkCnt;
    FTDF_Count rxEnhAckFrmOkCnt;
    FTDF_Count rxBeaconFrmOkCnt;
    FTDF_Count rxMultiPurpFrmOkCnt;
} FTDF_TrafficCounters;

typedef uint8_t FTDF_Energy;

typedef struct
{
    /** \brief Coordinator address mode */
    FTDF_AddressMode   coordAddrMode;
    /** \brief Coordinator PAN ID */
    FTDF_PANId         coordPANId;
    /** \brief Coordinator address */
    FTDF_Address       coordAddr;
    /** \brief Channel number */
    FTDF_ChannelNumber channelNumber;
    /** \brief Channnel page */
    FTDF_ChannelPage   channelPage;
    /** \brief NOT USED */
    FTDF_Bitmap16      superframeSpec;
    /** \brief NOT USED */
    FTDF_Boolean       GTSPermit;
    /** \brief Link quality */
    FTDF_LinkQuality   linkQuality;
    /** \brief */
    FTDF_Time          timestamp;
} FTDF_PANDescriptor;

typedef struct
{
    /** \brief Coordinator PAN ID */
    FTDF_PANId         coordPANId;
    /** \brief Coordinator short address */
    FTDF_ShortAddress  coordShortAddr;
    /** \brief Channel number */
    FTDF_ChannelNumber channelNumber;
    /** \brief Device short address */
    FTDF_ShortAddress  shortAddr;
    /** \brief Channnel page */
    FTDF_ChannelPage   channelPage;
} FTDF_CoordRealignDescriptor;

/**
 * \brief  Auto frame source address mode
 * \remark Valid values:
 * - FTDF_AUTO_NONE: No address
 * - FTDF_AUTO_SHORT: Short address
 * - FTDF_AUTO_FULL: Extended address
 */
typedef uint8_t FTDF_AutoSA;
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
typedef uint8_t FTDF_TXPowerTolerance;
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
typedef uint8_t FTDF_CCAMode;
#define FTDF_CCA_MODE_1 1
#define FTDF_CCA_MODE_2 2
#define FTDF_CCA_MODE_3 3

/**
 * \brief  Frame control options
 * \remark Supported options:
 * - FTDF_PAN_ID_PRESENT:    Controls the PAN ID compression/present bit in multipurpose and version 2 frames 
 * - FTDF_IES_INCLUDED:      Indicates that IEs must be included
 * - FTDF_SEQ_NR_SUPPRESSED: Controls suppression of the sequence number
 * \remark Multiple options can specified by bit wise or-ing them
 */
typedef uint8_t FTDF_FrameControlOptions;
#define FTDF_PAN_ID_PRESENT    0x01
#define FTDF_IES_INCLUDED      0x02
#define FTDF_SEQ_NR_SUPPRESSED 0x04

typedef struct
{
    FTDF_ChannelPage    channelPage;
    FTDF_Size           nrOfChannels;
    FTDF_ChannelNumber* channels;
} FTDF_ChannelDescriptor;

typedef struct
{
    FTDF_Size               nrOfChannelDescriptors;
    FTDF_ChannelDescriptor* channelDescriptors;
} FTDF_ChannelDescriptorList;

/**
 * \brief Dbm data type
 */
typedef int8_t  FTDF_DBm;

typedef uint8_t FTDF_FrameType;
#define FTDF_BEACON_FRAME          0
#define FTDF_DATA_FRAME            1
#define FTDF_ACKNOWLEDGEMENT_FRAME 2
#define FTDF_MAC_COMMAND_FRAME     3
#define FTDF_LLDN_FRAME            4
#define FTDF_MULTIPURPOSE_FRAME    5

/**
 * \brief  Command frame ID
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
typedef uint8_t FTDF_CommandFrameId;
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
typedef uint64_t FTDF_FrameCounter;

/**
 * \brief Security frame counter mode
 * \remark Valid range 4..5
 */
typedef uint8_t  FTDF_FrameCounterMode;

typedef struct
{
    FTDF_KeyIdMode   keyIdMode;
    FTDF_Octet       keySource[ 8 ];
    FTDF_KeyIndex    keyIndex;
    FTDF_AddressMode deviceAddrMode;
    FTDF_PANId       devicePANId;
    FTDF_Address     deviceAddress;
} FTDF_KeyIdLookupDescriptor;

typedef struct
{
    FTDF_PANId        PANId;
    FTDF_ShortAddress shortAddress;
    FTDF_ExtAddress   extAddress;
    FTDF_FrameCounter frameCounter;
    FTDF_Boolean      exempt;
} FTDF_DeviceDescriptor;

/**
 * \brief  Device descriptor handle
 * \remark This is the index of a device in \link FTDF_DeviceTable \endlink
 */
typedef uint8_t FTDF_DeviceDescriptorHandle;

typedef struct
{
    /** \brief Frame type */
    FTDF_FrameType      frameType;
    /** \brief Command frame ID */
    FTDF_CommandFrameId commandFrameId;
} FTDF_KeyUsageDescriptor;

typedef struct
{
    FTDF_Size                    nrOfKeyIdLookupDescriptors;
    FTDF_KeyIdLookupDescriptor*  keyIdLookupDescriptors;
    FTDF_Size                    nrOfDeviceDescriptorHandles;
    FTDF_DeviceDescriptorHandle* deviceDescriptorHandles;
    FTDF_Size                    nrOfKeyUsageDescriptors;
    FTDF_KeyUsageDescriptor*     keyUsageDescriptors;
    FTDF_Octet                   key[ 16 ];
} FTDF_KeyDescriptor;

typedef struct
{
    FTDF_FrameType      frameType;
    FTDF_CommandFrameId commandFrameId;
    FTDF_SecurityLevel  securityMinimum;
    FTDF_Boolean        deviceOverrideSecurityMinimum;
    FTDF_Bitmap8        allowedSecurityLevels;
} FTDF_SecurityLevelDescriptor;

/**
 * \brief  Key table
 * \remark As followes an example how to initialize a key table for usage with FTDF_SET_REQUEST:
 * \code
 * FTDF_KeyIdLookupDescriptor  lookupDescriptorsKey1[ ] = 
 *          { { 2, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }, 7, 0, 0 0 },
 *            { 0, { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 2, 0x1234, 0x0001 } };
 * FTDF_KeyIdLookupDescriptor  lookupDescriptorsKey2[ ] = 
 *          { { 1, { 0, 0, 0, 0, 0, 0, 0, 0 }, 5, 0, 0, 0 } };
 * FTDF_DeviceDescriptorHandle deviceDescriptorHandlesKey1[ ] = { 0, 2 };
 * FTDF_DeviceDescriptorHandle deviceDescriptorHandlesKey2[ ] = { 1 };
 * FTDF_KeyUsageDescriptor     keyUsageDescriptorKey1[ ] = { { 1, 0 }, { 3, 4 } };
 * FTDF_KeyUsageDescriptor     keyUsageDescriptorKey2[ ] = { { 1, 0 } };
 * FTDF_KeyDescriptor          keyDescriptors[ ] = 
 *          { { sizeof( lookupDescriptorsKey1 ) / sizeof( FTDF_KeyIdLookupDescriptor ),
 *              lookupDescriptorsKey1,
 *              sizeof( deviceDescriptorHandlesKey1 ) / sizeof( FTDF_DeviceDescriptorHandle ),
 *              deviceDescriptorHandlesKey1,
 *              sizeof( keyUsageDescriptorKey1 ) / sizeof( FTDF_KeyUsageDescriptor ),
 *              keyUsageDescriptorKey1,
 *              { 0x7e, 0x37, 0x2d, 0x41, 0xdf, 0x74, 0x72, 0x3f, 0x4c, 0x3a, 0xa7, 0x53, 0xa1, 0x11, 0x46, 0x8b } },
 *            { sizeof( lookupDescriptorsKey2 ) / sizeof( FTDF_KeyIdLookupDescriptor ),
 *              lookupDescriptorsKey2,
 *              sizeof( deviceDescriptorHandlesKey2 ) / sizeof( FTDF_DeviceDescriptorHandle ),
 *              deviceDescriptorHandlesKey2,
 *              sizeof( keyUsageDescriptorKey2 ) / sizeof( FTDF_KeyUsageDescriptor ),
 *              keyUsageDescriptorKey1,
 *              { 0xdf, 0x74, 0x72, 0x3f, 0x7e, 0x37, 0x2d, 0x41, 0x4c, 0x3a, 0xa1, 0x11, 0x46, 0x8, 0xa7, 0x53b } } };
 * FTDF_KeyTable               keyTable =
 *           { sizeof( keyDescriptors ) / sizeof( FTDF_KeyDescriptor ), keyDescriptors };
 * \endcode
 */
typedef struct
{
    /** \brief Number of key descriptors in the key table */
    FTDF_Size           nrOfKeyDescriptors;
    /** \brief Pointer to the first key descriptor entry */
    FTDF_KeyDescriptor* keyDescriptors;
} FTDF_KeyTable;

typedef struct
{
    /** \brief Number of device descriptors in the key table */
    FTDF_Size              nrOfDeviceDescriptors;
    /** \brief Pointer to the first device descriptor entry */
    FTDF_DeviceDescriptor* deviceDescriptors;
} FTDF_DeviceTable;

typedef struct
{
    /** \brief Number of security level descriptors in the key table */
    FTDF_Size                     nrOfSecurityLevelDescriptors;
    /** \brief Pointer to the first security level descriptor entry */
    FTDF_SecurityLevelDescriptor* securityLevelDescriptors;
} FTDF_SecurityLevelTable;

/**
 * \brief  IE type
 * \remark Supported IE types:
 *         - FTDF_SHORT_IE
 *         - FTDF_LONG_IE
 */
typedef uint8_t FTDF_IEType;
#define FTDF_SHORT_IE 0
#define FTDF_LONG_IE  1

typedef uint8_t FTDF_IEId;

typedef uint8_t FTDF_IELength;

typedef struct
{
    /** \brief IE type */
    FTDF_IEType   type;
    /** \brief Sub IE ID*/
    FTDF_IEId     subID;
    /** \brief Sub IE length */
    FTDF_IELength length;
    /** \brief Pointer to sub IE content */
    FTDF_Octet*   subContent;
} FTDF_SubIEDescriptor;

typedef struct
{
    /** \brief Number of sub IE descriptors in an MLME payload IE (ID = 1) */
    FTDF_Size             nrOfSubIEs;
    /** \brief Pointer to the first sub IE descriptor */
    FTDF_SubIEDescriptor* subIEs;
} FTDF_SubIEList;

typedef union
{
    /** \brief IE content if not a MLME payload IE (ID = 1) */
    FTDF_Octet*     raw;
    /** \brief IE content if a MLME payload IE (ID = 1) */
    FTDF_SubIEList* nested;
} FTDF_IEContent;

typedef struct
{
    /** \brief IE ID */
    FTDF_IEId      ID;
    /** \brief Content length of the IE, undefined/unused in case of a MLME payload IE (ID = 1) */
    FTDF_IELength  length;
    /** \brief Content of the IE */
    FTDF_IEContent content;
} FTDF_IEDescriptor;

typedef struct
{
    /** \brief Number of IE descriptors */
    FTDF_Size          nrOfIEs;
    /** \brief Pointer to the first IE descriptor */
    FTDF_IEDescriptor* IEs;
} FTDF_IEList;

typedef struct
{
    /** \brief Message ID */
    FTDF_MsgId msgId;
} FTDF_MsgBuffer;

typedef struct
{
    /** \brief Message ID = FTDF_DATA_REQUEST */
    FTDF_MsgId               msgId;
    /** \brief Source address mode */
    FTDF_AddressMode         srcAddrMode;
    /** \brief Destination address mode */
    FTDF_AddressMode         dstAddrMode;
    /** \brief Destination PAN ID */
    FTDF_PANId               dstPANId;
    /** \brief Destination address */
    FTDF_Address             dstAddr;
    /** \brief Length of MSDU payload */
    FTDF_DataLength          msduLength;
    /**
     * \brief MSDU payload data buffer. Must be freed by the application using FTDF_REL_DATA_BUFFER
     *        when it receives the FTDF_DATA_CONFIRM message
     */
    FTDF_Octet*              msdu;
    /** \brief Handle */
    FTDF_Handle              msduHandle;
    /** \brief Whether the data frame needs to be acknowledged or */
    FTDF_Boolean             ackTX;
    /** \brief NOT USED */
    FTDF_Boolean             GTSTX;
    /** 
     * \brief Indication whether the frame must be send directly or be queued until a DATA_REQUEST COMMAND
     *        frame is received
     */
    FTDF_Boolean             indirectTX;
    /** \brief Security level */
    FTDF_SecurityLevel       securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode           keyIdMode;
    /** \brief Security key source */
    FTDF_Octet               keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex            keyIndex;
    /** \brief Frame control option bitmap */
    FTDF_FrameControlOptions frameControlOptions;
    /**
     * \brief  Header IE list to be inserted.
     *         Ignored if the FTDF_IES_INCLUDED frame control option is NOT set.
     *         The IE list must be located in rentention RAM
     */
    FTDF_IEList*             headerIEList;
    /**
     * \brief  Payload IE list to be inserted.
     *         Ignored if the FTDF_IES_INCLUDED frame control option is NOT set.
     *         The IE list must be located in rentention RAM
     */
    FTDF_IEList*             payloadIEList;
    /** Indication whether a DATA or MULTIPURPOSE frame must be set */
    FTDF_Boolean             sendMultiPurpose;
    /** \brief DO NOT USE */
    uint8_t                  __private3;
} FTDF_DataRequest;

typedef struct
{
    /** \brief Message ID = FTDF_DATA_INDICATION */
    FTDF_MsgId         msgId;
    /** \brief Source address mode */
    FTDF_AddressMode   srcAddrMode;
    /** \brief Source PAN ID */
    FTDF_PANId         srcPANId;
    /** \brief Source address */
    FTDF_Address       srcAddr;
    /** \brief Destination address mode */
    FTDF_AddressMode   dstAddrMode;
    /** \brief Destination PAN ID */
    FTDF_PANId         dstPANId;
    /** \brief Destination address */
    FTDF_Address       dstAddr;
    /** \brief Length of MSDU payload */
    FTDF_DataLength    msduLength;
    /** \brief MSDU payload data buffer Must be freed by the application using FTDF_REL_DATA_BUFFER */
    FTDF_Octet*        msdu;
    /** \brief MPDU link quality */
    FTDF_LinkQuality   mpduLinkQuality;
    /** \brief Data sequence number */
    FTDF_SN            DSN;
    /** \brief Timestamp of the time the frame was received */
    FTDF_Time          timestamp;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
    /** /brief Payload IE list<br>
     *         The whole payload IE list (including IE descriptors, Sub IE descriptors and content)
     *         is allocated in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
     *         with the IEList pointer as parameter
     */
    FTDF_IEList*       payloadIEList;
} FTDF_DataIndication;

typedef struct
{
    /** \brief Message ID = FTDF_DATA_CONFIRM */
    FTDF_MsgId         msgId;
    /** /brief The handle given by the application in the correspoding FTDF_DATA_REQUEST */
    FTDF_Handle        msduHandle;
    /** /brief Timestmap of the time the frame has been sent */
    FTDF_Time          timestamp;
    /** /brief Status of the data request */
    FTDF_Status        status;
    /** /brief Number of backoffs, undefined in case status not equal to FTDF_SUCCESS or TSCH enabled */
    FTDF_NumOfBackoffs numOfBackoffs;
    /** /brief Data sequence number of the sent frame */
    FTDF_SN            DSN;
    /** /brief Acknowledgement payload IE list<br>
     *         The whole payload IE list (including IE descriptors, Sub IE descriptors and content)
     *         is allocated in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
     *         with the IEList pointer as parameter
     */
    FTDF_IEList*       ackPayload;
} FTDF_DataConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_PURGE_REQUEST */
    FTDF_MsgId  msgId;
    /** \brief Handle the data request to be purged */
    FTDF_Handle msduHandle;
} FTDF_PurgeRequest;

typedef struct
{
    /** \brief Message ID = FTDF_PURGE_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Handle of the purged data request */
    FTDF_Handle msduHandle;
    /** /brief Status of the purge request */
    FTDF_Status status;
} FTDF_PurgeConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_ASSOCIATE_REQUEST */
    FTDF_MsgId             msgId;
    /** \brief Channel number to be used for the associate request. Ignored in TSCH mode */
    FTDF_ChannelNumber     channelNumber;
    /** \brief Channel page to be used for the associate request. Ignored in TSCH mode */
    FTDF_ChannelPage       channelPage;
    /** \brief Coordinator address mode */
    FTDF_AddressMode       coordAddrMode;
    /** \brief Coordinator PAN ID */
    FTDF_PANId             coordPANId;
    /** \brief Coordinator address */
    FTDF_Address           coordAddr;
    /**
     * \brief Capabillity information bitmap. The following capabilities are defined:<br>
     *        FTDF_CAPABILITY_IS_FFD: Device is a full function device<br>
     *        FTDF_CAPABILITY_AC_POWER: Device has AC power<br>
     *        FTDF_CAPABILITY_RECEIVER_ON_WHEN_IDLE: Device receiver is on when idle<br>
     *        FTDF_CAPABILITY_FAST_ASSOCIATION: Device wants a fast association<br>
     *        FTDF_CAPABILITY_SUPPORTS_SECURITY: Device supports security<br>
     *        FTDF_CAPABILITY_WANTS_SHORT_ADDRESS: Device wants short address<br>
     */
    FTDF_Bitmap8           capabilityInformation;
    /** \brief Security level */
    FTDF_SecurityLevel     securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode         keyIdMode;
    /** \brief Security key source */
    FTDF_Octet             keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex          keyIndex;
    /** \brief NOT USED */
    FTDF_ChannelOffset     channelOffset;
    /** \brief NOT USED */
    FTDF_HoppingSequenceId hoppingSequenceId;
    /** \brief DO NOT USE */
    uint8_t                __private3;
} FTDF_AssociateRequest;

typedef struct
{
    /** \brief Message ID = FTDF_ASSOCIATE_INDICATION */
    FTDF_MsgId             msgId;
    /** \brief Extended address of the device that did the association request */
    FTDF_ExtAddress        deviceAddress;
    /**
     * \brief Capabillity information bitmap. The following capabilities are defined:<br>
     *        FTDF_CAPABILITY_IS_FFD: Device is a full function device<br>
     *        FTDF_CAPABILITY_AC_POWER: Device has AC power<br>
     *        FTDF_CAPABILITY_RECEIVER_ON_WHEN_IDLE: Device receiver is on when idle<br>
     *        FTDF_CAPABILITY_FAST_ASSOCIATION: Device wants a fast association<br>
     *        FTDF_CAPABILITY_SUPPORTS_SECURITY: Device supports security<br>
     *        FTDF_CAPABILITY_WANTS_SHORT_ADDRESS: Device wants short address<br>
     */
    FTDF_Bitmap8           capabilityInformation;
    /** \brief Security level */
    FTDF_SecurityLevel     securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode         keyIdMode;
    /** \brief Security key source */
    FTDF_Octet             keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex          keyIndex;
    /** \brief NOT USED */
    FTDF_ChannelOffset     channelOffset;
    /** \brief NOT USED */
    FTDF_HoppingSequenceId hoppingSequenceId;
} FTDF_AssociateIndication;

typedef struct
{
    /** \brief Message ID = FTDF_ASSOCIATE_RESPONSE */
    FTDF_MsgId             msgId;
    /** \brief Extended address of the device that did association request and to which the response needs to be send */
    FTDF_ExtAddress        deviceAddress;
    /** \brief The short address for the device that did the associate request */
    FTDF_ShortAddress      assocShortAddress;
    /** \brief The association status of the associate request */
    FTDF_AssociationStatus status;
    /** \brief Indication whether this a response to fast or normal association request */
    FTDF_Boolean           fastA;
    /** \brief Security level */
    FTDF_SecurityLevel     securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode         keyIdMode;
    /** \brief Security key source */
    FTDF_Octet             keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex          keyIndex;
    /** \brief NOT USED */
    FTDF_ChannelOffset     channelOffset;
    /** \brief NOT USED */
    FTDF_Length            hoppingSequenceLength;
    /** \brief NOT USED */
    FTDF_Octet*            hoppingSequence;
    /** \brief DO NOT USE */
    uint8_t                __private3;
} FTDF_AssociateResponse;

typedef struct
{
    /** \brief Message ID = FTDF_ASSOCIATE_CONFIRM */
    FTDF_MsgId         msgId;
    /** \brief The short adddress assigned by the coordinator */
    FTDF_ShortAddress  assocShortAddress;
    /** \brief Status of the associate request */
    FTDF_Status        status;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
    /** \brief NOT USED */
    FTDF_ChannelOffset channelOffset;
    /** \brief NOT USED */
    FTDF_Length        hoppingSequenceLength;
    /** \brief NOT USED */
    FTDF_Octet*        hoppingSequence;
} FTDF_AssociateConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_DISASSOCIATE_REQUEST */
    FTDF_MsgId              msgId;
    /** \brief Device address mode */
    FTDF_AddressMode        deviceAddrMode;
    /** \brief Device PAN ID */
    FTDF_PANId              devicePANId;
    /** \brief Device address */
    FTDF_Address            deviceAddress;
    /** \brief Disassociate reason */
    FTDF_DisassociateReason disassociateReason;
    /**
     * \brief Indication whether the disassociate request must be send directly or be queued until a
     *        DATA_REQUEST command frame is received
     */
    FTDF_Boolean            txIndirect;
    /** \brief Security level */
    FTDF_SecurityLevel      securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode          keyIdMode;
    /** \brief Security key source */
    FTDF_Octet              keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex           keyIndex;
    /** \brief DO NOT USE */
    uint8_t                 __private3;
} FTDF_DisassociateRequest;

typedef struct
{
    /** \brief Message ID = FTDF_DISASSOCIATE_INDICATION */
    FTDF_MsgId              msgId;
    /** \brief Extended address of the device that did disassociate request */
    FTDF_ExtAddress         deviceAddress;
    /** \brief Disassociate reason */
    FTDF_DisassociateReason disassociateReason;
    /** \brief Security level */
    FTDF_SecurityLevel      securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode          keyIdMode;
    /** \brief Security key source */
    FTDF_Octet              keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex           keyIndex;
} FTDF_DisassociateIndication;

typedef struct
{
    /** \brief Message ID = FTDF_DISASSOCIATE_CONFIRM */
    FTDF_MsgId       msgId;
    /** \brief Disassociate request status */
    FTDF_Status      status;
    /** \brief Address mode of the device that want/must disassociate */
    FTDF_AddressMode deviceAddrMode;
    /** \brief PAN ID of the device that want/must disassociate */
    FTDF_PANId       devicePANId;
    /** \brief address of the device that want/must disassociate */
    FTDF_Address     deviceAddress;
} FTDF_DisassociateConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_BEACON_NOTIFY_INDCATION */
    FTDF_MsgId          msgId;
    /** \brief Beacon sequence number, valid only when beacon type is FTDF_NORMAL_BEACON */
    FTDF_SN             BSN;
    /** \brief PAN descriptor*/
    FTDF_PANDescriptor* PANDescriptor;
    /** \brief NOT USED */
    FTDF_Bitmap8        pendAddrSpec;
    /** \brief NOT USED */
    FTDF_Address*       addrList;
    /** \brief Length of the beacon payload */
    FTDF_DataLength     sduLength;
    /** \brief Beacon payload */
    FTDF_Octet*         sdu;
    /** \brief Enhanced beacon sequence number, valid only when beacon type is FTDF_ENHANCED_BEACON */
    FTDF_SN             EBSN;
    /** \brief Beacon type */
    FTDF_BeaconType     beaconType;
    /**
     * \brief Payload IE list<br>
     *        The whole Payload IE list (including IE descriptors, Sub IE descriptors and content)
     *        is allocate in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
     *        with the IEList pointer as parameter.
     */
    FTDF_IEList*        IEList;
    /**
     * \brief Timestamp of the time the beacon frame was received<br>
     *        In TSCH mode this timestamp should be used to calculate the TSCH timeslot start time
     *        required in a FTDF_TSCH_MODE_REQUEST message
     */
    FTDF_Time64         timestamp;
} FTDF_BeaconNotifyIndication;

typedef struct
{
    /** \brief Message ID = FTDF_COMM_STATUS_INDICATION */
    FTDF_MsgId         msgId;
    /** \brief PAN ID of the frame causing the error */
    FTDF_PANId         PANId;
    /** \brief Source address mode of the frame causing the error  */
    FTDF_AddressMode   srcAddrMode;
    /** \brief Source address of the frame causing the error */
    FTDF_Address       srcAddr;
    /** \brief Destination address mode of the frame causing the error */
    FTDF_AddressMode   dstAddrMode;
    /** \brief Destination address of the frame causing the error */
    FTDF_Address       dstAddr;
    /** \brief Error status */
    FTDF_Status        status;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_CommStatusIndication;

typedef struct
{
    /** \brief Message ID = FTDF_GET_REQUEST */
    FTDF_MsgId        msgId;
    FTDF_PIBAttribute PIBAttribute;
} FTDF_GetRequest;

typedef struct
{
    /** \brief Message ID = FTDF_GET_CONFIRM */
    FTDF_MsgId                    msgId;
    /** \brief Status of the get request */
    FTDF_Status                   status;
    /** \brief PIB attribute ID of PIB attribute gotten */
    FTDF_PIBAttribute             PIBAttribute;
    /**
     * \brief Pointer to the PIB attribute value to be set<br>
     *        Each PIB attribute has its own type, see \link FTDF_PIBAttribute \endlink<br>
     *        In order to access the attribute value the application must cast this pinter
     *        to the correct type. 
     */
    const FTDF_PIBAttributeValue* PIBAttributeValue;
} FTDF_GetConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_SET_REQUEST */
    FTDF_MsgId                    msgId;
    /** \brief PIB attribute ID of attribute to be set */
    FTDF_PIBAttribute             PIBAttribute;
    /**
     * \brief Pointer to the PIB attribute value to be set<br>
     *        Each PIB attribute has its own type, see \link FTDF_PIBAttribute \endlink<br>
     *        FTDF does a shallow copy of the PIB attribute. So if the PIB attribute contains pointers to
     *        other data this data is NOT copied by FTDF. The application must make sure that data referred
     *        from a PIB attribute is statically allocated in retention RAM.
     */
    const FTDF_PIBAttributeValue* PIBAttributeValue;
} FTDF_SetRequest;

typedef struct
{
    /** \brief Message ID = FTDF_GET_CONFIRM */
    FTDF_MsgId        msgId;
    /** \brief Set request status */
    FTDF_Status       status;
    /** \brief PIB Attribute ID of PIB attribute that has been set */
    FTDF_PIBAttribute PIBAttribute;
} FTDF_SetConfirm;

typedef struct
{
    FTDF_MsgId             msgId;
    FTDF_GTSCharateristics GTSCharateristics;
    FTDF_SecurityLevel     securityLevel;
    FTDF_KeyIdMode         keyIdMode;
    FTDF_Octet             keySource[ 8 ];
    FTDF_KeyIndex          keyIndex;
} FTDF_GtsRequest;

typedef struct
{
    FTDF_MsgId             msgId;
    FTDF_GTSCharateristics GTSCharateristics;
    FTDF_Status            status;
} FTDF_GtsConfirm;

typedef struct
{
    FTDF_MsgId             msgId;
    FTDF_ShortAddress      deviceAddress;
    FTDF_GTSCharateristics GTSCharateristics;
    FTDF_SecurityLevel     securityLevel;
    FTDF_KeyIdMode         keyIdMode;
    FTDF_Octet             keySource[ 8 ];
    FTDF_KeyIndex          keyIndex;
} FTDF_GtsIndication;

typedef struct
{
    /** \brief Message ID = FTDF_ORPHAN_INDICATION */
    FTDF_MsgId         msgId;
    /** \brief Extended address of orphaned device */
    FTDF_ExtAddress    orphanAddress;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_OrphanIndication;

typedef struct
{
    /** \brief Message ID = FTDF_ORPHAN_RESPONSE */
    FTDF_MsgId         msgId;
    /** \brief Extended address of orphaned device */
    FTDF_ExtAddress    orphanAddress;
    /** \brief Short address of orphaned device */
    FTDF_ShortAddress  shortAddress;
    /** \brief Indication whether the device is associated with this coordinator or not */
    FTDF_Boolean       associatedMember;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_OrphanResponse;

typedef struct
{
    /** \brief Message ID = FTDF_RESET_REQUEST */
    FTDF_MsgId   msgId;
    /** \brief Indicatione whether the PIB attributes must be reset to their default values or not */
    FTDF_Boolean setDefaultPIB;
} FTDF_ResetRequest;

typedef struct
{
    /** \brief Message ID = FTDF_RESET_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the reset request */
    FTDF_Status status;
} FTDF_ResetConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_RX_ENABLE_REQUEST */
    FTDF_MsgId   msgId;
    /** \brief NOT USED */
    FTDF_Boolean deferPermit;
    /** \brief NOT USED */
    FTDF_Time    rxOnTime;
    /** \brief The time in aBaseSuperFramePeriod's (960 symbols) that the receiver will be on */
    FTDF_Time    rxOnDuration;
} FTDF_RxEnableRequest;

typedef struct
{
    /** \brief Message ID = FTDF_RX_ENABLE_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the RX enable request */
    FTDF_Status status;
} FTDF_RxEnableConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_SCAN_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Scan Type */
    FTDF_ScanType      scanType;
    /** \brief Bitmap of the channels to be scanned  (only channel 11 - 27 are supported ) */
    FTDF_Bitmap32      scanChannels;
    /** \brief Channel page of the channels to be scanned (only page 0 is supported ) */
    FTDF_ChannelPage   channelPage;
    /** \brief Scan duration per channel, duration = ( ( scanDuration ^ 2 ) - 1 ) * aBaseSuperFramePeriod symbols */
    FTDF_ScanDuration  scanDuration;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_ScanRequest;

typedef struct
{
    /** \brief Message ID = FTDF_SCAN_CONFIRM */
    FTDF_MsgId                   msgId;
    /** \brief Status of the scan request */
    FTDF_Status                  status;
    /** \brief Scan Type */
    FTDF_ScanType                scanType;
    /** \brief Channel page of the scanned channels */
    FTDF_ChannelPage             channelPage;
    /** \brief Bitmap of the channels not scanned but were requested to be scanned */
    FTDF_Bitmap32                unscannedChannels;
    /** \brief Number of entries in energyDetectList or PANDescriptorList */
    FTDF_Size                    resultListSize;
    /** \brief Pointer to the first energy detect result */
    FTDF_Energy*                 energyDetectList;
    /** \brief Pointer to the first PAN descriptor */
    FTDF_PANDescriptor*          PANDescriptorList;
    /** \brief */
    FTDF_CoordRealignDescriptor* coordRealignDescriptor;
} FTDF_ScanConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_START_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief PAN ID that will be used by the coordinator */
    FTDF_PANId         PANId;
    /** \brief Channel number that will be used by the coordinator */
    FTDF_ChannelNumber channelNumber;
    /** \brief Channel page that will be used by the coordinator */
    FTDF_ChannelPage   channelPage;
    /** \brief NOT USED */
    FTDF_Time          startTime;
    /** \brief Beacon order, only beacon order 15 (beaconless) is supported  */
    FTDF_Order         beaconOrder;
    /** \brief NOT USED */
    FTDF_Order         superframeOrder;
    /** \brief Indication whether the device must be a coordinator or a PAN coordinator */
    FTDF_Boolean       PANCoordinator;
    /** \brief NOT USED */
    FTDF_Boolean       batteryLifeExtension;
    /** \brief NOT USED */
    FTDF_Boolean       coordRealignment;
    /** \brief NOT USED */
    FTDF_SecurityLevel coordRealignSecurityLevel;
    /** \brief NOT USED */
    FTDF_KeyIdMode     coordRealignKeyIdMode;
    /** \brief NOT USED */
    FTDF_Octet         coordRealignKeySource[ 8 ];
    /** \brief NOT USED */
    FTDF_KeyIndex      coordRealignKeyIndex;
    /** \brief NOT USED */
    FTDF_SecurityLevel beaconSecurityLevel;
    /** \brief NOT USED */
    FTDF_KeyIdMode     beaconKeyIdMode;
    /** \brief NOT USED */
    FTDF_Octet         beaconKeySource[ 8 ];
    /** \brief NOT USED */
    FTDF_KeyIndex      beaconKeyIndex;
} FTDF_StartRequest;

typedef struct
{
    /** \brief Message ID = FTDF_START_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the start request */
    FTDF_Status status;
} FTDF_StartConfirm;

typedef struct
{
    FTDF_MsgId         msgId;
    FTDF_ChannelNumber channnelNumber;
    FTDF_ChannelPage   channelPage;
    FTDF_Boolean       trackbeacon;
} FTDF_SyncRequest;

typedef struct
{
    /** \brief Message ID = FTDF_SYNC_LOSS_INDICATION */
    FTDF_MsgId         msgId;
    /** \brief Loss Reason */
    FTDF_LossReason    lossReason;
    /** \brief PAN ID */
    FTDF_PANId         PANId;
    /** \brief Channel number */
    FTDF_ChannelNumber channelNumber;
    /** \brief Channel page */
    FTDF_ChannelPage   channelPage;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_SyncLossIndication;

typedef struct
{
    /** \brief Message ID = FTDF_POLL_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Coordinator address mode */
    FTDF_AddressMode   coordAddrMode;
    /** \brief coordinator PAN ID */
    FTDF_PANId         coordPANId;
    /** \brief Coordinator address */
    FTDF_Address       coordAddr;
    /** \brief Security level */
    FTDF_SecurityLevel securityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     keyIdMode;
    /** \brief Security key source */
    FTDF_Octet         keySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      keyIndex;
} FTDF_PollRequest;

typedef struct
{
    /** \brief Message ID = FTDF_POLL_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the poll request */
    FTDF_Status status;
} FTDF_PollConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_SET_SLOTFRAME_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Slotframe handle */
    FTDF_Handle        handle;
    /** \brief Set operation */
    FTDF_Operation     operation;
    /** \brief Number of timeslots in this slotframe */
    FTDF_SlotframeSize size;
} FTDF_SetSlotframeRequest;

typedef struct
{
    /** \brief Message ID = FTDF_SET_SLOTFRAME_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Slotframe handle */
    FTDF_Handle handle;
    /** \brief Status of the set sloytframe request */
    FTDF_Status status;
} FTDF_SetSlotframeConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_SET_LINK_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Set operation */
    FTDF_Operation     operation;
    /** \brief Handle of this link, used for delete and modify operations */
    FTDF_Handle        linkHandle;
    /** \brief Handle of the slotframe of this link */
    FTDF_Handle        slotframeHandle;
    /** \brief Timeslot in the slotframe */
    FTDF_Timeslot      timeslot;
    /** \brief Channel offset */
    FTDF_ChannelOffset channelOffset;
    /**
     * \brief Bit mapped link options, the following options are supported:<br>
     *        FTDF_LINK_OPTION_TRANSMIT: Lik can be used for transmitting<br>
     *        FTDF_LINK_OPTION_RECEIVE: Link can be used for receiving<br>
     *        FTDF_LINK_OPTION_SHARED: Shared link, multiple device can transmit in this link<br>
     *        FTDF_LINK_OPTION_TIME_KEEPING: Link will be used to synchronise timeslots<br>
     *        Multiple options can be defined by or-ing them.
     */
    FTDF_Bitmap8       linkOptions;
    /** \brief Link type */
    FTDF_LinkType      linkType;
    /** \brief Short address of the peer node, only valid if is a transmit link */
    FTDF_ShortAddress  nodeAddress;
} FTDF_SetLinkRequest;

typedef struct
{
    /** \brief Message ID = FTDF_SET_LINK_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the set link request */
    FTDF_Status status;
    /** \brief Link handle */
    FTDF_Handle linkHandle;
    /** \brief Slotframe handle */
    FTDF_Handle slotframeHandle;
} FTDF_SetLinkConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_TSCH_MODE_REQUEST */
    FTDF_MsgId    msgId;
    /** \brief The new TSCH mode */
    FTDF_TSCHMode tschMode;
    /**
     * \brief The timestamp in symbols of the start of the timeslot with ASN equal to the PIB attribute
     *        FTDF_PIB_ASN<br>
     *        The timestamp should be calculated from the timestamp of the BEACON_NOTIFY_INDICATION message<br>
     *        Before sending the FTDF_TSCH_MODE_REQUEST message the application should set FTDF_PIB_ASN with
     *        the ASN of the TSCH synchronization IE of the IEList of the BEACON_NOTIFY_INDICATION message
     */
    FTDF_Time     timeslotStartTime;
} FTDF_TschModeRequest;

typedef struct
{
    /** \brief Message ID = FTDF_TSCH_MODE_CONFIRM */
    FTDF_MsgId    msgId;
    /** \brief The new TSCH mode */
    FTDF_TSCHMode tschMode;
    /** \brief Status of the TSCH mode request */
    FTDF_Status   status;
} FTDF_TschModeConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_KEEP_ALIVE_REQUEST */
    FTDF_MsgId           msgId;
    /** \brief Short address to which keep alive message should be send */
    FTDF_ShortAddress    dstAddress;
    /** \brief */
    FTDF_KeepAlivePeriod keepAlivePeriod;
} FTDF_KeepAliveRequest;

typedef struct
{
    /** \brief Message ID = FTDF_KEEP_ALIVE_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the keep alive request */
    FTDF_Status status;
} FTDF_KeepAliveConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_BEACON_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Beacon type */
    FTDF_BeaconType    beaconType;
    /** \brief Channel number at which the beacon must be sent, ignored in TSCH mode */
    FTDF_ChannelNumber channel;
    /** \brief Channel page, ignored in TSCH mode */
    FTDF_ChannelPage   channelPage;
    /** \brief NOT USED */
    FTDF_Order         superframeOrder;
    /** \brief Security level */
    FTDF_SecurityLevel beaconSecurityLevel;
    /** \brief Security key ID mode */
    FTDF_KeyIdMode     beaconKeyIdMode;
    /** \brief Security key source */
    FTDF_Octet         beaconKeySource[ 8 ];
    /** \brief Security key index */
    FTDF_KeyIndex      beaconKeyIndex;
    /** \brief Beacon destination address mode */
    FTDF_AddressMode   dstAddrMode;
    /** \brief Beacon destination address */
    FTDF_Address       dstAddr;
    /** \brief Indication whether the BSN (or EBSN) in the beacon frame must suppressed */
    FTDF_Boolean       BSNSuppression;
    /** \brief DO NOT USE */
    uint8_t            __private3;
} FTDF_BeaconRequest;

typedef struct
{
    /** \brief Message ID = FTDF_BEACON_CONFIRM */
    FTDF_MsgId  msgId;
    /** \brief Status of the beacon request */
    FTDF_Status status;
} FTDF_BeaconConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_BEACON_REQUEST_INDICATION */
    FTDF_MsgId       msgId;
    /** \brief Beacon type */
    FTDF_BeaconType  beaconType;
    /** \brief Source address mode */
    FTDF_AddressMode srcAddrMode;
    /** \brief Source address */
    FTDF_Address     srcAddr;
    /** \brief Source PAN ID */
    FTDF_PANId       dstPANId;
    /**
     * \brief Payload IE list<br>
     *        The whole Payload IE list (including IE descriptors, Sub IE descriptors and content)
     *        is allocate in one data buffer that must be freed by the application using FTDF_REL_DATA_BUFFER
     *        with the IEList pointer as parameter.
     */
    FTDF_IEList*     IEList;
} FTDF_BeaconRequestIndication;

typedef struct
{
    /** \brief Message ID = FTDF_TRANSPARENT_ENABLE_REQUEST */
    FTDF_MsgId    msgId;
    /** \brief Enable or disable of transparent mode */
    FTDF_Boolean  enable;
    /** \brief Bitmapped transparent options, see FTDF_enableTransparentMode */
    FTDF_Bitmap32 options;
} FTDF_TransparentEnableRequest;

typedef struct
{
    /** \brief Message ID = FTDF_TRANSPARENT_REQUEST */
    FTDF_MsgId         msgId;
    /** \brief Frame length */
    FTDF_DataLength    frameLength;
    /** \brief Pointer to frame, must be allocated with FTDF_GET_DATA_BUFFER */
    FTDF_Octet*        frame;
    /** \brief Channel number */
    FTDF_ChannelNumber channel;
    /** \brief Packet Traffic Information (PTI) used for transmitting the frame. */
    FTDF_PTI           pti;
    /** \brief CSMA suppression */
    FTDF_Boolean       cmsaSuppress;
    /** \brief Application provided handle, will be returned in the transparent confirm call */
    void*              handle;
} FTDF_TransparentRequest;

typedef struct
{
    /** \brief Message ID = FTDF_TRANSPARENT_CONFIRM */
    FTDF_MsgId    msgId;
    /** \brief Application provided handle of the corresponding FTDF_sendFrameTransparent call */
    void*         handle;
    /** \brief Bit mapped status */
    FTDF_Bitmap32 status;
} FTDF_TransparentConfirm;

typedef struct
{
    /** \brief Message ID = FTDF_TRANSPARENT_INDICATION */
    FTDF_MsgId      msgId;
    /** \brief Frame length */
    FTDF_DataLength frameLength;
    /** \brief Pointer to frame, must be freed by the application using FTDF_REL_DATA_BUFFER */
    FTDF_Octet*     frame;
    /** \brief Bit mapped receive status */
    FTDF_Bitmap32   status;
} FTDF_TransparentIndication;

typedef struct
{
	/** \brief Message ID = FTDF_SLEEP_REQUEST */
	FTDF_MsgId msgId;
	/** \brief Sleep time */
	FTDF_USec  sleepTime;
} FTDF_SleepRequest;

#if FTDF_DBG_BUS_ENABLE
typedef struct
{
        /** \brief Message ID = FTDF_DBG_MODE_SET_REQUEST */
        FTDF_MsgId msgId;
        /** \brief Debug bus mode */
        FTDF_DbgMode dbgMode;
} FTDF_DbgModeSetRequest;
#endif /* FTDF_DBG_BUS_ENABLE */

/**
 * \brief       FTDF_getReleaseInfo - gets the LMAC (FPGA) and UMAC (FTDF SW driver) release info
 * \param[out]  lmacRelName:     LMAC release name
 * \param[out]  lmacBuildTime:   LMAC build time
 * \param[out]  umacRelName:     UMAC release name
 * \param[out]  umacBuildTime:   UMAC build time
 * \remark      Sets the application character strings pointers to character strings containing the release
 *              info. The character string space is statically allocated by FTDF and does not need 
 *              to be released by the application.
 */
void            FTDF_getReleaseInfo( char** lmacRelName, char** lmacBuildTime, char** umacRelName,
                                     char** umacBuildTime );

/**
 * \brief       FTDF_confirmLmacInterrupt - Confirmation that the LMAC interrupt has been received
 *              by the application
 * \remark      This function disables the LMAC interrupt. The application must wait for any other
 *              FTDF API function to be completed and call the function FTDF_eventHandler. This function
 *              will enable the LMAC interrupt again the function FTDF_eventHandler.
 */
void            FTDF_confirmLmacInterrupt( void );

/**
 * \brief       FTDF_eventHandler - Handles an event signalled by the LMAC interrupt
 * \remark      This function handles the event signalled by the LMAC interrupt and enables the LMAC
 *              interrupt when finished.
 * \warning     FTDF is strictly NON reentrant, so FTDF_eventHandler is NOT allowed to be called when another FTDF
 *              is being executed
 */
void            FTDF_eventHandler( void );

/**
 * \brief       FTDF_sndMsg - Sends a request (response) message to FTDF driver
 * \param[in]   msgBuf:      The message to be send. To application must set the msgId field and the request
 *                           specific parameters by casting it to the appropriate request message structure.
 * \remark      The message must be allocated by the application using the function FTDF_GET_MSG_BUFFER
 *              and will be released by FTDF using FTDF_REL_MSG_BUFFER. When finished FTDF releases the message
 *              buffer and sends a confirm (indication) to the application using the function FTDF_RCV_MSG.
 * \remark      As follows a table of message IDs accepted by FTDF_sndMsg, the message structure corresponding
 *              to this message ID and the confirm send back to the application when the request has been
 *              completed by FTDF:
 * \remark      <br>
 * \remark      Request message ID, Request structure, Request confirm
 * \remark      ---------------------------------------------------------------------------------
 * \remark      FTDF_DATA_REQUEST, FTDF_DataRequest, FTDF_DATA_CONFIRM
 * \remark      FTDF_PURGE_REQUEST, FTDF_PurgeRequest, FTDF_DATA_CONFIRM
 * \remark      FTDF_ASSOCIATE_REQUEST, FTDF_AssociateRequest, FTDF_ASSOCIATE_CONFIRM
 * \remark      FTDF_ASSOCIATE_RESPONSE, FTDF_AssociateResponse, FTDF_COMM_STATUS_INDICATION
 * \remark      FTDF_DISASSOCIATE_REQUEST, FTDF_DisassociateRequest, FTDF_DISASSOCIATE_CONFIRM
 * \remark      FTDF_GET_REQUEST, FTDF_GetRequest, FTDF_GET_CONFIRM
 * \remark      FTDF_SET_REQUEST, FTDF_SetRequest, FTDF_SET_CONFIRM
 * \remark      FTDF_ORPHAN_RESPONSE, FTDF_OrphanResponse,  FTDF_COMM_STATUS_INDICATION
 * \remark      FTDF_RESET_REQUEST, FTDF_ResetRequest, FTDF_RESET_CONFIRM
 * \remark      FTDF_RX_ENABLE_REQUEST, FTDF_ScanRequest, FTDF_RX_ENABLE_CONFIRM
 * \remark      FTDF_SCAN_REQUEST, FTDF_ScanRequest, FTDF_SCAN_CONFIRM
 * \remark      FTDF_START_REQUEST, FTDF_StartRequest, FTDF_START_CONFIRM
 * \remark      FTDF_POLL_REQUEST, FTDF_PollRequest, FTDF_POLL_CONFIRM
 * \remark      FTDF_SET_SLOTFRAME_REQUEST, FTDF_SetSlotframeRequest, FTDF_SET_SLOTFRAME_CONFIRM
 * \remark      FTDF_SET_LINK_REQUEST, FTDF_SetLinkRequest, FTDF_SET_LINK_CONFIRM
 * \remark      FTDF_TSCH_MODE_REQUEST, FTDF_TschModeRequest, FTDF_TSCH_MODE_CONFIRM
 * \remark      FTDF_KEEP_ALIVE_REQUEST, FTDF_KeepAliveRequest, FTDF_KEEP_ALIVE_CONFIRM
 * \remark      FTDF_BEACON_REQUEST, FTDF_BeaconRequest, FTDF_BEACON_CONFIRM
 * \warning     FTDF is strictly NON reentrant, so it is NOT allowed to call any other FTDF function before this API has been completed. This including the FTDF_eventHandler function.
 */
void            FTDF_sndMsg( FTDF_MsgBuffer* msgBuf );

/**
 * \brief       FTDF_RCV_MSG - Receives a confir or indication message from the FTDF driver
 * \param[in]   msgBuf:      The received message. The msgId field indicates the received message
 * \remark      This function must be implemented by the application and is called by FTDF to pass 
 *              a confirm or indication message to the application. It must be configured in the
 *              configuration file ftdf_config.h.
 * \remark      The message buffer is allocated by FTDF using FTDF_GET_MSG_BUFFER and must be released
 *              by the application using FTDF_REL_MSG_BUFFER
 * \remark      As follows a table of the confirm and indications messages and their message structures.
 * \remark      <br>
 * \remark      Confirm/indication message ID, Confirm/indication structure
 * \remark      ---------------------------------------------------------------------------------
 * \remark      FTDF_DATA_INDICATION, FTDF_DataIndication
 * \remark      FTDF_DATA_CONFIRM, FTDF_DataConfirm
 * \remark      FTDF_PURGE_CONFIRM, FTDF_PurgeConfirm
 * \remark      FTDF_ASSOCIATE_INDICATION, FTDF_AssociateIndication
 * \remark      FTDF_ASSOCIATE_CONFIRM, FTDF_AssociateConfirm
 * \remark      FTDF_DISASSOCIATE_INDICATION, FTDF_DisassociateIndication
 * \remark      FTDF_DISASSOCIATE_CONFIRM,FTDF_DisassociateConfirm
 * \remark      FTDF_BEACON_NOTIFY_INDICATION, FTDF_BeaconNotifyIndication
 * \remark      FTDF_COMM_STATUS_INDICATION, FTDF_CommStatusIndication
 * \remark      FTDF_GET_CONFIRM, FTDF_GetConfirm
 * \remark      FTDF_SET_CONFIRM, FTDF_SetConfirm
 * \remark      FTDF_ORPHAN_INDICATION, FTDF_OrphanIndication
 * \remark      FTDF_RESET_CONFIRM, FTDF_ResetConfirm
 * \remark      FTDF_RX_ENABLE_CONFIRM, FTDF_RxEnableConfirm
 * \remark      FTDF_SCAN_CONFIRM, FTDF_ScanConfirm
 * \remark      FTDF_START_CONFIRM, FTDF_StartConfirm
 * \remark      FTDF_SYNC_LOSS_INDICATION, FTDF_SyncLossIndication
 * \remark      FTDF_POLL_CONFIRM, FTDF_PollConfirm
 * \remark      FTDF_SET_SLOTFRAME_CONFIRM, FTDF_SetSlotframeConfirm
 * \remark      FTDF_SET_LINK_CONFIRM, FTDF_SetLinkConfirm
 * \remark      FTDF_TSCH_MODE_CONFIRM, FTDF_TschModeConfirm
 * \remark      FTDF_KEEP_ALIVE_CONFIRM, FTDF_KeepAliveConfirm
 * \remark      FTDF_BEACON_CONFIRM, FTDF_BeaconConfirm
 * \remark      FTDF_BEACON_REQUEST_INDICATION, FTDF_BeaconRequestIndication
 * \warning     FTDF is strictly NON reentrant. Because FTDF_RCV_MSG can be called by FTDF_sndMsg or FTDF_eventHandler
 *              it is not allowed to call other FTDF functions within FTDF_RCV_MSG
 */
void            FTDF_RCV_MSG( FTDF_MsgBuffer* msgBuf );

/**
 * \brief       FTDF_GET_MSG_BUFFER - Allocates a message buffer
 * \param[in]   len:      The length of the message buffer to be allocated
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      The allocated message buffer must be freed using FTDF_REL_MSG_BUFFER
 * \remark      The message buffer must be located in retention RAM
 * \return      pointer to the allocated message buffer.
 */
FTDF_MsgBuffer* FTDF_GET_MSG_BUFFER( FTDF_Size len );

/**
 * \brief       FTDF_REL_MSG_BUFFER - Frees a message buffer
 * \param[in]   msgBuf:      Pointer to the message buffer to be released
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      It will free message buffers allocated with FTDF_GET_MSG_BUFFER
 */
void            FTDF_REL_MSG_BUFFER( FTDF_MsgBuffer* msgBuf );

/**
 * \brief       FTDF_GET_DATA_BUFFER - Allocates a data buffer
 * \param[in]   len:      The length of the data buffer to be allocated
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      The allocated data buffer must be freed using FTDF_REL_DATA_BUFFER
 * \remark      The data buffer must located in retention RAM
 * \return      pointer to the allocated data buffer.
 */
FTDF_Octet*     FTDF_GET_DATA_BUFFER( FTDF_DataLength len );

/**
 * \brief       FTDF_REL_DATA_BUFFER - Frees a data buffer
 * \param[in]   dataBuf:      Pointer to the data buffer to be released
 * \remark      This function must be implemented by the application it will called by both the
 *              application and FTDF. It must be configured in the configuration file ftdf_config.h.
 * \remark      It will free data buffers allocated with FTDF_GET_DATA_BUFFER
 */
void            FTDF_REL_DATA_BUFFER( FTDF_Octet* dataBuf );

/**
 * \brief       FTDF_GET_EXT_ADDRESS - Gets the extended address
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      After a reset it is used by FTDF to get the extended address of the device
 * \return      An extended address. 
 */
FTDF_ExtAddress FTDF_GET_EXT_ADDRESS( void );

/**
 * \brief       FTDF_enableTransparentMode - Enables or disables the transparent mode
 * \param[in]   enable: If FTDF_TRUE transparent mode is enabled else it will be disabled
 * \param[in]   options: A bitmap of transparent mode options. Not used if enable equals to FTDF_FALSE.
 * \remark      If the transparent mode is enabled received frame are passed to the application using
 *              FTDF_RCV_FRAME_TRANSPARENT. The whole MAC frame including frame headers, security headers
 *              and FCS are passed to the application. Depending on the defined options some filtering
 *              is done.
 * \remark      When the transparent mode is enabled frames must be send using the function FTDF_sendFrameTransparent
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
 * \remark      FTDF_TRANSPARENT_PASS_ALL_PAN_ID: Frames with any PAN ID (even when it does not match its own
 *              PAN ID) are passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_PAN_ID: Frames with any address (even when it does not match its own
 *              extended or short address) are passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_BEACON: Any beacon frame is passed to the application
 * \remark      FTDF_TRANSPARENT_PASS_ALL_NO_DEST_ADDR: Frames with no destination address are always passed to
 *              the application
 * \remark      FTDF_TRANSPARENT_ENABLE_FCS_GENERATION: FTDF adds an FCS to frames send with FTDF_sendFrameTransparent
 */
void            FTDF_enableTransparentMode( FTDF_Boolean  enable,
                                            FTDF_Bitmap32 options );

/**
 * \brief       FTDF_sendFrameTransparent - Sends a frame transparent
 * \param[in]   frameLength:   Length of the MAC frame to be send
 * \param[in]   frame:         Data buffer with the frame to be sent. Must be allocate using FTDF_GET_DATA_BUFFER
 * \param[in]   channel:       The channel used to send the frame
 * \param[in]   pti:           Packet Traffic Information used for transmitting the packet
 * \param[in]   cmsaSuppress:  If FTDF_TRUE no CSMA-CA is performed before sending the frame
 * \param[in]   handle:        A user defined handle.
 * \remark      FTDF sends the frame tarnsparently. That is the application has to compose the MAC frame header, 
 *              security header, etc and make sure that it complies with the IEEE802.15.4 standard. Before sending
 *              it FTDF adds the  preamble, frame start, phy header octets and optionally an FCS to the frame provide
 *              by the application.
 * \remark      When FTDF has send the frame or failed to send the frame it will call FTDF_SEND_FRAME_TRANSPARENT_CONFIRM
 *              with the handle provided by the application and the send status.
 */
void FTDF_sendFrameTransparent( FTDF_DataLength    frameLength,
                                FTDF_Octet*        frame,
                                FTDF_ChannelNumber channel,
                                FTDF_PTI           pti,
                                FTDF_Boolean       cmsaSuppress,
                                void*              handle );

/**
 * \brief       FTDF_SEND_FRAME_TRANSPARENT_CONFIRM - Confirms a transparent send
 * \param[in]   handle:   The application provided handle of the corresponding FTDF_sendFrameTransparent call
 * \param[in]   status:   Bit mapped status
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when a frame has send successfully or failed to send. The application can
 *              now free the data buffer of the corresponding FTDF_sendFrameTransparent using FTDF_REL_DATA_BUFFER
 * \remark      If the frame is send successfully status is equal to 0
 * \remark      If sending the frame failed one of more of the following status bits is set:
 *              - FTDF_TRANSPARENT_CSMACA_FAILURE: Sending failed because CSMA-CA failed
 *              - FTDF_TRANSPARENT_OVERFLOW: Sending failed because of internal resource overflow
 */
void FTDF_SEND_FRAME_TRANSPARENT_CONFIRM( void*         handle,
                                          FTDF_Bitmap32 status );

/**
 * \brief       FTDF_sendFrameTransparentConfirm - Converts the FTDF_SEND_FRAME_TRANSPARENT_CONFIRM to a msg buffer
 *              and then calls FTDF_RCV_MSG.
 * \param[in]   handle:   The application provided handle of the corresponding FTDF_sendFrameTransparent call
 * \param[in]   status:   Bit mapped status
 * \remark      This function converts the input parameters to a FTDF_TransparentConfirm structure
 *              and then calls FTDF_RCV_MSG.
 * \remark      FTDF_SEND_FRAME_TRANSPARENT_CONFIRM can be defined to be this function.
 */
void FTDF_sendFrameTransparentConfirm( void*         handle,
                                       FTDF_Bitmap32 status );

/**
 * \brief       FTDF_RCV_FRAME_TRANSPARENT - Receive frame transparent
 * \param[in]   frameLength: Length of the received frame
 * \param[in]   frame:       Pointer to received frame. 
 * \param[in]   status:      Bitmapped receive status
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when a frame is received when transparent mode is enabled
 * \remark      The buffer with the frame must be copied by the application before this function returns
 * \remark      The following status bits can be set:
 *              - FTDF_TRANSPARENT_RCV_CRC_ERROR: The CRC (FCS) of the received frame is not correct
 *              - FTDF_TRANSPARENT_RCV_RES_FRAMETYPE: Received a reserved frame type
 *              - FTDF_TRANSPARENT_RCV_RES_FRAME_VERSION: Received a reserved frame version
 *              - FTDF_TRANSPARENT_RCV_UNEXP_DST_ADDR: Unexpected destination address
 *              - FTDF_TRANSPARENT_RCV_UNEXP_DST_PAN_ID: Unexpected destionation PAN ID
 *              - FTDF_TRANSPARENT_RCV_UNEXP_BEACON: Unexpected beacon frame
 *              - FTDF_TRANSPARENT_RCV_UNEXP_NO_DEST_ADDR: Unexpected frame without destination adres.
 */
void FTDF_RCV_FRAME_TRANSPARENT( FTDF_DataLength frameLength,
                                 FTDF_Octet*     frame,
                                 FTDF_Bitmap32   status,
                                 FTDF_LinkQuality lqi);

/**
 * \brief       FTDF_rcvFrameTransparent - Converts the FTDF_RCV_FRAME_TRANSPARENT to a msg buffer
 *              and then calls FTDF_RCV_MSG.
 * \param[in]   frameLength: Length of the received frame
 * \param[in]   frame:       Pointer to received frame. 
 * \param[in]   status:      Bitmapped receive status
 * \remark      This function convers the input parameters to a FTDF_TransparentIndication structure
 *              and then calls FTDF_RCV_MSG.
 * \remark      FTDF_RCV_FRAME_TRANSPARENT can be defined to be this function.
 */
void FTDF_rcvFrameTransparent( FTDF_DataLength frameLength,
                               FTDF_Octet*     frame,
                               FTDF_Bitmap32   status,
                               FTDF_LinkQuality lqi);

/**
 * \brief       FTDF_setSleepAttributes - Set sleep atributes
 * \param[in]   lowPowerClockCycle: The LP clock cycle in picoseconds
 * \param[in]   wakeUpLatency:      The wakep up latency time in low power clock cycles
 * \remark      This functions sets some attributes required by FTDF to go into a deep sleep and wake up from it.
 * \remark      The wake up latency is the time between the wakeup signal send by FTDF and the moment that the
 *              clocks are stable. It will be used by FTDF to determine when it must must send the wakeup signal.
 * \remark      The low power clock cycle will be used to determine how long the system has slept and recover 
 *              the symbol counter internally used by FTDF.
 */
void FTDF_setSleepAttributes( FTDF_PSec                  lowPowerClockCycle,
                              FTDF_NrLowPowerClockCycles wakeUpLatency );

/**
 * \brief       FTDF_canSleep - Indicates whether FTDF can go in a deep sleep
 * \remark      This function returns the time in microseconds that the FTDF can deep sleep.
 * \remark      It does NOT take into account the wakeup latency.
 * \return      The number of microseconds that the FTDF can deep sleep. If equal to 0 the FTDF is still active
 *              and cannot deep sleep
 */
FTDF_USec FTDF_canSleep( void );

/**
 * \brief       FTDF_prepareForSleep - Prepare the FTDF for a deep sleep
 * \param[in]   sleepTime: The sleep time. After the sleep time the FTDF must be ready to do next scheduled activity.
 * \remark      When this function is completed the application can safely power off the FTDF and put the uP in
 *              a deep sleep. At least wake up latency time microseconds before sleep time is expired the FTDF
 *              will send a wakeup signal. FTDF expect that the application calls FTDF_wakeUp as it has been
 *              woken up.
 * \return      FTDF_TRUE if the FTDF can be put in deep sleep, FTDF_FALSE if the sleepTime is too short to go to
 *              deep sleep and wake up again after sleepTime microseconds, the FTDF cannot be put in deep sleep.
 */
FTDF_Boolean FTDF_prepareForSleep( FTDF_USec sleepTime );

/**
 * \brief       FTDF_wakeUp - Wake up the FTDF after a sleep
 * \remark      This function will restores the FTDF to state before the deep sleep. It will does this in 2 steps
 *              1. It initialises it internal states and data structures and all the non timing specific HW. After
 *              that it waits until the wake up latency time has been expired. When expired FTDF assumes that the
 *              slocks are stable.
 *              2. It restores timers in HW and calls the function FTDF_WAKE_UP_READY to indicate FTDF is up again,
 *              ready to perform scheduled activities and ready to process new requests from the application.
 */
void FTDF_wakeUp( void );

/**
 * \brief       FTDF_WAKE_UP_READY - Indication that FTDF has woke up
 * \remark      This function must be implemented by the application it will called by FTDF. It must
 *              be configured in the configuration file ftdf_config.h.
 * \remark      FTDF calls this function when it is up again, ready to perform scheduled activities and ready to
 *              process new requests from the application.
 */
void FTDF_WAKE_UP_READY( void );

/**
 * \brief       FTDF_SLEEP_CALLBACK - Callback from FTDF_SLEEP_REQUEST
 * \remark      This function must be implemented by the application, it will be called by FTDF.
 *              It must be configured in the configuration file ftdf_config.h
 * \remark      FTDF calls this function when an FTDF_SLEEP_REQUEST message is sent through the FTDF_sndMsg function.
 */
void FTDF_SLEEP_CALLBACK( FTDF_USec sleepTime );

void FTDF_reset(int setDefaultPIB);

FTDF_PTI FTDF_getRxPti( void );

void FTDF_setRxPti( FTDF_PTI rx_pti );
#ifdef FTDF_PHY_API
FTDF_Status FTDF_sendFrameSimple( FTDF_DataLength    frameLength,
        FTDF_Octet*        frame,
        FTDF_ChannelNumber channel,
        FTDF_PTI           pti,
        FTDF_Boolean       csmaSuppress);
FTDF_PIBAttributeValue *FTDF_getValue(FTDF_PIBAttribute PIBAttribute);
FTDF_Status FTDF_setValue(FTDF_PIBAttribute PIBAttribute, const FTDF_PIBAttributeValue *PIBAttributeValue);
void FTDF_rxEnable(FTDF_Time rxOnDuration);
#endif /* FTDF_PHY_API */

#if FTDF_DBG_BUS_ENABLE
/**
 * \brief Checks current debug mode and makes appropriate hardware configurations.
 */
void FTDF_checkDbgMode(void);

/**
 * \brief Sets debug bus mode.
 */
void FTDF_setDbgMode(FTDF_DbgMode dbgMode);

/**
 * \brief Debug bus GPIO configuration callback. Makes appropriate GPIO configurations for the debug
 * bus.
 */
void FTDF_DBG_BUS_GPIO_CONFIG(void);
#endif /* FTDF_DBG_BUS_ENABLE */

#endif /* FTDF_H_ */

/**
 * \}
 * \}
 * \}
 */

