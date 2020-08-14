/********************************************************************************************************
 * @file     trng.h
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
#ifndef TRNG_H_
#define TRNG_H_

#include "sys.h"
#include "reg_include/register_9518.h"

/**
 * @brief     This function performs to get one random number.If chip in suspend TRNG module should be close.
 *            else its current will be larger.
 * @param[in] none.
 * @return    the value of one random number.
 */
void trng_init(void);

/**
 * @brief     This function performs to get one random number
 * @param[in] none.
 * @return    the value of one random number.
 */
unsigned int trng_rand(void);

#endif
