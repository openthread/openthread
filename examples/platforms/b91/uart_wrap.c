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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <openthread/platform/debug_uart.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>

#include "platform-b91.h"
#include "utils/code_utils.h"

enum
{
    kBaudRate          = 115200,
    kReceiveBufferSize = 128,
};

static const uint8_t *sTransmitBuffer = NULL;
static uint16_t       sTransmitLength = 0;

typedef struct RecvBuffer
{
    // The data buffer
    unsigned char mBuffer[kReceiveBufferSize];
    // The offset of the first item written to the list.
    unsigned short mHead;
    // The offset of the next item to be written to the list.
    unsigned short mTail;
} RecvBuffer;

static RecvBuffer sReceive;

static void init_recv_buffer()
{
    sReceive.mHead = 0;
    sReceive.mTail = 0;
}

otError otPlatUartEnable(void)
{
    unsigned short div;
    unsigned char  bwpc;

    init_recv_buffer();
    uart_reset(UART0);
    uart_set_pin(UART0_TX_PB2, UART0_RX_PB3); // uart tx/rx pin set
    uart_cal_div_and_bwpc(115200, sys_clk.pclk * 1000 * 1000, &div, &bwpc);

    uart_init(UART0, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    plic_interrupt_enable(IRQ19_UART0);
    uart_tx_irq_trig_level(UART0, 0);
    uart_rx_irq_trig_level(UART0, 1);

    uart_set_irq_mask(UART0, UART_RX_IRQ_MASK);
    uart_set_irq_mask(UART0, UART_ERR_IRQ_MASK);

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sTransmitBuffer == NULL, error = OT_ERROR_BUSY);

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

otError otPlatUartFlush(void)
{
    otEXPECT(sTransmitBuffer != NULL);

    for (; sTransmitLength > 0; sTransmitLength--)
    {
        uart_send_byte(UART0, *sTransmitBuffer++);
    }

    sTransmitBuffer = NULL;
    return OT_ERROR_NONE;

exit:
    return OT_ERROR_INVALID_STATE;
}

void processTransmit(void)
{
    otPlatUartFlush();
    otPlatUartSendDone();
}

void b91UartProcess(void)
{
    processReceive();
    processTransmit();
}

void irq_uart0_handler(void)
{
    uint8_t byte;

    if (uart_get_irq_status(UART0, UART_RXBUF_IRQ_STATUS))
    {
        byte = uart_read_byte(UART0);

        // We can only write if incrementing mTail doesn't equal mHead
        if (sReceive.mHead != (sReceive.mTail + 1) % kReceiveBufferSize)
        {
            sReceive.mBuffer[sReceive.mTail] = byte;
            sReceive.mTail                   = (sReceive.mTail + 1) % kReceiveBufferSize;
        }
    }
}
