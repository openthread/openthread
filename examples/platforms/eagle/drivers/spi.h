/********************************************************************************************************
 * @file     hspi.h
 *
 * @brief    This is the header file for TLSR8258
 *
 * @author	 peng.sun@telink-semi.com;
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


#ifndef HSPI_H
#define HSPI_H

#include "reg_include/register_9518.h"
#include "gpio.h"
#include "dma.h"
extern u8 hspi_tx_dma_chn;
extern u8 hspi_rx_dma_chn;
extern u8 pspi_tx_dma_chn;
extern u8 pspi_rx_dma_chn;

typedef enum {
	PSPI_MODULE = 0,
    HSPI_MODULE = 1 ,
}spi_sel_e;


/**
 *  @brief  Define the mode for SPI io mode
 */
typedef enum {
	SPI_SINGLE_MODE = 0,
    SPI_DUAL_MODE=1 ,
    HSPI_QUAD_MODE=2,
	SPI_3_LINE_MODE=3
} spi_io_mode_e;




typedef enum {
	SPI_NOMAL = 0,
    SPI_3LINE =3 ,
} spi_nomal_3line_mode_e;


typedef enum {
	HSPI_SINGLE = 0,
    HSPI_DUAL   =1 ,
    HSPI_QUAD   =2,
} hpspi_single_dual_quad_mode_e;

typedef enum {
	HSPI_MODE_WRITE_AND_READ=0,//write and read at the same.must enbale CmdEn
	HSPI_MODE_WRITE_ONLY,//write
	HSPI_MODE_READ_ONLY,// read must enbale CmdEn
	HSPI_MODE_WRITE_READ,//write_ read
	HSPI_MODE_READ_WRITE,//read_write
	HSPI_MODE_WRITE_DUMMY_READ,//write_dummy_read
	HSPI_MODE_READ_DUMMY_WRITE,//read_ dummy_write must enbale CmdEn
	HSPI_MODE_NONE_DATA,//must enbale CmdEn
	HSPI_MODE_DUMMY_WRITE,//dummy_write
	HSPI_MODE_DUMMY_READ,//dummy_read
	HSPI_MODE_RESERVED,
}spi_tans_mode_e;


typedef enum {
	SPI_MODE_WR_WRITE_ONLY      =1,//write
	SPI_MODE_WR_DUMMY_WRITE     =8,//dummy_write
}spi_wr_tans_mode_e;

typedef enum {
	SPI_MODE_RD_READ_ONLY       =2,//must enbale CmdEn
	SPI_MODE_RD_DUMMY_READ      =9,//dummy_read
}spi_rd_tans_mode_e;


typedef struct {
	hpspi_single_dual_quad_mode_e  hspi_io_mode;//set spi interface mode
	u8  hspi_dummy_cnt;//set dummy cnt if tans_mode have dummy .
	u8  hspi_cmd_en;//enable cmd phase
	u8  hspi_addr_en;//enable address phase
	u8  hspi_addr_len;//enable address phase
	u8  hspi_cmd_fmt_en;//if cmd_en enable cmd fmt will follow the interface (dual/quad)
	u8  hspi_addr_fmt_en;//if addr_en enable addr fmt will follow the interface (dual/quad)
}hspi_config_st;

typedef enum {
	PSPI_SINGLE= 0,
    PSPI_DUAL=1 ,
} pspi_single_dual_mode_e;

typedef struct {
	pspi_single_dual_mode_e  pspi_io_mode;//set spi interface mode
	u8  pspi_dummy_cnt;//set dummy cnt if tans_mode have dummy .
    _Bool  pspi_cmd_en;//enable cmd phase
}pspi_config_st;




typedef enum{
	HSPI_SCLK_PB4 = GPIO_PB4,
	HSPI_SCLK_PA2= GPIO_PA2,
}hspi_sclk_pin_def_e;

typedef enum{
	HSPI_CS_PB6 = GPIO_PB6,
	HSPI_CS_PA1= GPIO_PA1,
}hspi_cs_pin_def_e;


typedef enum{
	HSPI_SDO_PB3 = GPIO_PB3,
	HSPI_SDO_PA4 = GPIO_PA4,
}hspi_sdo_pin_def_e;

typedef enum{
	HSPI_SDI_PB2 = GPIO_PB2,
	HSPI_SDI_PA3 = GPIO_PA3,
}hspi_sdi_pin_def_e;

