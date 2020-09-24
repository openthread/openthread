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
 *   This file defines the settings for the UART instance.
 *
 * NOTE: This is only meant to be included by uart.c
 *
 */
#ifndef __UART_INIT_H__
#define __UART_INIT_H__

#include "hal-config.h"

#define USART_PORT USART0

const UARTDRV_InitUart_t USART_INIT = {
    .port               = USART_PORT,                                           /* USART port */
    .baudRate           = HAL_SERIAL_APP_BAUD_RATE,                         /* Baud rate */
#if defined(_USART_ROUTELOC0_MASK)
    .portLocationTx     = BSP_SERIAL_APP_TX_LOC,                            /* USART Tx pin location number */
    .portLocationRx     = BSP_SERIAL_APP_RX_LOC,                            /* USART Rx pin location number */
#elif defined(_USART_ROUTE_MASK)
#error This configuration is not supported
    // .portLocation    = NULL;
#elif defined(_GPIO_USART_ROUTEEN_MASK)
    .txPort             = BSP_SERIAL_APP_TX_PORT,                           /* USART Tx port number */
    .rxPort             = BSP_SERIAL_APP_RX_PORT,                           /* USART Rx port number */
    .txPin              = BSP_SERIAL_APP_TX_PIN,                            /* USART Tx pin number */
    .rxPin              = BSP_SERIAL_APP_RX_PIN,                            /* USART Rx pin number */
    .uartNum            = 0,                                                /* UART instance number */
#endif
    .stopBits           = (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */
    .parity             = (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */
    .oversampling       = (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/
#if defined(USART_CTRL_MVDIS)
    .mvdis              = false,                                            /* Majority vote disable */
#endif // USART_CTRL_MVDIS
    .fcType             = HAL_SERIAL_APP_FLOW_CONTROL,                      /* Flow control */
    .ctsPort            = BSP_SERIAL_APP_CTS_PORT,                          /* CTS port number */
    .ctsPin             = BSP_SERIAL_APP_CTS_PIN,                           /* CTS pin number */
    .rtsPort            = BSP_SERIAL_APP_RTS_PORT,                          /* RTS port number */
    .rtsPin             = BSP_SERIAL_APP_RTS_PIN,                           /* RTS pin number */
    .rxQueue            = (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue,      /* RX operation queue */
    .txQueue            = (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue,      /* TX operation queue */
#if defined(_USART_ROUTELOC1_MASK)
    .portLocationCts    = BSP_SERIAL_APP_CTS_LOC,                           /* CTS location */
    .portLocationRts    = BSP_SERIAL_APP_RTS_LOC                            /* RTS location */
#endif // _USART_ROUTELOC1_MASK
};

#endif // __UART_INIT_H__
