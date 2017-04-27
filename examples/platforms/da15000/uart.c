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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <hw_uart.h>

#include "openthread/platform/uart.h"

#include <common/code_utils.hpp>
#include "hw_gpio.h"
#include "platform-da15000.h"

static int sInFd;
static int sOutFd;
void UartBuffClear(void);

ThreadError otPlatUartEnable(void)
{
    ThreadError error = kThreadError_None;
    uart_config uart_init =
    {
        .baud_rate = HW_UART_BAUDRATE_9600,
        .data      = HW_UART_DATABITS_8,
        .stop      = HW_UART_STOPBITS_1,
        .parity    = HW_UART_PARITY_NONE,
        .use_dma   = 0,
        .use_fifo  = 1,
        .rx_dma_channel = HW_DMA_CHANNEL_0,
        .tx_dma_channel = HW_DMA_CHANNEL_1,
    };

    hw_uart_init(CONFIG_RETARGET_UART, &uart_init);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_3,
                             HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART_TX);
    hw_gpio_set_pin_function(HW_GPIO_PORT_2, HW_GPIO_PIN_3,
                             HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART_RX);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_5, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_GPIO);
    UartBuffClear();

    return error;
}

ThreadError otPlatUartDisable(void)
{
    ThreadError error = kThreadError_None;

    close(sInFd);
    close(sOutFd);

    return error;
}


void da15000UartProcess(void)
{
    uint8_t aBuf;

    // Wait until received data are available
    if (hw_uart_read_buf_empty(HW_UART1))
    {
        return;
    }

    // Read element from the receive FIFO
    aBuf = UBA(HW_UART1)->UART2_RBR_THR_DLL_REG;
    otPlatUartReceived(&aBuf, 1);
}


void UartBuffClear(void)
{

    volatile uint8_t aBuf;

    while (hw_uart_read_buf_empty(HW_UART1) == 0)
    {
        aBuf += UBA(HW_UART1)->UART2_RBR_THR_DLL_REG;
    }


}


ThreadError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    ThreadError error = kThreadError_None;
    hw_uart_write_buffer(HW_UART1, aBuf, aBufLength);

    otPlatUartSendDone();
    return error;
}



