/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes the platform abstraction for BLE HOST.
 */

#ifndef OPENTHREAD_PLATFORM_BLE_HOST_H_
#define OPENTHREAD_PLATFORM_BLE_HOST_H_

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-ble-host
 *
 * @brief
 *   This module includes the platform abstraction for BLE HOST communication.
 *
 * @{
 *
 */

/**
 * Enable the BLE HOST.
 *
 * @retval OT_ERROR_NONE    Successfully enabled the BLE HOST.
 * @retval OT_ERROR_FAILED  Failed to enabled the BLE HOST.
 *
 */
otError otBleHostInit(void);

/**
 * Disable the BLE HOST.
 *
 * @retval OT_ERROR_NONE    Successfully disabled the BLE HOST.
 * @retval OT_ERROR_FAILED  Failed to disable the BLE HOST.
 *
 */
otError otBleHostDeinit(void);

/**
 * Run all queued BLE stack tasklets at the time this is called.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
void otBleHostTaskletsProcess(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_BLE_HOST_H_
