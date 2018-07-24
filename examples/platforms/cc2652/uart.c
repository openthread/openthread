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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <openthread/platform/debug_uart.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>

#include "utils/code_utils.h"

#include <utils/code_utils.h>

#include <driverlib/ioc.h>
#include <driverlib/prcm.h>
#include <driverlib/sys_ctrl.h>
#include <driverlib/uart.h>

/**
 * @note This will configure the uart for 115200 baud 8-N-1, no HW flow control
 * RX pin IOID_2 TX pin IOID_3.
 *
 * If the DEBUG UART is enabled, IOID_0 = debug tx, IOID_1 = debug rx
 */

enum
{
    CC2652_RECV_CIRC_BUFF_SIZE = 256,
};

static uint8_t const *sSendBuffer = NULL;
static uint16_t       sSendLen    = 0;

static uint8_t  sReceiveBuffer[CC2652_RECV_CIRC_BUFF_SIZE];
static uint16_t sReceiveHeadIdx = 0;
static uint16_t sReceiveTailIdx = 0;

void UART0_intHandler(void);

static void uart_power_control(uint32_t who_base, int turnon)
{
    uint32_t value;

    if (turnon)
    {
        /* UART0 is in the SERIAL domain
         * UART1 is in the PERIPH domain.
         * See: ti/devices/cc13x2_cc26x2/driverlib/pcrm.h, line: 658
         */
        value = (who_base == UART0_BASE) ? PRCM_DOMAIN_SERIAL : PRCM_DOMAIN_PERIPH;
        PRCMPowerDomainOn(value);

        while (PRCMPowerDomainStatus(value) != PRCM_DOMAIN_POWER_ON)
            ;

        value = (who_base == UART0_BASE) ? PRCM_PERIPH_UART0 : PRCM_PERIPH_UART1;
        PRCMPeripheralRunEnable(value);
        PRCMPeripheralSleepEnable(value);
        PRCMPeripheralDeepSleepEnable(value);
        PRCMLoadSet();

        while (!PRCMLoadGet())
            ;
    }
    else
    {
        if (who_base == UART0_BASE)
        {
            PRCMPeripheralRunDisable(PRCM_PERIPH_UART0);
            PRCMPeripheralSleepDisable(PRCM_PERIPH_UART0);
            PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_UART0);
            PRCMLoadSet();
            PRCMPowerDomainOff(PRCM_DOMAIN_SERIAL);
        }
        else
        {
            /* we never turn the debug uart off */
        }
    }
}

/**
 * Function documented in platform/uart.h
 */
otError otPlatUartEnable(void)
{
    uart_power_control(UART0_BASE, true);

    IOCPinTypeUart(UART0_BASE, IOID_2, IOID_3, IOID_UNUSED, IOID_UNUSED);
    UARTConfigSetExpClk(UART0_BASE, SysCtrlClockGet(), 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    /* Note: UART1 could use IRQs
     * However, for reasons of debug simplicity
     * we do not use IRQs for the debug uart
     */
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntRegister(UART0_BASE, UART0_intHandler);
    UARTEnable(UART0_BASE);

    return OT_ERROR_NONE;
}

/**
 * Function documented in platform/uart.h
 */
otError otPlatUartDisable(void)
{
    UARTDisable(UART0_BASE);
    UARTIntUnregister(UART0_BASE);
    UARTIntDisable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    IOCPortConfigureSet(IOID_2, IOC_PORT_GPIO, IOC_STD_INPUT);
    IOCPortConfigureSet(IOID_3, IOC_PORT_GPIO, IOC_STD_INPUT);

    uart_power_control(UART0_BASE, false);

    return OT_ERROR_NONE;
}

/**
 * Function documented in platform/uart.h
 */
otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(sSendBuffer == NULL, error = OT_ERROR_BUSY);

    sSendBuffer = aBuf;
    sSendLen    = aBufLength;

exit:
    return error;
}

/**
 * @brief process the receive side of the buffers
 */
