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
 *  This file defines the top-level functions for the OpenThread NCP module.
 */

#ifndef OPENTHREAD_NCP_H_
#define OPENTHREAD_NCP_H_

#include <stdarg.h>

#include <openthread/platform/logging.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-ncp
 *
 * @brief
 *   This module includes functions that control the Thread stack's execution.
 *
 * @{
 */

/**
 * Pointer is called to send HDLC encoded NCP data.
 *
 * @param[in]  aBuf        A pointer to a buffer with an output.
 * @param[in]  aBufLength  A length of the output data stored in the buffer.
 *
 * @returns                Number of bytes processed by the callback.
 */
typedef int (*otNcpHdlcSendCallback)(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Is called after NCP send finished.
 */
void otNcpHdlcSendDone(void);

/**
 * Is called after HDLC encoded NCP data received.
 *
 * @param[in]  aBuf        A pointer to a buffer.
 * @param[in]  aBufLength  The length of the data stored in the buffer.
 */
void otNcpHdlcReceive(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Initialize the NCP based on HDLC framing.
 *
 * @param[in]  aInstance        The OpenThread instance structure.
 * @param[in]  aSendCallback    The function pointer used to send NCP data.
 */
void otNcpHdlcInit(otInstance *aInstance, otNcpHdlcSendCallback aSendCallback);

/**
 * Initialize the NCP based on HDLC framing.
 *
 * @param[in]  aInstances       The OpenThread instance pointers array.
 * @param[in]  aCount           Number of elements in the array.
 * @param[in]  aSendCallback    The function pointer used to send NCP data.
 */
void otNcpHdlcInitMulti(otInstance **aInstance, uint8_t aCount, otNcpHdlcSendCallback aSendCallback);

/**
 * Initialize the NCP based on SPI framing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
void otNcpSpiInit(otInstance *aInstance);

/**
 * @brief Send data to the host via a specific stream.
 *
 * Attempts to send the given data to the host
 * using the given aStreamId. This is useful for reporting
 * error messages, implementing debug/diagnostic consoles,
 * and potentially other types of datastreams.
 *
 * The write either is accepted in its entirety or rejected.
 * Partial writes are not attempted.
 *
 * @param[in]  aStreamId  A numeric identifier for the stream to write to.
 *                        If set to '0', will default to the debug stream.
 * @param[in]  aDataPtr   A pointer to the data to send on the stream.
 *                        If aDataLen is non-zero, this param MUST NOT be NULL.
 * @param[in]  aDataLen   The number of bytes of data from aDataPtr to send.
 *
 * @retval OT_ERROR_NONE         The data was queued for delivery to the host.
 * @retval OT_ERROR_BUSY         There are not enough resources to complete this
 *                               request. This is usually a temporary condition.
 * @retval OT_ERROR_INVALID_ARGS The given aStreamId was invalid.
 */
otError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen);

/**
 * Writes OpenThread Log using `otNcpStreamWrite`.
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aFormat     A pointer to the format string.
 * @param[in]  aArgs       va_list matching aFormat.
 */
void otNcpPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs);

//-----------------------------------------------------------------------------------------
// Peek/Poke memory access control delegates

/**
 * Defines delegate (function pointer) type to control behavior of peek/poke operation.
 *
 * This delegate function is called to decide whether to allow peek or poke of a specific memory region. It is used
 * if NCP support for peek/poke commands is enabled.
 *
 * @param[in] aAddress    Start address of memory region.
 * @param[in] aCount      Number of bytes to peek or poke.
 *
 * @returns  TRUE to allow peek/poke of the given memory region, FALSE otherwise.
 */
typedef bool (*otNcpDelegateAllowPeekPoke)(uint32_t aAddress, uint16_t aCount);

/**
 * Registers peek/poke delegate functions with NCP module.
 *
 * The delegate functions are called by NCP module to decide whether to allow peek or poke of a specific memory region.
 * If the delegate pointer is set to NULL, it allows peek/poke operation for any address.
 *
 * @param[in] aAllowPeekDelegate      Delegate function pointer for peek operation.
 * @param[in] aAllowPokeDelegate      Delegate function pointer for poke operation.
 */
void otNcpRegisterPeekPokeDelegates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                    otNcpDelegateAllowPeekPoke aAllowPokeDelegate);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_NCP_H_
