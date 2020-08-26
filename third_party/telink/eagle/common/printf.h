
/********************************************************************************************************
 * @file     printf.h
 *
 * @brief    This is the header file for TLSR8258
 *
 * @author	 public@telink-semi.com;
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

#pragma once
#include "../drivers/gpio.h"
//#define DEBUG_MODE	1

/**
 * @brief      This function serves to printf string.
 * @param[in]  *format  -  format string need to print
 * @param[in]  ...   	-  variable number of data
 * @return     none.
 */

int Tl_printf(const char *format, ...);

#define PRINT_BAUD_RATE 115200     // 1M baud rate,should Not bigger than 1Mb/s
#define DEBUG_INFO_TX_PIN GPIO_PB0 //
#define TX_PIN_GPIO_EN() gpio_set_gpio_en(DEBUG_INFO_TX_PIN);
#define TX_PIN_PULLUP_1M() gpio_set_up_down_res(DEBUG_INFO_TX_PIN, PM_PIN_PULLUP_1M);
#define TX_PIN_OUTPUT_EN() gpio_set_output_en(DEBUG_INFO_TX_PIN)
#define TX_PIN_OUTPUT_REG (0x140303 + ((DEBUG_INFO_TX_PIN >> 8) << 3))

#ifndef BIT_INTERVAL
#define BIT_INTERVAL (16 * 1000 * 1000 / PRINT_BAUD_RATE)
#endif
