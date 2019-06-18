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
 *   This file includes the declarations of the uart functions from the Qorvo library.
 *
 */

#ifndef _UART_QORVO_H_
#define _UART_QORVO_H_

#include <stdbool.h>
#include <stdint.h>
#include <openthread/platform/logging.h>

/**
 * This function performs UART driver processing.
 *
 */
void qorvoUartProcess(void);

/**
 * This function enables the UART driver.
 *
 */
void qorvoUartInit(void);

/**
 * This function disables the UART driver.
 *
 */
void qorvoUartDeInit(void);

/**
 * Callback function which will be called when uart transmission is done.
 *
 */
void cbQorvoUartTxDone(void);

/**
 * Callback function which will be called when uart data is received.
 *
 * @param[in]  aBuf         A pointer to an array of received bytes.
 * @param[in]  aBufLength   The number of bytes received from the uart.
 *
 */
void qorvoUartSendInput(uint8_t *aBuf, uint16_t aBufLength);

/**
 * Function which transmits data via the uart.
 *
 * @param[out]  aBuf        A pointer to an array of bytes which need to be transmitted.
 * @param[in]   aBufLength  The number of bytes to be transmitted via the uart.
 *
 */
void qorvoUartSendOutput(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Function which transmits logging via the qorvo platform.
 *
 * @param[in]   aLogLevel   The severity level of logging.
 * @param[in]   aLogRegion  The code region where the logging originated from.
 * @param[in]   aFormat     The format strings.
 * @param[in]   varargs     The arguments for the format string.
 *
 */
void qorvoUartLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...);

#endif // _UART_QORVO_H_
