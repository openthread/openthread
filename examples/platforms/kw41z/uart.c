/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for UART communication.
 *
 */

#include "fsl_device_registers.h"
#include <stddef.h>
#include <stdint.h>

#include <utils/code_utils.h>
#include "openthread/platform/uart.h"

#include "fsl_clock.h"
#include "fsl_lpuart.h"
#include "fsl_port.h"

enum
{
    kPlatformClock     = 32000000,
    kBaudRate          = 115200,
    kReceiveBufferSize = 256,
};

static void processReceive();
static void processTransmit();

static const uint8_t *sTransmitBuffer = NULL;
static uint16_t       sTransmitLength = 0;
static bool           sTransmitDone   = false;

typedef struct RecvBuffer
{
    // The data buffer
    uint8_t mBuffer[kReceiveBufferSize];
    // The offset of the first item written to the list.
    uint16_t mHead;
    // The offset of the next item to be written to the list.
    uint16_t mTail;
} RecvBuffer;

static RecvBuffer sReceive;

otError otPlatUartEnable(void)
{
    lpuart_config_t config;

    sReceive.mHead = 0;
    sReceive.mTail = 0;

    /* Pin MUX */
    CLOCK_EnableClock(kCLOCK_PortC);
    PORT_SetPinMux(PORTC, 6, kPORT_MuxAlt4);
    PORT_SetPinMux(PORTC, 7, kPORT_MuxAlt4);

    /* Set OSCERCLK as LPUART Rx/Tx clock */
    CLOCK_SetLpuartClock(2);

    LPUART_GetDefaultConfig(&config);
    config.enableRx     = 1;
    config.enableTx     = 1;
    config.baudRate_Bps = kBaudRate;
    LPUART_Init(LPUART0, &config, kPlatformClock);
    LPUART_EnableInterrupts(LPUART0, kLPUART_RxDataRegFullInterruptEnable);

    NVIC_ClearPendingIRQ(LPUART0_IRQn);
    NVIC_EnableIRQ(LPUART0_IRQn);

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    NVIC_DisableIRQ(LPUART0_IRQn);
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sTransmitBuffer == NULL, error = OT_ERROR_BUSY);

    sTransmitBuffer = aBuf + 1;
    sTransmitLength = aBufLength - 1;
    sTransmitDone   = false;
    LPUART_WriteByte(LPUART0, *aBuf);
    LPUART_ClearStatusFlags(LPUART0, kLPUART_TxDataRegEmptyFlag);
    LPUART_EnableInterrupts(LPUART0, kLPUART_TxDataRegEmptyInterruptEnable);

exit:
    return error;
}

static void processTransmit(void)
{
    if (sTransmitBuffer && sTransmitDone)
    {
        sTransmitDone   = false;
        sTransmitBuffer = NULL;
        otPlatUartSendDone();
    }

    return;
}

void kw41zUartProcess(void)
{
    processReceive();
    processTransmit();
}

static void processReceive(void)
{
    // Copy tail to prevent multiple reads
    uint16_t tail = sReceive.mTail;

    if (sReceive.mHead > tail)
    {
        otPlatUartReceived(sReceive.mBuffer + sReceive.mHead, kReceiveBufferSize - sReceive.mHead);

        // Reset the buffer mHead back to zero
        sReceive.mHead = 0;
    }

    if (sReceive.mHead != tail)
    {
        otPlatUartReceived(sReceive.mBuffer + sReceive.mHead, tail - sReceive.mHead);

        // Set mHead to the local tail we have cached
        sReceive.mHead = tail;
    }
}

void LPUART0_IRQHandler(void)
{
    uint32_t interrupts = LPUART_GetEnabledInterrupts(LPUART0);
    uint8_t  rx_data;

    /* Check if data was received */
    while (LPUART_GetStatusFlags(LPUART0) & (kLPUART_RxDataRegFullFlag))
    {
        rx_data = LPUART_ReadByte(LPUART0);
        LPUART_ClearStatusFlags(LPUART0, kLPUART_RxDataRegFullFlag);

        if (sReceive.mHead != (sReceive.mTail + 1) % kReceiveBufferSize)
        {
            sReceive.mBuffer[sReceive.mTail] = rx_data;
            sReceive.mTail                   = (sReceive.mTail + 1) % kReceiveBufferSize;
        }
    }

    /* Check if data Tx has end */
    if ((LPUART_GetStatusFlags(LPUART0) & kLPUART_TxDataRegEmptyFlag) &&
        (interrupts & kLPUART_TxDataRegEmptyInterruptEnable))
    {
        if (sTransmitLength)
        {
            sTransmitLength--;
            LPUART_WriteByte(LPUART0, *sTransmitBuffer++);
        }
        else if (!sTransmitDone)
        {
            sTransmitDone = true;
            LPUART_DisableInterrupts(LPUART0, kLPUART_TxDataRegEmptyInterruptEnable);
        }
    }

    if (LPUART_GetStatusFlags(LPUART0) & kLPUART_RxOverrunFlag)
    {
        LPUART_ClearStatusFlags(LPUART0, kLPUART_RxOverrunFlag);
    }
}
