/********************************************************************************************************
 * @file     uart.h
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
/**	@page UART
 *
 *	Introduction
 *	===============
 *	TLSR9518 supports two uart: uart0~ uart1.
 *
 *	API Reference
 *	===============
 *	Header File: uart.h
 */
#ifndef UART_H_
#define UART_H_

#include "dma.h"
#include "gpio.h"
#include "timer.h"
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
 *  @brief  Define parity type
 */
typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
} uart_parity_e;

/**
 *  @brief  Define UART chn
 */
typedef enum
{
    UART0 = 0,
    UART1,
} uart_num_e;

/**
 *  @brief  Define mul bits
 */
typedef enum
{
    UART_BW_MUL1 = 0,
    UART_BW_MUL2 = 1,
    UART_BW_MUL3 = 2,
    UART_BW_MUL4 = 3,
} uart_timeout_mul_e;

/**
 *  @brief  Define the length of stop bit
 */
typedef enum
{
    UART_STOP_BIT_ONE          = 0,
    UART_STOP_BIT_ONE_DOT_FIVE = BIT(4),
    UART_STOP_BIT_TWO          = BIT(5),
} uart_stop_bit_e;

/**
 *  @brief  Define UART RTS mode
 */
typedef enum
{
    UART_RTS_MODE_AUTO = 0,
    UART_RTS_MODE_MANUAL,
} uart_rts_mode_e;

/**
 *  @brief  Define UART CTS pin : UART0(PA1 PB6 PD0), UART1(PC4 PD4 PE1)
 */
typedef enum
{
    UART0_CTS_PA1 = GPIO_PA1,
    UART0_CTS_PB6 = GPIO_PB6,
    UART0_CTS_PD0 = GPIO_PD0,

    UART1_CTS_PC4 = GPIO_PC4,
    UART1_CTS_PD4 = GPIO_PD4,
    UART1_CTS_PE1 = GPIO_PE1,
} uart_cts_pin_e;

/**
 *  @brief  Define UART RTS pin : UART0(PA2 PB4 PD1), UART1(PC5 PD5 PE3)
 */
typedef enum
{
    UART0_RTS_PA2 = GPIO_PA2,
    UART0_RTS_PB4 = GPIO_PB4,
    UART0_RTS_PD1 = GPIO_PD1,

    UART1_RTS_PC5 = GPIO_PC5,
    UART1_RTS_PD5 = GPIO_PD5,
    UART1_RTS_PE3 = GPIO_PE3,
} uart_rts_pin_e;

/**
 *  @brief  Define UART TX pin : UART0(PA3 PB2 PD2), UART1(PC6 PD6 PE0)
 */
typedef enum
{
    UART0_TX_PA3 = GPIO_PA3,
    UART0_TX_PB2 = GPIO_PB2,
    UART0_TX_PD2 = GPIO_PD2,

    UART1_TX_PC6 = GPIO_PC6,
    UART1_TX_PD6 = GPIO_PD6,
    UART1_TX_PE0 = GPIO_PE0,
} uart_tx_pin_e;

/**
 *  @brief  Define UART RX pin : UART0(PA4 PB3 PD3), UART1(PC7 PD7 PE2)
 */
typedef enum
{
    UART0_RX_PA4 = GPIO_PA4,
    UART0_RX_PB3 = GPIO_PB3,
    UART0_RX_PD3 = GPIO_PD3,

    UART1_RX_PC7 = GPIO_PC7,
    UART1_RX_PD7 = GPIO_PD7,
    UART1_RX_PE2 = GPIO_PE2,
} uart_rx_pin_e;

/**
 *  @brief  Define UART IRQ MASK
 */
typedef enum
{
    UART_RX_IRQ_MASK = BIT(6),
    UART_TX_IRQ_MASK = BIT(7),

    UART_RXDONE_MASK  = BIT(2) | BIT(8),
    UART_TXDONE_MASK  = BIT(6) | BIT(8),
    UART_ERR_IRQ_MASK = BIT(7) | BIT(8),
} uart_irq_mask_e;

/**
 *  @brief  Define UART IRQ BIT STATUS FOR GET
 */
typedef enum
{
    UART_RX_ERR           = BIT(7),
    UART_TXDONE           = BIT(0),
    UART_TXBUF_IRQ_STATUS = BIT(1),
    UART_RXDONE           = BIT(2),
    UART_RXBUF_IRQ_STATUS = BIT(3),
} uart_irq_status_get_e;

/**
 *  @brief  Define UART IRQ BIT STATUS FOR CLR
 */
