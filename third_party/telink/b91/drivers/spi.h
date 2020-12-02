/********************************************************************************************************
 * @file	spi.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H / J.C.F
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
#ifndef SPI_H
#define SPI_H

#include "reg_include/register_b91.h"
#include "gpio.h"
#include "dma.h"
/**	@page SPI
 *
 *	Introduction
 *	===============
 *	B91 supports two spi 
 *
 *	API Reference
 *	===============
 *	Header File: spi.h
 */

typedef enum{
	SPI_RXFIFO_OR_INT_EN        =BIT(0),
	SPI_TXFIFO_UR_INT_EN        =BIT(1),
	SPI_RXFIFO_INT_EN 	        =BIT(2),
	SPI_TXFIFO_INT_EN		    =BIT(3),
	SPI_END_INT_EN              =BIT(4),
	SPI_SLV_CMD_EN	            =BIT(5),
}spi_irq_mask;

typedef enum{
	SPI_RXF_OR_INT	    =BIT(2),
	SPI_TXF_UR_INT	    =BIT(3),
	SPI_RXF_INT         =BIT(4),
	SPI_TXF_INT 	    =BIT(5),
	SPI_END_INT         =BIT(6),
    SPI_SLV_CMD_INT	    =BIT(7),
}spi_irq_status_e;

typedef enum{
	PSPI_MODULE = 0,
    HSPI_MODULE = 1 ,
}spi_sel_e;

/**
 * @brief  Define the work mode.
 */
typedef enum{
    SPI_MODE0 = 0,
    SPI_MODE2,
    SPI_MODE1,
    SPI_MODE3,
} spi_mode_type_e;

/**
 * @brief  Define the mode for SPI io mode.
 */
typedef enum{
	SPI_SINGLE_MODE	 = 0,
    SPI_DUAL_MODE	 = 1 ,
    HSPI_QUAD_MODE 	 = 2,
	SPI_3_LINE_MODE	 = 3
} spi_io_mode_e;

typedef enum{
	SPI_NOMAL = 0,
    SPI_3LINE = 3,
} spi_nomal_3line_mode_e;

typedef enum{
	PSPI_SINGLE = 0,
    PSPI_DUAL 	= 1 ,
} pspi_single_dual_mode_e;

typedef enum{
	HSPI_SINGLE = 0,
    HSPI_DUAL	= 1 ,
    HSPI_QUAD 	= 2,
} hspi_single_dual_quad_mode_e;


/**
 * @brief  Define the SPI command & translate mode.
 */
typedef enum{
	SPI_MODE_WRITE_AND_READ = 0,//write and read at the same.must enbale CmdEn
	SPI_MODE_WRITE_ONLY,//write
	SPI_MODE_READ_ONLY,// read must enbale CmdEn
	SPI_MODE_WRITE_READ,//write_ read
	SPI_MODE_READ_WRITE,//read_write
	SPI_MODE_WRITE_DUMMY_READ,//write_dummy_read
	SPI_MODE_READ_DUMMY_WRITE,//read_ dummy_write must enbale CmdEn
	SPI_MODE_NONE_DATA,//must enbale CmdEn
	SPI_MODE_DUMMY_WRITE,//dummy_write
	SPI_MODE_DUMMY_READ,//dummy_read
	SPI_MODE_RESERVED,
}spi_tans_mode_e;

typedef enum{
	SPI_MODE_WR_WRITE_ONLY  = 1,//write
	SPI_MODE_WR_DUMMY_WRITE = 8,//dummy_write
}spi_wr_tans_mode_e;

typedef enum{
	SPI_MODE_RD_READ_ONLY  = 2,//must enbale CmdEn
	SPI_MODE_RD_DUMMY_READ = 9,//dummy_read
}spi_rd_tans_mode_e;

typedef enum{
	SPI_MODE_WR_RD 		 = 3,//must enbale CmdEn
	SPI_MODE_WR_DUMMY_RD = 5,//write_dummy_read
}spi_wr_rd_tans_mode_e;

typedef struct{
	hspi_single_dual_quad_mode_e  hspi_io_mode;//set spi interface mode
	u8 hspi_dummy_cnt;//set dummy cnt if tans_mode have dummy .
	u8 hspi_cmd_en;//enable cmd phase
	u8 hspi_addr_en;//enable address phase
	u8 hspi_addr_len;//enable address phase
	u8 hspi_cmd_fmt_en;//if cmd_en enable cmd fmt will follow the interface (dual/quad)
	u8 hspi_addr_fmt_en;//if addr_en enable addr fmt will follow the interface (dual/quad)
}hspi_config_t;

typedef struct{
	pspi_single_dual_mode_e  pspi_io_mode;//set spi interface mode
	u8 pspi_dummy_cnt;//set dummy cnt if tans_mode have dummy .
    _Bool  pspi_cmd_en;//enable cmd phase
}pspi_config_t;


typedef enum{
	SPI_SLAVE_WRITE_DATA_CMD = 0x00,
	SPI_SLAVE_WRITE_DATA_DUAL_CMD = FLD_SPI_CMD_DATA_DUAL,
	SPI_SLAVE_WRITE_ADDR_DUAL_CMD = FLD_SPI_CMD_ADDR_DUAL,

	SPI_SLAVE_WRITE_DATA_DUAL_4CYC_CMD = FLD_SPI_CMD_DATA_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_SLAVE_WRITE_ADDR_DUAL_4CYC_CMD = FLD_SPI_CMD_ADDR_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_SLAVE_WRITE_DATA_AND_ADDR_DUL_4CYC_CMD = FLD_SPI_CMD_ADDR_DUAL | FLD_SPI_CMD_DATA_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,
}spi_slave_write_cmd_e;

