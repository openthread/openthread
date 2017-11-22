/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup GPIO
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_gpio.c
 *
 * @brief Implementation of the GPIO Low Level Driver.
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */

#if dg_configUSE_HW_GPIO


#include <stdint.h>
#include <hw_gpio.h>

/* Register adresses */
#define PX_DATA_REG_ADDR(_port)         ((volatile uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_DATA_REG)) + _port)
#define PX_DATA_REG(_port)              *PX_DATA_REG_ADDR(_port)
#define PX_SET_DATA_REG_ADDR(_port)     ((volatile uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_SET_DATA_REG)) + _port)
#define PX_SET_DATA_REG(_port)          *PX_SET_DATA_REG_ADDR(_port)
#define PX_RESET_DATA_REG_ADDR(_port)   ((volatile uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_RESET_DATA_REG)) + _port)
#define PX_RESET_DATA_REG(_port)        *PX_RESET_DATA_REG_ADDR(_port)
#define PXX_MODE_REG_ADDR(_port, _pin)  ((volatile uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P00_MODE_REG)) + (_port * 8)  + _pin)
#define PXX_MODE_REG(_port, _pin)       *PXX_MODE_REG_ADDR(_port, _pin)
#define PX_PADPWR_CTRL_REG_ADDR(_port)  ((volatile uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_PADPWR_CTRL_REG)) + _port)
#define PX_PADPWR_CTRL_REG(_port)       *PX_PADPWR_CTRL_REG_ADDR(_port)

/* on FPGA we cannot read the state of an output GPIO */

#ifdef FPGA_PAD_LOOPBACK_BROKEN
static uint16_t Px_DATA_REG_cache[5];
#endif

#if (dg_configIMAGE_SETUP == PRODUCTION_MODE) && (DEBUG_GPIO_ALLOC_MONITOR_ENABLED == 1)
        #warning "GPIO assignment monitoring is active in PRODUCTION build!"
#endif

static volatile uint8_t GPIO_status[HW_GPIO_NUM_PORTS] = { 0 };

const uint8_t hw_gpio_port_num_pins[HW_GPIO_NUM_PORTS] = {
                                                HW_GPIO_PORT_0_NUM_PINS, HW_GPIO_PORT_1_NUM_PINS,
                                                HW_GPIO_PORT_2_NUM_PINS, HW_GPIO_PORT_3_NUM_PINS,
                                                HW_GPIO_PORT_4_NUM_PINS };

/*
 * Global Functions
 ****************************************************************************************
 */

void hw_gpio_configure(const gpio_config cfg[])
{
#if dg_configIMAGE_SETUP == DEVELOPMENT_MODE
        int num_pins = 0;
        uint8_t set_mask[HW_GPIO_NUM_PORTS] = { 0 };
#endif

        if (!cfg) {
                return;
        }

        while (cfg->pin != 0xFF) {
                uint8_t port = cfg->pin >> 4;
                uint8_t pin = cfg->pin & 0x0F;

#if dg_configIMAGE_SETUP == DEVELOPMENT_MODE
                if (port >= HW_GPIO_NUM_PORTS || pin >= hw_gpio_port_num_pins[port]) {
                        /*
                         * invalid port or pin number specified, it was either incorrectly specified
                         * of cfg was not terminated properly using HW_GPIO_PINCONFIG_END so we're
                         * reading garbage
                         */
                        __BKPT(0);
                }

                if (++num_pins > HW_GPIO_NUM_PINS) {
                        /*
                         * trying to set more pins than available, perhaps cfg was not terminated
                         * properly using HW_GPIO_PINCONFIG_END?
                         */
                        __BKPT(0);
                }

                if (set_mask[port] & (1 << pin)) {
                        /*
                         * trying to set pin which has been already set by this call which means
                         * there is duplicated pin in configuration - does not make sense
                         */
                        __BKPT(0);
                }

                set_mask[port] |= (1 << pin);
#endif

                if (cfg->reserve) {
                        hw_gpio_reserve_and_configure_pin(port, pin, cfg->mode, cfg->func, cfg->high);
                } else {
                        hw_gpio_configure_pin(port, pin, cfg->mode, cfg->func, cfg->high);
                }

                cfg++;
        }
}

bool hw_gpio_reserve_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        if ((GPIO_status[port] & (1 << pin))) {
                return false;
        }

        GPIO_status[port] |= (1 << pin);

        return true;
}

