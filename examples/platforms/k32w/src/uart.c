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

/* NXP UART includes */
#include "board.h"
#include "fsl_clock.h"
#include "fsl_flexcomm.h"
#include "fsl_reset.h"
#include "fsl_usart.h"

/* Openthread general includes */
#include <utils/code_utils.h>
#include "openthread/platform/uart.h"

#if USE_RTOS
#include "UART_Serial_Adapter.h"
#include "openthread-system.h"
#endif

/* Defines */
#define K32W_UART_RX_BUFFERS 256
#define K32W_UART_BAUD_RATE 115200

/* Structures */
typedef struct
{
    uint8_t buffer[K32W_UART_RX_BUFFERS];
    uint8_t head;
    uint8_t tail;
    bool    isFull;
} rxRingBuffer;

/* Enums */
typedef enum
{
    UART_IDLE, /* TX idle. */
    UART_BUSY, /* TX busy. */
} K32WUartStates;

/* Private functions declaration */
static void     K32WResetRxRingBuffer(rxRingBuffer *aRxRing);
static uint8_t *K32WPopRxRingBuffer(rxRingBuffer *aRxRing);
static bool     K32WIsEmptyRxRingBuffer(rxRingBuffer *aRxRing);
static void     K32WPushRxRingBuffer(rxRingBuffer *aRxRing, uint8_t aCharacter);
static void     K32WProcessReceive(void);
static void     K32WProcessTransmit(void);
static void     USART0_IRQHandler(USART_Type *base, usart_handle_t *handle);

/* Private variables declaration */
static bool           sIsUartInitialized; /* Is UART module initialized? */
static bool           sIsTransmitDone;    /* Transmit done for the latest user-data buffer */
static usart_handle_t sUartHandle;        /* Handle to the UART module */
static rxRingBuffer   sUartRxRing;        /* Receive Ring Buffer */

void K32WUartProcess(void)
{
    if (sIsUartInitialized)
    {
        K32WProcessTransmit();
        K32WProcessReceive();
    }
}

otError otPlatUartEnable(void)
{
    status_t uartStatus;
    otError  error = OT_ERROR_NONE;

    usart_config_t config;
    uint32_t       kPlatformClock = CLOCK_GetFreq(kCLOCK_Fro32M);

    if (!sIsUartInitialized)
    {
        /* attach clock for USART0 */
        CLOCK_AttachClk(kOSC32M_to_USART_CLK);

        /* reset FLEXCOMM0 for USART0 */
        RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);

        memset(&sUartHandle, 0, sizeof(sUartHandle));
        sUartHandle.txState = UART_IDLE;

        USART_GetDefaultConfig(&config);
        config.baudRate_Bps = K32W_UART_BAUD_RATE;
        config.enableTx     = true;
        config.enableRx     = true;
        config.rxWatermark  = kUSART_RxFifo1;

        uartStatus = USART_Init(USART0, &config, kPlatformClock);
        otEXPECT_ACTION(uartStatus == kStatus_Success, error = OT_ERROR_INVALID_ARGS);

        K32WResetRxRingBuffer(&sUartRxRing);

        FLEXCOMM_SetIRQHandler(USART0, (flexcomm_irq_handler_t)USART0_IRQHandler, &sUartHandle);

        /* Enable interrupt in NVIC. */
#if USE_RTOS
        NVIC_SetPriority(USART0_IRQn, gUartIsrPrio_c >> (8 - __NVIC_PRIO_BITS));
        NVIC_ClearPendingIRQ(USART0_IRQn);
#endif
        EnableIRQ(USART0_IRQn);

        /* Enable RX interrupt. */
        USART_EnableInterrupts(USART0, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);

        sIsUartInitialized = true;
    }

exit:
    return error;
}

