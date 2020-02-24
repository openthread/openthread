/* Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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
 *   This file implements common function for Front End Module control of the nRF 802.15.4 radio driver.
 *
 */
#include "nrf_fem_protocol_api.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "compiler_abstraction.h"
#include "nrf_802154_config.h"
#include "nrf.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_ppi.h"
#include "nrf_radio.h"
#include "nrf_timer.h"

#if ENABLE_FEM

#define PPI_INVALID_CHANNEL 0xFF                               /**< Default value for the PPI holder variable. */

static nrf_fem_interface_config_t m_nrf_fem_interface_config = /**< FEM controller configuration. */
{
    .fem_config =
    {
        .pa_time_gap_us  = NRF_FEM_PA_TIME_IN_ADVANCE_US,
        .lna_time_gap_us = NRF_FEM_LNA_TIME_IN_ADVANCE_US,
        .pdn_settle_us   = NRF_FEM_PDN_SETTLE_US,
        .trx_hold_us     = NRF_FEM_TRX_HOLD_US,
    },
    .pa_pin_config =
    {
        .enable       = 1,
        .active_high  = 1,
        .gpio_pin     = NRF_FEM_CONTROL_DEFAULT_PA_PIN,
        .gpiote_ch_id = NRF_FEM_CONTROL_DEFAULT_PA_GPIOTE_CHANNEL
    },
    .lna_pin_config =
    {
        .enable       = 1,
        .active_high  = 1,
        .gpio_pin     = NRF_FEM_CONTROL_DEFAULT_LNA_PIN,
        .gpiote_ch_id = NRF_FEM_CONTROL_DEFAULT_LNA_GPIOTE_CHANNEL
    },
    .pdn_pin_config =
    {
        .enable       = 1,
        .gpio_pin     = NRF_FEM_CONTROL_DEFAULT_PDN_PIN,
        .active_high  = 1,
        .gpiote_ch_id = NRF_FEM_CONTROL_DEFAULT_PDN_GPIOTE_CHANNEL
    },
    .ppi_ch_id_set = NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL,
    .ppi_ch_id_clr = NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL,
    .ppi_ch_id_pdn = NRF_FEM_CONTROL_DEFAULT_PDN_PPI_CHANNEL
};
static uint8_t m_ppi_channel_ext = PPI_INVALID_CHANNEL; /**< PPI channel provided by the `override_ppi = true` functionality. */

/** Map the mask bits with the Compare Channels. */
static uint32_t get_available_compare_channel(uint8_t mask, uint32_t number)
{
    uint32_t i;

    for (i = 0; i < 4; i++)
    {
        if (mask & (1 << i))
        {
            if (number == 0)
            {
                break;
            }
            else
            {
                number--;
            }
        }
    }

    if (i == 0)
        return NRF_TIMER_CC_CHANNEL0;
    if (i == 1)
        return NRF_TIMER_CC_CHANNEL1;
    if (i == 2)
        return NRF_TIMER_CC_CHANNEL2;
    if (i == 3)
        return NRF_TIMER_CC_CHANNEL3;
    assert(false);
    return 0;
}

