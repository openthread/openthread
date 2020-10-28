/********************************************************************************************************
 * @file	core.h
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
#ifndef CORE_H
#define CORE_H
#include "nds_intrinsic.h"
#include "../common/types.h"
#include "../common/bit.h"
#define  read_csr(reg)		         __nds__csrr(reg)
#define  write_csr(reg, val)	      __nds__csrw(val, reg)
#define  swap_csr(reg, val)	          __nds__csrrw(val, reg)
#define set_csr(reg, bit)	         __nds__csrrs(bit, reg)
#define clear_csr(reg, bit)	         __nds__csrrc(bit, reg)

/*
 * Inline nested interrupt entry/exit macros
 */
/* Svae/Restore macro */
#define save_csr(r)             long __##r = read_csr(r);
#define restore_csr(r)           write_csr(r, __##r);
/* Support PowerBrake (Performance Throttling) feature */

#define MSTATUS_UIE         	0x00000001
#define MSTATUS_SIE         	0x00000002
#define MSTATUS_HIE         	0x00000004
#define MSTATUS_MIE         	0x00000008
#define MSTATUS_UPIE        	0x00000010
#define MSTATUS_SPIE        	0x00000020
#define MSTATUS_HPIE        	0x00000040
#define MSTATUS_MPIE        	0x00000080
#define MSTATUS_SPP         	0x00000100
#define MSTATUS_HPP         	0x00000600
#define MSTATUS_MPP         	0x00001800
#define MSTATUS_FS          	0x00006000
#define MSTATUS_XS          	0x00018000
#define MSTATUS_MPRV        	0x00020000
#define MSTATUS_PUM         	0x00040000
#define MSTATUS_MXR				0x00080000
#define MSTATUS_VM          	0x1F000000
#define MSTATUS32_SD        	0x80000000
#define MSTATUS64_SD        	0x8000000000000000

#define save_mxstatus()         save_csr(NDS_MXSTATUS)
#define restore_mxstatus()      restore_csr(NDS_MXSTATUS)

 /* Nested IRQ entry macro : Save CSRs and enable global interrupt. */
#define core_save_nested_context()                              \
	 save_csr(NDS_MEPC)                              \
	 save_csr(NDS_MSTATUS)                           \
	 save_mxstatus()                                 \
	 set_csr(NDS_MSTATUS, MSTATUS_MIE);

/* Nested IRQ exit macro : Restore CSRs */
#define core_restore_nested_context()                               \
	 clear_csr(NDS_MSTATUS, MSTATUS_MIE);            \
	 restore_csr(NDS_MSTATUS)                        \
	 restore_csr(NDS_MEPC)                           \
	 restore_mxstatus()


typedef enum{
	FLD_FEATURE_PREEMPT_PRIORITY_INT_EN = BIT(0),
	FLD_FEATURE_VECTOR_MODE_EN 			= BIT(1),
}
feature_e;


/**
 * @brief Disable interrupts globally in the system.external, timer and software interrupts.
 * @return  r - the value of machine interrupt enable(MIE) register.
 * @note  this function must be used when the system wants to disable all the interrupt.
 */
static inline u32 core_interrupt_disable(void){

	u32 r = read_csr (NDS_MIE);
	clear_csr(NDS_MIE, BIT(3)| BIT(7)| BIT(11));
	return r;
}


/**
 * @brief restore interrupts globally in the system. external,timer and software interrupts.
 * @param[in]  en - the value of machine interrupt enable(MIE) register before disable.
 * @return     0
 * @note this function must be used when the system wants to restore all the interrupt.
 */
static inline u32 core_restore_interrupt(u32 en){

	set_csr(NDS_MIE, en);
	return 0;
}

/**
 * @brief enable interrupts globally in the system.
 * @return  none
 */
static inline void core_interrupt_enable(void)
{
	set_csr(NDS_MSTATUS,1<<3);
	set_csr(NDS_MIE,1<<11 | 1 << 7 | 1 << 3);

}
#endif