otError otPlatUartDisable(void)
{
    sIsUartInitialized = false;
    USART_Deinit(USART0);

    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(!sUartHandle.txData, error = OT_ERROR_BUSY);
    sUartHandle.txData        = (uint8_t *)aBuf;
    sUartHandle.txDataSize    = aBufLength;
    sUartHandle.txDataSizeAll = aBufLength;

    /* Enable transmitter interrupt. */
    USART_EnableInterrupts(USART0, kUSART_TxLevelInterruptEnable);

exit:
    return error;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * Function used for blocking-write to the UART module.
 *
 * @param[in] aBuf             Pointer to the character buffer
 * @param[in] len              Length of the character buffer
 */
void K32WWriteBlocking(const uint8_t *aBuf, uint32_t len)
{
    otEXPECT(sIsUartInitialized && sUartHandle.txState != UART_BUSY);

    sUartHandle.txState = UART_BUSY;
    USART_WriteBlocking(USART0, aBuf, len);
    sUartHandle.txState = UART_IDLE;

exit:
    return;
}

/**
 * Process TX characters in process context and call the upper layer call-backs.
 */
static void K32WProcessTransmit(void)
{
    if (sIsTransmitDone)
    {
        sIsTransmitDone = false;
        otPlatUartSendDone();
    }
}

/**
 * Process RX characters in process context and call the upper layer call-backs.
 */
static void K32WProcessReceive(void)
{
    uint8_t  rx[K32W_UART_RX_BUFFERS];
    uint16_t rxIndex = 0;
    uint8_t *pCharacter;

    while ((pCharacter = K32WPopRxRingBuffer(&sUartRxRing)) != NULL)
    {
        rx[rxIndex] = *pCharacter;
        rxIndex++;
    }
    otPlatUartReceived(rx, rxIndex);
}

static void USART0_IRQHandler(USART_Type *base, usart_handle_t *handle)
{
    (void)base;
    (void)handle;

    bool isReceiveEnabled = true;
    bool isSendEnabled    = (sUartHandle.txDataSize != 0);

    /* If RX overrun. */
    if (USART0->FIFOSTAT & USART_FIFOSTAT_RXERR_MASK)
    {
        /* Clear RX error state. */
        USART0->FIFOSTAT |= USART_FIFOSTAT_RXERR_MASK;
        /* clear RX FIFO */
        USART0->FIFOCFG |= USART_FIFOCFG_EMPTYRX_MASK;
    }
    while ((isReceiveEnabled && (USART0->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK)) ||
           (isSendEnabled && (USART0->FIFOSTAT & USART_FIFOSTAT_TXNOTFULL_MASK)))
    {
        /* RX: an interrupt is fired for each received character */
        if (isReceiveEnabled && (USART0->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK))
        {
            volatile uint8_t rx_data = USART_ReadByte(USART0);

            {
                K32WPushRxRingBuffer(&sUartRxRing, rx_data);
            }
        }

        /* There are times when the UART interrupt fires unnecessarily
         * having the TXNOTFULL and TXEMPY bits set. Disable this!
         */
        if ((!sUartHandle.txDataSize) && (USART0->FIFOSTAT & USART_FIFOSTAT_TXNOTFULL_MASK) &&
            (USART0->FIFOSTAT & USART_FIFOSTAT_TXEMPTY_MASK))
        {
            USART0->FIFOINTENCLR = USART_FIFOINTENCLR_TXLVL_MASK;
        }

        /* TX: an interrupt is fired for each sent character */
        if (isSendEnabled && (USART0->FIFOSTAT & USART_FIFOSTAT_TXNOTFULL_MASK))
        {
            USART0->FIFOWR = *sUartHandle.txData;
            sUartHandle.txDataSize--;
            sUartHandle.txData++;
            isSendEnabled = (sUartHandle.txDataSize != 0);

            if (!isSendEnabled)
            {
                USART0->FIFOINTENCLR = USART_FIFOINTENCLR_TXLVL_MASK;
                sUartHandle.txData   = NULL;
                sIsTransmitDone      = true;
            }
        }

#if USE_RTOS
        otSysEventSignalPending();
#endif
    }
}

/**
 * Function used to push a received character to the RX Ring buffer.
 * In case the ring buffer is full, the oldest address is overwritten.
 *
 * @param[in] aRxRing             Pointer to the RX Ring Buffer
 * @param[in] aCharacter          The received character
 */
static void K32WPushRxRingBuffer(rxRingBuffer *aRxRing, uint8_t aCharacter)
{
    aRxRing->buffer[aRxRing->head] = aCharacter;

    if (aRxRing->isFull)
    {
        aRxRing->tail = (aRxRing->tail + 1) % K32W_UART_RX_BUFFERS;
    }

    aRxRing->head   = (aRxRing->head + 1) % K32W_UART_RX_BUFFERS;
    aRxRing->isFull = (aRxRing->head == aRxRing->tail);
}

/**
 * Function used to pop the address of a received character from the RX Ring buffer
 * Process Context: the consumer will pop frames with the interrupts disabled
 *                  to make sure the interrupt context(ISR) doesn't push in
 *                  the middle of a pop.
 *
 * @param[in] aRxRing           Pointer to the RX Ring Buffer
 *
 * @return    tsRxFrameFormat   Pointer to a received character
 * @return    NULL              In case the RX Ring buffer is empty
 */
static uint8_t *K32WPopRxRingBuffer(rxRingBuffer *aRxRing)
{
    uint8_t *pCharacter = NULL;

    DisableIRQ(USART0_IRQn);
    if (!K32WIsEmptyRxRingBuffer(aRxRing))
    {
        pCharacter      = &(aRxRing->buffer[aRxRing->tail]);
        aRxRing->isFull = false;
        aRxRing->tail   = (aRxRing->tail + 1) % K32W_UART_RX_BUFFERS;
    }
    EnableIRQ(USART0_IRQn);

    return pCharacter;
}

/**
 * Function used to check if an RX Ring buffer is empty
 *
 * @param[in] aRxRing           Pointer to the RX Ring Buffer
 *
 * @return    TRUE              RX Ring Buffer is not empty
 * @return    FALSE             RX Ring Buffer is empty
 */
static bool K32WIsEmptyRxRingBuffer(rxRingBuffer *aRxRing)
{
    return (!aRxRing->isFull && (aRxRing->head == aRxRing->tail));
}

/**
 * Function used to init/reset an RX Ring Buffer
 *
 * @param[in] aRxRing         Pointer to an RX Ring Buffer
 */
static void K32WResetRxRingBuffer(rxRingBuffer *aRxRing)
{
    aRxRing->head   = 0;
    aRxRing->tail   = 0;
    aRxRing->isFull = false;
}

/**
 * The UART driver weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatUartSendDone(void)
{
}

OT_TOOL_WEAK void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);
}
