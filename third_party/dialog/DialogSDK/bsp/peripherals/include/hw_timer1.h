/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup Timer1
 * \{
 * \brief Timer1
 */

/**
 *****************************************************************************************
 *
 * @file hw_timer1.h
 *
 * @brief Definition of API for the Timer1 Low Level Driver.
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

#ifndef HW_TIMER1_H_
#define HW_TIMER1_H_

#if dg_configUSE_HW_TIMER1

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>


/**
 * \brief Get the mask of a field of an TIMER1 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_TIMER1_REG_FIELD_MASK(reg, field) \
        (TIMER1_CAPTIM_##reg##_REG_##field##_Msk)

/**
 * \brief Get the bit position of a field of an TIMER1 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_TIMER1_REG_FIELD_POS(reg, field) \
        (TIMER1_CAPTIM_##reg##_REG_##field##_Pos)

/**
 * \brief Get the value of a field of an TIMER1 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_TIMER1_REG_GETF(reg, field) \
        ((TIMER1->CAPTIM_##reg##_REG & (TIMER1_CAPTIM_##reg##_REG_##field##_Msk)) >> (TIMER1_CAPTIM_##reg##_REG_##field##_Pos))

/**
 * \brief Set the value of a field of an TIMER1 register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_TIMER1_REG_SETF(reg, field, new_val) \
        TIMER1->CAPTIM_##reg##_REG = ((TIMER1->CAPTIM_##reg##_REG & ~(TIMER1_CAPTIM_##reg##_REG_##field##_Msk)) | \
        ((TIMER1_CAPTIM_##reg##_REG_##field##_Msk) & ((new_val) << (TIMER1_CAPTIM_##reg##_REG_##field##_Pos))))


#define EXT_CLK    16    // 16 MHz
#define INT_CLK    32    // 32 kHz

/**
 * \brief Mode of operation
 *
 * PWM is enabled in both modes.
 *
 */
typedef enum {
        HW_TIMER1_MODE_TIMER = 0,       /**< timer/capture mode */
        HW_TIMER1_MODE_ONESHOT = 1      /**< one-shot mode */
} HW_TIMER1_MODE;

/**
 * \brief Clock source for timer
 *
 */
typedef enum {
        HW_TIMER1_CLK_SRC_INT = 0,      /**< Internal clock 32 kHz (slow) */
        HW_TIMER1_CLK_SRC_EXT = 1       /**< External clock 2/4/8/16 MHz (fast) */
} HW_TIMER1_CLK_SRC;

/**
 * \brief Counting direction
 *
 */
typedef enum {
        HW_TIMER1_DIR_UP = 0,           /**< Timer counts up (counter is incremented) */
        HW_TIMER1_DIR_DOWN = 1          /**< Timer counts down (counter is decremented) */
} HW_TIMER1_DIR;

/**
 * \brief Type of triggering events
 *
 */
typedef enum {
        HW_TIMER1_TRIGGER_RISING = 0,   /**< Event activated rising edge */
        HW_TIMER1_TRIGGER_FALLING = 1   /**< Event activated falling edge */
} HW_TIMER1_TRIGGER;

/**
 * \brief One shot mode phases
 *
 */
typedef enum {
        HW_TIMER1_ONESHOT_WAIT = 0,     /**< Wait for the event */
        HW_TIMER1_ONESHOT_DELAY = 1,    /**< Delay before started */
        HW_TIMER1_ONESHOT_STARTED = 2,  /**< Start shot */
        HW_TIMER1_ONESHOT_ACTIVE = 3    /**< Shot is active */
} HW_TIMER1_ONESHOT;

/**
 * \brief GPIOs for timer1
 *
 * Mainly use for mark which GPIO will triggers timer counting.
 *
 */
typedef enum {
        HW_TIMER1_GPIO_NONE = 0,        /**< None of GPIO */
        /* port 0 */
        HW_TIMER1_GPIO_P00 = 1,         /**< Port 0, pin 0 */
        HW_TIMER1_GPIO_P01 = 2,         /**< Port 0, pin 1 */
        HW_TIMER1_GPIO_P02 = 3,         /**< Port 0, pin 2 */
        HW_TIMER1_GPIO_P03 = 4,         /**< Port 0, pin 3 */
        HW_TIMER1_GPIO_P04 = 5,         /**< Port 0, pin 4 */
        HW_TIMER1_GPIO_P05 = 6,         /**< Port 0, pin 5 */
        HW_TIMER1_GPIO_P06 = 7,         /**< Port 0, pin 6 */
        HW_TIMER1_GPIO_P07 = 8,         /**< Port 0, pin 7 */
        /* port 1 */
        HW_TIMER1_GPIO_P10 = 9,         /**< Port 1, pin 0 */
        HW_TIMER1_GPIO_P11 = 10,        /**< Port 1, pin 1 */
        HW_TIMER1_GPIO_P12 = 11,        /**< Port 1, pin 2 */
        HW_TIMER1_GPIO_P13 = 12,        /**< Port 1, pin 3 */
        HW_TIMER1_GPIO_P14 = 13,        /**< Port 1, pin 4 */
        HW_TIMER1_GPIO_P15 = 14,        /**< Port 1, pin 5 */
        HW_TIMER1_GPIO_P16 = 15,        /**< Port 1, pin 6 */
        HW_TIMER1_GPIO_P17 = 16,        /**< Port 1, pin 7 */
        /* port 2 */
        HW_TIMER1_GPIO_P20 = 17,        /**< Port 2, pin 0 */
        HW_TIMER1_GPIO_P21 = 18,        /**< Port 2, pin 1 */
        HW_TIMER1_GPIO_P22 = 19,        /**< Port 2, pin 2 */
        HW_TIMER1_GPIO_P23 = 20,        /**< Port 2, pin 3 */
        HW_TIMER1_GPIO_P24 = 21,        /**< Port 2, pin 4 */
        /* port 3 */
        HW_TIMER1_GPIO_P30 = 22,        /**< Port 3, pin 0 */
        HW_TIMER1_GPIO_P31 = 23,        /**< Port 3, pin 1 */
        HW_TIMER1_GPIO_P32 = 24,        /**< Port 3, pin 2 */
        HW_TIMER1_GPIO_P33 = 25,        /**< Port 3, pin 3 */
        HW_TIMER1_GPIO_P34 = 26,        /**< Port 3, pin 4 */
        HW_TIMER1_GPIO_P35 = 27,        /**< Port 3, pin 5 */
        HW_TIMER1_GPIO_P36 = 28,        /**< Port 3, pin 6 */
        HW_TIMER1_GPIO_P37 = 29,        /**< Port 3, pin 7 */
        /* port 4 */
        HW_TIMER1_GPIO_P40 = 30,        /**< Port 4, pin 0 */
        HW_TIMER1_GPIO_P41 = 31,        /**< Port 4, pin 1 */
        HW_TIMER1_GPIO_P42 = 32,        /**< Port 4, pin 2 */
        HW_TIMER1_GPIO_P43 = 33,        /**< Port 4, pin 3 */
        HW_TIMER1_GPIO_P44 = 34,        /**< Port 4, pin 4 */
        HW_TIMER1_GPIO_P45 = 35,        /**< Port 4, pin 5 */
        HW_TIMER1_GPIO_P46 = 36,        /**< Port 4, pin 6 */
        HW_TIMER1_GPIO_P47 = 37,        /**< Port 4, pin 7 */
} HW_TIMER1_GPIO;

/**
 * \brief Timer interrupt callback
 *
 */
typedef void (*hw_timer1_handler_cb)(void);

/*
 * \brief Timer configuration for timer/capture mode
 *
 * \sa timer1_config
 * \sa hw_timer1_configure_timer
 *
 */
typedef struct {
        HW_TIMER1_DIR       direction;    /**< counting direction */
        uint32_t            reload_val;   /**< reload value */
        bool                free_run;     /**< free-running mode state */

        HW_TIMER1_GPIO      gpio1;        /**< 1st GPIO for capture mode */
        HW_TIMER1_TRIGGER   trigger1;     /**< 1st GPIO capture trigger */
        HW_TIMER1_GPIO      gpio2;        /**< 2nd GPIO for capture mode */
        HW_TIMER1_TRIGGER   trigger2;     /**< 2nd GPIO capture trigger */
} timer1_config_timer_capture;

/*
 * \brief Timer configuration for oneshot mode
 *
 * \sa timer1_config
 * \sa hw_timer1_configure_oneshot
 *
 */
typedef struct {
        uint16_t            delay;        /**< delay (ticks) between GPIO event and output pulse */
        uint32_t            shot_width;   /**< width (ticks) of generated pulse */
        HW_TIMER1_GPIO      gpio;         /**< GPIO to wait for event */
        HW_TIMER1_TRIGGER   trigger;      /**< GPIO trigger */
} timer1_config_oneshot;

/*
 * \brief Timer PWM configuration
 *
 * \sa timer1_config
 * \sa hw_timer1_configure_pwm
 * \sa hw_timer1_set_pwm_freq
 * \sa hw_timer1_set_pwm_duty_cycle
 */
typedef struct {
        uint16_t            frequency;    /**< frequency */
        uint16_t            duty_cycle;   /**< duty cycle */
} timer1_config_pwm;

/*
 * \brief Timer configuration
 *
 * Only one of \p timer and \p oneshot configuration can be set since they are stored in the same
 * union (and will overwrite each other). Proper configuration structure is selected depending on
 * timer mode set.
 *
 * \sa timer1_config_timer_capture
 * \sa timer1_config_oneshot
 * \sa timer1_config_pwm
 * \sa hw_timer1_configure
 *
 */
typedef struct {
        HW_TIMER1_CLK_SRC   clk_src;                    /**< clock source */
        uint16_t            prescaler;                  /**< clock prescaler */

        union {
                timer1_config_timer_capture   timer;    /**< configuration for timer/capture mode */
                timer1_config_oneshot         oneshot;  /**< configuration for oneshot mode */
        };
        timer1_config_pwm   pwm;                        /**< PWM configuration */
} timer1_config;


#if dg_configUSER_CAN_USE_TIMER1 == 1

/**
 * \brief Timer initialization
 *
 * Turn on clock for timer and configure timer. After initialization both timer and its interrupt
 * are disabled. \p cfg can be NULL - no configuration is performed in such case.
 *
 * \param [in] mode timer mode
 * \param [in] cfg configuration
 *
 */
void hw_timer1_init(HW_TIMER1_MODE mode, const timer1_config *cfg);

/**
 * \brief Timer configuration
 *
 * Shortcut to call appropriate configuration function. If \p cfg is NULL, this function does
 * nothing except switching timer to selected mode.
 *
 * \param [in] mode timer mode
 * \param [in] cfg configuration
 *
 */
void hw_timer1_configure(HW_TIMER1_MODE mode, const timer1_config *cfg);

/**
 * \brief Timer configuration for timer/capture mode
 *
 * Shortcut to call appropriate configuration function. This does not switch timer to timer/capture
 * mode, it should be done separately using hw_timer1_set_mode().
 *
 * \param [in] cfg configuration
 *
 */
void hw_timer1_configure_timer(const timer1_config_timer_capture *cfg);

/**
 * \brief Timer configuration for oneshot mode
 *
 * Shortcut to call appropriate configuration function. This does not switch timer to oneshot
 * mode, it should be done separately using hw_timer1_set_mode().
 *
 * \param [in] cfg configuration
 *
 */
void hw_timer1_configure_oneshot(const timer1_config_oneshot *cfg);

/**
 * \brief Freeze timer
 *
 */
static inline void hw_timer1_freeze(void)
{
        GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SWTIM1_Msk;
}

/**
 * \brief Unfreeze timer
 *
 */
static inline void hw_timer1_unfreeze(void)
{
        GPREG->RESET_FREEZE_REG = GPREG_RESET_FREEZE_REG_FRZ_SWTIM1_Msk;
}

/**
 * \brief Check if timer is frozen
 *
 * \return true if it is frozen else false
 *
 */
static inline bool hw_timer1_frozen(void)
{
        return GPREG->SET_FREEZE_REG & GPREG_SET_FREEZE_REG_FRZ_SWTIM1_Msk ? true : false;
}

/**
 * \brief Set clock source of the timer
 *
 * \param [in] clk clock source of the timer, external or internal
 *
 */
static inline void hw_timer1_set_clk(HW_TIMER1_CLK_SRC clk)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_SYS_CLK_EN, clk);
}