typedef enum{
	SPI_SLAVE_READ_DATA_CMD = FLD_SPI_CMD_RD_EN,
	SPI_SLAVE_READ_DATA_DUAL_CMD = FLD_SPI_CMD_RD_EN | FLD_SPI_CMD_DATA_DUAL,
	SPI_SLAVE_READ_ADDR_DUAL_CMD = FLD_SPI_CMD_RD_EN | FLD_SPI_CMD_ADDR_DUAL,

	SPI_SLAVE_READ_DATA_DUAL_4CYC_CMD = FLD_SPI_CMD_RD_EN | FLD_SPI_CMD_DATA_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,
	SPI_SLAVE_READ_ADDR_DUAL_4CYC_CMD = FLD_SPI_CMD_RD_EN | FLD_SPI_CMD_ADDR_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,

	SPI_SLAVE_READ_DATA_AND_ADDR_DUL_4CYC_CMD = FLD_SPI_CMD_RD_EN | FLD_SPI_CMD_ADDR_DUAL | FLD_SPI_CMD_DATA_DUAL | FLD_SPI_CMD_RD_DUMMY_4CYCLE,

}spi_slave_read_cmd_e;

typedef enum{
	SPI_READ_STATUS_SINGLE_CMD  = 0x05,
	SPI_READ_STATUS_DUAL_CMD 	= 0x15,
	HSPI_READ_STATUS_QUAD_CMD   = 0x25,
	SPI_READ_DATA_SINGLE_CMD    = 0x0B,
	SPI_READ_DATA_DUAL_CMD 		= 0x0C,
	HSPI_READ_DATA_QUAD_CMD 	= 0x0E,
	SPI_WRITE_DATA_SINGLE_CMD   = 0x51,
	SPI_WRITE_DATA_DUAL_CMD 	= 0x52,
	HSPI_WRITE_DATA_QUAD_CMD 	= 0x54,
}pspi_hspi_cmd_e;
/**
 * @brief Define APS1604M-3SQR QSPI PSRAM CMD.
 */
typedef enum{
	PSRAM_READ_CMD				  = 0x03,
	PSRAM_FAST_READ_CMD			  = 0x0B,
	PSRAM_FAST_READ_QUAD_CMD	  = 0xEB,

	PSRAM_WRITE_CMD				  = 0x02,
	PSRAM_QUAD_WRITE_CMD		  = 0x38,//
	PSRAM_WRAPPED_READ_CMD		  = 0x8B,
	PSRAM_WRAPPED_WRITE_CMD		  = 0x82,

	PSRAM_MODE_RG_READ_CMD		  = 0xB5,
	PSRAM_MODE_RG_WRITE_CMD		  = 0xB1,

	PSRAM_ENTER_QUAD_MODE_CMD	  = 0x35,
	PSRAM_EXIT_QUAD_MODE_CMD	  = 0xF5,

	PSRAM_REST_ENABLE_CMD		  = 0x66,
	PSRAM_REST_CMD	    		  = 0x99,
	PSRAM_BURST_LENGTH_TOGGLE_CMD = 0xC0,
	PSRAM_READ_ID_CMD  		  	  = 0x95,

}spi_xip_cmd_e;

/**
 * @brief Define panel 2data_lane_mode
 */
typedef enum{
	HSPI_2DATA_LANE_CLOSE	= 0x00,
	HSPI_2DATA_LANE_RGB565  = 0x01,
	HSPI_2DATA_LANE_RGB666  = 0x03,
	HSPI_2DATA_LANE_RGB888  = 0x07,
}hspi_panel_2data_lane_mode_e;

/**
 * @brief  Define the SPI io.
 */
typedef enum{
	HSPI_CLK_PB4 = GPIO_PB4,
	HSPI_CLK_PA2 = GPIO_PA2,
}hspi_clk_pin_def_e;

typedef enum{
	HSPI_CSN_PB6 = GPIO_PB6,
	HSPI_CSN_PA1 = GPIO_PA1,
}hspi_csn_pin_def_e;

typedef enum{
	HSPI_MOSI_IO0_PB3 = GPIO_PB3,
	HSPI_MOSI_IO0_PA4 = GPIO_PA4,
}hspi_mosi_io0_pin_def_e;

typedef enum{
	HSPI_MISO_IO1_PB2 = GPIO_PB2,
	HSPI_MISO_IO1_PA3 = GPIO_PA3,
}hspi_miso_io1_pin_def_e;

typedef enum{
	HSPI_WP_IO2_PB1 = GPIO_PB1,
}hspi_wp_io2_pin_def_e;

typedef enum{
	HSPI_HOLD_IO3_PB0 = GPIO_PB0,
}hspi_hold_io3_pin_def_e;

typedef enum{
	HSPI_CLK_PB4_PIN = GPIO_PB4,
	HSPI_CLK_PA2_PIN = GPIO_PA2,

	HSPI_CSN_PB6_PIN = GPIO_PB6,
	HSPI_CSN_PA1_PIN = GPIO_PA1,

	HSPI_MOSI_IO0_PB3_PIN = GPIO_PB3,
	HSPI_MOSI_IO0_PA4_PIN = GPIO_PA4,

	HSPI_MISO_IO1_PB2_PIN = GPIO_PB2,
	HSPI_MISO_IO1_PA3_PIN = GPIO_PA3,

	HSPI_WP_IO2_PB1_PIN   = GPIO_PB1,
	HSPI_HOLD_IO3_PB0_PIN = GPIO_PB0,
	HSPI_NONE_PIN = 0xfff,
}hspi_pin_def_e;

