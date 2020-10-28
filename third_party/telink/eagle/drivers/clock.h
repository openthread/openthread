/********************************************************************************************************
 * @file	clock.h
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
/**	@page CLOCK
 *
 *	Introduction
 *	===============
 *	TLSRB91 clock setting. 
 *
 *	API Reference
 *	===============
 *	Header File: clock.h
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include "reg_include/register_b91.h"
#include "compiler.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
#define  	CCLK_16M_HCLK_16M_PCLK_16M		clock_init(PLL_CLK_192M, PAD_PLL_DIV, PLL_DIV12_TO_CCLK, CCLK_DIV1_TO_HCLK, HCLK_DIV1_TO_PCLK, PLL_DIV4_TO_MSPI_CLK)
#define		CCLK_24M_HCLK_24M_PCLK_24M		clock_init(PLL_CLK_192M, PAD_PLL_DIV, PLL_DIV8_TO_CCLK, CCLK_DIV1_TO_HCLK, HCLK_DIV1_TO_PCLK, PLL_DIV4_TO_MSPI_CLK)
#define		CCLK_32M_HCLK_32M_PCLK_16M		clock_init(PLL_CLK_192M, PAD_PLL_DIV, PLL_DIV6_TO_CCLK, CCLK_DIV1_TO_HCLK, HCLK_DIV2_TO_PCLK, PLL_DIV4_TO_MSPI_CLK)
#define		CCLK_48M_HCLK_48M_PCLK_24M		clock_init(PLL_CLK_192M, PAD_PLL_DIV, PLL_DIV4_TO_CCLK, CCLK_DIV1_TO_HCLK, HCLK_DIV2_TO_PCLK, PLL_DIV4_TO_MSPI_CLK)

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/

/**
 *  @brief  Define sys_clk struct.
 */
typedef struct {
	unsigned short pll_clk;		/**< pll clk */
	unsigned char cclk;			/**< cpu clk */
	unsigned char hclk;			/**< hclk */
	unsigned char pclk;			/**< pclk */
	unsigned char mspi_clk;		/**< mspi_clk */
}sys_clk_t;


/**
 * @brief system clock type
 * |           |              |               |
 * | :-------- | :----------- | :------------ |
 * |   <1:0>   |     <6:2>    |    <15:8>     |
 * |ana_09<3:2>|analog_80<4:0>|      clk      |
 */
typedef enum{
	PLL_CLK_48M 	= (0 | (16 << 2) | (48 << 8)),
	PLL_CLK_54M 	= (0 | (17 << 2) | (54 << 8)),
	PLL_CLK_60M 	= (0 | (18 << 2) | (54 << 8)),
	PLL_CLK_66M 	= (0 | (19 << 2) | (66 << 8)),
	PLL_CLK_96M 	= (1 | (16 << 2) | (96 << 8)),
	PLL_CLK_108M 	= (1 | (17 << 2) | (108 << 8)),
	PLL_CLK_120M 	= (1 | (18 << 2) | (120 << 8)),
	PLL_CLK_132M 	= (1 | (19 << 2) | (132 << 8)),
	PLL_CLK_192M 	= (2 | (16 << 2) | (192 << 8)),
	PLL_CLK_216M 	= (2 | (17 << 2) | (216 << 8)),
	PLL_CLK_240M 	= (2 | (18 << 2) | (240 << 8)),
	PLL_CLK_264M 	= (2 | (19 << 2) | (264 << 8)),
}sys_pll_clk_e;

/**
 * @brief system clock type.
 */
typedef enum{
	RC24M,
	PAD24M,
	PAD_PLL_DIV,
	PAD_PLL,
}sys_clock_src_e;

/**
 * @brief 32K clock type.
 */
typedef enum{
	CLK_32K_RC   =0,
	CLK_32K_XTAL =1,
}clk_32k_type_e;

/**
 * @brief pll div to cclk.
 */
typedef enum{
	PLL_DIV2_TO_CCLK    =    2,
	PLL_DIV3_TO_CCLK    =    3,
	PLL_DIV4_TO_CCLK    =    4,
	PLL_DIV5_TO_CCLK    =    5,
	PLL_DIV6_TO_CCLK    =    6,
	PLL_DIV7_TO_CCLK    =    7,
	PLL_DIV8_TO_CCLK    =    8,
	PLL_DIV9_TO_CCLK    =    9,
	PLL_DIV10_TO_CCLK   =    10,
	PLL_DIV11_TO_CCLK   =    11,
	PLL_DIV12_TO_CCLK   =    12,
	PLL_DIV13_TO_CCLK   =    13,
	PLL_DIV14_TO_CCLK   =    14,
	PLL_DIV15_TO_CCLK   =    15,
}sys_pll_div_to_cclk_e;