/**
 * \brief Set timer clock prescaler
 *
 * Actual timer frequency is \p timer_freq = \p freq_clock / (\p value + 1)
 *
 * \param [in] value prescaler
 *
 */
static inline void hw_timer1_set_prescaler(uint16_t value)
{
        TIMER1->CAPTIM_PRESCALER_REG = value;
}

/**
 * \brief Set timer reload value
 *
 * \note This changes the same register value as hw_timer1_set_oneshot_delay() since both parameters
 * share the same register (value is interpreted differently depending on timer mode).
 *
 * \param [in] value reload value
 *
 * \sa hw_timer1_set_oneshot_delay
 * \note For DA14682/3 chips, setting the reload value will also freeze the timer until both
 * reload registers are set.
 */
static inline void hw_timer1_set_reload(uint32_t value)
{
#if (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A) && (dg_configUSE_AUTO_CHIP_DETECTION != 1)
        bool timer1_frozen = hw_timer1_frozen();
        if (!timer1_frozen) {
                hw_timer1_freeze();
        }
        TIMER1->CAPTIM_RELOAD_REG = (uint16_t) value;
        TIMER1->CAPTIM_RELOAD_HIGH_REG = (uint16_t) value >> 16;
        if (!timer1_frozen) {
                hw_timer1_unfreeze();
        }
#else
        TIMER1->CAPTIM_RELOAD_REG = (uint16_t) value;
#endif
}

