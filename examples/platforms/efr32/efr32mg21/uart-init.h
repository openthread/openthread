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
 *   This file defines the settings for the UART instance.
 *
 * NOTE: This is only meant to be included by uart.c
 *
 */
#ifndef __UART_INIT_H__
#define __UART_INIT_H__

#include "hal-config.h"

#define USART_PORT USART0

#define USART_INIT                                                                        \
    {                                                                                     \
        USART_PORT,                                           /* USART port */            \
            115200,                                           /* Baud rate */             \
            BSP_SERIAL_APP_TX_PORT,                           /* USART Tx port number */  \
            BSP_SERIAL_APP_RX_PORT,                           /* USART Rx port number */  \
            BSP_SERIAL_APP_TX_PIN,                            /* USART Tx pin number */   \
            BSP_SERIAL_APP_RX_PIN,                            /* USART Rx pin number */   \
            0,                                                /* UART instance number */  \
            (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */             \
            (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */                \
            (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/      \
            false,                                            /* Majority vote disable */ \
            HAL_SERIAL_APP_FLOW_CONTROL,                      /* Flow control */          \
            BSP_SERIAL_APP_CTS_PORT,                          /* CTS port number */       \
            BSP_SERIAL_APP_CTS_PIN,                           /* CTS pin number */        \
            BSP_SERIAL_APP_RTS_PORT,                          /* RTS port number */       \
            BSP_SERIAL_APP_RTS_PIN,                           /* RTS pin number */        \
            (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue,      /* RX operation queue */    \
            (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue,      /* TX operation queue */    \
    }

#endif