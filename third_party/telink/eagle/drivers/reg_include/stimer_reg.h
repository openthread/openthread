/********************************************************************************************************
 * @file	stimer_reg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	B.Y
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
#ifndef STIMER_REG_H_
#define STIMER_REG_H_

/*******************************      sys clock registers: 0x140200       ******************************/
#define STIMER_BASE_ADDR			   	0x140200
#define reg_system_tick         		REG_ADDR32(STIMER_BASE_ADDR)

#define reg_system_irq_level         	REG_ADDR32(STIMER_BASE_ADDR+0x4)

#define reg_system_irq_mask				REG_ADDR8(STIMER_BASE_ADDR+0x8)
enum{
	FLD_SYSTEM_IRQ_MASK 	= 	BIT_RNG(0,2),
	FLD_SYSTEM_TRIG_PAST_EN = 	BIT(3),
};

#define reg_system_cal_irq		REG_ADDR8(STIMER_BASE_ADDR+0x9)

typedef enum{
	FLD_SYSTEM_IRQ  		= 	BIT(0),
	FLD_SYSTEM_32K_IRQ  	= 	BIT(1),
}stimer_irq_e;

#define reg_system_ctrl		    REG_ADDR8(STIMER_BASE_ADDR+0xa)
enum{
	FLD_SYSTEM_32K_WR_EN 		= 	BIT(0),
	FLD_SYSTEM_TIMER_EN 	    = 	BIT(1),
	FLD_SYSTEM_TIMER_AUTO 	    = 	BIT(2),
	FLD_SYSTEM_32K_CAL_EN 		= 	BIT(3),
	FLD_SYSTEM_32K_CAL_MODE 	= 	BIT_RNG(4,7),

};

#define reg_system_st		    REG_ADDR8(STIMER_BASE_ADDR+0xb)

enum{

	FLD_SYSTEM_CMD_STOP 			=   BIT(1),
	FLD_SYSTEM_CMD_SYNC		        =   BIT(3),
	FLD_SYSTEM_CLK_32K		        =   BIT(4),
	FLD_SYSTEM_CLR_RD_DONE			=   BIT(5),
	FLD_SYSTEM_RD_BUSY			    =   BIT(6),
	FLD_SYSTEM_CMD_SET_DLY_DONE	    =   BIT(7),

};

#define reg_system_timer_set_32k         	REG_ADDR32(STIMER_BASE_ADDR+0xc)

#define reg_system_timer_read_32k         	REG_ADDR32(STIMER_BASE_ADDR+0x10)

#define reg_system_cal_latch_32k         	REG_ADDR32(STIMER_BASE_ADDR+0x14)

#define reg_system_up_32k					REG_ADDR32(STIMER_BASE_ADDR+0x18)
enum{

	FLD_SYSTEM_UPDATE_UPON_32K 			=   BIT(0),
	FLD_SYSTEM_RUN_UPON_NXT_32K		    =   BIT(1),
};

#endif /* STIMER_REG_H_ */