/**
 * \brief Set pulse delay in oneshot mode
 *
 * \note This changes the same register value as hw_timer1_set_reload() since both parameters share
 * the same register (value is interpreted differently depending on timer mode).
 *
 * \param [in] delay delay (ticks)
 *
 * \sa hw_timer1_set_reload
 *
 */
static inline void hw_timer1_set_oneshot_delay(uint32_t delay)
{
        TIMER1->CAPTIM_RELOAD_REG = (uint16_t) delay;
}

/**
 * \brief Set shot width
 *
 * This applies only to one-shot mode.
 *
 * \param [in] duration shot phase duration
 *
 */
static inline void hw_timer1_set_shot_width(uint32_t duration)
{
        TIMER1->CAPTIM_SHOTWIDTH_REG = (uint16_t) duration;
}

/**
 * \brief Turn on free run mode of the timer
 *
 */
static inline void hw_timer1_set_freerun(bool enabled)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_FREE_RUN_MODE_EN, enabled);
}

/**
 * \brief Set a type of the edge which triggers event1
 *
 * \param [in] edge type of edge, rising or falling
 *
 */
static inline void hw_timer1_set_event1_trigger(HW_TIMER1_TRIGGER edge)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_IN1_EVENT_FALL_EN, edge);
}

/**
 * \brief Set a type of the edge which triggers event2
 *
 * \param [in] edge type of edge, rising or falling
 *
 */