typedef enum{
	HSPI_WP_PB1 = GPIO_PB1,
}hspi_wp_pin_def_e;

typedef enum{
	HSPI_HOLD_PB0 = GPIO_PB0,
}hspi_hold_pin_def_e;



typedef enum{
	HSPI_SCLK_PB4_PIN = GPIO_PB4,
	HSPI_SCLK_PA2_PIN = GPIO_PA2,

	HSPI_CS_PB6_PIN = GPIO_PB6,
	HSPI_CS_PA1_PIN = GPIO_PA1,

	HSPI_SDO_PB3_PIN = GPIO_PB3,
	HSPI_SDO_PA4_PIN = GPIO_PA4,

	HSPI_SDI_PB2_PIN = GPIO_PB2,
	HSPI_SDI_PA3_PIN = GPIO_PA3,

	HSPI_WP_PB1_PIN  = GPIO_PB1,
	HSPI_HOLD_PB0_PIN = GPIO_PB0,
	HSPI_NONE_PIN = 0xfff,
}hspi_pin_def_e;




typedef struct {
	hspi_sclk_pin_def_e          hspi_sclk_pin;
	hspi_cs_pin_def_e            hspi_cs_pin;
	hspi_sdo_pin_def_e           hspi_sdo_pin;
	hspi_sdi_pin_def_e           hspi_sdi_pin;
	hspi_wp_pin_def_e            hspi_wp_pin;
	hspi_hold_pin_def_e          hspi_hold_pin;
}hspi_pin_config_st;


typedef enum {
	SPI_WRITE_DATA_CMD=0x00,
	SPI_WRITE_DATA_DUL_CMD=FLD_SPI_CMD_DATA_DUAL,
	SPI_WRITE_ADDR_DUL_CMD=FLD_SPI_CMD_ADDR_DUAL,

	SPI_WRITE_DATA_DUL_4CYC_CMD=FLD_SPI_CMD_DATA_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_WRITE_ADDR_DUL_4CYC_CMD=FLD_SPI_CMD_ADDR_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_WRITE_DATA_AND_ADDR_DUL_4CYC_CMD=FLD_SPI_CMD_ADDR_DUAL|FLD_SPI_CMD_DATA_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,
}spi_write_cmd_e;


typedef enum {
	SPI_READ_DATA_CMD=FLD_SPI_CMD_RD_EN,
	SPI_READ_DATA_DUL_CMD=FLD_SPI_CMD_RD_EN|FLD_SPI_CMD_DATA_DUAL,
	SPI_READ_ADDR_DUL_CMD=FLD_SPI_CMD_RD_EN|FLD_SPI_CMD_ADDR_DUAL,

	SPI_READ_DATA_DUL_4CYC_CMD=FLD_SPI_CMD_RD_EN|FLD_SPI_CMD_DATA_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_READ_ADDR_DUL_4CYC_CMD=FLD_SPI_CMD_RD_EN|FLD_SPI_CMD_ADDR_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,

	SPI_READ_DATA_AND_ADDR_DUL_4CYC_CMD=FLD_SPI_CMD_RD_EN|FLD_SPI_CMD_ADDR_DUAL|FLD_SPI_CMD_DATA_DUAL|FLD_SPI_CMD_RD_DUMMY_4CYCLE,

}spi_read_cmd_e;


enum {
	SPI_READ_STATUS_SINGLE_CMD=0x05,
	SPI_READ_STATUS_DUAL_CMD  =0x15,
	HSPI_READ_STATUS_QUAD_CMD =0x25,
	SPI_READ_DATA_SINGLE_CMD  =0x0B,
	SPI_READ_DATA_DUAL_CMD    =0x0C,
	HSPI_READ_DATA_QUAD_CMD   =0x0E,
	SPI_WRITE_DATA_SINGLE_CMD =0x51,
	SPI_WRITE_DATA_DUAL_CMD   =0x52,
	HSPI_WRITE_DATA_QUAD_CMD  =0x54,
};





/**
 *  @brief  Define the work mode
 */
typedef enum {
    SPI_MODE0 = 0,
    SPI_MODE2,
    SPI_MODE1,
    SPI_MODE3,
} spi_mode_type_e;



typedef enum{
	PSPI_SCLK_PC5 = GPIO_PC5,
	PSPI_SCLK_PB5 = GPIO_PB5,
	PSPI_SCLK_PD1 = GPIO_PD1,
}pspi_sclk_pin_def_e;

