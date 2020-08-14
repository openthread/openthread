/********************************************************************************************************
 * @file     assert.h
 *
 * @brief    This is the header file for TLSR8278
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
 *
 *
 * @version  A001
 *
 *******************************************************************************************************/
#ifndef _ASSERT_H_
#define _ASSERT_H_

#if 1
#define assert(expression) ((void) 0)
#else
extern void bad_assertion (const char *mess);
#define  __str(x)  	# x
#define  __xstr(x)  __str(x)
#define  assert(expr)  ((expr)? (void)0 : \
		 bad_assertion("Assertion failed, file "__xstr(__FILE__)", line" __xstr(__LINE__) "\n"))
#endif
#endif
