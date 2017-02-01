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
 *  This file defines the top-level functions for the OpenThread library.
 */

#ifndef OPENTHREAD_H_
#define OPENTHREAD_H_

#include <openthread-types.h>

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
 * @defgroup coap CoAP
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
 * @defgroup core-global-address Global IPv6 Address
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
 * @defgroup core-tcp TCP
 * @defgroup core-link-quality Link Quality
 *
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
OTAPI const char *OTCALL otGetVersionString(void);

#ifdef OTDLL

/**
 * This function initializes a new instance of the OpenThread library.
 *
 * @retval otApiInstance*  The new OpenThread context structure.
 *
 * @sa otApiFinalize
 *
 */
OTAPI otApiInstance *OTCALL otApiInit(void);

/**
 * This function uninitializes the OpenThread library.
 *
 * Call this function when OpenThread is no longer in use.
 *
 * @param[in]  aApiInstance  The OpenThread api instance.
 *
 */
OTAPI void OTCALL otApiFinalize(otApiInstance *aApiInstance);

/**
 * This function frees any memory returned/allocated by the library.
 *
 * @param[in] aMem  The memory to free.
 *
 */
OTAPI void OTCALL otFreeMemory(const void *aMem);

/**
 * This function pointer is called to notify addition and removal of OpenThread devices.
 *
 * @param[in]  aAdded       A flag indicating if the device was added or removed.
 * @param[in]  aDeviceGuid  A GUID indicating which device state changed.
 * @param[in]  aContext     A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otDeviceAvailabilityChangedCallback)(bool aAdded, const GUID *aDeviceGuid, void *aContext);

/**
 * This function registers a callback to indicate OpenThread devices come and go.
 *
 * @param[in]  aApiInstance     The OpenThread api instance.
 * @param[in]  aCallback        A pointer to a function that is called with the state changes.
 * @param[in]  aContextContext  A pointer to application-specific context.
 *
 */
OTAPI void OTCALL otSetDeviceAvailabilityChangedCallback(otApiInstance *aApiInstance,
                                                         otDeviceAvailabilityChangedCallback aCallback, void *aCallbackContext);

/**
 * This function querys the list of OpenThread device contexts on the system.
 *
 * @param[in]  aApiInstance     The OpenThread api instance.
 *
 * @sa otFreeMemory
 */
OTAPI otDeviceList *OTCALL otEnumerateDevices(otApiInstance *aApiInstance);

/**
 * This function initializes an OpenThread context for a device.
 *
 * @param[in]  aApiInstance  The OpenThread api instance.
 * @param[in]  aDeviceGuid   The device guid to create an OpenThread context for.
 *
 * @retval otInstance*  The new OpenThread device instance structure for the device.
 *
 * @sa otFreeMemory
 *
 */
OTAPI otInstance *OTCALL otInstanceInit(otApiInstance *aApiInstance, const GUID *aDeviceGuid);

/**
 * This queries the Windows device/interface GUID for the otContext.
 *
 * @param[in] aContext  The OpenThread context structure.
 *
 * @retval GUID  The device GUID.
 *
 */
OTAPI GUID OTCALL otGetDeviceGuid(otInstance *aInstance);

/**
 * This queries the Windows device/interface IfIndex for the otContext.
 *
 * @param[in] aContext  The OpenThread context structure.
 *
 * @retval uint32_t  The device IfIndex.
 *
 */
OTAPI uint32_t OTCALL otGetDeviceIfIndex(otInstance *aInstance);

/**
 * This queries the Windows Compartment ID for the otContext.
 *
 * @param[in] aContext  The OpenThread context structure.
 *
 * @retval uint32_t  The compartment ID.
 *
 */
OTAPI uint32_t OTCALL otGetCompartmentId(otInstance *aInstance);

#else

#ifdef OPENTHREAD_MULTIPLE_INSTANCE
/**
 * This function initializes the OpenThread library.
 *
 * This function initializes OpenThread and prepares it for subsequent OpenThread API calls.  This function must be
 * called before any other calls to OpenThread. By default, OpenThread is initialized in the 'enabled' state.
 *
 * @param[in]    aInstanceBuffer      The buffer for OpenThread to use for allocating the otInstance structure.
 * @param[inout] aInstanceBufferSize  On input, the size of aInstanceBuffer. On output, if not enough space for otInstance,
                                      the number of bytes required for otInstance.
 *
 * @retval otInstance*  The new OpenThread instance structure.
 *
 * @sa otContextFinalize
 *
 */
otInstance *otInstanceInit(void *aInstanceBuffer, size_t *aInstanceBufferSize);
#else
/**
 * This function initializes the static instance of the OpenThread library.
 *
 * This function initializes OpenThread and prepares it for subsequent OpenThread API calls.  This function must be
 * called before any other calls to OpenThread. By default, OpenThread is initialized in the 'enabled' state.
 *
 * @retval otInstance*  The new OpenThread instance structure.
 *
 */