typedef struct{
	hspi_clk_pin_def_e      	      hspi_clk_pin;
	hspi_csn_pin_def_e         		  hspi_csn_pin;
	hspi_mosi_io0_pin_def_e           hspi_mosi_io0_pin;
	hspi_miso_io1_pin_def_e           hspi_miso_io1_pin;
	hspi_wp_io2_pin_def_e             hspi_wp_io2_pin;
	hspi_hold_io3_pin_def_e           hspi_hold_io3_pin;
}hspi_pin_config_t;


typedef enum{
	PSPI_CLK_PC5 = GPIO_PC5,
	PSPI_CLK_PB5 = GPIO_PB5,
	PSPI_CLK_PD1 = GPIO_PD1,
}pspi_clk_pin_def_e;

typedef enum{
	PSPI_CSN_PC4 = GPIO_PC4,
	PSPI_CSN_PC0 = GPIO_PC0,
	PSPI_CSN_PD0 = GPIO_PD0,
}pspi_csn_pin_def_e;

typedef enum{
	PSPI_MOSI_IO0_PC7 = GPIO_PC7,
	PSPI_MOSI_IO0_PB7 = GPIO_PB7,
	PSPI_MOSI_IO0_PD3 = GPIO_PD3,
}pspi_mosi_io0_pin_def_e;

typedef enum{
	PSPI_MISO_IO1_PC6 = GPIO_PC6,
	PSPI_MISO_IO1_PB6 = GPIO_PB6,
	PSPI_MISO_IO1_PD2 = GPIO_PD2,
}pspi_miso_io1_pin_def_e;

typedef enum{
	PSPI_CLK_PC5_PIN = GPIO_PC5,
	PSPI_CLK_PB5_PIN = GPIO_PB5,
	PSPI_CLK_PD1_PIN = GPIO_PD1,

	PSPI_CSN_PC4_PIN = GPIO_PC4,
	PSPI_CSN_PC0_PIN = GPIO_PC0,
	PSPI_CSN_PD0_PIN = GPIO_PD0,

	PSPI_MOSI_IO0_PC7_PIN = GPIO_PC7,
	PSPI_MOSI_IO0_PB7_PIN = GPIO_PB7,
	PSPI_MOSI_IO0_PD3_PIN = GPIO_PD3,

	PSPI_MISO_IO1_PC6_PIN = GPIO_PC6,
	PSPI_MISO_IO1_PB6_PIN = GPIO_PB6,
	PSPI_MISO_IO1_PD2_PIN = GPIO_PD2,
	PSPI_NONE_PIN = 0xfff,
}pspi_pin_def_e;

typedef struct{
	pspi_clk_pin_def_e      pspi_clk_pin;
	pspi_csn_pin_def_e      pspi_csn_pin;
	pspi_mosi_io0_pin_def_e pspi_mosi_io0_pin;
	pspi_miso_io1_pin_def_e pspi_miso_io1_pin;
}pspi_pin_config_t;


/**
 * @brief    This function reset HSPI module.
 * @return   none
 */
static inline void hspi_reset(void)
{
	reg_rst0 &= (~FLD_RST0_HSPI);
	reg_rst0 |= FLD_RST0_HSPI;
}
/**
 * @brief  This function reset PSPI module.
 * @return none
 */
static inline void pspi_reset(void)
{
	reg_rst1 &= (~FLD_RST1_PSPI);
	reg_rst1 |= FLD_RST1_PSPI;
}

/**
 * @brief 		This function get tx_fifo number.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		ntx_fifo 	- number that wait to be sent.
 */
static inline  unsigned char spi_get_txfifo_num(spi_sel_e spi_sel)
{
   return (reg_spi_fifo_num(spi_sel) & FLD_SPI_TXF_NUM) >> 4;
}

/**
 * @brief 		This function get rx_fifo number.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		rx_fifo 	- num that have been received.
 */
static inline unsigned char spi_get_rxfifo_num(spi_sel_e spi_sel)
{
   return reg_spi_fifo_num(spi_sel) & FLD_SPI_RXF_NUM;
}

/**
 * @brief 		This function set SPI rx_cnt.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	cnt 		- rx amount of data.
 * @return 		none
 */
static inline void spi_rx_cnt(spi_sel_e spi_sel, u32 cnt)
{
	reg_spi_rx_cnt2(spi_sel) = ((cnt - 1) >> 16) & 0xff;
	reg_spi_rx_cnt1(spi_sel) = ((cnt - 1) >> 8) & 0xff;
	reg_spi_rx_cnt0(spi_sel) = (cnt - 1) & 0xff;

}

/**
 * @brief 		This function set SPI tx_cnt.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	cnt 		- tx amount of data.
 * @return 		none
 */
static inline void spi_tx_cnt(spi_sel_e spi_sel, u32 cnt)
{
	reg_spi_tx_cnt2(spi_sel) = ((cnt - 1) >> 16) & 0xff;
	reg_spi_tx_cnt1(spi_sel) = ((cnt - 1) >> 8) & 0xff;
    reg_spi_tx_cnt0(spi_sel) = (cnt - 1) & 0xff;

}

