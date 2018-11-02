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
 *  This file defines the OpenThread Instance API.
 */

#ifndef OPENTHREAD_INSTANCE_H_
#define OPENTHREAD_INSTANCE_H_

#include <stdlib.h>
#ifdef OTDLL
#include <guiddef.h>
#endif

#include <openthread/error.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-instance
 *
 * @brief
 *   This module includes functions that control the OpenThread Instance.
 *
 * @{
 *
 */

/**
 * This structure represents the OpenThread instance structure.
 */
typedef struct otInstance otInstance;

#ifdef OTDLL

/**
 * This structure represents the handle to the OpenThread API.
 *
 */
typedef struct otApiInstance otApiInstance;

/**
 * This structure represents a list of device GUIDs.
 *
 */
typedef struct otDeviceList
{
    uint16_t aDevicesLength;
    GUID     aDevices[1];
} otDeviceList;

/**
 * This function pointer is called to notify addition and removal of OpenThread devices.
 *
 * @param[in]  aAdded       A flag indicating if the device was added or removed.
 * @param[in]  aDeviceGuid  A GUID indicating which device state changed.
 * @param[in]  aContext     A pointer to application-specific context.
 *
 */
typedef void(OTCALL *otDeviceAvailabilityChangedCallback)(bool aAdded, const GUID *aDeviceGuid, void *aContext);

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
typedef void(OTCALL *otDeviceAvailabilityChangedCallback)(bool aAdded, const GUID *aDeviceGuid, void *aContext);

/**
 * This function registers a callback to indicate OpenThread devices come and go.
 *
 * @param[in]  aApiInstance     The OpenThread api instance.
 * @param[in]  aCallback        A pointer to a function that is called with the state changes.
 * @param[in]  aContextContext  A pointer to application-specific context.
 *
 */
OTAPI void OTCALL otSetDeviceAvailabilityChangedCallback(otApiInstance *                     aApiInstance,
                                                         otDeviceAvailabilityChangedCallback aCallback,
                                                         void *                              aCallbackContext);

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
 * @returns  The new OpenThread device instance structure for the device.
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
 * @returns  The device GUID.
 *
 */
OTAPI GUID OTCALL otGetDeviceGuid(otInstance *aInstance);

/**
 * This queries the Windows device/interface IfIndex for the otContext.
 *
 * @param[in] aContext  The OpenThread context structure.
 *
 * @returns The device IfIndex.
 *
 */
OTAPI uint32_t OTCALL otGetDeviceIfIndex(otInstance *aInstance);

/**
 * This queries the Windows Compartment ID for the otContext.
 *
 * @param[in] aContext  The OpenThread context structure.
 *
 * @returns  The compartment ID.
 *
 */
OTAPI uint32_t OTCALL otGetCompartmentId(otInstance *aInstance);

#else // OTDLL

/**
 * This function initializes the OpenThread library.
 *
 * This function initializes OpenThread and prepares it for subsequent OpenThread API calls. This function must be
 * called before any other calls to OpenThread.
 *
 * This function is available and can only be used when support for multiple OpenThread instances is enabled.
 *
 * @param[in]    aInstanceBuffer      The buffer for OpenThread to use for allocating the otInstance structure.
 * @param[inout] aInstanceBufferSize  On input, the size of aInstanceBuffer. On output, if not enough space for
 *                                    otInstance, the number of bytes required for otInstance.
 *
 * @returns  A pointer to the new OpenThread instance.
 *
 * @sa otInstanceFinalize
 *
 */
otInstance *otInstanceInit(void *aInstanceBuffer, size_t *aInstanceBufferSize);

/**
 * This function initializes the static single instance of the OpenThread library.
 *
 * This function initializes OpenThread and prepares it for subsequent OpenThread API calls. This function must be
 * called before any other calls to OpenThread.
 *
 * This function is available and can only be used when support for multiple OpenThread instances is disabled.
 *
 * @returns A pointer to the single OpenThread instance.
 *
 */
otInstance *otInstanceInitSingle(void);

/**
 * This function indicates whether or not the instance is valid/initialized.
 *
 * The instance is considered valid if it is acquired and initialized using either `otInstanceInitSingle()` (in single
 * instance case) or `otInstanceInit()` (in multi instance case). A subsequent call to `otInstanceFinalize()` causes
 * the instance to be considered as uninitialized.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if the given instance is valid/initialized, FALSE otherwise.
 *
 */
bool otInstanceIsInitialized(otInstance *aInstance);

/**
 * This function disables the OpenThread library.
 *
 * Call this function when OpenThread is no longer in use.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
void otInstanceFinalize(otInstance *aInstance);

#endif // OTDLL

/**
 * This enumeration defines flags that are passed as part of `otStateChangedCallback`.
 *
 */
enum
{
    OT_CHANGED_IP6_ADDRESS_ADDED           = 1 << 0,  ///< IPv6 address was added
    OT_CHANGED_IP6_ADDRESS_REMOVED         = 1 << 1,  ///< IPv6 address was removed
    OT_CHANGED_THREAD_ROLE                 = 1 << 2,  ///< Role (disabled, detached, child, router, leader) changed
    OT_CHANGED_THREAD_LL_ADDR              = 1 << 3,  ///< The link-local address changed
    OT_CHANGED_THREAD_ML_ADDR              = 1 << 4,  ///< The mesh-local address changed
    OT_CHANGED_THREAD_RLOC_ADDED           = 1 << 5,  ///< RLOC was added
    OT_CHANGED_THREAD_RLOC_REMOVED         = 1 << 6,  ///< RLOC was removed
    OT_CHANGED_THREAD_PARTITION_ID         = 1 << 7,  ///< Partition ID changed
    OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER = 1 << 8,  ///< Thread Key Sequence changed
    OT_CHANGED_THREAD_NETDATA              = 1 << 9,  ///< Thread Network Data changed
    OT_CHANGED_THREAD_CHILD_ADDED          = 1 << 10, ///< Child was added
    OT_CHANGED_THREAD_CHILD_REMOVED        = 1 << 11, ///< Child was removed
    OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED   = 1 << 12, ///< Subscribed to a IPv6 multicast address
    OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED = 1 << 13, ///< Unsubscribed from a IPv6 multicast address
    OT_CHANGED_COMMISSIONER_STATE          = 1 << 14, ///< Commissioner state changed
    OT_CHANGED_JOINER_STATE                = 1 << 15, ///< Joiner state changed
    OT_CHANGED_THREAD_CHANNEL              = 1 << 16, ///< Thread network channel changed
    OT_CHANGED_THREAD_PANID                = 1 << 17, ///< Thread network PAN Id changed
    OT_CHANGED_THREAD_NETWORK_NAME         = 1 << 18, ///< Thread network name changed
    OT_CHANGED_THREAD_EXT_PANID            = 1 << 19, ///< Thread network extended PAN ID changed
    OT_CHANGED_MASTER_KEY                  = 1 << 20, ///< Master key changed
    OT_CHANGED_PSKC                        = 1 << 21, ///< PSKc changed
    OT_CHANGED_SECURITY_POLICY             = 1 << 22, ///< Security Policy changed
    OT_CHANGED_CHANNEL_MANAGER_NEW_CHANNEL = 1 << 23, ///< Channel Manager new pending Thread channel changed
    OT_CHANGED_SUPPORTED_CHANNEL_MASK      = 1 << 24, ///< Supported channel mask changed
    OT_CHANGED_BORDER_AGENT_STATE          = 1 << 25, ///< Border agent state changed
    OT_CHANGED_THREAD_NETIF_STATE          = 1 << 26, ///< Thread network interface state changed
};

/**
 * This type represents a bit-field indicating specific state/configuration that has changed. See `OT_CHANGED_*`
 * definitions.
 *
 */
typedef uint32_t otChangedFlags;

/**
 * This function pointer is called to notify certain configuration or state changes within OpenThread.
 *
 * @param[in]  aFlags    A bit-field indicating specific state that has changed.  See `OT_CHANGED_*` definitions.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void(OTCALL *otStateChangedCallback)(otChangedFlags aFlags, void *aContext);

/**
 * This function registers a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE     Added the callback to the list of callbacks.
 * @retval OT_ERROR_ALREADY  The callback was already registered.
 * @retval OT_ERROR_NO_BUFS  Could not add the callback due to resource constraints.
 *
 */
OTAPI otError OTCALL otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aContext);

