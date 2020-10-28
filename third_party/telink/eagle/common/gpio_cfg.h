/********************************************************************************************************
 * @file	gpio_cfg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	X.P.C
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
#ifndef DRIVERS_GPIO_CFG_H_
#define DRIVERS_GPIO_CFG_H_
/**********************************************************************************************************************
 *                                           GPIO   setting                                                            *
 *********************************************************************************************************************/

/************************************************************PA*******************************************************/
#ifndef PA0_INPUT_ENABLE
#define PA0_INPUT_ENABLE	0
#endif
#ifndef PA1_INPUT_ENABLE
#define PA1_INPUT_ENABLE	0
#endif
#ifndef PA2_INPUT_ENABLE
#define PA2_INPUT_ENABLE	0
#endif
#ifndef PA3_INPUT_ENABLE
#define PA3_INPUT_ENABLE	0
#endif
#ifndef PA4_INPUT_ENABLE
#define PA4_INPUT_ENABLE	0
#endif
#ifndef PA5_INPUT_ENABLE
#define PA5_INPUT_ENABLE	0	//USB
#endif
#ifndef PA6_INPUT_ENABLE
#define PA6_INPUT_ENABLE	0	//USB
#endif
#ifndef PA7_INPUT_ENABLE
#define PA7_INPUT_ENABLE	1	//SWS
#endif
#ifndef PA0_OUTPUT_ENABLE
#define PA0_OUTPUT_ENABLE	0
#endif
#ifndef PA1_OUTPUT_ENABLE
#define PA1_OUTPUT_ENABLE	0
#endif
#ifndef PA2_OUTPUT_ENABLE
#define PA2_OUTPUT_ENABLE	0
#endif
#ifndef PA3_OUTPUT_ENABLE
#define PA3_OUTPUT_ENABLE	0
#endif
#ifndef PA4_OUTPUT_ENABLE
#define PA4_OUTPUT_ENABLE	0
#endif
#ifndef PA5_OUTPUT_ENABLE
#define PA5_OUTPUT_ENABLE	0
#endif
#ifndef PA6_OUTPUT_ENABLE
#define PA6_OUTPUT_ENABLE	0
#endif
#ifndef PA7_OUTPUT_ENABLE
#define PA7_OUTPUT_ENABLE	0
#endif
#ifndef PA0_DATA_STRENGTH
#define PA0_DATA_STRENGTH	1
#endif
#ifndef PA1_DATA_STRENGTH
#define PA1_DATA_STRENGTH	1
#endif
#ifndef PA2_DATA_STRENGTH
#define PA2_DATA_STRENGTH	1
#endif
#ifndef PA3_DATA_STRENGTH
#define PA3_DATA_STRENGTH	1
#endif
#ifndef PA4_DATA_STRENGTH
#define PA4_DATA_STRENGTH	1
#endif
#ifndef PA5_DATA_STRENGTH
#define PA5_DATA_STRENGTH	1
#endif
#ifndef PA6_DATA_STRENGTH
#define PA6_DATA_STRENGTH	1
#endif
#ifndef PA7_DATA_STRENGTH
#define PA7_DATA_STRENGTH	1
#endif
#ifndef PA0_DATA_OUT
#define PA0_DATA_OUT	0
#endif
#ifndef PA1_DATA_OUT
#define PA1_DATA_OUT	0
#endif
#ifndef PA2_DATA_OUT
#define PA2_DATA_OUT	0
#endif
#ifndef PA3_DATA_OUT
#define PA3_DATA_OUT	0
#endif
#ifndef PA4_DATA_OUT
#define PA4_DATA_OUT	0
#endif
#ifndef PA5_DATA_OUT
#define PA5_DATA_OUT	0
#endif
#ifndef PA6_DATA_OUT
#define PA6_DATA_OUT	0
#endif
#ifndef PA7_DATA_OUT
#define PA7_DATA_OUT	0
#endif
#ifndef PA0_FUNC
#define PA0_FUNC	AS_GPIO
#endif
#ifndef PA1_FUNC
#define PA1_FUNC	AS_GPIO
#endif
#ifndef PA2_FUNC
#define PA2_FUNC	AS_GPIO
#endif
#ifndef PA3_FUNC
#define PA3_FUNC	AS_GPIO
#endif
#ifndef PA4_FUNC
#define PA4_FUNC	AS_GPIO
#endif
#ifndef PA5_FUNC
#define PA5_FUNC	AS_GPIO
#endif
#ifndef PA6_FUNC
#define PA6_FUNC	AS_GPIO
#endif
#ifndef PA7_FUNC
#define PA7_FUNC	AS_SWS
#endif
#ifndef PULL_WAKEUP_SRC_PA0
#define PULL_WAKEUP_SRC_PA0	0
#endif
#ifndef PULL_WAKEUP_SRC_PA1
#define PULL_WAKEUP_SRC_PA1	0
#endif
#ifndef PULL_WAKEUP_SRC_PA2
#define PULL_WAKEUP_SRC_PA2	0
#endif
#ifndef PULL_WAKEUP_SRC_PA3
#define PULL_WAKEUP_SRC_PA3	0
#endif
#ifndef PULL_WAKEUP_SRC_PA4
#define PULL_WAKEUP_SRC_PA4	0
#endif
#ifndef PULL_WAKEUP_SRC_PA5
#define PULL_WAKEUP_SRC_PA5	0
#endif
#ifndef PULL_WAKEUP_SRC_PA6
#define PULL_WAKEUP_SRC_PA6	0
#endif
#ifndef PULL_WAKEUP_SRC_PA7
#define PULL_WAKEUP_SRC_PA7		GPIO_PIN_PULLUP_1M	//sws pullup
#endif