static inline void hw_timer1_set_event2_trigger(HW_TIMER1_TRIGGER edge)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_IN2_EVENT_FALL_EN, edge);
}

/**
 * \brief Set a GPIO input which triggers event1
 *
 * \param [in] gpio GPIO input
 *
 */
static inline void hw_timer1_set_event1_gpio(HW_TIMER1_GPIO gpio)
{
        TIMER1->CAPTIM_GPIO1_CONF_REG = gpio;
}

/**
 * \brief Set a GPIO input which triggers event2
 *
 * \param [in] gpio GPIO input
 *
 */
static inline void hw_timer1_set_event2_gpio(HW_TIMER1_GPIO gpio)
{
        TIMER1->CAPTIM_GPIO2_CONF_REG = gpio;
}

/**
 * \brief Get clock source of the timer
 *
 * \return clock source
 *
 */
static inline HW_TIMER1_CLK_SRC hw_timer1_get_clk(void)
{
        return HW_TIMER1_REG_GETF(CTRL, CAPTIM_SYS_CLK_EN);
}

/**
 * \brief Get timer clock prescaler
 *
 * Actual timer frequency is \p timer_freq = \p freq_clock / (\p retval + 1)
 *
 * \return prescaler value
 *
 * \sa hw_timer1_get_prescaler_val
 *
 */
