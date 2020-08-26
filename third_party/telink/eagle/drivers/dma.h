/********************************************************************************************************
 * @file     dma.h
 *
 * @brief    This is the source file for TLSR9518
 *
 * @author	 Driver Group
 * @date     September 16, 2019
 *
 * @par      Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
 * @par      History:
 * 			 1.initial release(September 16, 2019)
 *
 * @version  A001
 *
 *******************************************************************************************************/

#ifndef DMA_H_
#define DMA_H_
#include "reg_include/register_9518.h"
enum
{
    TRANSIZE_BYTE_SIZE = BIT_RNG(22, 23),
    TRANSIZE           = BIT_RNG(0, 21),
};

typedef enum
{
    DMA0 = 0,
    DMA1,
    DMA2,
    DMA3,
    DMA4,
    DMA5,
    DMA6,
    DMA7,
} dma_chn_e;

typedef enum
{
    DMA_REQ_SPI_AHB_TX = 0,
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

} dma_req_sel_e;

typedef enum
{
    DMA_ADDR_INCREMENT = 0,
    DMA_ADDR_DECREMENT,
    DMA_ADDR_FIX,
} dma_addr_ctrl_e;

typedef enum
{
    DMA_NORMAL_MODE = 0,
    DMA_HANDSHAKE_MODE,
} dma_mode_e;

typedef enum
{
    DMA_CTR_BYTE_WIDTH = 0,
    DMA_CTR_HWORD_WIDTH,
    DMA_CTR_WORD_WIDTH,
} dma_ctr_width_e;

typedef enum
{
    DMA_BYTE_WIDTH  = 1,
    DMA_HWORD_WIDTH = 2,
    DMA_WORD_WIDTH  = 4,
} dma_transfer_width_e;

typedef enum
{
    TC_MASK  = BIT(1),
    ERR_MASK = BIT(2),
    ABT_MASK = BIT(3),
} dma_irq_mask_e;

typedef struct
{
    unsigned int dst_req_sel : 5;    /*DstReqSel   :8:4   */
    unsigned int src_req_sel : 5;    /*SrcReqSel   :13:9 */
    unsigned int dst_addr_ctrl : 2;  /*DstAddrCtrl :15:14  0:increment address 1: decrement address 2: fixed address */
    unsigned int src_addr_ctrl : 2;  /*SrcAddrCtrl :17:16  0:increment address 1: decrement address 2: fixed address */
    unsigned int dstmode : 1;        /*DstMode:18   0 normal mode  1 handshake*/
    unsigned int srcmode : 1;        /*SrcMode :19   0 normal mode  1 handshake*/
    unsigned int dstwidth : 2;       /*DstWidth :21:20 00:byte 01:hword 02:word*/
    unsigned int srcwidth : 2;       /*SrcWidth :23:22  00:byte 01:hword 02:word*/
    unsigned int src_burst_size : 3; /*SrcBurstSize: 26:24*/
    unsigned int vacant_bit : 1;     /*vacant:27*/
    unsigned int read_num_en : 1;    /*Rnum_en :28*/
    unsigned int priority : 1;       /*Pri :29*/
    unsigned int write_num_en : 1;   /*wnum_en : 30*/
    unsigned int auto_en : 1;        /*/*auto_en : 31*/
} dma_config_st;

/**
 * @brief     This function configures DMA control register.
 * @param[in] config - the prt of dma_config that configured control register
 * @return    none
 */
static inline void dma_config(dma_chn_e chn, dma_config_st *config)
{
    BM_CLR(reg_dma_ctrl(chn), BIT_RNG(4, 31));
    reg_dma_ctrl(chn) |= (*(u32 *)config) << 4;
}

/**
 * @brief      This function servers to enable dma that selected channel.
 * @param[in] che - dma channel.
 * @return    none
 */
static inline void dma_chn_en(dma_chn_e chn)
{
    BM_SET(reg_dma_ctr0(chn), BIT(0));
}

/**
 * @brief      This function servers to disable dma that selected channel.
 * @param[in] che - dma channel.
 * @return    none
 */
static inline void dma_chn_dis(dma_chn_e chn)
{
    BM_CLR(reg_dma_ctr0(chn), BIT(0));
}

/**
 * @brief      This function servers to set dma irq mask.
 * @param[in] che - dma channel.
 * @return    none
 */
static inline void dma_set_irq_mask(dma_chn_e chn, dma_irq_mask_e mask)
{
    reg_dma_ctr0(chn) = (reg_dma_ctr0(chn) | BIT_RNG(1, 3)) & (~(mask));
}

/**
 * @brief   this  function set  the DMA to tx/rx size byte.
 * @param[in] chn - DMA channel
 * @param[in] bytesize - the address of dma   tx/rx size
 * @param[in] byte_width -  dma   tx/rx  width
 *  */
static inline void dma_set_size(dma_chn_e chn, u32 size_byte, dma_transfer_width_e byte_width)
{
    reg_dma_size(chn) = ((size_byte + byte_width - 1) / byte_width) | ((size_byte % byte_width) << 22);
}

/**
 * @brief   this  function set  the DMA to get data from the RAM and sent to interface buff.
 * @param[in] chn - DMA channel
 * @param[in] src_address - the address of source.
 * @param[in]  dst_address - the address of destination.
 *  */
static inline void dma_set_address(dma_chn_e chn, u32 src_addr, u32 dst_addr)
{
    reg_dma_src_addr(chn) = src_addr;
    reg_dma_dst_addr(chn) = dst_addr;
}

#endif
