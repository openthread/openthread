/********************************************************************************************************
 * @file	trng.h
 *
 * @brief	This is the header file for B91
 *
 * @author	B.Y
 * @date	2019
 *
 * @par		Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
 *			All rights reserved.
 *
 *			The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or  
 *          alter) any information contained herein in whole or in part except as expressly authorized  
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible  
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)  
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
/**	@page TRNG
 *
 *	Introduction
 *	===============
 *	TLSRB91 supports hardware trng. 
 *
 *	API Reference
 *	===============
 *	Header File: trng.h
 */
#ifndef TRNG_H_
#define TRNG_H_

#include "sys.h"
#include "reg_include/register_b91.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/**
 * @brief     This function performs to get one random number.If chip in suspend TRNG module should be close.
 *            else its current will be larger.
 * @return    none
 **/
void trng_init(void);

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number
 **/
unsigned int trng_rand(void);

#endif
