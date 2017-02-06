/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup Wakeup_Timer
 * \{
 * \brief Wakeup Timer
 */

/**
 *****************************************************************************************
 *
 * @file hw_wkup.h
 *
 * @brief Definition of API for the Wakeup timer Low Level Driver.
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

#ifndef HW_WKUP_H_
#define HW_WKUP_H_

#if dg_configUSE_HW_WKUP

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>
#include "hw_gpio.h"


#if (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
#define WKUP_SEL_P0_BASE_REG (volatile uint16_t *)(&WAKEUP->WKUP_SEL_GPIO_P0_REG)
#else
#define WKUP_SEL_P0_BASE_REG (volatile uint16_t *)(&WAKEUP->WKUP_SELECT_P0_REG)
#endif

#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
/**
 * \brief Wakeup timer configuration
 *
 */
typedef struct {
        uint8_t pin_state[HW_GPIO_NUM_PORTS];   /**< pin states in each port, 1: enabled, 0: disabled */
        uint8_t pin_trigger[HW_GPIO_NUM_PORTS]; /**< pin triggers in each port, 1: low, 0: high */
} wkup_pin_config_t;
extern volatile wkup_pin_config_t wkup_pin_config;      /**< stores wkup pin configuration */
extern volatile uint8_t wkup_status[HW_GPIO_NUM_PORTS]; /**< stores wakeup sources */
#endif

/**
 * \brief Get the mask of a field of an WKUP register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_WKUP_REG_FIELD_MASK(reg, field) \
        (WAKEUP_WKUP_##reg##_REG_##field##_Msk)

/**
 * \brief Get the bit position of a field of an WKUP register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_WKUP_REG_FIELD_POS(reg, field) \
        (WAKEUP_WKUP_##reg##_REG_##field##_Pos)

/**
 * \brief Get the value of a field of an WKUP register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_WKUP_REG_GETF(reg, field) \
        ((WAKEUP->WKUP_##reg##_REG & (WAKEUP_WKUP_##reg##_REG_##field##_Msk)) >> (WAKEUP_WKUP_##reg##_REG_##field##_Pos))

/**
 * \brief Set the value of a field of an WKUP register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_WKUP_REG_SETF(reg, field, new_val) \
        WAKEUP->WKUP_##reg##_REG = ((WAKEUP->WKUP_##reg##_REG & ~(WAKEUP_WKUP_##reg##_REG_##field##_Msk)) | \
        ((WAKEUP_WKUP_##reg##_REG_##field##_Msk) & ((new_val) << (WAKEUP_WKUP_##reg##_REG_##field##_Pos))))


/**
 * \brief Pin state which increments event counter
 *
 */
typedef enum {
        HW_WKUP_PIN_STATE_HIGH = 0,
        HW_WKUP_PIN_STATE_LOW = 1
} HW_WKUP_PIN_STATE;

/**
 * \brief Wakeup timer configuration
 *
 */
typedef struct {
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        /**
         * counter threshold
         *
         * \warning Supported only in DA14680/1 chips
         */
        uint8_t threshold;
#endif
#if !((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))
        uint8_t debounce;       /**< debounce time in ms */
#endif
        uint8_t pin_state[HW_GPIO_NUM_PORTS];   /**< pin states in each port, see hw_wkup_configure_port */
        uint8_t pin_trigger[HW_GPIO_NUM_PORTS]; /**< pin triggers in each port, see hw_wkup_configure_port */
} wkup_config;

typedef void (*hw_wkup_interrupt_cb)(void);

/**
 * \brief Initialize peripheral
 *
 * Resets Wakeup Timer to initial state, i.e. interrupt is disabled and all pin triggers are
 * disabled.
 *
 * \p cfg can be NULL - no configuration is performed in such case.
 *
 * \param [in] cfg configuration
 *
 */
void hw_wkup_init(const wkup_config *cfg);

/**
 * \brief Configure peripheral
 *
 * Shortcut to call appropriate configuration function. If \p cfg is NULL, this function does
 * nothing.
 *
 * \param [in] cfg configuration
 *
 */
void hw_wkup_configure(const wkup_config *cfg);

/**
 * \brief Register interrupt handler
 *
 * Callback function is called when interrupt is generated, i.e. event counter reaches configured
 * value. Interrupt is automatically enabled after calling this function. Application should reset
 * interrupt in callback function using hw_wkup_reset_interrupt(). If no callback is specified,
 * interrupt will be automatically cleared by the driver.
 *
 * \param [in] cb callback function
 * \param [in] prio the priority of the interrupt
 *
 * \sa hw_wkup_unregister_interrupt
 * \sa hw_wkup_clear_interrupt
 * \sa hw_wkup_set_counter_threshold
 * \sa hw_wkup_get_counter
 *
 */
void hw_wkup_register_interrupt(hw_wkup_interrupt_cb cb, uint32_t prio);

/**
 * \brief Unregister interrupt handler
 *
 * Interrupt is automatically disabled after calling this function.
 *
 */
void hw_wkup_unregister_interrupt(void);

/**
 * \brief Reset interrupt
 *
 * This function MUST be called by any user-specified interrupt callback, to clear the interrupt.
 *
 */
static inline void hw_wkup_reset_interrupt(void)
{
        WAKEUP->WKUP_RESET_IRQ_REG = 1;
}

/**
 * \brief Interrupt handler
 *
 */
void hw_wkup_handler(void);

#if !((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))
/**
 * \brief Set debounce time
 *
 * Setting debounce time to 0 will disable hardware debouncing. Maximum debounce time is 63ms.
 *
 * \param [in] time_ms debounce time in milliseconds
 *
 */
static inline void hw_wkup_set_debounce_time(uint8_t time_ms)
{
        HW_WKUP_REG_SETF(CTRL, WKUP_DEB_VALUE, time_ms);
}
#endif //!((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))

/**
 * \brief Get current debounce time
 *
 * \return debounce time in milliseconds
 *
 */
static inline uint8_t hw_wkup_get_debounce_time(void)
{
        return HW_WKUP_REG_GETF(CTRL, WKUP_DEB_VALUE);
}

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
/**
 * \brief Set threshold for event counter
 *
 * Interrupt is generated after event counter reaches configured value.
 *
 * \param [in] level number of events
 *
 * \warning Supported only in DA14680/1 chips
 * \sa hw_wkup_register_interrupt
 * \sa hw_wkup_get_counter
 *
 */
static inline void hw_wkup_set_counter_threshold(uint8_t level)
{
        HW_WKUP_REG_SETF(COMPARE, COMPARE, level);
}

/**
 * \brief Get threshold for event counter
 *
 * \return number of events
 *
 * \warning Supported only in DA14680/1 chips
 */
static inline uint8_t hw_wkup_get_counter_threshold(void)
{
        return HW_WKUP_REG_GETF(COMPARE, COMPARE);
}

/**
 * \brief Get current value of event counter
 *
 * Number of events counted so far. Can be reset using hw_wkup_reset_counter().
 *
 * \return number of events
 *
 * \note The counter is automatically reset by the hardware when the interrupt is generated.
 *
 * \warning Supported only in DA14680/1 chips
 *
 * \sa hw_wkup_reset_counter
 *
 */
static inline uint8_t hw_wkup_get_counter(void)
{
        return HW_WKUP_REG_GETF(COUNTER, EVENT_VALUE);
}

/**
 * \brief Reset event counter
 *
 * There is no need to reset counter manually in interrupt callback - it's reset automatically by
 * hardware.
 *
 * \warning Supported only in DA14680/1 chips
 */
static inline void hw_wkup_reset_counter(void)
{
        WAKEUP->WKUP_RESET_CNTR_REG = 1;
}
#endif

/**
 * \brief Set GPIO pin event counting state
 *
 * Once enabled, state changes on pin will increment event counter. State which triggers event can
 * be set using hw_wkup_set_pin_trigger().
 *
 * \param [in] port port number
 * \param [in] pin pin number
 * \param [in] enabled pin event counting state
 *
 * \sa hw_wkup_set_pin_trigger
 * \sa hw_wkup_configure_pin
 * \sa hw_wkup_configure_port
 *
 */
static inline void hw_wkup_set_pin_state(HW_GPIO_PORT port, HW_GPIO_PIN pin, bool enabled)
{
        volatile uint16_t *wkup_pin_enable_reg = WKUP_SEL_P0_BASE_REG + port;
        uint16_t wkup_pin_enable_val = *wkup_pin_enable_reg;
        wkup_pin_enable_val &= ~(1 << pin);
        wkup_pin_enable_val |= (!!enabled) << pin;
        *wkup_pin_enable_reg = wkup_pin_enable_val;
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
        wkup_pin_config.pin_state[port] = wkup_pin_enable_val;
#endif
}

/** \brief Get GPIO pin event counting state
 *
 * \param [in] port port number
 * \param [in] pin pin number
 *
 * \return pin event counting state
 *
 * \sa hw_wkup_set_pin_state
 *
 */
static inline bool hw_wkup_get_pin_state(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        uint16_t wkup_pin_enable_val = *(WKUP_SEL_P0_BASE_REG + port);
        return (wkup_pin_enable_val & (1 << pin)) >> pin;
}

/**
 * \brief Set GPIO pin state which triggers event
 *
 * Pin event counting should be enabled for this setting to have any effect.
 *
 * \param [in] port port number
 * \param [in] pin pin number
 * \param [in] state pin state
 *
 * \sa hw_wkup_set_pin_counter_state
 * \sa hw_wkup_configure_pin
 * \sa hw_wkup_configure_port
 *
 */
static inline void hw_wkup_set_pin_trigger(HW_GPIO_PORT port, HW_GPIO_PIN pin,
                                                                        HW_WKUP_PIN_STATE state)
{
        uint16_t pol_rx_reg = *((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + port);
        pol_rx_reg &= ~(1 << pin);
        pol_rx_reg |= (!!state) << pin;
        *((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + port) = pol_rx_reg;
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
        wkup_pin_config.pin_trigger[port] = pol_rx_reg;
#endif
}

/** \brief Get GPIO pin state which triggers event
 *
 * \param [in] port port number
 * \param [in] pin pin number
 *
 * \return pin state
 *
 * \sa hw_wkup_set_pin_trigger
 *
 */
static inline HW_WKUP_PIN_STATE hw_wkup_get_pin_trigger(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        uint16_t pol_rx_reg = *((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + port);
        return (pol_rx_reg & (1 << pin)) >> pin;
}

/**
 * \brief Set GPIO pin event counting and triggered state
 *
 * Effectively, this is shortcut for calling hw_wkup_set_pin_state() and hw_wkup_set_pin_trigger().
 *
 * \param [in] port port number
 * \param [in] pin pin number
 * \param [in] enabled pin event counting state
 * \param [in] state pin state
 *
 * \sa hw_wkup_set_pin_counter_state
 * \sa hw_wkup_set_pin_trigger
 * \sa hw_wkup_configure_port
 *
 */
static inline void hw_wkup_configure_pin(HW_GPIO_PORT port, HW_GPIO_PIN pin, bool enabled,
                                                                        HW_WKUP_PIN_STATE state)
{
        // first set up the proper polarity...
        hw_wkup_set_pin_trigger(port, pin, state);
        // ...then enable counting on the specific GPIO
        hw_wkup_set_pin_state(port, pin, enabled);
}

/**
 * \brief Configure event counting and triggering state for whole GPIO port
 *
 * In \p enabled and \p state bitmasks each bit describes state of corresponding pin in port.
 * For \p enabled 0 means disabled and 1 means enabled.
 * For \p state 0 means event is triggered on low state and 1 means trigger is on high state.
 *
 * \param [in] port port number
 * \param [in] enabled pin event counting bitmask
 * \param [in] state pin state bitmask
 *
 * \sa hw_wkup_set_pin_counter_state
 * \sa hw_wkup_set_pin_trigger
 * \sa hw_wkup_configure_pin
 *
 */
static inline void hw_wkup_configure_port(HW_GPIO_PORT port, uint8_t enabled, uint8_t state)
{
        *((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + port) = ~state; // register has inverted logic than state bitmask
        *(WKUP_SEL_P0_BASE_REG + port) = enabled;
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
        wkup_pin_config.pin_state[port] = enabled;
        wkup_pin_config.pin_trigger[port] = ~state;
#endif
}

/**
 * \brief Get state (enabled/disabled) of all pins in GPIO port
 *
 * Meaning of bits in returned bitmask is the same as in hw_wkup_configure_port().
 *
 * \return port pin event counter state bitmask
 *
 * \sa hw_wkup_configure_port
 *
 */
static inline uint8_t hw_wkup_get_port_state(HW_GPIO_PORT port)
{
#if dg_configLATCH_WKUP_SOURCE
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        return wkup_pin_config.pin_state[port];
#else
        return *((volatile uint16_t *)(&WAKEUP->WKUP_SEL_GPIO_P0_REG) + port);
#endif
#else
        return *((volatile uint16_t *)(&WAKEUP->WKUP_SELECT_P0_REG) + port);
#endif
}

/**
 * \brief Get event triggering state for all pins in GPIO port
 *
 * Meaning of bits in returned bitmask is the same as in hw_wkup_configure_port().
 *
 * \return port pin event triggering state bitmask
 *
 * \sa hw_wkup_configure_port
 *
 */
static inline uint8_t hw_wkup_get_port_trigger(HW_GPIO_PORT port)
{
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
        return wkup_pin_config.pin_trigger[port];
#else
        return ~(*((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + port)); // register has inverted logic that returned bitmask
#endif
}

/**
 * \brief Emulate key hit
 *
 * Event counter will be increased with debounce time taken into account (if enabled).
 *
 */
static inline void hw_wkup_emulate_key_hit(void)
{
        HW_WKUP_REG_SETF(CTRL, WKUP_SFT_KEYHIT, 1);
        HW_WKUP_REG_SETF(CTRL, WKUP_SFT_KEYHIT, 0);
}

/**
 * \brief Freeze wakeup timer
 *
 */
static inline void hw_wkup_freeze(void)
{
        GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WKUPTIM_Msk;
}

/**
 * \brief Unfreeze wakeup timer
 *
 */
static inline void hw_wkup_unfreeze(void)
{
        GPREG->RESET_FREEZE_REG = GPREG_RESET_FREEZE_REG_FRZ_WKUPTIM_Msk;
}

/**
 * \brief Get port status on last wake up
 *
 * Meaning of bits in returned bitmask is the same as in hw_wkup_configure_port().
 *
 * \return port pin event counter state bitmask
 *
 * \sa hw_wkup_configure_port
 *
 */
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
static inline uint8_t hw_wkup_get_status(HW_GPIO_PORT port)
{
        return wkup_status[port];
}
#elif dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
static inline uint8_t hw_wkup_get_status(HW_GPIO_PORT port)
{
        switch (port) {
        case HW_GPIO_PORT_0:
                return HW_WKUP_REG_GETF(STATUS_0, WKUP_STAT_P0);
        case HW_GPIO_PORT_1:
                return HW_WKUP_REG_GETF(STATUS_0, WKUP_STAT_P1);
        case HW_GPIO_PORT_2:
                return HW_WKUP_REG_GETF(STATUS_1, WKUP_STAT_P2);
        case HW_GPIO_PORT_3:
                return HW_WKUP_REG_GETF(STATUS_2, WKUP_STAT_P3);
        case HW_GPIO_PORT_4:
                return HW_WKUP_REG_GETF(STATUS_2, WKUP_STAT_P4);
        default:
                ASSERT_WARNING(0);//Invalid argument
        }
}
#endif

/**
 * \brief Clear latch status
 *
 * This function MUST be called by any user-specified interrupt callback,
 * to clear the interrupt latch status.
 *
 * \param [in] port port number
 * \param [in] status pin status bitmask
 *
 * \sa hw_wkup_get_status
 */
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) && dg_configLATCH_WKUP_SOURCE
static inline void hw_wkup_clear_status(HW_GPIO_PORT port, uint8_t status)
{
        ASSERT_WARNING((port>=HW_GPIO_PORT_0) && (port<=HW_GPIO_PORT_4))
        wkup_status[port] &= ~status;
}
#elif dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
static inline void hw_wkup_clear_status(HW_GPIO_PORT port, uint8_t status)
{
        switch (port) {
        case HW_GPIO_PORT_0:
                HW_WKUP_REG_SETF(CLEAR_0, WKUP_CLEAR_P0, status);
                break;
        case HW_GPIO_PORT_1:
                HW_WKUP_REG_SETF(CLEAR_0, WKUP_CLEAR_P1, status);
                break;
        case HW_GPIO_PORT_2:
                HW_WKUP_REG_SETF(CLEAR_1, WKUP_CLEAR_P2, status);
                break;
        case HW_GPIO_PORT_3:
                HW_WKUP_REG_SETF(CLEAR_2, WKUP_CLEAR_P3, status);
                break;
        case HW_GPIO_PORT_4:
                HW_WKUP_REG_SETF(CLEAR_2, WKUP_CLEAR_P4, status);
                break;
        default:
                ASSERT_WARNING(0);//Invalid argument
        }
}
#endif

#endif /* dg_configUSE_HW_WKUP */
#endif /* HW_WKUP_H_ */

/**
 * \}
 * \}
 * \}
 */
