/********************************************************************************************************
 * @file     timer.h
 *
 * @brief    This is the header file for TLSR8258
 *
 * @author	 Driver Group
 * @date     May 8, 2018
 *
 * @par      Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *           The information contained herein is confidential property of Telink
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *           Co., Ltd. and the licensee or the terms described here-in. This heading
 *           MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 * @version  A001
 *
 *******************************************************************************************************/
/**	@page TIMER
 *
 *	Introduction
 *	===============
 *	TLSR9518 supports two timers: Timer0~ Timer1. The two timers all support four modes:
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
#include "reg_include/register_9518.h"


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


/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

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
 * @param[in] lev_en
 * @return    none
 */
void timer_gpio_init(timer_type_e type, gpio_pin_e pin, gpio_pol_e pol );


#endif /* TIMER_H_ */
