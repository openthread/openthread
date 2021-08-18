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

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-history-tracker
 *
 * @{
 *
 */

/**
 * @brief
 *   This header defines the public API for History Tracker.
 *
 *   History Tracker module records history of different events (e.g. RX and TX messages or network info changes, etc.)
 *   as the Thread network operates. All tracked entries are timestamped.
 *
 *   The functions in this module are available when `OPENTHREAD_CONFIG_HISTOR_TRACKER_ENABLE` is enabled.
 *
 */

/**
 * This constant specifies the maximum age of entries which is 49 days (in msec).
 *
 * Entries older than the max age will give this value as their age.
 *
 */
#define OT_HISTORY_TRACKER_MAX_AGE (49 * 24 * 60 * 60 * 1000u)

/**
 * This type represents an iterator to iterate through a history list.
 *
 * The fields in this type are opaque (intended for use by OpenThread core) and therefore should not be accessed/used
 * by caller.
 *
 * Before using an iterator, it MUST be initialized using `otHistoryTrackerInitIterator()`,
 *
 */
typedef struct otHistoryTrackerIterator
{
    uint32_t mData32;
    uint16_t mData16;
} otHistoryTrackerIterator;

/**
 * This structure represents Thread network info.
 *
 */
typedef struct otHistoryTrackerNetworkInfo
{
    otDeviceRole     mRole;        ///< Device Role.
    otLinkModeConfig mMode;        ///< Device Mode.
    uint16_t         mRloc16;      ///< Device RLOC16.
    uint32_t         mPartitionId; ///< Partition ID (valid when attached).
} otHistoryTrackerNetworkInfo;

/**
 * Constants representing message priority used in `otHistoryTrackerMessageInfo` struct.
 *
 */
enum
{
    OT_HISTORY_TRACKER_MSG_PRIORITY_LOW    = OT_MESSAGE_PRIORITY_LOW,      ///< Low priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL = OT_MESSAGE_PRIORITY_NORMAL,   ///< Normal priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_HIGH   = OT_MESSAGE_PRIORITY_HIGH,     ///< High priority level.
    OT_HISTORY_TRACKER_MSG_PRIORITY_NET    = OT_MESSAGE_PRIORITY_HIGH + 1, ///< Network Control priority level.
};

/**
 * This structure represents a RX/TX IPv6 message info.
 *
 * Some of the fields in this struct are applicable to a RX message or a TX message only, e.g., `mAveRxRss` is the
 * average RSS of all fragment frames that form a received message and is only applicable for a RX message.
 *
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
    int8_t     mAveRxRss;            ///< RSS of received message or OT_RADIO_INVALI_RSSI if not known.
    bool       mLinkSecurity : 1;    ///< Indicates whether msg used link security.
    bool       mTxSuccess : 1;       ///< Indicates TX success (e.g., ack received). Applicable for TX msg only.
    uint8_t    mPriority : 2;        ///< Message priority (`OT_HISTORY_TRACKER_MSG_PRIORITY_*` enumeration).
    bool       mRadioIeee802154 : 1; ///< Indicates whether msg was sent/received over a 15.4 radio link.
    bool       mRadioTrelUdp6 : 1;   ///< Indicates whether msg was sent/received over a TREL radio link.
} otHistoryTrackerMessageInfo;

/**
 * This function initializes an `otHistoryTrackerIterator`.
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
 *
 */
void otHistoryTrackerInitIterator(otHistoryTrackerIterator *aIterator);

/**
 * This function iterates over the entries in the network info history list.
 *
 * @param[in]    aInstance   A pointer to the OpenThread instance.
 * @param[inout] aIterator   A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]   aEntryAge   A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns A pointer to `otHistoryTrackerNetworkInfo` entry or `NULL` if no more entries in the list.
 *
 */
const otHistoryTrackerNetworkInfo *otHistoryTrackerIterateNetInfoHistory(otInstance *              aInstance,
                                                                         otHistoryTrackerIterator *aIterator,
                                                                         uint32_t *                aEntryAge);

/**
 * This function iterates over the entries in the RX message history list.
 *
 * @param[in]    aInstance   A pointer to the OpenThread instance.
 * @param[inout] aIterator   A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]   aEntryAge   A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerMessageInfo` entry or `NULL` if no more entries in the list.
 *
 */
const otHistoryTrackerMessageInfo *otHistoryTrackerIterateRxHistory(otInstance *              aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t *                aEntryAge);

/**
 * This function iterates over the entries in the TX message history list.
 *
 * @param[in]    aInstance   A pointer to the OpenThread instance.
 * @param[inout] aIterator   A pointer to an iterator. MUST be initialized or the behavior is undefined.
 * @param[out]   aEntryAge   A pointer to a variable to output the entry's age. MUST NOT be NULL.
 *                           Age is provided as the duration (in milliseconds) from when entry was recorded to
 *                           @p aIterator initialization time. It is set to `OT_HISTORY_TRACKER_MAX_AGE` for entries
 *                           older than max age.
 *
 * @returns The `otHistoryTrackerMessageInfo` entry or `NULL` if no more entries in the list.
 *
 */
const otHistoryTrackerMessageInfo *otHistoryTrackerIterateTxHistory(otInstance *              aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t *                aEntryAge);

#define OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE 21 ///< Recommended size for string representation of an entry age.

/**
 * This function converts a given entry age to a human-readable string.
 *
 * The entry age string follows the format "<hh>:<mm>:<ss>.<mmmm>" for hours, minutes, seconds and millisecond (if
 * shorter than one day) or "<dd> days <hh>:<mm>:<ss>.<mmmm>" (if longer than one day).
 *
 * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be truncated
 * but the outputted string is always null-terminated.
 *
 * @param[in]  aEntryAge The entry age (duration in msec).
 * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be NULL).
 * @param[in]  aSize     The size of @p aBuffer (in bytes). Recommended to use `OT_IP6_ADDRESS_STRING_SIZE`.
 *
 */
void otHistoryTrackerEntryAgeToString(uint32_t aEntryAge, char *aBuffer, uint16_t aSize);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_HISTORY_TRACKER_H_
