/********************************************************************************************************
 * @file	gpio.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H / X.P.C
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
/**	@page GPIO
 *
 *	Introduction
 *	===============
 *	B91 contain two six group gpio(A~F), total 44 gpio pin.
 *
 *	API Reference
 *	===============
 *	Header File: gpio.h
 */
#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_
#include "../common/bit.h"
#include "../common/types.h"
#include "analog.h"
#include "plic.h"
#include "reg_include/gpio_reg.h"
/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**
 *  @brief  Define GPIO types
 */
typedef enum{
		GPIO_GROUPA    = 0x000,
		GPIO_GROUPB    = 0x100,
		GPIO_GROUPC    = 0x200,
		GPIO_GROUPD    = 0x300,
		GPIO_GROUPE    = 0x400,
		GPIO_GROUPF    = 0x500,
		GPIO_ALL       = 0x600,
	    GPIO_PA0 = GPIO_GROUPA | BIT(0),
		GPIO_PA1 = GPIO_GROUPA | BIT(1),
		GPIO_PA2 = GPIO_GROUPA | BIT(2),
		GPIO_PA3 = GPIO_GROUPA | BIT(3),
		GPIO_PA4 = GPIO_GROUPA | BIT(4),
		GPIO_PA5 = GPIO_GROUPA | BIT(5),GPIO_DM=GPIO_PA5,
		GPIO_PA6 = GPIO_GROUPA | BIT(6),GPIO_DP=GPIO_PA6,
		GPIO_PA7 = GPIO_GROUPA | BIT(7),GPIO_SWS=GPIO_PA7,
		GPIOA_ALL = GPIO_GROUPA | 0x00ff,

		GPIO_PB0 = GPIO_GROUPB | BIT(0),
		GPIO_PB1 = GPIO_GROUPB | BIT(1),
		GPIO_PB2 = GPIO_GROUPB | BIT(2),
		GPIO_PB3 = GPIO_GROUPB | BIT(3),
		GPIO_PB4 = GPIO_GROUPB | BIT(4),
		GPIO_PB5 = GPIO_GROUPB | BIT(5),
		GPIO_PB6 = GPIO_GROUPB | BIT(6),
		GPIO_PB7 = GPIO_GROUPB | BIT(7),

		GPIO_PC0 = GPIO_GROUPC | BIT(0),
		GPIO_PC1 = GPIO_GROUPC | BIT(1),
		GPIO_PC2 = GPIO_GROUPC | BIT(2),
		GPIO_PC3 = GPIO_GROUPC | BIT(3),
		GPIO_PC4 = GPIO_GROUPC | BIT(4),
		GPIO_PC5 = GPIO_GROUPC | BIT(5),
		GPIO_PC6 = GPIO_GROUPC | BIT(6),
		GPIO_PC7 = GPIO_GROUPC | BIT(7),
		GPIOC_ALL = GPIO_GROUPC | 0x00ff,

		GPIO_PD0 = GPIO_GROUPD | BIT(0),
		GPIO_PD1 = GPIO_GROUPD | BIT(1),
		GPIO_PD2 = GPIO_GROUPD | BIT(2),
		GPIO_PD3 = GPIO_GROUPD | BIT(3),
		GPIO_PD4 = GPIO_GROUPD | BIT(4),
		GPIO_PD5 = GPIO_GROUPD | BIT(5),
		GPIO_PD6 = GPIO_GROUPD | BIT(6),
		GPIO_PD7 = GPIO_GROUPD | BIT(7),

		GPIO_PE0 = GPIO_GROUPE | BIT(0),
		GPIO_PE1 = GPIO_GROUPE | BIT(1),
		GPIO_PE2 = GPIO_GROUPE | BIT(2),
		GPIO_PE3 = GPIO_GROUPE | BIT(3),
		GPIO_PE4 = GPIO_GROUPE | BIT(4),
		GPIO_PE5 = GPIO_GROUPE | BIT(5),
		GPIO_PE6 = GPIO_GROUPE | BIT(6),
		GPIO_PE7 = GPIO_GROUPE | BIT(7),
		GPIOE_ALL = GPIO_GROUPE | 0x00ff,

		GPIO_PF0 = GPIO_GROUPF | BIT(0),
		GPIO_PF1 = GPIO_GROUPF | BIT(1),
		GPIO_PF2 = GPIO_GROUPF | BIT(2),
		GPIO_PF3 = GPIO_GROUPF | BIT(3),

}gpio_pin_e;

/**
 *  @brief  Define GPIO mux func
 */