static inline uint16_t hw_timer1_get_prescaler(void)
{
        return TIMER1->CAPTIM_PRESCALER_REG;
}

/**
 * \brief Get timer reload value
 *
 * \return reload value
 *
 * \sa hw_timer1_set_reload
 *
 */
static inline uint32_t hw_timer1_get_reload(void)
{
        return TIMER1->CAPTIM_RELOAD_REG;
}

/**
 * \brief Get pulse delay in oneshot mode
 *
 * \return delay (ticks)
 *
 * \sa hw_timer1_get_oneshot_delay
 *
 */
static inline uint32_t hw_timer1_get_oneshot_delay(void)
{
        return TIMER1->CAPTIM_RELOAD_REG;
}

/**
 * \brief Get shot width
 *
 * This applies only to one-shot mode.
 *
 * \return shot width value
 *
 */
static inline uint32_t hw_timer1_get_shot_width(void)
{
        return TIMER1->CAPTIM_SHOTWIDTH_REG;
}

/**
 * \brief Get free-running mode state
 *
 * \return free-running mode state
 *
 */
static inline bool hw_timer1_get_freerun(void)
{
        return HW_TIMER1_REG_GETF(CTRL, CAPTIM_FREE_RUN_MODE_EN);
}

/**
 * \brief Get a type of the edge which triggers event1
 *
 * \return edge type
 *
 */
static inline HW_TIMER1_TRIGGER hw_timer1_get_event1_trigger(void)
{
        return HW_TIMER1_REG_GETF(CTRL, CAPTIM_IN1_EVENT_FALL_EN);
}

/**
 * \brief Get a type of the edge which triggers event2
 *
 * \return edge type
 *
 */
static inline HW_TIMER1_TRIGGER hw_timer1_get_event2_trigger(void)
{
        return HW_TIMER1_REG_GETF(CTRL, CAPTIM_IN2_EVENT_FALL_EN);
}

/**
 * \brief Get a GPIO input which triggers event1
 *
 * \return GPIO input
 *
 */
static inline HW_TIMER1_GPIO hw_timer1_get_event1_gpio(void)
{
        return TIMER1->CAPTIM_GPIO1_CONF_REG;
}

/**
 * \brief Get a GPIO input which triggers event2
 *
 * \return GPIO input
 *
 */
static inline HW_TIMER1_GPIO hw_timer1_get_event2_gpio(void)
{
        return TIMER1->CAPTIM_GPIO2_CONF_REG;
}

/**
 * \brief Get the capture time for event on GPIO1
 *
 * \return time for event on GPIO1
 *
 */
static inline uint32_t hw_timer1_get_capture1(void)
{
        return TIMER1->CAPTIM_CAPTURE_GPIO1_REG;
}

/**
 * \brief Get the capture time for event on GPIO2
 *
 * \return time for event on GPIO2
 *
 */
static inline uint32_t hw_timer1_get_capture2(void)
{
        return TIMER1->CAPTIM_CAPTURE_GPIO2_REG;
}

/**
 * \brief Set the direction of timer counting
 *
 * \param [in] dir counting direction of the timer, up or down
 *
 */
static inline void hw_timer1_set_direction(HW_TIMER1_DIR dir)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_COUNT_DOWN_EN, dir);
}

/**
 * \brief Turn on one shot mode
 *
 */
static inline void hw_timer1_set_mode(HW_TIMER1_MODE mode)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_ONESHOT_MODE_EN, mode);
}

/**
 * \brief Turn on capture mode
 *
 */
static inline HW_TIMER1_MODE hw_timer1_get_mode(void)
{
        return HW_TIMER1_REG_GETF(CTRL, CAPTIM_ONESHOT_MODE_EN);
}

/**
 * \brief Get the tick count of the timer
 *
 * \return current value of the timer ticks
 *
 * \sa hw_timer1_get_prescaler_val
 *
 */
