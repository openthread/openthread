/********************************************************************************************************
 * @file	spi_reg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H
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
#ifndef SPI_REG_H
#define SPI_REG_H
#include "../../common/bit.h"
#include "../sys.h"

/*******************************      hspi registers: 0x1FFFFC0     ******************************/



#define    HSPI_BASE_ADDR			        0x1FFFFC0
#define    PSPI_BASE_ADDR			        0x140040
#define    BASE_ADDR_DIFF			        0x1EBFF80

#define   reg_hspi_data_buf_adr               0x1FFFFC8
#define   reg_hspi_xip_base_adr               0x1000000
#define   reg_spi_data_buf_adr(i)             0x140048+(i)*BASE_ADDR_DIFF
/**
 * BIT[0:1] the minimum time between the edge of SPI_CS and  the edges of SPI_CLK.the actual duration is (SPI_CLK_OUT/2)*(cs2sclk+1).master only
 * BIT[2]  set 3line mode ,MOSI is bi-directional signal in regular mode.master only
 * BIT[3]  transfer data with least significant bit first.1: LSB  0: MSB default. master/slave
 * BIT[4]  set dual io mode.master only
 * BIT[5:6] set spi 4 mode. master/slave
 *  *  *         bit5: CPHA-SPI_CLK Phase,bit6: CPOL-SPI_CLK Polarity
 *            MODE0:  CPHA = 0 , CPOL =0;
 *            MODE1:  CPHA = 0 , CPOL =1;
 *            MODE2:  CPHA = 1 , CPOL =0;
 *            MODE3:  CPHA = 1,  CPOL =1;
 * BIT[7]  set master/slave mode. 0 slave 1 master default.master/slave
 */
#define reg_spi_mode0(i) 			REG_ADDR8(PSPI_BASE_ADDR+(i)*BASE_ADDR_DIFF)
enum{
	FLD_SPI_CS2SCLK 	        = BIT_RNG(0,1),
	FLD_SPI_3LINE 	     	    = BIT(2),
	FLD_SPI_LSB 		        = BIT(3),
	FLD_SPI_DUAL			    = BIT(4),
    FLD_SPI_MODE_WORK_MODE     = BIT_RNG(5,6),
	FLD_SPI_MASTER_MODE      	= BIT(7),
};

/*the clock freq ratio between the source_clock and spi_clock.master only
 * spi_clock=source_clock/((spi_clk_div+1)*2)
 * spi_clk_div=reg_hspi_mode1[7:0]. max_value=0xff,spi_clock==source_clock
 */
//#define reg_hspi_mode1			REG_ADDR8(HSPI_BASE_ADDR+0x01)
#define reg_spi_mode1(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x01+(i)*BASE_ADDR_DIFF)

/**
 * BIT[0]  set cmd format 0: single mode  1: the format of the cmd phase is the same as the data phase(Dual/Quad).master only
 * BIT[1]  set spi quad I/O mode. master only
 * BIT[2]  set the spi commnd phase enable.master only
 * BIT[4:7]   the minimum time that SPI CS should stay HIGH.the actual duration is (SPI_CLK period_out / 2)*(csht+1).default=2,master only
 */
#define reg_spi_mode2(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x02+(i)*BASE_ADDR_DIFF)


enum{
	FLD_HSPI_CMD_FMT 	         =BIT(0),
	FLD_HSPI_QUAD 	             =BIT(1),
	FLD_SPI_CMD_EN 	       		 =BIT(2),
	FLD_SPI_HSPI_MODE2_RESERVED  =BIT(3),
	FLD_SPI_CSHT                 =BIT_RNG(4,7),
};

/**
 * BIT[0:7]   transfer count0 for write data.master only
 */
#define reg_spi_tx_cnt0(i)		  	   REG_ADDR8(PSPI_BASE_ADDR+0x03+(i)*BASE_ADDR_DIFF)

/**
 * BIT[0:7]   transfer count1 for write data.master only
 */

#define reg_spi_tx_cnt1(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x12+(i)*(BASE_ADDR_DIFF-0x12+0x20))
//#define reg_hspi_tx_cnt1		   REG_ADDR8(HSPI_BASE_ADDR+0x20)
/**
 * BIT[0:7]   transfer count2 for write data.master only
 */

#define reg_spi_tx_cnt2(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x13+(i)*(BASE_ADDR_DIFF-0x13+0x21))

/**
 * BIT[0:7]   transfer count0 for read data.master only
 */