otInstance *otInstanceInit(void);
#endif

/**
 * This function disables the OpenThread library.
 *
 * Call this function when OpenThread is no longer in use.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
void otInstanceFinalize(otInstance *aInstance);

#endif

/**
 * This function brings up the IPv6 interface.
 *
 * Call this function to bring up the IPv6 interface and enables IPv6 communication.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully enabled the IPv6 interface,
 *                                    or the interface was already enabled.
 *
 */
OTAPI ThreadError OTCALL otInterfaceUp(otInstance *aInstance);

/**
 * This function brings down the IPv6 interface.
 *
 * Call this function to bring down the IPv6 interface and disable all IPv6 communication.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully brought the interface down,
 *                                    or the interface was already down.
 *
 */
OTAPI ThreadError OTCALL otInterfaceDown(otInstance *aInstance);

/**
 * This function indicates whether or not the IPv6 interface is up.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   The IPv6 interface is up.
 * @retval FALSE  The IPv6 interface is down.
 *
 */
OTAPI bool OTCALL otIsInterfaceUp(otInstance *aInstance);

/**
 * This function starts Thread protocol operation.
 *
 * The interface must be up when calling this function.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully started Thread protocol operation.
 * @retval kThreadError_InvalidState  The network interface was not not up.
 *
 */
OTAPI ThreadError OTCALL otThreadStart(otInstance *aInstance);

/**
 * This function stops Thread protocol operation.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None          Successfully stopped Thread protocol operation.
 *
 */
OTAPI ThreadError OTCALL otThreadStop(otInstance *aInstance);

/**
 * This function indicates whether a node is the only router on the network.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   It is the only router in the network.
 * @retval FALSE  It is a child or is not a single router in the network.
 *
 */
OTAPI bool OTCALL otIsSingleton(otInstance *aInstance);

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
OTAPI ThreadError OTCALL otActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                      otHandleActiveScanResult aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Active Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Active Scan is in progress, false otherwise.
 */
OTAPI bool OTCALL otIsActiveScanInProgress(otInstance *aInstance);

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
OTAPI ThreadError OTCALL otEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                      otHandleEnergyScanResult aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Energy Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Energy Scan is in progress, false otherwise.
 *
 */
OTAPI bool OTCALL otIsEnergyScanInProgress(otInstance *aInstance);

/**
 * This function starts a Thread Discovery scan.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
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
OTAPI ThreadError OTCALL otDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                    uint16_t aPanid,
                                    otHandleActiveScanResult aCallback, void *aCallbackContext);

/**
 * This function determines if an MLE Thread Discovery is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
OTAPI bool OTCALL otIsDiscoverInProgress(otInstance *aInstance);

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
OTAPI ThreadError OTCALL otSendMacDataRequest(otInstance *aInstance);

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
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 channel.
 *
 * @sa otSetChannel
 */
OTAPI uint8_t OTCALL otGetChannel(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 channel
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aChannel  The IEEE 802.15.4 channel.
 *
 * @retval  kThreadErrorNone         Successfully set the channel.
 * @retval  kThreadErrorInvalidArgs  If @p aChnanel is not in the range [11, 26].
 *
 * @sa otGetChannel
 */
OTAPI ThreadError OTCALL otSetChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Get the maximum number of children currently allowed.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of children currently allowed.
 *
 * @sa otSetMaxAllowedChildren
 */
OTAPI uint8_t OTCALL otGetMaxAllowedChildren(otInstance *aInstance);

/**
 * Set the maximum number of children currently allowed.
 *
 * This parameter can only be set when Thread protocol operation
 * has been stopped.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMaxChildren  The maximum allowed children.
 *
 * @retval  kThreadErrorNone           Successfully set the max.
 * @retval  kThreadError_InvalidArgs   If @p aMaxChildren is not in the range [1, OPENTHREAD_CONFIG_MAX_CHILDREN].
 * @retval  kThreadError_InvalidState  If Thread isn't stopped.
 *
 * @sa otGetMaxAllowedChildren, otThreadStop
 */
OTAPI ThreadError OTCALL otSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren);

/**
 * Get the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Child Timeout value.
 *
 * @sa otSetChildTimeout
 */
OTAPI uint32_t OTCALL otGetChildTimeout(otInstance *aInstance);

/**
 * Set the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otSetChildTimeout
 */
OTAPI void OTCALL otSetChildTimeout(otInstance *aInstance, uint32_t aTimeout);

/**
 * Get the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended Address.
 */
OTAPI const uint8_t *OTCALL otGetExtendedAddress(otInstance *aInstance);

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
OTAPI ThreadError OTCALL otSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress);

/**
 * Get the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otSetExtendedPanId
 */
OTAPI const uint8_t *OTCALL otGetExtendedPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aExtendedPanId  A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otGetExtendedPanId
 */
OTAPI void OTCALL otSetExtendedPanId(otInstance *aInstance, const uint8_t *aExtendedPanId);

