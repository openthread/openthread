/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *  This file defines the top-level functions for the OpenThread library.
 */

#ifndef OPENTHREAD_H_
#define OPENTHREAD_H_

#include <stdint.h>
#include <stdbool.h>

#include <openthread-types.h>
#include <platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup api  API
 * @brief
 *   This module includes the application programming interface to the OpenThread stack.
 *
 * @{
 *
 * @defgroup execution Execution
 * @defgroup commands Commands
 * @defgroup config Configuration
 * @defgroup diags Diagnostics
 * @defgroup messages Message Buffers
 * @defgroup ip6 IPv6
 * @defgroup udp UDP
 *
 * @}
 *
 */

/**
 * @defgroup platform  Platform Abstraction
 * @brief
 *   This module includes the platform abstraction used by the OpenThread stack.
 *
 * @{
 * @}
 *
 */

/**
 * @defgroup core Core
 * @brief
 *   This module includes the core OpenThread stack.
 *
 * @{
 *
 * @defgroup core-6lowpan 6LoWPAN
 * @defgroup core-coap CoAP
 * @defgroup core-ipv6 IPv6
 * @defgroup core-mac MAC
 * @defgroup core-mesh-forwarding Mesh Forwarding
 * @defgroup core-message Message
 * @defgroup core-mle MLE
 * @defgroup core-netdata Network Data
 * @defgroup core-netif Network Interface
 * @defgroup core-arp RLOC Mapping
 * @defgroup core-security Security
 * @defgroup core-tasklet Tasklet
 * @defgroup core-timer Timer
 * @defgroup core-udp UDP
 * @defgroup core-link-quality Link Quality
 *
 * @}
 *
 */

/**
 * @addtogroup execution  Execution
 *
 * @brief
 *   This module includes functions that control the Thread stack's execution.
 *
 * @{
 *
 */

/**
 * Run the next queued tasklet in OpenThread.
 */
void otProcessNextTasklet(void);

/**
 * Indicates whether or not OpenThread has tasklets pending.
 *
 * @retval TRUE   If there are tasklets pending.
 * @retval FALSE  If there are no tasklets pending.
 */
bool otAreTaskletsPending(void);

/**
 * OpenThread calls this function when the tasklet queue transitions from empty to non-empty.
 *
 */
extern void otSignalTaskletPending(void);

/**
 * @}
 *
 */

/**
 * @addtogroup commands  Commands
 *
 * @brief
 *   This module includes functions for OpenThread commands.
 *
 * @{
 *
 */

/**
 * Get the OpenThread version string.
 *
 * @returns A pointer to the OpenThread version.
 *
 */
const char *otGetVersionString(void);

/**
 * This function initializes the OpenThread library.
 *
 * This function initializes OpenThread and prepares it for subsequent OpenThread API calls.  This function must be
 * called before any other calls to OpenThread.
 *
 * @retval kThreadError_None  Successfully enabled the Thread interface.
 *
 */
ThreadError otEnable(void);

/**
 * This function disables the OpenThread library.
 *
 * Call this function when OpenThread is no longer in use.  The client must call otEnable() to use OpenThread
 * again.
 *
 * @retval kThreadError_None  Successfully disabled the Thread interface.
 *
 */
ThreadError otDisable(void);

/**
 * This function brings up the IPv6 interface.
 *
 * Call this function to bring up the IPv6 interface and enables IPv6 communication.
 *
 * @retval kThreadError_None          Successfully enabled the IPv6 interface.
 * @retval kThreadError_InvalidState  OpenThread is not enabled or the IPv6 interface is already up.
 *
 */
ThreadError otInterfaceUp(void);

/**
 * This function brings down the IPv6 interface.
 *
 * Call this function to bring down the IPv6 interface and disable all IPv6 communication.
 *
 * @retval kThreadError_None          Successfully brought the interface down.
 * @retval kThreadError_InvalidState  The interface was not up.
 *
 */
ThreadError otInterfaceDown(void);

/**
 * This function indicates whether or not the IPv6 interface is up.
 *
 * @retval TRUE   The IPv6 interface is up.
 * @retval FALSE  The IPv6 interface is down.
 *
 */
bool otIsInterfaceUp(void);

/**
 * This function starts Thread protocol operation.
 *
 * The interface must be up when calling this function.
 *
 * @retval kThreadError_None          Successfully started Thread protocol operation.
 * @retval kThreadError_InvalidState  Thread protocol operation is already started or the interface is not up.
 *
 */
ThreadError otThreadStart(void);

/**
 * This function stops Thread protocol operation.
 *
 * @retval kThreadError_None          Successfully stopped Thread protocol operation.
 * @retval kThreadError_InvalidState  The Thread protocol operation was not started.
 *
 */
ThreadError otThreadStop(void);

/**
 * This function indicates whether a node is the only router on the network.
 *
 * @retval TRUE   It is the only router in the network.
 * @retval FALSE  It is a child or is not a single router in the network.
 *
 */