typedef enum{
	    AS_GPIO,
		AS_MSPI,

		AS_SWS,
		AS_SWM,

		AS_USB_DP,
		AS_USB_DM,

		AS_TDI,
		AS_TDO,
		AS_TMS,
		AS_TCK,



}gpio_fuc_e;



/**
 *  @brief  Define rising/falling types
 */
typedef enum{
	POL_RISING   = 0,
	POL_FALLING  = 1,
}gpio_pol_e;


/**
 *  @brief  Define interrupt types
 */
typedef enum{
	 INTR_RISING_EDGE=0,
	 INTR_FALLING_EDGE ,
	 INTR_HIGH_LEVEL,
	 INTR_LOW_LEVEL,
} gpio_irq_trigger_type_e;

/**
 *  @brief  Define pull up or down types
 */
typedef enum {
	GPIO_PIN_UP_DOWN_FLOAT    = 0,
	GPIO_PIN_PULLUP_1M     	= 1,
	GPIO_PIN_PULLDOWN_100K  	= 2,
	GPIO_PIN_PULLUP_10K 		= 3,
}gpio_pull_type_e;
#if 0
/**
 *  @brief  Define port init by bit types
 */
typedef union{
	unsigned char port;
	struct{
		unsigned char p0 : 1;
		unsigned char p1 : 1;
		unsigned char p2 : 1;
		unsigned char p3 : 1;
		unsigned char p4 : 1;
		unsigned char p5 : 1;
		unsigned char p6 : 1;
		unsigned char p7 : 1;
	};
}port_bit_init_u;

/**
 *  @brief  Define port init by double bits types
 */
typedef union{
	unsigned short port;
	struct{
		unsigned char p0 : 2;
		unsigned char p1 : 2;
		unsigned char p2 : 2;
		unsigned char p3 : 2;
		unsigned char p4 : 2;
		unsigned char p5 : 2;
		unsigned char p6 : 2;
		unsigned char p7 : 2;
	};
}port_2bits_init_u;

/**
 *  @brief  Define port init by byte types
 */
typedef struct{
	union{
		unsigned int setting1;
		struct{
			port_bit_init_u input;
			port_bit_init_u ie;
			port_bit_init_u oen;
			port_bit_init_u output;
		};
	};
	union{
		unsigned int setting2;
		struct{
			port_bit_init_u polarity;
			port_bit_init_u ds;
			port_bit_init_u io;
			port_bit_init_u irq;
		};
	};
	port_2bits_init_u fs;
	port_2bits_init_u pull;
}port_init_t;

/**
 *  @brief  Define gpio init types
 */
typedef struct{
	port_init_t PA;
	port_init_t PB;
	port_init_t PC;
	port_init_t PD;
	port_init_t PE;
	port_init_t PF;
}gpio_init_t;
#endif
/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/


/**
 * @brief      This function servers to enable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
static inline void gpio_function_en(gpio_pin_e pin)
{
	u8	bit = pin & 0xff;
	BM_SET(reg_gpio_func(pin), bit);
}


/**
 * @brief      This function servers to disable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
static inline void gpio_function_dis(gpio_pin_e pin)
{
	u8	bit = pin & 0xff;
	BM_CLR(reg_gpio_func(pin), bit);
}



/**
 * @brief     This function set the pin's output high level.
 * @param[in] pin - the pin needs to set its output level.
 * @return    none.
 */
static inline void gpio_set_high_level(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_out(pin), bit);

}


/**
 * @brief     This function set the pin's output low level.
 * @param[in] pin - the pin needs to set its output level.
 * @return    none.
 */
static inline void gpio_set_low_level(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_CLR(reg_gpio_out(pin), bit);

}


/**
 * @brief     This function read the pin's input/output level.
 * @param[in] pin - the pin needs to read its level.
 * @return    1: the pin's level is high.
 * 			  0: the pin's level is low.
 */
static inline _Bool gpio_get_level(gpio_pin_e pin)
{
	return BM_IS_SET(reg_gpio_in(pin), pin & 0xff);
}


/**
 * @brief      This function read all the pins' input level.
 * @param[out] p - the buffer used to store all the pins' input level
 * @return     none
 */
static inline void gpio_get_level_all(unsigned char *p)
{
	p[0] = reg_gpio_pa_in;
	p[1] = reg_gpio_pb_in;
	p[2] = reg_gpio_pc_in;
	p[3] = reg_gpio_pd_in;
	p[4] = reg_gpio_pe_in;
}