/**
 * @brief cclk/pll_div to mspi clk.
 */
typedef enum{
	CCLK_TO_MSPI_CLK       	=    1,
	PLL_DIV2_TO_MSPI_CLK    =    2,
	PLL_DIV3_TO_MSPI_CLK    =    3,
	PLL_DIV4_TO_MSPI_CLK    =    4,
	PLL_DIV5_TO_MSPI_CLK    =    5,
	PLL_DIV6_TO_MSPI_CLK    =    6,
	PLL_DIV7_TO_MSPI_CLK    =    7,
	PLL_DIV8_TO_MSPI_CLK    =    8,
	PLL_DIV9_TO_MSPI_CLK    =    9,
	PLL_DIV10_TO_MSPI_CLK   =    10,
	PLL_DIV11_TO_MSPI_CLK   =    11,
	PLL_DIV12_TO_MSPI_CLK   =    12,
	PLL_DIV13_TO_MSPI_CLK   =    13,
	PLL_DIV14_TO_MSPI_CLK   =    14,
	PLL_DIV15_TO_MSPI_CLK   =    15,
}sys_pll_div_to_mspi_clk_e;

/**
 * @brief hclk div to pclk.
 */
typedef enum{
	HCLK_DIV1_TO_PCLK    =    1,
	HCLK_DIV2_TO_PCLK    =    2,
	HCLK_DIV4_TO_PCLK    =    4,
}sys_hclk_div_to_pclk_e;

/**
 * @brief cclk div to hclk.
 */
typedef enum{
	CCLK_DIV1_TO_HCLK    =    1,
	CCLK_DIV2_TO_HCLK    =    2,  /*< can not use in A0. if use reboot when hclk = 1/2cclk will cause problem */
}sys_cclk_div_to_hclk_e;

/**
 *  @brief  Define rc_24M_cal enable/disable.
 */
typedef enum {
	RC_24M_CAL_DISABLE=0,
	RC_24M_CAL_ENABLE,

}rc_24M_cal_e;


/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/
extern sys_clk_t sys_clk;
extern clk_32k_type_e g_clk_32k_src;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/**
 * @brief       This function use to select the system clock source.
 * @param[in]   pll - pll clock.
 * @param[in]	src - cclk source.
 * @param[in]	cclk_div - the cclk divide from pll.it is useless if src is not PAD_PLL_DIV. cclk max is 96M
 * @param[in]	hclk_div - the hclk divide from cclk.hclk max is 48M.
 * @param[in]	pclk_div - the pclk divide from hclk.pclk max is 24M.
 * @param[in]	mspi_clk_div - mspi_clk has two source. pll div and hclk.mspi max is 64M.
 * @return      none
 */
_attribute_ram_code_sec_noinline_ void clock_init(sys_pll_clk_e pll,
		sys_clock_src_e src,
		sys_pll_div_to_cclk_e cclk_div,
		sys_cclk_div_to_hclk_e hclk_div,
		sys_hclk_div_to_pclk_e pclk_div,
		sys_pll_div_to_mspi_clk_e mspi_clk_div);

/**
 * @brief   	This function serves to set 32k clock source.
 * @param[in]   src - variable of 32k type.
 * @return  	none.
 */
void clock_32k_init(clk_32k_type_e src);

/**
 * @brief   	This function serves to kick 32k xtal.
 * @param[in]   xtal_times - kick times.
 * @return  	1 success, 0 error.
 */
unsigned char clock_kick_32k_xtal(unsigned char xtal_times);

/**
 * @brief     	This function performs to select 24M as the system clock source.
 * @return		none.
 */
_attribute_ram_code_sec_ void clock_cal_24m_rc (void);

/**
 * @brief     This function performs to select 32K as the system clock source.
 * @return    none.
 */
void clock_cal_32k_rc (void);

/**
 * @brief  This function serves to get the 32k tick.
 * @return none.
 */
_attribute_ram_code_sec_  unsigned int clock_get_32k_tick (void);

/**
 * @brief  This function serves to set the 32k tick.
 * @param  tick - the value of to be set to 32k.
 * @return none.
 */
_attribute_ram_code_sec_ void clock_set_32k_tick(unsigned int tick);
#endif