typedef enum{
	PSPI_CS_PC4 = GPIO_PC4,
	PSPI_CS_PC0 = GPIO_PC0,
	PSPI_CS_PD0 = GPIO_PD0,
}pspi_cs_pin_def_e;


typedef enum{
	PSPI_SDO_PC7 = GPIO_PC7,
	PSPI_SDO_PB7 = GPIO_PB7,
	PSPI_SDO_PD3 = GPIO_PD3,
}pspi_sdo_pin_def_e;

typedef enum{
	PSPI_SDI_PC6 = GPIO_PC6,
	PSPI_SDI_PB6 = GPIO_PB6,
	PSPI_SDI_PD2 = GPIO_PD2,
}pspi_sdi_pin_def_e;
typedef enum{
	PSPI_SCLK_PC5_PIN = GPIO_PC5,
	PSPI_SCLK_PB5_PIN = GPIO_PB5,
	PSPI_SCLK_PD1_PIN = GPIO_PD1,

	PSPI_CS_PC4_PIN = GPIO_PC4,
	PSPI_CS_PC0_PIN = GPIO_PC0,
	PSPI_CS_PD0_PIN = GPIO_PD0,

	PSPI_SDO_PC7_PIN = GPIO_PC7,
	PSPI_SDO_PB7_PIN = GPIO_PB7,
	PSPI_SDO_PD3_PIN = GPIO_PD3,

	PSPI_SDI_PC6_PIN = GPIO_PC6,
	PSPI_SDI_PB6_PIN = GPIO_PB6,
	PSPI_SDI_PD2_PIN = GPIO_PD2,
	PSPI_NONE_PIN = 0xfff,
}pspi_pin_def_e;




typedef struct {
	pspi_sclk_pin_def_e     pspi_sclk_pin;
	pspi_cs_pin_def_e       pspi_cs_pin;
	pspi_sdo_pin_def_e      pspi_sdo_pin;
	pspi_sdi_pin_def_e      pspi_sdi_pin;
}pspi_pin_config_st;


/**
 * @brief This function reset HSPI module.
 * @param[in] none
 * @return none
 */
static inline void hspi_reset(void)
{
	reg_rst0 &= (~FLD_RST0_HSPI );
	reg_rst0 |=FLD_RST0_HSPI;

}
/**
 * @brief This function reset SPI module.
 * @param[in] none
 * @return none
 */
static inline void pspi_reset(void)
{
	reg_rst1 &= (~FLD_RST1_PSPI);
	reg_rst1 |= FLD_RST1_PSPI;

}


/**
 * @brief This function get tx_fifo number.
 * @param[in] none
 * @return ntx_fifo number that wait to be sent
 */
static inline  unsigned char spi_get_txfifo_num(spi_sel_e spi_sel)
{
   return (reg_spi_fifo_num(spi_sel)&FLD_SPI_TXF_NUM )>>4;
}


/**
 * @brief This function get rx_fifo number.
 * @param[in] none
 * @return rx_fifo num that have been received
 */
static inline unsigned char spi_get_rxfifo_num(spi_sel_e spi_sel)
{
   return reg_spi_fifo_num(spi_sel)&FLD_SPI_RXF_NUM ;
}


/**
 * @brief This function set SPI rx_cnt.
 * @param[in] cnt  rx amount of data
 * @return none
 */
static inline void spi_rx_cnt(spi_sel_e spi_sel,u32 cnt)
{

	reg_spi_rx_cnt2(spi_sel)=((cnt-1)>>16)&0xff;
	reg_spi_rx_cnt1(spi_sel)=((cnt-1)>>8)&0xff;
	reg_spi_rx_cnt0(spi_sel)=(cnt-1)&0xff;

}

/**
 * @brief This function set SPI tx_cnt.
 * @param[in] cnt  tx amount of data
 * @return none
 */
static inline void spi_tx_cnt(spi_sel_e spi_sel,u32 cnt)
{

	reg_spi_tx_cnt2(spi_sel)=((cnt-1)>>16)&0xff;
	reg_spi_tx_cnt1(spi_sel)=((cnt-1)>>8)&0xff;
    reg_spi_tx_cnt0(spi_sel)= (cnt-1)&0xff;

}


/**
 * @brief This function  clear tx_fifo.
 * @param[in] none
 * @return none
 */