static inline uint32_t hw_timer1_get_count(void)
{
#if (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A) && (dg_configUSE_AUTO_CHIP_DETECTION != 1)
        uint32_t lo, hi;
        GLOBAL_INT_DISABLE();
        do {
                lo = TIMER1->CAPTIM_TIMER_VAL_REG;
                hi = TIMER1->CAPTIM_TIMER_HVAL_REG;
        } while (lo != TIMER1->CAPTIM_TIMER_VAL_REG);
        GLOBAL_INT_RESTORE();
        return (hi << 16) | lo;
#else
        return TIMER1->CAPTIM_TIMER_VAL_REG;
#endif
}

/**
 * \brief Get the current phase of the one shot mode
 *
 * \return current phase of the one shot mode
 *
 */
static inline HW_TIMER1_ONESHOT hw_timer1_get_oneshot_phase(void)
{
        return HW_TIMER1_REG_GETF(STATUS, CAPTIM_ONESHOT_PHASE);
}

/**
 * \brief Get the current state of IN1
 *
 * \return current logic level of IN1
 *
 */
static inline bool hw_timer1_get_gpio1_state(void)
{
        return HW_TIMER1_REG_GETF(STATUS, CAPTIM_IN1_STATE);
}

/**
 * \brief Get the current state of IN2
 *
 * \return current logic level of IN2
 *
 */
static inline bool hw_timer1_get_gpio2_state(void)
{
        return HW_TIMER1_REG_GETF(STATUS, CAPTIM_IN2_STATE);
}

/**
 * \brief Get the current prescaler counter value
 *
 * This is value of internal counter used for prescaling. It can be used to have finer granularity
 * when reading timer value.
 *
 * For reading current setting of prescaler, see hw_timer1_get_prescaler().
 *
 * \return current prescaler counter value
 *
 * \sa hw_timer1_get_count
 * \sa hw_timer1_get_prescaler
 *
 */
static inline uint16_t hw_timer1_get_prescaler_val(void)
{
        return TIMER1->CAPTIM_PRESCALER_VAL_REG;
}

/**
 * \brief Register an interrupt handler.
 *
 * \param [in] handler function pointer to handler to call when an interrupt occurs
 *
 */
void hw_timer1_register_int(hw_timer1_handler_cb handler);

/**
 * \brief Unregister an interrupt handler
 *
 */
void hw_timer1_unregister_int(void);

#else // (dg_configUSER_CAN_USE_TIMER1 == 0)

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#define LP_CNT_REG_RANGE        ( 16 )
#else
#define LP_CNT_REG_RANGE        ( 32 )
#endif
#define LP_CNT_MAX_VALUE        ( (1ULL << LP_CNT_REG_RANGE) - 1 )
#define LP_CNT_PRESCALED_MASK   ( LP_CNT_MAX_VALUE )
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#define LP_CNT_NATIVE_MASK      ( (1ULL << (LP_CNT_REG_RANGE + dg_configTim1PrescalerBitRange)) - 1 )
#else
#define LP_CNT_NATIVE_MASK      ( (1ULL << (LP_CNT_REG_RANGE)) - 1 )
#endif

/**
 * \brief Configure timer as tick timer
 *
 * \details Setup Timer1 for use as a tick timer:
 *         <ul>
 *              <li> use LP clock and prescaler (if configured)
 *              <li> free running mode
 *              <li> IRQ masked
 *              <li> up counter
 *              <li> timer mode
 *              <li> disabled
 *         </ul>
 *
 */
void hw_timer1_lp_clk_init(void);


/**
 * \brief Enable interrupt
 *
 * \return void
 *
 */
static inline void hw_timer1_int_enable(void) __attribute__((always_inline));

static inline void hw_timer1_int_enable(void)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_IRQ_EN, 1);
        NVIC_ClearPendingIRQ(SWTIM1_IRQn);
        NVIC_EnableIRQ(SWTIM1_IRQn);
}


/**
 * \brief Disable interrupt
 *
 * \return void
 *
 */
static inline void hw_timer1_int_disable(void) __attribute__((always_inline));

static inline void hw_timer1_int_disable(void)
{
        NVIC_DisableIRQ(SWTIM1_IRQn);
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_IRQ_EN, 0);
}


/**
 * \brief Get counter's value
 *
 * \return The current value of the counter (prescaled).
 *
 */
