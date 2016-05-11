/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the platform abstraction for serial communication.
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup serial Serial
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction for serial communication.
 *
 * @{
 *
 */

/**
 * Enable the serial.
 *
 * @retval ::kThreadError_None  Successfully enabled the serial.
 * @retval ::kThreadError_Fail  Failed to enabled the serial.
 */
ThreadError otPlatSerialEnable(void);

/**
 * Disable the serial.
 *
 * @retval ::kThreadError_None  Successfully disabled the serial.
 * @retval ::kThreadError_Fail  Failed to disable the serial.
 */
ThreadError otPlatSerialDisable(void);

/**
 * Send bytes over the serial.
 *
 * @param[in] aBuf        A pointer to the data buffer.
 * @param[in] aBufLength  Number of bytes to transmit.
 *
 * @retval ::kThreadError_None  Successfully started transmission.
 * @retval ::kThreadError_Fail  Failed to start the transmission.
 */
ThreadError otPlatSerialSend(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Signal that the bytes send operation has completed.
 *
 * This may be called from interrupt context.  This will schedule calls to otPlatSerialHandleSendDone().
 */
extern void otPlatSerialSignalSendDone(void);

/**
 * Complete the send sequence.
 */
void otPlatSerialHandleSendDone(void);

/**
 * Signal that bytes have been received.
 *
 * This may be called from interrupt context.  This will schedule calls to otPlatSerialGetReceivedBytes() and
 * otPlatSerialHandleReceiveDone().
 */
extern void otPlatSerialSignalReceive(void);

/**
 * Get a pointer to the received bytes.
 *
 * @param[out]  aBufLength  A pointer to a variable that this function will put the number of bytes received.
 *
 * @returns A pointer to the received bytes.  NULL, if there are no received bytes to process.
 */
const uint8_t *otPlatSerialGetReceivedBytes(uint16_t *aBufLength);

/**
 * Release received bytes.
 */
void otPlatSerialHandleReceiveDone();

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SERIAL_H_
