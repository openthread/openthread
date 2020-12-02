/********************************************************************************************************
 * @file	mdec.h
 *
 * @brief	This is the header file for B91
 *
 * @author	Z.W.H
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
#pragma once

#include "analog.h"
#include "reg_include/mdec_reg.h"

/**
 * @brief		This function servers to reset the MDEC module.When the system is wakeup by MDEC, you should
 * 			  	to reset the MDEC module to clear the flag bit of MDEC wakeup.
 * @return		none.
 */
static inline void mdec_reset(void)
{
	analog_write_reg8(mdec_rst_addr,analog_read_reg8(mdec_rst_addr) | FLD_MDEC_RST);
	analog_write_reg8(mdec_rst_addr,analog_read_reg8(mdec_rst_addr) & (~FLD_MDEC_RST));
}

/**
 * @brief		After all packet data are received, it can check whether packet transmission is finished.
 * @param[in]	status	- the interrupt status to be obtained.
 * @return		irq status.
 */
static inline unsigned char mdec_get_irq_status(wakeup_status_e status)
{
	return (analog_read_reg8(reg_wakeup_status) & status);
}

/**
 * @brief		This function serves to clear the wake mdec bit.After all packet
 *				data are received, corresponding flag bit will be set as 1.
 *				needed to manually clear this flag bit so as to avoid misjudgment.
 * @param[in]	status	- the interrupt status that needs to be cleared.
 * @return		none.
 */
static inline void mdec_clr_irq_status(wakeup_status_e status)
{
	analog_write_reg8(reg_wakeup_status, (analog_read_reg8(reg_wakeup_status) | status));
}

/**
 * @brief		This function is used to initialize the MDEC module,include clock setting and input IO select.
 * @param[in]	pin	- mdec pin.
 * 					  In order to distinguish which pin the data is input from,only one input pin can be selected one time.
 * @return		none.
 */
void mdec_init(mdec_pin_e pin);

/**
 * @brief		This function is used to read the receive data of MDEC module's IO.
 * @param[out]	dat		- The array to store date.
 * @return		1 decode success,  0 decode failure.
 */
unsigned char mdec_read_dat(unsigned char *dat);



