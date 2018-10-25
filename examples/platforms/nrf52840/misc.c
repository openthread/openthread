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

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/platform/misc.h>

#include <nrf.h>

#include "platform-nrf5.h"

#if SOFTDEVICE_PRESENT
#include "softdevice.h"
#endif

static uint32_t sResetReason;

bool gPlatformPseudoResetWasRequested;

__WEAK void nrf5CryptoInit(void)
{
    // This function is defined as weak so it could be overridden with external implementation.
}

__WEAK void nrf5CryptoDeinit(void)
{
    // This function is defined as weak so it could be overridden with external implementation.
}

void nrf5MiscInit(void)
{
#if SOFTDEVICE_PRESENT
    sd_power_reset_reason_get(&sResetReason);
    sd_power_reset_reason_clr(0xFFFFFFFF);
#else
    sResetReason         = NRF_POWER->RESETREAS;
    NRF_POWER->RESETREAS = 0xFFFFFFFF;
#endif // SOFTDEVICE_PRESENT
}

void nrf5MiscDeinit(void)
{
    // Intentionally empty.
}

void otPlatReset(otInstance *aInstance)
{
    (void)aInstance;
#if OPENTHREAD_PLATFORM_USE_PSEUDO_RESET
    gPlatformPseudoResetWasRequested = true;
    sResetReason                     = POWER_RESETREAS_SREQ_Msk;
#else  // if OPENTHREAD_PLATFORM_USE_PSEUDO_RESET
    NVIC_SystemReset();
#endif // else OPENTHREAD_PLATFORM_USE_PSEUDO_RESET
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    (void)aInstance;
    otPlatResetReason reason;

    if (sResetReason & POWER_RESETREAS_RESETPIN_Msk)
    {
        reason = OT_PLAT_RESET_REASON_EXTERNAL;
    }
    else if (sResetReason & POWER_RESETREAS_DOG_Msk)
    {
        reason = OT_PLAT_RESET_REASON_WATCHDOG;
    }
    else if (sResetReason & POWER_RESETREAS_SREQ_Msk)
    {
        reason = OT_PLAT_RESET_REASON_SOFTWARE;
    }
    else if (sResetReason & POWER_RESETREAS_LOCKUP_Msk)
    {
        reason = OT_PLAT_RESET_REASON_FAULT;
    }
    else if ((sResetReason & POWER_RESETREAS_OFF_Msk) || (sResetReason & POWER_RESETREAS_LPCOMP_Msk) ||
             (sResetReason & POWER_RESETREAS_DIF_Msk) || (sResetReason & POWER_RESETREAS_NFC_Msk) ||
             (sResetReason & POWER_RESETREAS_VBUS_Msk))
    {
        reason = OT_PLAT_RESET_REASON_OTHER;
    }
    else
    {
        reason = OT_PLAT_RESET_REASON_POWER_ON;
    }

    return reason;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}
