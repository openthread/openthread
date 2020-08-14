/********************************************************************************************************
 * @file     pspi_reg.h
 *
 * @brief    This is the source file for TLSR9518
 *
 * @author	 Driver Group
 * @date     September 16, 2019
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
 * 			 1.initial release(September 16, 2019)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#ifndef PSPI_REG_H
#define PSPI_REG_H
#include "../../common/bit.h"
#include "../sys.h"
/*******************************      pspi registers: 0x140040     ******************************/
#define    reg_pspi_data_buf_adr                0x80140048
#define    PSPI_BASE_ADDR			        0x140040
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
 * BIT[7]  set master/salve mode. 0 salve 1 master default.master/slave
 */
#define reg_pspi_mode0			REG_ADDR8(PSPI_BASE_ADDR)
enum{
	FLD_PSPI_CS2SCLK 	        = BIT_RNG(0,1),
	FLD_PSPI_3LINE 	     	    = BIT(2),
	FLD_PSPI_LSB 		        = BIT(3),
	FLD_PSPI_DUAL			    = BIT(4),
    FLD_PSPI_MODE_WORK_MODE     = BIT_RNG(5,6),
	FLD_PSPI_MASTER_MODE      	= BIT(7),
};

/*the clock freq ratio between the source_clock and spi_clock.master only
 * spi_clock=source_clock/((spi_clk_div+1)*2)
 * spi_clk_div=reg_pspi_mode1[7:0]. max_value=0xff,spi_clock==source_clock
 */
#define reg_pspi_mode1			REG_ADDR8(PSPI_BASE_ADDR+0x01)

/**
 * BIT[2]  set the spi commnd phase enable.master only
 * BIT[4:7]   the minimum time that SPI CS should stay HIGH.the actual duration is (SPI_CLK period_out / 2)*(csht+1).default=2,master only
 */
#define reg_pspi_mode2			REG_ADDR8(PSPI_BASE_ADDR+0x02)//PSPI not support quad mode  and cmd format
enum{
	FLD_PSPI_MODE2_RESERVED0 =BIT_RNG(0,1),
	FLD_PSPI_CMD_EN 	     =BIT(2),
	FLD_PSPI_MODE2_RESERVED1 =BIT(3),
	FLD_PSPI_CSHT            =BIT_RNG(4,7),//default=2,the minimum time that SPI CS should stay HIGH.the actual duration is (SPI_CLK period / 2)*(csht+1)
};

/**
 * BIT[0:7]   transfer count0 for write data.master only
 */
#define reg_pspi_tx_cnt0			   REG_ADDR8(PSPI_BASE_ADDR+0x03)

/**
 * BIT[0:7]   transfer count1 for write data.master only
 */
#define reg_pspi_tx_cnt1			   REG_ADDR8(PSPI_BASE_ADDR+0x12)

/**
 * BIT[0:7]   transfer count2 for write data.master only
 */
#define reg_pspi_tx_cnt2			   REG_ADDR8(PSPI_BASE_ADDR+0x13)


/**
 * BIT[0:7]   transfer count for read data.master only
 */
#define reg_pspi_rx_cnt0			REG_ADDR8(PSPI_BASE_ADDR+0x04)

/**
 * BIT[0:7]   transfer count for read data.master only
 */
#define reg_pspi_rx_cnt1			REG_ADDR8(PSPI_BASE_ADDR+0x10)

/**
 * BIT[0:7]   transfer count for read data.master only
 */
#define reg_pspi_rx_cnt2			REG_ADDR8(PSPI_BASE_ADDR+0x11)


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
#define reg_pspi_trans0			REG_ADDR8(PSPI_BASE_ADDR+0x05)
enum{
	FLD_PSPI_DUMMY_CNT	       = BIT_RNG(0,3),
	FLD_PSPI_TRANSMODE 	        =BIT_RNG(4,7),
};


/**
 * BIT[0:7]  SPI Command.
 */
#define reg_pspi_trans1		   REG_ADDR8(PSPI_BASE_ADDR+0x06)
enum{
	FLD_PSPI_CMD_RESERVED             =BIT(0),
	FLD_PSPI_CMD_TRANS_HWORD          =BIT(1),//1 apb hword transfer
	FLD_PSPI_CMD_TRANS_WORD	         =BIT(2),//1 apb word transfer
	FLD_PSPI_CMD_RD_DUMMY_4CYCLE		 =BIT(3),// 0 8cycle 1 4cycle
	FLD_PSPI_CMD_ADDR_AUTO_INCREASE   =BIT(4),// 0 AUTO incease
	FLD_PSPI_CMD_DATA_DUAL	         =BIT(5),// 0 Single 1 DuaL
	FLD_PSPI_CMD_ADDR_DUAL            =BIT(6),// 0 Single 1 DuaL
	FLD_PSPI_CMD_RD_EN	             =BIT(7),// 0  write 1 read
};

/**
 * BIT[0] enable the SPI Receive FIFO Overrun interrupt . slave only
 * BIT[1] enable the SPI Transmit FIFO Underrun interrupt. slave only
 * BIT[2] enable the SPI Receive FIFO Threshold interrupt.master/salve
 * BIT[3] enable the SPI Transmit FIFO Threshold interrupt.master/salve
 * BIT[4] enable the SPI Transmit end interrupt.master/salve
 * BIT[5] enable  slvCmdint.The slave command interrupt is triggered each byte command received (starting 8 bit) .slave only
 * BIT[6] enable RX DMA
 * BIT[7] enable TX DMA
 */