/**
 * @brief 		This function clear tx_fifo.
 * @param[in] 	spi_sel - the spi module.
 * @return 		none
 */
static inline void spi_tx_fifo_clr(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_fifo_state(spi_sel), FLD_SPI_TXF_CLR);
}

/**
 * @brief 		This function clear rx_fifo.
 * @param[in]	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_rx_fifo_clr(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_fifo_state(spi_sel), FLD_SPI_RXF_CLR);
}

/**
 * @brief 		This function set hspi command content.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	cmd 		- command content.
 * @return 		none
 */
static inline void spi_set_cmd(spi_sel_e spi_sel, u8 cmd)
{
	reg_spi_trans1(spi_sel) = cmd;
}

/**
 * @brief 		This function servers to enable cmd which will sent a byte cmd.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_cmd_en(spi_sel_e spi_sel)
{
	BM_SET(	reg_spi_mode2(spi_sel), FLD_SPI_CMD_EN);
}

/**
 * @brief 		This function servers to disable cmd which will not sent cmd.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_cmd_dis(spi_sel_e spi_sel)
{

	BM_CLR(reg_spi_mode2(spi_sel), FLD_SPI_CMD_EN);
}

/**
 * @brief 	This function servers enable cmd  format,the format of cmd phase is the same as the data phase(Dual/Quad).
 * @return 	none
 */
static inline void hspi_cmd_fmt_en()
{
	BM_SET(	reg_spi_mode2(HSPI_MODULE), FLD_HSPI_CMD_FMT);
}

/**
 * @brief 	This function servers disable cmd  format.
 * @return 	none
 */
static inline void hspi_cmd_fmt_dis()
{
	BM_CLR(reg_spi_mode2(HSPI_MODULE), FLD_HSPI_CMD_FMT);
}

/**
 * @brief 	This function servers to enable hspi quad mode.
 * @return 	none
 */
static inline void hspi_quad_mode_en()
{
	BM_SET(reg_spi_mode2(HSPI_MODULE), FLD_HSPI_QUAD);
}

/**
 * @brief 		This function servers to disable hspi quad mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void hspi_quad_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode2(spi_sel), FLD_HSPI_QUAD);
}

/**
 * @brief 		This function servers to enable hspi dual mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_dual_mode_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_mode0(spi_sel), FLD_SPI_DUAL);
}

/**
 * @brief 		This function servers to disable hspi dual mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_dual_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode0(spi_sel), FLD_SPI_DUAL);
}

/**
 * @brief 		This function servers to enable hspi 3line mode.
 * @param[in] 	spi_sel - the spi module.
 * @return 		none
 */
static inline void spi_3line_mode_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_mode0(spi_sel), FLD_SPI_3LINE);
}

/**
 * @brief 		This function servers to disable hspi 3line mode..
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_3line_mode_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_mode0(spi_sel), FLD_SPI_3LINE);
}

/**
 * @brief 	This function to enable address format.the format of addr phase is the same as the data phase(Dual/Quad).
 * @return 	none
 */
static inline void hspi_addr_fmt_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_ADDR_FMT);

}

/**
 * @brief 	This function to disable address format.
 * @return 	none
 */
static inline void hspi_addr_fmt_dis(void)
{
	BM_CLR(	reg_hspi_xip_ctrl, FLD_HSPI_ADDR_FMT);

}

/**
 * @brief 		This function to determine whether the bus is busy.
 * @param[in] 	spi_sel 	- the spi module.
 * @return   	1:Indicates that the bus is busy. 0:Indicates that the bus is free.
 */
static inline _Bool spi_is_busy(spi_sel_e spi_sel)
{
	return  reg_spi_status(spi_sel) & FLD_HSPI_BUSY;

}

/**
 * @brief 		This function enable tx dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_tx_dma_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_trans2(spi_sel), FLD_SPI_TX_DMA_EN);
}

/**
 * @brief 		This function disable tx dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_tx_dma_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_trans2(spi_sel), FLD_SPI_TX_DMA_EN);
}

/**
 * @brief 		This function enable rx dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_rx_dma_en(spi_sel_e spi_sel)
{
	BM_SET(reg_spi_trans2(spi_sel), FLD_SPI_RX_DMA_EN);
}

/**
 * @brief 		This function disable rx dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_rx_dma_dis(spi_sel_e spi_sel)
{
	BM_CLR(reg_spi_trans2(spi_sel), FLD_SPI_RX_DMA_EN);
}

/**
 * @brief 		This function set xip write command content.
 * @param[in] 	wr_cmd 		- write command content.
 * @return 		none
 */
static inline void hspi_xip_set_wr_cmd(u8 wr_cmd)
{
	reg_hspi_xip_wr_cmd	= wr_cmd;
}

/**
 * @brief 		This function set xip read command content.
 * @param[in] 	rd_cmd		- read command content.
 * @return 		none
 */
static inline void hspi_xip_set_rd_cmd(u8 rd_cmd)
{

	reg_hspi_xip_rd_cmd	= rd_cmd;
}

/**
 * @brief 		This function set xip address offset.
 * @param[in] 	addr_offset 	- the offset of data.
 * @return 		none
 */
