/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file defines the OpenThread IEEE 802.15.4 Link Layer extension API, which provides link-layer mechanisms
 *   built on top of the raw link-layer (see `link_raw.h`).
 */

#ifndef OPENTHREAD_LINK_EXT_H_
#define OPENTHREAD_LINK_EXT_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-link-ext
 *
 * @brief
 *   This module includes functions for link-layer extensions built on top of the raw link-layer.
 *
 * @{
 */

/**
 * Represents the configuration of a burst transmission.
 */
typedef struct otLinkExtBurstConfig
{
    uint16_t mCount;       ///< Number of burst ticks (remaining ticks, including the current one, when read).
    uint32_t mPeriodUs;    ///< Tick period in microseconds (0 for back-to-back ticks).
    uint32_t mChannelMask; ///< Channels (bit N for channel N) to transmit on in each tick (0 for the frame channel).
} otLinkExtBurstConfig;

/**
 * Pointer on receipt of the end of a burst transmission.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
typedef void (*otLinkExtBurstDone)(otInstance *aInstance);

/**
 * Begins a burst transmission of the frame in the transmit buffer.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * The frame is transmitted unchanged in each of `mCount` ticks. In each tick, the frame is transmitted on every
 * channel in `mChannelMask` in ascending order (or on the frame channel if `mChannelMask` is 0), back-to-back. The
 * frame MUST NOT request an ACK and MUST NOT require security processing. This function MUST NOT be called while a
 * transmission requested by `otLinkRawTransmit()` is ongoing.
 *
 * When `mPeriodUs` is non-zero, ticks start every `mPeriodUs` on the radio clock, from the frame's target TX time if
 * specified, or from now. A timed tick is handed to the radio `OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US` before
 * its target TX time. Only the first channel of a tick uses the target TX time; the other channels are sent
 * right after it. A tick whose start time has passed (e.g. overrun by the previous tick) is skipped. The frame's
 * maximum CSMA backoffs and maximum frame retries SHOULD be 0 for a timed burst, since any CSMA-CA backoff or
 * retransmission would be late.
 *
 * Between ticks, the radio is in the state last requested by `otLinkRawReceive()` (on the channel set by
 * `otLinkSetChannel()`) or `otLinkRawSleep()`, which do not end the burst. The frame's RX channel after TX done is
 * ignored.
 *
 * The transmit buffer MUST NOT be modified, and `otLinkRawTransmit()` and `otLinkRawEnergyScan()` fail with
 * `OT_ERROR_INVALID_STATE`, until the burst ends. The burst ends after its last tick, when stopped by
 * `otLinkExtStopBurst()`, or when the raw link-layer is disabled. @p aCallback is then called, except when
 * `otLinkExtStopBurst()` ends the burst right away.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aConfig    A pointer to the burst configuration.
 * @param[in]  aCallback  A pointer to a function called when the burst ends.
 *
 * @retval OT_ERROR_NONE           Successfully started the burst.
 * @retval OT_ERROR_INVALID_ARGS   The configuration or the transmit frame is invalid for a burst, or all its ticks
 *                                 are already in the past.
 * @retval OT_ERROR_INVALID_STATE  The raw link-layer isn't enabled.
 * @retval OT_ERROR_BUSY           A burst transmission is ongoing.
 */
otError otLinkExtTransmitBurst(otInstance                 *aInstance,
                               const otLinkExtBurstConfig *aConfig,
                               otLinkExtBurstDone          aCallback);

/**
 * Stops the ongoing burst transmission.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * If no burst frame is with the radio, the burst ends right away, and the callback of `otLinkExtTransmitBurst()` is
 * not called (`otLinkExtGetBurstConfig()` reads `mCount` as 0). Otherwise, the frame being transmitted cannot be
 * taken back, and the burst ends when the radio is done with it, which is reported by the callback (until then,
 * `otLinkExtGetBurstConfig()` reads `mCount` as 1).
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           The burst is stopped or will stop, or no burst is ongoing.
 * @retval OT_ERROR_INVALID_STATE  The raw link-layer isn't enabled.
 */
otError otLinkExtStopBurst(otInstance *aInstance);

/**
 * Gets the configuration of the ongoing burst transmission.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * `mCount` is the number of remaining ticks, including the one in progress. All fields are zero if no burst is
 * ongoing.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[out] aConfig    A pointer to return the configuration.
 */
void otLinkExtGetBurstConfig(otInstance *aInstance, otLinkExtBurstConfig *aConfig);

/**
 * Represents a periodic receive schedule.
 *
 * A receive window starts at every radio time `T` (see `otPlatRadioGetNow()`) that satisfies
 * `T % mPeriodUs == mPhaseUs` and lasts `mDurationUs` microseconds.
 */
typedef struct otLinkExtPeriodicRxSchedule
{
    uint32_t mPhaseUs;    ///< Phase of the window start within the period (in usec), less than `mPeriodUs`.
    uint32_t mPeriodUs;   ///< Period (in usec), non-zero.
    uint16_t mDurationUs; ///< Window duration (in usec), non-zero and not more than `mPeriodUs`.
    uint8_t  mChannel;    ///< Channel to listen on, in the radio's supported channel mask.
} otLinkExtPeriodicRxSchedule;

/**
 * Sets the periodic receive schedules.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES`.
 *
 * Atomically replaces all schedules. While the radio is in the Sleep state, receive windows are opened according to
 * the schedules using `otPlatRadioReceiveAt()`. Receive, transmit and energy scan take precedence, and periodic RX
 * resumes when the radio returns to Sleep. Array order defines priority (index 0 has the highest priority) between
 * overlapping windows. Passing zero schedules disables periodic RX. A window already scheduled on the radio may still
 * occur once after the schedules are changed.
 *
 * @param[in] aInstance      A pointer to an OpenThread instance.
 * @param[in] aSchedules     A pointer to an array of schedules (can be NULL if @p aNumSchedules is zero).
 * @param[in] aNumSchedules  The number of schedules in @p aSchedules.
 *
 * @retval OT_ERROR_NONE          Successfully set the schedules.
 * @retval OT_ERROR_NO_BUFS       More than `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES` schedules. The
 *                                schedules are left unchanged.
 * @retval OT_ERROR_INVALID_ARGS  A schedule is invalid. The schedules are left unchanged.
 */
otError otLinkExtSetPeriodicRxSchedules(otInstance                        *aInstance,
                                        const otLinkExtPeriodicRxSchedule *aSchedules,
                                        uint8_t                            aNumSchedules);

/**
 * Gets the periodic receive schedules.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES`.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[out] aNumSchedules  A pointer to return the number of schedules.
 *
 * @returns A pointer to the array of schedules.
 */
const otLinkExtPeriodicRxSchedule *otLinkExtGetPeriodicRxSchedules(otInstance *aInstance, uint8_t *aNumSchedules);

/*
 * MAC frame filter policy flags (see `otLinkExtSetMacFilterPolicy()`).
 */
#define OT_LINK_EXT_MAC_FILTER_POLICY_DROP_MALFORMED (1 << 0)        ///< Drop frames which cannot be parsed.
#define OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNSECURED (1 << 1)        ///< Drop frames without security enabled.
#define OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY (1 << 2)      ///< Drop secured frames whose key is unknown.
#define OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_NEIGHBOR (1 << 3) ///< Drop secured frames from unknown neighbors.
#define OT_LINK_EXT_MAC_FILTER_POLICY_DROP_REPLAYED (1 << 4)         ///< Drop replayed secured frames.

/**
 * Sets the MAC frame filter policy.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * Received frames are checked in the following order, and each check drops the frame only if its flag is set in
 * @p aPolicy. A frame passing all checks is delivered to the raw link-layer receive callback unmodified.
 *
 * 1. `OT_LINK_EXT_MAC_FILTER_POLICY_DROP_MALFORMED`: The frame cannot be parsed. A malformed frame which is not
 *    dropped is delivered without further checks.
 * 2. `OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNSECURED`: The frame does not have security enabled. An unsecured frame
 *    which is not dropped is delivered without further checks.
 * 3. `OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY`: The key of the secured frame is unknown. A key is known if
 *    it matches an entry of the MAC key table (see `otLinkExtSetMacKeyTable()`), or if the frame uses Key ID Mode 1
 *    with the key index of the previous, current, or next MAC key (see `otLinkRawSetMacKey()`).
 * 4. A frame matching an entry without a key is delivered. For a matching entry with a key, the AES-CCM* MIC is
 *    verified using the extended address of each neighbor of that entry matching the frame (see
 *    `otLinkExtSetMacNeighborTable()`) for the nonce. When a neighbor verifies the MIC, the frame is checked for
 *    replay against, and then updates, the neighbor's next expected frame counter. If no neighbor matches, a frame
 *    with an extended source address is verified using that address. Entries may share a Key ID, so all matching
 *    entries are checked. A frame whose MIC verification fails is dropped.
 *    - `OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_NEIGHBOR`: No neighbor matches the frame.
 *    - `OT_LINK_EXT_MAC_FILTER_POLICY_DROP_REPLAYED`: The frame counter is lower than the next expected frame counter
 *      of the neighbor which verified the MIC.
 *
 * The MIC is verified on a copy of the frame. Frames are not decrypted. A policy of zero disables the filter.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aPolicy    The policy bitmask, a combination of `OT_LINK_EXT_MAC_FILTER_POLICY_*` flags.
 *
 * @retval OT_ERROR_NONE             Successfully set the policy.
 * @retval OT_ERROR_INVALID_ARGS     @p aPolicy contains an unknown flag.
 * @retval OT_ERROR_NOT_IMPLEMENTED  @p aPolicy requires the MAC key table, which is disabled
 *                                   (`OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES` is zero).
 */
otError otLinkExtSetMacFilterPolicy(otInstance *aInstance, uint8_t aPolicy);

/**
 * Gets the MAC frame filter policy.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @returns The policy bitmask, a combination of `OT_LINK_EXT_MAC_FILTER_POLICY_*` flags.
 */
uint8_t otLinkExtGetMacFilterPolicy(otInstance *aInstance);

#define OT_LINK_EXT_MAC_KEY_ID_MAX_SIZE 5 ///< Max Key ID size (Key ID Mode 2: 4-byte Key Source and Key Index).

/**
 * Represents an entry of the MAC key table of the MAC frame filter.
 *
 * The Key ID Mode of the entry is determined by `mKeyIdLength`:
 * - 0: Key ID Mode 0 (no Key ID).
 * - 1: Key ID Mode 1 (`mKeyId` is the Key Index).
 * - 5: Key ID Mode 2 (`mKeyId` is the 4-byte Key Source followed by the Key Index).
 */
typedef struct otLinkExtMacKeyTableEntry
{
    uint8_t  mKeyId[OT_LINK_EXT_MAC_KEY_ID_MAX_SIZE]; ///< The Key ID (first `mKeyIdLength` bytes are used).
    uint8_t  mKeyIdLength;                            ///< The Key ID length, which determines the Key ID Mode.
    uint8_t  mAllowedSecurityLevels; ///< Allowed security levels (bit `i` allows level `i`), non-zero, bit 0 clear.
    bool     mHasKey;                ///< Whether `mKey` is set, in which case the MIC is verified.
    otMacKey mKey;                   ///< The key (applicable if `mHasKey`).
} otLinkExtMacKeyTableEntry;

/**
 * Sets the MAC key table of the MAC frame filter.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES`.
 *
 * Atomically replaces all entries. A secured frame matches an entry if its Key ID Mode and Key ID equal those of the
 * entry, and its security level is allowed by the entry (see `otLinkExtSetMacFilterPolicy()`). Setting the key table
 * clears the neighbor table (see `otLinkExtSetMacNeighborTable()`).
 *
 * @param[in] aInstance    A pointer to an OpenThread instance.
 * @param[in] aEntries     A pointer to an array of entries (can be NULL if @p aNumEntries is zero).
 * @param[in] aNumEntries  The number of entries in @p aEntries.
 *
 * @retval OT_ERROR_NONE          Successfully set the table.
 * @retval OT_ERROR_NO_BUFS       More than `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES` entries. The table
 *                                is left unchanged.
 * @retval OT_ERROR_INVALID_ARGS  An entry is invalid. The table is left unchanged.
 */
otError otLinkExtSetMacKeyTable(otInstance *aInstance, const otLinkExtMacKeyTableEntry *aEntries, uint8_t aNumEntries);

/**
 * Gets the MAC key table of the MAC frame filter.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES`.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[out] aNumEntries  A pointer to return the number of entries.
 *
 * @returns A pointer to the array of entries.
 */
const otLinkExtMacKeyTableEntry *otLinkExtGetMacKeyTable(otInstance *aInstance, uint8_t *aNumEntries);

/**
 * Represents an entry of the MAC neighbor table of the MAC frame filter.
 *
 * A frame matches a neighbor when its source address is the neighbor's extended address, or the neighbor's short
 * address if set, or when it has no source address. If the neighbor's PAN ID is set and the frame's source PAN ID is
 * known, the source PAN ID must also be equal to it. The PAN ID scopes the short address, which is only unique within
 * a PAN. Extended addresses are assumed to be unique, so the PAN ID is not a security check.
 */
typedef struct otLinkExtMacNeighborTableEntry
{
    otExtAddress   mExtAddress;    ///< The extended address, which is used for the AES-CCM* nonce.
    uint32_t       mFrameCounter;  ///< The next expected frame counter.
    otPanId        mPanId;         ///< The PAN ID (`OT_PANID_BROADCAST` if not set).
    otShortAddress mShortAddress;  ///< The short address (`OT_RADIO_BROADCAST_SHORT_ADDR` if not set).
    uint8_t        mKeyTableIndex; ///< The index of the MAC key table entry, which MUST have a key.
} otLinkExtMacNeighborTableEntry;

/**
 * Sets the MAC neighbor table of the MAC frame filter.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES`.
 *
 * Atomically replaces all entries. Each extended address and key table index pair MUST be unique. The next expected
 * frame counter of a neighbor is updated when the neighbor verifies a frame. For an entry with the same extended
 * address and key table index as a current entry, the frame counter is set to the larger of the two, so that it never
 * moves backwards.
 *
 * @param[in] aInstance    A pointer to an OpenThread instance.
 * @param[in] aEntries     A pointer to an array of entries (can be NULL if @p aNumEntries is zero).
 * @param[in] aNumEntries  The number of entries in @p aEntries.
 *
 * @retval OT_ERROR_NONE          Successfully set the table.
 * @retval OT_ERROR_NO_BUFS       More than `OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES` entries. The
 *                                table is left unchanged.
 * @retval OT_ERROR_INVALID_ARGS  An entry is invalid, or two entries have the same extended address and key table
 *                                index. The table is left unchanged.
 */
otError otLinkExtSetMacNeighborTable(otInstance                           *aInstance,
                                     const otLinkExtMacNeighborTableEntry *aEntries,
                                     uint8_t                               aNumEntries);

/**
 * Gets the MAC neighbor table of the MAC frame filter.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES`.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[out] aNumEntries  A pointer to return the number of entries.
 *
 * @returns A pointer to the array of entries.
 */
const otLinkExtMacNeighborTableEntry *otLinkExtGetMacNeighborTable(otInstance *aInstance, uint8_t *aNumEntries);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LINK_EXT_H_
