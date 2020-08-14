/********************************************************************************************************
 * @file     watchdog.h
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

#ifndef WATCHDOG_H_
#define WATCHDOG_H_
#include "analog.h"
#include "gpio.h"

/**
 * @brief     start watchdog. ie enable watchdog
 * @param[in] none
 * @return    none
 */
static inline void wd_start(void){

	BM_SET(reg_tmr_ctrl2, FLD_TMR_WD_EN);
}


/**
 * @brief     stop watchdog. ie disable watchdog
 * @param[in] none
 * @return    none
 */
static inline void wd_stop(void){
	BM_CLR(reg_tmr_ctrl2, FLD_TMR_WD_EN);
}


/**
 * @brief     clear watchdog.
 * @param[in] none
 * @return    none
 */
static inline void wd_clear(void)
{
	reg_tmr_sta = FLD_TMR_STA_WD|FLD_TMR_WD_CNT_CLR;

}

/**
 * @brief     clear watchdog timer tick cnt.
 * @param[in] none
 * @return    none
 */
static inline void wd_clear_cnt(void)
{
	reg_tmr_sta = FLD_TMR_WD_CNT_CLR;

}

/**
 * @brief     This function set the millisecond period.It is likely with WD_SetInterval.
 *            Just this function calculate the value to set the register automatically .
 * @param[in] period_ms - The  millisecond to set. unit is  millisecond
 * @return    none
 * notice: cause 0x14014c constant equals 0x00, period eror=(0x00~0xff)/(APB clock),unit is s.
 */
static inline void wd_set_interval_ms(unsigned int period_ms,unsigned long int tick_per_ms)
{
	static unsigned int tmp_period_ms = 0;
	tmp_period_ms=period_ms*tick_per_ms;
	reg_wt_target=tmp_period_ms;
}
#endif
