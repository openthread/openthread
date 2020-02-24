/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements the pseudo-random number generator abstraction layer.
 *
 * This pseudo-random number abstraction layer uses standard library rand() function.
 *
 */

#include "nrf_802154_random.h"

#include <stdlib.h>
#include <stdint.h>

#include "nrf.h"

#if RAAL_SOFTDEVICE
#include <nrf_soc.h>
#endif // RAAL_SOFTDEVICE

void nrf_802154_random_init(void)
{
    uint32_t seed;

#if RAAL_SOFTDEVICE
    uint32_t result;

    do
    {
        result = sd_rand_application_vector_get((uint8_t *)&seed, sizeof(seed));
    }
    while (result != NRF_SUCCESS);
#else // RAAL_SOFTDEVICE
    NRF_RNG->TASKS_START = 1;

    while (!NRF_RNG->EVENTS_VALRDY);
    NRF_RNG->EVENTS_VALRDY = 0;

    seed = NRF_RNG->VALUE;
#endif // RAAL_SOFTDEVICE

    srand((unsigned int)seed);
}

void nrf_802154_random_deinit(void)
{
    // Intentionally empty
}

uint32_t nrf_802154_random_get(void)
{
    return (uint32_t)rand();
}
