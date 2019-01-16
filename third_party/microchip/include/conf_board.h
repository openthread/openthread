/**
 * \file
 *
 * \brief SAM R21 Xplained Pro board configuration.
 *
 * Copyright (c) 2014-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef CONF_BOARD_H_INCLUDED
#define CONF_BOARD_H_INCLUDED

#define CONF_BOARD_AT86RFX

#define AT86RFX_SPI_BAUDRATE            5000000UL

#if (BOARD==SAMR21G18_MODULE) || (BOARD==SAMR21B18_MODULE)

#define CONF_USER_ROW_EXIST

#define UART_SERCOM_MODULE              SERCOM2
#define UART_SERCOM_MUX_SETTING         USART_RX_3_TX_2_XCK_3
#define UART_SERCOM_PINMUX_PAD0         PINMUX_UNUSED
#define UART_SERCOM_PINMUX_PAD1         PINMUX_UNUSED
#define UART_SERCOM_PINMUX_PAD2         PINMUX_PA14C_SERCOM2_PAD2
#define UART_SERCOM_PINMUX_PAD3         PINMUX_PA15C_SERCOM2_PAD3
#define UART_SERCOM_DMAC_ID_TX          SERCOM2_DMAC_ID_TX
#define UART_SERCOM_DMAC_ID_RX          SERCOM2_DMAC_ID_RX

#endif

#if BOARD==SAMR21_XPLAINED_PRO

#define CONF_KIT_DATA_EXIST

#define UART_SERCOM_MODULE              EDBG_CDC_MODULE
#define UART_SERCOM_MUX_SETTING         EDBG_CDC_SERCOM_MUX_SETTING
#define UART_SERCOM_PINMUX_PAD0         EDBG_CDC_SERCOM_PINMUX_PAD0
#define UART_SERCOM_PINMUX_PAD1         EDBG_CDC_SERCOM_PINMUX_PAD1
#define UART_SERCOM_PINMUX_PAD2         EDBG_CDC_SERCOM_PINMUX_PAD2
#define UART_SERCOM_PINMUX_PAD3         EDBG_CDC_SERCOM_PINMUX_PAD3
#define UART_SERCOM_DMAC_ID_TX          EDBG_CDC_SERCOM_DMAC_ID_TX
#define UART_SERCOM_DMAC_ID_RX          EDBG_CDC_SERCOM_DMAC_ID_RX

#endif

#endif /* CONF_BOARD_H_INCLUDED */