#define reg_pspi_trans2		   REG_ADDR8(PSPI_BASE_ADDR+0x07)//default 0x00
enum{
	FLD_PSPI_RXFIFO_OR_INT_EN        =BIT(0),// enable the SPI Receive FIFO Overrun interrupt .not value in  slave mode
	FLD_PSPI_TXFIFO_OU_INT_EN        =BIT(1),// enable the SPI Transmit FIFO Underrun interrupt. not value in  slave mode
	FLD_PSPI_RXFIFO_INT_EN 	         =BIT(2),//enable the SPI Receive FIFO Threshold interrupt
	FLD_PSPI_TXFIFO_INT_EN		     =BIT(3),//enable the SPI Transmit FIFO Threshold interrupt
	FLD_PSPI_END_INT_EN              =BIT(4),
	FLD_PSPI_SLV_CMD_EN	             =BIT(5),//slvCmdint
	FLD_PSPI_RX_DMA_EN               =BIT(6),
	FLD_PSPI_TX_DMA_EN 	             =BIT(7),
};



/**
 * BIT[0:7]   data0[7:0] to transmit or received.
 */
#define reg_pspi_wr_rd_data0		      REG_ADDR8(PSPI_BASE_ADDR+0x08)
/**
 * BIT[0:7]   data1[7:0] to transmit or received.
 */
#define reg_pspi_wr_rd_data1		      REG_ADDR8(PSPI_BASE_ADDR+0x09)
/**
 * BIT[0:7]   data2[7:0] to transmit or received.
 */
#define reg_pspi_wr_rd_data2		      REG_ADDR8(PSPI_BASE_ADDR+0x0a)
/**
 * BIT[0:7]   data3[7:0] to transmit or received.
 */
#define reg_pspi_wr_rd_data3		      REG_ADDR8(PSPI_BASE_ADDR+0x0b)

#define reg_pspi_wr_rd_data(i)		      REG_ADDR8(PSPI_BASE_ADDR+0x08+i)

/**
 * BIT[0:3]  number of valid entries in the rxfifo.
 * BIT[4:7]  number of valid entries in the txfifo.
 */
#define reg_pspi_fifo_num  REG_ADDR8(PSPI_BASE_ADDR+0x0c)
enum{
FLD_PSPI_RXF_NUM          = BIT_RNG(0,3),
FLD_PSPI_TXF_NUM          = BIT_RNG(4,7),
};

/**
 * BIT[2]  rxfifo reset,write 1 to reset. Spi clock must turn on.
 * BIT[3]  txfifo reset,write 1 to reset. Spi clock must turn on
 * BIT[4]  rxfifo full flag.
 * BIT[5]  rxfifo empty flag
 * BIT[6]  txfifo full flag.
 * BIT[7]  txfifo empty flag
 */
#define reg_pspi_fifo_state	  REG_ADDR8(PSPI_BASE_ADDR+0x0d)
enum{
	FLD_PSPI_FIFO_STA_RESERVED = BIT_RNG(0,1),
	FLD_PSPI_RXF_CLR	       =BIT(2),
	FLD_PSPI_TXF_CLR		   =BIT(3),
	FLD_PSPI_RXF_FULL          =BIT(4),
	FLD_PSPI_RXF_EMPTY	       =BIT(5),
	FLD_PSPI_TXF_FULL          =BIT(6),
    FLD_PSPI_TXF_EMPTY	       =BIT(7),
};

/**
 * BIT[2]  RX FIFO Overrun interrupt status. slave only
 * BIT[3]  TX FIFO Overrun interrupt status. slave only
 * BIT[4]  RX FIFO Threshold interrupt status.set 1 to clear. master/slave
 * BIT[5]  TX FIFO Threshold interrupt status.set 1 to clear. master/slave
 * BIT[6]  End of SPI Transfer interrupt status.set 1 to clear.master/slave
 * BIT[7]  Slave Command Interrupt status.set 1 to clear.slave only
 */
#define reg_pspi_interrupt_state	  REG_ADDR8(PSPI_BASE_ADDR+0x0e)
enum{
	FLD_PSPI_STATE_RESERVED  =BIT_RNG(0,1),
	FLD_PSPI_RXF_UR_INT	     =BIT(2),
	FLD_PSPI_TXF_UR_INT	     =BIT(3),
	FLD_PSPI_RXF_INT         =BIT(4),
	FLD_PSPI_TXF_INT 	     =BIT(5),
	FLD_PSPI_END_INT         =BIT(6),
    FLD_PSPI_SLV_CMD_INT	 =BIT(7),
};

/**
 * BIT[0]  set this bit to indicate that spi as salve is ready for data transaction
 * BIT[1]  spi soft reset.high valid.
 * BIT[4:6] fifo threshold. 0x4 default.
 * BIT[7]  SPI transfer status .1 is busy, 0 not busy.
 */
#define reg_pspi_status		REG_ADDR8(PSPI_BASE_ADDR+0x0f)
enum{
	FLD_PSPI_SLAVE_READY	    =BIT(0),
	FLD_PSPI_SOFT_RESET	        =BIT(1),
	FLD_PSPI_STATUS_RESERVED    =BIT_RNG(2,3),
	FLD_PSPI_FIFO_THRES         =BIT_RNG(4,6),
	FLD_PSPI_BUSY               =BIT(7),
};


#endif