static inline void hspi_xip_addr_offset(u32 addr_offset)
{
    reg_hspi_xip_addr_offset0 = addr_offset & 0xff;
	reg_hspi_xip_addr_offset1 = (addr_offset >> 8) & 0xff;
	reg_hspi_xip_addr_offset2 = (addr_offset >> 16) & 0xff;
	reg_hspi_xip_addr_offset3 = (addr_offset >> 24) & 0xff;
}

/**
 * @brief 		This function servers to set xip read transfer mode.
 * @param[in] 	rd_mode 	- read mode in enum hspi_tans_mode_e.
 * @return  	none
 */
static inline void hspi_xip_read_transmode(u8 rd_mode)
{
	reg_hspi_xip_trans_mode	&= (~FLD_HSPI_XIP_RD_TRANS_MODE);
	reg_hspi_xip_trans_mode |= (rd_mode & 0xf) << 4;
}

/**
 * @brief 		This function servers to set xip write transfer mode.
 * @param[in] 	wr_mode 	- write mode in enum hspi_tans_mode_e.
 * @return  	none
 */
static inline void hspi_xip_write_transmode(u8 wr_mode)
{
	reg_hspi_xip_trans_mode	&= (~FLD_HSPI_XIP_WR_TRANS_MODE);
	reg_hspi_xip_trans_mode |= wr_mode & FLD_HSPI_XIP_WR_TRANS_MODE;
}

/**
 * @brief 	This function enable address phase.
 * @return 	none
 */
static inline void hspi_addr_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_ADDR_EN);
}

/**
 * @brief 	This function disable address phase.
 * @return 	none
 */
static inline void hspi_addr_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl, FLD_HSPI_ADDR_EN);
}

/**
 * @brief 		This function servers to set hspi address length.
 * @param[in] 	len 	- 2'b00:1bye  2'b01:2bytes  2'b10:3bytes  2'b11:4bytes
 * @return 		none
 */
static inline void hspi_set_addr_len(u8 len)
{
	reg_hspi_xip_ctrl |= ((len - 1) & 0x3) << 2;
}

/**
 * @brief 	This function servers to enable xip sequential mode.
 * @return 	none
 */
static inline void hspi_xip_seq_mode_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_XIP_MODE);
}

/**
 * @brief 	This function servers to disable xip sequential mode.
 * @return 	none
 */
static inline void hspi_xip_seq_mode_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl, FLD_HSPI_XIP_MODE);
}

/**
 * @brief 	This function servers to enable xip.
 * @return 	none
 */
static inline void hspi_xip_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_XIP_ENABLE);
}

/**
 * @brief 	This function servers to disable xip.
 * @return 	none
 */
static inline void hspi_xip_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl, FLD_HSPI_XIP_ENABLE);
}

/**
 * @brief 	This function servers to stop xip.
 * @return 	none
 */
static inline void hspi_xip_stop(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_XIP_STOP);
}

/**
 * @brief 	This function servers to enable xip timeout that set hight level.
 * @return 	none
 */
static inline void hspi_xip_timeout_mode_en(void)
{
	BM_SET(reg_hspi_xip_ctrl, FLD_HSPI_XIP_TIMEOUT_MODE);
}

/**
 * @brief 	This function servers to disable xip timeout.
 * @return 	none
 */
static inline void hspi_xip_timeout_mode_dis(void)
{
	BM_CLR(reg_hspi_xip_ctrl, FLD_HSPI_XIP_TIMEOUT_MODE);
}

/**
 * @brief 		This function servers to set xip timeout cnt,when two data frame intervals exceed spi_clock_out_period*timeout_cnt,cs will set high level.
 * @param[in] 	cnt 	- xip timeout cnt.
 * @return 		none
 */
static inline void hspi_xip_timeout_cnt(u8 cnt)
{
	reg_hspi_xip_timeout_cnt = cnt & 0xff;
}

/**
 * @brief 		This function servers to set page_size.
 * @param[in] 	page_size_i 	- page boundary 2^page_size_i bytes.
 * @return 		none
 */
static inline void hspi_xip_page_size(u8 page_size_i)
{
	reg_hspi_page_size = page_size_i;
}

/**
 * @brief 		This function servers to reply master slave is ready .When slave is ready,  slave ready reply a byte data:0x5a.   indicating that slave is ready for data transmission.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_slave_ready_en(spi_sel_e spi_sel)
{
	  BM_SET(reg_spi_status(spi_sel), FLD_HSPI_SLAVE_READY);
}

/**
 * @brief 		This function servers to reply master slave is not ready.slave reply a byte data: 0x00.indicating that slave is not ready for data transmission.
 * @param[in] 	spi_sel 	- the spi module.
 * @return 		none
 */
static inline void spi_slave_ready_dis(spi_sel_e spi_sel)
{
	  BM_CLR(reg_spi_status(spi_sel), FLD_HSPI_SLAVE_READY);
}

/**
 * @brief 		This function servers to read cmd from master for slave.
 * @param[in] 	spi_sel 	- the spi module.
 * @return  	cmd transferd by master.
 */
static inline u8 spi_slave_get_cmd(spi_sel_e spi_sel)
{
	return reg_spi_trans1(spi_sel);
}

/**
 * @brief      	The function of this API is to set the number of bytes to triggered the receive and transmit interrupt.
 *            	Its default value is 4. recommend setting 4.
 * @param[in] 	spi_sel - the spi module.
 * @param[in]  	cnt 	- the interrupt trigger level.
 * @return   	none
 */
static inline void spi_rx_tx_irq_trig_cnt(spi_sel_e spi_sel, u8 cnt)
{
	BM_CLR(reg_spi_status(spi_sel), FLD_HSPI_FIFO_THRES);
	reg_spi_status(spi_sel) |= ((cnt & 7) << 4);
}

