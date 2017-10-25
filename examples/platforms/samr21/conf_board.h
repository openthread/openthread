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

#ifndef CONF_BOARD_H_INCLUDED
#define CONF_BOARD_H_INCLUDED

#define CONF_BOARD_AT86RFX

#define AT86RFX_SPI_BAUDRATE            5000000UL

#if BOARD==SAMR21_XPLAINED_PRO

#define CONF_IEEE_ADDRESS 0x0001020304050607LL

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