bool otIsSingleton(void);

/**
 * This function pointer is called during an IEEE 802.15.4 Active Scan when an IEEE 802.15.4 Beacon is received or
 * the scan completes.
 *
 * @param[in]  aResult   A valid pointer to the beacon information or NULL when the active scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otHandleActiveScanResult)(otActiveScanResult *aResult, void *aContext);

/**
 * This function starts an IEEE 802.15.4 Active Scan
 *
 * @param[in]  aScanChannels    A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called on receiving a beacon or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval kThreadError_None  Accepted the Active Scan request.
 * @retval kThreadError_Busy  Already performing an Active Scan.
 *
 */
ThreadError otActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, otHandleActiveScanResult aCallback,
                         void *aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Active Scan is currently in progress.
 *
 * @returns true if an IEEE 802.15.4 Active Scan is in progress, false otherwise.
 *
 */
bool otIsActiveScanInProgress(void);

/**
 * This function starts a Thread Discovery scan.
 *
 * @param[in]  aScanChannels     A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aPanId            The PAN ID filter (set to Broadcast PAN to disable filter).
 * @param[in]  aCallback         A pointer to a function called on receiving an MLE Discovery Response or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval kThreadError_None  Accepted the Thread Discovery request.
 * @retval kThreadError_Busy  Already performing an Thread Discovery.
 *
 */
ThreadError otDiscover(uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanid,
                       otHandleActiveScanResult aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not an MLE Thread Discovery is currently in progress.
 *
 * @returns true if an MLE Thread Discovery is in progress, false otherwise.
 *
 */
bool otIsDiscoverInProgress(void);

/**
 * @}
 *
 */

/**
 * @addtogroup config  Configuration
 *
 * @brief
 *   This module includes functions for configuration.
 *
 * @{
 *
 */

/**
 * @defgroup config-general  General
 *
 * @brief
 *   This module includes functions that manage configuration parameters for the Thread Child, Router, and Leader roles.
 *
 * @{
 *
 */

/**
 * Get the IEEE 802.15.4 channel.
 *
 * @returns The IEEE 802.15.4 channel.
 *
 * @sa otSetChannel
 */
uint8_t otGetChannel(void);

/**
 * Set the IEEE 802.15.4 channel
 *
 * @param[in]  aChannel  The IEEE 802.15.4 channel.
 *
 * @retval  kThreadErrorNone         Successfully set the channel.
 * @retval  kThreadErrorInvalidArgs  If @p aChnanel is not in the range [11, 26].
 *
 * @sa otGetChannel
 */
ThreadError otSetChannel(uint8_t aChannel);

/**
 * Get the Thread Child Timeout used when operating in the Child role.
 *
 * @returns The Thread Child Timeout value.
 *
 * @sa otSetChildTimeout
 */
uint32_t otGetChildTimeout(void);

/**
 * Set the Thread Child Timeout used when operating in the Child role.
 *
 * @sa otSetChildTimeout
 */
void otSetChildTimeout(uint32_t aTimeout);

/**
 * Get the IEEE 802.15.4 Extended Address.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended Address.
 */
const uint8_t *otGetExtendedAddress(void);

/**
 * This function sets the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadError_None         Successfully set the IEEE 802.15.4 Extended Address.
 * @retval kThreadError_InvalidArgs  @p aExtendedAddress was NULL.
 *
 */
ThreadError otSetExtendedAddress(const otExtAddress *aExtendedAddress);

/**
 * Get the IEEE 802.15.4 Extended PAN ID.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otSetExtendedPanId
 */
const uint8_t *otGetExtendedPanId(void);

/**
 * Set the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aExtendedPanId  A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otGetExtendedPanId
 */
void otSetExtendedPanId(const uint8_t *aExtendedPanId);

/**
 * This function returns a pointer to the Leader's RLOC.
 *
 * @param[out]  aLeaderRloc  A pointer to where the Leader's RLOC will be written.
 *
 * @retval kThreadError_None         The Leader's RLOC was successfully written to @p aLeaderRloc.
 * @retval kThreadError_InvalidArgs  @p aLeaderRloc was NULL.
 * @retval kThreadError_Detached     Not currently attached to a Thread Partition.
 *
 */
ThreadError otGetLeaderRloc(otIp6Address *aLeaderRloc);

/**
 * Get the MLE Link Mode configuration.
 *
 * @returns The MLE Link Mode configuration.
 *
 * @sa otSetLinkMode
 */
otLinkModeConfig otGetLinkMode(void);

/**
 * Set the MLE Link Mode configuration.
 *
 * @param[in]  aConfig  A pointer to the Link Mode configuration.
 *
 * @retval kThreadErrorNone  Successfully set the MLE Link Mode configuration.
 *
 * @sa otGetLinkMode
 */
ThreadError otSetLinkMode(otLinkModeConfig aConfig);

/**
 * Get the thrMasterKey.
 *
 * @param[out]  aKeyLength  A pointer to an unsigned 8-bit value that the function will set to the number of bytes that
 *                          represent the thrMasterKey. Caller may set to NULL.
 *
 * @returns A pointer to a buffer containing the thrMasterKey.
 *
 * @sa otSetMasterKey
 */
const uint8_t *otGetMasterKey(uint8_t *aKeyLength);

/**
 * Set the thrMasterKey.
 *
 * @param[in]  aKey        A pointer to a buffer containing the thrMasterKey.
 * @param[in]  aKeyLength  Number of bytes representing the thrMasterKey stored at aKey. Valid range is [0, 16].
 *
 * @retval kThreadErrorNone         Successfully set the thrMasterKey.
 * @retval kThreadErrorInvalidArgs  If aKeyLength is larger than 16.
 *
 * @sa otGetMasterKey
 */
ThreadError otSetMasterKey(const uint8_t *aKey, uint8_t aKeyLength);

/**
 * This function returns the maximum transmit power setting in dBm.
 *
 * @returns  The maximum transmit power setting.
 *
 */
int8_t otGetMaxTransmitPower(void);

/**
 * This function sets the maximum transmit power in dBm.
 *
 * @param[in]  aPower  The maximum transmit power in dBm.
 *
 */
void otSetMaxTransmitPower(int8_t aPower);

/**
 * This function returns a pointer to the Mesh Local EID.
 *
 * @returns A pointer to the Mesh Local EID.
 *
 */
const otIp6Address *otGetMeshLocalEid(void);

/**
 * This function returns a pointer to the Mesh Local Prefix.
 *
 * @returns A pointer to the Mesh Local Prefix.
 *
 */
const uint8_t *otGetMeshLocalPrefix(void);

/**
 * This function sets the Mesh Local Prefix.
 *
 * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix.
 *
 * @retval kThreadError_None  Successfully set the Mesh Local Prefix.
 *
 */
ThreadError otSetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix);

