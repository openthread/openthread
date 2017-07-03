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

#include <openthread/types.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-link-link
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
 * @retval OT_ERROR_NONE  Accepted the Active Scan request.
 * @retval OT_ERROR_BUSY  Already performing an Active Scan.
 *
 */
OTAPI otError OTCALL otLinkActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
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
 * @retval OT_ERROR_NONE  Accepted the Energy Scan request.
 * @retval OT_ERROR_BUSY  Could not start the energy scan.
 *
 */
OTAPI otError OTCALL otLinkEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
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
 * @retval OT_ERROR_NONE           Successfully enqueued an IEEE 802.15.4 Data Request message.
 * @retval OT_ERROR_ALREADY        An IEEE 802.15.4 Data Request message is already enqueued.
 * @retval OT_ERROR_INVALID_STATE  Device is not in rx-off-when-idle mode.
 * @retval OT_ERROR_NO_BUFS        Insufficient message buffers available.
 *
 */
OTAPI otError OTCALL otLinkSendDataRequest(otInstance *aInstance);

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
 * This function will only succeed when Thread protocols are disabled.  A successful
 * call to this function will also invalidate the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aChannel  The IEEE 802.15.4 channel.
 *
 * @retval  OT_ERROR_NONE           Successfully set the channel.
 * @retval  OT_ERROR_INVALID_ARGS   If @p aChnanel is not in the range [11, 26].
 * @retval  OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otLinkGetChannel
 */
OTAPI otError OTCALL otLinkSetChannel(otInstance *aInstance, uint8_t aChannel);

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
 * This function will only succeed when Thread protocols are disabled.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Extended Address.
 * @retval OT_ERROR_INVALID_ARGS   @p aExtendedAddress was NULL.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 */
OTAPI otError OTCALL otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress);

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
 * This function will only succeed when Thread protocols are disabled.  A successful
 * call to this function will also invalidate the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPanId    The IEEE 802.15.4 PAN ID.
 *
 * @retval OT_ERROR_NONE           Successfully set the PAN ID.
 * @retval OT_ERROR_INVALID_ARGS   If aPanId is not in the range [0, 65534].
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otLinkGetPanId
 */
OTAPI otError OTCALL otLinkSetPanId(otInstance *aInstance, otPanId aPanId);

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
 * Get the IEEE 802.15.4 Short Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Short Address.
 */
OTAPI otShortAddress OTCALL otLinkGetShortAddress(otInstance *aInstance);

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
typedef void (*otLinkPcapCallback)(const otRadioFrame *aFrame, void *aContext);

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
 * @retval OT_ERROR_NONE           Successfully enabled promiscuous mode.
 * @retval OT_ERROR_INVALID_STATE  Could not enable promiscuous mode because
 *                                 the Thread interface is enabled.
 *
 */
otError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous);

/**
 * This function gets the AddressFilter state.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @returns  the AddressFilter State.
 *
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterClearEntries
 * @sa otLinkAddressFilterReset
 *
 */
uint8_t otLinkAddressFilterGetState(otInstance *aInstance);

/**
 * This function sets the AddressFilter state.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aState        The AddressFilter state to set.
 *
 * @retval OT_ERROR_NONE           Successfully set the AddressFilter State.
 * @retval OT_ERROR_INVALID_STATE  The AddressFilter is in use.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterClearEntries
 * @sa otLinkAddressFilterReset
 *
 */
otError otLinkAddressFilterSetState(otInstance *aInstance, uint8_t aState);

/**
 * This method adds an Extended Address to the address filter.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully added the Extended Address to the filter.
 * @retval OT_ERROR_ALREADY        The Extended Address is already in the filter.
 * @retval OT_ERROR_NO_BUFS        No available filter enty.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterClearEntries
 * @sa otLinkAddressFilterReset
 *
 */
otError otLinkAddressFilterAddEntry(otInstance *aInstance, const otExtAddress *aAddress);

/**
 * This method removes an Extended Address from the filter.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aAddress      A pointer to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully removed the MAC Filter entry.
 * @retval OT_ERROR_NOT_FOUND      The Extended Address is not filtered.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterClearEntries
 * @sa otLinkAddressFilterReset
 *
 */
