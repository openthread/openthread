/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup GPIO
 * \{
 * \brief GPIO Control
 */

/**
 *****************************************************************************************
 *
 * @file hw_gpio.h
 *
 * @brief Definition of API for the GPIO Low Level Driver.
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
 *****************************************************************************************
 */

#ifndef HW_GPIO_H_
#define HW_GPIO_H_

#if dg_configUSE_HW_GPIO

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>

/* GPIO layout definitions */

/** Number of GPIO ports available */
#define HW_GPIO_NUM_PORTS       (5)
/** Number of GPIO pins available (cumulative) */
#define HW_GPIO_NUM_PINS        (HW_GPIO_PORT_0_NUM_PINS + HW_GPIO_PORT_1_NUM_PINS + \
                                        HW_GPIO_PORT_2_NUM_PINS + HW_GPIO_PORT_3_NUM_PINS + \
                                        HW_GPIO_PORT_4_NUM_PINS)
/** Number of GPIO pins available in port 0 */
#define HW_GPIO_PORT_0_NUM_PINS (8)
/** Number of GPIO pins available in port 1 */
#define HW_GPIO_PORT_1_NUM_PINS (8)
/** Number of GPIO pins available in port 2 */
#define HW_GPIO_PORT_2_NUM_PINS (5)
/** Number of GPIO pins available in port 3 */
#define HW_GPIO_PORT_3_NUM_PINS (8)
/** Number of GPIO pins available in port 4 */
#define HW_GPIO_PORT_4_NUM_PINS (8)
/** Number of pins in each GPIO port */
extern const uint8_t hw_gpio_port_num_pins[HW_GPIO_NUM_PORTS];


/**
 * \brief GPIO input/output mode
 *
 */
typedef enum {
        HW_GPIO_MODE_INPUT = 0,                 /**< GPIO as an input */
        HW_GPIO_MODE_INPUT_PULLUP = 0x100,      /**< GPIO as an input with pull-up */
        HW_GPIO_MODE_INPUT_PULLDOWN = 0x200,    /**< GPIO as an input with pull-down */
        HW_GPIO_MODE_OUTPUT = 0x300,            /**< GPIO as an (implicitly push-pull) output */
        HW_GPIO_MODE_OUTPUT_PUSH_PULL = 0x300,  /**< GPIO as an (explicitly push-pull) output */
        HW_GPIO_MODE_OUTPUT_OPEN_DRAIN = 0x700, /**< GPIO as an open-drain output */
} HW_GPIO_MODE;

/**
 * \brief GPIO power source
 *
 */
typedef enum {
        HW_GPIO_POWER_V33 = 0,          /**< V33 (3.3 V) power rail */
        HW_GPIO_POWER_VDD1V8P = 1,      /**< VDD1V8P (1.8 V) power rail */
} HW_GPIO_POWER;

/**
 * \brief GPIO port number
 *
 */
typedef enum {
        HW_GPIO_PORT_0 = 0,     /**< GPIO Port 0 */
        HW_GPIO_PORT_1 = 1,     /**< GPIO Port 1 */
        HW_GPIO_PORT_2 = 2,     /**< GPIO Port 2 */
        HW_GPIO_PORT_3 = 3,     /**< GPIO Port 3 */
        HW_GPIO_PORT_4 = 4      /**< GPIO Port 4 */
} HW_GPIO_PORT;

/**
 * \brief GPIO pin number
 *
 */
typedef enum {
        HW_GPIO_PIN_0 = 0,      /**< GPIO Pin 0 */
        HW_GPIO_PIN_1 = 1,      /**< GPIO Pin 1 */
        HW_GPIO_PIN_2 = 2,      /**< GPIO Pin 2 */
        HW_GPIO_PIN_3 = 3,      /**< GPIO Pin 3 */
        HW_GPIO_PIN_4 = 4,      /**< GPIO Pin 4 */
        HW_GPIO_PIN_5 = 5,      /**< GPIO Pin 5 */
        HW_GPIO_PIN_6 = 6,      /**< GPIO Pin 6 */
        HW_GPIO_PIN_7 = 7,      /**< GPIO Pin 7 */
} HW_GPIO_PIN;

