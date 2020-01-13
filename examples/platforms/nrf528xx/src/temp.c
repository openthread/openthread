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

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <hal/nrf_temp.h>
#include <utils/code_utils.h>

#include "platform-nrf5.h"
#include <drivers/radio/platform/temperature/nrf_802154_temperature.h>

#if SOFTDEVICE_PRESENT
#include "softdevice.h"
#endif

#define US_PER_S 1000000ULL

static uint64_t sLastReadTimestamp;
static int32_t  sTemperature;

#if !SOFTDEVICE_PRESENT
__STATIC_INLINE void dataReadyEventClear(void)
{
    NRF_TEMP->EVENTS_DATARDY = 0;
    volatile uint32_t dummy  = NRF_TEMP->EVENTS_DATARDY;
    OT_UNUSED_VARIABLE(dummy);
}
#endif

void nrf5TempInit(void)
{
#if !SOFTDEVICE_PRESENT
    nrf_temp_init();

    NRF_TEMP->TASKS_START = 1;
#endif
}

void nrf5TempDeinit(void)
{
#if !SOFTDEVICE_PRESENT
    NRF_TEMP->TASKS_STOP = 1;
#endif
}

void nrf5TempProcess(void)
{
    int32_t  prevTemperature = sTemperature;
    uint64_t now;

#if SOFTDEVICE_PRESENT
    now = nrf5AlarmGetCurrentTime();

    if (now - sLastReadTimestamp > (TEMP_MEASUREMENT_INTERVAL * US_PER_S))
    {
        (void)sd_temp_get(&sTemperature);
        sLastReadTimestamp = now;
    }
#else
    if (NRF_TEMP->EVENTS_DATARDY)
    {
        dataReadyEventClear();

        sTemperature = nrf_temp_read();
    }

    now = nrf5AlarmGetCurrentTime();

    if (now - sLastReadTimestamp > (TEMP_MEASUREMENT_INTERVAL * US_PER_S))
    {
        NRF_TEMP->TASKS_START = 1;
        sLastReadTimestamp    = now;
    }
#endif

    if (prevTemperature != sTemperature)
    {
        nrf_802154_temperature_changed();
    }
}

int32_t nrf5TempGet(void)
{
    // Provide temperature value in [0.25 C] unit.
    return sTemperature;
}

void nrf_802154_temperature_init(void)
{
    // Intentionally empty
}

void nrf_802154_temperature_deinit(void)
{
    // Intentionally empty
}

int8_t nrf_802154_temperature_get(void)
{
    // Provide temperature value in [C].
    return (int8_t)(sTemperature / 4);
}
