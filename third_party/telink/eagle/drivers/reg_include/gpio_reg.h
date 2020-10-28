/********************************************************************************************************
 * @file	gpio_reg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H / X.P.C
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
#ifndef GPIO_REG_H_
#define GPIO_REG_H_
#include "../sys.h"
#include "../../common/bit.h"
/*******************************      gpio registers: 0x140300      ******************************/
//PA
#define reg_gpio_pa_setting1	REG_ADDR32(0x140300)
#define reg_gpio_pa_in			REG_ADDR8(0x140300)
#define reg_gpio_pa_ie			REG_ADDR8(0x140301)
#define reg_gpio_pa_oen			REG_ADDR8(0x140302)
#define reg_gpio_pa_out			REG_ADDR8(0x140303)

#define reg_gpio_pa_setting2	REG_ADDR32(0x140304)
#define reg_gpio_pa_pol			REG_ADDR8(0x140304)
#define reg_gpio_pa_ds			REG_ADDR8(0x140305)
#define reg_gpio_pa_gpio		REG_ADDR8(0x140306)
#define reg_gpio_pa_irq_en		REG_ADDR8(0x140307)

#define reg_gpio_pa_fs			REG_ADDR16(0x140330)
#define reg_gpio_pa_fuc_l		REG_ADDR8(0x140330)
#define reg_gpio_pa_fuc_h		REG_ADDR8(0x140331)

//PB
#define reg_gpio_pb_setting1	REG_ADDR32(0x140308)
#define reg_gpio_pb_in			REG_ADDR8(0x140308)
#define reg_gpio_pb_ie			REG_ADDR8(0x140309)
#define reg_gpio_pb_oen			REG_ADDR8(0x14030a)
#define reg_gpio_pb_out			REG_ADDR8(0x14030b)

#define reg_gpio_pb_setting2	REG_ADDR32(0x14030c)
#define reg_gpio_pb_pol			REG_ADDR8(0x14030c)
#define reg_gpio_pb_ds			REG_ADDR8(0x14030d)
#define reg_gpio_pb_gpio		REG_ADDR8(0x14030e)
#define reg_gpio_pb_irq_en		REG_ADDR8(0x14030f)

#define reg_gpio_pb_fs			REG_ADDR16(0x140332)
#define reg_gpio_pb_fuc_l		REG_ADDR8(0x140332)
#define reg_gpio_pb_fuc_h		REG_ADDR8(0x140333)

//PC
#define reg_gpio_pc_setting1	REG_ADDR32(0x140310)
#define reg_gpio_pc_in			REG_ADDR8(0x140310)
#define areg_gpio_pc_ie			0xbd
#define reg_gpio_pc_oen			REG_ADDR8(0x140312)
#define reg_gpio_pc_out			REG_ADDR8(0x140313)

#define reg_gpio_pc_setting2	REG_ADDR32(0x140314)
#define reg_gpio_pc_pol			REG_ADDR8(0x140314)
#define areg_gpio_pc_ds			0xbf
#define reg_gpio_pc_gpio		REG_ADDR8(0x140316)
#define reg_gpio_pc_irq_en		REG_ADDR8(0x140317)

#define reg_gpio_pc_fs			REG_ADDR16(0x140334)
#define reg_gpio_pc_fuc_l		REG_ADDR8(0x140334)
#define reg_gpio_pc_fuc_h		REG_ADDR8(0x140335)

//PD
#define reg_gpio_pd_setting1	REG_ADDR32(0x140318)
#define reg_gpio_pd_in			REG_ADDR8(0x140318)
#define areg_gpio_pd_ie			0xc0
#define reg_gpio_pd_oen			REG_ADDR8(0x14031a)
#define reg_gpio_pd_out			REG_ADDR8(0x14031b)

#define reg_gpio_pd_setting2	REG_ADDR32(0x14031c)
#define reg_gpio_pd_pol			REG_ADDR8(0x14031c)
#define areg_gpio_pd_ds			0xc2
#define reg_gpio_pd_gpio		REG_ADDR8(0x14031e)
#define reg_gpio_pd_irq_en		REG_ADDR8(0x14031f)

#define reg_gpio_pd_fs			REG_ADDR16(0x140336)
#define reg_gpio_pd_fuc_l		REG_ADDR8(0x140336)//default 0xf0
#define reg_gpio_pd_fuc_h		REG_ADDR8(0x140337)

//PE
#define reg_gpio_pe_setting1	REG_ADDR32(0x140320)
#define reg_gpio_pe_in			REG_ADDR8(0x140320)
#define reg_gpio_pe_ie			REG_ADDR8(0x140321)
#define reg_gpio_pe_oen			REG_ADDR8(0x140322)
#define reg_gpio_pe_out			REG_ADDR8(0x140323)