#define reg_spi_rx_cnt0(i) 			 REG_ADDR8(PSPI_BASE_ADDR+0x04+(i)*BASE_ADDR_DIFF)

/**
 * BIT[0:7]   transfer count1 for read data.master only
 */

#define reg_spi_rx_cnt1(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x10+(i)*(BASE_ADDR_DIFF-0x10+0x1e))
/**
 * BIT[0:7]   transfer count2 for read data.master only
 */

#define reg_spi_rx_cnt2(i) 			REG_ADDR8(PSPI_BASE_ADDR+0x11+(i)*(BASE_ADDR_DIFF-0x11+0x1f))



/**
 * BIT[0:3]  dummy data cnt, dummy is always single wire mode, dummy number = dummy_cnt + 1.master only
 * BIT[4:7]  the transfer mode.master only
 * the transfer sequence could be:
 * 0x0:write and read at the same time(must enbale CmdEn)
 * 0x1:write only
 * 0x2:read only(must enbale CmdEn)
 * 0x3:write,read
 * 0x4:read,write
 * 0x5:write,dummy,read
 * 0x6:read,dummy,write(must enbale CmdEn)
 * 0x7:None Data(must enbale CmdEn)
 * 0x8:Dummy,write
 * 0x9:Dummy,read
 * 0xa~0xf:reserved
 */
#define reg_spi_trans0(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x05+(i)*BASE_ADDR_DIFF)

enum{
	FLD_SPI_DUMMY_CNT	       = BIT_RNG(0,3),
	FLD_SPI_TRANSMODE 	        =BIT_RNG(4,7),
};



/**
 * BIT[0:7]  SPI Command.master only
 */

#define reg_spi_trans1(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x06+(i)*BASE_ADDR_DIFF)
enum{
	FLD_SPI_CMD_RESERVED            =BIT(0),
	FLD_SPI_CMD_TRANS_HWORD         =BIT(1),//1 apb hword transfer
	FLD_SPI_CMD_TRANS_WORD	         =BIT(2),//1 apb word transfer
	FLD_SPI_CMD_RD_DUMMY_4CYCLE	 =BIT(3),// 0 8cycle 1 4cycle
	FLD_SPI_CMD_ADDR_AUTO_INCREASE  =BIT(4),// 0 AUTO incease
	FLD_SPI_CMD_DATA_DUAL	         =BIT(5),// 0 Single 1 DuaL
	FLD_SPI_CMD_ADDR_DUAL           =BIT(6),// 0 Single 1 DuaL
	FLD_SPI_CMD_RD_EN	             =BIT(7),// 0  write 1 read
};

/**
 * BIT[0] enable the SPI Receive FIFO Overrun interrupt . slave only
 * BIT[1] enable the SPI Transmit FIFO Underrun interrupt. slave only
 * BIT[2] enable the SPI Receive FIFO Threshold interrupt.master/slave
 * BIT[3] enable the SPI Transmit FIFO Threshold interrupt.master/slave
 * BIT[4] enable the SPI Transmit end interrupt.master/slave
 * BIT[5] enable  slvCmdint.The slave command interrupt is triggered each byte command received (starting 8 bit) .slave only
 * BIT[6] enable RX DMA
 * BIT[7] enable TX DMA
 */

#define reg_spi_trans2(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x07+(i)*BASE_ADDR_DIFF)
enum{
	FLD_SPI_RXFIFO_OR_INT_EN        =BIT(0),
	FLD_SPI_TXFIFO_UR_INT_EN        =BIT(1),
	FLD_SPI_RXFIFO_INT_EN 	        =BIT(2),
	FLD_SPI_TXFIFO_INT_EN		    =BIT(3),
	FLD_SPI_END_INT_EN              =BIT(4),
	FLD_SPI_SLV_CMD_EN	            =BIT(5),
	FLD_SPI_RX_DMA_EN               =BIT(6),
	FLD_SPI_TX_DMA_EN 	            =BIT(7),
};


/**
 * BIT[0:7]   data0[7:0] to transmit or received.
 */

#define reg_spi_wr_rd_data0(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x08+(i)*BASE_ADDR_DIFF)
/**
 * BIT[0:7]   data1[7:0] to transmit or received.
 */

#define reg_spi_wr_rd_data1(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x09+(i)*BASE_ADDR_DIFF)
/**
 * BIT[0:7]   data2[7:0] to transmit or received.
 */
#define reg_spi_wr_rd_data2(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x0a+(i)*BASE_ADDR_DIFF)

