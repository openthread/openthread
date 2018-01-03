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

#include "bsp.h"
#include "em_chip.h"
#include "hal_common.h"

#include "platform-efr32.h"
#include "openthread-core-efr32-config.h"

void halInitChipSpecific(void);

otInstance *sInstance;

void PlatformInit(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    CHIP_Init();

    halInitChipSpecific();

    BSP_Init(BSP_INIT_BCC);

    efr32RadioInit();
    efr32AlarmInit();
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