/**
 * \brief GPIO function
 *
 */
typedef enum {
        HW_GPIO_FUNC_GPIO = 0,                  /**< GPIO */
        HW_GPIO_FUNC_UART_RX = 1,               /**< GPIO as UART RX */
        HW_GPIO_FUNC_UART_TX = 2,               /**< GPIO as UART TX */
        HW_GPIO_FUNC_UART_IRDA_RX = 3,          /**< GPIO as UART IRDA RX */
        HW_GPIO_FUNC_UART_IRDA_TX = 4,          /**< GPIO as UART IRDA TX */
        HW_GPIO_FUNC_UART2_RX = 5,              /**< GPIO as UART2 RX */
        HW_GPIO_FUNC_UART2_TX = 6,              /**< GPIO as UART2 TX */
        HW_GPIO_FUNC_UART2_IRDA_RX = 7,         /**< GPIO as UART2 IRDA RX */
        HW_GPIO_FUNC_UART2_IRDA_TX = 8,         /**< GPIO as UART2 IRDA TX */
        HW_GPIO_FUNC_UART2_CTSN = 9,            /**< GPIO as UART2 CTSN */
        HW_GPIO_FUNC_UART2_RTSN = 10,           /**< GPIO as UART2 RTSN */
        HW_GPIO_FUNC_SPI_DI = 11,               /**< GPIO as SPI DI */
        HW_GPIO_FUNC_SPI_DO = 12,               /**< GPIO as SPI DO */
        HW_GPIO_FUNC_SPI_CLK = 13,              /**< GPIO as SPI CLK */
        HW_GPIO_FUNC_SPI_EN = 14,               /**< GPIO as SPI EN */
        HW_GPIO_FUNC_SPI2_DI = 15,              /**< GPIO as SPI2 DI */
        HW_GPIO_FUNC_SPI2_DO = 16,              /**< GPIO as SPI2 DO */
        HW_GPIO_FUNC_SPI2_CLK = 17,             /**< GPIO as SPI2 CLK */
        HW_GPIO_FUNC_SPI2_EN = 18,              /**< GPIO as SPI2 EN */
        HW_GPIO_FUNC_I2C_SCL = 19,              /**< GPIO as I2C SCL */
        HW_GPIO_FUNC_I2C_SDA = 20,              /**< GPIO as I2C SDA */
        HW_GPIO_FUNC_I2C2_SCL = 21,             /**< GPIO as I2C2 SCL */
        HW_GPIO_FUNC_I2C2_SDA = 22,             /**< GPIO as I2C2 SDA */
        HW_GPIO_FUNC_PWM0 = 23,                 /**< GPIO as PWM0 */
        HW_GPIO_FUNC_PWM1 = 24,                 /**< GPIO as PWM1 */
        HW_GPIO_FUNC_PWM2 = 25,                 /**< GPIO as PWM2 */
        HW_GPIO_FUNC_PWM3 = 26,                 /**< GPIO as PWM3 */
        HW_GPIO_FUNC_PWM4 = 27,                 /**< GPIO as PWM4 */
        HW_GPIO_FUNC_BLE_DIAG = 28,             /**< GPIO as BLE DIAG */
        HW_GPIO_FUNC_FTDF_DIAG = 29,            /**< GPIO as FTDF DIAG */
        HW_GPIO_FUNC_PCM_DI = 30,               /**< GPIO as PCM DI */
        HW_GPIO_FUNC_PCM_DO = 31,               /**< GPIO as PCM DO */
        HW_GPIO_FUNC_PCM_FSC = 32,              /**< GPIO as PCM FSC */
        HW_GPIO_FUNC_PCM_CLK = 33,              /**< GPIO as PCM CLK */
        HW_GPIO_FUNC_PDM_DI = 34,               /**< GPIO as PDM DI */
        HW_GPIO_FUNC_PDM_DO = 35,               /**< GPIO as PDM DO */
        HW_GPIO_FUNC_PDM_CLK = 36,              /**< GPIO as PDM CLK */
        HW_GPIO_FUNC_USB_SOF = 37,              /**< GPIO as USB SOF */
        HW_GPIO_FUNC_ADC = 38,                  /**< GPIO as ADC */
        HW_GPIO_FUNC_USB = 38,                  /**< GPIO as USB */
        HW_GPIO_FUNC_QSPI = 38,                 /**< GPIO as QSPI */
        HW_GPIO_FUNC_XTAL32 = 38,               /**< GPIO as XTAL32 */
        HW_GPIO_FUNC_QUADEC_XA = 39,            /**< GPIO as QUADEC XA */
        HW_GPIO_FUNC_QUADEC_XB = 40,            /**< GPIO as QUADEC XB */
        HW_GPIO_FUNC_QUADEC_YA = 41,            /**< GPIO as QUADEC YA */
        HW_GPIO_FUNC_QUADEC_YB = 42,            /**< GPIO as QUADEC YB */
        HW_GPIO_FUNC_QUADEC_ZA = 43,            /**< GPIO as QUADEC ZA */
        HW_GPIO_FUNC_QUADEC_ZB = 44,            /**< GPIO as QUADEC ZB */
        HW_GPIO_FUNC_IR_OUT = 45,               /**< GPIO as IR OUT */
        HW_GPIO_FUNC_BREATH = 46,               /**< GPIO as BREATH */
        HW_GPIO_FUNC_KB_ROW = 47,               /**< GPIO as KB ROW */
        HW_GPIO_FUNC_COEX_EXT_ACT0 = 48,        /**< GPIO as COEX EXT ACT0 */
        HW_GPIO_FUNC_COEX_EXT_ACT1 = 49,        /**< GPIO as COEX EXT ACT1 */
        HW_GPIO_FUNC_COEX_SMART_ACT = 50,       /**< GPIO as COEX SMART ACT */
        HW_GPIO_FUNC_COEX_SMART_PRI = 51,       /**< GPIO as COEX SMART PRI */
        HW_GPIO_FUNC_CLOCK = 52,                /**< GPIO as CLOCK */
        HW_GPIO_FUNC_ONESHOT = 53,              /**< GPIO as ONESHOT */
        HW_GPIO_FUNC_PWM5 = 54,                 /**< GPIO as PWM5 */
        HW_GPIO_FUNC_PORT0_DCF = 55,            /**< GPIO as PORT0 DCF */
        HW_GPIO_FUNC_PORT1_DCF = 56,            /**< GPIO as PORT1 DCF */
        HW_GPIO_FUNC_PORT2_DCF = 57,            /**< GPIO as PORT2 DCF */
        HW_GPIO_FUNC_PORT3_DCF = 58,            /**< GPIO as PORT3 DCF */
        HW_GPIO_FUNC_PORT4_DCF = 59,            /**< GPIO as PORT4 DCF */
        HW_GPIO_FUNC_RF_ANT_TRIM0 = 60,         /**< GPIO as RF ANT TRIM0 */
        HW_GPIO_FUNC_RF_ANT_TRIM1 = 61,         /**< GPIO as RF ANT TRIM1 */
        HW_GPIO_FUNC_RF_ANT_TRIM2 = 62          /**< GPIO as RF ANT TRIM2 */
} HW_GPIO_FUNC;

