/********************************************************************************************************
 * @file     core.h
 *
 * @brief    This is the header file for TLSR9518
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
#ifndef CORE_H
#define CORE_H
#include "nds_intrinsic.h"
#include "../common/bit.h"
#include "../common/types.h"
#define read_csr(reg) __nds__csrr(reg)
#define write_csr(reg, val) __nds__csrw(val, reg)
#define swap_csr(reg, val) __nds__csrrw(val, reg)
#define set_csr(reg, bit) __nds__csrrs(bit, reg)
#define clear_csr(reg, bit) __nds__csrrc(bit, reg)

typedef enum
{
    FLD_FEATURE_PREEMPT_PRIORITY_INT_EN = BIT(0),
    FLD_FEATURE_VECTOR_MODE_EN          = BIT(1),
} feature_e;
/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
void core_enable_interrupt(void);

/* Disable the Machine external, timer and software interrupts until setup is done */
/** @brief Disable interrupts globally in the system.
 * This function must be used when the system wants to disable all the interrupt
 * it could handle.
 */
u32 core_disable_interrupt(void);

u32 core_restore_interrupt(u32 en);
#endif