/**
 * This method provides a full or stable copy of the Leader's Thread Network Data.
 *
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
ThreadError otGetNetworkDataLeader(bool aStable, uint8_t *aData, uint8_t *aDataLength);

/**
 * This method provides a full or stable copy of the local Thread Network Data.
 *
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
ThreadError otGetNetworkDataLocal(bool aStable, uint8_t *aData, uint8_t *aDataLength);

/**
 * Get the Thread Network Name.
 *
 * @returns A pointer to the Thread Network Name.
 *
 * @sa otSetNetworkName
 */
const char *otGetNetworkName(void);

/**
 * Set the Thread Network Name.
 *
 * @param[in]  aNetworkName  A pointer to the Thread Network Name.
 *
 * @retval kThreadErrorNone  Successfully set the Thread Network Name.
 *
 * @sa otGetNetworkName
 */
ThreadError otSetNetworkName(const char *aNetworkName);

/**
 * This function gets the next On Mesh Prefix in the Network Data.
 *
 * @param[in]     aLocal     TRUE to retrieve from the local Network Data, FALSE for partition's Network Data
 * @param[inout]  aIterator  A pointer to the Network Data iterator context.
 * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
 *
 * @retval kThreadError_None      Successfully found the next On Mesh prefix.
 * @retval kThreadError_NotFound  No subsequent On Mesh prefix exists in the Thread Network Data.
 *
 */
ThreadError otGetNextOnMeshPrefix(bool aLocal, otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig);

/**
 * Get the IEEE 802.15.4 PAN ID.
 *
 * @returns The IEEE 802.15.4 PAN ID.
 *
 * @sa otSetPanId
 */
otPanId otGetPanId(void);

/**
 * Set the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aPanId  The IEEE 802.15.4 PAN ID.
 *
 * @retval kThreadErrorNone         Successfully set the PAN ID.
 * @retval kThreadErrorInvalidArgs  If aPanId is not in the range [0, 65534].
 *
 * @sa otGetPanId
 */
ThreadError otSetPanId(otPanId aPanId);

/**
 * This function indicates whether or not the Router Role is enabled.
 *
 * @retval TRUE   If the Router Role is enabled.
 * @retval FALSE  If the Router Role is not enabled.
 *
 */
bool otIsRouterRoleEnabled(void);

/**
 * This function sets whether or not the Router Role is enabled.
 *
 * @param[in]  aEnabled  TRUE if the Router Role is enabled, FALSE otherwise.
 *
 */
void otSetRouterRoleEnabled(bool aEnabled);

/**
 * Get the IEEE 802.15.4 Short Address.
 *
 * @returns A pointer to the IEEE 802.15.4 Short Address.
 */
otShortAddress otGetShortAddress(void);

/**
 * Get the list of IPv6 addresses assigned to the Thread interface.
 *
 * @returns A pointer to the first Network Interface Address.
 */
const otNetifAddress *otGetUnicastAddresses(void);

