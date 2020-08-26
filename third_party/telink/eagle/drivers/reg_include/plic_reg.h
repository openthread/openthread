/********************************************************************************************************
 * @file     interrupt_reg.h
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
#ifndef INTERRUPT_REG_H
#define INTERRUPT_REG_H
#include "../../common/bit.h"
#include "../sys.h"

/*******************************     interrupt registers:     ******************************/
#define reg_irq_feature (*(volatile unsigned long *)(0 + (0xe4000000)))

#define reg_irq_pending(i) (*(volatile unsigned long *)(0 + (0xe4001000 + ((i > 31) ? 4 : 0))))

#define reg_irq_src0 (*(volatile unsigned long *)(0 + (0xe4002000)))
#define reg_irq_src1 (*(volatile unsigned long *)(0 + (0xe4002004)))

#define reg_irq_src(i) (*(volatile unsigned long *)(0 + (0xe4002000 + ((i > 31) ? 4 : 0))))

#define reg_irq_threshold (*(volatile unsigned long *)(0 + (0xe4200000)))
#define reg_irq_done (*(volatile unsigned long *)(0 + (0xe4200004)))

#define reg_irq_src_priority(i) (*(volatile unsigned long *)(0 + 0xe4000000 + ((i) << 2)))

#endif
