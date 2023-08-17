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
 *   This file defines the RTT implementation of the uart API and default constants used by uart_rtt.c.
 *
 */

#ifndef UTILS_UART_RTT_H
#define UTILS_UART_RTT_H

#include "openthread-core-config.h"
#include <openthread/config.h>

#include "logging_rtt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def UART_RTT_BUFFER_INDEX
 *
 * RTT buffer index used for the uart.
 *
 */
#ifndef UART_RTT_BUFFER_INDEX
#define UART_RTT_BUFFER_INDEX 1
#endif

#if OPENTHREAD_UART_RTT_ENABLE && (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) && \
    (LOG_RTT_BUFFER_INDEX == UART_RTT_BUFFER_INDEX)
#error "Log buffer index matches uart buffer index"
#endif

/**
 * @def UART_RTT_BUFFER_NAME
 *
 * RTT name used for the uart. Only used if UART_RTT_BUFFER_INDEX is not 0.
 * Otherwise, the buffer name is fixed to "Terminal".
 *
 */
#ifndef UART_RTT_BUFFER_NAME
#define UART_RTT_BUFFER_NAME "Terminal"
#endif

/**
 * @def UART_RTT_UP_BUFFER_SIZE
 *
 * RTT up buffer size used for the uart. Only used if UART_RTT_BUFFER_INDEX
 * is not 0. To configure buffer #0 size, check the BUFFER_SIZE_UP definition
 * in SEGGER_RTT_Conf.h
 *
 */
#ifndef UART_RTT_UP_BUFFER_SIZE
#define UART_RTT_UP_BUFFER_SIZE 256
#endif

/**
 * @def UART_RTT_DOWN_BUFFER_SIZE
 *
 * RTT down buffer size used for the uart. Only used if UART_RTT_BUFFER_INDEX
 * is not 0. To configure buffer #0 size, check the BUFFER_SIZE_DOWN definition
 * in SEGGER_RTT_Conf.h
 *
 */
#ifndef UART_RTT_DOWN_BUFFER_SIZE
#define UART_RTT_DOWN_BUFFER_SIZE 16
#endif

/**
 * @def UART_RTT_READ_BUFFER_SIZE
 *
 * Size of the temporary buffer used when reading from the RTT channel. It will be
 * locally allocated on the stack.
 *
 */
#ifndef UART_RTT_READ_BUFFER_SIZE
#define UART_RTT_READ_BUFFER_SIZE 16
#endif

/**
 * Updates the rtt uart. Must be called frequently to process receive and send done.
 *
 */
void utilsUartRttProcess(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_UART_RTT_H
