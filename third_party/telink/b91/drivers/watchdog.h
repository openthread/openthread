/********************************************************************************************************
 * @file	watchdog.h
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
#ifndef WATCHDOG_H_
#define WATCHDOG_H_
#include "analog.h"
#include "gpio.h"

/**
 * @brief     start watchdog.
 * @return    none
 */
static inline void wd_start(void){

	BM_SET(reg_tmr_ctrl2, FLD_TMR_WD_EN);
}


/**
 * @brief     stop watchdog. 
 * @return    none
 */
static inline void wd_stop(void){
	BM_CLR(reg_tmr_ctrl2, FLD_TMR_WD_EN);
}


/**
 * @brief     clear watchdog.
 * @return    none
 */
static inline void wd_clear(void)
{
	reg_tmr_sta = FLD_TMR_STA_WD|FLD_TMR_WD_CNT_CLR;

}

/**
 * @brief     clear watchdog timer tick cnt.
 * @return    none
 */
static inline void wd_clear_cnt(void)
{
	reg_tmr_sta = FLD_TMR_WD_CNT_CLR;

}

/**
 * @brief     This function set the watchdog trigger time.
 * 			  Because the lower 8bit of the wd timer register will always be 0, there will be an error ,
			  The time error = (0x00~0xff)/(APB clock frequency)
 * @param[in] period_ms - The watchdog trigger time. Unit is  millisecond
 * @param[in] tick_per_ms - Number of tick in 1 millisecond;
 * @return    none
 */
static inline void wd_set_interval_ms(unsigned int period_ms,unsigned long int tick_per_ms)
{
	static unsigned int tmp_period_ms = 0;
	tmp_period_ms=period_ms*tick_per_ms;
	reg_wt_target=tmp_period_ms;
}

#endif
