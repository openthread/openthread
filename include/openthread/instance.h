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

#include <openthread/types.h>
#include <openthread/platform/logging.h>

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

#else // OTDLL

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
#else // OPENTHREAD_MULTIPLE_INSTANCE
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
#endif // OPENTHREAD_MULTIPLE_INSTANCE

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
 * @retval OT_ERROR_NONE     Added the callback to the list of callbacks.
 * @retval OT_ERROR_NO_BUFS  Could not add the callback due to resource constraints.
 *
 */
OTAPI otError OTCALL otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback,
                                               void *aContext);

/**
 * This function removes a callback to indicate when certain configuration or state changes within OpenThread.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called with certain configuration or state changes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
OTAPI void OTCALL otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback,
                                              void *aCallbackContext);

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
 * This function returns the current dynamic log level.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns the currently set dynamic log level.
 *
 */
otLogLevel otGetDynamicLogLevel(otInstance *aInstance);

/**
 * This function sets the dynamic log level.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aLogLevel The dynamic log level.
 *
 * @retval OT_ERROR_NONE          The log level was changed successfully.
 * @retval OT_ERROR_NOT_CAPABLE   The dynamic log level is not supported.
 *
 */
otError otSetDynamicLogLevel(otInstance *aInstance, otLogLevel aLogLevel);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_INSTANCE_H_
