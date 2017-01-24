/* Copyright (c) 2016, Nordic Semiconductor ASA
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

#include "nrf_drv_clock.h"

#define NRF_ERROR_MODULE_ALREADY_INITIALIZED 0x8085

/*lint -save -e652 */
#define NRF_CLOCK_LFCLK_RC    CLOCK_LFCLKSRC_SRC_RC
#define NRF_CLOCK_LFCLK_Xtal  CLOCK_LFCLKSRC_SRC_Xtal
#define NRF_CLOCK_LFCLK_Synth CLOCK_LFCLKSRC_SRC_Synth
/*lint -restore */

volatile uint32_t m_hfclk_requests;  /*< High frequency clock requests. */
volatile uint32_t m_lfclk_requests;  /*< Low frequency clock requests. */

/**@brief Function for starting LFCLK. This function will return immediately without waiting for start.
 */
static void lfclk_start(void)
{
    nrf_clock_event_clear(NRF_CLOCK_EVENT_LFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTART);
}

/**@brief Function for stopping LFCLK and calibration (if it was set up).
 */
static void lfclk_stop(void)
{
    nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTOP);

    while (nrf_clock_lf_is_running()) {}
}

static void hfclk_start(void)
{
    nrf_clock_event_clear(NRF_CLOCK_EVENT_HFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTART);
}

static void hfclk_stop(void)
{
    nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTOP);

    while (nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY)) {}
}

uint32_t nrf_drv_clock_init(void)
{
    nrf_clock_lf_src_set(NRF_CLOCK_LFCLK_Xtal);
    return 0;
}

void nrf_drv_clock_uninit(void)
{
    lfclk_stop();
    hfclk_stop();
}

void nrf_drv_clock_lfclk_request(nrf_drv_clock_handler_item_t *p_handler_item)
{
    volatile uint32_t lfclk_requests;
    (void) p_handler_item;

    do
    {

        lfclk_requests = __LDREXW(&m_lfclk_requests);
        lfclk_requests += 1;
    }
    while (__STREXW(lfclk_requests, &m_lfclk_requests));

    if (!nrf_clock_lf_is_running())
    {
        lfclk_start();
    }
}

void nrf_drv_clock_lfclk_release(void)
{
    volatile uint32_t lfclk_requests;

    do
    {
        lfclk_requests = __LDREXW(&m_lfclk_requests);
        lfclk_requests -= 1;
    }
    while (__STREXW(lfclk_requests, &m_lfclk_requests));

    if (nrf_clock_lf_is_running() && m_lfclk_requests == 0)
    {
        lfclk_stop();
    }
}

bool nrf_drv_clock_lfclk_is_running(void)
{
    return nrf_clock_lf_is_running();
}

void nrf_drv_clock_hfclk_request(nrf_drv_clock_handler_item_t *p_handler_item)
{
    volatile uint32_t hfclk_requests;
    (void) p_handler_item;

    do
    {

        hfclk_requests = __LDREXW(&m_hfclk_requests);
        hfclk_requests += 1;
    }
    while (__STREXW(hfclk_requests, &m_hfclk_requests));

    if (!nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY))
    {
        hfclk_start();
    }
}

void nrf_drv_clock_hfclk_release(void)
{
    volatile uint32_t hfclk_requests;

    do
    {
        hfclk_requests = __LDREXW(&m_hfclk_requests);
        hfclk_requests -= 1;
    }
    while (__STREXW(hfclk_requests, &m_hfclk_requests));

    if (nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY) && m_hfclk_requests == 0)
    {
        hfclk_stop();
    }
}

bool nrf_drv_clock_hfclk_is_running(void)
{
    return nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY);
}