/**
 * BIT[0:7]   data3[7:0] to transmit or received.
 */

#define reg_spi_wr_rd_data3(i) 		  	REG_ADDR8(PSPI_BASE_ADDR+0x0b+(i)*BASE_ADDR_DIFF)

#define reg_spi_wr_rd_data(i,j)		 REG_ADDR8(PSPI_BASE_ADDR+0x08+(j)+(i)*BASE_ADDR_DIFF)



/**
 * BIT[0:3]  number of valid entries in the rxfifo.
 * BIT[4:7]  number of valid entries in the txfifo.
 */
#define reg_spi_fifo_num(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x0c+(i)*BASE_ADDR_DIFF)

enum{
	FLD_SPI_RXF_NUM          = BIT_RNG(0,3),
	FLD_SPI_TXF_NUM          = BIT_RNG(4,7),
};

/**
 * BIT[2]  rxfifo reset,write 1 to reset. Spi clock must turn on.
 * BIT[3]  txfifo reset,write 1 to reset. Spi clock must turn on
 * BIT[4]  rxfifo full flag.
 * BIT[5]  rxfifo empty flag
 * BIT[6]  txfifo full flag.
 * BIT[7]  txfifo empty flag
 */
#define reg_spi_fifo_state(i) 		REG_ADDR8(PSPI_BASE_ADDR+0x0d+(i)*BASE_ADDR_DIFF)

enum{
	FLD_SPI_FIFO_STA_RESERVED 	= BIT_RNG(0,1),
	FLD_SPI_RXF_CLR	      		=BIT(2),
	FLD_SPI_TXF_CLR		        =BIT(3),
	FLD_SPI_RXF_FULL            =BIT(4),
	FLD_SPI_RXF_EMPTY	        =BIT(5),
	FLD_SPI_TXF_FULL            =BIT(6),
    FLD_SPI_TXF_EMPTY	        =BIT(7),
};

/**
 * BIT[2]  RX FIFO Overrun interrupt status. slave only
 * BIT[3]  TX FIFO Underrun interrupt status. slave only
 * BIT[4]  RX FIFO Threshold interrupt status.set 1 to clear. master/slave
 * BIT[5]  TX FIFO Threshold interrupt status.set 1 to clear. master/slave
 * BIT[6]  End of SPI Transfer interrupt status.set 1 to clear.master/slave
 * BIT[7]  Slave Command Interrupt status.set 1 to clear.slave only
 */
#define reg_spi_irq_state(i) 		REG_ADDR8(PSPI_BASE_ADDR + 0x0e +(i)*BASE_ADDR_DIFF)

enum{
	FLD_SPI_STATE_RESERVED  =BIT_RNG(0,1),
	FLD_SPI_RXF_OR_INT	    =BIT(2),
	FLD_SPI_TXF_UR_INT	    =BIT(3),
	FLD_SPI_RXF_INT         =BIT(4),
	FLD_SPI_TXF_INT 	    =BIT(5),
	FLD_SPI_END_INT         =BIT(6),
    FLD_SPI_SLV_CMD_INT	    =BIT(7),
};

/**
 * BIT[0]  set this bit to indicate that spi as slave is ready for data transaction
 * BIT[1]  spi soft reset.high valid.
 * BIT[4:6] fifo threshold. 0x4 default.
 * BIT[7]  SPI transfer status .1 is busy, 0 not busy.
 */
#define reg_spi_status(i) 		REG_ADDR8(PSPI_BASE_ADDR + 0x0f +(i)*BASE_ADDR_DIFF)
enum{
	FLD_HSPI_SLAVE_READY	       =BIT(0),
	FLD_HSPI_SOFT_RESET	           =BIT(1),
	FLD_HSPI_HSPI_STATUS_RESERVED  =BIT_RNG(2,3),
	FLD_HSPI_FIFO_THRES            =BIT_RNG(4,6),
	FLD_HSPI_BUSY                  =BIT(7),
};

/**
 * BIT[0:7]  hspi_addr0.
 */
#define reg_hspi_addr0		REG_ADDR8(HSPI_BASE_ADDR+0x10)

/**
 * BIT[0:7]  hspi_addr1.
 */
#define reg_hspi_addr1		REG_ADDR8(HSPI_BASE_ADDR+0x11)

/**
 * BIT[0:7]  hspi_addr2.
 */
#define reg_hspi_addr2		REG_ADDR8(HSPI_BASE_ADDR+0x12)

/**
 * BIT[0:7]  hspi_addr3.
 */
