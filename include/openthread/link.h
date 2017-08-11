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
OTAPI const otExtAddress *OTCALL otLinkGetExtendedAddress(otInstance *aInstance);

/**
 * This function sets the IEEE 802.15.4 Extended Address.
 *
 * This function will only succeed when Thread protocols are disabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Extended Address.
 * @retval OT_ERROR_INVALID_ARGS   @p aExtAddress was NULL.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 */
OTAPI otError OTCALL otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

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
 * @returns  The data poll period of sleepy end device in milliseconds.
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
 * @param[in]  aPollPeriod  data poll period in milliseconds.
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
 *
 */
OTAPI otShortAddress OTCALL otLinkGetShortAddress(otInstance *aInstance);

/**
 * Get the Joining Permitted flag used in any subsequently transmitted IEEE 802.15.4 Beacon frames.
 *
 * @param[in]     aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   if the Joining Permitted flag will be set in Beacon frames.
 * @retval FALSE  if the Joining Permitted flag will not be set in Beacon frames.
 *
 */
bool otLinkGetBeaconJoinableFlag(otInstance *aInstance);

/**
 * Set the Joining Permitted flag used in any subsequently transmitted IEEE 802.15.4 Beacon frames.
 *
 * @param[in] aInstance      A pointer to an OpenThread instance.
 * @param[in] aJoinableFlag  The Joining Permitted flag to use in IEEE 802.15.4 Beacon frames.
 *
 */
void otLinkSetBeaconJoinableFlag(otInstance *aInstance, bool aJoinableFlag);

/**
 * This function gets the address mode of MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  the address mode.
 *
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otMacFilterAddressMode OTCALL otLinkFilterGetAddressMode(otInstance *aInstance);

/**
 * This function sets the address mode of MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aMode      The address mode to set.
 *
 * @retval OT_ERROR_NONE           Successfully set the address mode.
 * @retval OT_ERROR_INVALID_ARGS   @p aMode is not valid.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterSetAddressMode(otInstance *aInstance, otMacFilterAddressMode aMode);

/**
 * This method adds an Extended Address to MAC filter.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A reference to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully added @p aExtAddress to MAC filter.
 * @retval OT_ERROR_ALREADY        If @p aExtAddress was already in MAC filter.
 * @retval OT_ERROR_INVALID_ARGS   If @p aExtAddress is NULL.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterAddAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method removes an Extended Address from MAC filter.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A reference to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully removed @p aExtAddress from MAC filter.
 * @retval OT_ERROR_INVALID_ARGS   If @p aExtAddress is NULL.
 * @retval OT_ERROR_NOT_FOUND      @p aExtAddress is not in MAC filter.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterRemoveAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method clears all the Extended Addresses from MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI void OTCALL otLinkFilterClearAddresses(otInstance *aInstance);

/**
 * This method gets an in-use address filter entry.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the MAC filter iterator context. To get the first in-use address filter entry,
 *                           it should be set to OT_MAC_FILTER_ITERATOR_INIT.
 * @param[out]    aEntry     A pointer to where the information is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved an in-use address filter entry.
 * @retval OT_ERROR_INVALID_ARGS  If @p aIterator or @p aEntry is NULL.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterGetNextAddress(otInstance *aInstance,
                                                otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * This method sets the received signal strength (in dBm) for the messages from the Extended Address.
 * The default received signal strength for all received messages would be set if no Extended Address is specified.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address, or NULL to set the default received signal
 *                          strength.
 * @param[in]  aRss         The received signal strength (in dBm) to set.
 *
 * @retval OT_ERROR_NONE           Successfully set @p aRss for @p aExtAddress or set the default @p aRss for all
 *                                 received messages if @p aExtAddress is NULL.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterAddRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss);

/**
 * This method removes the received signal strength setting for the received messages from the Extended Address or
 * removes the default received signal strength setting if no Extended Address is specified.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address, or NULL to reset the default received
 *                          signal strength.
 *
 * @retval OT_ERROR_NONE       Successfully removed received signal strength setting for @p aExtAddress or
 *                             removed the default received signal strength setting if @p aExtAddress is NULL.
 * @retval OT_ERROR_NOT_FOUND  @p aExtAddress is not in MAC filter if it is not NULL.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterRemoveRssIn(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method clears all the received signal strength settings.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
OTAPI void OTCALL otLinkFilterClearRssIn(otInstance *aInstance);

/**
 * This method gets an in-use RssIn filter entry.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A reference to the MAC filter iterator context. To get the first in-use RssIn Filter entry,
 *                           it should be set to OT_MAC_FILTER_ITERATOR_INIT.
 * @param[out]    aEntry     A reference to where the information is placed. The last entry would have the extended
 *                           address as all 0xff to indicate the default received signal strength if it was set.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved an in-use RssIn Filter entry.
 * @retval OT_ERROR_INVALID_ARGS  If @p aIterator or @p aEntry is NULL.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 *
 */
OTAPI otError OTCALL otLinkFilterGetNextRssIn(otInstance *aInstance,
                                              otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * This method converts received signal strength to link quality.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRss       The received signal strength value to be converted.
 *
 * @return Link quality value mapping to @p aRss.
 *
 */
uint8_t otLinkConvertRssToLinkQuality(otInstance *aInstance, int8_t aRss);

/**
 * This method converts link quality to typical received signal strength.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aLinkQuality  LinkQuality value, should be in range [0,3].
 *
 * @return Typical platform received signal strength mapping to @p aLinkQuality.
 *
 */
int8_t otLinkConvertLinkQualityToRss(otInstance *aInstance, uint8_t aLinkQuality);

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
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_LINK_H_