/**
 * Get the factory-assigned IEEE EUI-64.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 * @param[out]  aEui64     A pointer to where the factory-assigned IEEE EUI-64 is placed.
 *
 */
OTAPI void OTCALL otGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64);

/**
 * Get the Hash Mac Address.
 *
 * Hash Mac Address is the first 64 bits of the result of computing SHA-256 over factory-assigned
 * IEEE EUI-64, which is used as IEEE 802.15.4 Extended Address during commissioning process.
 *
 * @param[in]   aInstance          A pointer to the OpenThread instance.
 * @param[out]  aHashMacAddress    A pointer to where the Hash Mac Address is placed.
 *
 */
OTAPI void OTCALL otGetHashMacAddress(otInstance *aInstance, otExtAddress *aHashMacAddress);

/**
 * This function returns a pointer to the Leader's RLOC.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderRloc  A pointer to where the Leader's RLOC will be written.
 *
 * @retval kThreadError_None         The Leader's RLOC was successfully written to @p aLeaderRloc.
 * @retval kThreadError_InvalidArgs  @p aLeaderRloc was NULL.
 * @retval kThreadError_Detached     Not currently attached to a Thread Partition.
 *
 */
OTAPI ThreadError OTCALL otGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc);

/**
 * Get the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The MLE Link Mode configuration.
 *
 * @sa otSetLinkMode
 */
OTAPI otLinkModeConfig OTCALL otGetLinkMode(otInstance *aInstance);

/**
 * Set the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the Link Mode configuration.
 *
 * @retval kThreadErrorNone  Successfully set the MLE Link Mode configuration.
 *
 * @sa otGetLinkMode
 */
OTAPI ThreadError OTCALL otSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig);

/**
 * Get the thrMasterKey.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[out]  aKeyLength  A pointer to an unsigned 8-bit value that the function will set to the number of bytes that
 *                          represent the thrMasterKey. Caller may set to NULL.
 *
 * @returns A pointer to a buffer containing the thrMasterKey.
 *
 * @sa otSetMasterKey
 */
OTAPI const uint8_t *OTCALL otGetMasterKey(otInstance *aInstance, uint8_t *aKeyLength);

/**
 * Set the thrMasterKey.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aKey        A pointer to a buffer containing the thrMasterKey.
 * @param[in]  aKeyLength  Number of bytes representing the thrMasterKey stored at aKey. Valid range is [0, 16].
 *
 * @retval kThreadErrorNone         Successfully set the thrMasterKey.
 * @retval kThreadErrorInvalidArgs  If aKeyLength is larger than 16.
 *
 * @sa otGetMasterKey
 */
OTAPI ThreadError OTCALL otSetMasterKey(otInstance *aInstance, const uint8_t *aKey, uint8_t aKeyLength);

/**
 * This function returns the maximum transmit power setting in dBm.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 *
 * @returns  The maximum transmit power setting.
 *
 */
OTAPI int8_t OTCALL otGetMaxTransmitPower(otInstance *aInstance);

/**
 * This function sets the maximum transmit power in dBm.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPower    The maximum transmit power in dBm.
 *
 */
OTAPI void OTCALL otSetMaxTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * This function returns a pointer to the Mesh Local EID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local EID.
 *
 */
OTAPI const otIp6Address *OTCALL otGetMeshLocalEid(otInstance *aInstance);

/**
 * This function returns a pointer to the Mesh Local Prefix.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local Prefix.
 *
 */
OTAPI const uint8_t *OTCALL otGetMeshLocalPrefix(otInstance *aInstance);

/**
 * This function sets the Mesh Local Prefix.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix.
 *
 * @retval kThreadError_None  Successfully set the Mesh Local Prefix.
 *
 */
OTAPI ThreadError OTCALL otSetMeshLocalPrefix(otInstance *aInstance, const uint8_t *aMeshLocalPrefix);

/**
 * This method provides a full or stable copy of the Leader's Thread Network Data.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
OTAPI ThreadError OTCALL otGetNetworkDataLeader(otInstance *aInstance, bool aStable, uint8_t *aData,
                                                uint8_t *aDataLength);

/**
 * This method provides a full or stable copy of the local Thread Network Data.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
OTAPI ThreadError OTCALL otGetNetworkDataLocal(otInstance *aInstance, bool aStable, uint8_t *aData,
                                               uint8_t *aDataLength);

/**
 * Get the Thread Network Name.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Network Name.
 *
 * @sa otSetNetworkName
 */
OTAPI const char *OTCALL otGetNetworkName(otInstance *aInstance);

/**
 * Set the Thread Network Name.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aNetworkName  A pointer to the Thread Network Name.
 *
 * @retval kThreadErrorNone  Successfully set the Thread Network Name.
 *
 * @sa otGetNetworkName
 */
OTAPI ThreadError OTCALL otSetNetworkName(otInstance *aInstance, const char *aNetworkName);