/**
 * Add a Network Interface Address to the Thread interface.
 *
 * The passed in instance @p aAddress will be added and stored by the Thread interface, so the caller should ensure
 * that the address instance remains valid (not de-alloacted) and is not modified after a successful call to this
 * method.
 *
 * @param[in]  aAddress  A pointer to a Network Interface Address.
 *
 * @retval kThreadErrorNone  Successfully added the Network Interface Address.
 * @retval kThreadErrorBusy  The Network Interface Address pointed to by @p aAddress is already added.
 */
ThreadError otAddUnicastAddress(otNetifAddress *aAddress);

/**
 * Remove a Network Interface Address from the Thread interface.
 *
 * @param[in]  aAddress  A pointer to a Network Interface Address.
 *
 * @retval kThreadErrorNone      Successfully removed the Network Interface Address.
 * @retval kThreadErrorNotFound  The Network Interface Address point to by @p aAddress was not added.
 */
ThreadError otRemoveUnicastAddress(otNetifAddress *aAddress);

/**
 * This function pointer is called to notify certain configuration or state changes within OpenThread.
 *
 * @param[in]  aFlags    A bit-field indicating specific state that has changed.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otStateChangedCallback)(uint32_t aFlags, void *aContext);

/**
 * This function registers a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aCallback  A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 */
void otSetStateChangedCallback(otStateChangedCallback aCallback, void *aContext);

/**
 * This function gets the Active Operational Dataset.
 *
 * @param[out]  aDataset  A pointer to where the Active Operational Dataset will be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the Active Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
ThreadError otGetActiveDataset(otOperationalDataset *aDataset);

/**
 * This function sets the Active Operational Dataset.
 *
 * @param[in]  aDataset  A pointer to the Active Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Active Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Active Operational Datset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
ThreadError otSetActiveDataset(otOperationalDataset *aDataset);

/**
 * This function gets the Pending Operational Dataset.
 *
 * @param[out]  aDataset  A pointer to where the Pending Operational Dataset will be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the Pending Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
ThreadError otGetPendingDataset(otOperationalDataset *aDataset);

/**
 * This function sets the Pending Operational Dataset.
 *
 * @param[in]  aDataset  A pointer to the Pending Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Pending Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Pending Operational Datset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
ThreadError otSetPendingDataset(otOperationalDataset *aDataset);

/**
 * Get the data poll period of sleepy end deivce.
 *
 * @returns  The data poll period of sleepy end device.
 *
 * @sa otSetPollPeriod
 */
uint32_t otGetPollPeriod(void);

/**
 * Set the data poll period for sleepy end deivce.
 *
 * param[in]  aPollPeriod  data poll period.
 *
 * @sa otGetPollPeriod
 */
void otSetPollPeriod(uint32_t aPollPeriod);

/**
 * @}
 */

/**
 * @defgroup config-router  Router/Leader
 *
 * @brief
 *   This module includes functions that manage configuration parameters for the Thread Router and Leader roles.
 *
 * @{
 *
 */

/**
 * Get the Thread Leader Weight used when operating in the Leader role.
 *
 * @returns The Thread Leader Weight value.
 *
 * @sa otSetLeaderWeight
 */
uint8_t otGetLocalLeaderWeight(void);

/**
 * Set the Thread Leader Weight used when operating in the Leader role.
 *
 * @param[in]  aWeight  The Thread Leader Weight value..
 *
 * @sa otGetLeaderWeight
 */
void otSetLocalLeaderWeight(uint8_t aWeight);

/**
 * Get the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @returns The Thread Leader Partition Id value.
 *
 */
uint32_t otGetLocalLeaderPartitionId(void);

/**
 * Set the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @param[in]  aPartitionId  The Thread Leader Partition Id value.
 *
 */
void otSetLocalLeaderPartitionId(uint32_t aPartitionId);

/**
 * @}
 */

/**
 * @defgroup config-br  Border Router
 *
 * @brief
 *   This module includes functions that manage configuration parameters that apply to the Thread Border Router role.
 *
 * @{
 *
 */

/**
 * Add a border router configuration to the local network data.
 *
 * @param[in]  aConfig  A pointer to the border router configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveBorderRouter
 * @sa otSendServerData
 */
ThreadError otAddBorderRouter(const otBorderRouterConfig *aConfig);

/**
 * Remove a border router configuration from the local network data.
 *
 * @param [in]  aPrefix  A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddBorderRouter
 * @sa otSendServerData
 */
ThreadError otRemoveBorderRouter(const otIp6Prefix *aPrefix);

/**
 * Add an external route configuration to the local network data.
 *
 * @param[in]  aConfig  A pointer to the external route configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveExternalRoute
 * @sa otSendServerData
 */
ThreadError otAddExternalRoute(const otExternalRouteConfig *aConfig);

/**
 * Remove an external route configuration from the local network data.
 *
 * @param[in]  aPrefix  A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddExternalRoute
 * @sa otSendServerData
 */
ThreadError otRemoveExternalRoute(const otIp6Prefix *aPrefix);