/************************************************************PB*******************************************************/
#ifndef PB0_INPUT_ENABLE
#define PB0_INPUT_ENABLE	0
#endif
#ifndef PB1_INPUT_ENABLE
#define PB1_INPUT_ENABLE	0
#endif
#ifndef PB2_INPUT_ENABLE
#define PB2_INPUT_ENABLE	0
#endif
#ifndef PB3_INPUT_ENABLE
#define PB3_INPUT_ENABLE	0
#endif
#ifndef PB4_INPUT_ENABLE
#define PB4_INPUT_ENABLE	0
#endif
#ifndef PB5_INPUT_ENABLE
#define PB5_INPUT_ENABLE	0
#endif
#ifndef PB6_INPUT_ENABLE
#define PB6_INPUT_ENABLE	0
#endif
#ifndef PB7_INPUT_ENABLE
#define PB7_INPUT_ENABLE	0
#endif
#ifndef PB0_OUTPUT_ENABLE
#define PB0_OUTPUT_ENABLE	0
#endif
#ifndef PB1_OUTPUT_ENABLE
#define PB1_OUTPUT_ENABLE	0
#endif
#ifndef PB2_OUTPUT_ENABLE
#define PB2_OUTPUT_ENABLE	0
#endif
#ifndef PB3_OUTPUT_ENABLE
#define PB3_OUTPUT_ENABLE	0
#endif
#ifndef PB4_OUTPUT_ENABLE
#define PB4_OUTPUT_ENABLE	0
#endif
#ifndef PB5_OUTPUT_ENABLE
#define PB5_OUTPUT_ENABLE	0
#endif
#ifndef PB6_OUTPUT_ENABLE
#define PB6_OUTPUT_ENABLE	0
#endif
#ifndef PB7_OUTPUT_ENABLE
#define PB7_OUTPUT_ENABLE	0
#endif
#ifndef PB0_DATA_STRENGTH
#define PB0_DATA_STRENGTH	1
#endif
#ifndef PB1_DATA_STRENGTH
#define PB1_DATA_STRENGTH	1
#endif
#ifndef PB2_DATA_STRENGTH
#define PB2_DATA_STRENGTH	1
#endif
#ifndef PB3_DATA_STRENGTH
#define PB3_DATA_STRENGTH	1
#endif
#ifndef PB4_DATA_STRENGTH
#define PB4_DATA_STRENGTH	1
#endif
#ifndef PB5_DATA_STRENGTH
#define PB5_DATA_STRENGTH	1
#endif
#ifndef PB6_DATA_STRENGTH
#define PB6_DATA_STRENGTH	1
#endif
#ifndef PB7_DATA_STRENGTH
#define PB7_DATA_STRENGTH	1
#endif
#ifndef PB0_DATA_OUT
#define PB0_DATA_OUT	0
#endif
#ifndef PB1_DATA_OUT
#define PB1_DATA_OUT	0
#endif
#ifndef PB2_DATA_OUT
#define PB2_DATA_OUT	0
#endif
#ifndef PB3_DATA_OUT
#define PB3_DATA_OUT	0
#endif
#ifndef PB4_DATA_OUT
#define PB4_DATA_OUT	0
#endif
#ifndef PB5_DATA_OUT
#define PB5_DATA_OUT	0
#endif
#ifndef PB6_DATA_OUT
#define PB6_DATA_OUT	0
#endif
#ifndef PB7_DATA_OUT
#define PB7_DATA_OUT	0
#endif
#ifndef PB0_FUNC
#define PB0_FUNC	AS_GPIO
#endif
#ifndef PB1_FUNC
#define PB1_FUNC	AS_GPIO
#endif
#ifndef PB2_FUNC
#define PB2_FUNC	AS_GPIO
#endif
#ifndef PB3_FUNC
#define PB3_FUNC	AS_GPIO
#endif
#ifndef PB4_FUNC
#define PB4_FUNC	AS_GPIO
#endif
#ifndef PB5_FUNC
#define PB5_FUNC	AS_GPIO
#endif
#ifndef PB6_FUNC
#define PB6_FUNC	AS_GPIO
#endif
#ifndef PB7_FUNC
#define PB7_FUNC	AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PB0
#define PULL_WAKEUP_SRC_PB0	0
#endif
#ifndef PULL_WAKEUP_SRC_PB1
#define PULL_WAKEUP_SRC_PB1	0
#endif
#ifndef PULL_WAKEUP_SRC_PB2
#define PULL_WAKEUP_SRC_PB2	0
#endif
#ifndef PULL_WAKEUP_SRC_PB3
#define PULL_WAKEUP_SRC_PB3	0
#endif
#ifndef PULL_WAKEUP_SRC_PB4
#define PULL_WAKEUP_SRC_PB4	0
#endif
#ifndef PULL_WAKEUP_SRC_PB5
#define PULL_WAKEUP_SRC_PB5	0
#endif
#ifndef PULL_WAKEUP_SRC_PB6
#define PULL_WAKEUP_SRC_PB6	0
#endif
#ifndef PULL_WAKEUP_SRC_PB7
#define PULL_WAKEUP_SRC_PB7	0
#endif

