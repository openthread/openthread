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
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include <string.h>

#include <openthread/platform/uart.h>

#include "common/logging.hpp"

#include "pa.h"
#include "pti.h"
#include "bsp.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_chip.h"
#include "platform-efr32.h"
#include "rail.h"
#include "rail_ieee802154.h"
#include "openthread-core-efr32-config.h"

otInstance *sInstance;

void HAL_Init(void);

void PlatformInit(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    CHIP_Init();
    HAL_Init();
    BSP_Init(BSP_INIT_BCC);

    otPlatUartEnable();

    RAIL_Init_t railInitParams =
    {
        128, // maxPacketLength: UNUSED
        RADIO_CONFIG_XTAL_FREQUENCY,
        0
    };
    RAIL_RfInit(&railInitParams);
    RAIL_RfIdle();

    efr32AlarmInit();
    efr32RadioInit();
    efr32MiscInit();
    efr32RandomInit();
}

void PlatformDeinit(void)
{
    efr32RadioDeinit();
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    sInstance = aInstance;

    // should sleep and wait for interrupts here

    efr32UartProcess();
    efr32RadioProcess(aInstance);
    efr32AlarmProcess(aInstance);
}

void halInitChipSpecific(void)
{
    CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_WSTK_DEFAULT;
    RADIO_PTIInit_t ptiInit = RADIO_PTI_INIT;
    RADIO_PAInit_t paInit;

    SYSTEM_ChipRevision_TypeDef chipRev;
    SYSTEM_ChipRevisionGet(&chipRev);

    // Init DCDC regulator and HFXO with WSTK radio board specific parameters
    // from s025_sw\kits\SLWSTK6100A_EFR32MG\config\bspconfig.h
#ifdef EMU_DCDCINIT_WSTK_DEFAULT
    EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_WSTK_DEFAULT;
    EMU_DCDCInit(&dcdcInit);
#else
    EMU_DCDCPowerOff();
#endif
    CMU_HFXOInit(&hfxoInit);
    SystemHFXOClockSet(RADIO_CONFIG_XTAL_FREQUENCY);

    // Initialize the Packet Trace Interface (PTI) to match the configuration in
    // the board header
    RADIO_PTI_Init(&ptiInit);

    /* Switch HFCLK to HFXO and disable HFRCO */
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

    // Initialize the PA now that the HFXO is up and the timing is correct
#if (RADIO_CONFIG_BASE_FREQUENCY < 1000000000UL)
    paInit = (RADIO_PAInit_t) RADIO_PA_SUBGIG_INIT;
#else
    paInit = (RADIO_PAInit_t) RADIO_PA_2P4_INIT;
#endif

    if (!RADIO_PA_Init(&paInit))
    {
        // Error: The PA could not be initialized due to an improper configuration.
        // Please ensure your configuration is valid for the selected part.
        while (1);
    }

    // Initialize other chip clocks
    CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
    CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);
    CMU_ClockEnable(cmuClock_CORELE, true);
}

void HAL_Init(void)
{
    halInitChipSpecific();
}
