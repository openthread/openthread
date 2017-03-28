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
 *  This file defines the OpenThread IEEE 802.15.4 Link Layer API.
 */

#ifndef OPENTHREAD_LINK_H_
#define OPENTHREAD_LINK_H_

#include "openthread/types.h"
#include "platform/radio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup link  Link
 *
 * @brief
 *   This module includes functions that control link-layer configuration.
 *
 * @{
 *
 */

/**
 * This function pointer is called during an IEEE 802.15.4 Active Scan when an IEEE 802.15.4 Beacon is received or
 * the scan completes.
 *
 * @param[in]  aResult   A valid pointer to the beacon information or NULL when the active scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otHandleActiveScanResult)(otActiveScanResult *aResult, void *aContext);

/**
 * This function starts an IEEE 802.15.4 Active Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called on receiving a beacon or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval kThreadError_None  Accepted the Active Scan request.
 * @retval kThreadError_Busy  Already performing an Active Scan.
 *
 */
OTAPI ThreadError OTCALL otLinkActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                          otHandleActiveScanResult aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Active Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Active Scan is in progress, false otherwise.
 */
OTAPI bool OTCALL otLinkIsActiveScanInProgress(otInstance *aInstance);

/**
 * This function pointer is called during an IEEE 802.15.4 Energy Scan when the result for a channel is ready or the
 * scan completes.
 *
 * @param[in]  aResult   A valid pointer to the energy scan result information or NULL when the energy scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otHandleEnergyScanResult)(otEnergyScanResult *aResult, void *aContext);

/**
 * This function starts an IEEE 802.15.4 Energy Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating on which channels to perform energy scan.
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called to pass on scan result on indicate scan completion.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval kThreadError_None  Accepted the Energy Scan request.
 * @retval kThreadError_Busy  Could not start the energy scan.
 *
 */
OTAPI ThreadError OTCALL otLinkEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                          otHandleEnergyScanResult aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Energy Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Energy Scan is in progress, false otherwise.
 *
 */
OTAPI bool OTCALL otLinkIsEnergyScanInProgress(otInstance *aInstance);

/**
 * This function enqueues an IEEE 802.15.4 Data Request message for transmission.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully enqueued an IEEE 802.15.4 Data Request message.
 * @retval kThreadError_Already       An IEEE 802.15.4 Data Request message is already enqueued.
 * @retval kThreadError_InvalidState  Device is not in rx-off-when-idle mode.
 * @retval kThreadError_NoBufs        Insufficient message buffers available.
 *
 */
OTAPI ThreadError OTCALL otLinkSendDataRequest(otInstance *aInstance);

/**
 * This function indicates whether or not an IEEE 802.15.4 MAC is in the transmit state.
 *
 * MAC module is in the transmit state during CSMA/CA procedure, CCA, Data, Beacon or Data Request frame transmission
 * and receiving an ACK of a transmitted frame. MAC module is not in the transmit state during transmission of an ACK
 * frame or a Beacon Request frame.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 MAC is in the transmit state, false otherwise.
 *
 */
OTAPI bool OTCALL otLinkIsInTransmitState(otInstance *aInstance);

/**
 * Get the IEEE 802.15.4 channel.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 channel.
 *
 * @sa otLinkSetChannel
 */
OTAPI uint8_t OTCALL otLinkGetChannel(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 channel
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aChannel  The IEEE 802.15.4 channel.
 *
 * @retval  kThreadErrorNone         Successfully set the channel.
 * @retval  kThreadErrorInvalidArgs  If @p aChnanel is not in the range [11, 26].
 *
 * @sa otLinkGetChannel
 */
OTAPI ThreadError OTCALL otLinkSetChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Get the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended Address.
 */
OTAPI const uint8_t *OTCALL otLinkGetExtendedAddress(otInstance *aInstance);

/**
 * This function sets the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadError_None         Successfully set the IEEE 802.15.4 Extended Address.
 * @retval kThreadError_InvalidArgs  @p aExtendedAddress was NULL.
 *
 */
OTAPI ThreadError OTCALL otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress);

/**
 * Get the factory-assigned IEEE EUI-64.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 * @param[out]  aEui64     A pointer to where the factory-assigned IEEE EUI-64 is placed.
 *
 */
OTAPI void OTCALL otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64);

/**
 * Get the Joiner ID.
 *
 * Joiner ID is the first 64 bits of the result of computing SHA-256 over factory-assigned
 * IEEE EUI-64, which is used as IEEE 802.15.4 Extended Address during commissioning process.
 *
 * @param[in]   aInstance          A pointer to the OpenThread instance.
 * @param[out]  aHashMacAddress    A pointer to where the Hash Mac Address is placed.
 *
 */
OTAPI void OTCALL otLinkGetJoinerId(otInstance *aInstance, otExtAddress *aHashMacAddress);