/**
 * This function gets the next On Mesh Prefix in the Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[in]     aLocal     TRUE to retrieve from the local Network Data, FALSE for partition's Network Data
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first on-mesh entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
 *
 * @retval kThreadError_None      Successfully found the next On Mesh prefix.
 * @retval kThreadError_NotFound  No subsequent On Mesh prefix exists in the Thread Network Data.
 *
 */
OTAPI ThreadError OTCALL otGetNextOnMeshPrefix(otInstance *aInstance, bool aLocal, otNetworkDataIterator *aIterator,
                                               otBorderRouterConfig *aConfig);

/**
 * Get the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 PAN ID.
 *
 * @sa otSetPanId
 */
OTAPI otPanId OTCALL otGetPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPanId    The IEEE 802.15.4 PAN ID.
 *
 * @retval kThreadErrorNone         Successfully set the PAN ID.
 * @retval kThreadErrorInvalidArgs  If aPanId is not in the range [0, 65534].
 *
 * @sa otGetPanId
 */
OTAPI ThreadError OTCALL otSetPanId(otInstance *aInstance, otPanId aPanId);

/**
 * This function indicates whether or not the Router Role is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   If the Router Role is enabled.
 * @retval FALSE  If the Router Role is not enabled.
 *
 */
OTAPI bool OTCALL otIsRouterRoleEnabled(otInstance *aInstance);

/**
 * This function sets whether or not the Router Role is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE if the Router Role is enabled, FALSE otherwise.
 *
 */
OTAPI void OTCALL otSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Get the IEEE 802.15.4 Short Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Short Address.
 */
OTAPI otShortAddress OTCALL otGetShortAddress(otInstance *aInstance);

/**
 * Get the list of IPv6 addresses assigned to the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the first Network Interface Address.
 */
OTAPI const otNetifAddress *OTCALL otGetUnicastAddresses(otInstance *aInstance);

/**
 * Add a Network Interface Address to the Thread interface.
 *
 * The passed in instance @p aAddress will be copied by the Thread interface. The Thread interface only
 * supports a fixed number of externally added unicast addresses. See OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to a Network Interface Address.
 *
 * @retval kThreadErrorNone          Successfully added (or updated) the Network Interface Address.
 * @retval kThreadError_InvalidArgs  The IP Address indicated by @p aAddress is an internal address.
 * @retval kThreadError_NoBufs       The Network Interface is already storing the maximum allowed external addresses.
 */
OTAPI ThreadError OTCALL otAddUnicastAddress(otInstance *aInstance, const otNetifAddress *aAddress);

/**
 * Remove a Network Interface Address from the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to an IP Address.
 *
 * @retval kThreadErrorNone          Successfully removed the Network Interface Address.
 * @retval kThreadError_InvalidArgs  The IP Address indicated by @p aAddress is an internal address.
 * @retval kThreadError_NotFound     The IP Address indicated by @p aAddress was not found.
 */
OTAPI ThreadError OTCALL otRemoveUnicastAddress(otInstance *aInstance, const otIp6Address *aAddress);

/**
 * This function pointer is called to notify certain configuration or state changes within OpenThread.
 *
 * @param[in]  aFlags    A bit-field indicating specific state that has changed.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otStateChangedCallback)(uint32_t aFlags, void *aContext);

/**
 * This function registers a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * @retval kThreadError_None    Added the callback to the list of callbacks.
 * @retval kThreadError_NoBufs  Could not add the callback due to resource constraints.
 *
 */
OTAPI ThreadError OTCALL otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback,
                                                   void *aContext);

/**
 * This function removes a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 */
OTAPI void OTCALL otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback,
                                              void *aCallbackContext);

/**
 * This function gets the Active Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Active Operational Dataset will be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the Active Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otGetActiveDataset(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Active Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Active Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Active Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Active Operational Datset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otSetActiveDataset(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function indicates whether a valid network is present in the Active Operational Dataset or not.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if a valid network is present in the Active Operational Dataset, FALSE otherwise.
 *
 */
OTAPI bool OTCALL otIsNodeCommissioned(otInstance *aInstance);

/**
 * This function gets the Pending Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Pending Operational Dataset will be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the Pending Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otGetPendingDataset(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Pending Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Pending Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Pending Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Pending Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otSetPendingDataset(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function sends MGMT_ACTIVE_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvTypes  A pointer to the TLV Types.
 * @param[in]  aLength    The length of TLV Types.
 * @param[in]  aAddress   A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendActiveGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                         const otIp6Address *aAddress);

/**
 * This function sends MGMT_ACTIVE_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendActiveSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                         const uint8_t *aTlvs, uint8_t aLength);

/**
 * This function sends MGMT_PENDING_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvTypes  A pointer to the TLV Types.
 * @param[in]  aLength    The length of TLV Types.
 * @param[in]  aAddress   A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendPendingGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                          const otIp6Address *aAddress);

/**
 * This function sends MGMT_PENDING_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendPendingSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                          const uint8_t *aTlvs, uint8_t aLength);

/**
 * Get the data poll period of sleepy end device.
 *
 * @note This function updates only poll period of sleepy end device. To update child timeout the function
 *       otGetChildTimeout() shall be called.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns  The data poll period of sleepy end device.
 *
 * @sa otSetPollPeriod
 */