/**
 * \brief GPIO pin configuration
 *
 * It's recommended to use \p HW_GPIO_PINCONFIG and \p HW_GPIO_PINCONFIG_RESERVE to set pin entries.
 * Each configuration must be terminated using \p HW_GPIO_PINCONFIG_END macro.
 *
 */
typedef struct {
        uint8_t         pin;     /**< pin name, high-nibble is port number and low-nibble is pin */
        HW_GPIO_MODE    mode;    /**< pin mode */
        HW_GPIO_FUNC    func;    /**< pin function */
        bool            high;    /**< initial pin state, true for high and false for low */
        bool            reserve; /*<< true if pin should be also reserved */
} __attribute__((packed)) gpio_config;

/**
 * \brief GPIO pin configuration for \p gpio_config
 *
 * \p xport and \p xpin are specified as symbols from \p HW_GPIO_PORT and \p HW_GPIO_PIN enums
 * respectively or more conveniently as plain numeric values.
 * \p xmode and \p xfunc have the same values as defined in \p HW_GPIO_MODE and \p HW_GPIO_FUNC enums
 * respectively, except they have prefix stripped.
 *
 * \param [in] xport port number
 * \param [in] xpin pin number
 * \param [in] xmode pin mode
 * \param [in] xfunc pin function
 * \param [in] xhigh true for high state, false otherwise
 *
 */
