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

#include <stddef.h>

#include <common/code_utils.hpp>
#include <openthread/types.h>
#include <openthread/platform/uart.h>

#include "bspconfig.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

enum
{
    kPlatformClock = 32000000,
    kBaudRate = 115200,
    kReceiveBufferSize = 128,
};

static void processReceive(void);
static void processTransmit(void);

static const uint8_t *sTransmitBuffer = NULL;
static uint16_t sTransmitLength = 0;

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

ThreadError otPlatUartEnable(void)
{
    USART_TypeDef           *usart = USART0;
    USART_InitAsync_TypeDef init   = USART_INITASYNC_DEFAULT;

    sReceive.mHead = 0;
    sReceive.mTail = 0;

    /* Enable peripheral clocks */
    CMU_ClockEnable(cmuClock_HFPER, true);
    /* Configure GPIO pins */
    CMU_ClockEnable(cmuClock_GPIO, true);

    /* Configure GPIO pin for UART TX */
    /* To avoid false start, configure output as high. */
    GPIO_PinModeSet(BSP_BCC_TXPORT, BSP_BCC_TXPIN, gpioModePushPull, 1u);
    /* Configure GPIO pin for UART RX */
    GPIO_PinModeSet(BSP_BCC_RXPORT, BSP_BCC_RXPIN, gpioModeInput, 1u);

    /* Enable the switch that enables UART communication. */
    GPIO_PinModeSet(BSP_BCC_ENABLE_PORT, BSP_BCC_ENABLE_PIN, gpioModePushPull, 1u);

    CMU_ClockEnable(cmuClock_USART0, true);

    /* Configure USART for basic async operation */
    init.enable = usartDisable;
    USART_InitAsync(usart, &init);

    /* Enable pins at correct UART/USART location. */
    usart->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
    usart->ROUTELOC0 = (usart->ROUTELOC0 & ~(_USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK))
                       | (_USART_ROUTELOC0_TXLOC_LOC0 << _USART_ROUTELOC0_TXLOC_SHIFT)
                       | (_USART_ROUTELOC0_RXLOC_LOC0 << _USART_ROUTELOC0_RXLOC_SHIFT);

    /* Clear previous RX interrupts */
    USART_IntClear(usart, USART_IF_RXDATAV);
    NVIC_ClearPendingIRQ(USART0_RX_IRQn);

    /* Enable RX interrupts */
    USART_IntEnable(usart, USART_IF_RXDATAV);
    NVIC_EnableIRQ(USART0_RX_IRQn);

    /* Finally enable it */
    USART_Enable(usart, usartEnable);

    return kThreadError_None;
}

void USART0_RX_IRQHandler(void)
{
    if (USART0->STATUS & USART_STATUS_RXDATAV)
    {
        uint8_t byte = USART_Rx(USART0);

        // We can only write if incrementing mTail doesn't equal mHead
        if (sReceive.mHead != (sReceive.mTail + 1) % kReceiveBufferSize)
        {
            sReceive.mBuffer[sReceive.mTail] = byte;
            sReceive.mTail = (sReceive.mTail + 1) % kReceiveBufferSize;
        }
    }
}

ThreadError otPlatUartDisable(void)
{
    return kThreadError_NotImplemented;
}

ThreadError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sTransmitBuffer == NULL, error = kThreadError_Busy);

    sTransmitBuffer = aBuf;
    sTransmitLength = aBufLength;

exit:
    return error;
}

void processReceive(void)
{
    // Copy tail to prevent multiple reads
    uint16_t tail = sReceive.mTail;

    // If the data wraps around, process the first part
    if (sReceive.mHead > tail)
    {
        otPlatUartReceived(sReceive.mBuffer + sReceive.mHead, kReceiveBufferSize - sReceive.mHead);

        // Reset the buffer mHead back to zero.
        sReceive.mHead = 0;
    }

    // For any data remaining, process it
    if (sReceive.mHead != tail)
    {
        otPlatUartReceived(sReceive.mBuffer + sReceive.mHead, tail - sReceive.mHead);

        // Set mHead to the local tail we have cached
        sReceive.mHead = tail;
    }
}

void processTransmit(void)
{
    VerifyOrExit(sTransmitBuffer != NULL, ;);

    for (; sTransmitLength > 0; sTransmitLength--)
    {
        USART_Tx(USART0, *sTransmitBuffer++);
    }

    sTransmitBuffer = NULL;
    otPlatUartSendDone();

exit:
    return;
}

void efr32UartProcess(void)
{
    processReceive();
    processTransmit();
}
