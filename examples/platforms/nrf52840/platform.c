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

#include "platform-nrf5.h"
#include <drivers/clock/nrf_drv_clock.h>
#include <nrf.h>

#include <openthread/config.h>

void __cxa_pure_virtual(void)
{
    while (1)
        ;
}

void PlatformInit(int argc, char *argv[])
{
    extern bool gPlatformPseudoResetWasRequested;

    if (gPlatformPseudoResetWasRequested)
    {
        nrf5RadioPseudoReset();
        nrf5AlarmDeinit();
        nrf5AlarmInit();

        gPlatformPseudoResetWasRequested = false;

        return;
    }

    (void)argc;
    (void)argv;

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
    nrf5UartInit();
#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveInit();
#endif
    nrf5MiscInit();
    nrf5CryptoInit();
    nrf5RadioInit();
    nrf5TempInit();
}

void PlatformDeinit(void)
{
    nrf5TempDeinit();
    nrf5RadioDeinit();
    nrf5CryptoDeinit();
    nrf5MiscDeinit();
#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveDeinit();
#endif
    nrf5UartDeinit();
    nrf5RandomDeinit();
    nrf5AlarmDeinit();
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    nrf5LogDeinit();
#endif
}

bool PlatformPseudoResetWasRequested(void)
{
    extern bool gPlatformPseudoResetWasRequested;
    return gPlatformPseudoResetWasRequested;
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    nrf5AlarmProcess(aInstance);
    nrf5RadioProcess(aInstance);
    nrf5UartProcess();
    nrf5TempProcess();
#ifndef SPIS_TRANSPORT_DISABLE
    nrf5SpiSlaveProcess();
#endif
}

__WEAK void PlatformEventSignalPending(void)
{
    // Intentionally empty
}