#define reg_hspi_addr3		REG_ADDR8(HSPI_BASE_ADDR+0x13)

#define reg_hspi_addr(i)	REG_ADDR8(HSPI_BASE_ADDR+0x10+i)

/**   hspi_addr0~ hspi_addr3.
 */
#define reg_hspi_addr_32		REG_ADDR32(HSPI_BASE_ADDR+0x10)



/**
 * BIT[0]  1:enable addr phase, master only
 * BIT[1]  0:single mode  1:the format of addr phase is the same as the data phase(Dual/Quad).master only
 * BIT[2:3] address length.2'b00:1bye  2'b01:2bytes  2'b10:3bytes  2'b11:4bytes.master only
 * BIT[4]  enable xip.
 * BIT[5]  stop xip.
 * BIT[6]  xip mode 0:xip normal mode  1:xip sequential mode
 * BIT[7]  0:xip timeout ,disable 1:xip timeout enable .default 1
 */
#define reg_hspi_xip_ctrl		REG_ADDR8(HSPI_BASE_ADDR+0x14)
enum{
	FLD_HSPI_ADDR_EN	        = BIT(0),
	FLD_HSPI_ADDR_FMT	        = BIT(1),
	FLD_HSPI_ADDR_LEN           = BIT_RNG(2,3),
	FLD_HSPI_XIP_ENABLE         = BIT(4),
	FLD_HSPI_XIP_STOP           = BIT(5),
	FLD_HSPI_XIP_MODE           = BIT(6),
	FLD_HSPI_XIP_TIMEOUT_MODE   = BIT(7),
};


/**
 * BIT[0:7] write command used for xip
 */
#define reg_hspi_xip_wr_cmd		 REG_ADDR8(HSPI_BASE_ADDR+0x15)

/**
 * BIT[0:7] read command used for xip
 */
#define reg_hspi_xip_rd_cmd		 REG_ADDR8(HSPI_BASE_ADDR+0x16)

/**
 * BIT[0:7]  Use this combined with xip_mode being xip sequential mode.default page boundary size is 32byte, 2^page_size.
 */
#define reg_hspi_page_size		 REG_ADDR8(HSPI_BASE_ADDR+0x17)


/**
 * BIT[0:3]  xip write mode .default 1, write trans mode is write only.
 * BIT[4:7]  xip read mode.  default 2 ,read trans  mode is read only.
 */
#define reg_hspi_xip_trans_mode  REG_ADDR8(HSPI_BASE_ADDR+0x18)
enum{
 FLD_HSPI_XIP_WR_TRANS_MODE          = BIT_RNG(0,3),
 FLD_HSPI_XIP_RD_TRANS_MODE          = BIT_RNG(4,7),
};

/**
 * BIT[0:7]  xip_addr_offset0.
 */
#define reg_hspi_xip_addr_offset0  REG_ADDR8(HSPI_BASE_ADDR+0x19)

/**
 * BIT[0:7]  xip_addr_offset1.
 */
#define reg_hspi_xip_addr_offset1  REG_ADDR8(HSPI_BASE_ADDR+0x1a)


/**
 * BIT[0:7]  xip_addr_offset2.
 */
#define reg_hspi_xip_addr_offset2  REG_ADDR8(HSPI_BASE_ADDR+0x1b)

/**
 * BIT[0:7]  xip_addr_offset3.
 */
#define reg_hspi_xip_addr_offset3  REG_ADDR8(HSPI_BASE_ADDR+0x1c)



/**
 * BIT[0:7]  when XIP_TIMEOUT_MODE enable,timeout period=spi_clock_out_period*timeout_cnt
 */
#define reg_hspi_xip_timeout_cnt   REG_ADDR8(HSPI_BASE_ADDR+0x1d)
/**
 * BIT[0]    1:enable hspi 3line dcx (data/command selection), master only,for spi panel(LCD OLED ...)
 * BIT[1]    hspi 3line dcx (data/command selection). 0:command 1:data
 * BIT[4:2]  2data_lane mode.3'b000:2data_lane close  3'b001:RGB565  3'b011:RGB666  3'b011:RGB888.
 */
#define reg_hspi_panel_ctrl		REG_ADDR8(HSPI_BASE_ADDR+0x22)
enum{
	FLD_HSPI_PANEL_3LINE_DCX_EN     = BIT(0),
	FLD_HSPI_PANEL_3LINE_DCX	    = BIT(1),
	FLD_HSPI_PANEL_2DATA_LANE       = BIT_RNG(2,4),
};


#endif