/**
 * Immediately register the local network data with the Leader.
 *
 *
 * retval kThreadErrorNone  Successfully queued a Server Data Request message for delivery.
 *
 * @sa otAddBorderRouter
 * @sa otRemoveBorderRouter
 * @sa otAddExternalRoute
 * @sa otRemoveExternalRoute
 */
ThreadError otSendServerData(void);

/**
 * This function adds a port to the allowed unsecured port list.
 *
 * @param[in]  aPort  The port value.
 *
 * @retval kThreadError_None    The port was successfully added to the allowed unsecure port list.
 * @retval kThreadError_NoBufs  The unsecure port list is full.
 *
 */
ThreadError otAddUnsecurePort(uint16_t aPort);

/**
 * This function removes a port from the allowed unsecure port list.
 *
 * @param[in]  aPort  The port value.
 *
 * @retval kThreadError_None      The port was successfully removed from the allowed unsecure port list.
 * @retval kThreadError_NotFound  The port was not found in the unsecure port list.
 *
 */
ThreadError otRemoveUnsecurePort(uint16_t aPort);

/**
 * This function returns a pointer to the unsecure port list.
 *
 * @note Port value 0 is used to indicate an invalid entry.
 *
 * @param[out]  aNumEntries  The number of entries in the list.
 *
 * @returns A pointer to the unsecure port list.
 *
 */
const uint16_t *otGetUnsecurePorts(uint8_t *aNumEntries);

/**
 * @}
 *
 */

/**
 * @defgroup config-test  Test
 *
 * @brief
 *   This module includes functions that manage configuration parameters required for Thread Certification testing.
 *
 * @{
 *
 */

/**
 * Get the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @returns The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otSetContextIdReuseDelay
 */
uint32_t otGetContextIdReuseDelay(void);

/**
 * Set the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @param[in]  aDelay  The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otGetContextIdReuseDelay
 */
void otSetContextIdReuseDelay(uint32_t aDelay);

/**
 * Get the thrKeySequenceCounter.
 *
 * @returns The thrKeySequenceCounter value.
 *
 * @sa otSetKeySequenceCounter
 */
uint32_t otGetKeySequenceCounter(void);

/**
 * Set the thrKeySequenceCounter.
 *
 * @param[in]  aKeySequenceCounter  The thrKeySequenceCounter value.
 *
 * @sa otGetKeySequenceCounter
 */
void otSetKeySequenceCounter(uint32_t aKeySequenceCounter);

/**
 * Get the NETWORK_ID_TIMEOUT parameter used in the Router role.
 *
 * @returns The NETWORK_ID_TIMEOUT value.
 *
 * @sa otSetNetworkIdTimeout
 */
uint8_t otGetNetworkIdTimeout(void);

/**
 * Set the NETWORK_ID_TIMEOUT parameter used in the Leader role.
 *
 * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
 *
 * @sa otGetNetworkIdTimeout
 */
void otSetNetworkIdTimeout(uint8_t aTimeout);

/**
 * Get the ROUTER_UPGRADE_THRESHOLD parameter used in the REED role.
 *
 * @returns The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otSetRouterUpgradeThreshold
 */
uint8_t otGetRouterUpgradeThreshold(void);

/**
 * Set the ROUTER_UPGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otGetRouterUpgradeThreshold
 */
void otSetRouterUpgradeThreshold(uint8_t aThreshold);

/**
 * Release a Router ID that has been allocated by the device in the Leader role.
 *
 * @param[in]  aRouterId  The Router ID to release. Valid range is [0, 62].
 *
 * @retval kThreadErrorNone  Successfully released the Router ID specified by aRouterId.
 */
ThreadError otReleaseRouterId(uint8_t aRouterId);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC whitelist.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC whitelist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC whitelist entry.
 *
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
ThreadError otAddMacWhitelist(const uint8_t *aExtAddr);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC whitelist and fix the RSSI value.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aRssi     The RSSI in dBm to use when receiving messages from aExtAddr.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC whitelist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC whitelist entry.
 *
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
ThreadError otAddMacWhitelistRssi(const uint8_t *aExtAddr, int8_t aRssi);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC whitelist.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
void otRemoveMacWhitelist(const uint8_t *aExtAddr);

/**
 * This function gets a MAC whitelist entry.
 *
 * @param[in]   aIndex  An index into the MAC whitelist table.
 * @param[out]  aEntry  A pointer to where the information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the MAC whitelist entry.
 * @retval kThreadError_InvalidArgs  @p aIndex is out of bounds or @p aEntry is NULL.
 *
 */
ThreadError otGetMacWhitelistEntry(uint8_t aIndex, otMacWhitelistEntry *aEntry);

/**
 *  Remove all entries from the MAC whitelist.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
void otClearMacWhitelist(void);

/**
 * Disable MAC whitelist filtering.
 *
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otEnableMacWhitelist
 */
void otDisableMacWhitelist(void);

/**
 * Enable MAC whitelist filtering.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 */
