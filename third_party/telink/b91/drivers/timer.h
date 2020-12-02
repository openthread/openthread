/********************************************************************************************************
 * @file	timer.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H
 * @date	2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *          
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *          
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *          
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions 
 *              in binary form must reproduce the above copyright notice, this list of 
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *          
 *              3. Neither the name of TELINK, nor the names of its contributors may be 
 *              used to endorse or promote products derived from this software without 
 *              specific prior written permission.
 *          
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or 
 *              relating to such deletion(s), modification(s) or alteration(s).
 *         
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *         
 *******************************************************************************************************/
/**	@page TIMER
 *
 *	Introduction
 *	===============
 *	B91 supports two timers: Timer0~ Timer1. The two timers all support four modes:
 *		- Mode 0 (System Clock Mode),
 *		- Mode 1 (GPIO Trigger Mode),
 *		- Mode 2 (GPIO Pulse Width Mode),
 *		- Mode 3 (Tick Mode),
 *
 *	Timer 1 can also be configured as "watchdog" to monitor firmware running.
 *
 *	API Reference
 *	===============
 *	Header File: timer.h
 */
#ifndef TIMER_H_
#define TIMER_H_

#include "analog.h"
#include "gpio.h"
#include "reg_include/register_b91.h"


/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/
/**
 *  @brief  Define system tick
 */
#define	    tl_sys_tick_per_us   				16

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**
 * @brief   Type of Timer
 */
typedef enum{
	TIMER0		=0,
	TIMER1		=1,
}timer_type_e;


/**
 * @brief   Mode of Timer
 */
typedef enum{
	TIMER_MODE_SYSCLK		=0,
	TIMER_MODE_GPIO_TRIGGER	=1,
	TIMER_MODE_GPIO_WIDTH	=2,
	TIMER_MODE_TICK			=3,
}timer_mode_e;

typedef enum{
	TMR_STA_TMR0   =		BIT(0),
    TMR_STA_TMR1   =		BIT(1),
}time_irq_e;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/*
 * @brief    This function refer to get timer irq status.
 * @param[in] status - variable of enum to select the timer interrupt source.
 * @return   the status of timer0/timer1.
 */
static inline unsigned char timer_get_irq_status(time_irq_e status)
{
    return  reg_tmr_sta&status ;
}

/*
 * @brief     This function refer to clr timer0 irq status.
 * @param[in] status - variable of enum to select the timerinterrupt source.
 * @return    none
 */
static inline void timer_clr_irq_status(time_irq_e status)
{
		reg_tmr_sta= status;
}


/*
 * @brief   This function refer to get timer0 tick.
 * @return  none
 */
static inline  u32 timer0_get_gpio_width(void)
{
	 return reg_tmr0_tick;

}


/*
 * @brief   This function refer to get timer1 tick.
 * @return  none
 */
static inline u32 timer1_get_gpio_width(void)
{
	return reg_tmr1_tick;

}


/*
 * @brief   This function refer to set timer0 tick .
 * @param[in] tick - the tick of timer0
 * @return  none
 */
static inline void timer0_set_tick(u32 tick)
{
	reg_tmr0_tick = tick;
}

/*
 * @brief   This function refer to get timer0 tick.
 * @return  none
 */
static inline u32 timer0_get_tick(void)
{
	return reg_tmr0_tick ;
}


/*
 * @brief   This function refer to set timer1 tick.
 * @param[in] tick - the tick of timer1
 * @return  none
 */
static inline void timer1_set_tick(u32 tick)
{
	reg_tmr1_tick = tick;
}

/*
 * @brief   This function refer to get timer1 tick.
 * @return  none
 */
static inline u32 timer1_get_tick(void)
{
	return reg_tmr1_tick;
}


/**
 * @brief     the specifed timer start working.
 * @param[in] type - select the timer to start.
 * @return    none
 */
void timer_start(timer_type_e type);

/**
 * @brief     set mode, initial tick and capture of timer.
 * @param[in] type - select the timer to start.
 * @param[in] mode - select mode for timer.
 * @param[in] init_tick - initial tick.
 * @param[in] cap_tick  - tick of capture.
 * @return    none
 */
void timer_set_mode(timer_type_e type, timer_mode_e mode,unsigned int init_tick, unsigned int cap_tick);

/**
 * @brief     initiate GPIO for gpio trigger and gpio width mode of timer.
 * @param[in] type - select the timer to start.
 * @param[in] pin - select pin for timer.
 * @param[in] pol - select polarity for gpio trigger and gpio width
 * @return    none
 */
void timer_gpio_init(timer_type_e type, gpio_pin_e pin, gpio_pol_e pol );



/**
 * @brief     the specifed timer stop working.
 * @param[in] type - select the timer to stop.
 * @return    none
 */
void timer_stop(timer_type_e type);



#endif /* TIMER_H_ */