/************************************************************PC*******************************************************/
#ifndef PC0_INPUT_ENABLE
#define PC0_INPUT_ENABLE	0
#endif
#ifndef PC1_INPUT_ENABLE
#define PC1_INPUT_ENABLE	0
#endif
#ifndef PC2_INPUT_ENABLE
#define PC2_INPUT_ENABLE	0
#endif
#ifndef PC3_INPUT_ENABLE
#define PC3_INPUT_ENABLE	0
#endif
#ifndef PC4_INPUT_ENABLE
#define PC4_INPUT_ENABLE	0
#endif
#ifndef PC5_INPUT_ENABLE
#define PC5_INPUT_ENABLE	0
#endif
#ifndef PC6_INPUT_ENABLE
#define PC6_INPUT_ENABLE	0
#endif
#ifndef PC7_INPUT_ENABLE
#define PC7_INPUT_ENABLE	0
#endif
#ifndef PC0_OUTPUT_ENABLE
#define PC0_OUTPUT_ENABLE	0
#endif
#ifndef PC1_OUTPUT_ENABLE
#define PC1_OUTPUT_ENABLE	0
#endif
#ifndef PC2_OUTPUT_ENABLE
#define PC2_OUTPUT_ENABLE	0
#endif
#ifndef PC3_OUTPUT_ENABLE
#define PC3_OUTPUT_ENABLE	0
#endif
#ifndef PC4_OUTPUT_ENABLE
#define PC4_OUTPUT_ENABLE	0
#endif
#ifndef PC5_OUTPUT_ENABLE
#define PC5_OUTPUT_ENABLE	0
#endif
#ifndef PC6_OUTPUT_ENABLE
#define PC6_OUTPUT_ENABLE	0
#endif
#ifndef PC7_OUTPUT_ENABLE
#define PC7_OUTPUT_ENABLE	0
#endif
#ifndef PC0_DATA_STRENGTH
#define PC0_DATA_STRENGTH	1
#endif
#ifndef PC1_DATA_STRENGTH
#define PC1_DATA_STRENGTH	1
#endif
#ifndef PC2_DATA_STRENGTH
#define PC2_DATA_STRENGTH	1
#endif
#ifndef PC3_DATA_STRENGTH
#define PC3_DATA_STRENGTH	1
#endif
#ifndef PC4_DATA_STRENGTH
#define PC4_DATA_STRENGTH	1
#endif
#ifndef PC5_DATA_STRENGTH
#define PC5_DATA_STRENGTH	1
#endif
#ifndef PC6_DATA_STRENGTH
#define PC6_DATA_STRENGTH	1
#endif
#ifndef PC7_DATA_STRENGTH
#define PC7_DATA_STRENGTH	1
#endif
#ifndef PC0_DATA_OUT
#define PC0_DATA_OUT	0
#endif
#ifndef PC1_DATA_OUT
#define PC1_DATA_OUT	0
#endif
#ifndef PC2_DATA_OUT
#define PC2_DATA_OUT	0
#endif
#ifndef PC3_DATA_OUT
#define PC3_DATA_OUT	0
#endif
#ifndef PC4_DATA_OUT
#define PC4_DATA_OUT	0
#endif
#ifndef PC5_DATA_OUT
#define PC5_DATA_OUT	0
#endif
#ifndef PC6_DATA_OUT
#define PC6_DATA_OUT	0
#endif
#ifndef PC7_DATA_OUT
#define PC7_DATA_OUT	0
#endif
#ifndef PC0_FUNC
#define PC0_FUNC	AS_GPIO
#endif
#ifndef PC1_FUNC
#define PC1_FUNC	AS_GPIO
#endif
#ifndef PC2_FUNC
#define PC2_FUNC	AS_GPIO
#endif
#ifndef PC3_FUNC
#define PC3_FUNC	AS_GPIO
#endif
#ifndef PC4_FUNC
#define PC4_FUNC	AS_GPIO
#endif
#ifndef PC5_FUNC
#define PC5_FUNC	AS_GPIO
#endif
#ifndef PC6_FUNC
#define PC6_FUNC	AS_GPIO
#endif
#ifndef PC7_FUNC
#define PC7_FUNC	AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PC0
#define PULL_WAKEUP_SRC_PC0	0
#endif
#ifndef PULL_WAKEUP_SRC_PC1
#define PULL_WAKEUP_SRC_PC1	0
#endif
#ifndef PULL_WAKEUP_SRC_PC2
#define PULL_WAKEUP_SRC_PC2	0
#endif
#ifndef PULL_WAKEUP_SRC_PC3
#define PULL_WAKEUP_SRC_PC3	0
#endif
#ifndef PULL_WAKEUP_SRC_PC4
#define PULL_WAKEUP_SRC_PC4	0
#endif
#ifndef PULL_WAKEUP_SRC_PC5
#define PULL_WAKEUP_SRC_PC5	0
#endif
#ifndef PULL_WAKEUP_SRC_PC6
#define PULL_WAKEUP_SRC_PC6	0
#endif
#ifndef PULL_WAKEUP_SRC_PC7
#define PULL_WAKEUP_SRC_PC7	0
#endif