/**
 * @brief 		This function servers to get irq status.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	status 	- the irq status.
 * @return    - the value of status is be set.
 */
static inline u8 spi_get_irq_status(spi_sel_e spi_sel,spi_irq_status_e status )
{
	return reg_spi_irq_state(spi_sel)&status;
}

/**
 * @brief 		This function servers to clear irq status.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	status 	- the irq status.
 * @return  	none.
 */
static inline void spi_clr_irq_status(spi_sel_e spi_sel, spi_irq_status_e status)
{
	reg_spi_irq_state(spi_sel) = status;
}

/**
 * @brief 		This function servers to set irq mask.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	mask 	- the irq mask.
 * @return  	cmd 	- transferd by master.
 */
static inline void spi_set_irq_mask(spi_sel_e spi_sel, spi_irq_mask mask)
{
   BM_SET(reg_spi_trans2(spi_sel), mask);
}

/**
 * @brief 		This function servers to clear irq mask.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	mask 	- the irq mask.
 * @return  	cmd 	- transferd by master.
 */
static inline void spi_clr_irq_mask(spi_sel_e spi_sel, spi_irq_mask mask)
{
	BM_CLR(reg_spi_trans2(spi_sel), mask);
}

/**
 * @brief 		This function enable 3line_dcx module which is use for panel(lcd oled..).
 * @return 		none
 */
static inline void hspi_3line_dcx_en(void)
{
	BM_SET(reg_hspi_panel_ctrl, FLD_HSPI_PANEL_3LINE_DCX_EN);
}

/**
 * @brief 		This function disable 3line_dcx module which is use for panel(lcd oled..).
 * @return 		none
 */
static inline void hspi_3line_dcx_dis(void)
{
	BM_CLR(reg_hspi_panel_ctrl, FLD_HSPI_PANEL_3LINE_DCX_EN);
}

/**
 * @brief 		This function set 3line_dcx translate data.
 * @return 		none
 */
static inline void hspi_set_3line_dcx_data(void)
{
	BM_SET(reg_hspi_panel_ctrl, FLD_HSPI_PANEL_3LINE_DCX);
}

/**
 * @brief 		This function set 3line_dcx translate command.
 * @return 		none
 */
static inline void hspi_set_3line_dcx_cmd(void)
{
	BM_CLR(reg_hspi_panel_ctrl, FLD_HSPI_PANEL_3LINE_DCX);
}

/**
 * @brief 		This function set 2data_lane mode for panel.
 * @return 		none
 */
static inline void hspi_set_panel_2data_lane_mode(hspi_panel_2data_lane_mode_e mode)
{
	reg_hspi_panel_ctrl &= (~FLD_HSPI_PANEL_2DATA_LANE);
	reg_hspi_panel_ctrl |= (mode & 0xf) << 2;
}

/**
 * @brief   This function selects  pin  for hspi master or slave.
 * @return  none
 */
void spi_slave_set_pin(void);
/**
 * @brief     	This function configures hspi pin.
 * @param[in] 	config - the pointer of pin config struct.
 * @return  	none
 */
void hspi_set_pin(hspi_pin_config_t *config);

/**
 * @brief     	This function configures pspi pin.
 * @param[in] 	config - the pointer of pin config struct.
 * @return 		none
 */
void pspi_set_pin(pspi_pin_config_t *config);

/**
 * @brief     	This function configures the clock and working mode for SPI interface.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	div_clock 	- the division factor for SPI module.
 *           	spi_clock_out = ahb_clock / ((div_clock+1)*2)
 * @param[in]	 mode 		- the selected working mode of SPI module.
 *           	bit5:CPHA-Clock Polarity ; bit6:CPOL:CPHA-Clock Phase
 *           	MODE0:  CPHA = 0, CPOL = 0;
 *           	MODE1:  CPHA = 0, CPOL = 1;
 *           	MODE2:  CPHA = 1, CPOL = 0;
 *           	MODE3:  CPHA = 1, CPOL = 1;
 * @return  	none
 */
void spi_master_init(spi_sel_e spi_sel, u8 div_clock, spi_mode_type_e mode);

/**
 * @brief     	This function configures the clock and working mode for SPI interface.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	mode 	- the selected working mode of SPI module.
 *          	bit5:CPHA-Clock Polarity ; bit6:CPOL:CPHA-Clock Phase
 *           	MODE0:  CPHA = 0, CPOL = 0;
 *           	MODE1:  CPHA = 0, CPOL = 1;
 *          	MODE2:  CPHA = 1, CPOL = 0;
 *           	MODE3:  CPHA = 1,  CPOL = 1;
 * @return  	none
 * @note  		spi_clock_in  (spi_slave_clock frequency)/3
 */
void spi_slave_init(spi_sel_e spi_sel, spi_mode_type_e mode);

/**
 * @brief     	This function servers to set dummy cycle cnt.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	dummy_cnt 	- the cnt of dummy clock.
 * @return  	none
 */
void spi_set_dummy_cnt(spi_sel_e spi_sel, u8 dummy_cnt);

/**
 * @brief     	This function servers to set slave address hspi only.
 * @param[in] 	addr 	- address of slave.
 * @return  	none
 */
void hspi_set_address(u32 addr);

/**
 * @brief     	This function servers to set spi transfer mode.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	mode 	- transfer mode.
 * @return  	none
 */
