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
 *   This file implements the RTT implementation of the uart API.
 */

#include <stdint.h>

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <utils/code_utils.h>
#include <openthread/error.h>

#if OPENTHREAD_UART_RTT_ENABLE

#include "SEGGER_RTT.h"
#include "uart.h"
#include "uart_rtt.h"

static bool sUartInitialized = false;
static bool sUartPendingUp   = false;

#if UART_RTT_BUFFER_INDEX != 0
static uint8_t sUartUpBuffer[UART_RTT_UP_BUFFER_SIZE];
static uint8_t sUartDownBuffer[UART_RTT_DOWN_BUFFER_SIZE];
#endif

otError otPlatUartEnable(void)
{
    otError error = OT_ERROR_FAILED;

#if UART_RTT_BUFFER_INDEX != 0
    int resUp   = SEGGER_RTT_ConfigUpBuffer(UART_RTT_BUFFER_INDEX, UART_RTT_BUFFER_NAME, sUartUpBuffer,
                                            UART_RTT_UP_BUFFER_SIZE, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
    int resDown = SEGGER_RTT_ConfigDownBuffer(UART_RTT_BUFFER_INDEX, UART_RTT_BUFFER_NAME, sUartDownBuffer,
                                              UART_RTT_DOWN_BUFFER_SIZE, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
#else
    int resUp   = SEGGER_RTT_SetFlagsUpBuffer(UART_RTT_BUFFER_INDEX, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
    int resDown = SEGGER_RTT_SetFlagsDownBuffer(UART_RTT_BUFFER_INDEX, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
#endif

    otEXPECT(resUp >= 0 && resDown >= 0);

    sUartInitialized = true;
    sUartPendingUp   = false;
    error            = OT_ERROR_NONE;

exit:
    return error;
}

otError otPlatUartDisable(void)
{
    sUartInitialized = false;

    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(SEGGER_RTT_Write(UART_RTT_BUFFER_INDEX, aBuf, aBufLength) != 0, error = OT_ERROR_FAILED);
    sUartPendingUp = true;

exit:
    return error;
}

otError otPlatUartFlush(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sUartPendingUp, error = OT_ERROR_INVALID_STATE);

    while (SEGGER_RTT_HasDataUp(UART_RTT_BUFFER_INDEX) != 0)
    {
    }

exit:
    return error;
}

void utilsUartRttProcess(void)
{
    uint8_t  buf[UART_RTT_READ_BUFFER_SIZE];
    unsigned count;

    otEXPECT(sUartInitialized);

    if (sUartPendingUp && SEGGER_RTT_HasDataUp(UART_RTT_BUFFER_INDEX) == 0)
    {
        sUartPendingUp = false;
        otPlatUartSendDone();
    }

    count = SEGGER_RTT_Read(UART_RTT_BUFFER_INDEX, &buf, sizeof(buf));
    if (count > 0)
    {
        otPlatUartReceived((const uint8_t *)&buf, count);
    }

exit:
    return;
}

#endif // OPENTHREAD_UART_RTT_ENABLE
