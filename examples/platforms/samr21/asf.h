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

#ifndef ASF_H
#define ASF_H

// From module: Common SAM0 compiler driver
#include <compiler.h>
#include <status_codes.h>

// From module: Generic board support
#include <board.h>

// From module: EXTINT - External Interrupt (Callback APIs)
#include <extint.h>
#include <extint_callback.h>

// From module: Interrupt management - SAM implementation
#include <interrupt.h>

// From module: NVM - Non-Volatile Memory
#include <nvm.h>

// From module: PORT - GPIO Pin Control
#include <port.h>

// From module: Part identification macros
#include <parts.h>

// From module: SYSTEM - Clock Management for SAMD21/R21/DA/HA
#include <clock.h>
#include <gclk.h>

// From module: SERCOM Callback API
#include <sercom.h>
#include <sercom_interrupt.h>

// From module: SERCOM I2C - Master Mode I2C (Polled APIs)
#include <i2c_common.h>
#include <i2c_master.h>

// From module: SERCOM SPI - Serial Peripheral Interface (Polled APIs)
#include <spi.h>

// From module: SERCOM USART - Serial Communications (Callback APIs)
#include <usart.h>
#include <usart_interrupt.h>

// From module: SYSTEM - Core System Driver
#include <system.h>

// From module: SYSTEM - I/O Pin Multiplexer
#include <pinmux.h>

// From module: SYSTEM - Interrupt Driver
#include <system_interrupt.h>

// From module: SYSTEM - Power Management for SAM D20/D21/R21/D09/D10/D11/DA/HA
#include <power.h>

// From module: SYSTEM - Reset Management for SAM D20/D21/R21/D09/D10/D11/DA/HA
#include <reset.h>

#include <phy.h>

#endif // ASF_H
