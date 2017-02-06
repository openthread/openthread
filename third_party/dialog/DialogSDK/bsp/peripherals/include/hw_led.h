/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup LED
 * \{
 * \brief LED Controller
 */

/**
 *****************************************************************************************
 *
 * @file hw_led.h
 *
 * @brief Definition of API for the LED Low Level Driver.
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

#ifndef HW_LED_H_
#define HW_LED_H_

#include <sdk_defs.h>

/**
 * \brief Source for LED1
 */
typedef enum {
        HW_LED_SRC1_PWM2   = 0,      /**< LED1 controlled by PWM2 */
        HW_LED_SRC1_BREATH = 1       /**< LED1 controlled by Breath Timer */
} HW_LED_SRC1;

/**
 * \brief Source for LED2
 */
typedef enum {
        HW_LED_SRC2_PWM3   = 0,      /**< LED2 controlled by PWM3 */
        HW_LED_SRC2_BREATH = 1       /**< LED2 controlled by Breath Timer */
} HW_LED_SRC2;

/**
 * \brief Source for LED3
 */
typedef enum {
        HW_LED_SRC3_PWM4   = 0,      /**< LED3 controlled by PWM4 */
        HW_LED_SRC3_BREATH = 1       /**< LED3 controlled by Breath Timer */
} HW_LED_SRC3;


/**
 * \brief Set the source that controls LED1
 *
 * \param [in] src the source that controls LED1
 *
 * \sa HW_LED_SRC1
 */
__STATIC_INLINE void hw_led_set_led1_src(HW_LED_SRC1 src)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED1_SRC_SEL, src);
}

/**
 * \brief Set the source that controls LED2
 *
 * \param [in] src the source that controls LED2
 *
 * \sa HW_LED_SRC2
 */
__STATIC_INLINE void hw_led_set_led2_src(HW_LED_SRC2 src)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED2_SRC_SEL, src);
}

/**
 * \brief Set the source that controls LED3
 *
 * \param [in] src the source that controls LED3
 *
 * \sa HW_LED_SRC3
 */
__STATIC_INLINE void hw_led_set_led3_src(HW_LED_SRC3 src)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED3_SRC_SEL, src);
}

/**
 * \brief Enable or disable LED1
 *
 * \param [in] state true to enable LED1, or false to disable it
 *
 */
__STATIC_INLINE void hw_led_enable_led1(bool state)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED1_EN, state);
}

/**
 * \brief Enable or disable LED2
 *
 * \param [in] state true to enable LED2, or false to disable it
 *
 */
__STATIC_INLINE void hw_led_enable_led2(bool state)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED2_EN, state);
}

/**
 * \brief Enable or disable LED3
 *
 * \param [in] state true to enable LED3, or false to disable it
 *
 */
__STATIC_INLINE void hw_led_enable_led3(bool state)
{
        REG_SETF(GPREG, LED_CONTROL_REG, LED3_EN, state);
}


#endif /* HW_LED_H_ */

/**
 * \}
 * \}
 * \}
 */