static void processReceive(void)
{
    while (sReceiveHeadIdx != sReceiveTailIdx)
    {
        uint16_t tailIdx;

        if (sReceiveHeadIdx < sReceiveTailIdx)
        {
            tailIdx = sReceiveTailIdx;
            otPlatUartReceived(&(sReceiveBuffer[sReceiveHeadIdx]), tailIdx - sReceiveHeadIdx);
            sReceiveHeadIdx = tailIdx;
        }
        else
        {
            tailIdx = CC2652_RECV_CIRC_BUFF_SIZE;
            otPlatUartReceived(&(sReceiveBuffer[sReceiveHeadIdx]), tailIdx - sReceiveHeadIdx);
            sReceiveHeadIdx = 0;
        }
    }
}

/**
 * @brief process the transmit side of the buffers
 */
static void processTransmit(void)
{
    otEXPECT(sSendBuffer != NULL);

    for (; sSendLen > 0; sSendLen--)
    {
        UARTCharPut(UART0_BASE, *sSendBuffer);
        sSendBuffer++;
    }

    sSendBuffer = NULL;
    sSendLen    = 0;
    otPlatUartSendDone();

exit:
    return;
}

/**
 * Function documented in platform-cc2652.h
 */
void cc2652UartProcess(void)
{
    processReceive();
    processTransmit();
}

/**
 * @brief the interrupt handler for the uart interrupt vector
 */
void UART0_intHandler(void)
{
    while (UARTCharsAvail(UART0_BASE))
    {
        uint32_t c = UARTCharGet(UART0_BASE);
        /* XXX process error flags for this character ?? */
        sReceiveBuffer[sReceiveTailIdx] = (uint8_t)c;
        sReceiveTailIdx++;

        if (sReceiveTailIdx >= CC2652_RECV_CIRC_BUFF_SIZE)
        {
            sReceiveTailIdx = 0;
        }
    }
}

#if OPENTHREAD_CONFIG_ENABLE_DEBUG_UART

/*
 *  Documented in platform-cc2652.h
 */
void cc2652DebugUartInit(void)
{
    uart_power_control(UART1_BASE, true);
    /*
     * LaunchPad Pin29 = tx, Pin 30 = rxd
     *
     * The function IOCPinTypeUart() is hard coded to
     * only support UART0 - and does not support UART1.
     *
     * Thus, these pins are configured using a different way.
     */
    IOCPortConfigureSet(IOID_0, IOC_PORT_MCU_UART1_TX, IOC_STD_INPUT);
    IOCPortConfigureSet(IOID_1, IOC_PORT_MCU_UART1_RX, IOC_STD_INPUT);

    UARTConfigSetExpClk(UART1_BASE, SysCtrlClockGet(), 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    UARTEnable(UART1_BASE);
}

/* This holds the last key pressed */
static int debug_uart_ungetbuf;

/*
 * Documented in "src/core/common/debug_uart.h"
 */
int otPlatDebugUart_getc(void)
{
    int ch = -1;

    if (otPlatDebugUart_kbhit())
    {
        /* get & clear 0x100 bit used below as flag */
        ch                  = debug_uart_ungetbuf & 0x0ff;
        debug_uart_ungetbuf = 0;
    }

    return ch;
}

/*
 * Documented in "src/core/common/debug_uart.h"
 */
int otPlatDebugUart_kbhit(void)
{
    int r;

    /* if something is in the unget buf... */
    r = !!debug_uart_ungetbuf;

    if (!r)
    {
        /*
         * Driverlib code returns "-1", or "char" on something
         * but it comes with flags in upper bits
         */
        r = (int)UARTCharGetNonBlocking(UART1_BASE);

        if (r < 0)
        {
            r = 0; /* no key pressed */
        }
        else
        {
            /* key was pressed, mask flags
             * and set 0x100 bit, to distinguish
             * the value "0x00" from "no-key-pressed"
             */
            debug_uart_ungetbuf = ((r & 0x0ff) | 0x0100);

            r = 1; /* key pressed */
        }
    }

    return r;
}

/*
 * Documented in "src/core/common/debug_uart.h"
 */
void otPlatDebugUart_putchar_raw(int b)
{
    UARTCharPut(UART1_BASE, b);
}

#endif /* OPENTHREAD_CONFIG_ENABLE_DEBUG_UART */
