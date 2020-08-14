/********************************************************************************************************
 * @file     gpio_8258.h 
 *
 * @brief    This is the header file for TLSR8258
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
/**	@page GPIO
 *
 *	Introduction
 *	===============
 *	TLSR9518 contain two six group gpio(A~F), total 44 gpio pin.
 *
 *	API Reference
 *	===============
 *	Header File: gpio.h
 */
#ifndef GPIO_H_
#define GPIO_H_

#include "../common/bit.h"
#include "../common/types.h"
#include "analog.h"
#include "plic.h"
#include "reg_include/register_9518.h"
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
		AS_NGPIO,
		AS_MSPI,
		AS_SWS,
		AS_SWM,

		AS_UART0_TX,
		AS_UART0_RX,
		AS_UART0_RTS,
		AS_UART0_CTS,

		AS_UART1_TX,
		AS_UART1_RX,
		AS_UART1_RTS,
		AS_UART1_CTS,

		AS_I2C_SCK,
		AS_I2C_SDA,

		AS_SSPI_CN,
		AS_SSPI_CK,
		AS_SSPI_DO,
		AS_SSPI_DI,

		AS_HSPI_HD,
		AS_HSPI_WP,
		AS_HSPI_CN,
		AS_HSPI_CK,
		AS_HSPI_DO,
		AS_HSPI_DI,

		AS_LSPI_CN,
		AS_LSPI_CK,
		AS_LSPI_DO,
		AS_LSPI_DI,

		AS_I2S_SCK,
		AS_I2S_IO,
		AS_BCK_IO,

		AS_AMIC,
		AS_DMIC_SCK,
		AS_DMIC_DI,
		AS_SDM,

		AS_USB_DP,
		AS_USB_DM,

		AS_ADC_LR_IO,
		AS_ADC_DAT,

		AS_DAC_LR_IO,
		AS_DAC_DAT,
		AS_CMP,
		AS_ATS,

		AS_PWM0,
		AS_PWM1,
		AS_PWM2,
		AS_PWM3,
		AS_PWM4,
		AS_PWM5,
		AS_PWM0_N,
		AS_PWM1_N,
		AS_PWM2_N,
		AS_PWM3_N,
		AS_PWM4_N,
		AS_PWM5_N,

		AS_7816_CLK,
		AS_32K_CLK,
		AS_ATSEL0,
		AS_ATSEL1,
		AS_ATSEL2,

		AS_TX_CYC2PA,
		AS_RX_CYC2LNA,

		AS_BT_INBAND,

		AS_TDI,
		AS_TDO,
		AS_TMS,
		AS_TCK,

		AS_ADC_Q_DATX,
		AS_DAC_Q_DATX,

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
	PM_PIN_UP_DOWN_FLOAT    = 0,
	PM_PIN_PULLUP_1M     	= 1,
	PM_PIN_PULLDOWN_100K  	= 2,
	PM_PIN_PULLUP_10K 		= 3,
}gpio_pull_type_e;

/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/
/**
 * @brief     This function set the pin's output high level.
 * @param[in] pin - the pin needs to set its output level
 * @return    none
 */
static inline void gpio_set_high_level(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_out(pin), bit);

}


/**
 * @brief     This function set the pin's output low level.
 * @param[in] pin - the pin needs to set its output level
 * @return    none
 */
static inline void gpio_set_low_level(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_CLR(reg_gpio_out(pin), bit);

}

/**
 * @brief     This function set the pin's output level.
 * @param[in] pin - the pin needs to set its output level
 * @param[in] value - value of the output level(1: high 0: low)
 * @return    none
 */
static inline void gpio_write(gpio_pin_e pin, unsigned int value)
{
	unsigned char	bit = pin & 0xff;
	if(value){
		BM_SET(reg_gpio_out(pin), bit);
	}else{
		BM_CLR(reg_gpio_out(pin), bit);
	}
}

/**
 * @brief     This function read the pin's input/output level.
 * @param[in] pin - the pin needs to read its level
 * @return    the pin's level(1: high 0: low)
 */
static inline _Bool gpio_get_level(gpio_pin_e pin)
{
	return BM_IS_SET(reg_gpio_in(pin), pin & 0xff);
}

/**
 * @brief     This function read the pin's input/output level.
 * @param[in] pin - the pin needs to read its level
 * @return    the pin's level(1: high 0: low)
 */
static inline unsigned int gpio_read(gpio_pin_e pin)
{
	return gpio_get_level(pin);
}

/**
 * @brief     This function set the pin toggle.
 * @param[in] pin - the pin needs to toggle
 * @return    none
 */
static inline void gpio_toggle(gpio_pin_e pin)
{
	reg_gpio_out(pin) ^= (pin & 0xFF);
}



/**
 * @brief      This function enable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function
 * @return     none
 */
static inline void gpio_set_output_en(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
		BM_CLR(reg_gpio_oen(pin), bit);
}

/**
 * @brief      This function disable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function
 * @return     none
 */
static inline void gpio_set_output_dis(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
		BM_SET(reg_gpio_oen(pin), bit);
}