OTAPI uint32_t OTCALL otGetPollPeriod(otInstance *aInstance);

/**
 * Set the data poll period for sleepy end device.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPollPeriod  data poll period.
 *
 * @sa otGetPollPeriod
 */
OTAPI void OTCALL otSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod);

/**
 * Set the preferred Router Id.
 *
 * Upon becoming a router/leader the node attempts to use this Router Id. If the
 * preferred Router Id is not set or if it can not be used, a randomly generated
 * router id is picked. This property can be set only when the device role is
 * either detached or disabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aRouterId    The preferred Router Id.
 *
 * @retval kThreadError_None         Successfully set the preferred Router Id.
 * @retval kThreadError_InvalidState Could not set (role is not detached or disabled)
 *
 */
ThreadError otSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId);

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
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Leader Weight value.
 *
 * @sa otSetLeaderWeight
 */
OTAPI uint8_t OTCALL otGetLocalLeaderWeight(otInstance *aInstance);

/**
 * Set the Thread Leader Weight used when operating in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aWeight   The Thread Leader Weight value..
 *
 * @sa otGetLeaderWeight
 */
OTAPI void OTCALL otSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight);

/**
 * Get the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Leader Partition Id value.
 *
 */
OTAPI uint32_t OTCALL otGetLocalLeaderPartitionId(otInstance *aInstance);

/**
 * Set the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPartitionId  The Thread Leader Partition Id value.
 *
 */
OTAPI void OTCALL otSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId);

/**
 * Get the Joiner UDP Port.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The Joiner UDP Port number.
 *
 * @sa otSetJoinerUdpPort
 */
OTAPI uint16_t OTCALL otGetJoinerUdpPort(otInstance *aInstance);

/**
 * Set the Joiner UDP Port
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aJoinerUdpPort  The Joiner UDP Port number.
 *
 * @retval  kThreadErrorNone   Successfully set the Joiner UDP Port.
 *
 * @sa otGetJoinerUdpPort
 */
OTAPI ThreadError OTCALL otSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort);

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
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the border router configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveBorderRouter
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otAddBorderRouter(otInstance *aInstance, const otBorderRouterConfig *aConfig);

/**
 * Remove a border router configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddBorderRouter
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otRemoveBorderRouter(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * Add an external route configuration to the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the external route configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveExternalRoute
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otAddExternalRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig);

/**
 * Remove an external route configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddExternalRoute
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otRemoveExternalRoute(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * Immediately register the local network data with the Leader.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * retval kThreadErrorNone  Successfully queued a Server Data Request message for delivery.
 *
 * @sa otAddBorderRouter
 * @sa otRemoveBorderRouter
 * @sa otAddExternalRoute
 * @sa otRemoveExternalRoute
 */
OTAPI ThreadError OTCALL otSendServerData(otInstance *aInstance);

/**
 * This function adds a port to the allowed unsecured port list.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPort     The port value.
 *
 * @retval kThreadError_None    The port was successfully added to the allowed unsecure port list.
 * @retval kThreadError_NoBufs  The unsecure port list is full.
 *
 */
ThreadError otAddUnsecurePort(otInstance *aInstance, uint16_t aPort);

/**
 * This function removes a port from the allowed unsecure port list.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPort     The port value.
 *
 * @retval kThreadError_None      The port was successfully removed from the allowed unsecure port list.
 * @retval kThreadError_NotFound  The port was not found in the unsecure port list.
 *
 */
ThreadError otRemoveUnsecurePort(otInstance *aInstance, uint16_t aPort);

/**
 * This function returns a pointer to the unsecure port list.
 *
 * @note Port value 0 is used to indicate an invalid entry.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aNumEntries  The number of entries in the list.
 *
 * @returns A pointer to the unsecure port list.
 *
 */
const uint16_t *otGetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries);

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
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otSetContextIdReuseDelay
 */
OTAPI uint32_t OTCALL otGetContextIdReuseDelay(otInstance *aInstance);

/**
 * Set the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDelay    The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otGetContextIdReuseDelay
 */
OTAPI void OTCALL otSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay);

/**
 * Get the thrKeySequenceCounter.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySequenceCounter value.
 *
 * @sa otSetKeySequenceCounter
 */
OTAPI uint32_t OTCALL otGetKeySequenceCounter(otInstance *aInstance);

/**
 * Set the thrKeySequenceCounter.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySequenceCounter  The thrKeySequenceCounter value.
 *
 * @sa otGetKeySequenceCounter
 */
OTAPI void OTCALL otSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter);

