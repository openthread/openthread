/********************************************************************************************************
 * @file     dma_reg.h
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
#ifndef DMA_REG_H
#define DMA_REG_H
#include "../../common/bit.h"
#include "../sys.h"
/*******************************    dma registers:  0x100400     ******************************/
#define reg_dma_id REG_ADDR32(0x100400)
#define reg_dma_cfg REG_ADDR32(0x100410)

#define FLD_DMA_CHANNEL_NUM BIT_RNG(0, 3)
#define FLD_DMA_FIFO_DEPTH BIT_RNG(4, 9)
#define FLD_DMA_REQ_NUM BIT_RNG(10, 14)
#define FLD_DMA_REQ_SYNC BIT(30)
#define FLD_DMA_CHANINXFR BIT(31)

#define reg_dma_ctrl(i) REG_ADDR32((0x00100444 + (i)*0x14))

enum
{
    FLD_DMA_CHANNEL_ENABLE        = BIT(0),
    FLD_DMA_CHANNEL_TC_MASK       = BIT(1),
    FLD_DMA_CHANNEL_ERR_MASK      = BIT(2),
    FLD_DMA_CHANNEL_ABT_MASK      = BIT(3),
    FLD_DMA_CHANNEL_DST_REQ_SEL   = BIT_RNG(4, 8),
    FLD_DMA_CHANNEL_SRC_REQ_SEL   = BIT_RNG(9, 13),
    FLD_DMA_CHANNEL_DST_ADDR_CTRL = BIT_RNG(14, 15),
    FLD_DMA_CHANNEL_SRC_ADDR_CTRL = BIT_RNG(16, 17),
    FLD_DMA_CHANNEL_DST_MODE      = BIT(18),
    FLD_DMA_CHANNEL_SRC_MODE      = BIT(19),
    FLD_DMA_CHANNEL_DST_WIDTH     = BIT_RNG(20, 21),
    FLD_DMA_CHANNEL_SRC_WIDTH     = BIT_RNG(22, 23),
};

#define reg_dma_ctr0(i) REG_ADDR8((0x00100444 + (i)*0x14))

#define reg_dma_isr REG_ADDR32(0x100430)
enum
{
    FLD_DMA_CHANNEL0_IRQ = BIT(16),
    FLD_DMA_CHANNEL1_IRQ = BIT(17),
    FLD_DMA_CHANNEL2_IRQ = BIT(18),
    FLD_DMA_CHANNEL3_IRQ = BIT(19),
    FLD_DMA_CHANNEL4_IRQ = BIT(20),
    FLD_DMA_CHANNEL5_IRQ = BIT(21),
    FLD_DMA_CHANNEL6_IRQ = BIT(22),
    FLD_DMA_CHANNEL7_IRQ = BIT(23),
};

#define reg_dma_ctr3(i) REG_ADDR8((0x00100447 + (i)*0x14))

enum
{
    FLD_DMA_SRC_BURST_SIZE = BIT_RNG(0, 2),
    FLD_DMA_R_NUM_EN       = BIT(4),
    FLD_DMA_PRIORITY       = BIT(5),
    FLD_DMA_W_NUM_EN       = BIT(6),
    FLD_DMA_AUTO_ENABLE_EN = BIT(7),
};

#define reg_dma_addr(addr) ((u32)(addr)-0x80000 + 0xc0200000)
#define reg_dma_src_addr(i) REG_ADDR32((0x00100448 + (i)*0x14))
#define reg_dma_dst_addr(i) REG_ADDR32((0x0010044c + (i)*0x14))
#define reg_dma_size(i) REG_ADDR32((0x00100450 + (i)*0x14))

enum
{
    FLD_DMA_TX_SIZE     = BIT_RNG(0, 21),
    FLD_DMA_TX_SIZE_IDX = BIT_RNG(22, 23),
};

#define reg_dma_cr3_size(i) (*(volatile unsigned long *)(0x00100452 + (i)*0x14))

enum
{
    FLD_DMA_TSR2_SIZE_IDX = BIT_RNG(6, 7),
};

#define reg_dma_llp(i) REG_ADDR32((0x00100454 + (i)*0x14))

#endif
