/********************************************************************************************************
 * @file	i2c.h
 *
 * @brief	This is the header file for B91
 *
 * @author	L.R 
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
/**	@page I2C
 *
 *	Introduction
 *	===============
 *	i2c support master mode or slave mode.
 *
 *	API Reference
 *	===============
 *	Header File: i2c.h
 */

#ifndef I2C_H
#define I2C_H
#include "gpio.h"
#include "../common/bit.h"
#include "analog.h"
#include "reg_include/i2c_reg.h"
#include "dma.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/


/**
 *  @brief  select pin as SDA and SCL of i2c
 */
typedef enum{
	I2C_GPIO_SDA_B3		= GPIO_PB3,
	I2C_GPIO_SDA_C2		= GPIO_PC2,
	I2C_GPIO_SDA_E2		= GPIO_PE2,
	I2C_GPIO_SDA_E3		= GPIO_PE3,
}i2c_sda_pin_e;


typedef enum{
	I2C_GPIO_SCL_B2		= GPIO_PB2,
	I2C_GPIO_SCL_C1		= GPIO_PC1,
	I2C_GPIO_SCL_E0		= GPIO_PE0,
	I2C_GPIO_SCL_E1		= GPIO_PE1,
}i2c_scl_pin_e;



typedef enum{
	I2C_RX_BUF_MASK         =  BIT(2),
	I2C_TX_BUF_MASK         =  BIT(3),
	I2C_RX_DONE_MASK        =  BIT(5),
}i2c_irq_mask_e;



typedef enum{
	I2C_RX_BUFF_CLR  		= BIT(6),
	I2C_TX_BUFF_CLR         = BIT(7),
}i2c_buff_clr_e;

typedef enum{

	I2C_TX_BUF_STATUS          = BIT(1),
	I2C_RXDONE_STATUS          = BIT(2),
	I2C_RX_BUF_STATUS          = BIT(3),
}i2c_irq_status_e;


/**
 * @brief      The function of this API is to determine whether the bus is busy.
 * @return     1:Indicates that the bus is busy. 0:Indicates that the bus is free
 */
static inline BOOL i2c_master_busy(void)
{
    return reg_i2c_mst & FLD_I2C_MST_BUSY;
}

/**
 * @brief      The function of this API is to Get the number of bytes in tx_buffer.
 * @return     The actual number of bytes in tx_buffer.
 */
static inline unsigned char i2c_get_tx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_TX_BUFCNT)>>4;
}


/**
 * @brief      The function of this API is to Get the number of bytes in rx_buffer.
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
 * @brief      The function of this interface is equivalent to that after the user finishes calling the write or read interface, the stop signal is not sent,
 * 			   and then the write or read command is executed again. The driver defaults that every write or read API will send a stop command at the end
 * @param[in]  en - Input parameters.Decide whether to disable the stop function after each write or read interface
 * @return     none
 */
void i2c_master_send_stop(unsigned char en);

/**
 * @brief      This function selects a pin port for I2C interface.
 * @param[in]  sda_pin - the pin port selected as I2C sda pin port.
 * @param[in]  scl_pin - the pin port selected as I2C scl pin port.
 * @return     none
 */
void i2c_set_pin(i2c_sda_pin_e sda_pin,i2c_scl_pin_e scl_pin);


/**
 * @brief      This function serves to enable i2c master function.
 * @return     none.
 */
void i2c_master_init(void);


/**
 * @brief      This function serves to enable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
 * @param[in]  mask - to select Interrupt type.
 * @return     none
 */
static inline void i2c_set_irq_mask(i2c_irq_mask_e mask)
{
	reg_i2c_sct0  |=  mask;
}

/**
 * @brief      This function serves to disable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
 * @param[in]  mask - to select Interrupt type.
 * @return     none
 */
static inline void i2c_clr_irq_mask(i2c_irq_mask_e mask)
{
	reg_i2c_sct0  &=  (~mask);
}


/**
 * @brief      This function serves to get i2c interrupt status.
 * @return     none
 *
 */
static inline unsigned char i2c_get_irq_status(i2c_irq_status_e status)
{
	return reg_i2c_irq_status&status;
}


/**
 * @brief      This function serves to clear i2c rx/tx fifo.
 * @param[in]  clr - to select Interrupt type.
 * @return     none
 */
static inline void i2c_clr_fifo(i2c_buff_clr_e clr)
{
	 reg_i2c_status = clr;
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
 * @param[in]  data - Store the read data
 * @param[in]  len - The total length of the data read back.
 * @return     none.
 */
void i2c_master_read_dma(unsigned char *data, unsigned char len);




/**
 * @brief      This function serves to write a packet of data to master device.
 * @param[in]  data - the pointer of tx_buff.
 * @param[in]  len - The total length of the data .
 * @return     none.
 */
void i2c_slave_write_dma( unsigned char *data, unsigned char len);



/**
 * @brief      This function serves to receive a packet of data from  master device.
 * @param[in]  data - the pointer of rx_buff.
 * @param[in]  len  - The total length of the data .
 * @return     none.
 */
void i2c_slave_read_dma(unsigned char *data, unsigned char len);


/**
 * @brief     This function serves to receive data .
 * @param[in]  data - the data need read.
 * @param[in]  len - The total length of the data
 * @return    none
 */
void i2c_slave_read(unsigned char* data , unsigned char len );


/**
 * @brief     This function serves to receive uart data by byte with not DMA method.
 * @param[in]  data - the data need send.
 * @param[in]  len - The total length of the data.
 * @return    none
 */
void i2c_slave_write(unsigned char* data , unsigned char len);


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