/************************************************************PD*******************************************************/
#ifndef PD0_INPUT_ENABLE
#define PD0_INPUT_ENABLE	0
#endif
#ifndef PD1_INPUT_ENABLE
#define PD1_INPUT_ENABLE	0
#endif
#ifndef PD2_INPUT_ENABLE
#define PD2_INPUT_ENABLE	0
#endif
#ifndef PD3_INPUT_ENABLE
#define PD3_INPUT_ENABLE	0
#endif
#ifndef PD4_INPUT_ENABLE
#define PD4_INPUT_ENABLE	0
#endif
#ifndef PD5_INPUT_ENABLE
#define PD5_INPUT_ENABLE	0
#endif
#ifndef PD6_INPUT_ENABLE
#define PD6_INPUT_ENABLE	0
#endif
#ifndef PD7_INPUT_ENABLE
#define PD7_INPUT_ENABLE	0
#endif
#ifndef PD0_OUTPUT_ENABLE
#define PD0_OUTPUT_ENABLE	0
#endif
#ifndef PD1_OUTPUT_ENABLE
#define PD1_OUTPUT_ENABLE	0
#endif
#ifndef PD2_OUTPUT_ENABLE
#define PD2_OUTPUT_ENABLE	0
#endif
#ifndef PD3_OUTPUT_ENABLE
#define PD3_OUTPUT_ENABLE	0
#endif
#ifndef PD4_OUTPUT_ENABLE
#define PD4_OUTPUT_ENABLE	0
#endif
#ifndef PD5_OUTPUT_ENABLE
#define PD5_OUTPUT_ENABLE	0
#endif
#ifndef PD6_OUTPUT_ENABLE
#define PD6_OUTPUT_ENABLE	0
#endif
#ifndef PD7_OUTPUT_ENABLE
#define PD7_OUTPUT_ENABLE	0
#endif
#ifndef PD0_DATA_STRENGTH
#define PD0_DATA_STRENGTH	1
#endif
#ifndef PD1_DATA_STRENGTH
#define PD1_DATA_STRENGTH	1
#endif
#ifndef PD2_DATA_STRENGTH
#define PD2_DATA_STRENGTH	1
#endif
#ifndef PD3_DATA_STRENGTH
#define PD3_DATA_STRENGTH	1
#endif
#ifndef PD4_DATA_STRENGTH
#define PD4_DATA_STRENGTH	1
#endif
#ifndef PD5_DATA_STRENGTH
#define PD5_DATA_STRENGTH	1
#endif
#ifndef PD6_DATA_STRENGTH
#define PD6_DATA_STRENGTH	1
#endif
#ifndef PD7_DATA_STRENGTH
#define PD7_DATA_STRENGTH	1
#endif
#ifndef PD0_DATA_OUT
#define PD0_DATA_OUT	0
#endif
#ifndef PD1_DATA_OUT
#define PD1_DATA_OUT	0
#endif
#ifndef PD2_DATA_OUT
#define PD2_DATA_OUT	0
#endif
#ifndef PD3_DATA_OUT
#define PD3_DATA_OUT	0
#endif
#ifndef PD4_DATA_OUT
#define PD4_DATA_OUT	0
#endif
#ifndef PD5_DATA_OUT
#define PD5_DATA_OUT	0
#endif
#ifndef PD6_DATA_OUT
#define PD6_DATA_OUT	0
#endif
#ifndef PD7_DATA_OUT
#define PD7_DATA_OUT	0
#endif
#ifndef PD0_FUNC
#define PD0_FUNC	AS_GPIO
#endif
#ifndef PD1_FUNC
#define PD1_FUNC	AS_GPIO
#endif
#ifndef PD2_FUNC
#define PD2_FUNC	AS_GPIO
#endif
#ifndef PD3_FUNC
#define PD3_FUNC	AS_GPIO
#endif
#ifndef PD4_FUNC
#define PD4_FUNC	AS_GPIO
#endif
#ifndef PD5_FUNC
#define PD5_FUNC	AS_GPIO
#endif
#ifndef PD6_FUNC
#define PD6_FUNC	AS_GPIO
#endif
#ifndef PD7_FUNC
#define PD7_FUNC	AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PD0
#define PULL_WAKEUP_SRC_PD0	0
#endif
#ifndef PULL_WAKEUP_SRC_PD1
#define PULL_WAKEUP_SRC_PD1	0
#endif
#ifndef PULL_WAKEUP_SRC_PD2
#define PULL_WAKEUP_SRC_PD2	0
#endif
#ifndef PULL_WAKEUP_SRC_PD3
#define PULL_WAKEUP_SRC_PD3	0
#endif
#ifndef PULL_WAKEUP_SRC_PD4
#define PULL_WAKEUP_SRC_PD4	0
#endif
#ifndef PULL_WAKEUP_SRC_PD5
#define PULL_WAKEUP_SRC_PD5	0
#endif
#ifndef PULL_WAKEUP_SRC_PD6
#define PULL_WAKEUP_SRC_PD6	0
#endif
#ifndef PULL_WAKEUP_SRC_PD7
#define PULL_WAKEUP_SRC_PD7	0
#endif