static inline uint32_t hw_timer1_get_value(void) __attribute__((always_inline));

static inline uint32_t hw_timer1_get_value(void)
{
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        return TIMER1->CAPTIM_TIMER_VAL_REG;
#else
        uint32_t lo, hi;
        GLOBAL_INT_DISABLE();
        do {
                lo = TIMER1->CAPTIM_TIMER_VAL_REG;
                hi = TIMER1->CAPTIM_TIMER_HVAL_REG;
        } while (lo != TIMER1->CAPTIM_TIMER_VAL_REG);
        GLOBAL_INT_RESTORE();
        return (hi << 16) | lo;
#endif
}


/**
 * \brief Macro to get the current value of the timer, both prescaled and in LP clock cycles.
 *
 * \details Special care is needed when a prescaler is used. In this case, the
 *         counting sequence is the following (assuminga prescaler of 3):
 *         prescaler: 0  1  2  3  0  1  2  3  0
 *         counter:   0           1           2
 *
 *         If not implemented properly then the following may happen:
 *         - let's assume that we are at the end of the {0, 3} period
 *         - the counter's value is read and the result is 0
 *         - at that moment, the counter increases it's prescaler value (and the counter's value
 *           respectively), so the time becomes {1, 0}
 *         - the reading of the prescaler's value will give 0 but {0, 0} is earlier than {0, 3} (when
 *           the operation started) and this may result in errors in time computations.
 * \note This macro should be used inside a critical section.
 *
 */
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#define HW_TIMER1_GET_INSTANT(prescaled, fine)                                                  \
do {                                                                                            \
        if (dg_configTim1Prescaler != 0) {                                                      \
                uint32_t prescaler_v;                                                           \
                                                                                                \
                do {                                                                            \
                        prescaler_v = TIMER1->CAPTIM_PRESCALER_VAL_REG;                         \
                        prescaled = TIMER1->CAPTIM_TIMER_VAL_REG;                               \
                } while (prescaler_v != TIMER1->CAPTIM_PRESCALER_VAL_REG);                      \
                                                                                                \
                fine = ( (uint32_t)prescaled * (1 + dg_configTim1Prescaler) + prescaler_v);     \
        }                                                                                       \
        else {                                                                                  \
                prescaled = TIMER1->CAPTIM_TIMER_VAL_REG;                                       \
                fine = prescaled;                                                               \
        }                                                                                       \
} while (0)
#else
#define HW_TIMER1_GET_INSTANT(prescaled, fine)                                                  \
do {                                                                                            \
        uint32_t lo, hi;                                                                        \
        do {                                                                                    \
                lo = TIMER1->CAPTIM_TIMER_VAL_REG;                                              \
                hi = TIMER1->CAPTIM_TIMER_HVAL_REG;                                             \
        } while (lo != TIMER1->CAPTIM_TIMER_VAL_REG);                                           \
        prescaled = fine = (hi << 16) | lo;                                                     \
} while (0)
#endif
/**
 * \brief Macro to set the trigger value. The previous trigger value is returned to the caller.
 * \note For DA14682/3 chips, this macro will disable timer1 interrupt until reload registers
 * programmed so that false interrupts are avoided.
 */
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#define HW_TIMER1_SET_TRIGGER(value, last_value)        \
do {                                                    \
        last_value = TIMER1->CAPTIM_RELOAD_REG;         \
        TIMER1->CAPTIM_RELOAD_REG = value;              \
} while (0)
#else
#define HW_TIMER1_SET_TRIGGER(value, last_value)                                                   \
do {                                                                                               \
        bool timer1_irq_en =                                                                       \
                TIMER1->CAPTIM_CTRL_REG & TIMER1_CAPTIM_CTRL_REG_CAPTIM_IRQ_EN_Msk ? true : false; \
        last_value = TIMER1->CAPTIM_RELOAD_REG |                                                   \
                ((uint32_t) TIMER1->CAPTIM_RELOAD_HIGH_REG) << 16;                                 \
        if (timer1_irq_en) {                                                                       \
                TIMER1->CAPTIM_CTRL_REG &= ~TIMER1_CAPTIM_CTRL_REG_CAPTIM_IRQ_EN_Msk;              \
        }                                                                                          \
        TIMER1->CAPTIM_RELOAD_REG = (uint16_t) value;                                              \
        TIMER1->CAPTIM_RELOAD_HIGH_REG = (uint16_t) ((value) >> 16);                               \
        if (timer1_irq_en) {                                                                       \
                TIMER1->CAPTIM_CTRL_REG |= TIMER1_CAPTIM_CTRL_REG_CAPTIM_IRQ_EN_Msk;               \
        }                                                                                          \
} while (0)
#endif