/** Configure GPIOTE module. */
static void gpiote_configure(void)
{
    if (m_nrf_fem_interface_config.pa_pin_config.enable)
    {
        nrf_gpiote_task_configure(m_nrf_fem_interface_config.pa_pin_config.gpiote_ch_id,
                                  m_nrf_fem_interface_config.pa_pin_config.gpio_pin,
                                  (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
                                  (nrf_gpiote_outinit_t) !m_nrf_fem_interface_config.pa_pin_config.active_high);

        nrf_gpiote_task_enable(m_nrf_fem_interface_config.pa_pin_config.gpiote_ch_id);
    }

    if (m_nrf_fem_interface_config.lna_pin_config.enable)
    {
        nrf_gpiote_task_configure(m_nrf_fem_interface_config.lna_pin_config.gpiote_ch_id,
                                  m_nrf_fem_interface_config.lna_pin_config.gpio_pin,
                                  (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
                                  (nrf_gpiote_outinit_t) !m_nrf_fem_interface_config.lna_pin_config.active_high);

        nrf_gpiote_task_enable(m_nrf_fem_interface_config.lna_pin_config.gpiote_ch_id);
    }

    if (m_nrf_fem_interface_config.pdn_pin_config.enable)
    {
        nrf_gpiote_task_configure(m_nrf_fem_interface_config.pdn_pin_config.gpiote_ch_id,
                                  m_nrf_fem_interface_config.pdn_pin_config.gpio_pin,
                                  (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
                                  (nrf_gpiote_outinit_t) !m_nrf_fem_interface_config.pdn_pin_config.active_high);

        nrf_gpiote_task_enable(m_nrf_fem_interface_config.pdn_pin_config.gpiote_ch_id);
    }
}

/** Configure the event with the provided values. */
static int32_t event_configuration_set(const nrf_802154_fal_event_t * const p_event,
                                       nrf_fem_gpiote_pin_config_t        * p_pin_config,
                                       bool                                 activate,
                                       uint32_t                             time_delay)
{
    uint32_t task_addr;
    uint8_t  ppi_ch;

    assert(p_event);
    assert(p_pin_config);

    if (p_event->override_ppi)
    {
        assert(p_event->ppi_ch_id != PPI_INVALID_CHANNEL);
        if (m_ppi_channel_ext == PPI_INVALID_CHANNEL)
        {
            /* External PPI channel placeholder is free. */
            m_ppi_channel_ext = ppi_ch = p_event->ppi_ch_id;
        }
        else if ((m_ppi_channel_ext == p_event->ppi_ch_id) &&
                 (!NRF_PPI->FORK[(uint32_t)m_ppi_channel_ext].TEP))
        {
            /* PPI is equal to the already set, but the one set has a free fork endpoint. */
            ppi_ch = p_event->ppi_ch_id;
        }
        else
        {
            return NRF_ERROR_INVALID_STATE;
        }
    }
    else
    {
        ppi_ch =
            activate ? m_nrf_fem_interface_config.ppi_ch_id_set : m_nrf_fem_interface_config.
            ppi_ch_id_clr;
    }

    if (p_pin_config->active_high ^ activate)
    {
        task_addr = (uint32_t)(&NRF_GPIOTE->TASKS_CLR[p_pin_config->gpiote_ch_id]);
    }
    else
    {
        task_addr = (uint32_t)(&NRF_GPIOTE->TASKS_SET[p_pin_config->gpiote_ch_id]);
    }

    switch (p_event->type)
    {
        case NRF_802154_FAL_EVENT_TYPE_GENERIC:
        {
            if (NRF_PPI->CH[(uint32_t)ppi_ch].TEP)
            {
                nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)ppi_ch, task_addr);
            }
            else
            {
                nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)ppi_ch,
                                               p_event->event.generic.register_address,
                                               task_addr);
            }

            nrf_ppi_channel_enable((nrf_ppi_channel_t)ppi_ch);
        }
        break;

        case NRF_802154_FAL_EVENT_TYPE_TIMER:
        {
            assert(p_event->event.timer.compare_channel_mask);

            uint32_t compare_channel;
            uint32_t pdn_task_addr;

            /* EN pin */
            compare_channel = get_available_compare_channel(
                p_event->event.timer.compare_channel_mask,
                0);

            nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)ppi_ch,
                                           (uint32_t)(&(p_event->event.timer.p_timer_instance->
                                                        EVENTS_COMPARE[compare_channel])),
                                           task_addr);
            nrf_ppi_channel_enable((nrf_ppi_channel_t)ppi_ch);

            nrf_timer_cc_write(p_event->event.timer.p_timer_instance,
                               (nrf_timer_cc_channel_t)compare_channel,
                               p_event->event.timer.counter_value - time_delay);

            /* PDN pin */
            if (m_nrf_fem_interface_config.pdn_pin_config.active_high)
            {
                pdn_task_addr =
                    (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_interface_config.pdn_pin_config.
                                                      gpiote_ch_id]);
            }
            else
            {
                pdn_task_addr =
                    (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_interface_config.pdn_pin_config.
                                                      gpiote_ch_id]);
            }

            compare_channel = get_available_compare_channel(
                p_event->event.timer.compare_channel_mask,
                1);

            nrf_ppi_channel_endpoint_setup(
                (nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_pdn,
                (uint32_t)(&(p_event->event.timer.p_timer_instance->
                             EVENTS_COMPARE[compare_channel])),
                pdn_task_addr);

            nrf_ppi_channel_enable((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_pdn);

            nrf_timer_cc_write(p_event->event.timer.p_timer_instance,
                               (nrf_timer_cc_channel_t)compare_channel,
                               p_event->event.timer.counter_value - time_delay -
                               m_nrf_fem_interface_config.fem_config.pdn_settle_us);
            break;
        }

        default:
            assert(false);
            break;
    }

    return NRF_SUCCESS;
}

