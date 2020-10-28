/********************************************************************************************************
 * @file	dma_reg.h
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
#ifndef DMA_REG_H
#define DMA_REG_H
#include "../../common/bit.h"
#include "../sys.h"
/*******************************    dma registers:  0x100400     ******************************/
#define reg_dma_id					REG_ADDR32(0x100400)
#define reg_dma_cfg					REG_ADDR32(0x100410)

#define	FLD_DMA_CHANNEL_NUM		 BIT_RNG(0,3)
#define	FLD_DMA_FIFO_DEPTH		 BIT_RNG(4,9)
#define	FLD_DMA_REQ_NUM			 BIT_RNG(10,14)
#define	FLD_DMA_REQ_SYNC		 BIT(30)
#define	FLD_DMA_CHANINXFR		 BIT(31)

#define reg_dma_ctrl(i)				REG_ADDR32(( 0x00100444 +(i)*0x14))

enum{
	FLD_DMA_CHANNEL_ENABLE			= BIT(0),
	FLD_DMA_CHANNEL_TC_MASK			= BIT(1),
	FLD_DMA_CHANNEL_ERR_MASK		= BIT(2),
	FLD_DMA_CHANNEL_ABT_MASK		= BIT(3),
	FLD_DMA_CHANNEL_DST_REQ_SEL		= BIT_RNG(4,8),
	FLD_DMA_CHANNEL_SRC_REQ_SEL		= BIT_RNG(9,13),
	FLD_DMA_CHANNEL_DST_ADDR_CTRL	= BIT_RNG(14,15),
	FLD_DMA_CHANNEL_SRC_ADDR_CTRL	= BIT_RNG(16,17),
	FLD_DMA_CHANNEL_DST_MODE		= BIT(18),
	FLD_DMA_CHANNEL_SRC_MODE		= BIT(19),
	FLD_DMA_CHANNEL_DST_WIDTH		= BIT_RNG(20,21),
	FLD_DMA_CHANNEL_SRC_WIDTH		= BIT_RNG(22,23),
};

#define reg_dma_ctr0(i)			    REG_ADDR8(( 0x00100444 +(i)*0x14))


#define reg_dma_err_isr					REG_ADDR8(0x100430)
#define reg_dma_abt_isr					REG_ADDR8(0x100431)
#define reg_dma_tc_isr					REG_ADDR8(0x100432)

enum{
	FLD_DMA_CHANNEL0_IRQ		= BIT(0),
	FLD_DMA_CHANNEL1_IRQ		= BIT(1),
	FLD_DMA_CHANNEL2_IRQ		= BIT(2),
	FLD_DMA_CHANNEL3_IRQ		= BIT(3),
	FLD_DMA_CHANNEL4_IRQ		= BIT(4),
	FLD_DMA_CHANNEL5_IRQ		= BIT(5),
	FLD_DMA_CHANNEL6_IRQ		= BIT(6),
	FLD_DMA_CHANNEL7_IRQ		= BIT(7),
};





#define reg_dma_ctr3(i)			    REG_ADDR8((0x00100447 +(i)*0x14))

enum{
	FLD_DMA_SRC_BURST_SIZE    		=	BIT_RNG(0,2),
	FLD_DMA_R_NUM_EN    	        =	BIT(4),
	FLD_DMA_PRIORITY    	        =	BIT(5),
	FLD_DMA_W_NUM_EN   	            =	BIT(6),
	FLD_DMA_AUTO_ENABLE_EN           =	BIT(7),
};



#define reg_dma_src_addr(i)			REG_ADDR32 (( 0x00100448 +(i)*0x14))
#define reg_dma_dst_addr(i)			REG_ADDR32 (( 0x0010044c +(i)*0x14))
#define reg_dma_size(i)			    REG_ADDR32 (( 0x00100450 +(i)*0x14))

enum{
	FLD_DMA_TX_SIZE    		=	BIT_RNG(0,21),
	FLD_DMA_TX_SIZE_IDX    	=	BIT_RNG(22,23),
};




#define reg_dma_cr3_size(i)			(*(volatile unsigned long*) ( 0x00100452 +(i)*0x14))

enum{
	FLD_DMA_TSR2_SIZE_IDX    	=	BIT_RNG(6,7),
};

#define reg_dma_llp(i)			    REG_ADDR32 (( 0x00100454 +(i)*0x14))



#endif
