/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include <openthread/platform/uart.h>

#include "platform-da15000.h"

#include "hw_gpio.h"
#include "hw_uart.h"

static bool    sUartWriteDone = false;
static bool    sUartReadDone  = false;
static char *  sInitBuf       = NULL;
static uint8_t sUartBuf;

static void UartSignalWrite(void *p, uint16_t transferred)
{
    if (transferred)
    {
        sUartWriteDone = true;
    }
}

static void UartSignalRead(void *p, uint16_t transferred)
{
    if (transferred)
    {
        sUartReadDone = true;
    }
}

otError otPlatUartEnable(void)
{
    // clang-format off
    uart_config_ex uart_init =
    {
        .baud_rate = HW_UART_BAUDRATE_115200,
        .data      = HW_UART_DATABITS_8,
        .parity    = HW_UART_PARITY_NONE,
        .stop      = HW_UART_STOPBITS_1,
        .auto_flow_control = 0,
        .use_fifo  = 1,
        .use_dma   = 1,
        .tx_fifo_tr_lvl = 0,
        .rx_fifo_tr_lvl = 0,
        .tx_dma_channel = HW_DMA_CHANNEL_3,
        .rx_dma_channel = HW_DMA_CHANNEL_2,
    };
    // clang-format on

    hw_uart_init_ex(HW_UART2, &uart_init);

    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_3, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_TX);
    hw_gpio_set_pin_function(HW_GPIO_PORT_2, HW_GPIO_PIN_3, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_RX);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_5, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);

    hw_uart_receive(HW_UART2, &sUartBuf, 1, UartSignalRead, NULL);

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

void da15000UartProcess(void)
{
    if (sUartReadDone)
    {
        otPlatUartReceived(&sUartBuf, 1);
        sUartReadDone = false;
        hw_uart_receive(HW_UART2, &sUartBuf, 1, UartSignalRead, NULL);
    }

    if (sUartWriteDone)
    {
        sUartWriteDone = false;
        otPlatUartSendDone();
    }

    if (sInitBuf == NULL)
    {
        sInitBuf = "\n";
        otPlatUartReceived((uint8_t *)sInitBuf, 1);
    }
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    hw_uart_send(HW_UART2, aBuf, aBufLength, UartSignalWrite, NULL);

    return OT_ERROR_NONE;
}