/**
 * This function returns the maximum transmit power setting in dBm.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 *
 * @returns  The maximum transmit power setting.
 *
 */
OTAPI int8_t OTCALL otLinkGetMaxTransmitPower(otInstance *aInstance);

/**
 * This function sets the maximum transmit power in dBm.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPower    The maximum transmit power in dBm.
 *
 */
OTAPI void OTCALL otLinkSetMaxTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * Get the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 PAN ID.
 *
 * @sa otLinkSetPanId
 */
OTAPI otPanId OTCALL otLinkGetPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPanId    The IEEE 802.15.4 PAN ID.
 *
 * @retval kThreadErrorNone         Successfully set the PAN ID.
 * @retval kThreadErrorInvalidArgs  If aPanId is not in the range [0, 65534].
 *
 * @sa otLinkGetPanId
 */
OTAPI ThreadError OTCALL otLinkSetPanId(otInstance *aInstance, otPanId aPanId);

/**
 * Get the data poll period of sleepy end device.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns  The data poll period of sleepy end device.
 *
 * @sa otLinkSetPollPeriod
 */
OTAPI uint32_t OTCALL otLinkGetPollPeriod(otInstance *aInstance);

/**
 * Set the data poll period for sleepy end device.
 *
 * @note This function updates only poll period of sleepy end device. To update child timeout the function
 *       otSetChildTimeout() shall be called.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPollPeriod  data poll period.
 *
 * @sa otLinkGetPollPeriod
 */
OTAPI void OTCALL otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod);

/**
 * Enqueue an IEEE 802.15.4 Data Request message.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully enqueued an IEEE 802.15.4 Data Request message.
 * @retval kThreadError_Already       An IEEE 802.15.4 Data Request message is already enqueued.
 * @retval kThreadError_InvalidState  Device is not in rx-off-when-idle mode.
 * @retval kThreadError_NoBufs        Insufficient message buffers available.
 */
OTAPI ThreadError OTCALL otSendMacDataRequest(otInstance *aInstance);

/**
 * Get the IEEE 802.15.4 Short Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Short Address.
 */
OTAPI otShortAddress OTCALL otLinkGetShortAddress(otInstance *aInstance);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC whitelist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC whitelist entry.
 *
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkRemoveWhitelist
 * @sa otLinkClearWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkSetWhitelistEnabled
 */
OTAPI ThreadError OTCALL otLinkAddWhitelist(otInstance *aInstance, const uint8_t *aExtAddr);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC whitelist and fix the RSSI value.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aRssi     The RSSI in dBm to use when receiving messages from aExtAddr.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC whitelist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC whitelist entry.
 *
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkRemoveWhitelist
 * @sa otLinkClearWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkIsWhitelistEnabled
 * @sa otLinkSetWhitelistEnabled
 */
OTAPI ThreadError OTCALL otLinkAddWhitelistRssi(otInstance *aInstance, const uint8_t *aExtAddr, int8_t aRssi);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otLinkAddWhitelist
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkClearWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkIsWhitelistEnabled
 * @sa otLinkSetWhitelistEnabled
 */
OTAPI void OTCALL otLinkRemoveWhitelist(otInstance *aInstance, const uint8_t *aExtAddr);

/**
 * This function gets a MAC whitelist entry.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[in]   aIndex    An index into the MAC whitelist table.
 * @param[out]  aEntry    A pointer to where the information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the MAC whitelist entry.
 * @retval kThreadError_InvalidArgs  @p aIndex is out of bounds or @p aEntry is NULL.
 *
 */
OTAPI ThreadError OTCALL otLinkGetWhitelistEntry(otInstance *aInstance, uint8_t aIndex, otMacWhitelistEntry *aEntry);

/**
 * Remove all entries from the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otLinkAddWhitelist
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkRemoveWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkIsWhitelistEnabled
 * @sa otLinkSetWhitelistEnabled
 */
OTAPI void OTCALL otLinkClearWhitelist(otInstance *aInstance);

/**
 * Enable MAC whitelist filtering.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   TRUE to enable the whitelist, FALSE otherwise.
 *
 * @sa otLinkAddWhitelist
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkRemoveWhitelist
 * @sa otLinkClearWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkIsWhitelistEnabled
 */
OTAPI void OTCALL otLinkSetWhitelistEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function indicates whether or not the MAC whitelist is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if the MAC whitelist is enabled, FALSE otherwise.
 *
 * @sa otLinkAddWhitelist
 * @sa otLinkAddWhitelistRssi
 * @sa otLinkRemoveWhitelist
 * @sa otLinkClearWhitelist
 * @sa otLinkGetWhitelistEntry
 * @sa otLinkSetWhitelistEnabled
 */