static inline void spi_tx_fifo_clr(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_fifo_state(spi_sel),FLD_SPI_TXF_CLR);
}


/**
 * @brief This function  clear rx_fifo.
 * @param[in] none
 * @return none
 */
static inline void spi_rx_fifo_clr(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_fifo_state(spi_sel),FLD_SPI_RXF_CLR);
}


/**
 * @brief This function set hspi command content
 * @param[in] cmd: command content
 * @return none
 */
static inline void spi_set_cmd(spi_sel_e spi_sel,u8 cmd )
{
	reg_spi_trans1(spi_sel)=cmd;

}

/**
 * @brief This function servers to enable cmd which will sent a byte cmd .
 * @param[in] none
 * @return none
 */
static inline void spi_cmd_en(spi_sel_e spi_sel)
{

	BM_SET(	reg_spi_mode2(spi_sel),FLD_SPI_CMD_EN);
}


/**
 * @brief This function servers to disable cmd which will not sent cmd .
 * @param[in] none
 * @return none
 */
static inline void spi_cmd_dis(spi_sel_e spi_sel)
{

	BM_CLR(reg_spi_mode2(spi_sel),FLD_SPI_CMD_EN);
}




/**
 * @brief This function servers enable cmd  format,the format of cmd phase is the same as the data phase(Dual/Quad).
 * @return none
 */
static inline void hspi_cmd_fmt_en()
{
	BM_SET(	reg_spi_mode2(HSPI_MODULE),FLD_HSPI_CMD_FMT);
}

/**
 * @brief This function servers disable cmd  format).
 * @return none
 */
static inline void hspi_cmd_fmt_dis()
{
	BM_CLR(reg_spi_mode2(HSPI_MODULE),FLD_HSPI_CMD_FMT );
}




/**
 * @brief This function servers to enable hspi quad mode.
 * @param[in] none
 * @return none
 */
static inline void hspi_quad_mode_en()
{
	BM_SET(reg_spi_mode2(HSPI_MODULE),FLD_HSPI_QUAD );
}


/**
 * @brief This function servers to disable hspi quad mode.
 * @param[in] none
 * @return none
 */
static inline void hspi_quad_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode2(spi_sel),FLD_HSPI_QUAD );
}

/**
 * @brief This function servers to enable hspi dual mode.
 * @param[in] none
 * @return none
 */
static inline void spi_dual_mode_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_mode0(spi_sel),FLD_SPI_DUAL );
}


/**
 * @brief This function servers to disable hspi dual mode.
 * @param[in] none
 * @return none
 */
static inline void spi_dual_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode0(spi_sel),FLD_SPI_DUAL );
}

/**
 * @brief This function servers to enable hspi 3line mode..
 * @param[in] none
 * @return none
 */
static inline void spi_3line_mode_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_mode0(spi_sel),FLD_SPI_3LINE );
}


/**
 * @brief This function servers to disable hspi 3line mode..
 * @param[in] none
 * @return none
 */
static inline void spi_3line_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode0(spi_sel),FLD_SPI_3LINE);
}


/**
 * @brief This function to enable address format.the format of addr phase is the same as the data phase(Dual/Quad).
 * @param[in] none.
 * @return none
 */
static inline void hspi_addr_fmt_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_ADDR_FMT) ;

}

/**
 * @brief This function to disable address format.
 * @param[in] none.
 * @return none
 */
static inline void hspi_addr_fmt_dis(void)
{
	BM_CLR(	reg_hspi_xip_ctrl,FLD_HSPI_ADDR_FMT) ;

}



/**
 * @brief      This function to determine whether the bus is busy..
 * @param[in]  none.
 * @return     1:Indicates that the bus is busy. 0:Indicates that the bus is free
 */
static inline _Bool spi_is_busy(spi_sel_e spi_sel)
{
	 return  reg_spi_status(spi_sel)&FLD_HSPI_BUSY ;

}

/**
 * @brief This function enable tx dma.
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void spi_tx_dma_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_trans2(spi_sel),FLD_SPI_TX_DMA_EN) ;
}



/**
 * @brief This function enable tx dma.
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void spi_tx_dma_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_trans2(spi_sel),FLD_SPI_TX_DMA_EN);
}


/**
 * @brief This function enable rx dma.
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void spi_rx_dma_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_trans2(spi_sel),FLD_SPI_RX_DMA_EN);
}

/**
 * @brief This function enable rx dma.
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void spi_rx_dma_dis(spi_sel_e spi_sel)
{

	BM_CLR(reg_spi_trans2(spi_sel),FLD_SPI_RX_DMA_EN );
}
/**
 * @brief This function set xip write command content
 * @param[in] cmd: write command content
 * @return none
 */
