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

#include "asf.h"

#include <openthread/platform/uart.h>

enum
{
    kBaudRate          = 115200,
    kReceiveBufferSize = 128,
};

typedef struct RecvBuffer
{
    // The data buffer
    uint8_t mBuffer[kReceiveBufferSize];
    // The offset of the first item written to the list.
    uint16_t mHead;
    // The offset of the next item to be written to the list.
    uint16_t mTail;
} RecvBuffer;

static struct usart_module sUsartInstance;

static RecvBuffer sReceive;

static bool sTransmitDone = false;

static void usartReadCallback(struct usart_module *const usartModule)
{
    if (sReceive.mHead != (sReceive.mTail + 1) % kReceiveBufferSize)
    {
        sReceive.mTail = (sReceive.mTail + 1) % kReceiveBufferSize;
    }

    usart_read_job(&sUsartInstance, (uint16_t *)&sReceive.mBuffer[sReceive.mTail]);
}

static void usartWriteCallback(struct usart_module *const usartModule)
{
    sTransmitDone = true;
}

static void processReceive(void)
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

static void processTransmit(void)
{
    if (sTransmitDone)
    {
        sTransmitDone = false;
        otPlatUartSendDone();
    }
}

void samr21UartProcess(void)
{
    processReceive();
    processTransmit();
}

otError otPlatUartEnable(void)
{
    struct usart_config configUsart;

    usart_get_config_defaults(&configUsart);

    configUsart.baudrate    = 115200;
    configUsart.mux_setting = UART_SERCOM_MUX_SETTING;
    configUsart.pinmux_pad0 = UART_SERCOM_PINMUX_PAD0;
    configUsart.pinmux_pad1 = UART_SERCOM_PINMUX_PAD1;
    configUsart.pinmux_pad2 = UART_SERCOM_PINMUX_PAD2;
    configUsart.pinmux_pad3 = UART_SERCOM_PINMUX_PAD3;

    while (usart_init(&sUsartInstance, UART_SERCOM_MODULE, &configUsart) != STATUS_OK)
        ;

    usart_enable(&sUsartInstance);

    sReceive.mHead = 0;
    sReceive.mTail = 0;

    usart_register_callback(&sUsartInstance, usartWriteCallback, USART_CALLBACK_BUFFER_TRANSMITTED);
    usart_register_callback(&sUsartInstance, usartReadCallback, USART_CALLBACK_BUFFER_RECEIVED);

    usart_enable_callback(&sUsartInstance, USART_CALLBACK_BUFFER_TRANSMITTED);
    usart_enable_callback(&sUsartInstance, USART_CALLBACK_BUFFER_RECEIVED);

    usart_read_job(&sUsartInstance, (uint16_t *)&sReceive.mBuffer[sReceive.mTail]);

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    usart_disable(&sUsartInstance);

    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    if (usart_write_buffer_job(&sUsartInstance, (uint8_t *)aBuf, aBufLength) != STATUS_OK)
    {
        error = OT_ERROR_FAILED;
    }

    return error;
}
