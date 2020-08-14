/********************************************************************************************************
 * @file     uart.h
 *
 * @brief    This is the header file for TLSR8258
 *
 * @author	 Driver Group
 * @date     May 8, 2018
 *
 * @par      Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd.
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
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/
/**	@page SYS
 *
 *	Introduction
 *	===============
 *	Clock init and system timer delay.
 *
 *	API Reference
 *	===============
 *	Header File: sys.h
 */

#ifndef SYS_H_
#define SYS_H_
#include "reg_include/soc.h"
#include "reg_include/stimer_reg.h"
#include "../common/types.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/

#define BASE_ADDRESS   			    0
#define REG_ADDR8(a)				(*(volatile unsigned char*)(BASE_ADDRESS | (a)))
#define REG_ADDR16(a)				(*(volatile unsigned short*)(BASE_ADDRESS | (a)))
#define REG_ADDR32(a)				(*(volatile unsigned long*)(BASE_ADDRESS | (a)))

#define write_reg8(addr,v)			(*(volatile unsigned char*)(BASE_ADDRESS | (addr)) = (unsigned char)(v))
#define write_reg16(addr,v)			(*(volatile unsigned short*)(BASE_ADDRESS | (addr)) = (unsigned short)(v))
#define write_reg32(addr,v)			(*(volatile unsigned long*)(BASE_ADDRESS | (addr)) = (unsigned long)(v))

#define read_reg8(addr)				 (*(volatile unsigned char*)(BASE_ADDRESS | (addr)))
#define read_reg16(addr)              (*(volatile unsigned short*)(BASE_ADDRESS | (addr)))
#define read_reg32(addr)              (*(volatile unsigned long*)(BASE_ADDRESS | (addr)))


/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**
 *  @brief  Define sys_clk struct.
 */
typedef struct SYS_CLK{
	unsigned short pll_clk;		/**< pll clk */
	unsigned char cclk;			/**< cpu clk */
	unsigned char hclk;			/**< hclk */
	unsigned char pclk;			/**< pclk */
	unsigned char mspi_clk;		/**< mspi_clk */
	unsigned char stimer_clk;	/**< stimer_clk */
}sys_clk_s;

/**
 * @brief 	Power type for different application
 */
typedef enum{
	LDO_MODE 		=0x40,	/**< LDO mode */
	DCDC_LDO_MODE	=0x41,	/**< DCDC_LDO mode */
	DCDC_MODE		=0x43,	/**< DCDC mode (16pin is not suported this mode. */
}power_mode_e;

/**
 * @brief system clock type
 */
typedef enum{                                        /**<     <1:0>         <2:6>   <15:8> */
	PLL_CLK_48M 	= (0 | (16 << 2) | (48 << 8)),   /**< ana_09<3:2> | ana_80<4:0> | clk  */
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
 * @brief pll div to mspi_clk.
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
 * @brief cclk div to hclk.
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
	CCLK_DIV2_TO_HCLK    =    2,
}sys_cclk_div_to_hclk_e;


/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/
extern sys_clk_s sys_clk;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/*
 * @brief     This function performs to enable system timer and 32K calibration.
 * @param[in] none.
 * @return    system timer tick value.
**/
static inline  void  sys_clock_time_en(void)
{
reg_system_ctrl|=FLD_SYSTEM_TIMER_EN|FLD_SYSTEM_32K_CAL_EN ;
}


/*
 * @brief     This function performs to get system timer tick.
 * @param[in] none.
 * @return    system timer tick value.
**/
static inline u32 sys_get_stimer_tick(void)
{

	return reg_system_tick;
}


/**
 * @brief     This function serves to set timeout by us.
 * @param[in] ref - reference tick of system timer .
 * @param[in] us  - count by us.
 * @return    true - timeout, false - not timeout
 */
static inline _Bool sys_timeout(unsigned int ref, unsigned int us)
{
	return ((unsigned int)(sys_get_stimer_tick() - ref) > us * 16);
}

/**
 * @brief   	This function serves to initialize system.
 * @param[in]	power_mode - power mode(LDO/DCDC/LDO_DCDC)
 * @return  	none
 */
void sys_init(power_mode_e power_mode);


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
void clock_init(sys_pll_clk_e pll,
		sys_clock_src_e src,
		sys_pll_div_to_cclk_e pll_div,
		sys_cclk_div_to_hclk_e cclk_div,
		sys_hclk_div_to_pclk_e pclk_div,
		sys_pll_div_to_mspi_clk_e mspi_clk_div);




/**
 * @brief     This function performs to set delay time by us.
 * @param[in] microsec - need to delay.
 * @return    none
 */
void delay_us(u32 microsec);

/**
 * @brief     This function performs to set delay time by ms.
 * @param[in] millisec - need to delay.
 * @return    none
 */
void delay_ms(u32 millisec);



static inline u32 clock_time(void)
{

	return reg_system_tick;
}

static inline _Bool clock_time_exceed(unsigned int ref, unsigned int us)
{
	return ((unsigned int)(sys_get_stimer_tick() - ref) > us * 16);
}


#endif
