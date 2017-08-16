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

#include <openthread/platform/logging.h>

#include <device/nrf.h>
#include <drivers/clock/nrf_drv_clock.h>
#include "platform-nrf5.h"

#include <openthread/config.h>

void __cxa_pure_virtual(void) { while (1); }

void PlatformInit(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

#if !SOFTDEVICE_PRESENT
    // Enable I-code cache
    NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled;
#endif

    nrf_drv_clock_init();

#if (OPENTHREAD_CONFIG_ENABLE_DEFAULT_LOG_OUTPUT == 0)
    nrf5LogInit();
#endif
    nrf5AlarmInit();
    nrf5RandomInit();
    nrf5UartInit();
    nrf5MiscInit();
    nrf5CryptoInit();
    nrf5RadioInit();
}

void PlatformDeinit(void)
{
    nrf5RadioDeinit();
    nrf5CryptoDeinit();
    nrf5MiscDeinit();
    nrf5UartDeinit();
    nrf5RandomDeinit();
    nrf5AlarmDeinit();
#if (OPENTHREAD_CONFIG_ENABLE_DEFAULT_LOG_OUTPUT == 0)
    nrf5LogDeinit();
#endif
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    nrf5AlarmProcess(aInstance);
    nrf5RadioProcess(aInstance);
    nrf5UartProcess();
}