otError otLinkAddressFilterRemoveEntry(otInstance *aInstance, const otExtAddress *aAddress);

/**
 * This method removes all address filter entries from the filter.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_INVALID_STATE  The AddressFilter is not in use.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterReset
 *
 */
otError otLinkAddressFilterClearEntries(otInstance *aInstance);

/**
 * This method resets address filter which would be disabled and cleared.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterClearEntries
 *
 */
void otLinkAddressFilterReset(otInstance *aInstance);

/**
 * This method sets the default fixed LinkQualityIn value for all the received messages.
 *
 * @param[in] aInstance       A pointer to an OpenThread instance.
 * @param[in] aLinkQualityIn  The LinkQualityIn value.
 *
 * @retval OT_ERROR_NONE          Successfully set the default fixed LinkQualityIn.
 * @retval OT_ERROR_INVALID_ARGS  The @p aLinkQualityIn is not valid.
 *
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 */
otError otLinkLinkQualityInFilterSet(otInstance *aInstance, uint8_t aLinkQuality);

/**
 * This method gets the default fixed LinkQualityIn value for all the received messages if any
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in] aLinkQualityIn  A pointer to where the information is placed.
 *
 * @retval OT_ERROR_NONE          Successfully set the default fixed LinkQualityIn.
 * @retval OT_ERROR_NOT_FOUND     No default fixed LinkQualityIn is set.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 *
 */

otError otLinkLinkQualityInFilterGet(otInstance *aInstance, uint8_t *aLinkQuality);

/**
 * This method unsets the default fixed LinkQualityIn value for all the received messages if any.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 *
 */
void otLinkLinkQualityInFilterUnset(otInstance *aInstance);

/**
 * This method sets the LinkQualityIn for the Extended Address with a fixed value.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aAddress        A pointer to the Extended Address.
 * @param[in]  aLinkQualityIn  A fixed LinkQualityIn value to set.
 *
 * @retval OT_ERROR_NONE          The default fixed LinkQualityIn is set and written to @p aLinkQualityIn.
 * @retval OT_ERROR_INVALID_ARGS  The @p aLinkQualityIn is not valid.
 * @retval OT_ERROR_NO_BUFS       No available filter enty.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 *
 */
otError otLinkLinkQualityInFilterAddEntry(otInstance *aInstance, const otExtAddress *aAddress, uint8_t aLinkQuality);

/**
 * This method removes the fixed LinkQualityIn setting from the Extended Address.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aAddress        A pointer to the Extended Address.
 *
 * @retval OT_ERROR_NONE       Successfully removed the fixed LinkQualityIn for @p aAddress
 * @retval OT_ERROR_NOT_FOUND  No fixed LinkQualityIn for @p aAddress.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 *
 */
otError otLinkLinkQualityInFilterRemoveEntry(otInstance *aInstance, const otExtAddress *aAddress);

/**
 * This method removes LinkQualityIn filter entry if any.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterReset
 *
 */
void otLinkLinkQualityInFilterClearEntries(otInstance *aInstance);

/**
 * This method resets LinkQualityIn filter.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 *
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 *
 */
void otLinkLinkQualityInFilterReset(otInstance *aInstance);

/**
 * This method gets an in-use Filter entry.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aIterator  A pointer to the MAC filter iterator context. To get the first in-use filter entry,
 *                        it should be set to OT_MAC_FILTER_ITEERATOR_INIT.
 * @param[out]  aEntry    A pointer to where the information is placed.
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the in-use Filter entry.
 * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
 *
 * @sa otLinkAddressFilterGetState
 * @sa otLinkAddressFilterSetState
 * @sa otLinkAddressFilterAddEntry
 * @sa otLinkAddressFilterRemoveEntry
 * @sa otLinkAddressFilterClearEntries
 * @sa otLinkAddressFilterReset
 * @sa otLinkLinkQualityInFilterSet
 * @sa otLinkLinkQualityInFilterGet
 * @sa otLinkLinkQualityInFilterUnset
 * @sa otLinkLinkQualityInFilterAddEntry
 * @sa otLinkLinkQualityInFilterRemoveEntry
 * @sa otLinkLinkQualityInFilterClearEntries
 * @sa otLinkLinkQualityInFilterReset
 *
 */
otError otLinkFilterGetNextEntry(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_LINK_H_
