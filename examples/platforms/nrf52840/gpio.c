/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for GPIO and GPIOTE.
 *
 */

#include <openthread/config.h>

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include <common/logging.hpp>
#include <openthread/types.h>
#include <openthread/platform/toolchain.h>
#include <openthread/platform/gpio.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/logging.h>
#include <utils/code_utils.h>

#include <drivers/clock/nrf_drv_clock.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include "platform-nrf5.h"

#define TE_IDX_TO_EVENT_ADDR(idx)    (nrf_gpiote_events_t)((uint32_t)NRF_GPIOTE_EVENTS_IN_0 + \
                                     (sizeof(uint32_t) * (idx)))

static uint32_t sPin;

otPlatGpioEventHandler mGpioEventHandler = NULL;
void * mGpioEventHandlerContext = NULL;

void nrf5GpioInit(void)
{
    for (uint32_t i = 0; i < NUMBER_OF_PINS; i++)
    {
        nrf_gpio_pin_clear(i);
    }
}

void nrf5GpioDeinit(void)
{
    for (uint32_t i = 0; i < NUMBER_OF_PINS; i++)
    {
        nrf_gpio_cfg_default(i);
    }
    nrf_gpiote_te_default(GPIOTE_CHANNEL);
}

void otGpioInit(void)
{
    nrf5GpioInit();
}

void otPlatGpioCfgOutput(uint32_t aPin)
{
    nrf_gpio_cfg_output(aPin);
}

void otPlatGpioCfgInput(uint32_t aPin)
{
    nrf_gpio_cfg_input(aPin, NRF_GPIO_PIN_NOPULL);
}

void otPlatGpioWrite(uint32_t aPin, uint32_t aValue)
{
    nrf_gpio_pin_write(aPin, aValue);
}

void otPlatGpioClear(uint32_t aPin)
{
    nrf_gpio_pin_clear(aPin);
}

uint32_t otPlatGpioRead(uint32_t aPin)
{
    return nrf_gpio_pin_read(aPin);
}

void otPlatGpioSet(uint32_t aPin)
{
    nrf_gpio_pin_set(aPin);
}

void otPlatGpioToggle(uint32_t aPin)
{
    nrf_gpio_pin_toggle(aPin);
}

void otPlatGpioEnableInterrupt(uint32_t aPin, otPlatGpioEventHandler aGpioEventHandler, void *aContext)
{
    // init gpiote for event/interrupt 
    NRFX_IRQ_PRIORITY_SET(GPIOTE_IRQn, NRFX_GPIOTE_CONFIG_IRQ_PRIORITY);
    NRFX_IRQ_ENABLE(GPIOTE_IRQn);
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
    nrf_gpiote_int_enable(GPIOTE_INTENSET_PORT_Msk);

    nrf_gpio_cfg_input(aPin, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpiote_event_configure(GPIOTE_CHANNEL, aPin, GPIOTE_CONFIG_POLARITY_LoToHi);

    nrf_gpiote_events_t event = TE_IDX_TO_EVENT_ADDR(GPIOTE_CHANNEL);

    nrf_gpiote_event_enable(GPIOTE_CHANNEL);
    nrf_gpiote_event_clear(event);
    nrf_gpiote_int_enable(1 << GPIOTE_CHANNEL);
    sPin = aPin;
    mGpioEventHandler = aGpioEventHandler;
    mGpioEventHandlerContext = aContext;
}

void otPlatGpioDisableInterrupt(uint32_t aPin)
{
    NRFX_IRQ_DISABLE(GPIOTE_IRQn);
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
    nrf_gpiote_int_disable(GPIOTE_INTENSET_PORT_Msk);

    nrf_gpio_pin_clear(aPin);
    nrf_gpiote_event_disable(GPIOTE_CHANNEL);
    nrf_gpiote_int_disable(1 << GPIOTE_CHANNEL);
    mGpioEventHandler = NULL;
}

void GPIOTE_IRQHandler(void)
{
    uint32_t status = 0;
    uint32_t i;
    nrf_gpiote_events_t event = NRF_GPIOTE_EVENTS_IN_0;
    uint32_t mask = (uint32_t)NRF_GPIOTE_INT_IN0_MASK;

    for (i = 0; i < GPIOTE_CH_NUM; i++)
    {
        if (nrf_gpiote_event_is_set(event) && nrf_gpiote_int_is_enabled(mask))
        {
            nrf_gpiote_event_clear(event);
            status |= mask;
        }
        mask <<= 1;
        event = (nrf_gpiote_events_t)((uint32_t)event + sizeof(uint32_t));
    }

    /* Process pin events. */
    if (status & NRF_GPIOTE_INT_IN_MASK)
    {
        mask = (uint32_t)NRF_GPIOTE_INT_IN0_MASK;

        for (i = 0; i < GPIOTE_CH_NUM; i++)
        {
            if (mask & status)
            {
                uint32_t pin = nrf_gpiote_event_pin_get(i);
                if (pin == sPin)
		{
                    if (mGpioEventHandler != NULL)
                    {
    		        mGpioEventHandler(mGpioEventHandlerContext);
                    }
                }
            }
            mask <<= 1;
        }
    }
}

/**
 * The gpio driver weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatGpioSignalEvent(uint32_t aPin)
{
}