OTAPI bool OTCALL otLinkIsWhitelistEnabled(otInstance *aInstance);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC blacklist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC blacklist entry.
 *
 * @sa otLinkRemoveBlacklist
 * @sa otLinkClearBlacklist
 * @sa otLinkGetBlacklistEntry
 * @sa otLinkIsBlacklistEnabled
 * @sa otLinkSetBlacklistEnabled
 */
OTAPI ThreadError OTCALL otLinkAddBlacklist(otInstance *aInstance, const uint8_t *aExtAddr);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otLinkAddBlacklist
 * @sa otLinkClearBlacklist
 * @sa otLinkGetBlacklistEntry
 * @sa otLinkIsBlacklistEnabled
 * @sa otLinkSetBlacklistEnabled
 */
OTAPI void OTCALL otLinkRemoveBlacklist(otInstance *aInstance, const uint8_t *aExtAddr);

/**
 * This function gets a MAC Blacklist entry.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[in]   aIndex    An index into the MAC Blacklist table.
 * @param[out]  aEntry    A pointer to where the information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the MAC Blacklist entry.
 * @retval kThreadError_InvalidArgs  @p aIndex is out of bounds or @p aEntry is NULL.
 *
 */
OTAPI ThreadError OTCALL otLinkGetBlacklistEntry(otInstance *aInstance, uint8_t aIndex, otMacBlacklistEntry *aEntry);

/**
 *  Remove all entries from the MAC Blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otLinkAddBlacklist
 * @sa otLinkRemoveBlacklist
 * @sa otLinkGetBlacklistEntry
 * @sa otLinkIsBlacklistEnabled
 * @sa otLinkSetBlacklistEnabled
 */
OTAPI void OTCALL otLinkClearBlacklist(otInstance *aInstance);

/**
 * Enable MAC Blacklist filtering.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @parma[in]  aEnabled   TRUE to enable the blacklist, FALSE otherwise.
 *
 * @sa otLinkAddBlacklist
 * @sa otLinkRemoveBlacklist
 * @sa otLinkClearBlacklist
 * @sa otLinkGetBlacklistEntry
 * @sa otLinkIsBlacklistEnabled
 */
OTAPI void OTCALL otLinkSetBlacklistEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function indicates whether or not the MAC Blacklist is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if the MAC Blacklist is enabled, FALSE otherwise.
 *
 * @sa otLinkAddBlacklist
 * @sa otLinkRemoveBlacklist
 * @sa otLinkClearBlacklist
 * @sa otLinkGetBlacklistEntry
 * @sa otLinkSetBlacklistEnabled
 *
 */
OTAPI bool OTCALL otLinkIsBlacklistEnabled(otInstance *aInstance);

/**
 * Get the assigned link quality which is on the link to a given extended address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aLinkQuality A pointer to the assigned link quality.
 *
 * @retval kThreadError_None  Successfully retrieved the link quality to aLinkQuality.
 * @retval kThreadError_InvalidState  No attached child matches with a given extended address.
 *
 * @sa otLinkSetAssignLinkQuality
 */
OTAPI ThreadError OTCALL otLinkGetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr,
                                                    uint8_t *aLinkQuality);

/**
 * Set the link quality which is on the link to a given extended address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aLinkQuality  The link quality to be set on the link.
 *
 * @sa otLinkGetAssignLinkQuality
 */
OTAPI void OTCALL otLinkSetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t aLinkQuality);

/**
 * Get the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the MAC layer counters.
 */
OTAPI const otMacCounters *OTCALL otLinkGetCounters(otInstance *aInstance);

/**
 * This function pointer is called when an IEEE 802.15.4 frame is received.
 *
 * @note This callback is called after FCS processing and @p aFrame may not contain the actual FCS that was received.
 *
 * @note This callback is called before IEEE 802.15.4 security processing and mSecurityValid in @p aFrame will
 * always be false.
 *
 * @param[in]  aFrame    A pointer to the received IEEE 802.15.4 frame.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otLinkPcapCallback)(const RadioPacket *aFrame, void *aContext);

/**
 * This function registers a callback to provide received raw IEEE 802.15.4 frames.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otLinkSetPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext);

/**
 * This function indicates whether or not promiscuous mode is enabled at the link layer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval true   Promiscuous mode is enabled.
 * @retval false  Promiscuous mode is not enabled.
 *
 */
bool otLinkIsPromiscuous(otInstance *aInstance);

/**
 * This function enables or disables the link layer promiscuous mode.
 *
 * @note Promiscuous mode may only be enabled when the Thread interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
 *
 * @retval kThreadError_None          Successfully enabled promiscuous mode.
 * @retval kThreadError_InvalidState  Could not enable promiscuous mode because
 *                                    the Thread interface is enabled.
 *
 */
ThreadError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_LINK_H_
