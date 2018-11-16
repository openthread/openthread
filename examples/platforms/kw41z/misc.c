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

#include "openthread/platform/misc.h"
#include "fsl_device_registers.h"
#include <stdint.h>

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    NVIC_SystemReset();

    while (1)
    {
    }
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otPlatResetReason reason;

    if (RCM->SRS0 & RCM_SRS0_POR_MASK)
    {
        reason = OT_PLAT_RESET_REASON_POWER_ON;
    }
    else if (RCM->SRS1 & RCM_SRS1_SW_MASK)
    {
        reason = OT_PLAT_RESET_REASON_SOFTWARE;
    }
    else if (RCM->SRS0 & RCM_SRS0_WDOG_MASK)
    {
        reason = OT_PLAT_RESET_REASON_WATCHDOG;
    }
    else if (RCM->SRS0 & RCM_SRS0_PIN_MASK)
    {
        reason = OT_PLAT_RESET_REASON_EXTERNAL;
    }
    else if ((RCM->SRS0 & RCM_SRS0_LOC_MASK) || (RCM->SRS1 & RCM_SRS1_SACKERR_MASK) ||
             (RCM->SRS1 & RCM_SRS1_LOCKUP_MASK))
    {
        reason = OT_PLAT_RESET_REASON_FAULT;
    }
    else if ((RCM->SRS0 & RCM_SRS0_WAKEUP_MASK) || (RCM->SRS0 & RCM_SRS0_LVD_MASK) ||
             (RCM->SRS1 & RCM_SRS1_MDM_AP_MASK))
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
    // TODO: implement an operation to wake the host from sleep state.
}