typedef enum
{
    UART_CLR_RX = BIT(6),
    UART_CLR_TX = BIT(7),
} uart_irq_status_clr_e;

/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/
/**
 * @brief     This function serves to get the rxfifo cnt.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_rxfifo_num(uart_num_e uart_num)
{
    return reg_uart_buf_cnt(uart_num) & FLD_UART_RX_BUF_CNT;
}

/**
 * @brief     This function serves to get the txfifo cnt.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_txfifo_num(uart_num_e uart_num)
{
    return (reg_uart_buf_cnt(uart_num) & FLD_UART_TX_BUF_CNT) >> 4;
}

/**
 * @brief     This function resets the UART module.
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_reset(uart_num_e uart_num)
{
    reg_rst0 &= (~((uart_num) ? FLD_RST0_UART1 : FLD_RST0_UART0));
    reg_rst0 |= ((uart_num) ? FLD_RST0_UART1 : FLD_RST0_UART0);
}

/**
 * @brief     This function enable the clock of UART module.
 * @param[in] uart_num - UART0/UART1
 * @return    none
 */
static inline void uart_clk_en(uart_num_e uart_num)
{
    reg_clk_en0 |= ((uart_num) ? FLD_CLK0_UART1_EN : FLD_CLK0_UART0_EN);
}

/**
 * @brief      	This function initializes the UART module.
 * @param[in]	uart_num	-  UART0 or UART1
 * @param[in]  	div  		-  uart clock divider
 * @param[in]  	bwpc     	-  bitwidth, should be set to larger than
 *
 *  			sys_clk      baud rate   g_uart_div         g_bwpc
 *
 *	  	  	  	16Mhz        9600          118   			 13
 *           	 	 	 	 19200         118     			 6
 *          	 	 	 	 115200         9       		 13
 *
 *	  	  	  	24Mhz        9600          249       		 9
 *           	 	 	 	 19200		   124               9
 *           	 	 	 	 115200         12    			 15
 *
 *	  	  	  	32Mhz        9600          235       		 13
 *           	 	 	 	 19200		   235               6
 *           	 	 	 	 115200         17    			 13
 *
 *	  	  	  	48Mhz        9600          499       		 9
 *           	 	 	 	 19200		   249               9
 *           	 	 	 	 115200         25    			 15
 *
 * @param[in]  parity      - selected parity type for UART interface
 * @param[in]  stop_bit     - selected length of stop bit for UART interface
 * @return     none
 */
extern void uart_init(uart_num_e      uart_num,
                      unsigned short  div,
                      unsigned char   bwpc,
                      uart_parity_e   parity,
                      uart_stop_bit_e stop_bit);

/***********************************************************
 * @brief  		calculate the best bwpc(bit width) .i.e reg0x96
 * @algorithm: 	BaudRate*(div+1)*(bwpc+1) = system clock
 *    			simplify the expression: div*bwpc =  constant(z)
 *    			bwpc range from 3 to 15.so loop and
 *    			get the minimum one decimal point
 * @param[in]	baudrate: baut rate of UART
 * 				tmp_sysclk: system clock
 * @param[out]	div  --  uart clock divider
 * @param[out]	bwpc --  bitwidth, should be set to larger than 2
 * @return 		the position of getting the minimum value
 */
extern void uart_cal_div_and_bwpc(unsigned int    baudrate,
                                  unsigned int    System_clock,
                                  unsigned short *div,
                                  unsigned char *bwpc); // get the best bwpc and uart_div

/***********************************************************
 * @brief  		set r_rxtimeout. this setting is transfer one bytes need cycles base on uart_clk.
 * 				For example, if  transfer one bytes (1start bit+8bits data+1 priority bit+2stop bits) total 12 bits,
 * 				this register setting should be (bpwc+1)*12.
 * @param[in]	uart_num	-- 	UART0 or UART1
 * @param[in]	bwpc 		--  bitwidth, should be set to larger than 2
 * @param[in]	bit_cnt 	--  bit number
 * @param[in]	mul		 	--  mul
 * @return 		none
 */
extern void uart_set_dma_rx_timeout(uart_num_e         uart_num,
                                    unsigned char      bwpc,
                                    unsigned char      bit_cnt,
                                    uart_timeout_mul_e mul);

/**
 * @brief     config the number level setting the irq bit of status register.
 *            If the cnt register value(0x14008c[0,3]) larger or equal than the value of 0x140089[0,3]
 *            it will set the irq bit of status register 0x14008d, ie 0x14008d[3]
 * @param[in] uart_num - UART0 or UART1
 * @param[in] rx_level - receive level value. ie 0x140089[0,3]
 * @return    none
 */
