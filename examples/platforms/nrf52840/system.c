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

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/platform/logging.h>

#include "openthread-system.h"
#include "platform-fem.h"
#include "platform-nrf5.h"
#include <drivers/clock/nrf_drv_clock.h>
#include <nrf.h>

#include <openthread/config.h>

extern bool gPlatformPseudoResetWasRequested;

void __cxa_pure_virtual(void)
{
    while (1)
        ;
}

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    if (gPlatformPseudoResetWasRequested)
    {
        otSysDeinit();
    }

#if !SOFTDEVICE_PRESENT
    // Enable I-code cache
    NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled;
#endif

    nrf_drv_clock_init();

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    nrf5LogInit();
#endif
    nrf5AlarmInit();
    nrf5RandomInit();
    if (!gPlatformPseudoResetWasRequested)
    {
        nrf5UartInit();
        nrf5CryptoInit();
    }
    else
    {
        nrf5UartClearPendingData();
    }

#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveInit();
#endif
    nrf5MiscInit();
    nrf5RadioInit();
    nrf5TempInit();

#if PLATFORM_FEM_ENABLE_DEFAULT_CONFIG
    PlatformFemSetConfigParams(&PLATFORM_FEM_DEFAULT_CONFIG);
#endif

    gPlatformPseudoResetWasRequested = false;
}

void otSysDeinit(void)
{
    nrf5TempDeinit();
    nrf5RadioDeinit();
    nrf5MiscDeinit();
#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveDeinit();
#endif
    if (!gPlatformPseudoResetWasRequested)
    {
        nrf5CryptoDeinit();
        nrf5UartDeinit();
    }
    nrf5RandomDeinit();
    nrf5AlarmDeinit();
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    nrf5LogDeinit();
#endif
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysProcessDrivers(otInstance *aInstance)
{
    nrf5RadioProcess(aInstance);
    nrf5UartProcess();
    nrf5TempProcess();
#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveProcess();
#endif
    nrf5AlarmProcess(aInstance);
}

__WEAK void otSysEventSignalPending(void)
{
    // Intentionally empty
}
