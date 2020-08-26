/********************************************************************************************************
 * @file     analog_reg.h
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
#ifndef ANALOG_REG_H
#define ANALOG_REG_H
#include "../../common/bit.h"
#include "../sys.h"
/*******************************      alg  registers: 140180      ******************************/
#define ALG_BASE_ADDR 0x140180
#define reg_ana_addr REG_ADDR8(ALG_BASE_ADDR)
#define reg_ana_ctrl REG_ADDR8(ALG_BASE_ADDR + 0x02)
enum
{
    // FLD_ANA_TX_EN         = BIT(0),
    FLD_ANA_RX_EN         = BIT(1),
    FLD_ANA_MASKX_TX_DONE = BIT(2),
    FLD_ANA_MASKX_RX_DONE = BIT(3),
    FLD_ANA_CONTIU_ACC    = BIT(4),
    FLD_ANA_RW            = BIT(5), // 1:write,0:read
    FLD_ANA_CYC           = BIT(6),
    FLD_ANA_BUSY          = BIT(7),

};
#define reg_ana_len REG_ADDR8(ALG_BASE_ADDR + 0x03)
#define reg_ana_data(n) REG_ADDR8(ALG_BASE_ADDR + 0x04 + (n))
#define reg_ana_addr_data16 REG_ADDR16(ALG_BASE_ADDR + 0x04)
#define reg_ana_addr_data32 REG_ADDR32(ALG_BASE_ADDR + 0x04)

#define reg_ana_sta REG_ADDR8(ALG_BASE_ADDR + 0x09)
#define reg_ana_dma_ctl REG_ADDR8(ALG_BASE_ADDR + 0x0b)
enum
{
    FLD_ANA_CYC1   = BIT(0),
    FLD_ANA_DMA_EN = BIT(1),

};

#endif