/**
 * @brief     This function set the pin toggle.
 * @param[in] pin - the pin needs to toggle.
 * @return    none.
 */
static inline void gpio_toggle(gpio_pin_e pin)
{
	reg_gpio_out(pin) ^= (pin & 0xFF);
}



/**
 * @brief      This function enable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function.
 * @return     none.
 */
static inline void gpio_output_en(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_CLR(reg_gpio_oen(pin), bit);
}

/**
 * @brief      This function disable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function.
 * @return     none.
 */
static inline void gpio_output_dis(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_oen(pin), bit);
}

/**
 * @brief      This function determines whether the output function of a pin is enabled.
 * @param[in]  pin - the pin needs to determine whether its output function is enabled.
 * @return     1: the pin's output function is enabled.
 *             0: the pin's output function is disabled.
 */
static inline _Bool  gpio_is_output_en(gpio_pin_e pin)
{
	return !BM_IS_SET(reg_gpio_oen(pin), pin & 0xff);
}


/**
 * @brief     This function determines whether the input function of a pin is enabled.
 * @param[in] pin - the pin needs to determine whether its input function is enabled(not include group_pc).
 * @return    1: the pin's input function is enabled.
 *            0: the pin's input function is disabled.
 */
static inline _Bool gpio_is_input_en(gpio_pin_e pin)
{
	return BM_IS_SET(reg_gpio_ie(pin), pin & 0xff);
}

/**
 * @brief      This function serves to enable gpio irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_irq_en(gpio_pin_e pin)
{
	BM_SET(reg_gpio_irq_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to disable gpio irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_irq_dis(gpio_pin_e pin)
{
	BM_CLR(reg_gpio_irq_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to enable gpio risc0 irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc0_irq_en(gpio_pin_e pin)
{
	BM_SET(reg_gpio_irq_risc0_en(pin), pin & 0xff);
}
/**
 * @brief      This function serves to disable gpio risc0 irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc0_irq_dis(gpio_pin_e pin)
{
	BM_CLR(reg_gpio_irq_risc0_en(pin), pin & 0xff);
}
/**
 * @brief      This function serves to enable gpio risc1 irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc1_irq_en(gpio_pin_e pin)
{
	BM_SET(reg_gpio_irq_risc1_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to disable gpio risc1 irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc1_irq_dis(gpio_pin_e pin)
{
	BM_CLR(reg_gpio_irq_risc1_en(pin), pin & 0xff);
}
/**
 * @brief      This function serves to clr gpio irq status.
 * @param[in]  status  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_clr_irq_status(gpio_irq_status_e status)
{
	reg_gpio_irq_clr=status;
}

/**
 * @brief      This function set the pin's driving strength at strong.
 * @param[in]  pin - the pin needs to set the driving strength.
 * @return     none.
 */
void gpio_ds_en(gpio_pin_e pin);


/**
 * @brief      This function set the pin's driving strength.
 * @param[in]  pin - the pin needs to set the driving strength at poor.
 * @return     none.
 */
 void gpio_ds_dis(gpio_pin_e pin);




void gpio_set_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC0.
 * @param[in] pin 			- the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none.
 */
void gpio_set_gpio2risc0_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC1.
 * @param[in] pin 			- the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none.
 */
void gpio_set_gpio2risc1_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);


/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_en(gpio_pin_e pin);

/**
 * @brief      This function disable the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_dis(gpio_pin_e pin);

/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO.
 * @return     none.
 */
void gpio_shutdown(gpio_pin_e pin);

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] pin - the pin needs to set its pull-up/down resistor.
 * @param[in] up_down_res - the type of the pull-up/down resistor.
 * @return    none.
 */
void gpio_set_up_down_res(gpio_pin_e pin, gpio_pull_type_e up_down_res);
#if 0
/**
 * @brief      This function servers to initialization all gpio.
 * @param[in]  anaRes_init_en  -  if mcu wake up from deep retention mode, determine whether it is NOT necessary to reset analog register.
 *                                1: set analog register.
 *                                0: not set analog register.
 * @param[in]  st  			   -  structure of gpio settings.
 * @return     none.
 * @attention  Processing methods of unused GPIO
 * 			   Set it to high resistance state and set it to open pull-up or pull-down resistance to
 *             let it be in the determined state.When GPIO uses internal pull-up or pull-down resistance,
 *             do not use pull-up or pull-down resistance on the board in the process of practical
 *             application because it may have the risk of electric leakage .
 */
void gpio_usr_init(int anaRes_init_en, gpio_init_t* st);
#endif
#endif