/**
 * @brief      This function determines whether the output function of a pin is enabled.
 * @param[in]  pin - the pin needs to determine whether its output function is enabled.
 * @return     1: the pin's output function is enabled ;
 *             0: the pin's output function is disabled
 */
static inline _Bool  gpio_is_output_en(gpio_pin_e pin)
{
	return !BM_IS_SET(reg_gpio_oen(pin), pin & 0xff);
}


/**
 * @brief     This function determines whether the input function of a pin is enabled.
 * @param[in] pin - the pin needs to determine whether its input function is enabled(not include group_pc).
 * @return    1: the pin's input function is enabled ;
 *            0: the pin's input function is disabled
 */
static inline _Bool gpio_is_input_en(gpio_pin_e pin)
{
	return BM_IS_SET(reg_gpio_ie(pin), pin & 0xff);
}
/**
 * @brief      This function set the pin's driving strength at strong.
 * @param[in]  pin - the pin needs to set the driving strength
 * @return     none
 */
static inline void gpio_set_ds_en(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	unsigned short group = pin & 0xf00;
	if(group == GPIO_GROUPC)
	{analog_write_reg8(areg_gpio_pc_ds, analog_read_reg8(areg_gpio_pc_ds)|bit);}
	else
	{BM_SET(reg_gpio_ds(pin), bit);}
}

/**
 * @brief      This function set the pin's driving strength.
 * @param[in]  pin - the pin needs to set the driving strength at poor.
 * @return     none
 */
static inline void gpio_set_ds_dis(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	unsigned short group = pin & 0xf00;
	if(group == GPIO_GROUPC)
	{analog_write_reg8(areg_gpio_pc_ds, analog_read_reg8(areg_gpio_pc_ds)&(~bit));}
	else
	{BM_CLR(reg_gpio_ds(pin), bit);}
}


/**
 * @brief     This function enables a pin's IRQ function.
 * @param[in] pin - the pin needs to enables its IRQ function.
 * @return    none
 */
static inline  void gpio_set_intr_en(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_en(pin), bit);
}

/**
 * @brief     This function enables a pin's IRQ function.
 * @param[in] pin - the pin needs to enables its IRQ function.
 * @return    none
 */
static inline  void gpio_set_intr_dis(gpio_pin_e pin)
{
	unsigned char	bit = pin & 0xff;
	BM_CLR(reg_gpio_irq_en(pin), bit);
}


/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin - the pin needs to enable its IRQ
 * @param[in] falling - value of the edge polarity(1: falling edge 0: rising edge)
 * @return    none
 */
static inline void gpio_set_interrupt(gpio_pin_e pin, gpio_pol_e falling)
{
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_en(pin), bit);
	if(falling){
		BM_SET(reg_gpio_pol(pin), bit);
	}else{
		BM_CLR(reg_gpio_pol(pin), bit);
	}
	reg_gpio_irq_ctrl |= FLD_GPIO_CORE_INTERRUPT_EN;
	reg_gpio_irq_risc_mask|=FLD_GPIO_IRQ_MASK_GPIO;

}
/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */

void gpio_setup_up_down_resistor(gpio_pin_e gpio, gpio_pull_type_e up_down);
/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin 			- the pin needs to enable its IRQ
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none
 */
void gpio_set_gpio_irq_trigger_type(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC0.
 * @param[in] pin 			- the pin needs to enable its IRQ
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none
 */
void gpio_set_gpio2risc0_irq_trigger_type(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC1.
 * @param[in] pin 			- the pin needs to enable its IRQ
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none
 */
void gpio_set_gpio2risc1_irq_trigger_type(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief      This function serves to enable gpio irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_irq_en(gpio_pin_e pin);

/**
 * @brief      This function serves to disable gpio irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_irq_dis(gpio_pin_e pin);

/**
 * @brief      This function serves to enable gpio risc0 irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_gpio2risc0_irq_en(gpio_pin_e pin);

/**
 * @brief      This function serves to disable gpio risc0 irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_gpio2risc0_irq_dis(gpio_pin_e pin);

/**
 * @brief      This function serves to enable gpio risc1 irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_gpio2risc1_irq_en(gpio_pin_e pin);

/**
 * @brief      This function serves to disable gpio risc1 irq function.
 * @param[in]  none.
 * @return     none.
 */
void gpio_gpio2risc1_irq_dis(gpio_pin_e pin);

/**
 * @brief      This function servers to enable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
void gpio_set_gpio_en(gpio_pin_e pin);

/**
 * @brief      This function servers to disable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
void gpio_set_gpio_dis(gpio_pin_e pin);

/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function
 * @param[in]  value - enable or disable the pin's input function(0: disable, 1: enable)
 * @return     none
 */
void gpio_set_input_en(gpio_pin_e pin);

/**
 * @brief      This function disable the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none
 */
void gpio_set_input_dis(gpio_pin_e pin);

/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO
 * @return     none.
 */
void gpio_shutdown(gpio_pin_e pin);

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */
void gpio_set_up_down_res(gpio_pin_e pin, gpio_pull_type_e up_down_res);

#endif
