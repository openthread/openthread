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

#include <utils/code_utils.h>
#include <hal/nrf_temp.h>

#include "platform-nrf5.h"

#if SOFTDEVICE_PRESENT
#include "softdevice.h"
#endif

#if !SOFTDEVICE_PRESENT
__STATIC_INLINE void dataReadyEventClear(void)
{
    NRF_TEMP->EVENTS_DATARDY = 0;
    volatile uint32_t dummy = NRF_TEMP->EVENTS_DATARDY;
    (void)dummy;
}
#endif

void nrf5TempInit(void)
{
#if !SOFTDEVICE_PRESENT
    nrf_temp_init();
#endif
}

void nrf5TempDeinit(void)
{
#if !SOFTDEVICE_PRESENT
    NRF_TEMP->TASKS_STOP = 1;
#endif
}

int32_t nrf5TempGet(void)
{
#if SOFTDEVICE_PRESENT
    int32_t temperature;
    (void) sd_temp_get(&temperature);
#else
    NRF_TEMP->TASKS_START = 1;

    while (NRF_TEMP->EVENTS_DATARDY == 0)
    {
        ;
    }

    dataReadyEventClear();

    int32_t temperature = nrf_temp_read();

    NRF_TEMP->TASKS_STOP = 1;
#endif

    return temperature;
}