static inline void uart_rx_irq_trig_level_ndma(uart_num_e uart_num, unsigned char rx_level)
{
    reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_RX_IRQ_TRIQ_LEV)) | (rx_level & 0x0f);
}

/**
 * @brief     config the number level setting the irq bit of status register.
 *            If the cnt register value(0x14008c[4,7]) less or equal than the value of 0x140089[4,7],
 *            it will set the irq bit of status register 0x14008d, ie 0x14008d[3]
 * @param[in] uart_num - UART0 or UART1
 * @param[in] tx_level - transmit level value.ie 0x140089[4,7]
 * @return    none
 */
static inline void uart_tx_irq_trig_level_ndma(uart_num_e uart_num, unsigned char tx_level)
{
    reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_TX_IRQ_TRIQ_LEV)) | (tx_level << 4);
}

/**
 * @brief     uart send data function by byte with not DMA method.
 *            variable uart_TxIndex,it must cycle the four registers 0x14080 0x14081 0x14082 0x14083 for the design of
 * SOC. so we need variable to remember the index.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] tx_data - the data to be send.
 * @return    none
 */
extern void uart_send_byte(uart_num_e uart_num, unsigned char tx_data);

/**
 * @brief     This function serves to receive uart data by byte with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
extern unsigned char uart_read_byte(uart_num_e uart_num);

/**
 * @brief     This function serves to send uart0 data by halfword with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] tx_data - the data to be send.
 * @return    none
 */
extern void uart_send_hword(uart_num_e uart_num, unsigned short tx_data);

/**
 * @brief     This function serves to send uart0 data by word with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] uartData - the data to be send.
 * @return    none
 */
extern void uart_send_word(uart_num_e uart_num, unsigned int uartData);

/**
 * @brief     This function sets the RTS pin's level manually
 * @param[in] uart_num - UART0 or UART1
 * @param[in] Polarity - set the output of RTS pin(only for manual mode)
 * @return    none
 */
extern void uart_set_rts_level(uart_num_e uart_num, unsigned char Polarity);

/**
 *	@brief		This function serves to set pin for UART0 cts function .
 *	@param[in]  cts_pin -To set cts pin
 *	@return		none
 */
extern void uart_set_cts_pin(uart_cts_pin_e cts_pin);

/**
 *	@brief		This function serves to set pin for UART0 rts function .
 *	@param[in]  rts_pin -To set rts pin
 *	@return		none
 */
extern void uart_set_rts_pin(uart_rts_pin_e rts_pin);

/**
 * @brief      This function serves to select pin for UART module.
 * @param[in]  tx_pin   - the pin to send data.
 * @param[in]  rx_pin   - the pin to receive data.
 * @return     none
 */
extern void uart_set_pin(uart_tx_pin_e tx_pin, uart_rx_pin_e rx_pin);

/**
 * @brief     	uart0 send data function, this  function tell the DMA to get data from the RAM and start
 * @param[in]  	uart_num - UART0 or UART1
 *            	the DMA transmission
 * @param[in] 	Addr - pointer to the buffer containing data need to send
 * @param[in] 	len - DMA transmission length
 * @return  	none
 */
extern unsigned char uart_send_dma(uart_num_e uart_num, unsigned char *Addr, unsigned char len);

/**
 * @brief     	uart0 receive data function, this  function tell the DMA to get data from the uart0 data fifo and start
 *            	the DMA transmission
 * @param[in]  	uart_num - UART0 or UART1
 * @param[in] 	Addr - pointer to the buffer  receive data¡£
 * @return     	none
 */
extern void uart_receive_dma(uart_num_e uart_num, unsigned char *Addr);

/**
 * @brief     This function serves to set uart tx_dam channel and config dma tx default.
 * @param[in] uart_num: UART0 or UART1.
 * @param[in] chn: dma channel.
 * @return    none
 */
extern void uart_set_tx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
 * @brief     This function serves to set uart rx_dam channel and config dma rx default.
 * @param[in] uart_num: UART0 or UART1.
 * @param[in] chn: dma channel.
 * @return    none
 */
extern void uart_set_rx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
 * @brief     get the status of uart irq.
 * @param[in] uart_num - UART0 or UART1
 * @return    0: not uart irq ;
 *            not 0: indicate txdone rxdone tx_buf rx_buf rx_err irq
 */
static inline unsigned char uart_ndmairq_get(uart_num_e uart_num)
{
    return ((reg_uart_status1(uart_num)) & FLD_UART_IRQ_O);
}