/**
 * Get the thrKeySwitchGuardTime
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otSetKeySwitchGuardTime
 */
OTAPI uint32_t OTCALL otGetKeySwitchGuardTime(otInstance *aInstance);

/**
 * Set the thrKeySwitchGuardTime
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySwitchGuardTime  The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otGetKeySwitchGuardTime
 */
OTAPI void OTCALL otSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime);


/**
 * Get the NETWORK_ID_TIMEOUT parameter used in the Router role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The NETWORK_ID_TIMEOUT value.
 *
 * @sa otSetNetworkIdTimeout
 */
OTAPI uint8_t OTCALL otGetNetworkIdTimeout(otInstance *aInstance);

/**
 * Set the NETWORK_ID_TIMEOUT parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
 *
 * @sa otGetNetworkIdTimeout
 */
OTAPI void OTCALL otSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout);

/**
 * Get the ROUTER_UPGRADE_THRESHOLD parameter used in the REED role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otSetRouterUpgradeThreshold
 */
OTAPI uint8_t OTCALL otGetRouterUpgradeThreshold(otInstance *aInstance);

/**
 * Set the ROUTER_UPGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otGetRouterUpgradeThreshold
 */
OTAPI void OTCALL otSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold);

/**
 * Release a Router ID that has been allocated by the device in the Leader role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRouterId  The Router ID to release. Valid range is [0, 62].
 *
 * @retval kThreadErrorNone  Successfully released the Router ID specified by aRouterId.
 */
OTAPI ThreadError OTCALL otReleaseRouterId(otInstance *aInstance, uint8_t aRouterId);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
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
OTAPI ThreadError OTCALL otAddMacWhitelist(otInstance *aInstance, const uint8_t *aExtAddr);

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
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
OTAPI ThreadError OTCALL otAddMacWhitelistRssi(otInstance *aInstance, const uint8_t *aExtAddr, int8_t aRssi);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
OTAPI void OTCALL otRemoveMacWhitelist(otInstance *aInstance, const uint8_t *aExtAddr);

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
OTAPI ThreadError OTCALL otGetMacWhitelistEntry(otInstance *aInstance, uint8_t aIndex, otMacWhitelistEntry *aEntry);

/**
 * Remove all entries from the MAC whitelist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 * @sa otEnableMacWhitelist
 */
OTAPI void OTCALL otClearMacWhitelist(otInstance *aInstance);

/**
 * Disable MAC whitelist filtering.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otEnableMacWhitelist
 */
OTAPI void OTCALL otDisableMacWhitelist(otInstance *aInstance);

/**
 * Enable MAC whitelist filtering.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otAddMacWhitelist
 * @sa otAddMacWhitelistRssi
 * @sa otRemoveMacWhitelist
 * @sa otClearMacWhitelist
 * @sa otGetMacWhitelistEntry
 * @sa otDisableMacWhitelist
 */
OTAPI void OTCALL otEnableMacWhitelist(otInstance *aInstance);

/**
 * This function indicates whether or not the MAC whitelist is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
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
OTAPI bool OTCALL otIsMacWhitelistEnabled(otInstance *aInstance);

/**
 * Detach from the Thread network.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone          Successfully detached from the Thread network.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otBecomeDetached(otInstance *aInstance);

/**
 * Attempt to reattach as a child.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aFilter   Identifies whether to join any, same, or better partition.
 *
 * @retval kThreadErrorNone          Successfully begin attempt to become a child.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otBecomeChild(otInstance *aInstance, otMleAttachFilter aFilter);

/**
 * Attempt to become a router.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone         Successfully begin attempt to become a router.
 * @retval kThreadErrorInvalidState Thread is disabled.
 */
OTAPI ThreadError OTCALL otBecomeRouter(otInstance *aInstance);

/**
 * Become a leader and start a new partition.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone          Successfully became a leader and started a new partition.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otBecomeLeader(otInstance *aInstance);

/**
 * Add an IEEE 802.15.4 Extended Address to the MAC blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
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
OTAPI ThreadError OTCALL otAddMacBlacklist(otInstance *aInstance, const uint8_t *aExtAddr);

/**
 * Remove an IEEE 802.15.4 Extended Address from the MAC blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @sa otAddMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 */
OTAPI void OTCALL otRemoveMacBlacklist(otInstance *aInstance, const uint8_t *aExtAddr);

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
OTAPI ThreadError OTCALL otGetMacBlacklistEntry(otInstance *aInstance, uint8_t aIndex, otMacBlacklistEntry *aEntry);

/**
 *  Remove all entries from the MAC Blacklist.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 * @sa otEnableMacBlacklist
 */
OTAPI void OTCALL otClearMacBlacklist(otInstance *aInstance);

/**
 * Disable MAC blacklist filtering.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otEnableMacBlacklist
 */
OTAPI void OTCALL otDisableMacBlacklist(otInstance *aInstance);