/************************************************************PE*******************************************************/
#ifndef PE0_INPUT_ENABLE
#define PE0_INPUT_ENABLE	0
#endif
#ifndef PE1_INPUT_ENABLE
#define PE1_INPUT_ENABLE	0
#endif
#ifndef PE2_INPUT_ENABLE
#define PE2_INPUT_ENABLE	0
#endif
#ifndef PE3_INPUT_ENABLE
#define PE3_INPUT_ENABLE	0
#endif
#ifndef PE4_INPUT_ENABLE
#define PE4_INPUT_ENABLE	1	//JTAG
#endif
#ifndef PE5_INPUT_ENABLE
#define PE5_INPUT_ENABLE	1	//JTAG
#endif
#ifndef PE6_INPUT_ENABLE
#define PE6_INPUT_ENABLE	1	//JTAG
#endif
#ifndef PE7_INPUT_ENABLE
#define PE7_INPUT_ENABLE	1	//JTAG
#endif
#ifndef PE0_OUTPUT_ENABLE
#define PE0_OUTPUT_ENABLE	0
#endif
#ifndef PE1_OUTPUT_ENABLE
#define PE1_OUTPUT_ENABLE	0
#endif
#ifndef PE2_OUTPUT_ENABLE
#define PE2_OUTPUT_ENABLE	0
#endif
#ifndef PE3_OUTPUT_ENABLE
#define PE3_OUTPUT_ENABLE	0
#endif
#ifndef PE4_OUTPUT_ENABLE
#define PE4_OUTPUT_ENABLE	0
#endif
#ifndef PE5_OUTPUT_ENABLE
#define PE5_OUTPUT_ENABLE	0
#endif
#ifndef PE6_OUTPUT_ENABLE
#define PE6_OUTPUT_ENABLE	0
#endif
#ifndef PE7_OUTPUT_ENABLE
#define PE7_OUTPUT_ENABLE	0
#endif
#ifndef PE0_DATA_STRENGTH
#define PE0_DATA_STRENGTH	1
#endif
#ifndef PE1_DATA_STRENGTH
#define PE1_DATA_STRENGTH	1
#endif
#ifndef PE2_DATA_STRENGTH
#define PE2_DATA_STRENGTH	1
#endif
#ifndef PE3_DATA_STRENGTH
#define PE3_DATA_STRENGTH	1
#endif
#ifndef PE4_DATA_STRENGTH
#define PE4_DATA_STRENGTH	1
#endif
#ifndef PE5_DATA_STRENGTH
#define PE5_DATA_STRENGTH	1
#endif
#ifndef PE6_DATA_STRENGTH
#define PE6_DATA_STRENGTH	1
#endif
#ifndef PE7_DATA_STRENGTH
#define PE7_DATA_STRENGTH	1
#endif
#ifndef PE0_DATA_OUT
#define PE0_DATA_OUT	0
#endif
#ifndef PE1_DATA_OUT
#define PE1_DATA_OUT	0
#endif
#ifndef PE2_DATA_OUT
#define PE2_DATA_OUT	0
#endif
#ifndef PE3_DATA_OUT
#define PE3_DATA_OUT	0
#endif
#ifndef PE4_DATA_OUT
#define PE4_DATA_OUT	0
#endif
#ifndef PE5_DATA_OUT
#define PE5_DATA_OUT	0
#endif
#ifndef PE6_DATA_OUT
#define PE6_DATA_OUT	0
#endif
#ifndef PE7_DATA_OUT
#define PE7_DATA_OUT	0
#endif
#ifndef PE0_FUNC
#define PE0_FUNC	AS_GPIO
#endif
#ifndef PE1_FUNC
#define PE1_FUNC	AS_GPIO
#endif
#ifndef PE2_FUNC
#define PE2_FUNC	AS_GPIO
#endif
#ifndef PE3_FUNC
#define PE3_FUNC	AS_GPIO
#endif
#ifndef PE4_FUNC
#define PE4_FUNC	AS_TDI
#endif
#ifndef PE5_FUNC
#define PE5_FUNC	AS_TDO
#endif
#ifndef PE6_FUNC
#define PE6_FUNC	AS_TMS
#endif
#ifndef PE7_FUNC
#define PE7_FUNC	AS_TCK
#endif
#ifndef PULL_WAKEUP_SRC_PE0
#define PULL_WAKEUP_SRC_PE0	0
#endif
#ifndef PULL_WAKEUP_SRC_PE1
#define PULL_WAKEUP_SRC_PE1	0
#endif
#ifndef PULL_WAKEUP_SRC_PE2
#define PULL_WAKEUP_SRC_PE2	0
#endif
#ifndef PULL_WAKEUP_SRC_PE3
#define PULL_WAKEUP_SRC_PE3	0
#endif
#ifndef PULL_WAKEUP_SRC_PE4
#define PULL_WAKEUP_SRC_PE4	0
#endif
#ifndef PULL_WAKEUP_SRC_PE5
#define PULL_WAKEUP_SRC_PE5	0
#endif
#ifndef PULL_WAKEUP_SRC_PE6
#define PULL_WAKEUP_SRC_PE6	0
#endif
#ifndef PULL_WAKEUP_SRC_PE7
#define PULL_WAKEUP_SRC_PE7	0
#endif
/************************************************************PF*******************************************************/
#ifndef PF0_INPUT_ENABLE
#define PF0_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF1_INPUT_ENABLE
#define PF1_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF2_INPUT_ENABLE
#define PF2_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF3_INPUT_ENABLE
#define PF3_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF4_INPUT_ENABLE
#define PF4_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF5_INPUT_ENABLE
#define PF5_INPUT_ENABLE	1   //MSPI
#endif
#ifndef PF0_OUTPUT_ENABLE
#define PF0_OUTPUT_ENABLE	0
#endif
#ifndef PF1_OUTPUT_ENABLE
#define PF1_OUTPUT_ENABLE	0
#endif
#ifndef PF2_OUTPUT_ENABLE
#define PF2_OUTPUT_ENABLE	0
#endif
#ifndef PF3_OUTPUT_ENABLE
#define PF3_OUTPUT_ENABLE	0
#endif
#ifndef PF4_OUTPUT_ENABLE
#define PF4_OUTPUT_ENABLE	0
#endif
#ifndef PF5_OUTPUT_ENABLE
#define PF5_OUTPUT_ENABLE	0
#endif
#ifndef PF0_DATA_STRENGTH
#define PF0_DATA_STRENGTH	1
#endif
#ifndef PF1_DATA_STRENGTH
#define PF1_DATA_STRENGTH	1
#endif
#ifndef PF2_DATA_STRENGTH
#define PF2_DATA_STRENGTH	1
#endif
#ifndef PF3_DATA_STRENGTH
#define PF3_DATA_STRENGTH	1
#endif
#ifndef PF4_DATA_STRENGTH
#define PF4_DATA_STRENGTH	1
#endif
#ifndef PF5_DATA_STRENGTH
#define PF5_DATA_STRENGTH	1
#endif
#ifndef PF0_DATA_OUT
#define PF0_DATA_OUT	0
#endif
#ifndef PF1_DATA_OUT
#define PF1_DATA_OUT	0
#endif
#ifndef PF2_DATA_OUT
#define PF2_DATA_OUT	0
#endif
#ifndef PF3_DATA_OUT
#define PF3_DATA_OUT	0
#endif
#ifndef PF4_DATA_OUT
#define PF4_DATA_OUT	0
#endif
#ifndef PF5_DATA_OUT
#define PF5_DATA_OUT	0
#endif
#ifndef PF0_FUNC
#define PF0_FUNC	AS_MSPI
#endif
#ifndef PF1_FUNC
#define PF1_FUNC	AS_MSPI
#endif
#ifndef PF2_FUNC
#define PF2_FUNC	AS_MSPI
#endif
#ifndef PF3_FUNC
#define PF3_FUNC	AS_MSPI
#endif
#ifndef PF4_FUNC
#define PF4_FUNC	AS_MSPI
#endif
#ifndef PF5_FUNC
#define PF5_FUNC	AS_MSPI
#endif


/**********************************************************************************************************************
 *                                    related  	macro   function                                                      *
 *********************************************************************************************************************/
/**
 * @brief      This function servers to set the structure, which is used for gpio initialization setting, as the default value.
 * @param[in]  st  -  structure of gpio initialization settings
 */
#define set_gpio_struct_as_default(s)	\
	do{\
		s.PA.setting1 = 0x00ffff00;s.PA.setting2 = 0x00ff6100;s.PA.fs.port = 0x0000;s.PA.pull.port = 0x0000;\
		s.PB.setting1 = 0x00ffff00;s.PB.setting2 = 0x00ffff00;s.PB.fs.port = 0x0000;s.PB.pull.port = 0x0000;\
		s.PC.setting1 = 0x00ffff00;s.PC.setting2 = 0x00ffff00;s.PC.fs.port = 0x0000;s.PC.pull.port = 0x0000;\
		s.PD.setting1 = 0x00ffff00;s.PD.setting2 = 0x00ffff00;s.PD.fs.port = 0xf000;s.PD.pull.port = 0x0000;\
		s.PE.setting1 = 0x00ffff00;s.PE.setting2 = 0x00ff0f00;s.PE.fs.port = 0x0000;s.PE.pull.port = 0x0000;\
		s.PF.setting1 = 0x00ffff00;s.PF.setting2 = 0x00ff0000;s.PF.fs.port = 0x0000;\
	}while(0)