/**
 * @brief     config the irq of uart tx and rx
 * @param[in] uart_num - UART0 or UART1
 * @param[in] mask - uart irq mask
 * @return    none
 */
static inline void uart_set_irq_mask(uart_num_e uart_num, uart_irq_mask_e mask)
{
    if ((mask == UART_RX_IRQ_MASK) || (mask == UART_TX_IRQ_MASK))
    {
        reg_uart_ctrl0(uart_num) |= (mask);
    }
    else
    {
        reg_uart_rx_timeout1(uart_num) |= ((unsigned char)mask);
    }
}

/**
 * @brief     clear the irq of uart tx and rx
 * @param[in] uart_num - UART0 or UART1
 * @param[in] mask - uart irq mask
 * @return    none
 */
static inline void uart_clr_irq_mask(uart_num_e uart_num, uart_irq_mask_e mask)
{
    if ((mask == UART_RX_IRQ_MASK) || (mask == UART_TX_IRQ_MASK))
    {
        reg_uart_ctrl0(uart_num) &= ~(mask);
    }
    else
    {
        reg_uart_rx_timeout1(uart_num) &= ~((unsigned char)mask);
    }
}

/**
 * @brief     get the irq status of uart tx and rx
 * @param[in] uart_num - UART0 or UART1
 * @param[in] status 	- uart irq mask
 * @return    irq status
 */
static inline u32 uart_get_irq_status(uart_num_e uart_num, uart_irq_status_get_e status)
{
    if (status == UART_RX_ERR)
    {
        return (reg_uart_status1(uart_num) & (status));
    }
    else
    {
        return (reg_uart_status2(uart_num) & (status));
    }
}

/**
 * @brief     clear the irq status of uart tx and rx
 * @param[in] uart_num - UART0 or UART1
 * @param[in] status 	- uart irq mask
 * @return    none
 */
static inline void uart_clr_irq_status(uart_num_e uart_num, uart_irq_status_clr_e status)
{
    reg_uart_status1(uart_num) |= (status);
}

/**
 * @brief     uart rts enable
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_set_rts_en(uart_num_e uart_num)
{
    reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_EN; // enable RTS function
}

/**
 * @brief     uart rts disable
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_set_rts_dis(uart_num_e uart_num)
{
    reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_EN); // disable RTS function
}

/**
 * @brief     uart cts enable
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_set_cts_en(uart_num_e uart_num)
{
    reg_uart_ctrl1(uart_num) |= FLD_UART_TX_CTS_ENABLE; // enable CTS function
}

/**
 * @brief     uart cts disable
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_set_cts_dis(uart_num_e uart_num)
{
    reg_uart_ctrl1(uart_num) &= (~FLD_UART_TX_CTS_ENABLE); // disable CTS function
}

/**
 * @brief     UART hardware flow control configuration. Configure CTS.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] cts_pin   - RTS pin select.
 * @param[in] cts_parity - when CTS's input equals to select, tx will be stopped
 * @return    none
 */
extern void uart_cts_config(uart_num_e uart_num, uart_cts_pin_e cts_pin, u8 cts_parity);

/**
 * @brief     UART hardware flow control configuration. Configure RTS.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] rts_pin - RTS pin select.
 * @param[in] rts_parity - whether invert the output of RTS pin(only for auto mode)
 * @param[in] auto_mode_en - set the mode of RTS(auto or manual).
 * @return    none
 */
extern void uart_rts_config(uart_num_e uart_num, uart_rts_pin_e rts_pin, u8 rts_parity, u8 auto_mode_en);

/**
 * @brief     set uart rts trig lexel in auto mode
 * @param[in] uart_num - UART0 or UART1
 * @param[in] level - threshold of trig RTS pin's level toggle(only for auto mode),
 * 						it means the number of bytes that has arrived in Rx buf.
 * @return    none
 */
static inline void uart_rts_trig_level_auto_mode(uart_num_e uart_num, u8 level)
{
    reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_TRIQ_LEV);
    reg_uart_ctrl2(uart_num) |= (level & FLD_UART_RTS_TRIQ_LEV);
}

/**
 * @brief     set uart rts auto mode
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_rts_auto_mode(uart_num_e uart_num)
{
    reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_MANUAL_M);
}

/**
 * @brief     set uart rts manual mode
 * @param[in] uart_num - UART0 or UART1
 * @return    none
 */
static inline void uart_rts_manual_mode(uart_num_e uart_num)
{
    reg_uart_ctrl2(uart_num) |= (FLD_UART_RTS_MANUAL_M);
}

#endif /* UART_H_ */