bool hw_gpio_reserve_and_configure_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                                HW_GPIO_FUNC function, bool high)
{
        if (!hw_gpio_reserve_pin(port, pin)) {
                return false;
        }

        hw_gpio_configure_pin(port, pin, mode, function, high);

        return true;
}

void hw_gpio_unreserve_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        GPIO_status[port] &= ~(1 << pin);
}

static void hw_gpio_verify_reserved(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
#if (DEBUG_GPIO_ALLOC_MONITOR_ENABLED == 1)
        if (!(GPIO_status[port] & (1 << pin))) {
                // If debugger stops at this line, there is configuration problem
                // pin is used without being reserved first
                __BKPT(0); /* this pin has not been previously reserved! */
        }
#endif // (DEBUG_GPIO_ALLOC_MONITOR_ENABLED == 1)
}


void hw_gpio_set_pin_function(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                                        HW_GPIO_FUNC function)
{
        hw_gpio_verify_reserved(port, pin);

        PXX_MODE_REG(port, pin) = mode | function;
}

void hw_gpio_get_pin_function(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE* mode,
                                                                        HW_GPIO_FUNC* function)
{
        uint16_t val;

        hw_gpio_verify_reserved(port, pin);

        val = PXX_MODE_REG(port, pin);
        *mode = val & 0x0700;
        *function = val & 0x00ff;
}

void hw_gpio_configure_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                        HW_GPIO_FUNC function, const bool high)
{
        hw_gpio_verify_reserved(port, pin);

        if (high)
                hw_gpio_set_active(port, pin);
        else
                hw_gpio_set_inactive(port, pin);

        hw_gpio_set_pin_function(port, pin, mode, function);
}

void hw_gpio_configure_pin_power(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_POWER power)
{
        uint16_t padpwr;

        padpwr = PX_PADPWR_CTRL_REG(port);
        padpwr &= ~(1 << pin);
        if (power == HW_GPIO_POWER_VDD1V8P) {
                padpwr |= (1 << pin);
        }
        PX_PADPWR_CTRL_REG(port) = padpwr;
}

void hw_gpio_set_active(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        hw_gpio_verify_reserved(port, pin);

        PX_SET_DATA_REG(port) = 1 << pin;
#ifdef FPGA_PAD_LOOPBACK_BROKEN
        Px_DATA_REG_cache[port] |= 1 << pin;
#endif
}

void hw_gpio_set_inactive(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        hw_gpio_verify_reserved(port, pin);

        PX_RESET_DATA_REG(port) = 1 << pin;
#ifdef FPGA_PAD_LOOPBACK_BROKEN
        Px_DATA_REG_cache[port] &= ~(1 << pin);
#endif
}

bool hw_gpio_get_pin_status(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
#ifdef FPGA_PAD_LOOPBACK_BROKEN
        HW_GPIO_MODE mode;
        HW_GPIO_FUNC function;
#endif
        hw_gpio_verify_reserved(port, pin);

#ifdef FPGA_PAD_LOOPBACK_BROKEN
        hw_gpio_get_pin_function(port, pin, &mode, &function);
        if (mode != HW_GPIO_MODE_OUTPUT)
                return !!(PX_DATA_REG(port) & (1 << pin));
        else
                return !!(Px_DATA_REG_cache[port] & (1 << pin));
#else
        return ( (PX_DATA_REG(port) & (1 << pin)) != 0 );
#endif
}

void hw_gpio_toggle(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        hw_gpio_verify_reserved(port, pin);

        if (hw_gpio_get_pin_status(port, pin))
                hw_gpio_set_inactive(port, pin);
        else
                hw_gpio_set_active(port, pin);
}

int hw_gpio_get_pins_with_function(HW_GPIO_FUNC func, uint8_t *buf, int buf_size)
{
        int count = 0;
        int port;
        int pin;
        HW_GPIO_MODE mode;
        HW_GPIO_FUNC pin_func;


        for (port = 0; port < HW_GPIO_NUM_PORTS; ++port) {
                for (pin = 0; pin < hw_gpio_port_num_pins[port]; ++pin) {
                        hw_gpio_get_pin_function(port, pin, &mode, &pin_func);
                        if (pin_func != func) {
                                continue;
                        }
                        if (count < buf_size && buf != NULL) {
                                buf[count] = (uint8_t) ((port << 4) | pin);
                        }
                        count++;
                }
        }
        return count;
}

#endif /* dg_configUSE_HW_GPIO */
/**
 * \}
 * \}
 * \}
 */