#define HW_GPIO_PINCONFIG(xport, xpin, xmode, xfunc, xhigh) \
        {                                               \
                .pin = (xport << 4) | (xpin & 0x0F),    \
                .mode = HW_GPIO_MODE_ ## xmode,         \
                .func = HW_GPIO_FUNC_ ## xfunc,         \
                .high = xhigh,                          \
                .reserve = false,                       \
        }

/**
 * \brief GPIO pin configuration and reservation for \p gpio_config
 *
 * This macro is virtually identical to \p HW_GPIO_PINCONFIG, except it also reserves pin.
 *
 * \param [in] xport port number
 * \param [in] xpin pin number
 * \param [in] xmode pin mode
 * \param [in] xfunc pin function
 * \param [in] xhigh true for high state, false otherwise
 *
 * \sa HW_GPIO_PINCONFIG
 *
 */
#define HW_GPIO_PINCONFIG_RESERVE(xport, xpin, xmode, xfunc, xhigh) \
        {                                               \
                .pin = (xport << 4) | (xpin & 0x0F),    \
                .mode = HW_GPIO_MODE_ ## xmode,         \
                .func = HW_GPIO_FUNC_ ## xfunc,         \
                .high = xhigh,                          \
                .reserve = true,                        \
        }

/**
 * \brief Macro to properly terminate array of \p gpio_config definition
 *
 */
#define HW_GPIO_PINCONFIG_END \
        {                       \
                .pin = 0xFF,    \
        }

/**
 * \brief GPIO configuration
 *
 * This is a shortcut to configure multiple GPIOs in one call.
 * \p cfg is an array of GPIO pins configuration, it should be terminated by dummy element with
 * \p pin member set to 0xFF (macro \p HW_GPIO_PINCONFIG can be used for this purpose).
 *
 * \param [in] cfg GPIO pins configuration
 *
 * \sa hw_gpio_configure_pin
 * \sa hw_gpio_set_pin_function
 *
 */
void hw_gpio_configure(const gpio_config cfg[]);

/**
 * \brief Reserve GPIO pin
 *
 * Reserve pin for exclusive usage.
 * This macro can be used in application peripheral_setup function to detect
 * usage of same GPIO pin by different applications.
 *
 * \param [in] port GPIO port number
 * \param [in] pin GPIO pin number
 *
 * \return true if pin was successfully reserved and setup, false if pin was already reserved
 *
 */
bool hw_gpio_reserve_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin);

/**
 * \brief Reserve GPIO pin and set pin function
 *
 * Reserve pin and set up it function. If pin was already reserved do nothing.
 *
 * \param [in] port GPIO port number
 * \param [in] pin GPIO pin number
 * \param [in] mode GPIO access mode
 * \param [in] function GPIO function
 * \param [in] high in case of PID_GPIO and OUTPUT value to set on pin
 *
 * \return true if pin was successfully reserved and setup, false if pin was already reserved
 *
 */
bool hw_gpio_reserve_and_configure_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                                HW_GPIO_FUNC function, bool high);

/**
 * \brief Unreserve GPIO pin
 *
 * Free reserved pin. If pin was not reserved do nothing.
 * Configuration of pin does not change just reservation.
 *
 * \note If pin was reserved using RESERVE_GPIO it will also be unreserved.
 * If RESERVE_GPIO was not enabled by compile time flags call to this function
 * may cause unexpected result.
 *
 * \param [in] port GPIO port number
 * \param [in] pin GPIO pin number
 *
 * \sa hw_gpio_reserve_and_configure_pin
 * \sa hw_gpio_reserve_pin
 * \sa RESERVE_GPIO
 *
 */
void hw_gpio_unreserve_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin);

