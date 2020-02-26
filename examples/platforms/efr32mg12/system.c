/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include <assert.h>
#include <string.h>

#include "openthread-system.h"
#include <openthread/platform/uart.h>

#include "common/logging.hpp"

#include "bsp.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_core.h"
#include "em_emu.h"
#include "em_system.h"
#include "hal-config.h"
#include "hal_common.h"
#include "rail.h"
#include "sl_mpu.h"
#include "sl_sleeptimer.h"

#include "openthread-core-efr32-config.h"
#include "platform-efr32.h"

#if (HAL_FEM_ENABLE)
#include "fem-control.h"
#endif

#define USE_EFR32_LOG                                                                   \
    ((OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
     (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL))

void halInitChipSpecific(void);

otInstance *sInstance;
static bool (*sCanSleepCallback)(void);

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    sl_status_t status;

    __disable_irq();

#undef FIXED_EXCEPTION
#define FIXED_EXCEPTION(vectorNumber, functionName, deviceIrqn, deviceIrqHandler)
#define EXCEPTION(vectorNumber, functionName, deviceIrqn, deviceIrqHandler, priorityLevel, subpriority) \
    NVIC_SetPriority(deviceIrqn, NVIC_EncodePriority(PRIGROUP_POSITION, priorityLevel, subpriority));
#include NVIC_CONFIG
#undef EXCEPTION

    NVIC_SetPriorityGrouping(PRIGROUP_POSITION);
    CHIP_Init();
    halInitChipSpecific();
    BSP_Init(BSP_INIT_BCC);

    CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFRCO);
    CMU_ClockEnable(cmuClock_CORELE, true);
    CMU_ClockEnable(cmuClock_RTCC, true);
    status = sl_sleeptimer_init();
    assert(status == SL_STATUS_OK);

#if (HAL_FEM_ENABLE)
    initFem();
    wakeupFem();
#endif

    __enable_irq();

#if USE_EFR32_LOG
    efr32LogInit();
#endif
    efr32RadioInit();
    efr32AlarmInit();
    efr32MiscInit();
    efr32RandomInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

void otSysDeinit(void)
{
    efr32RadioDeinit();

#if USE_EFR32_LOG
    efr32LogDeinit();
#endif
}

void efr32SetSleepCallback(bool (*aCallback)(void))
{
    sCanSleepCallback = aCallback;
}

void efr32Sleep(void)
{
    bool canDeepSleep      = false;
    int  wakeupProcessTime = 1000;
    CORE_DECLARE_IRQ_STATE;

    if (RAIL_Sleep(wakeupProcessTime, &canDeepSleep) == RAIL_STATUS_NO_ERROR)
    {
        if (canDeepSleep)
        {
            CORE_ENTER_ATOMIC();
            if (sCanSleepCallback != NULL && sCanSleepCallback())
            {
                EMU_EnterEM2(true);
            }
            CORE_EXIT_ATOMIC();
            // TODO OT will handle an interrupt here and it mustn't call any RAIL APIs

            while (RAIL_Wake(0) != RAIL_STATUS_NO_ERROR)
            {
            }
        }
        else
        {
            CORE_ENTER_ATOMIC();
            if (sCanSleepCallback != NULL && sCanSleepCallback())
            {
                EMU_EnterEM1();
            }
            CORE_EXIT_ATOMIC();
        }
    }
}

void otSysProcessDrivers(otInstance *aInstance)
{
    sInstance = aInstance;

    // should sleep and wait for interrupts here

    efr32UartProcess();
    efr32RadioProcess(aInstance);
    efr32AlarmProcess(aInstance);
}

__WEAK void otSysEventSignalPending(void)
{
    // Intentionally empty
}
