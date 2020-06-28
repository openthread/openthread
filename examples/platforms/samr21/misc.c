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
 *   This file implements the OpenThread platform abstraction for miscellaneous behaviors.
 */

#include "asf.h"

#include <openthread/platform/misc.h>

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    system_reset();

    while (1)
    {
    }
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    otPlatResetReason reason;

    switch (system_get_reset_cause())
    {
    /** The system was last reset by a software reset */
    case SYSTEM_RESET_CAUSE_SOFTWARE:
        reason = OT_PLAT_RESET_REASON_SOFTWARE;
        break;

    /** The system was last reset by the watchdog timer */
    case SYSTEM_RESET_CAUSE_WDT:
        reason = OT_PLAT_RESET_REASON_WATCHDOG;
        break;

    /** The system was last reset because the external reset
        line was pulled low */
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET:
        reason = OT_PLAT_RESET_REASON_EXTERNAL;
        break;

    /** The system was last reset by the BOD33 */
    case SYSTEM_RESET_CAUSE_BOD33:

    // no break: same reason as below

    /** The system was last reset by the BOD12 */
    case SYSTEM_RESET_CAUSE_BOD12:
        reason = OT_PLAT_RESET_REASON_FAULT;
        break;

    /** The system was last reset by the POR (Power on reset) */
    case SYSTEM_RESET_CAUSE_POR:
        reason = OT_PLAT_RESET_REASON_POWER_ON;
        break;

    default:
        reason = OT_PLAT_RESET_REASON_UNKNOWN;
        break;
    }

    return reason;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}