void spi_set_transmode(spi_sel_e spi_sel, spi_tans_mode_e mode);


/**
 * @brief     	This function servers to set normal mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return  	none
 */
void spi_set_normal_mode(spi_sel_e spi_sel);

/**
 * @brief     	This function servers to set dual mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return  	none
 */
void spi_set_dual_mode(spi_sel_e spi_sel);

/**
 * @brief	This function servers to set quad mode.
 * @return	none
 */
void hspi_set_quad_mode();

/**
 * @brief     	This function servers to set 3line mode.
 * @param[in] 	spi_sel 	- the spi module.
 * @return  	none
 */
void spi_set_3line_mode(spi_sel_e spi_sel);

/**
 * @brief     	This function servers to set hspi io  mode.
 * @param[in] 	spi_sel - the spi module.
 * @param[in]	mode 	- single/dual/quad /3line.
 * @return  	none
  */
void spi_set_io_mode(spi_sel_e spi_sel, spi_io_mode_e mode);

/**
 * @brief     	This function servers to config normal mode.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	mode 	- nomal ,mode 3line.
 * @return  	none
 */
void spi_master_config(spi_sel_e spi_sel, spi_nomal_3line_mode_e mode);

/**
 * @brief     	This function servers to config hspi special mode.
 * @param[in] 	config 	- the pointer of pin special config struct.
 * @return  	none
 */
void hspi_master_config_plus(hspi_config_t *config);

/**
 * @brief     	This function servers to config pspi special mode.
 * @param[in] 	config 	- the pointer of pin special config struct.
 * @return  	none
 */
void pspi_master_config_plus(pspi_config_t *config);

/**
 * @brief     	This function servers to write hspi fifo.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	data 	- the pointer to the data for write.
 * @param[in] 	len 	- write length.
 * @return    	none
 */
void spi_write(spi_sel_e spi_sel, u8 *data, u32 len);

/**
 * @brief     	This function servers to read hspi fifo.
 * @param[in] 	spi_sel	- the spi module.
 * @param[in] 	data 	- the pointer to the data for read.
 * @param[in] 	len 	- write length.
 * @return    	none
 */
void spi_read(spi_sel_e spi_sel, u8 *data, u32 len);

/**
 * @brief     	This function serves to normal write data in normal.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	data 	- the pointer to the data for write.
 * @param[in] 	len 	- write length.
 * @return  	none
 */
void spi_master_write(spi_sel_e spi_sel, u8 *data, u32 len);

/**
 * @brief     	This function serves to normal write and read data.
 * @param[in] 	spi_sel - the spi module.
 * @param[in] 	wr_data - the pointer to the data for write.
 * @param[in] 	wr_len 	- write length.
 * @param[in] 	rd_data - the pointer to the data for read.
 * @param[in] 	rd_len 	- read length.
 * @return  	none
 */
void spi_master_write_read(spi_sel_e spi_sel, u8 *wr_data, u32 wr_len, u8 *rd_data, u32 rd_len);

/**
 * @brief     	This function serves to single/dual/quad write to the SPI slave.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	cmd 		- cmd one byte will first write.
 * @param[in] 	addr 		- the address of slave.
 * @param[in] 	data 		-  pointer to the data need to write.
 * @param[in] 	data_len 	- length in byte of the data need to write.
 * @param[in] 	wr_mode 	- write mode.dummy or not dummy.
 * @return  	none
 */
void spi_master_write_plus(spi_sel_e spi_sel, u8 cmd, u32 addr, u8 *data, u32 data_len, spi_wr_tans_mode_e wr_mode);

/**
 * @brief     	This function serves to single/dual/quad  read from the SPI slave.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr 		- the address of slave.
 * @param[in]  	data 		- pointer to the data need to read.
 * @param[in]  	data_len 	- the length of data.
 * @param[in]  	rd_mode 	- read mode.dummy or not dummy.
 * @return   	none
 */
void spi_master_read_plus(spi_sel_e spi_sel, u8 cmd, u32 addr, u8 *data, u32 data_len, spi_rd_tans_mode_e rd_mode);

/**
 * @brief      	This function serves to write address, then read data from the SPI slave.
 * @param[in]  	spi_sel	 	- the spi module.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addrs 		- pointer to the address of slave.
 * @param[in]  	addr_len 	- the length of address.
 * @param[in]  	data 		- the pointer to the data for read.
 * @param[in]  	data_len 	- read length.
 * @param[in] 	wr_mode 	- write mode.dummy or not dummy.
 * @return   	none
 */
void spi_master_write_read_plus(spi_sel_e spi_sel, u8 cmd, u8 *addrs, u32 addr_len, u8 *data, u32 data_len, spi_rd_tans_mode_e wr_mode);

/**
 * @brief     	This function serves to set tx_dam channel and config dma tx default.
 * @param[in] 	chn 	- dma channel.
 * @return  	none
 */
void hspi_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     	This function serves to set rx_dam channel and config dma rx default.
 * @param[in] 	chn 	- dma channel.
 * @return  	none
 */
void hspi_set_rx_dma_config(dma_chn_e chn);

/**
 * @brief     	This function serves to set tx_dam channel and config dma tx default.
 * @param[in] 	chn 	- dma channel.
 * @return  	none
 */
void pspi_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     	This function serves to set rx_dam channel and config dma rx default.
 * @param[in] 	chn 	- dma channel.
 * @return  	none
 */
