/*******************************      analog control registers: 0xb8      ******************************/
#pragma once

#include "../common/types.h"
#include "dma.h"
#include "reg_include/register_9518.h"

/**
 * @brief      This function serves to analog register read by byte.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
u8 analog_read_reg8(u8 addr);

/**
 * @brief      This function serves to analog register write by byte.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
void analog_write_reg8(u8 addr, u8 data);

/**
 * @brief      This function serves to analog register read by halfword.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
u16 analog_read_reg16(u8 addr);

/**
 * @brief      This function serves to analog register write by halfword.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
void analog_write_reg16(u8 addr, u16 data);

/**
 * @brief      This function serves to analog register read by word.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
 u32 analog_read_reg32(u8 addr);

 /**
  * @brief      This function serves to analog register write by word.
  * @param[in]  addr - address need to be write.
  * @param[in]  data - the value need to be write.
  * @return     none.
  */
void analog_write_reg32(u8 addr, u32 data);
/**
 * @brief      This function serves to analog register read.
 * @param[in]  addr - address need to be read.
 * @param[in]  *buff - the value need to be read.
 * @param[in]  len - the length of read value.
 * @return     none.
 */
//void analog_read_buff(unsigned char addr, unsigned char* buff, int len);

/**
 * @brief      This function serves to analog register write.
 * @param[in]  addr - address need to be write.
 * @param[in]  *buff - the value need to be write.
 * @param[in]  len - the length of write value.
 * @return     none.
 */

//void analog_write_buff(unsigned char addr, unsigned char* buff, int len);



/**
 * @brief      This function serves to analog register read use DMA.
 * @param[in]  addr - address need to be read.
 * @param[in]  *buff - the value need to be read.
 * @param[in]  len - the length of read value.
 * @return     none.
 */
void analog_dma_write_reg32(dma_chn_e chn, unsigned char addr, void * pdat);
/**
 * @brief      This function serves to analog register write use DMA.
 * @param[in]  addr - address need to be read.
 * @param[in]  *buff - the value need to be read.
 * @param[in]  len - the length of read value.
 * @return     none.
 */
void analog_dma_read_reg32(dma_chn_e chn, unsigned char addr,void * pdat);
/**
 * @brief      This function serves to analog register write adr_dat mode.
 * @param[in]  addr - address need to be read.
 * @param[in]  *buff - the value need to be read.
 * @param[in]  len - the length of read value.
 * @return     none.
 */
void analog_dma_write_addr_data(dma_chn_e chn, void * pdat, int len);