/**
 * Enable MAC Blacklist filtering.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otAddMacBlacklist
 * @sa otRemoveMacBlacklist
 * @sa otClearMacBlacklist
 * @sa otGetMacBlacklistEntry
 * @sa otDisableMacBlacklist
 */
OTAPI void OTCALL otEnableMacBlacklist(otInstance *aInstance);

/**
 * This function indicates whether or not the MAC Blacklist is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
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
OTAPI bool OTCALL otIsMacBlacklistEnabled(otInstance *aInstance);

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
 * @sa otSetAssignLinkQuality
 */
OTAPI ThreadError OTCALL otGetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t *aLinkQuality);

/**
 * Set the link quality which is on the link to a given extended address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aExtAddr  A pointer to the IEEE 802.15.4 Extended Address.
 * @param[in]  aLinkQuality  The link quality to be set on the link.
 *
 * @sa otGetAssignLinkQuality
 */
OTAPI void OTCALL otSetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t aLinkQuality);

/**
 * This method triggers a platform reset.
 *
 * The reset process ensures that all the OpenThread state/info (stored in volatile memory) is erased. Note that the
 * `otPlatformReset` does not erase any persistent state/info saved in non-volatile memory.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
OTAPI void OTCALL otPlatformReset(otInstance *aInstance);

/**
 * This method deletes all the settings stored on non-volatile memory, and then triggers platform reset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
OTAPI void OTCALL otFactoryReset(otInstance *aInstance);

/**
 * This function erases all the OpenThread persistent info (network settings) stored on non-volatile memory.
 * Erase is successful only if the device is in `disabled` state/role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None  All persistent info/state was erased successfully.
 * @retval kThreadError_InvalidState  Device is not in `disabled` state/role.
 *
 */
ThreadError otPersistentInfoErase(otInstance *aInstance);

/**
 * Get the ROUTER_DOWNGRADE_THRESHOLD parameter used in the Router role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
 *
 * @sa otSetRouterDowngradeThreshold
 */
OTAPI uint8_t OTCALL otGetRouterDowngradeThreshold(otInstance *aInstance);

/**
 * Set the ROUTER_DOWNGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aThreshold  The ROUTER_DOWNGRADE_THRESHOLD value.
 *
 * @sa otGetRouterDowngradeThreshold
 */
OTAPI void OTCALL otSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold);

/**
 * Get the ROUTER_SELECTION_JITTER parameter used in the REED/Router role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_SELECTION_JITTER value.
 *
 * @sa otSetRouterSelectionJitter
 */
OTAPI uint8_t OTCALL otGetRouterSelectionJitter(otInstance *aInstance);

/**
 * Set the ROUTER_SELECTION_JITTER parameter used in the REED/Router role.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aRouterJitter  The ROUTER_SELECTION_JITTER value.
 *
 * @sa otGetRouterSelectionJitter
 */
OTAPI void OTCALL otSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter);

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
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[in]   aChildId    The Child ID or RLOC16 for the attached child.
 * @param[out]  aChildInfo  A pointer to where the child information is placed.
 *
 */
OTAPI ThreadError OTCALL otGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo);

/**
 * The function retains diagnostic information for an attached Child by the internal table index.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aChildIndex  The table index.
 * @param[out]  aChildInfo   A pointer to where the child information is placed.
 *
 */
OTAPI ThreadError OTCALL otGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo);

/**
 * This function gets the next neighbor information. It is used to go through the entries of
 * the neighbor table.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the iterator context. To get the first neighbor entry
                             it should be set to OT_NEIGHBOR_INFO_ITERATOR_INIT.
 * @param[out]    aInfo      A pointer to where the neighbor information will be placed.
 *
 * @retval kThreadError_None         Successfully found the next neighbor entry in table.
 * @retval kThreadError_NotFound     No subsequent neighbor entry exists in the table.
 * @retval kThreadError_InvalidArgs  @p aIterator or @p aInfo was NULL.
 *
 */
ThreadError otGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo);

/**
 * Get the device role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval ::kDeviceRoleDisabled  The Thread stack is disabled.
 * @retval ::kDeviceRoleDetached  The device is not currently participating in a Thread network/partition.
 * @retval ::kDeviceRoleChild     The device is currently operating as a Thread Child.
 * @retval ::kDeviceRoleRouter    The device is currently operating as a Thread Router.
 * @retval ::kDeviceRoleLeader    The device is currently operating as a Thread Leader.
 */
OTAPI otDeviceRole OTCALL otGetDeviceRole(otInstance *aInstance);

/**
 * This function gets an EID cache entry.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[in]   aIndex    An index into the EID cache table.
 * @param[out]  aEntry    A pointer to where the EID information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the EID cache entry.
 * @retval kThreadError_InvalidArgs  @p aIndex was out of bounds or @p aEntry was NULL.
 *
 */
OTAPI ThreadError OTCALL otGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry);