static inline void hspi_xip_wr_cmd_set(u8 wr_cmd )
{
	reg_hspi_xip_wr_cmd	= wr_cmd;
}

/**
 * @brief This function set xip read command content
 * @param[in] cmd: read command content
 * @return none
 */
static inline void hspi_xip_rd_cmd_set(u8 rd_cmd )
{

	reg_hspi_xip_rd_cmd	= rd_cmd;
}



/**
 * @brief This function set xip address offset
 * @param[in] addr_offset:
 * @return none
 */
static inline void hspi_xip_addr_offset(u32  addr_offset)
{
    reg_hspi_xip_addr_offset0= addr_offset&0xff ;
	reg_hspi_xip_addr_offset1= (addr_offset>>8)&0xff;
	reg_hspi_xip_addr_offset2= (addr_offset>>16)&0xff;
	reg_hspi_xip_addr_offset3= (addr_offset>>24)&0xff;
}

/**
 * @brief     This function servers to set xip read transfer mode。
 * @param[in] rd_mode: read mode in enum hspi_tans_mode_e
 * @return    none
 */
static inline void hspi_xip_read_transmode(u8  rd_mode)
{
	reg_hspi_xip_trans_mode	&=(~FLD_HSPI_XIP_RD_TRANS_MODE);
	reg_hspi_xip_trans_mode|=  (rd_mode&0xf)<<4 ;
}

/**
 * @brief     This function servers to set xip write transfer mode。
 * @param[in] wr_mode:write mode in enum hspi_tans_mode_e
 * @return    none
 */
static inline void hspi_xip_write_transmode(u8  wr_mode)
{
	reg_hspi_xip_trans_mode	&=(~FLD_HSPI_XIP_WR_TRANS_MODE);
	reg_hspi_xip_trans_mode|=  wr_mode& FLD_HSPI_XIP_WR_TRANS_MODE ;
}

/**
 * @brief This function enable address phase .
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void hspi_addr_en(void)
{
 BM_SET(reg_hspi_xip_ctrl,FLD_HSPI_ADDR_EN);
}



/**
 * @brief This function enable address phase .
 * @param[in] 1:enable 0:disable
 * @return none
 */
static inline void hspi_addr_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl,FLD_HSPI_ADDR_EN);
}




/**
 * @brief This function servers to set hspi address length.
 * @param[in] len :2'b00:1bye  2'b01:2bytes  2'b10:3bytes  2'b11:4bytes
 * @return none
 */
static inline void hspi_set_addr_len(u8 len)
{
	reg_hspi_xip_ctrl|= ((len-1)&0x3)<<2;
}


/**
 * @brief This function servers to enable xip sequential mode
 * @param[in] none
 * @return none
 */
static inline void hspi_xip_seq_mode_en(void)
{
	BM_SET(reg_hspi_xip_ctrl,FLD_HSPI_XIP_MODE);
}

/**
 * @brief This function servers to disable xip sequential mode
 * @param[in] none
 * @return none
 */
static inline void hspi_xip_seq_mode_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl,FLD_HSPI_XIP_MODE);
}

/**
 * @brief This function servers to enable xip.
 * @param[in] 1:enable
 * @return none
 */
static inline void hspi_xip_en(void)
{
	BM_SET(reg_hspi_xip_ctrl,FLD_HSPI_XIP_ENABLE);
}

/**
 * @brief This function servers to stop xip.
 * @param[in] 1:enable
 * @return none
 */
static inline void hspi_xip_stop(void)
{
	BM_SET(reg_hspi_xip_ctrl,FLD_HSPI_XIP_STOP);
}


/**
 * @brief This function servers to enable xip timeout that set hight level .
 * @param[in] none
 * @return none
 */
static inline void hspi_xip_timeout_mode_en(void)
{
	BM_SET(reg_hspi_xip_ctrl,FLD_HSPI_XIP_TIMEOUT_MODE);
}


/**
 * @brief This function servers to disable xip timeout.
 * @param[in] none
 * @return none
 */