/**
 * @brief      This function servers to set the structure, which is used for gpio initialization setting, as the value defined
 * 			   in this file.
 * @param[in]  st  -  structure of gpio initialization settings
 */
#define set_gpio_struct_from_file(s)	\
	do{\
		/*******************PA********************/\
		s.PA.setting1 = \
		(PA0_INPUT_ENABLE<<8) 	| (PA1_INPUT_ENABLE<<9)	| (PA2_INPUT_ENABLE<<10)	| (PA3_INPUT_ENABLE<<11) |\
		(PA4_INPUT_ENABLE<<12)	| (PA5_INPUT_ENABLE<<13)	| (PA6_INPUT_ENABLE<<14)	| (PA7_INPUT_ENABLE<<15) |\
		((PA0_OUTPUT_ENABLE?0:1)<<16)	| ((PA1_OUTPUT_ENABLE?0:1)<<17) | ((PA2_OUTPUT_ENABLE?0:1)<<18)	| ((PA3_OUTPUT_ENABLE?0:1)<<19) |\
		((PA4_OUTPUT_ENABLE?0:1)<<20)	| ((PA5_OUTPUT_ENABLE?0:1)<<21) | ((PA6_OUTPUT_ENABLE?0:1)<<22)	| ((PA7_OUTPUT_ENABLE?0:1)<<23) |\
		(PA0_DATA_OUT<<24)	| (PA1_DATA_OUT<<25)	| (PA2_DATA_OUT<<26)	| (PA3_DATA_OUT<<27) |\
		(PA4_DATA_OUT<<28)	| (PA5_DATA_OUT<<29)	| (PA6_DATA_OUT<<30)	| (PA7_DATA_OUT<<31) ;\
		s.PA.setting2 = \
		(PA0_DATA_STRENGTH<<8)		| (PA1_DATA_STRENGTH<<9)| (PA2_DATA_STRENGTH<<10)	| (PA3_DATA_STRENGTH<<11) |\
		(PA4_DATA_STRENGTH<<12)	| (PA5_DATA_STRENGTH<<13)	| (PA6_DATA_STRENGTH<<14)	| (PA7_DATA_STRENGTH<<15) |\
		(PA0_FUNC==AS_GPIO ? BIT(16):0)	| (PA1_FUNC==AS_GPIO ? BIT(17):0)| (PA2_FUNC==AS_GPIO ? BIT(18):0)| (PA3_FUNC==AS_GPIO ? BIT(19):0) |\
		(PA4_FUNC==AS_GPIO ? BIT(20):0)	| (PA5_FUNC==AS_GPIO ? BIT(21):0)| (PA6_FUNC==AS_GPIO ? BIT(22):0)| (PA7_FUNC==AS_GPIO ? BIT(23):0);\
		s.PA.pull.port = \
		PULL_WAKEUP_SRC_PA0 | (PULL_WAKEUP_SRC_PA1<<2) | (PULL_WAKEUP_SRC_PA2<<4) | (PULL_WAKEUP_SRC_PA3<<6) |\
		(PULL_WAKEUP_SRC_PA4<<8) | (PULL_WAKEUP_SRC_PA5<<10) | (PULL_WAKEUP_SRC_PA6<<12) | (PULL_WAKEUP_SRC_PA7<<14);\
		/*******************PB********************/\
		s.PB.setting1 = \
		(PB0_INPUT_ENABLE<<8) 	| (PB1_INPUT_ENABLE<<9)	| (PB2_INPUT_ENABLE<<10)	| (PB3_INPUT_ENABLE<<11) |\
		(PB4_INPUT_ENABLE<<12)	| (PB5_INPUT_ENABLE<<13)	| (PB6_INPUT_ENABLE<<14)	| (PB7_INPUT_ENABLE<<15) |\
		((PB0_OUTPUT_ENABLE?0:1)<<16)	| ((PB1_OUTPUT_ENABLE?0:1)<<17) | ((PB2_OUTPUT_ENABLE?0:1)<<18)	| ((PB3_OUTPUT_ENABLE?0:1)<<19) |\
		((PB4_OUTPUT_ENABLE?0:1)<<20)	| ((PB5_OUTPUT_ENABLE?0:1)<<21) | ((PB6_OUTPUT_ENABLE?0:1)<<22)	| ((PB7_OUTPUT_ENABLE?0:1)<<23) |\
		(PB0_DATA_OUT<<24)	| (PB1_DATA_OUT<<25)	| (PB2_DATA_OUT<<26)	| (PB3_DATA_OUT<<27) |\
		(PB4_DATA_OUT<<28)	| (PB5_DATA_OUT<<29)	| (PB6_DATA_OUT<<30)	| (PB7_DATA_OUT<<31) ;\
		s.PB.setting2 = \
		(PB0_DATA_STRENGTH<<8)		| (PB1_DATA_STRENGTH<<9)| (PB2_DATA_STRENGTH<<10)	| (PB3_DATA_STRENGTH<<11) |\
		(PB4_DATA_STRENGTH<<12)	| (PB5_DATA_STRENGTH<<13)	| (PB6_DATA_STRENGTH<<14)	| (PB7_DATA_STRENGTH<<15) |\
		(PB0_FUNC==AS_GPIO ? BIT(16):0)	| (PB1_FUNC==AS_GPIO ? BIT(17):0)| (PB2_FUNC==AS_GPIO ? BIT(18):0)| (PB3_FUNC==AS_GPIO ? BIT(19):0) |\
		(PB4_FUNC==AS_GPIO ? BIT(20):0)	| (PB5_FUNC==AS_GPIO ? BIT(21):0)| (PB6_FUNC==AS_GPIO ? BIT(22):0)| (PB7_FUNC==AS_GPIO ? BIT(23):0);\
		s.PB.pull.port = \
		PULL_WAKEUP_SRC_PB0 | (PULL_WAKEUP_SRC_PB1<<2) | (PULL_WAKEUP_SRC_PB2<<4) | (PULL_WAKEUP_SRC_PB3<<6)  |\
		(PULL_WAKEUP_SRC_PB4<<8) | (PULL_WAKEUP_SRC_PB5<<10) | (PULL_WAKEUP_SRC_PB6<<12) | (PULL_WAKEUP_SRC_PB7<<14);\
		/*******************PC********************/\
		s.PC.setting1 = reg_gpio_pc_setting1;\
		s.PC.setting2 = reg_gpio_pc_setting2;\
		s.PC.ie.port =\
		(PC0_INPUT_ENABLE<<0) 	| (PC1_INPUT_ENABLE<<1)	| (PC2_INPUT_ENABLE<<2)	| (PC3_INPUT_ENABLE<<3) |\
		(PC4_INPUT_ENABLE<<4)	| (PC5_INPUT_ENABLE<<5) | (PC6_INPUT_ENABLE<<6)	| (PC7_INPUT_ENABLE<<7);\
		s.PC.oen.port =\
		((PC0_OUTPUT_ENABLE?0:1)<<0)	| ((PC1_OUTPUT_ENABLE?0:1)<<1) | ((PC2_OUTPUT_ENABLE?0:1)<<2)	| ((PC3_OUTPUT_ENABLE?0:1)<<3) |\
		((PC4_OUTPUT_ENABLE?0:1)<<4)	| ((PC5_OUTPUT_ENABLE?0:1)<<5) | ((PC6_OUTPUT_ENABLE?0:1)<<6)	| ((PC7_OUTPUT_ENABLE?0:1)<<7);\
		s.PC.output.port =\
		(PC0_DATA_OUT<<0)	| (PC1_DATA_OUT<<1)	| (PC2_DATA_OUT<<2)	| (PC3_DATA_OUT<<3) |\
		(PC4_DATA_OUT<<4)	| (PC5_DATA_OUT<<5)	| (PC6_DATA_OUT<<6)	| (PC7_DATA_OUT<<7) ;\
		s.PC.ds.port = \
		(PC0_DATA_STRENGTH<<0) 	| (PC1_DATA_STRENGTH<<1)  | (PC2_DATA_STRENGTH<<2)	| (PC3_DATA_STRENGTH<<3) |\
		(PC4_DATA_STRENGTH<<4)	| (PC5_DATA_STRENGTH<<5)  | (PC6_DATA_STRENGTH<<6)	| (PC7_DATA_STRENGTH<<7);\
		s.PC.io.port = \
		(PC0_FUNC==AS_GPIO ? BIT(0):0)	| (PC1_FUNC==AS_GPIO ? BIT(1):0)| (PC2_FUNC==AS_GPIO ? BIT(2):0)| (PC3_FUNC==AS_GPIO ? BIT(3):0) |\
		(PC4_FUNC==AS_GPIO ? BIT(4):0)	| (PC5_FUNC==AS_GPIO ? BIT(5):0)| (PC6_FUNC==AS_GPIO ? BIT(6):0)| (PC7_FUNC==AS_GPIO ? BIT(7):0);\
		s.PC.pull.port = \
		PULL_WAKEUP_SRC_PC0 | (PULL_WAKEUP_SRC_PC1<<2) | (PULL_WAKEUP_SRC_PC2<<4) | (PULL_WAKEUP_SRC_PC3<<6) |\
		(PULL_WAKEUP_SRC_PC4<<8) | (PULL_WAKEUP_SRC_PC5<<10) | (PULL_WAKEUP_SRC_PC6<<12) | (PULL_WAKEUP_SRC_PC7<<14);\
		/*******************PD********************/\
		s.PD.setting1 = reg_gpio_pd_setting1;\
		s.PD.setting2 = reg_gpio_pd_setting2;\
		s.PD.ie.port=\
		(PD0_INPUT_ENABLE<<0) 	| (PD1_INPUT_ENABLE<<1)	| (PD2_INPUT_ENABLE<<2)	| (PD3_INPUT_ENABLE<<3) |\
		(PD4_INPUT_ENABLE<<4)	| (PD5_INPUT_ENABLE<<5) | (PD6_INPUT_ENABLE<<6)	| (PD7_INPUT_ENABLE<<7);\
		s.PD.oen.port = \
		((PD0_OUTPUT_ENABLE?0:1)<<0)	| ((PD1_OUTPUT_ENABLE?0:1)<<1) | ((PD2_OUTPUT_ENABLE?0:1)<<2)	| ((PD3_OUTPUT_ENABLE?0:1)<<3) |\
		((PD4_OUTPUT_ENABLE?0:1)<<4)	| ((PD5_OUTPUT_ENABLE?0:1)<<5) | ((PD6_OUTPUT_ENABLE?0:1)<<6)	| ((PD7_OUTPUT_ENABLE?0:1)<<7);\
		s.PD.output.port = \
		(PD0_DATA_OUT<<0)	| (PD1_DATA_OUT<<1)	| (PD2_DATA_OUT<<2)	| (PD3_DATA_OUT<<3) |\
		(PD4_DATA_OUT<<4)	| (PD5_DATA_OUT<<5)	| (PD6_DATA_OUT<<6)	| (PD7_DATA_OUT<<7) ;\
		s.PD.ds.port = \
		(PD0_DATA_STRENGTH<<0) 	| (PD1_DATA_STRENGTH<<1)  | (PD2_DATA_STRENGTH<<2)	| (PD3_DATA_STRENGTH<<3) |\
		(PD4_DATA_STRENGTH<<4)	| (PD5_DATA_STRENGTH<<5)  | (PD6_DATA_STRENGTH<<6)	| (PD7_DATA_STRENGTH<<7);\
		s.PD.io.port = \
		(PD0_FUNC==AS_GPIO ? BIT(0):0)	| (PD1_FUNC==AS_GPIO ? BIT(1):0)| (PD2_FUNC==AS_GPIO ? BIT(2):0)| (PD3_FUNC==AS_GPIO ? BIT(3):0) |\
		(PD4_FUNC==AS_GPIO ? BIT(4):0)	| (PD5_FUNC==AS_GPIO ? BIT(5):0)| (PD6_FUNC==AS_GPIO ? BIT(6):0)| (PD7_FUNC==AS_GPIO ? BIT(7):0);\
		s.PD.pull.port = \
		PULL_WAKEUP_SRC_PD0 | (PULL_WAKEUP_SRC_PD1<<2) | (PULL_WAKEUP_SRC_PD2<<4) | (PULL_WAKEUP_SRC_PD3<<6) |\
		(PULL_WAKEUP_SRC_PD4<<8) | (PULL_WAKEUP_SRC_PD5<<10) | (PULL_WAKEUP_SRC_PD6<<12) | (PULL_WAKEUP_SRC_PD7<<14);\
		/*******************PE********************/\
		s.PE.setting1 = \
		(PE0_INPUT_ENABLE<<8) 	| (PE1_INPUT_ENABLE<<9)	| (PE2_INPUT_ENABLE<<10)	| (PE3_INPUT_ENABLE<<11) |\
		(PE4_INPUT_ENABLE<<12)	| (PE5_INPUT_ENABLE<<13)	| (PE6_INPUT_ENABLE<<14)	| (PE7_INPUT_ENABLE<<15) |\
		((PE0_OUTPUT_ENABLE?0:1)<<16)	| ((PE1_OUTPUT_ENABLE?0:1)<<17) | ((PE2_OUTPUT_ENABLE?0:1)<<18)	| ((PE3_OUTPUT_ENABLE?0:1)<<19) |\
		((PE4_OUTPUT_ENABLE?0:1)<<20)	| ((PE5_OUTPUT_ENABLE?0:1)<<21) | ((PE6_OUTPUT_ENABLE?0:1)<<22)	| ((PE7_OUTPUT_ENABLE?0:1)<<23) |\
		(PE0_DATA_OUT<<24)	| (PE1_DATA_OUT<<25)	| (PE2_DATA_OUT<<26)	| (PE3_DATA_OUT<<27) |\
		(PE4_DATA_OUT<<28)	| (PE5_DATA_OUT<<29)	| (PE6_DATA_OUT<<30)	| (PE7_DATA_OUT<<31) ;\
		s.PE.setting2 = \
		(PE0_DATA_STRENGTH<<8)		| (PE1_DATA_STRENGTH<<9)| (PE2_DATA_STRENGTH<<10)	| (PE3_DATA_STRENGTH<<11) |\
		(PE4_DATA_STRENGTH<<12)	| (PE5_DATA_STRENGTH<<13)	| (PE6_DATA_STRENGTH<<14)	| (PE7_DATA_STRENGTH<<15) |\
		(PE0_FUNC==AS_GPIO ? BIT(16):0)	| (PE1_FUNC==AS_GPIO ? BIT(17):0)| (PE2_FUNC==AS_GPIO ? BIT(18):0)| (PE3_FUNC==AS_GPIO ? BIT(19):0) |\
		(PE4_FUNC==AS_GPIO ? BIT(20):0)	| (PE5_FUNC==AS_GPIO ? BIT(21):0)| (PE6_FUNC==AS_GPIO ? BIT(22):0)| (PE7_FUNC==AS_GPIO ? BIT(23):0);\
		s.PE.pull.port = \
		PULL_WAKEUP_SRC_PE0 | (PULL_WAKEUP_SRC_PE1<<2) | (PULL_WAKEUP_SRC_PE2<<4) | (PULL_WAKEUP_SRC_PE3<<6) |\
		(PULL_WAKEUP_SRC_PE4<<8) | (PULL_WAKEUP_SRC_PE5<<10) | (PULL_WAKEUP_SRC_PE6<<12) | (PULL_WAKEUP_SRC_PE7<<14);\
		/*******************PF********************/\
		s.PF.setting1 = \
		(PF0_INPUT_ENABLE<<8) 	| (PF1_INPUT_ENABLE<<9)	| (PF2_INPUT_ENABLE<<10)	| (PF3_INPUT_ENABLE<<11) |\
		(PF4_INPUT_ENABLE<<12)	| (PF5_INPUT_ENABLE<<13)	| \
		((PF0_OUTPUT_ENABLE?0:1)<<16)	| ((PF1_OUTPUT_ENABLE?0:1)<<17) | ((PF2_OUTPUT_ENABLE?0:1)<<18)	| ((PF3_OUTPUT_ENABLE?0:1)<<19) |\
		((PF4_OUTPUT_ENABLE?0:1)<<20)	| ((PF5_OUTPUT_ENABLE?0:1)<<21) | \
		(PF0_DATA_OUT<<24)	| (PF1_DATA_OUT<<25)	| (PF2_DATA_OUT<<26)	| (PF3_DATA_OUT<<27) |\
		(PF4_DATA_OUT<<28)	| (PF5_DATA_OUT<<29);\
		s.PF.setting2 = reg_gpio_pf_setting2;\
		s.PF.ds.port = \
		(PF0_DATA_STRENGTH<<0) 	| (PF1_DATA_STRENGTH<<1)  | (PF2_DATA_STRENGTH<<2)	| (PF3_DATA_STRENGTH<<3) |\
		(PF4_DATA_STRENGTH<<4)	| (PF5_DATA_STRENGTH<<5) ;\
		s.PF.io.port = \
		(PF0_FUNC==AS_GPIO ? BIT(0):0)	| (PF1_FUNC==AS_GPIO ? BIT(1):0)| (PF2_FUNC==AS_GPIO ? BIT(2):0)| (PF3_FUNC==AS_GPIO ? BIT(3):0) |\
		(PF4_FUNC==AS_GPIO ? BIT(4):0)	| (PF5_FUNC==AS_GPIO ? BIT(5):0);\
	}while(0)

/**
 * @brief      This function servers to initialization all gpio.
 * @param[in]  anaRes_init_en  -  if mcu wake up from deep retention mode, determine whether it is NOT necessary to reset analog register
 *                    			  1: set analog register
 *                    			  0: not set analog register
 */
#define gpio_init(anaRes_init_en)	\
	do{\
		gpio_init_t st;\
		set_gpio_struct_from_file(st);\
		gpio_usr_init(anaRes_init_en, &st);\
	}while(0)

#endif /* DRIVERS_GPIO_SETTING_H_ */

