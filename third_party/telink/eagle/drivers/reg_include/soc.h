/********************************************************************************************************
 * @file     soc.h
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
#ifndef SOC_H_
#define SOC_H_
#include "../../common/bit.h"
#include "../sys.h"

#define LM_BASE 0x80000000

#define ILM_BASE (LM_BASE + 0x40000000)
#define DLM_BASE (LM_BASE + 0x40200000)
#define CPU_ILM_BASE (0x00000000)
#define CPU_DLM_BASE (0x00080000)

#define SC_BASE_ADDR 0x1401c0
/*******************************      reset registers: 1401e0      ******************************/
#define reg_rst REG_ADDR32(0x1401e0)

#define reg_rst0 REG_ADDR8(0x1401e0)
enum
{
    FLD_RST0_HSPI  = BIT(0),
    FLD_RST0_I2C   = BIT(1),
    FLD_RST0_UART0 = BIT(2),
    FLD_RST0_USB   = BIT(3),
    FLD_RST0_PWM   = BIT(4),
    FLD_RST0_UART1 = BIT(6),
    FLD_RST0_SWIRE = BIT(7),
};

#define reg_rst1 REG_ADDR8(0x1401e1)
enum
{
    FLD_RST1_SYS_STIMER = BIT(1),
    FLD_RST1_DMA        = BIT(2),
    FLD_RST1_ALGM       = BIT(3),
    FLD_RST1_PKE        = BIT(4),
    // RSVD
    FLD_RST1_PSPI   = BIT(6),
    FLD_RST1_SPISLV = BIT(7),
};

#define reg_rst2 REG_ADDR8(0x1401e2)
enum
{
    FLD_RST2_TIMER = BIT(0),
    FLD_RST2_AUD   = BIT(1),
    FLD_RST2_TRNG  = BIT(2),
    // RSVD
    FLD_RST2_MCU = BIT(4),
    FLD_RST2_LM  = BIT(5),
    FLD_RST2_NPE = BIT(6),
    // RSVD
};

#define reg_rst3 REG_ADDR8(0x1401e3)
enum
{
    FLD_RST3_ZB       = BIT(0),
    FLD_RST3_MSTCLK   = BIT(1),
    FLD_RST3_LPCLK    = BIT(2),
    FLD_RST3_ZB_CRYPT = BIT(3),
    FLD_RST3_SARADC   = BIT(6),
    FLD_RST3_ALG      = BIT(7),
};

#define reg_clk_en REG_ADDR32(0x1401e4)

#define reg_clk_en0 REG_ADDR8(0x1401e4)
enum
{
    FLD_CLK0_HSPI_EN  = BIT(0),
    FLD_CLK0_I2C_EN   = BIT(1),
    FLD_CLK0_UART0_EN = BIT(2),
    FLD_CLK0_USB_EN   = BIT(3),
    FLD_CLK0_PWM_EN   = BIT(4),
    FLD_CLK0_UART1_EN = BIT(6),
    FLD_CLK0_SWIRE_EN = BIT(7),
};

#define reg_clk_en1 REG_ADDR8(0x1401e5)
enum
{
    FLD_CLK1_SYS_TIMER_EN   = BIT(1),
    FLD_CLK1_DMA_EN         = BIT(2),
    FLD_CLK1_ALGM_EN        = BIT(3),
    FLD_CLK1_PKE_EN         = BIT(4),
    FLD_CLK1_MACHINETIME_EN = BIT(5),
    FLD_CLK1_PSPI_EN        = BIT(6),
    FLD_CLK1_SPISLV_EN      = BIT(7),

};

#define reg_clk_en2 REG_ADDR8(0x1401e6)
enum
{
    FLD_CLK2_TIMER_EN = BIT(0),
    FLD_CLK2_AUD_EN   = BIT(1),
    FLD_CLK2_TRNG_EN  = BIT(2),
    FLD_CLK2_MCU_EN   = BIT(4),
    FLD_CLK2_LM_EN    = BIT(5),
    FLD_CLK2_NPE_EN   = BIT(6),
    FLD_CLK2_EOC_EN   = BIT(7),
};

#define reg_clk_en3 REG_ADDR8(0x1401e7)
enum
{
    FLD_CLK3_ZB_PCLK_EN   = BIT(0),
    FLD_CLK3_ZB_MSTCLK_EN = BIT(1),
    FLD_CLK3_ZB_LPCLK_EN  = BIT(2),

};

#define reg_i2s_step REG_ADDR8(SC_BASE_ADDR + 0x2a)
enum
{
    FLD_I2S_STEP   = BIT_RNG(0, 6),
    FLD_I2S_CLK_EN = BIT(7),
};

#define reg_i2s_mod REG_ADDR8(SC_BASE_ADDR + 0x2b)

#define reg_dmic_step REG_ADDR8(SC_BASE_ADDR + 0x2c)
enum
{
    FLD_DMIC_STEP = BIT_RNG(0, 6),
    FLD_DMIC_SEL  = BIT(7),
};

#define reg_dmic_mod REG_ADDR8(SC_BASE_ADDR + 0x2d)

#define reg_dmic_clk_set REG_ADDR8(SC_BASE_ADDR + 0x33)

#endif