static inline void hspi_xip_timeout_mode_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl,FLD_HSPI_XIP_TIMEOUT_MODE);
}


/**
 * @brief This function servers to set xip timeout cnt,when two data frame intervals exceed spi_clock_out_period*timeout_cnt,cs will set high level.
 * @param[in] cnt: xip timeout cnt.
 * @return none
 */
static inline void hspi_xip_timeout_cnt(u8 cnt )
{
	reg_hspi_xip_timeout_cnt = cnt&0xff;
}


/**
 * @brief This function servers to set page_size
 * @param[in] page_size_i  page boundary 2^page_size_i bytes.
 * @return none
 */
static inline void hspi_xip_page_size(u8 page_size_i)
{
	reg_hspi_page_size = page_size_i;
}

/**
 * @brief This function servers to reply master slave is ready .When slave is ready,  slave ready reply a byte data:0x5a.   indicating that slave is ready for data transmission.
 * @param[in] none:
 * @return none
 */
static inline void spi_slave_ready_en(spi_sel_e spi_sel)
{
	  BM_SET(reg_spi_status(spi_sel), FLD_HSPI_SLAVE_READY);
}

/**
 * @brief This function servers to reply master slave is not ready.slave reply a byte data: 0x00.indicating that slave is not ready for data transmission.
 * @param[in] none
 * @return none
 */
static inline void spi_slave_ready_dis(spi_sel_e spi_sel)
{
	  BM_CLR(reg_spi_status(spi_sel), FLD_HSPI_SLAVE_READY);
}

/**
 * @brief This function servers to read cmd from master for slave.
 * @return  cmd transferd by master.
 */
static inline u8 spi_slave_get_cmd(spi_sel_e spi_sel)
{
	return   reg_spi_trans1(spi_sel);
}


/**
 * @brief      The function of this API is to set the number of bytes to triggered the receive and transmit interrupt.
 *             Its default value is 4.  recommend setting  4.
 * @param[in]  cnt - the interrupt trigger level.
 * @return     none
 */
static inline void spi_rx_tx_irq_trig_cnt(spi_sel_e spi_sel,u8 cnt)
{
	BM_CLR(reg_spi_status(spi_sel),FLD_HSPI_FIFO_THRES);
	reg_spi_status(spi_sel) |= ((cnt&7)<<4);
}

/**
 * @brief This function servers to get irq status.
 * @return  cmd transferd by master.
 */
static inline u8 spi_get_irq_status(spi_sel_e spi_sel)
{
	return   reg_spi_interrupt_state(spi_sel);
}

/**
 * @brief This function servers to clear irq status.
 * @return  cmd transferd by master.
 */
static inline void spi_clr_irq_status(spi_sel_e spi_sel ,u8 mask)
{
   reg_spi_interrupt_state(spi_sel)=mask;
}


/**
 * @brief This function servers to set irq mask.
 * @return  cmd transferd by master.
 */
static inline void spi_set_irq_mask(spi_sel_e spi_sel,u8 mask)
{
   BM_SET( reg_spi_trans2(spi_sel),mask);
}


/**
 * @brief This function servers to set irq mask.
 * @return  cmd transferd by master.
 */
static inline void spi_clr_irq_mask(spi_sel_e spi_sel,u8 mask)
{
	BM_CLR( reg_spi_trans2(spi_sel),mask);
}



/**
 * @brief     This function selects  pin  for hspi master or slave.
 * @param[in]  pin - the selected pin.
 * @return    none
 */
//void hspi_set_pin_mux(hspi_pin_def_e pin);
void spi_slave_set_pin(void);
/**
 * @brief     This function configures hspi pin.
 * @param[in] config - the pointer of pin config struct.
 * @return    none
 */
void hspi_set_pin(hspi_pin_config_st * config);

/**
 * @brief     This function configures hspi pin.
 * @param[in] config - the pointer of pin config struct.
 * @return    none
 */
void pspi_set_pin(pspi_pin_config_st * config);


/**
 * @brief     This function configures the clock and working mode for SPI interface
 * @param[in] div_clock - the division factor for SPI module
 *            spi_clock_out = ahb_clock / ((div_clock+1)*2)
 * @param[in] mode - the selected working mode of SPI module
 *            bit5:CPHA-Clock Polarity  ; bit6:CPOL:CPHA-Clock Phase
 *            MODE0:  CPHA = 0 , CPOL =0;
 *            MODE1:  CPHA = 0 , CPOL =1;
 *            MODE2:  CPHA = 1 , CPOL =0;
 *            MODE3:  CPHA = 1,  CPOL =1;
 * @return    none
 */
