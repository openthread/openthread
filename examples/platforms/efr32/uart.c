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
#include <assert.h>
#include <string.h>

#include "hal-config.h"

enum
{
    kReceiveFifoSize = 128,
    kDmaBlockSize    = 32,
};

#define USART_PORT USART0
#define USART_PORT_RX_IRQn USART0_RX_IRQn

#define USART_INIT                                                                               \
    {                                                                                            \
        USART_PORT,                                           /* USART port */                   \
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

static CORE_DECLARE_NVIC_MASK(sRxNvicMask);
static UARTDRV_HandleData_t sUartHandleData;
static UARTDRV_Handle_t     sUartHandle     = &sUartHandleData;
static const uint8_t *      sTransmitBuffer = NULL;
static volatile uint16_t    sTransmitLength = 0;

typedef struct ReceiveFifo_t
{
    // The data buffer
    uint8_t mBuffer[kReceiveFifoSize];
    // The offset of the first item to be read from the list
    uint16_t mReadStart;
    // The offset of the last item to be read plus one
    volatile uint16_t mReadEnd;
    // The offset of first unused item
    volatile uint16_t mWrite;

} ReceiveFifo_t;

static ReceiveFifo_t sReceiveFifo;

static void queueNextReceive(void);

static void updateReceiveProgress(uint8_t *aData, UARTDRV_Count_t aCount)
{
    assert(aData != NULL);

    const uint16_t buffPos = aData - sReceiveFifo.mBuffer;

    if (buffPos + kDmaBlockSize == kReceiveFifoSize)
    {
        assert(sReceiveFifo.mReadEnd >= buffPos || sReceiveFifo.mReadEnd == 0);
        assert(sReceiveFifo.mReadEnd <= buffPos + aCount);
    }
    else
    {
        assert(sReceiveFifo.mReadEnd >= buffPos);
        assert(sReceiveFifo.mReadEnd <= buffPos + aCount);
    }

    sReceiveFifo.mReadEnd = (buffPos + aCount) % kReceiveFifoSize;
}

static void receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    updateReceiveProgress(aData, aCount);
    queueNextReceive();
}

static void queueNextReceive(void)
{
    if (sReceiveFifo.mWrite > sReceiveFifo.mReadStart)
    {
        assert(kReceiveFifoSize - sReceiveFifo.mWrite >= kDmaBlockSize);
    }
    else if (sReceiveFifo.mWrite < sReceiveFifo.mReadStart)
    {
        assert(sReceiveFifo.mReadStart - sReceiveFifo.mWrite >= kDmaBlockSize);
    }
    else
    {
        assert(sReceiveFifo.mReadStart == sReceiveFifo.mReadEnd);
        assert(kReceiveFifoSize - sReceiveFifo.mWrite >= kDmaBlockSize);
    }

    UARTDRV_Receive(sUartHandle, sReceiveFifo.mBuffer + sReceiveFifo.mWrite, kDmaBlockSize, receiveDone);
    sReceiveFifo.mWrite = (sReceiveFifo.mWrite + kDmaBlockSize) % kReceiveFifoSize;
}

static void transmitDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    sTransmitLength = 0;
}

static void processReceive(void)
{
    uint16_t        readEnd;
    uint8_t *       buffer;
    UARTDRV_Count_t itemsReceived;
    UARTDRV_Count_t itemsRemaining;

    CORE_DECLARE_NVIC_STATE;

    CORE_ENTER_NVIC(&sRxNvicMask);

    UARTDRV_GetReceiveStatus(sUartHandle, &buffer, &itemsReceived, &itemsRemaining);
    updateReceiveProgress(buffer, itemsReceived);

    readEnd = sReceiveFifo.mReadEnd;

    CORE_EXIT_NVIC();

    if (sReceiveFifo.mReadStart > readEnd)
    {
        otPlatUartReceived(sReceiveFifo.mBuffer + sReceiveFifo.mReadStart, kReceiveFifoSize - sReceiveFifo.mReadStart);
        sReceiveFifo.mReadStart = 0;
    }

    if (sReceiveFifo.mReadStart != readEnd)
    {
        otPlatUartReceived(sReceiveFifo.mBuffer + sReceiveFifo.mReadStart, readEnd - sReceiveFifo.mReadStart);
        sReceiveFifo.mReadStart = readEnd;
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

    memset(&sRxNvicMask, 0, sizeof(sRxNvicMask));
    CORE_NvicMaskSetIRQ(LDMA_IRQn, &sRxNvicMask);
    CORE_NvicMaskSetIRQ(USART_PORT_RX_IRQn, &sRxNvicMask);

    sReceiveFifo.mReadStart = 0;
    sReceiveFifo.mReadEnd   = 0;
    sReceiveFifo.mWrite     = 0;

    UARTDRV_Init(sUartHandle, &uartInit);

    CORE_DECLARE_NVIC_STATE;
    CORE_ENTER_NVIC(&sRxNvicMask);

    for (int i = 0; i < 2; i++)
    {
        queueNextReceive();
    }

    CORE_EXIT_NVIC();

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
