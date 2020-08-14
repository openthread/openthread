/********************************************************************************************************
 * @file     trng_reg.h
 *
 * @brief    This is the source file for TLSR9518
 *
 * @author	 Driver Group
 * @date     March 18, 2020
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
 * 			 1.initial release(March 18, 2020)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#ifndef TRNG_REG_H_
#define TRNG_REG_H_

/*******************************      trng registers: 101800      ******************************/
#define REG_TRNG_BASE					0x101800


#define reg_trng_cr0					REG_ADDR8(REG_TRNG_BASE)
enum{
	FLD_TRNG_CR0_RBGEN		= 	BIT(0),
	FLD_TRNG_CR0_ROSEN0 	= 	BIT(1),
	FLD_TRNG_CR0_ROSEN1 	= 	BIT(2),
	FLD_TRNG_CR0_ROSEN2 	= 	BIT(3),
	FLD_TRNG_CR0_ROSEN3 	= 	BIT(4),
};

#define reg_trng_rtcr					REG_ADDR32(REG_TRNG_BASE+0x04)
enum{
	FLD_TRNG_RTCR_MSEL		= 	BIT(0),
};

#define reg_rbg_sr						REG_ADDR8(REG_TRNG_BASE+0x08)
enum{
	FLD_RBG_SR_DRDY			= 	BIT(0),
};

#define reg_rbg_dr						REG_ADDR32(REG_TRNG_BASE+0x0c)

#endif
