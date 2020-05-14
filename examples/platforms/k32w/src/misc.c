/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "openthread/platform/misc.h"
#include "fsl_device_registers.h"
#include "fsl_power.h"
#include "fsl_reset.h"

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    RESET_SystemReset();

    while (1)
    {
    }
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otPlatResetReason reason;
    reset_cause_t     cause = POWER_GetResetCause();

    if (cause & RESET_POR)
    {
        reason = OT_PLAT_RESET_REASON_POWER_ON;
    }
    else if ((cause & RESET_SYS_REQ) || (cause & RESET_SW_REQ))
    {
        reason = OT_PLAT_RESET_REASON_SOFTWARE;
    }
    else if (cause & RESET_WDT)
    {
        reason = OT_PLAT_RESET_REASON_WATCHDOG;
    }
    else if (cause & RESET_EXT_PIN)
    {
        reason = OT_PLAT_RESET_REASON_EXTERNAL;
    }
    else if (cause & RESET_BOR)
    {
        reason = OT_PLAT_RESET_REASON_FAULT;
    }
    else if ((cause & RESET_WAKE_DEEP_PD) || (cause & RESET_WAKE_PD))
    {
        reason = OT_PLAT_RESET_REASON_ASSERT;
    }
    else
    {
        reason = OT_PLAT_RESET_REASON_OTHER;
    }

    return reason;
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
    OT_UNUSED_VARIABLE(aFilename);
    OT_UNUSED_VARIABLE(aLineNumber);
}

void otPlatWakeHost(void)
{
    /* TODO */
}
