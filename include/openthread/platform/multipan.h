/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file defines the multipan interface for OpenThread.
 *
 *   Multipan RCP is a feature that allows single RCP operate in multiple networks.
 *
 *   Currently we support two types of multipan RCP:
 *   - Full multipan: RCP operates in parallel on both networks (for example using more than one transceiver)
 *   - Switching RCP: RCP can communicate only with one network at a time and requires network switching mechanism.
 *                    Switching can be automatic (for example time based, radio sleep based) or manually controlled by
 *                    the host.
 *
 *   Full multipan RCP and Automatic Switching RCP do not require any special care from the host side.
 *   Manual Switching RCP requires host to switch currently active network.
 */

#ifndef OPENTHREAD_PLATFORM_MULTIPAN_H_
#define OPENTHREAD_PLATFORM_MULTIPAN_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-multipan
 *
 * @brief
 *   This module includes the platform abstraction for multipan support.
 *
 * @{
 */

/**
 * Get instance currently in control of the radio.
 *
 * If radio does not operate in parallel on all interfaces, this function returns an instance object with granted
 * radio access.
 *
 * @param[out]  aInstance        Pointer to the variable for storing the active instance pointer.
 *
 * @retval  OT_ERROR_NONE               Successfully retrieved the property.
 * @retval  OT_ERROR_NOT_IMPLEMENTED    Failed due to lack of the support in radio.
 * @retval  OT_ERROR_INVALID_COMMAND    Platform supports all interfaces simultaneously.
 */
otError otPlatMultipanGetActiveInstance(otInstance **aInstance);

/**
 * Set `aInstance` as the current active instance controlling radio.
 *
 * This function allows selecting the currently active instance on platforms that do not support parallel
 * communication on multiple interfaces. In other words, if more than one instance is in a receive state, calling
 * #otPlatMultipanSetActiveInstance guarantees that specified instance will be the one receiving. This function returns
 * if the request was received properly. After interface switching is complete, the platform should call
 * #otPlatMultipanSwitchoverDone. Switching interfaces may take longer if `aCompletePending` is set true.
 *
 * @param[in] aInstance         The OpenThread instance structure.
 * @param[in] aCompletePending  True if ongoing radio operation should complete before interface switch (Soft switch),
 *                              false for force switch.
 *
 * @retval  OT_ERROR_NONE               Successfully set the property.
 * @retval  OT_ERROR_BUSY               Failed due to another operation ongoing.
 * @retval  OT_ERROR_NOT_IMPLEMENTED    Failed due to unknown instance or more instances than interfaces available.
 * @retval  OT_ERROR_INVALID_COMMAND    Platform supports all interfaces simultaneously.
 * @retval  OT_ERROR_ALREADY            Given interface is already active.
 */
otError otPlatMultipanSetActiveInstance(otInstance *aInstance, bool aCompletePending);

/**
 * The platform completed the interface switching procedure.
 *
 * Should be invoked immediately after processing #otPlatMultipanSetActiveInstance if no delay is needed, or if
 * some longer radio operations need to complete first, after the switch in interfaces is fully complete.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aSuccess  True if successfully switched the interfaces, false if switching failed.
 */
extern void otPlatMultipanSwitchoverDone(otInstance *aInstance, bool aSuccess);

/**
 * Get the instance pointer corresponding to the given IID.
 *
 * @param[in] aIid  The IID of the interface.
 *
 * @retval  Instance pointer if aIid is has an instance assigned, NULL otherwise.
 */
otInstance *otPlatMultipanIidToInstance(uint8_t aIid);

/**
 * Get the IID corresponding to the given OpenThread instance pointer.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 *
 * @retval  IID of the given instance, broadcast IID otherwise.
 */
uint8_t otPlatMultipanInstanceToIid(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_MULTIPAN_H_
