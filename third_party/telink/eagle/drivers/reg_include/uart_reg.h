/********************************************************************************************************
 * @file	uart_reg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H / B.Y
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
#ifndef UART_REG_H
#define UART_REG_H
#include "../sys.h"
#include "../../common/bit.h"
/*******************************      uart0 registers: 0x140080   *******************************/
/*******************************      uart1 registers: 0x1400c0      ******************************/
#define reg_uart_data_buf_adr(i)  (0x140080+(i)*0x40)  //uart(i)

#define reg_uart_data_buf(i,j)    REG_ADDR8(reg_uart_data_buf_adr(i)+(j)) //uart(i)_buf(j)
#define reg_uart_data_hword_buf(i,j)  REG_ADDR16(reg_uart_data_buf_adr(i)+(j)*2)

#define reg_uart_data_word_buf(i)  REG_ADDR32(reg_uart_data_buf_adr(i)) //uart(i)

#define reg_uart_clk_div(i)		REG_ADDR16(0x140084+(i)*0x40)

enum{
	FLD_UART_CLK_DIV = 			BIT_RNG(0,14),
	FLD_UART_CLK_DIV_EN = 		BIT(15)
};

#define reg_uart_ctrl0(i)			REG_ADDR8(0x140086+(i)*0x40)

enum{
	FLD_UART_BPWC_O = 			BIT_RNG(0,3),
	FLD_UART_MASK_RX_IRQ = 		BIT(6),
	FLD_UART_MASK_TX_IRQ =		BIT(7)
};

#define reg_uart_ctrl1(i)         		REG_ADDR8(0x140087+(i)*0x40)

enum {
    FLD_UART_TX_CTS_POLARITY = 	BIT(0),
    FLD_UART_TX_CTS_ENABLE = 	BIT(1),
    FLD_UART_PARITY_ENABLE = 	BIT(2),
    FLD_UART_PARITY_POLARITY = 	BIT(3),       //1:odd parity   0:even parity
    FLD_UART_STOP_SEL = 		BIT_RNG(4,5),
    FLD_UART_TTL_ENABLE = 		BIT(6),
    FLD_UART_LOOPBACK_O = 		BIT(7),
};


#define reg_uart_ctrl2(i)			REG_ADDR16(0x140088+(i)*0x40)

enum {
    FLD_UART_RTS_TRIQ_LEV   	= BIT_RNG(0,3),
    FLD_UART_RTS_POLARITY 		= BIT(4),
    FLD_UART_RTS_MANUAL_V 	 	= BIT(5),
    FLD_UART_RTS_MANUAL_M 		= BIT(6),
    FLD_UART_RTS_EN 			= BIT(7),
};

#define reg_uart_ctrl3(i)        	REG_ADDR8(0x140089+(i)*0x40)

enum {
	FLD_UART_RX_IRQ_TRIQ_LEV 	= BIT_RNG(0,3),
	FLD_UART_TX_IRQ_TRIQ_LEV 	= BIT_RNG(4,7),
};
////////////////////////////////////////////////////
#define reg_uart_rx_timeout0(i)	REG_ADDR8(0x14008a+(i)*0x40)

enum{
	FLD_UART_TIMEOUT_BW		 = 	BIT_RNG(0,7),
};

#define reg_uart_rx_timeout1(i)  REG_ADDR8(0x14008b+0x40*(i))

enum{
	FLD_UART_TIMEOUT_MUL	 = 	BIT_RNG(0,1),
	FLD_UART_MARK_RXDONE	 =  BIT(2),
	//rsvd BIT(4)
	FLD_UART_P7816_EN	 	 =  BIT(5),
	FLD_UART_MASK_TXDONE	 =  BIT(6),
	FLD_UART_MASK_ERR_IRQ 	 =  BIT(7),
};



#define reg_uart_buf_cnt(i)       REG_ADDR8(0x14008c+(i)*0x40)
enum{
	FLD_UART_RX_BUF_CNT		=  BIT_RNG(0,3),
	FLD_UART_TX_BUF_CNT		=  BIT_RNG(4,7),
};

#define reg_uart_status1(i)			REG_ADDR8(0x14008d+((i)*0x40))
enum{
	FLD_UART_RBCNT 	     	=  BIT_RNG(0,2),
	FLD_UART_IRQ_O    		=  BIT(3),
	FLD_UART_WBCNT 	     	=  BIT_RNG(4,6), //R
	FLD_UART_CLEAR_RX 		=  BIT(6),		 //Write 1 clear RX
	FLD_UART_RX_ERR	 		=  BIT(7),		//R
	FLD_UART_CLEAR_TX	 	=  BIT(7),		//Write 1 clear TX
};


#define reg_uart_status2(i)       REG_ADDR8((0x14008e) +(0x40*(i)))
enum{
	FLD_UART_TX_DONE   	  =  BIT(0),//only for dma default 1.
	FLD_UART_TX_BUF_IRQ   =  BIT(1),
	FLD_UART_RX_DONE   	  =  BIT(2),
	FLD_UART_RX_BUF_IRQ   =  BIT(3),
};

//state machine use for IC debug
#define reg_uart_state(i)       REG_ADDR8(0x14008f+0x40*(i))
enum{
	FLD_UART_TSTATE_i   	=  BIT_RNG(0,2),//only for dma default 1.
	FLD_UART_RSTATE_i   	=  BIT_RNG(4,7),
};

/*******************************      7816 registers: 0x1401f0     ******************************/
#define reg_7816_clk_div		REG_ADDR8(0x1401f0)
enum{
	FLD_7816_CLK_DIV = 			BIT_RNG(4,7),
};
#endif
