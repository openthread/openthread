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
    kReceiveFifoSize     = 128,
    kDmaBlockSize        = 32,
    kConcurrentRxBuffers = 2,
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
static UARTDRV_Handle_t     sUartHandle = &sUartHandleData;
static const uint8_t *      sTransmitBuffer;
static volatile uint16_t    sTransmitLength;
static volatile uint8_t     sDeferredReceives;

typedef struct ReceiveFifo_t
{
    // The data buffer
    uint8_t mBuffer[kReceiveFifoSize];
    // The offset of the first item to be read from the list (unwrapped)
    uint16_t mReadStart;
    // The offset of the last item to be read plus one (unwrapped)
    volatile uint16_t mReadEnd;
    // The offset of first unused item (unwrapped)
    volatile uint16_t mWrite;
    // Last number of items value in current transfer
    volatile uint16_t mLastCount;
} ReceiveFifo_t;

static ReceiveFifo_t sReceiveFifo;

static bool enqueueNextReceive(void);

static void updateReceiveProgress(uint8_t *aData, UARTDRV_Count_t aCount)
{
    if (aCount < sReceiveFifo.mLastCount)
    {
        // aCount has wrapped
        sReceiveFifo.mReadEnd += kDmaBlockSize - sReceiveFifo.mLastCount;
        sReceiveFifo.mLastCount = 0;
    }

    sReceiveFifo.mReadEnd += aCount - sReceiveFifo.mLastCount;
    sReceiveFifo.mLastCount = aCount;
}

static void receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    updateReceiveProgress(aData, aCount);

    if (!enqueueNextReceive() && sDeferredReceives < UINT8_MAX)
    {
        // A failure to enqueue the next receive is due to no free blocks remaining in the buffer. Defer enqueueing
        // the next receive operation to processReceive() (running in the main execution context) where the
        // contents of the buffer shall firstly be emptied. In the mean time, assuming all (kConcurrentRxBuffers)
        // receive operations have been deferred, flow control RTS will be deasserted.
        assert(sDeferredReceives < kConcurrentRxBuffers);
        sDeferredReceives += 1;
    }
}

static bool enqueueNextReceive(void)
{
    bool           result;
    const uint16_t wrappedWrite     = sReceiveFifo.mWrite % kReceiveFifoSize;
    const uint16_t wrappedReadStart = sReceiveFifo.mReadStart % kReceiveFifoSize;
    Ecode_t        status;

    // Buffer has a remaining block or buffer is totally empty
    result = wrappedWrite != wrappedReadStart || sReceiveFifo.mReadEnd == sReceiveFifo.mReadStart;

    otEXPECT(result);

    status = UARTDRV_Receive(sUartHandle, sReceiveFifo.mBuffer + wrappedWrite, kDmaBlockSize, receiveDone);
    assert(ECODE_OK == status);
    otEXPECT_ACTION(ECODE_OK == status, result = false);

    sReceiveFifo.mWrite += kDmaBlockSize;
exit:
    return result;
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
    uint8_t         numEnqueuedReceives = 0;
    uint16_t        wrappedReadStart;
    uint16_t        wrappedReadEnd;
    uint16_t        readLength;
    CORE_DECLARE_NVIC_STATE;

    CORE_ENTER_NVIC(&sRxNvicMask);

    UARTDRV_GetReceiveStatus(sUartHandle, &buffer, &itemsReceived, &itemsRemaining);
    updateReceiveProgress(buffer, itemsReceived);

    readEnd = sReceiveFifo.mReadEnd;

    CORE_EXIT_NVIC();

    wrappedReadStart = sReceiveFifo.mReadStart % kReceiveFifoSize;
    wrappedReadEnd   = readEnd % kReceiveFifoSize;

    if (wrappedReadStart > wrappedReadEnd)
    {
        readLength = kReceiveFifoSize - wrappedReadStart;
        otPlatUartReceived(sReceiveFifo.mBuffer + wrappedReadStart, readLength);

        sReceiveFifo.mReadStart += readLength;
    }

    wrappedReadStart = sReceiveFifo.mReadStart % kReceiveFifoSize;

    if (sReceiveFifo.mReadStart != readEnd)
    {
        readLength = wrappedReadEnd - wrappedReadStart;
        otPlatUartReceived(sReceiveFifo.mBuffer + wrappedReadStart, readLength);

        assert(sReceiveFifo.mReadStart + readLength == readEnd);
        sReceiveFifo.mReadStart = readEnd;
    }

    CORE_ENTER_NVIC(&sRxNvicMask);

    // Now the buffer has been emptied, attempt to enqueue any receive operations that previously failed to enqueue due
    // to a full buffer.

    for (uint8_t i = 0; i < sDeferredReceives; i++)
    {
        if (enqueueNextReceive())
        {
            numEnqueuedReceives += 1;
        }
    }

    assert(sDeferredReceives >= numEnqueuedReceives);
    sDeferredReceives -= numEnqueuedReceives;

    CORE_EXIT_NVIC();
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
    otError        error               = OT_ERROR_NONE;
    UARTDRV_Init_t uartInit            = USART_INIT;
    uint8_t        numEnqueuedReceives = 0;

    memset(&sRxNvicMask, 0, sizeof(sRxNvicMask));
    CORE_NvicMaskSetIRQ(LDMA_IRQn, &sRxNvicMask);
    CORE_NvicMaskSetIRQ(USART_PORT_RX_IRQn, &sRxNvicMask);

    sReceiveFifo.mReadStart = 0;
    sReceiveFifo.mReadEnd   = 0;
    sReceiveFifo.mWrite     = 0;
    sReceiveFifo.mLastCount = 0;
    sDeferredReceives       = 0;
    sTransmitLength         = 0;
    sTransmitBuffer         = NULL;

    otEXPECT_ACTION(ECODE_OK == UARTDRV_Init(sUartHandle, &uartInit), error = OT_ERROR_FAILED);

    CORE_DECLARE_NVIC_STATE;
    CORE_ENTER_NVIC(&sRxNvicMask);

    for (uint8_t i = 0; i < kConcurrentRxBuffers; i++)
    {
        if (enqueueNextReceive())
        {
            numEnqueuedReceives += 1;
        }
    }

    CORE_EXIT_NVIC();

    assert(numEnqueuedReceives == kConcurrentRxBuffers);
    otEXPECT_ACTION(numEnqueuedReceives == kConcurrentRxBuffers, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;
    Ecode_t status;

    otEXPECT_ACTION(sTransmitBuffer == NULL, error = OT_ERROR_BUSY);

    sTransmitBuffer = aBuf;
    sTransmitLength = aBufLength;

    status = UARTDRV_Transmit(sUartHandle, (uint8_t *)sTransmitBuffer, sTransmitLength, transmitDone);
    assert(ECODE_OK == status);
    otEXPECT_ACTION(ECODE_OK == status, error = OT_ERROR_FAILED);
exit:
    return error;
}

void efr32UartProcess(void)
{
    processReceive();
    processTransmit();
}