#if (DEBUG_GPIO_ALLOC_MONITOR_ENABLED == 1)
/**
 * \brief Reserve GPIO pin
 *
 * Reserve pin for exclusive usage. If pin is already allocated trigger breakpoint.
 * This macro should be used in application peripheral_setup function to detect
 * usage of same GPIO pin by different applications.
 *
 * If runtime GPIO reservation is needed, use hw_gpio_reserve_pin,
 * hw_gpio_reserve_and_configure_pin and hw_gpio_unreserve_pin instead.
 *
 * \param [in] name parameter ignored, used for debug only
 * \param [in] port GPIO port number
 * \param [in] pin GPIO pin number
 * \param [in] func parameter ignored (for compatibility)
 *
 * \sa hw_gpio_reserve_pin
 * \sa hw_gpio_reserve_and_configure_pin
 * \sa hw_gpio_unreserve_pin instead
 *
 */
#define RESERVE_GPIO(name, port, pin, func)                                                     \
        do {                                                                                    \
                if (!hw_gpio_reserve_pin((port), (pin))) {                                      \
                        /* If debugger stops at this line, there is configuration problem */    \
                        /* pin is used without being reserved first */                          \
                        __BKPT(0); /* this pin has not been previously reserved! */             \
                }                                                                               \
        } while (0)

#else

#define RESERVE_GPIO( name, port, pin, func )   \
        do {                                    \
                                                \
        } while (0)

#endif // (DEBUG_GPIO_ALLOC_MONITOR_ENABLED == 1)

/**
 * \brief Set the pin type and mode
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 * \param [in] mode GPIO pin mode
 * \param [in] function GPIO pin usage
 *
 */
void hw_gpio_set_pin_function(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                                        HW_GPIO_FUNC function);

/**
 * \brief Get the pin type and mode
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 * \param [out] mode GPIO pin mode
 * \param [out] function GPIO pin usage
 *
 */
void hw_gpio_get_pin_function(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE* mode,
                                                                        HW_GPIO_FUNC* function);

/**
 * \brief Combined function to set the state and the type and mode of the GPIO pin
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 * \param [in] mode GPIO pin mode
 * \param [in] function GPIO pin usage
 * \param [in] high set to TRUE to set the pin into high else low
 *
 */
void hw_gpio_configure_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_MODE mode,
                                                        HW_GPIO_FUNC function, const bool high);

/**
 * \brief Configure power source for pin output
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 * \param [in] power GPIO power source
 *
 */
void hw_gpio_configure_pin_power(HW_GPIO_PORT port, HW_GPIO_PIN pin, HW_GPIO_POWER power);

/**
 * \brief Set a GPIO in high state
 *
 * The GPIO should have been previously configured as an output!
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 *
 */
void hw_gpio_set_active(HW_GPIO_PORT port, HW_GPIO_PIN pin);

/**
 * \brief Set a GPIO in low state
 *
 * The GPIO should have been previously configured as an output!
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 *
 */
void hw_gpio_set_inactive(HW_GPIO_PORT port, HW_GPIO_PIN pin);

/**
 * \brief Get the GPIO status
 *
 * The GPIO should have been previously configured as input!
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 *
 * \return true if the pin is high, false if low
 *
 */
bool hw_gpio_get_pin_status(HW_GPIO_PORT port, HW_GPIO_PIN pin);

/**
 * \brief Toggle GPIO pin state
 *
 * \param [in] port GPIO port
 * \param [in] pin GPIO pin
 *
 */
void hw_gpio_toggle(HW_GPIO_PORT port, HW_GPIO_PIN pin);

/**
 * \brief Find pins with specific function
 *
 * Function searches for pins configured for specific function.
 * If buf is not NULL and buf_size is greater than 0 pins are stored in buf
 * high-nibble is port number and low-nibble is pin.
 * If number of pins found is greater then buf_size only buf_size entries are filled, though
 * the returned number of found pins is correct.
 *
 * \param [in] func function to lookup
 * \param [out] buf buffer for port-pin pairs that are configured for specific function
 * \param [in] buf_size size of buf
 *
 * \return number of pins with specific function put in buf
 *         0 - no pin is configured for this function
 *
 */
int hw_gpio_get_pins_with_function(HW_GPIO_FUNC func, uint8_t *buf, int buf_size);


#endif /* dg_configUSE_HW_GPIO */

#endif /* HW_GPIO_H_ */

/**
 * \}
 * \}
 * \}
 */
