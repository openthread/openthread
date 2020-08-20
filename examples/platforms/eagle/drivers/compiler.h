/********************************************************************************************************
 * @file     compiler.h
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

#ifndef COMPILER_H_
#define COMPILER_H_

#define _attribute_ram_code_ __attribute__((section(".ram_code"))) __attribute__((noinline))
#define _attribute_ram_code_without_oninline __attribute__((section(".ram_code")))

#define _attribute_data_retention_ __attribute__((section(".retention_data")))
/// define the static keyword for this compiler
#define __STATIC static

/// define the force inlining attribute for this compiler
#define __INLINE static __attribute__((__always_inline__)) inline

/// define the IRQ handler attribute for this compiler
#define __IRQ __attribute__((interrupt("machine"), aligned(4)))

/// define the BLE IRQ handler attribute for this compiler
#define __BTIRQ

/// define the BLE IRQ handler attribute for this compiler
#define __BLEIRQ

/// define the FIQ handler attribute for this compiler
#define __FIQ __attribute__((__interrupt__("FIQ")))

/// define size of an empty array (used to declare structure with an array size not defined)
#define __ARRAY_EMPTY

/// Function returns struct in registers (4 in rvds, var with gnuarm).
/// With Gnuarm, feature depends on command line options and
/// impacts ALL functions returning 2-words max structs
/// (check -freg-struct-return and -mabi=xxx)
#define __VIR

/// function has no side effect and return depends only on arguments
#define __PURE __attribute__((const))

/// Align instantiated lvalue or struct member on 4 bytes
#define __ALIGN4 __attribute__((aligned(4)))

/// __MODULE__ comes from the RVDS compiler that supports it
#define __MODULE__ __BASE_FILE__

/// Pack a structure field
#define __PACKED __attribute__((__packed__))

/// Put a variable in a memory maintained during deep sleep
#define __LOWPOWER_SAVED

#define ASSERT_ERR(x)

#endif
