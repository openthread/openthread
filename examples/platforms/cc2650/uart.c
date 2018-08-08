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

#include <stddef.h>

#include <driverlib/ioc.h>
#include <driverlib/prcm.h>
#include <driverlib/sys_ctrl.h>
#include <driverlib/uart.h>

#include <utils/code_utils.h>
#include <openthread/platform/uart.h>

/**
 * \note this will configure the uart for 115200 baud 8-N-1, no HW flow control
 * RX pin IOID_2 TX pin IOID_3
 *
 * \note make sure that data being sent is not in a volatile area
 */

enum
{
    CC2650_RECV_CIRC_BUFF_SIZE = 256,
};

static uint8_t const *sSendBuffer = NULL;
static uint16_t       sSendLen    = 0;

static uint8_t  sReceiveBuffer[CC2650_RECV_CIRC_BUFF_SIZE];
static uint16_t sReceiveHeadIdx = 0;
static uint16_t sReceiveTailIdx = 0;

void UART0_intHandler(void);

/**
 * Function documented in platform/uart.h
 */
otError otPlatUartEnable(void)
{
    PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);

    while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) != PRCM_DOMAIN_POWER_ON)
        ;

    PRCMPeripheralRunEnable(PRCM_PERIPH_UART0);
    PRCMPeripheralSleepEnable(PRCM_PERIPH_UART0);
    PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_UART0);
    PRCMLoadSet();

    while (!PRCMLoadGet())
        ;

    IOCPinTypeUart(UART0_BASE, IOID_2, IOID_3, IOID_UNUSED, IOID_UNUSED);

    UARTConfigSetExpClk(UART0_BASE, SysCtrlClockGet(), 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
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

    PRCMPeripheralRunDisable(PRCM_PERIPH_UART0);
    PRCMPeripheralSleepDisable(PRCM_PERIPH_UART0);
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_UART0);
    PRCMLoadSet();
    /**
     * \warn this assumes that there are no other devices being used in the
     * serial power domain
     */
    PRCMPowerDomainOff(PRCM_DOMAIN_SERIAL);
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
            tailIdx = CC2650_RECV_CIRC_BUFF_SIZE;
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
 * Function documented in platform-cc2650.h
 */
void cc2650UartProcess(void)
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

        if (sReceiveTailIdx >= CC2650_RECV_CIRC_BUFF_SIZE)
        {
            sReceiveTailIdx = 0;
        }
    }
}
