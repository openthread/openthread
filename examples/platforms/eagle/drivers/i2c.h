/********************************************************************************************************
 * @file     i2c.h
 *
 * @brief    This is the header file for TLSR8258
 *
 * @author	 Driver Group
 * @date     May 8, 2018
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
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/

#ifndef I2C_H
#define I2C_H



#include "gpio.h"
#include "../common/bit.h"
#include "analog.h"
#include "reg_include/register_9518.h"
#include "dma.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
/**
 * The default is 4, we recommend setting it to 1 or 4.
 */
#define     SLAVE_RX_IRQ_TRIG_LEVEL     4





/**
 *  @brief  select pin as SDA and SCL of i2c
 */
typedef enum{
	I2C_GPIO_SDA_B3		= GPIO_PB3,
	I2C_GPIO_SDA_C2		= GPIO_PC2,
}i2c_sda_pin_e;


typedef enum{
	I2C_GPIO_SCL_B2		= GPIO_PB2,
	I2C_GPIO_SCL_C1		= GPIO_PC1,
}i2c_scl_pin_e;


/**
 * @brief      The function of this API is to determine whether the bus is busy.
 * @param[in]  none.
 * @return     1:Indicates that the bus is busy. 0:Indicates that the bus is free
 */
static inline BOOL i2c_master_busy(void)
{
    return reg_i2c_mst & FLD_I2C_MST_BUSY;
}

/**
 * @brief      The function of this API is to Get the number of bytes in tx_buffer.
 * @param[in]  none.
 * @return     The actual number of bytes in tx_buffer.
 */
static inline unsigned char i2c_get_tx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_TX_BUFCNT)>>4;
}


/**
 * @brief      The function of this API is to Get the number of bytes in rx_buffer.
 * @param[in]  none.
 * @return     The actual number of bytes in rx_buffer.
 */
static inline unsigned char i2c_get_rx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_RX_BUFCNT);
}


/**
 * @brief      The function of this API is to set the number of bytes to triggered the receive interrupt.
 *             Its default value is 4. We recommend setting it to 1 or 4.
 * @param[in]  cnt - the interrupt trigger level.
 * @return     none
 */
static inline void i2c_rx_irq_trig_cnt(unsigned char cnt)
{
	reg_i2c_trig &= (~FLD_I2C_RX_IRQ_TRIG_LEV);
	reg_i2c_trig |= cnt;
}

/**
 * @brief      This function selects a pin port for I2C interface.
 * @param[in]  sda_pin - the pin port selected as I2C sda pin port.
 * @param[in]  scl_pin - the pin port selected as I2C scl pin port.
 * @return     none
 */
void i2c_set_pin(i2c_sda_pin_e sda_pin,i2c_scl_pin_e scl_pin);


/**
 * @brief      This function serves to enable i2c master function.
 * @param[in]  none.
 * @return     none.
 */
void i2c_master_init(void);


/**
 * @brief      This function serves to enable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
 * @param[in]  mask - to select Interrupt type.
 * @return     none
 */
static inline void i2c_set_irq_mask(i2c_mask_irq_type_e mask)
{
	reg_i2c_sct0  |=  mask;
}


/**
 * @brief      This function serves to disable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
 * @param[in]  mask - to select Interrupt type.
 * @return     none
 */
static inline void i2c_clr_irq_mask(i2c_mask_irq_type_e mask)
{
	reg_i2c_sct0  |=  ~mask;
}


/**
 * @brief      This function serves to get i2c interrupt status.
 * @param[in]  irq_type - to select Interrupt type.
 * @return     none
 */
static inline unsigned char i2c_get_irq_status(i2c_irq_status_type_e irq_type)
{
	return (reg_i2c_irq_status & irq_type);
}


/**
 * @brief      This function serves to clear i2c interrupt status.
 * @param[in]  irq_type - to select Interrupt type.
 * @return     none
 */
static inline unsigned char i2c_clr_irq_status(i2c_irq_clr_type_e irq_clr_type)
{
	return (reg_i2c_status |= irq_clr_type);
}


/**
 * @brief      This function serves to enable slave mode.
 * @param[in]  id - the id of slave device.it contains write or read bit,the laster bit is write or read bit.
 *                       ID|0x01 indicate read. ID&0xfe indicate write.
 * @return     none
 */
void i2c_slave_init(unsigned char id);


/**
 *  @brief      The function of this API is to ensure that the data can be successfully sent out.
 *  @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 *  @param[in]  data - The data to be sent, The first three bytes can be set as the RAM address of the slave.
 *  @param[in]  len - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 *  @return     none
 */
void i2c_master_write(unsigned char id, unsigned char *data, unsigned char len);


/**
 * @brief      This function serves to read a packet of data from the specified address of slave device
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  data - Store the read data
 * @param[in]  len - The total length of the data read back.
 * @return     none.
 */
void i2c_master_read(unsigned char id, unsigned char *data, unsigned char len);



/**
 * @brief      The function of this API is just to write data to the i2c tx_fifo by DMA.
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  data - The data to be sent, The first three bytes represent the RAM address of the slave.
 * @param[in]  len - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 * @return     none.
 */
void i2c_master_write_dma(unsigned char id, unsigned char *data, unsigned char len);



/**
 * @brief      This function serves to read a packet of data from the specified address of slave device.
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  rx_data - Store the read data
 * @param[in]  len - The total length of the data read back.
 * @return     none.
 */
void i2c_master_read_dma(unsigned char id, unsigned char *data, unsigned char len);

/**
 * @brief      This function serves to set the i2c clock frequency.The i2c clock is consistent with the system clock.
 *             Currently, the default system clock is 48M, and the i2c clock is also 48M.
 * @param[in]  clock - the division factor of I2C clock,
 *             I2C frequency = System_clock / (4*DivClock).
 * @return     none
 */
void i2c_set_master_clk(unsigned char clock);

/**
 * @brief     This function serves to set i2c tx_dam channel and config dma tx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to set i2c rx_dam channel and config dma rx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_rx_dma_config(dma_chn_e chn);

#endif










