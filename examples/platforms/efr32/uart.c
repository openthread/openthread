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

#include <openthread/platform/uart.h>

#include "utils/code_utils.h"

#include "em_core.h"
#include "uartdrv.h"

#include "hal-config.h"

enum
{
    kReceiveFifoSize = 128,
};

#define USART_INIT                                                                               \
    {                                                                                            \
        USART0,                                               /* USART port */                   \
            115200,                                           /* Baud rate */                    \
            BSP_SERIAL_APP_TX_LOC,                            /* USART Tx pin location number */ \
            BSP_SERIAL_APP_RX_LOC,                            /* USART Rx pin location number */ \
            (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */                    \
            (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */                       \
            (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/             \
            false,                                            /* Majority vote disable */        \
            uartdrvFlowControlHwUart,                         /* Flow control */                 \
            BSP_SERIAL_APP_CTS_PORT,                          /* CTS port number */              \
            BSP_SERIAL_APP_CTS_PIN,                           /* CTS pin number */               \
            BSP_SERIAL_APP_RTS_PORT,                          /* RTS port number */              \
            BSP_SERIAL_APP_RTS_PIN,                           /* RTS pin number */               \
            (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue,      /* RX operation queue */           \
            (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue,      /* TX operation queue */           \
            BSP_SERIAL_APP_CTS_LOC,                           /* CTS location */                 \
            BSP_SERIAL_APP_RTS_LOC                            /* RTS location */                 \
    }

DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, sUartRxQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, sUartTxQueue);

static UARTDRV_HandleData_t sUartHandleData;
static UARTDRV_Handle_t     sUartHandle = &sUartHandleData;
static uint8_t              sReceiveBuffer[2];
static const uint8_t *      sTransmitBuffer = NULL;
static volatile uint16_t    sTransmitLength = 0;

typedef struct ReceiveFifo_t
{
    // The data buffer
    uint8_t mBuffer[kReceiveFifoSize];
    // The offset of the first item written to the list.
    volatile uint16_t mHead;
    // The offset of the next item to be written to the list.
    volatile uint16_t mTail;
} ReceiveFifo_t;

static ReceiveFifo_t sReceiveFifo;

static void processReceive(void);

static void receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    // We can only write if incrementing mTail doesn't equal mHead
    if (sReceiveFifo.mHead != (sReceiveFifo.mTail + 1) % kReceiveFifoSize)
    {
        sReceiveFifo.mBuffer[sReceiveFifo.mTail] = aData[0];
        sReceiveFifo.mTail                       = (sReceiveFifo.mTail + 1) % kReceiveFifoSize;
    }

    UARTDRV_Receive(aHandle, aData, 1, receiveDone);
}

static void transmitDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    sTransmitLength = 0;
}

static void processReceive(void)
{
    // Copy tail to prevent multiple reads
    uint16_t tail = sReceiveFifo.mTail;

    // If the data wraps around, process the first part
    if (sReceiveFifo.mHead > tail)
    {
        otPlatUartReceived(sReceiveFifo.mBuffer + sReceiveFifo.mHead, kReceiveFifoSize - sReceiveFifo.mHead);

        // Reset the buffer mHead back to zero.
        sReceiveFifo.mHead = 0;
    }

    // For any data remaining, process it
    if (sReceiveFifo.mHead != tail)
    {
        otPlatUartReceived(sReceiveFifo.mBuffer + sReceiveFifo.mHead, tail - sReceiveFifo.mHead);

        // Set mHead to the local tail we have cached
        sReceiveFifo.mHead = tail;
    }
}

static void processTransmit(void)
{
    if (sTransmitBuffer != NULL && sTransmitLength == 0)
    {
        sTransmitBuffer = NULL;
        otPlatUartSendDone();
    }
}

otError otPlatUartEnable(void)
{
    UARTDRV_Init_t uartInit = USART_INIT;

    sReceiveFifo.mHead = 0;
    sReceiveFifo.mTail = 0;

    UARTDRV_Init(sUartHandle, &uartInit);

    for (uint8_t i = 0; i < sizeof(sReceiveBuffer); i++)
    {
        UARTDRV_Receive(sUartHandle, &sReceiveBuffer[i], sizeof(sReceiveBuffer[i]), receiveDone);
    }

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sTransmitBuffer == NULL, error = OT_ERROR_BUSY);

    sTransmitBuffer = aBuf;
    sTransmitLength = aBufLength;

    UARTDRV_Transmit(sUartHandle, (uint8_t *)sTransmitBuffer, sTransmitLength, transmitDone);

exit:
    return error;
}

void efr32UartProcess(void)
{
    processReceive();
    processTransmit();
}
