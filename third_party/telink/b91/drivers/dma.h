/********************************************************************************************************
 * @file	dma.h
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
/**	@page DMA
 *
 *	Introduction
 *	===============
 *	
 *
 *	API Reference
 *	===============
 *	Header File: dma.h
 */
#ifndef DMA_H_
#define DMA_H_
#include "reg_include/register_b91.h"
 typedef enum{
	DMA0=0,
	DMA1,
	DMA2,
	DMA3,
	DMA4,
	DMA5,
	DMA6,
	DMA7,
}dma_chn_e;

typedef enum{
	DMA_CHN0_IRQ		= BIT(0),
	DMA_CHN1_IRQ		= BIT(1),
	DMA_CHN2_IRQ		= BIT(2),
	DMA_CHN3_IRQ		= BIT(3),
	DMA_CHN4_IRQ		= BIT(4),
	DMA_CHN5_IRQ		= BIT(5),
	DMA_CHN6_IRQ		= BIT(6),
	DMA_CHN7_IRQ		= BIT(7),
}dma_irq_chn_e;

typedef enum{
	DMA_REQ_SPI_AHB_TX=0,
	DMA_REQ_SPI_AHB_RX,
	DMA_REQ_UART0_TX,
	DMA_REQ_UART0_RX,
	DMA_REQ_SPI_APB_TX,
	DMA_REQ_SPI_APB_RX,
	DMA_REQ_I2C_TX,
	DMA_REQ_I2C_RX,
	DMA_REQ_ZB_TX,
	DMA_REQ_ZB_RX,
	DMA_REQ_PWM_TX,
	DMA_REQ_RESERVED,
	DMA_REQ_ALGM_TX,
	DMA_REQ_ALGM_RX,
	DMA_REQ_UART1_TX,
	DMA_REQ_UART1_RX,
	DMA_REQ_AUDIO0_TX,
	DMA_REQ_AUDIO0_RX,
	DMA_REQ_AUDIO1_TX,
	DMA_REQ_AUDIO1_RX,

}dma_req_sel_e;

typedef enum{
	DMA_ADDR_INCREMENT=0,
	DMA_ADDR_DECREMENT,
	DMA_ADDR_FIX,
}
dma_addr_ctrl_e;


typedef enum{
	DMA_NORMAL_MODE=0,
	DMA_HANDSHAKE_MODE,
}
dma_mode_e;

typedef enum{
	DMA_CTR_BYTE_WIDTH=0,
	DMA_CTR_HWORD_WIDTH,
	DMA_CTR_WORD_WIDTH,
}
dma_ctr_width_e;

typedef enum{
	DMA_BYTE_WIDTH=1,
	DMA_HWORD_WIDTH=2,
	DMA_WORD_WIDTH=4,
}
dma_transfer_width_e;

typedef enum{
	TC_MASK   =   	BIT(1),
	ERR_MASK  =		BIT(2),
	ABT_MASK  =		BIT(3),
}dma_irq_mask_e;

typedef struct {
	unsigned int dst_req_sel:5;  /*DstReqSel   :8:4   */
	unsigned int src_req_sel:5;  /*SrcReqSel   :13:9 */
	unsigned int dst_addr_ctrl:2;/*DstAddrCtrl :15:14  0:increment address 1: decrement address 2: fixed address */
	unsigned int src_addr_ctrl:2;/*SrcAddrCtrl :17:16  0:increment address 1: decrement address 2: fixed address */
	unsigned int dstmode:1;      /*DstMode:18   0 normal mode  1 handshake*/
	unsigned int srcmode:1;     /*SrcMode :19   0 normal mode  1 handshake*/
	unsigned int dstwidth:2;  /*DstWidth :21:20 00:byte 01:hword 02:word*/
	unsigned int srcwidth:2;  /*SrcWidth :23:22  00:byte 01:hword 02:word*/
	unsigned int src_burst_size:3;  /*SrcBurstSize: 26:24*/
	unsigned int vacant_bit  :1;/*vacant:27*/
	unsigned int read_num_en:1;/*Rnum_en :28*/
	unsigned int priority:1; /*Pri :29*/
	unsigned int write_num_en:1;/*wnum_en : 30*/
	unsigned int auto_en:1;/*/*auto_en : 31*/
}dma_config_t;


typedef struct {
	unsigned int dma_chain_ctl;
	unsigned int dma_chain_src_addr;
	unsigned int dma_chain_dst_addr;
	unsigned int dma_chain_data_len;
	unsigned int dma_chain_llp_ptr;
}dma_chain_config_t ;


/**
 * @brief     This function configures DMA control register.
 * @param[in] chn    - dma channel
 * @param[in] config - the prt of dma_config that configured control register
 * @return    none
 */
static inline void dma_config(dma_chn_e chn ,dma_config_t *config)
{
	BM_CLR(reg_dma_ctrl(chn),BIT_RNG(4,31));
	reg_dma_ctrl(chn) |= (*(u32*)config)<<4;
}


/**
 * @brief      This function servers to enable dma that selected channel.
 * @param[in] chn - dma channel.
 * @return    none
 */
static inline void dma_chn_en(dma_chn_e chn)
{
	BM_SET(reg_dma_ctr0(chn),BIT(0));
}