/** Deconfigure the event with the provided values. */
static int32_t event_configuration_clear(const nrf_802154_fal_event_t * const p_event,
                                         bool                                 activate)
{
    uint8_t ppi_ch;

    assert(p_event);

    if (p_event->override_ppi)
    {
        ppi_ch = p_event->ppi_ch_id;
    }
    else
    {
        ppi_ch =
            activate ? m_nrf_fem_interface_config.ppi_ch_id_set : m_nrf_fem_interface_config.
            ppi_ch_id_clr;
    }

    nrf_ppi_channel_disable((nrf_ppi_channel_t)ppi_ch);
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)ppi_ch, 0, 0);
    nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)ppi_ch, 0);

    switch (p_event->type)
    {
        case NRF_802154_FAL_EVENT_TYPE_GENERIC:
        case NRF_802154_FAL_EVENT_TYPE_TIMER:
            break;

        default:
            assert(false);
            break;
    }

    return NRF_SUCCESS;
}

int32_t nrf_802154_fal_pa_configuration_set(const nrf_802154_fal_event_t * const p_activate_event,
                                            const nrf_802154_fal_event_t * const p_deactivate_event)
{
    int32_t ret_code;

    if (!m_nrf_fem_interface_config.pa_pin_config.enable)
    {
        return NRF_ERROR_FORBIDDEN;
    }

    if (p_activate_event)
    {
        ret_code = event_configuration_set(p_activate_event,
                                           &m_nrf_fem_interface_config.pa_pin_config,
                                           true,
                                           m_nrf_fem_interface_config.fem_config.pa_time_gap_us);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    if (p_deactivate_event)
    {
        ret_code = event_configuration_set(p_deactivate_event,
                                           &m_nrf_fem_interface_config.pa_pin_config,
                                           false,
                                           m_nrf_fem_interface_config.fem_config.pa_time_gap_us);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    return NRF_SUCCESS;
}

int32_t nrf_802154_fal_lna_configuration_set(const nrf_802154_fal_event_t * const p_activate_event,
                                             const nrf_802154_fal_event_t * const p_deactivate_event)
{
    int32_t ret_code;

    if (!m_nrf_fem_interface_config.lna_pin_config.enable)
    {
        return NRF_ERROR_FORBIDDEN;
    }

    if (p_activate_event)
    {
        ret_code = event_configuration_set(p_activate_event,
                                           &m_nrf_fem_interface_config.lna_pin_config,
                                           true,
                                           m_nrf_fem_interface_config.fem_config.lna_time_gap_us);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    if (p_deactivate_event)
    {
        ret_code = event_configuration_set(p_deactivate_event,
                                           &m_nrf_fem_interface_config.lna_pin_config,
                                           false,
                                           m_nrf_fem_interface_config.fem_config.lna_time_gap_us);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    return NRF_SUCCESS;
}

int32_t nrf_802154_fal_pa_configuration_clear(const nrf_802154_fal_event_t * const p_activate_event,
                                              const nrf_802154_fal_event_t * const p_deactivate_event)
{
    int32_t ret_code;

    if (!m_nrf_fem_interface_config.pa_pin_config.enable)
    {
        return NRF_ERROR_FORBIDDEN;
    }

    if (p_activate_event)
    {
        ret_code = event_configuration_clear(p_activate_event, true);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    if (p_deactivate_event)
    {
        ret_code = event_configuration_clear(p_deactivate_event, false);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    return NRF_SUCCESS;
}

int32_t nrf_802154_fal_lna_configuration_clear(
    const nrf_802154_fal_event_t * const p_activate_event,
    const nrf_802154_fal_event_t * const p_deactivate_event)
{
    int32_t ret_code;

    if (!m_nrf_fem_interface_config.lna_pin_config.enable)
    {
        return NRF_ERROR_FORBIDDEN;
    }

    if (p_activate_event)
    {
        ret_code = event_configuration_clear(p_activate_event, true);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    if (p_deactivate_event)
    {
        ret_code = event_configuration_clear(p_deactivate_event, false);
        if (ret_code != NRF_SUCCESS)
        {
            return ret_code;
        }
    }

    return NRF_SUCCESS;
}

void nrf_802154_fal_deactivate_now(nrf_fal_functionality_t type)
{
    if (m_nrf_fem_interface_config.pa_pin_config.enable && (type & NRF_802154_FAL_PA))
    {
        if (m_nrf_fem_interface_config.pa_pin_config.active_high)
        {
            nrf_gpiote_task_force(m_nrf_fem_interface_config.pa_pin_config.gpiote_ch_id,
                                  NRF_GPIOTE_INITIAL_VALUE_LOW);
        }
        else
        {
            nrf_gpiote_task_force(m_nrf_fem_interface_config.pa_pin_config.gpiote_ch_id,
                                  NRF_GPIOTE_INITIAL_VALUE_HIGH);
        }
    }

    if (m_nrf_fem_interface_config.lna_pin_config.enable && (type & NRF_802154_FAL_LNA))
    {
        if (m_nrf_fem_interface_config.lna_pin_config.active_high)
        {
            nrf_gpiote_task_force(m_nrf_fem_interface_config.lna_pin_config.gpiote_ch_id,
                                  NRF_GPIOTE_INITIAL_VALUE_LOW);
        }
        else
        {
            nrf_gpiote_task_force(m_nrf_fem_interface_config.lna_pin_config.gpiote_ch_id,
                                  NRF_GPIOTE_INITIAL_VALUE_HIGH);
        }
    }
}

int32_t nrf_fem_interface_configuration_set(nrf_fem_interface_config_t const * const p_config)
{
    m_nrf_fem_interface_config = *p_config;

    if (m_nrf_fem_interface_config.pa_pin_config.enable ||
        m_nrf_fem_interface_config.lna_pin_config.enable)
    {
        gpiote_configure();
    }

    return NRF_SUCCESS;
}

int32_t nrf_fem_interface_configuration_get(nrf_fem_interface_config_t * p_config)
{
    *p_config = m_nrf_fem_interface_config;

    return NRF_SUCCESS;
}

void nrf_802154_fal_cleanup(void)
{
    nrf_ppi_channel_disable((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_set);
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_set, 0,
                                   0);
    nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_set, 0);
    nrf_ppi_channel_disable((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_clr);
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_clr, 0,
                                   0);
    nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_clr, 0);
    if (m_ppi_channel_ext != PPI_INVALID_CHANNEL)
    {
        nrf_ppi_channel_disable((nrf_ppi_channel_t)m_ppi_channel_ext);
        nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)m_ppi_channel_ext, 0, 0);
        nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)m_ppi_channel_ext, 0);
        m_ppi_channel_ext = PPI_INVALID_CHANNEL;
    }
}

bool nrf_fem_prepare_powerdown(NRF_TIMER_Type  * p_instance,
                               uint32_t          compare_channel,
                               nrf_ppi_channel_t ppi_id)
{
    uint32_t pdn_task_addr;

    if (!m_nrf_fem_interface_config.pdn_pin_config.enable)
    {
        return false;
    }

    if (m_nrf_fem_interface_config.pdn_pin_config.active_high)
    {
        pdn_task_addr =
            (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_interface_config.pdn_pin_config.gpiote_ch_id
                       ]);
    }
    else
    {
        pdn_task_addr =
            (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_interface_config.pdn_pin_config.gpiote_ch_id
                       ]);
    }

    nrf_timer_cc_write(p_instance,
                       (nrf_timer_cc_channel_t)compare_channel,
                       m_nrf_fem_interface_config.fem_config.trx_hold_us + 1);
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)m_nrf_fem_interface_config.ppi_ch_id_pdn,
                                   (uint32_t)(&(p_instance->EVENTS_COMPARE[compare_channel])),
                                   pdn_task_addr);

    uint32_t event_addr = (uint32_t)nrf_radio_event_address_get(NRF_RADIO_EVENT_DISABLED);
    uint32_t task_addr  = (uint32_t)nrf_timer_task_address_get(p_instance, NRF_TIMER_TASK_START);

    nrf_timer_shorts_enable(p_instance, NRF_TIMER_SHORT_COMPARE0_STOP_MASK);
    nrf_ppi_channel_endpoint_setup(ppi_id, event_addr, task_addr);
    nrf_ppi_fork_endpoint_setup(ppi_id, 0);
    nrf_ppi_channel_enable(ppi_id);

    nrf_timer_event_clear(p_instance, NRF_TIMER_EVENT_COMPARE0);

    return true;
}

#endif // ENABLE_FEM