void otEnableMacWhitelist(void);

/**
 * This function indicates whether or not the MAC whitelist is enabled.
 *
 * @returns TRUE if the MAC whitelist is enabled, FALSE otherwise.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 *
 */
bool otIsMacWhitelistEnabled(void);

/**
 * Detach from the Thread network.
 *
 * @retval kThreadErrorNone    Successfully detached from the Thread network.
 * @retval kThreadErrorBusy    Thread is disabled.
 */
ThreadError otBecomeDetached(void);

/**
 * Attempt to reattach as a child.
 *
 * @param[in]  aFilter  Identifies whether to join any, same, or better partition.
 *
 * @retval kThreadErrorNone    Successfully begin attempt to become a child.
 * @retval kThreadErrorBusy    Thread is disabled or in the middle of an attach process.
 */
ThreadError otBecomeChild(otMleAttachFilter aFilter);

/**
 * Attempt to become a router.
 *
 * @retval kThreadErrorNone    Successfully begin attempt to become a router.
 * @retval kThreadErrorBusy    Thread is disabled or already operating in a router or leader role.
 */
ThreadError otBecomeRouter(void);

/**
 * Become a leader and start a new partition.
 *
 * @retval kThreadErrorNone  Successfully became a leader and started a new partition.
 */
ThreadError otBecomeLeader(void);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC blacklist.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval kThreadErrorNone    Successfully added to the MAC blacklist.
 * @retval kThreadErrorNoBufs  No buffers available for a new MAC blacklist entry.
 *
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 */
ThreadError otAddMacBlacklist(const uint8_t *aExtAddr);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC blacklist.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otAddMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 */
void otRemoveMacBlacklist(const uint8_t *aExtAddr);

/**
 * This function gets a MAC Blacklist entry.
 *
 * @param[in]   aIndex  An index into the MAC Blacklist table.
 * @param[out]  aEntry  A pointer to where the information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the MAC Blacklist entry.
 * @retval kThreadError_InvalidArgs  @p aIndex is out of bounds or @p aEntry is NULL.
 *
 */
ThreadError otGetMacBlacklistEntry(uint8_t aIndex, otMacBlacklistEntry *aEntry);

/**
 *  Remove all entries from the MAC Blacklist.
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 */
void otClearMacBlacklist(void);

/**
 * Disable MAC blacklist filtering.
 *
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otEnableMacBlacklist
 */
void otDisableMacBlacklist(void);

/**
 * Enable MAC Blacklist filtering.
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 */
void otEnableMacBlacklist(void);

/**
 * This function indicates whether or not the MAC Blacklist is enabled.
 *
 * @returns TRUE if the MAC Blacklist is enabled, FALSE otherwise.
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 *
 */
bool otIsMacBlacklistEnabled(void);

/**
 * Get the assigned link quality which is on the link to a given extended address.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aLinkQuality A pointer to the assigned link quality.
 *
 * @retval kThreadError_None  Successfully retrieved the link quality to aLinkQuality.
 * @retval kThreadError_InvalidState  No attached child matches with a given extended address.
 *
 * @sa otSetAssignLinkQuality
 */
ThreadError otGetAssignLinkQuality(const uint8_t *aExtAddr, uint8_t *aLinkQuality);

/**
 * Set the link quality which is on the link to a given extended address.
 *
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aLinkQuality  The link quality to be set on the link.
 *
 * @sa otGetAssignLinkQuality
 */
void otSetAssignLinkQuality(const uint8_t *aExtAddr, uint8_t aLinkQuality);

/**
 * This method triggers platform reset.
 */
void otPlatformReset(void);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

/**
 * @addtogroup diags  Diagnostics
 *
 * @brief
 *   This module includes functions that expose internal state.
 *
 * @{
 *
 */

/**
 * The function retains diagnostic information for an attached Child by its Child ID or RLOC16.
 *
 * @param[in]   aChildId    The Child ID or RLOC16 for the attached child.
 * @param[out]  aChildInfo  A pointer to where the child information is placed.
 *
 */
ThreadError otGetChildInfoById(uint16_t aChildId, otChildInfo *aChildInfo);

/**
 * The function retains diagnostic information for an attached Child by the internal table index.
 *
 * @param[in]   aChildIndex  The table index.
 * @param[out]  aChildInfo   A pointer to where the child information is placed.
 *
 */
ThreadError otGetChildInfoByIndex(uint8_t aChildIndex, otChildInfo *aChildInfo);

/**
 * Get the device role.
 *
 * @retval ::kDeviceRoleDisabled  The Thread stack is disabled.
 * @retval ::kDeviceRoleDetached  The device is not currently participating in a Thread network/partition.
 * @retval ::kDeviceRoleChild     The device is currently operating as a Thread Child.
 * @retval ::kDeviceRoleRouter    The device is currently operating as a Thread Router.
 * @retval ::kDeviceRoleLeader    The device is currently operating as a Thread Leader.
 */