/**
 * \brief Get trigger value
 *
 * \return The current value of the trigger (prescaled).
 *
 */
static inline uint32_t hw_timer1_get_trigger(void)
{
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        return TIMER1->CAPTIM_RELOAD_REG;
#else
        return TIMER1->CAPTIM_RELOAD_REG | ((uint32_t) TIMER1->CAPTIM_RELOAD_HIGH_REG) << 16;
#endif
}

/**
 * \brief Set an "invalid" trigger value, which refers far away in the future
 *
 */
static inline void hw_timer1_invalidate_trigger(void)  __attribute__((always_inline));

static inline void hw_timer1_invalidate_trigger(void)
{
        uint32_t lp_current_time;                       // prescaled
        uint32_t trigger;
        uint32_t dummy __attribute__((unused));

        lp_current_time = hw_timer1_get_value();        // Get current time
        trigger = (lp_current_time - 1) & LP_CNT_PRESCALED_MASK;  // Get an "invalid" trigger
        HW_TIMER1_SET_TRIGGER(trigger, dummy);          // Move trigger too far in the future so
                                                        // that no interrupt from the timer arrives.
}

#endif // dg_configUSER_CAN_USE_TIMER1


/**
 * \brief Enable the timer
 *
 */
static inline void hw_timer1_enable(void)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_EN, 1);
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_TMR_REG, TMR1_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable the timer
 *
 */
static inline void hw_timer1_disable(void)
{
        HW_TIMER1_REG_SETF(CTRL, CAPTIM_EN, 0);
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_TMR_REG, TMR1_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Timer PWM configuration
 *
 * Shortcut to call appropriate configuration function.
 *
 * \param [in] cfg configuration
 *
 */
void hw_timer1_configure_pwm(const timer1_config_pwm *cfg);

/**
 * \brief Set PWM frequency prescaler
 *
 * Actual PWM frequency is \p pwm_freq = \p timer_freq / (\p value + 1)
 *
 * \param [in] value PWM frequency defined as above
 *
 * \sa hw_timer1_set_prescaler
 *
 */
static inline void hw_timer1_set_pwm_freq(uint16_t value)
{
        TIMER1->CAPTIM_PWM_FREQ_REG = value;
}

/**
 * \brief Set PWM duty cycle
 *
 * Actualy PWM duty cycle is \p pwm_dc = \p value / (\p pwm_freq + 1)
 *
 * \param [in] value PWM duty cycle defined as above
 *
 * \sa hw_timer1_set_pwm_freq
 *
 */
static inline void hw_timer1_set_pwm_duty_cycle(uint16_t value)
{
        TIMER1->CAPTIM_PWM_DC_REG = value;
}

/**
 * \brief Get PWM frequency prescaler
 *
 * Actual PWM frequency is \p pwm_freq = \p timer_freq / (\p retval + 1)
 *
 * \return PWM frequency as defined above
 *
 */
static inline uint16_t hw_timer1_get_pwm_freq(void)
{
        return TIMER1->CAPTIM_PWM_FREQ_REG;
}

/**
 * \brief Get PWM duty cycle
 *
 * Actualy PWM duty cycle is \p pwm_dc = \p retval / (\p pwm_freq + 1)
 *
 * \return PWM duty cycle as defined above
 *
 */
static inline uint16_t hw_timer1_get_pwm_duty_cycle(void)
{
        return TIMER1->CAPTIM_PWM_DC_REG;
}

#endif /* dg_configUSE_HW_TIMER1 */

#endif /* HW_TIMER1_H_ */

/**
 * \}
 * \}
 * \}
 */