/**
 * This function get the Thread Leader Data.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderData  A pointer to where the leader data is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the leader data.
 * @retval kThreadError_Detached     Not currently attached.
 * @retval kThreadError_InvalidArgs  @p aLeaderData is NULL.
 *
 */
OTAPI ThreadError OTCALL otGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData);

/**
 * Get the Leader's Router ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Router ID.
 */
OTAPI uint8_t OTCALL otGetLeaderRouterId(otInstance *aInstance);

/**
 * Get the Leader's Weight.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Weight.
 */
OTAPI uint8_t OTCALL otGetLeaderWeight(otInstance *aInstance);

/**
 * Get the Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Network Data Version.
 */
OTAPI uint8_t OTCALL otGetNetworkDataVersion(otInstance *aInstance);

/**
 * Get the Partition ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Partition ID.
 */
OTAPI uint32_t OTCALL otGetPartitionId(otInstance *aInstance);

/**
 * Get the RLOC16.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The RLOC16.
 */
OTAPI uint16_t OTCALL otGetRloc16(otInstance *aInstance);

/**
 * Get the current Router ID Sequence.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Router ID Sequence.
 */
OTAPI uint8_t OTCALL otGetRouterIdSequence(otInstance *aInstance);

/**
 * The function retains diagnostic information for a given Thread Router.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
 * @param[out]  aRouterInfo  A pointer to where the router information is placed.
 *
 */
OTAPI ThreadError OTCALL otGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo);

/**
 * The function retains diagnostic information for a Thread Router as parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentInfo  A pointer to where the parent router information is placed.
 *
 */
OTAPI ThreadError OTCALL otGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo);

/**
 * Get the Stable Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Stable Network Data Version.
 */
OTAPI uint8_t OTCALL otGetStableNetworkDataVersion(otInstance *aInstance);

/**
 * This function pointer is called when Network Diagnostic Get response is received.
 *
 * @param[in]  aMessage      A pointer to the message buffer containing the received Network Diagnostic
 *                           Get response payload.
 * @param[in]  aMessageInfo  A pointer to the message info for @p aMessage.
 * @param[in]  aContext      A pointer to application-specific context.
 *
 */
typedef void (*otReceiveDiagnosticGetCallback)(otMessage aMessage, const otMessageInfo *aMessageInfo,
                                               void *aContext);

/**
 * This function registers a callback to provide received raw Network Diagnostic Get response payload.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when Network Diagnostic Get response
 *                               is received or NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otSetReceiveDiagnosticGetCallback(otInstance *aInstance, otReceiveDiagnosticGetCallback aCallback,
                                       void *aCallbackContext);

/**
 * Send a Network Diagnostic Get request.
 *
 * @param[in]  aDestination   A pointer to destination address.
 * @param[in]  aTlvTypes      An array of Network Diagnostic TLV types.
 * @param[in]  aCount         Number of types in aTlvTypes
 */
OTAPI ThreadError OTCALL otSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination,
                                             const uint8_t aTlvTypes[], uint8_t aCount);

/**
 * Send a Network Diagnostic Reset request.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aDestination   A pointer to destination address.
 * @param[in]  aTlvTypes      An array of Network Diagnostic TLV types. Currently only Type 9 is allowed.
 * @param[in]  aCount         Number of types in aTlvTypes
 */
OTAPI ThreadError OTCALL otSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination,
                                               const uint8_t aTlvTypes[], uint8_t aCount);

/**
 * Get the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the MAC layer counters.
 */
OTAPI const otMacCounters *OTCALL otGetMacCounters(otInstance *aInstance);

/**
 * Get the Message Buffer information.
 *
 * @param[in]   aInstance    A pointer to the OpenThread instance.
 * @param[out]  aBufferInfo  A pointer where the message buffer information is written.
 *
 */
OTAPI void OTCALL otGetMessageBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo);

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
OTAPI bool OTCALL otIsIp6AddressEqual(const otIp6Address *a, const otIp6Address *b);

/**
 * Convert a human-readable IPv6 address string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aAddress  A pointer to an IPv6 address.
 *
 * @retval kThreadErrorNone        Successfully parsed the string.
 * @retval kThreadErrorInvalidArg  Failed to parse the string.
 */
OTAPI ThreadError OTCALL otIp6AddressFromString(const char *aString, otIp6Address *aAddress);

/**
 * This function returns the prefix match length (bits) for two IPv6 addresses.
 *
 * @param[in]  aFirst   A pointer to the first IPv6 address.
 * @param[in]  aSecond  A pointer to the second IPv6 address.
 *
 * @returns  The prefix match length in bits.
 *
 */
OTAPI uint8_t OTCALL otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond);

/**
 * This function converts a ThreadError enum into a string.
 *
 * @param[in]  aError     A ThreadError enum.
 *
 * @returns  A string representation of a ThreadError.
 *
 */
OTAPI const char *OTCALL otThreadErrorToString(ThreadError aError);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_H_
