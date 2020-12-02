/********************************************************************************************************
 * @file	analog.h
 *
 * @brief	This is the header file for B91
 *
 * @author	B.Y
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
/*******************************      analog control registers: 0xb8      ******************************/
/**	@page ANALOG
 *
 *	Introduction
 *	===============
 *	TLSRB91 analog support dma and normal mode, in each mode, support byte/halfword/word/buffer write and read. 
 *
 *	API Reference
 *	===============
 *	Header File: analog.h
 */
#pragma once

#include "../common/types.h"
#include "dma.h"
#include "compiler.h"
#include "reg_include/register_b91.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/**
 * @brief      This function serves to analog register read by byte.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned char analog_read_reg8(unsigned char addr);

/**
 * @brief      This function serves to analog register write by byte.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg8(unsigned char addr, unsigned char data);

/**
 * @brief      This function serves to analog register read by halfword.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
unsigned short analog_read_reg16(unsigned char addr);

/**
 * @brief      This function serves to analog register write by halfword.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
void analog_write_reg16(unsigned char addr, unsigned short data);

/**
 * @brief      This function serves to analog register read by word.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
 unsigned int analog_read_reg32(unsigned char addr);

 /**
  * @brief      This function serves to analog register write by word.
  * @param[in]  addr - address need to be write.
  * @param[in]  data - the value need to be write.
  * @return     none.
  */
void analog_write_reg32(unsigned char addr, unsigned int data);
/**
 * @brief      This function serves to analog register read.
 * @param[in]  addr  - address need to be read.
 * @param[out] buff  - the ptr of buffer to store the read data.
 * @param[in]  len   - the length of read value.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_read_buff(unsigned char addr, unsigned char *buff, int len);

/**
 * @brief      This function serves to analog register write.
 * @param[in]  addr  - address need to be write.
 * @param[in]  buff  - the ptr of value need to be write.
 * @param[in]  len   - the length of write value.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_buff(unsigned char addr, unsigned char *buff, int len);


/**
 * @brief      This function serves to analog register write by word using dma.
 * @param[in]  chn  - the dma channel.
 * @param[in]  addr - address need to be write.
 * @param[in]  pdat - the ptr of data need to be write.
 * @return     none.
 */
void analog_write_reg32_dma(dma_chn_e chn, unsigned char addr, void *pdat);

/**
 * @brief      This function serves to analog register write by word using dma.
 * @param[in]  chn  - the dma channel.
 * @param[in]  addr - address need to be read.
 * @param[out] pdat - the buffer ptr to store read data.
 * @return     none.
 */
void analog_read_reg32_dma(dma_chn_e chn, unsigned char addr,void *pdat);

/**
 * @brief      This function write buffer to analog register by dma channel.
 * @param[in]  chn  - the dma channel.
 * @param[in]  addr - address need to be write.
 * @param[in]  pdat - the buffer ptr need to be write.
 * @param[in]  len  - the length of buffer.
 * @return     none.
 */
void analog_write_buff_dma(dma_chn_e chn, unsigned char addr, unsigned char *pdat, unsigned int len);

/**
 * @brief      This function write buffer to analog register by dma channel.
 * @param[in]  chn  - the dma channel.
 * @param[in]  addr - address need to be read from.
 * @param[out] pdat - the buffer ptr to store read data.
 * 			   note: The size of array pdat must be a multiple of 4.
 * 			  	 	For example, if you just need read 5 byte by dma, you should
 * 			  	 	define the size of array pdat to be greater than 8 other than 5.
 * 			  	 	Because the dma would return 4 byte data every time, 5 byte is
 * 			  	 	not enough to store them.
 * @param[in]  len  - the length of read data.
 * @return     none.
 */
void analog_read_buff_dma(dma_chn_e chn, unsigned char addr, unsigned char *pdat, unsigned int len);

/**
 * @brief      This function write buffer to analog register by dma channel.
 * @param[in]  chn  - the dma channel.
 * @param[in]  pdat - the buffer(addr & data) ptr need to be write,
 * 			   note: The array pdat should look like this,
 * 			   |  pdat     |            |        |
 * 			   |  :------  | :----------|  :---- |
 * 			   |  pdat[0]  |   address  |  0x3a  |
 * 			   |  pdat[1]  |    data    |  0x11  |
 * 			   |  pdat[2]  |   address  |  0x3b  |
 *			   |  pdat[3]  |    data    |  0x22  |
 *			   |  ......   |            |        |
 * 				It means write data 0x11 to address 0x3a,
 * 						 write data 0x22 to address 0x3b,
 * 						 ......
 * @param[in]  len - the length of read data.
 * @return     none.
 */
void analog_write_addr_data_dma(dma_chn_e chn, void *pdat, int len);
