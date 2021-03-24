/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
#include <string.h>

#include "openthread-system.h"
#include <openthread-core-config.h>

#include "utils/code_utils.h"
#include "utils/uart.h"

#include "ecode.h"
#include "em_core.h"
#include "sl_sleeptimer.h"
#include "sl_status.h"
#include "uartdrv.h"

#include "hal-config.h"
#include "platform-efr32.h"
#include "sl_uartdrv_usart_vcom_config.h"

#define HELPER1(x) USART##x##_RX_IRQn
#define HELPER2(x) HELPER1(x)
#define USART_IRQ HELPER2(SL_UARTDRV_USART_VCOM_PERIPHERAL_NO)

#define HELPER3(x) USART##x##_RX_IRQHandler
#define HELPER4(x) HELPER3(x)
#define USART_IRQHandler HELPER4(SL_UARTDRV_USART_VCOM_PERIPHERAL_NO)

enum
{
    kReceiveFifoSize = 128,
};

DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, sUartRxQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, sUartTxQueue);

static const UARTDRV_InitUart_t USART_INIT = {
    .port     = USART0,                   /* USART port */
    .baudRate = HAL_SERIAL_APP_BAUD_RATE, /* Baud rate */
#if defined(_USART_ROUTELOC0_MASK)
    .portLocationTx = BSP_SERIAL_APP_TX_LOC, /* USART Tx pin location number */
    .portLocationRx = BSP_SERIAL_APP_RX_LOC, /* USART Rx pin location number */
#elif defined(_USART_ROUTE_MASK)
#error This configuration is not supported
// .portLocation    = NULL;
#elif defined(_GPIO_USART_ROUTEEN_MASK)
    .txPort  = BSP_SERIAL_APP_TX_PORT, /* USART Tx port number */
    .rxPort  = BSP_SERIAL_APP_RX_PORT, /* USART Rx port number */
    .txPin   = BSP_SERIAL_APP_TX_PIN,  /* USART Tx pin number */
    .rxPin   = BSP_SERIAL_APP_RX_PIN,  /* USART Rx pin number */
    .uartNum = 0,                      /* UART instance number */
#endif
    .stopBits     = (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */
    .parity       = (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */
    .oversampling = (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/
#if defined(USART_CTRL_MVDIS)
    .mvdis = false,                                         /* Majority vote disable */
#endif                                                      // USART_CTRL_MVDIS
    .fcType  = HAL_SERIAL_APP_FLOW_CONTROL,                 /* Flow control */
    .ctsPort = BSP_SERIAL_APP_CTS_PORT,                     /* CTS port number */
    .ctsPin  = BSP_SERIAL_APP_CTS_PIN,                      /* CTS pin number */
    .rtsPort = BSP_SERIAL_APP_RTS_PORT,                     /* RTS port number */
    .rtsPin  = BSP_SERIAL_APP_RTS_PIN,                      /* RTS pin number */
    .rxQueue = (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue, /* RX operation queue */
    .txQueue = (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue, /* TX operation queue */
#if defined(_USART_ROUTELOC1_MASK)
    .portLocationCts = BSP_SERIAL_APP_CTS_LOC, /* CTS location */
    .portLocationRts = BSP_SERIAL_APP_RTS_LOC  /* RTS location */
#endif                                         // _USART_ROUTELOC1_MASK
};

static UARTDRV_HandleData_t sUartHandleData;
static UARTDRV_Handle_t     sUartHandle = &sUartHandleData;

// In order to reduce the probability of data loss due to disabled interrupts, we use
// two duplicate receive buffers so we can always have one "active" receive request.
#define RECEIVE_BUFFER_SIZE 128
static uint8_t       sReceiveBuffer1[RECEIVE_BUFFER_SIZE];
static uint8_t       sReceiveBuffer2[RECEIVE_BUFFER_SIZE];
static uint8_t       lastCount           = 0;
static volatile bool sCheckForTxComplete = false;

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
static void processTransmit(void);

static void receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    OT_UNUSED_VARIABLE(aStatus);

    // We can only write if incrementing mTail doesn't equal mHead
    if (sReceiveFifo.mHead != (sReceiveFifo.mTail + aCount - lastCount) % kReceiveFifoSize)
    {
        memcpy(sReceiveFifo.mBuffer + sReceiveFifo.mTail, aData + lastCount, aCount - lastCount);
        sReceiveFifo.mTail = (sReceiveFifo.mTail + aCount - lastCount) % kReceiveFifoSize;
        lastCount          = 0;
    }

    UARTDRV_Receive(aHandle, aData, aCount, receiveDone);
    otSysEventSignalPending();
}

static void transmitDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aStatus);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aCount);

    // This value will be used later in processTransmit() to call otPlatUartSendDone()
    sCheckForTxComplete = true;

    otSysEventSignalPending();
}

static void processReceive(void)
{
    uint8_t *       aData;
    UARTDRV_Count_t aCount, remaining;
    CORE_ATOMIC_SECTION(UARTDRV_GetReceiveStatus(sUartHandle, &aData, &aCount, &remaining);

                        if (aCount > lastCount) {
                            memcpy(sReceiveFifo.mBuffer + sReceiveFifo.mTail, aData + lastCount, aCount - lastCount);
                            sReceiveFifo.mTail = (sReceiveFifo.mTail + aCount - lastCount) % kReceiveFifoSize;
                            lastCount          = aCount;
                        })

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

static void flushTimeoutAlarmCallback(sl_sleeptimer_timer_handle_t *aHandle, void *aData)
{
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aData);
    bool *flushTimedOut = (bool *)aData;
    *flushTimedOut      = true;
}

otError otPlatUartFlush(void)
{
    otError                      error         = OT_ERROR_NONE;
    sl_status_t                  status        = SL_STATUS_OK;
    volatile bool                flushTimedOut = false;
    sl_sleeptimer_timer_handle_t flushTimer;

    // Start flush timeout timer
    status = sl_sleeptimer_start_timer_ms(&flushTimer, OPENTHREAD_CONFIG_EFR32_UART_TX_FLUSH_TIMEOUT_MS,
                                          flushTimeoutAlarmCallback, NULL, 0,
                                          SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    otEXPECT_ACTION(status == SL_STATUS_OK, error = OT_ERROR_FAILED);

    // Block until DMA has finished transmitting every buffer in sUartTxQueue and becomes idle
    uint8_t transmitQueueDepth = 0;
    bool    uartFullyFlushed   = false;
    bool    uartIdle           = false;

    do
    {
        // Check both peripheral status and queue depth
        transmitQueueDepth = UARTDRV_GetTransmitDepth(sUartHandle);
        uartIdle           = UARTDRV_GetPeripheralStatus(sUartHandle) & (UARTDRV_STATUS_TXIDLE | UARTDRV_STATUS_TXC);
        uartFullyFlushed   = uartIdle && (transmitQueueDepth == 0);
    } while (!uartFullyFlushed && !flushTimedOut);

    sl_sleeptimer_stop_timer(&flushTimer);

    if (flushTimedOut)
    {
        // Abort all transmits
        UARTDRV_Abort(sUartHandle, uartdrvAbortTransmit);
    }

exit:
    return error;
}

static void processTransmit(void)
{
    // NOTE: This check needs to be done in here and cannot be done in transmitDone because the transmit may not be
    // fully complete when the transmitDone callback is called.
    if (!sCheckForTxComplete)
    {
        return;
    }

    bool    queueEmpty = UARTDRV_GetPeripheralStatus(sUartHandle) & (UARTDRV_STATUS_TXIDLE | UARTDRV_STATUS_TXC);
    uint8_t transmitQueueDepth = UARTDRV_GetTransmitDepth(sUartHandle);

    if (transmitQueueDepth == 0 && queueEmpty)
    {
        sCheckForTxComplete = false;
        otPlatUartSendDone();
    }
}

void USART_IRQHandler(void)
{
    otSysEventSignalPending();
}

otError otPlatUartEnable(void)
{
    UARTDRV_InitUart_t uartInit = USART_INIT;

    sReceiveFifo.mHead = 0;
    sReceiveFifo.mTail = 0;

    UARTDRV_Init(sUartHandle, &uartInit);

    // When one receive request is completed, the other buffer is used for a separate receive request, issued
    // immediately.
    UARTDRV_Receive(sUartHandle, sReceiveBuffer1, RECEIVE_BUFFER_SIZE, receiveDone);
    UARTDRV_Receive(sUartHandle, sReceiveBuffer2, RECEIVE_BUFFER_SIZE, receiveDone);

    // Enable USART0 interrupt to wake OT task when data arrives
    NVIC_ClearPendingIRQ(USART_IRQ);
    NVIC_EnableIRQ(USART_IRQ);
    USART_IntEnable(SL_UARTDRV_USART_VCOM_PERIPHERAL, USART_IF_RXDATAV);

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error  = OT_ERROR_NONE;
    Ecode_t status = ECODE_EMDRV_UARTDRV_OK;

    // Ensure that no ongoing transmits have started finishing.
    // This prevents queued buffers from being modified before they are transmitted
    if (sCheckForTxComplete)
    {
        otPlatUartFlush();
    }

    status = UARTDRV_Transmit(sUartHandle, (uint8_t *)aBuf, aBufLength, transmitDone);
    otEXPECT_ACTION(status == ECODE_EMDRV_UARTDRV_OK, error = OT_ERROR_FAILED);

exit:
    return error;
}

void efr32UartProcess(void)
{
    processReceive();
    processTransmit();
}