/**
 * @brief      This function servers to disable dma that selected channel.
 * @param[in] chn - dma channel.
 * @return    none
 */
static inline void dma_chn_dis(dma_chn_e chn)
{
	BM_CLR(reg_dma_ctr0(chn),BIT(0));
}

/**
 * @brief      This function servers to set dma irq mask.
 * @param[in] chn  - dma channel.
 * @param[in] mask - dma irq mask.
 * @return    none
 */
static inline void dma_set_irq_mask(dma_chn_e chn,dma_irq_mask_e mask)
{
	reg_dma_ctr0(chn) = (reg_dma_ctr0(chn) | BIT_RNG(1,3)) & (~(mask));
}

/**
 * @brief      This function servers to clr dma irq mask.
 * @param[in] chn  - dma channel.
 * @param[in] mask - dma irq mask.
 * @return    none
 * attention the mask of dma tc/err/abt is enable default.we must disable when don'n use it.
 */
static inline void dma_clr_irq_mask(dma_chn_e chn,dma_irq_mask_e mask)
{
	reg_dma_ctr0(chn) |= (mask);
}

/**
 * @brief     This function servers to get the terminal count irq status channel.
 * @return   the dma terminal count irq status channel.
 */
static inline unsigned char dma_get_tc_irq_status(dma_irq_chn_e tc_chn)
{
	return reg_dma_tc_isr&tc_chn;
}


/**
 * @brief    This function servers to clear the irq of terminal count status.
 * @param[in] tc_chn -terminal count irq status channel.
 * @return    none
 */
static inline void dma_clr_tc_irq_status(dma_irq_chn_e tc_chn)
{
	reg_dma_tc_isr = tc_chn;
}


/**
 * @brief     This function servers to get the error  irq status channel.
 * @return   the dma error irq status channel.
 */
static inline unsigned char dma_get_err_irq_status(dma_irq_chn_e err_chn)
{
	return reg_dma_err_isr&err_chn;
}


/**
 * @brief    This function servers to clear the abort status of channel.
 * @param[in] err_chn -error status channel.
 * @return    none
 */
static inline void dma_clr_err_irq_status(dma_irq_chn_e err_chn)
{
	reg_dma_err_isr = err_chn;
}


/**
 * @brief    This function servers to get the abort status of channel.
 * @return   the dma abort irq status channel.
 */
static inline unsigned char dma_get_abt_irq_status(dma_irq_chn_e abt_chn)
{
	return reg_dma_abt_isr&abt_chn;
}


/**
 * @brief    This function servers to clear the abort status of channel.
 * @param[in] abt_chn -abort irq status channel.
 * @return    none
 */
static inline void dma_clr_abt_irq_status(dma_irq_chn_e abt_chn)
{
	reg_dma_abt_isr = abt_chn;
}

/**
 * @brief   this  function set  the DMA to tx/rx size byte.
 * @param[in] chn - DMA channel
 * @param[in] size_byte  - the address of dma   tx/rx size
 * @param[in] byte_width -  dma   tx/rx  width
 * @return    none
 */
static inline void dma_set_size(dma_chn_e chn,u32 size_byte,dma_transfer_width_e byte_width)
{
	 reg_dma_size(chn) =((size_byte+byte_width-1)/byte_width)|( (size_byte % byte_width)<<22);
}


/**
 * @brief   this  function calculate the DMA to tx/rx size byte.
 * @param[in] size_byte  - the address of dma   tx/rx size
 * @param[in] byte_width -  dma   tx/rx  width
 * @return    none
 */
static inline u32 dma_cal_size(u32 size_byte,dma_transfer_width_e byte_width)
{
	 return (((size_byte+byte_width-1)/byte_width)|( (size_byte % byte_width)<<22));
}


/**
 * @brief   this function set source and destination address for DMA,
 *        src_address          dst_address
 * tx      sram               interface fifo.
 * rx      interface fifo      sram
 * @param[in]  chn      - DMA channel
 * @param[in]  src_addr - the address of source.
 * @param[in]  dst_addr - the address of destination.
 * @return    none
 */
static inline void dma_set_address(dma_chn_e chn,u32 src_addr,u32 dst_addr)
{
	reg_dma_src_addr(chn)=src_addr;
	reg_dma_dst_addr(chn)=dst_addr;
}


/**
 * @brief   this function set source address for DMA,
 * @param[in]  chn - DMA channel
 * @param[in]  src_addr - the address of source.
 *  */
static inline void dma_set_src_address(dma_chn_e chn,u32 src_addr)
{
	reg_dma_src_addr(chn)=src_addr;
}

/**
 * @brief   this function set destination address for DMA,
 * @param[in]  chn - DMA channel
 * @param[in]  dst_addr - the address of destination.
 *  */
static inline void dma_set_dst_address(dma_chn_e chn,u32 dst_addr)
{
	reg_dma_dst_addr(chn)=dst_addr;
}


/**
 * @brief   this function set reset  DMA,
 * @return    none
 */
static inline void dma_reset(void)
{
	reg_rst1 &= ~(FLD_RST1_DMA);
	reg_rst1 |= FLD_RST1_DMA;
}


#endif