/**
 * This function removes a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aCallback   A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aContext    A pointer to application-specific context.
 *
 */
OTAPI void OTCALL otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aContext);

/**
 * This method triggers a platform reset.
 *
 * The reset process ensures that all the OpenThread state/info (stored in volatile memory) is erased. Note that the
 * `otPlatformReset` does not erase any persistent state/info saved in non-volatile memory.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
OTAPI void OTCALL otInstanceReset(otInstance *aInstance);

/**
 * This method deletes all the settings stored on non-volatile memory, and then triggers platform reset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
OTAPI void OTCALL otInstanceFactoryReset(otInstance *aInstance);

/**
 * This function erases all the OpenThread persistent info (network settings) stored on non-volatile memory.
 * Erase is successful only if the device is in `disabled` state/role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           All persistent info/state was erased successfully.
 * @retval OT_ERROR_INVALID_STATE  Device is not in `disabled` state/role.
 *
 */
otError otInstanceErasePersistentInfo(otInstance *aInstance);

/**
 * This function gets the OpenThread version string.
 *
 * @returns A pointer to the OpenThread version.
 *
 */
OTAPI const char *OTCALL otGetVersionString(void);

/**
 * This function gets the OpenThread radio version string.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the OpenThread radio version.
 *
 */
const char *otGetRadioVersionString(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_INSTANCE_H_
