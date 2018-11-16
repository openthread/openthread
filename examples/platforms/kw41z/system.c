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
 *   This file includes the platform-specific initializers.
 *
 */

#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"
#include "fsl_port.h"
#include "platform-kw41z.h"
#include <stdint.h>
#include "openthread/platform/uart.h"

otInstance *sInstance;

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    uint32_t temp, tempTrim;
    uint8_t  revId;

    /* enable clock for PORTs */
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);

    SIM->SCGC6 |= (SIM_SCGC6_DMAMUX_MASK); /* Enable clock to DMA_MUX (SIM module) */
    SIM->SCGC7 |= (SIM_SCGC7_DMA_MASK);

    /* Obtain REV ID from SIM */
    revId = (uint8_t)((SIM->SDID & SIM_SDID_REVID_MASK) >> SIM_SDID_REVID_SHIFT);

    if (revId == 0)
    {
        tempTrim = RSIM->ANA_TRIM;
        RSIM->ANA_TRIM |= RSIM_ANA_TRIM_BB_LDO_XO_TRIM_MASK; /* max trim for BB LDO for XO */
    }

    /* Turn on clocks for the XCVR */
    /* Enable RF OSC in RSIM and wait for ready */
    temp = RSIM->CONTROL;
    temp &= ~RSIM_CONTROL_RF_OSC_EN_MASK;
    RSIM->CONTROL = temp | RSIM_CONTROL_RF_OSC_EN(1);
    /* Prevent XTAL_OUT_EN from generating XTAL_OUT request */
    RSIM->RF_OSC_CTRL |= RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK;

    /* wait for RF_OSC_READY */
    while ((RSIM->CONTROL & RSIM_CONTROL_RF_OSC_READY_MASK) == 0)
    {
    }

    if (revId == 0)
    {
        SIM->SCGC5 |= SIM_SCGC5_PHYDIG_MASK;
        XCVR_TSM->OVRD0 |= XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_MASK |
                           XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_MASK; /* Force ADC DAC LDO on to prevent BGAP failure */
        /* Reset LDO trim settings */
        RSIM->ANA_TRIM = tempTrim;
    } /* Workaround for Rev 1.0 XTAL startup and ADC analog diagnostics circuitry */

    /* Init board clock */
    BOARD_BootClockRUN();

    kw41zAlarmInit();
    kw41zRandomInit();
    kw41zRadioInit();

    otPlatUartEnable();
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

void otSysDeinit(void)
{
}

void otSysProcessDrivers(otInstance *aInstance)
{
    sInstance = aInstance;
    kw41zUartProcess();
    kw41zRadioProcess(aInstance);
    kw41zAlarmProcess(aInstance);
}

void NMI_Handler(void)
{
    /* Change NMI Pin MUX */
    CLOCK_EnableClock(kCLOCK_PortB);
    PORT_SetPinMux(PORTB, 18, kPORT_MuxAlt2);
}
