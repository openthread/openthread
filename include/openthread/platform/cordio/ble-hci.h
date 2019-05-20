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
 *   This file includes the platform abstraction for Cordio BLE HCI communication.
 */

#ifndef OPENTHREAD_PLATFORM_CORDIO_BLE_HCI_H_
#define OPENTHREAD_PLATFORM_CORDIO_BLE_HCI_H_

#include <stdint.h>

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-cordio-hci
 *
 * @brief
 *   This module includes the platform abstraction for BLE HCI communication.
 *
 * @{
 *
 */

/**
 * Enable the BLE HCI.
 *
 * @retval OT_ERROR_NONE    Successfully enabled the BLE HCI.
 * @retval OT_ERROR_FAILED  Failed to enable the BLE HCI.
 *
 */
otError otCordioPlatHciEnable(void);

/**
 * Disable the BLE HCI.
 *
 * @retval OT_ERROR_NONE    Successfully disabled the BLE HCI.
 * @retval OT_ERROR_FAILED  Failed to disable the BLE HCI.
 *
 */
otError otCordioPlatHciDisable(void);

/**
 * Send bytes over the BLE HCI.
 *
 * @param[in] aBuf        A pointer to the data buffer.
 * @param[in] aBufLength  Number of bytes to transmit.
 *
 * @retval OT_ERROR_NONE    Successfully started transmission.
 * @retval OT_ERROR_FAILED  Failed to start the transmission.
 *
 */
otError otCordioPlatHciSend(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * The BLE HCI driver calls this method to notify OpenThread that the requested bytes have been sent.
 *
 */
extern void otCordioPlatHciSendDone(void);

/**
 * The BLE HCI driver calls this method to notify OpenThread that bytes have been received.
 *
 * @param[in]  aBuf        A pointer to the received bytes.
 * @param[in]  aBufLength  The number of bytes received.
 *
 */
extern void otCordioPlatHciReceived(uint8_t *aBuf, uint8_t aBufLength);

/*
 * Enable HCI driver interrupt.
 *
 */
void otCordioPlatHciEnableInterrupt(void);

/*
 * Disable HCI driver interrupt.
 *
 */
void otCordioPlatHciDisableInterrupt(void);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_CORDIO_BLE_HCI_H_