void spi_master_init(spi_sel_e spi_sel,u8 div_clock, spi_mode_type_e mode);

/**
 * @brief     This function configures the clock and working mode for SPI interface
 * @param[in] mode - the selected working mode of SPI module
 *           bit5:CPHA-Clock Polarity  ; bit6:CPOL:CPHA-Clock Phase
 *            MODE0:  CPHA = 0 , CPOL =0;
 *            MODE1:  CPHA = 0 , CPOL =1;
 *            MODE2:  CPHA = 1 , CPOL =0;
 *            MODE3:  CPHA = 1,  CPOL =1;
 * @return    none
 * note  spi_clock_in ＋ (spi_slave_clock frequency)/3
 */
void spi_slave_init(spi_sel_e spi_sel,spi_mode_type_e mode);

/**
 * @brief     This function servers to set dummy cycle cnt
 * @param[in] dummy_cnt: the cnt of dummy clock.
 * @return    none
 */
void spi_set_dummy_cnt(spi_sel_e spi_sel,u8 dummy_cnt);



/**
 * @brief     This function servers to set slave address hspi only 。
 * @param[in] addr:address of slave
 * @return    none
 */
void hspi_set_address(u32 addr);

/**
 * @brief     This function servers to set spi transfer mode。
 * @param[in] spi_tans_mode_e:
 * @return    none
 */
void spi_set_transmode(spi_sel_e spi_sel,spi_tans_mode_e mode);


/**
 * @brief     This function servers to set normal mode。
 * @param[in] none
 * @return    none
 */
void spi_set_normal_mode(spi_sel_e spi_sel);

/**
 * @brief     This function servers to set dual mode。
 * @param[in] none
 * @return    none
 */
void spi_set_dual_mode(spi_sel_e spi_sel);

/**
 * @brief     This function servers to set quad mode。
 * @param[in] none
 * @return    none
 */
void hspi_set_quad_mode();

/**
 * @brief     This function servers to set 3line mode。
 * @param[in] none
 * @return    none
 */
void spi_set_3line_mode(spi_sel_e spi_sel);


/**
 * @brief     This function servers to set hspi io  mode。
 * @param[in] spi_io_mode_e: single/dual/quad /3line.
 * @return    nonee
  */
void spi_set_io_mode(spi_sel_e spi_sel,spi_io_mode_e mode);
/**
 * @brief     This function servers to config normal mode。
 * @param[in] mode: nomal ,mode 3line
 * @return    none
 */
void spi_master_config(spi_sel_e spi_sel,spi_nomal_3line_mode_e mode );
/**
 * @brief     This function configures DMA control register.
 * @param[in] config - the prt of dma_config that configured control register
 * @return    none
 */
void hspi_master_config_plus(hspi_config_st *config);
/**
 * @brief     This function servers to special mode。
 * @param[in] config: the pointer of pin special config struct.
 * @return    none
 */
void pspi_master_config_plus(pspi_config_st *config);

/**
 * @brief     This function serves to normal write data in normal.
 * @param[in] data:  the pointer to the data for write.
 * @param[in] len: write length.
 * @return    none
 */
void spi_master_write(spi_sel_e spi_sel,u8 *data, u32 len);


/**
 * @brief     This function serves to normal write and read data.
 * @param[in] write: the pointer to the data for write.
 * @param[in] wr_len:write length.
 * @param[in] rd_data:the pointer to the data for read.
 * @param[in] rd_len:read length.
 * @return    none
 */
void spi_master_write_read(spi_sel_e spi_sel,u8 *wr_data, u32 wr_len,u8 *rd_data, u32 rd_len);


/**
 * @brief      This function serves to single/dual/quad write to the SPI slave
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr:the address of slave
 * @param[in]  addr_len:the length of address
 * @param[in]  data: pointer to the data need to write
 * @param[in]  data_len:length in byte of the data need to write
 * @param[in]  wr_mode: write mode.dummy or not dummy
 * @return     none
 */
void spi_master_write_plus(spi_sel_e spi_sel,u8 cmd, u32 addr,u8 *data, u32 data_len, spi_wr_tans_mode_e wr_mode);

