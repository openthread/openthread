/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup Timer0
 * \{
 * \brief Timer0 
 */

/**
 *****************************************************************************************
 *
 * @file hw_timer0.h
 *
 * @brief Definition of API for the Timer0 Low Level Driver.
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

#ifndef HW_TIMER0_H_
#define HW_TIMER0_H_

#if dg_configUSE_HW_TIMER0

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>


/**
 * \brief Get the mask of a field of an TIMER0 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_TIMER0_REG_FIELD_MASK(reg, field) \
        (GP_TIMERS_TIMER0_##reg##_REG_##field##_Msk)

/**
 * \brief Get the bit position of a field of an TIMER0 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_TIMER0_REG_FIELD_POS(reg, field) \
        (GP_TIMERS_TIMER0_##reg##_REG_##field##_Pos)

/**
 * \brief Get the value of a field of an TIMER0 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_TIMER0_REG_GETF(reg, field) \
        ((GP_TIMERS->TIMER0_##reg##_REG & (GP_TIMERS_TIMER0_##reg##_REG_##field##_Msk)) >> (GP_TIMERS_TIMER0_##reg##_REG_##field##_Pos))

/**
 * \brief Set the value of a field of an TIMER0 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_TIMER0_REG_SETF(reg, field, new_val) \
        GP_TIMERS->TIMER0_##reg##_REG = ((GP_TIMERS->TIMER0_##reg##_REG & ~(GP_TIMERS_TIMER0_##reg##_REG_##field##_Msk)) | \
        ((GP_TIMERS_TIMER0_##reg##_REG_##field##_Msk) & ((new_val) << (GP_TIMERS_TIMER0_##reg##_REG_##field##_Pos))))


/**
 * \brief Clock source for timer
 *
 */
typedef enum {
        HW_TIMER0_CLK_SRC_SLOW = 0,        /**< 32kHz (slow) clock */
        HW_TIMER0_CLK_SRC_FAST = 1         /**< 2/4/8/16MHz (fast) clock */
} HW_TIMER0_CLK_SRC;

/**
 * \brief Fast clock division factor
 *
 */
typedef enum {
        HW_TIMER0_FAST_CLK_DIV_1 = 0,      /**< divide by 1 */
        HW_TIMER0_FAST_CLK_DIV_2,          /**< divide by 2 */
        HW_TIMER0_FAST_CLK_DIV_4,          /**< divide by 4 */
        HW_TIMER0_FAST_CLK_DIV_8,          /**< divide by 8 */
} HW_TIMER0_FAST_CLK_DIV;

/**
 * \brief PWM mode
 *
 */
typedef enum {
        HW_TIMER0_MODE_PWM = 0,            /**< PWM signal is '1' during high state */
        HW_TIMER0_MODE_CLOCK = 1           /**< PWM signal is clock divided by 2 during high state */
} HW_TIMER0_MODE;

/**
 * \brief Timer interrupt callback
 *
 */
typedef void (*hw_timer0_interrupt_cb)(void);

/*
 * \brief Timer configuration
 *
 */
typedef struct {
        HW_TIMER0_CLK_SRC       clk_src;        /**< clock source */
        HW_TIMER0_FAST_CLK_DIV  fast_clk_div;   /**< clock division factor (only applicable when fast clock is selected as source) */
        bool                    on_clock_div;   /**< enable ON-counter clock divider (clock for ON-counter is divided by 10) */
        uint16_t                on_reload;      /**< reload value for ON-counter */
        uint16_t                t0_reload_m;    /**< reload value for T0-counter M-register */
        uint16_t                t0_reload_n;    /**< reload value for T0-counter N-register */
} timer0_config;

/**
 * \brief Initialize the timer
 *
 * Turn on clock for timer and configure timer. After initialization both timer and its interrupt
 * are disabled. \p cfg can be NULL - no configuration is performed in such case.
 *
 * \param [in] cfg configuration
 *
 */
void hw_timer0_init(const timer0_config *cfg);

/**
 * \brief Timer configuration
 *
 * Shortcut to call appropriate configuration function. If \p cfg is NULL, this function does
 * nothing.
 *
 * \param [in] cfg configuration
 *
 */
void hw_timer0_configure(const timer0_config *cfg);

/**
 * \brief Register an interrupt handler
 *
 * \param [in] handler Function pointer to call when timer interrupt occurs
 *
 */
void hw_timer0_register_int(hw_timer0_interrupt_cb handler);

/**
 * \brief Unregister an interrupt handler
 *
 */
void hw_timer0_unregister_int(void);

/**
 * \brief Enable the timer
 *
 */
static inline void hw_timer0_enable(void)
{
        HW_TIMER0_REG_SETF(CTRL, TIM0_CTRL, 1);
}

/**
 * \brief Disable the timer
 *
 */
static inline void hw_timer0_disable(void)
{
        HW_TIMER0_REG_SETF(CTRL, TIM0_CTRL, 0);
}

/**
 * \brief Set clock source for timer
 *
 * \param [in] clk_src clock source
 *
 * \sa TIMER0_CLK_SRC
 *
 */
static inline void hw_timer0_set_clock_source(HW_TIMER0_CLK_SRC clk_src)
{
        HW_TIMER0_REG_SETF(CTRL, TIM0_CLK_SEL, clk_src);
}

/**
 * \brief Get current clock source for timer
 *
 * \return clock source
 *
 */
static inline HW_TIMER0_CLK_SRC hw_timer0_get_clock_source(void)
{
        return HW_TIMER0_REG_GETF(CTRL, TIM0_CLK_SEL);
}

/**
 * \brief Set fast clock division factor
 *
 * \param [in] div division factor
 *
 * \sa TIMER0_FAST_CLK_DIV
 *
 */
static inline void hw_timer0_set_fast_clock_div(HW_TIMER0_FAST_CLK_DIV div)
{
        GLOBAL_INT_DISABLE();
        REG_SETF(CRG_TOP, CLK_TMR_REG, TMR0_DIV, div);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Get current fast clock division factor
 *
 * \return division factor
 *
 */
static inline HW_TIMER0_FAST_CLK_DIV  hw_timer0_get_fast_clock_div(void)
{
        return REG_GETF(CRG_TOP, CLK_TMR_REG, TMR0_DIV);
}

/**
 * \brief Set state of clock divider for ON-counter
 *
 * Once enabled, ON-counter clock will be divided by 10.
 *
 * \param [in] enabled clock divider status
 *
 */
static inline void hw_timer0_set_on_clock_div(bool enabled)
{
        HW_TIMER0_REG_SETF(CTRL, TIM0_CLK_DIV, !enabled);
}

/**
 * \brief Get current state of clock divider for ON-counter
 *
 * \return clock divider status
 *
 */
static inline bool hw_timer0_get_on_clock_div(void)
{
        return !HW_TIMER0_REG_GETF(CTRL, TIM0_CLK_DIV);
}

/**
 * \brief Set PWM mode for timer
 *
 * \param [in] mode PWM mode
 *
 * \sa TIMER0_MODE
 *
 */
static inline void hw_timer0_set_pwm_mode(HW_TIMER0_MODE mode)
{
        HW_TIMER0_REG_SETF(CTRL, PWM_MODE, mode);
}

/**
 * \brief Get current PWM mode for timer
 *
 * \return PWM mode
 *
 */
static inline HW_TIMER0_MODE hw_timer0_get_pwm_mode(void)
{
        return HW_TIMER0_REG_GETF(CTRL, PWM_MODE);
}

/**
 * \brief Set reload value for T0-counter
 *
 * T0 counter value is decremented on each clock cycle. At the beginning it's loaded from M-register
 * and then, once reached zero, loaded from N-register (and then M and N again).
 *
 * PWM0 is high when counting down M-register and low when counting down N-register.
 * For PWM1 is the opposite.
 *
 * \param [in] m_value M-register reload value
 * \param [in] n_value N-register reload value
 *
 */
static inline void hw_timer0_set_t0_reload(uint16_t m_value, uint16_t n_value)
{
        GP_TIMERS->TIMER0_RELOAD_M_REG = m_value;
        GP_TIMERS->TIMER0_RELOAD_N_REG = n_value;
}

/**
 * \brief Set reload value for ON-counter
 *
 * ON counter value is decremented on each clock cycle. Once ON reaches zero it will remain zero
 * until T0 counter completes decrementing N-register value. When this happens, interrupt is
 * generated.
 *
 * \param [in] value reload value
 *
 * \sa hw_timer0_set_on_clock_div
 *
 */
static inline void hw_timer0_set_on_reload(uint16_t value)
{
        GP_TIMERS->TIMER0_ON_REG = value;
}

/**
 * \brief Get T0-counter value
 *
 * \return counter value
 *
 */
static inline uint16_t hw_timer0_get_t0(void)
{
        return (GP_TIMERS->TIMER0_RELOAD_M_REG);
}

/**
 * \brief Get ON-counter value
 *
 * \return counter value
 *
 */
static inline uint16_t hw_timer0_get_on(void)
{
        return (GP_TIMERS->TIMER0_ON_REG);
}

/**
 * \brief Freeze timer
 *
 */
static inline void hw_timer0_freeze(void)
{
        GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SWTIM0_Msk;
}

/**
 * \brief Unfreeze timer
 *
 */
static inline void hw_timer0_unfreeze(void)
{
        GPREG->RESET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SWTIM0_Msk;
}

#endif /* dg_configUSE_HW_TIMER0 */

#endif /* HW_TIMER0_H_ */

/**
 * \}
 * \}
 * \}
 */