void pspi_set_rx_dma_config(dma_chn_e chn);

/**
 * @brief   	this  function set spi dma channel.
 * @param[in]  	spi_dma_chn - dma channel.
 * @param[in]  	src_addr 	- the address of source.
 * @param[in]  	dst_addr 	- the address of destination.
 * @param[in]  	len 		- the length of data.
 * */
void spi_set_dma(dma_chn_e spi_dma_chn, u32 src_addr, u32 dst_addr, u32 len);




/**
 * @brief   	this  function set spi tx dma channel.
 * @param[in]  	spi_sel     - the spi module.
 * @param[in]  	src_addr 	- the address of source.
 * @param[in]  	len 		- the length of data.
 * */
void spi_set_tx_dma(spi_sel_e spi_sel, u8* src_addr,u32 len);


/**
 * @brief   	this  function set spi rx dma channel.
 * @param[in]  	spi_sel     - the spi module.
 * @param[in]  	dst_addr 	- the address of destination.
 * @param[in]  	len 		- the length of data.
 * */
void spi_set_rx_dma(spi_sel_e spi_sel, u8* dst_addr,u32 len);
/**
 * @brief     	This function serves to normal write data by dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	src_addr 	- the pointer to the data for write.
 * @param[in] 	len 		- write length.
 * @return  	none
 */
void spi_master_write_dma(spi_sel_e spi_sel, u8 *src_addr, u32 len);

/**
 * @brief     	This function serves to normal write cmd and address, then read data by dma.
 * @param[in] 	spi_sel 	- the spi module.
 * @param[in] 	addr 		- the pointer to the cmd and address for write.
 * @param[in] 	addr_len 	- write length.
 * @param[in] 	data 		- the pointer to the data for read.
 * @param[in] 	data_len 	- read length.
 * @return  	none
 */
void spi_master_write_read_dma(spi_sel_e spi_sel, u8 *addr, u32 addr_len, u8 *data, u32 data_len);

/**
 * @brief      	This function serves to single/dual/quad  write to the SPI slave by dma.
 * @param[in]  	spi_sel 	- the spi module.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr 		- the address of slave.
 * @param[in]  	data 		- pointer to the data need to write.
 * @param[in]  	data_len 	- length in byte of the data need to write.
 * @param[in]  	wr_mode 	- write mode.dummy or not dummy.
 * @return   	none
 */
void spi_master_write_dma_plus(spi_sel_e spi_sel, u8 cmd, u32 addr, u8 *data, u32 data_len, spi_wr_tans_mode_e wr_mode);

/**
 * @brief      	This function serves to single/dual/quad  read from the SPI slave by dma.
 * @param[in]  	spi_sel 	- the spi module.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr 		- the address of slave.
 * @param[in]  	dst_addr 	- pointer to the buffer that will cache the reading out data.
 * @param[in]  	data_len 	- length in byte of the data need to read.
 * @param[in]  	rd_mode 	- read mode.dummy or not dummy.
 * @return   	none
 */
void spi_master_read_dma_plus(spi_sel_e spi_sel, u8 cmd, u32 addr, u8 *dst_addr, u32 data_len, spi_rd_tans_mode_e rd_mode);

/**
 * @brief      	This function serves to single/dual/quad write address and read from the SPI slave by dma.
 * @param[in]  	spi_sel 	- the spi module.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr 		- the address of slave.
 * @param[in]  	addr_len 	- the length of address.
 * @param[in]  	rd_data 	- pointer to the buffer that will cache the reading out data.
 * @param[in]  	rd_len	 	- length in byte of the data need to read.
 * @param[in]  	rd_mode 	- read mode.dummy or not dummy.
 * @return   	none
 */
void spi_master_write_read_dma_plus(spi_sel_e spi_sel, u8 cmd, u8 *addr, u32 addr_len, u8 *rd_data, u32 rd_len, spi_rd_tans_mode_e rd_mode);

/**
 * @brief      	This function serves to single/dual (quad) write to the SPI slave by xip.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr_offset - offset of xip base address.
 * @param[in]  	data 		- pointer to the data need to write.
 * @param[in]  	data_len 	- length in byte of the data need to write.
 * @param[in]  	wr_mode 	- write mode  dummy or not dummy.
 * @return   	none
 */
void hspi_master_write_xip(u8 cmd, u32 addr_offset, u8 *data, u32 data_len, spi_wr_tans_mode_e wr_mode);

/**
 * @brief      	This function serves to single/dual (quad) read from the SPI slave by xip.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr_offset - offset of xip base address.
 * @param[in]  	data 		- pointer to the data need to read.
 * @param[in]  	data_len 	- length in byte of the data need to read.
 * @param[in]  	rd_mode 	- read mode.dummy or not dummy.
 * @return   	none
 */
void hspi_master_read_xip(u8 cmd, u32 addr_offset, u8 *data, u32 data_len, spi_rd_tans_mode_e rd_mode);

/**
 * @brief      	This function serves to a cmd and one data write to the SPI slave by xip.
 * @param[in]  	cmd 		- cmd one byte will first write.
 * @param[in]  	addr_offset - offset of xip base address.
 * @param[in]  	data_in 	- data need to write.
 * @param[in]  	wr_mode 	- write mode  dummy or not dummy.
 * @return   	none
 */
void hspi_master_write_xip_cmd_data(u8 cmd, u32 addr_offset, u8 data_in, spi_wr_tans_mode_e wr_mode);

#endif