/**
 * @brief      This function serves to single/dual/quad  read from the SPI slave
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr:the address of slave
 * @param[in]  addr_len:the length of address
 * @param[in]  data: pointer to the data need to read
 * @param[in]  rd_mode: read mode.dummy or not dummy
 * @return     none
 */
void spi_master_read_plus(spi_sel_e spi_sel,u8 cmd, u32 addr,u8 *data, u32 data_len,spi_rd_tans_mode_e rd_mode);


/**
 * @brief     This function serves to set tx_dam channel and config dma tx default.
 * @param[in] dma_chn_e: dma channel.
 * @return    none
 */
void hspi_set_tx_dma_config(dma_chn_e chn);


/**
 * @brief     This function serves to set rx_dam channel and config dma rx default.
 * @param[in] dma_chn_e: dma channel.
 * @return    none
 */
void hspi_set_rx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to set tx_dam channel and config dma tx default.
 * @param[in] dma_chn_e: dma channel.
 * @return    none
 */
void pspi_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to set rx_dam channel and config dma rx default.
 * @param[in] dma_chn_e: dma channel.
 * @return    none
 */
void pspi_set_rx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to normal write data by dma.
 * @param[in] data:  the pointer to the data for write.
 * @param[in] len: write length.
 * @return    none
 */
void spi_master_write_dma(spi_sel_e spi_sel,u8 *src_addr, u32 len);


/**
 * @brief     This function serves to normal write and read data by dma.
 * @param[in] write: the pointer to the data for write.
 * @param[in] wr_len:write length.
 * @param[in] rd_data:the pointer to the data for read.
 * @param[in] rd_len:read length.
 * @return    none
 */
void spi_master_write_read_dma(spi_sel_e spi_sel,u8 *src_addr, u32 wr_len,u8 * dst_addr, u32 rd_len);


/**
 * @brief      This function serves to single/dual/quad  write to the SPI slave by dma.
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr:the address of slave
 * @param[in]  addr_len:the length of address
 * @param[in]  data: pointer to the data need to write
 * @param[in]  data_len:length in byte of the data need to write
 * @param[in]  wr_mode: write mode.dummy or not dummy
 * @return     none
 */
void spi_master_write_dma_plus(spi_sel_e spi_sel,u8 cmd,u32 addr,u8 * src_addr, u32 data_len,spi_wr_tans_mode_e wr_mode);
/**
 * @brief      This function serves to single/dual/quad  read from the SPI slave by dma
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr:the address of slave
 * @param[in]  addr_len:the length of address
 * @param[in]  dst_addr: pointer to the buffer that will cache the reading out data
 * @param[in]  data_len:length in byte of the data need to read
 * @param[in]  rd_mode: read mode.dummy or not dummy
 * @return     none
 */
void spi_master_read_dma_plus(spi_sel_e spi_sel,u8 cmd, u32 addr,u8 * dst_addr,u32 data_len,spi_rd_tans_mode_e rd_mode);



/**
 * @brief      This function serves to single/dual (quad) write to the SPI slave by xip
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr_offset:offset of xip base address.
 * @param[in]  data: pointer to the data need to write
 * @param[in]  data_len:length in byte of the data need to write
 * @param[in]  wr_mode: write mode  dummy or not dummy
 * @return     none
 */
void hspi_master_write_xip( u8 cmd,u32 addr_offset, u8 *Data,u32 data_len,spi_wr_tans_mode_e wr_mode);

/**
 * @brief      This function serves to single/dual (quad)  read from the SPI slave by xip
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr_offset:offset of xip base address.
 * @param[in]  data: pointer to the data need to read
 * @param[in]  addr_len:the length of address
 * @param[in]  rd_mode: read mode.dummy or not dummy
 * @return     none
 */
void hspi_master_read_xip(u8 cmd,u32 addr_offset, u8 *Data, u32 data_len,spi_rd_tans_mode_e rd_mode);



/**
 * @brief      This function serves to a cmd and one data write to the SPI slave by xip
 * @param[in]  cmd:cmd one byte will first write.
 * @param[in]  addr_offset:offset of xip base address.
 * @param[in]  data_in: data need to write
 * @param[in]  wr_mode: write mode  dummy or not dummy
 * @return     none
 */
void hspi_master_write_xip_cmd_data( u8 cmd,u32 addr_offset,u8 data_in,spi_wr_tans_mode_e wr_mode);
#endif