otDeviceRole otGetDeviceRole(void);

/**
 * This function gets an EID cache entry.
 *
 * @param[in]   aIndex  An index into the EID cache table.
 * @param[out]  aEntry  A pointer to where the EID information is placed.
 *
 * @retval kThreadError_None         Successfully retreieved the EID cache entry.
 * @retval kThreadError_InvalidArgs  @p aIndex was out of bounds or @p aEntry was NULL.
 *
 */
ThreadError otGetEidCacheEntry(uint8_t aIndex, otEidCacheEntry *aEntry);

/**
 * This function get the Thread Leader Data.
 *
 * @param[out]  aLeaderData  A pointer to where the leader data is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the leader data.
 * @retval kThreadError_Detached     Not currently attached.
 * @retval kThreadError_InvalidArgs  @p aLeaderData is NULL.
 *
 */
ThreadError otGetLeaderData(otLeaderData *aLeaderData);

/**
 * Get the Leader's Router ID.
 *
 * @returns The Leader's Router ID.
 */
uint8_t otGetLeaderRouterId(void);

/**
 * Get the Leader's Weight.
 *
 * @returns The Leader's Weight.
 */
uint8_t otGetLeaderWeight(void);

/**
 * Get the Network Data Version.
 *
 * @returns The Network Data Version.
 */
uint8_t otGetNetworkDataVersion(void);

/**
 * Get the Partition ID.
 *
 * @returns The Partition ID.
 */
uint32_t otGetPartitionId(void);

/**
 * Get the RLOC16.
 *
 * @returns The RLOC16.
 */
uint16_t otGetRloc16(void);

/**
 * Get the current Router ID Sequence.
 *
 * @returns The Router ID Sequence.
 */
uint8_t otGetRouterIdSequence(void);

/**
 * The function retains diagnostic information for a given Thread Router.
 *
 * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
 * @param[out]  aRouterInfo  A pointer to where the router information is placed.
 *
 */
ThreadError otGetRouterInfo(uint16_t aRouterId, otRouterInfo *aRouterInfo);

/**
 * The function retains diagnostic information for a Thread Router as parent.
 *
 * @param[out]  aParentInfo  A pointer to where the parent router information is placed.
 *
 */
ThreadError otGetParentInfo(otRouterInfo *aParentInfo);

/**
 * Get the Stable Network Data Version.
 *
 * @returns The Stable Network Data Version.
 */
uint8_t otGetStableNetworkDataVersion(void);

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
 * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otSetLinkPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext);

/**
 * This function indicates whether or not promiscuous mode is enabled at the link layer.
 *
 * @retval true   Promiscuous mode is enabled.
 * @retval false  Promiscuous mode is not enabled.
 *
 */
bool otIsLinkPromiscuous(void);

/**
 * This function enables or disables the link layer promiscuous mode.
 *
 * @note Promiscuous mode may only be enabled when the Thread interface is disabled.
 *
 * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
 *
 * @retval kThreadError_None  Successfully enabled promiscuous mode.
 * @retval kThreadError_Busy  Could not enable promiscuous mode because the Thread interface is enabled.
 *
 */
ThreadError otSetLinkPromiscuous(bool aPromiscuous);

/**
 * Get the MAC layer counters.
 *
 * @returns A pointer to the MAC layer counters.
 */
const otMacCounters *otGetMacCounters(void);

/**
 * @}
 *
 */

/**
 * Test if two IPv6 addresses are the same.
 *
 * @param[in]  a  A pointer to the first IPv6 address to compare.
 * @param[in]  b  A pointer to the second IPv6 address to compare.
 *
 * @retval TRUE   The two IPv6 addresses are the same.
 * @retval FALSE  The two IPv6 addresses are not the same.
 */
bool otIsIp6AddressEqual(const otIp6Address *a, const otIp6Address *b);

/**
 * Convert a human-readable IPv6 address string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aAddress  A pointer to an IPv6 address.
 *
 * @retval kThreadErrorNone        Successfully parsed the string.
 * @retval kThreadErrorInvalidArg  Failed to parse the string.
 */
ThreadError otIp6AddressFromString(const char *aString, otIp6Address *aAddress);

/**
 * @addtogroup messages  Message Buffers
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers
 *
 * @{
 *
 */

/**
 * Free an allocated message buffer.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval kThreadErrorNone  Successfully freed the message buffer.
 *
 * @sa otNewUdpMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otFreeMessage(otMessage aMessage);

/**
 * Get the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message length in bytes.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 * @sa otSetMessageLength
 */
uint16_t otGetMessageLength(otMessage aMessage);