#define reg_gpio_pe_setting2	REG_ADDR32(0x140324)
#define reg_gpio_pe_pol			REG_ADDR8(0x140324)
#define reg_gpio_pe_ds			REG_ADDR8(0x140325)
#define reg_gpio_pe_gpio		REG_ADDR8(0x140326)
#define reg_gpio_pe_irq_en		REG_ADDR8(0x140327)

#define reg_gpio_pe_fs			REG_ADDR16(0x140350)
#define reg_gpio_pe_fuc_l		REG_ADDR8(0x140350)
#define reg_gpio_pe_fuc_h		REG_ADDR8(0x140351)

//PF
#define reg_gpio_pf_setting1	REG_ADDR32(0x140328)
#define reg_gpio_pf_in			REG_ADDR8(0x140328)
#define reg_gpio_pf_ie			REG_ADDR8(0x140329)
#define reg_gpio_pf_oen			REG_ADDR8(0x14032a)
#define reg_gpio_pf_out			REG_ADDR8(0x14032b)

#define reg_gpio_pf_setting2	REG_ADDR32(0x14032c)
#define reg_gpio_pf_ds			REG_ADDR8(0x14032d)
#define reg_gpio_pf_gpio		REG_ADDR8(0x14032e)

#define reg_gpio_pf_fs			REG_ADDR16(0x140356)
#define reg_gpio_pf_fuc_l		REG_ADDR8(0x140356)
#define reg_gpio_pf_fuc_h		REG_ADDR8(0x140357)

#define reg_gpio_in(i)			REG_ADDR8(0x140300+((i>>8)<<3))
#define reg_gpio_ie(i)			REG_ADDR8(0x140301+((i>>8)<<3))
#define reg_gpio_oen(i)			REG_ADDR8(0x140302+((i>>8)<<3))
#define reg_gpio_out(i)			REG_ADDR8(0x140303+((i>>8)<<3))
#define reg_gpio_pol(i)			REG_ADDR8(0x140304+((i>>8)<<3))
#define reg_gpio_ds(i)			REG_ADDR8(0x140305+((i>>8)<<3))


#define reg_gpio_func(i)			REG_ADDR8(0x140306+((i>>8)<<3))
#define reg_gpio_irq_en(i)	        REG_ADDR8(0x140307+((i>>8)<<3))  // reg_irq_mask: FLD_IRQ_GPIO_EN
#define reg_gpio_irq_risc0_en(i)  	REG_ADDR8(0x140338 + (i >> 8))	  // reg_irq_mask: FLD_IRQ_GPIO_RISC0_EN
#define reg_gpio_irq_risc1_en(i)  	REG_ADDR8(0x140340 + (i >> 8))	  // reg_irq_mask: FLD_IRQ_GPIO_RISC1_EN

#define reg_gpio_func_mux(i)        REG_ADDR8(0x140330 + (((i>>8)>3) ? 0x20 : ((i>>8)<<1) ) + ((i&0x0f0) ? 1 : 0 ))


#define reg_gpio_irq_risc_mask  	REG_ADDR8(0x140352)
enum{
	FLD_GPIO_IRQ_MASK_GPIO       = 			BIT(0),
	FLD_GPIO_IRQ_MASK_GPIO2RISC0 = 			BIT(1),
	FLD_GPIO_IRQ_MASK_GPIO2RISC1 = 			BIT(2),

	FLD_GPIO_IRQ_LVL_GPIO        = 			BIT(4),
	FLD_GPIO_IRQ_LVL_GPIO2RISC0  = 			BIT(5),
	FLD_GPIO_IRQ_LVL_GPIO2RISC1  = 			BIT(6),

};
#define reg_gpio_irq_ctrl  		REG_ADDR8(0x140353)
enum{
    FLD_GPIO_CORE_WAKEUP_EN  	= BIT(2),
    FLD_GPIO_CORE_INTERRUPT_EN 	= BIT(3),
};
#define reg_gpio_pad_mul_sel		REG_ADDR8(0x140355)

#define reg_gpio_irq_clr  	       REG_ADDR8(0x140358)
typedef enum{
	FLD_GPIO_IRQ_CLR                      = BIT(0),
	FLD_GPIO_IRQ_GPIO2RISC0_CLR           = BIT(1),
	FLD_GPIO_IRQ_GPIO2RISC1_CLR = 			BIT(2),
}gpio_irq_status_e;

#endif
