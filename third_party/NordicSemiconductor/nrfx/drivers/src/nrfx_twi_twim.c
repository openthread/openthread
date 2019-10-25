/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx.h>
#include <nrf_gpio.h>

#define TWI_TWIM_PIN_CONFIGURE(_pin) nrf_gpio_cfg((_pin),                     \
                                                  NRF_GPIO_PIN_DIR_OUTPUT,    \
                                                  NRF_GPIO_PIN_INPUT_CONNECT, \
                                                  NRF_GPIO_PIN_PULLUP,        \
                                                  NRF_GPIO_PIN_S0D1,          \
                                                  NRF_GPIO_PIN_NOSENSE)

nrfx_err_t nrfx_twi_twim_bus_recover(uint32_t scl_pin, uint32_t sda_pin)
{
    nrf_gpio_pin_set(scl_pin);
    nrf_gpio_pin_set(sda_pin);

    TWI_TWIM_PIN_CONFIGURE(scl_pin);
    TWI_TWIM_PIN_CONFIGURE(sda_pin);
    NRFX_DELAY_US(4);

    for (uint8_t i = 0; i < 9; i++)
    {
        if (nrf_gpio_pin_read(sda_pin))
        {
            break;
        }
        else
        {
            // Pulse CLOCK signal
            nrf_gpio_pin_clear(scl_pin);
            NRFX_DELAY_US(4);
            nrf_gpio_pin_set(scl_pin);
            NRFX_DELAY_US(4);
        }
    }

    // Generate a STOP condition on the bus
    nrf_gpio_pin_clear(sda_pin);
    NRFX_DELAY_US(4);
    nrf_gpio_pin_set(sda_pin);
    NRFX_DELAY_US(4);

    if (nrf_gpio_pin_read(sda_pin))
    {
        return NRFX_SUCCESS;
    }
    else
    {
        return NRFX_ERROR_INTERNAL;
    }
}