/**
 * Set the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aLength   A length in bytes.
 *
 * @retval kThreadErrorNone    Successfully set the message length.
 * @retval kThreadErrorNoBufs  No available buffers to grow the message.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageLength(otMessage aMessage, uint16_t aLength);

/**
 * Get the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message offset value.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
uint16_t otGetMessageOffset(otMessage aMessage);

/**
 * Set the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 *
 * @retval kThreadErrorNone        Successfully set the message offset.
 * @retval kThreadErrorInvalidArg  The offset is beyond the message length.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageOffset(otMessage aMessage, uint16_t aOffset);

/**
 * Append bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aBuf      A pointer to the data to append.
 * @param[in]  aLength   Number of bytes to append.
 *
 * @returns The number of bytes appended.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
int otAppendMessage(otMessage aMessage, const void *aBuf, uint16_t aLength);

/**
 * Read bytes from a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are read to.
 * @param[in]  aLength   Number of bytes to read.
 *
 * @returns The number of bytes read.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otWriteMessage
 */
int otReadMessage(otMessage aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength);

/**
 * Write bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are written from.
 * @param[in]  aLength   Number of bytes to write.
 *
 * @returns The number of bytes written.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 */
int otWriteMessage(otMessage aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength);

/**
 * @}
 *
 */

/**
 * @addtogroup ip6  IPv6
 *
 * @brief
 *   This module includes functions that control IPv6 communication.
 *
 * @{
 *
 */

/**
 * This function pointer is called when an IPv6 datagram is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otReceiveIp6DatagramCallback)(otMessage aMessage, void *aContext);

/**
 * This function registers a callback to provide received IPv6 datagrams.
 *
 * By default, this callback does not pass Thread control traffic.  See otSetReceiveIp6FilterEnabled() to change
 * the Thread control traffic filter setting.
 *
 * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received
 *                               or NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @sa otIsReceiveIp6FilterEnabled
 * @sa otSetReceiveIp6FilterEnabled
 *
 */
void otSetReceiveIp6DatagramCallback(otReceiveIp6DatagramCallback aCallback, void *aCallbackContext);


/**
 * This function indicates whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otSetReceiveIp6DatagramCallback().
 *
 * @returns  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otSetReceiveDatagramCallback
 * @sa otSetReceiveIp6FilterEnabled
 *
 */
bool otIsReceiveIp6DatagramFilterEnabled(void);

/**
 * This function sets whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otSetReceiveIp6DatagramCallback().
 *
 * @param[in]  aEnabled  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otSetReceiveDatagramCallback
 * @sa otIsReceiveIp6FilterEnabled
 *
 */
void otSetReceiveIp6DatagramFilterEnabled(bool aEnabled);

/**
 * This function sends an IPv6 datagram via the Thread interface.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the IPv6 datagram.
 *
 */
ThreadError otSendIp6Datagram(otMessage aMessage);

/**
 * This function indicates whether or not ICMPv6 Echo processing is enabled.
 *
 * @retval TRUE   ICMPv6 Echo processing is enabled.
 * @retval FALSE  ICMPv6 Echo processing is disabled.
 *
 */
bool otIsIcmpEchoEnabled(void);

/**
 * This function sets whether or not ICMPv6 Echo processing is enabled.
 *
 * @param[in]  aEnabled  TRUE to enable ICMPv6 Echo processing, FALSE otherwise.
 *
 */
void otSetIcmpEchoEnabled(bool aEnabled);

/**
 * This function returns the prefix match length (bits) for two IPv6 addresses.
 *
 * @param[in]  aFirst   A pointer to the first IPv6 address.
 * @param[in]  aSecond  A pointer to the second IPv6 address.
 *
 * @returns  The prefix match length in bits.
 *
 */
uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond);

/**
 * @}
 *
 */

/**
 * @addtogroup udp  UDP
 *
 * @brief
 *   This module includes functions that control UDP communication.
 *
 * @{
 *
 */

/**
 * Allocate a new message buffer for sending a UDP message.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available.
 *
 * @sa otFreeMessage
 */
otMessage otNewUdpMessage(void);

/**
 * Open a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aCallback  A pointer to the application callback function.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * @retval kThreadErrorNone  Successfully opened the socket.
 * @retval kThreadErrorBusy  Socket is already opened.
 *
 * @sa otNewUdpMessage
 * @sa otCloseUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otOpenUdpSocket(otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext);

/**
 * Close a UDP/IPv6 socket.
 *
 * @param[in]  aSocket  A pointer to a UDP socket structure.
 *
 * @retval kThreadErrorNone  Successfully closed the socket.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otCloseUdpSocket(otUdpSocket *aSocket);

/**
 * Bind a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval kThreadErrorNone  Bind operation was successful.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otCloseUdpSocket
 * @sa otSendUdp
 */
ThreadError otBindUdpSocket(otUdpSocket *aSocket, otSockAddr *aSockName);

/**
 * Send a UDP/IPv6 message.
 *
 * @param[in]  aSocket       A pointer to a UDP socket structure.
 * @param[in]  aMessage      A pointer to a message buffer.
 * @param[in]  aMessageInfo  A pointer to a message info structure.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otCloseUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otSendUdp(otUdpSocket *aSocket, otMessage aMessage, const otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_H_
