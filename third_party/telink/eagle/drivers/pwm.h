/********************************************************************************************************
 * @file	pwm.h
 *
 * @brief	This is the header file for B91
 *
 * @author	D.M.H / Z.S.X
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
#ifndef PWM_H_
#define PWM_H_
#include "gpio.h"
#include "dma.h"
#include "reg_include/register_b91.h"

#define get_pwmid(gpio)     ((gpio==PWM_PWM0_PB4) ? 0 : (  \
                     (gpio==PWM_PWM0_PC0) 	? 0 : (  \
                     (gpio==PWM_PWM0_PE3) 	? 0 : (  \
                     (gpio==PWM_PWM0_N_PD0) ? 0 : (  \
                     (gpio==PWM_PWM1_PB5) 	? 1 : (  \
                     (gpio==PWM_PWM1_PE1) 	? 1 : (  \
                     (gpio==PWM_PWM1_N_PD1) ? 1 : (  \
                     (gpio==PWM_PWM2_PB7) 	? 2 : (  \
                     (gpio==PWM_PWM2_PE2) 	? 2 : (  \
                     (gpio==PWM_PWM2_N_PD2) ? 2 : (  \
                     (gpio==PWM_PWM2_N_PE6) ? 2 : (  \
                     (gpio==PWM_PWM3_PB1) 	? 3 : (  \
                     (gpio==PWM_PWM3_PE0) 	? 3 : (  \
                     (gpio==PWM_PWM3_N_PD3) ? 3 : (  \
                     (gpio==PWM_PWM3_N_PE7) ? 3 : (  \
                     (gpio==PWM_PWM4_PD7) 	? 4 : (  \
                     (gpio==PWM_PWM4_PE4) 	? 4 : (  \
                     (gpio==PWM_PWM4_N_PD4) ? 4 : (  \
                     (gpio==PWM_PWM5_PB0) 	? 5 : (  \
                     (gpio==PWM_PWM5_PE5) 	? 5 : (  \
                     (gpio==PWM_PWM5_N_PD5) ? 5 : 0  \
                    		 )))))))))))))))))))))

#define get_pwm_invert_val(gpio)     ((gpio==PWM_PWM0_N_PD0) ||    \
                     (gpio==PWM_PWM1_N_PD1) ||        \
                     (gpio==PWM_PWM2_N_PD2) ||        \
                     (gpio==PWM_PWM2_N_PE6) ||        \
                     (gpio==PWM_PWM3_N_PD3) ||        \
                     (gpio==PWM_PWM3_N_PE7) ||        \
                     (gpio==PWM_PWM4_N_PD4) ||        \
                     (gpio==PWM_PWM5_N_PD5))

/**
 * @brief  enum variable, the number of PWM channels supported
 */
typedef enum {
	PWM0_ID = 0,
	PWM1_ID,
	PWM2_ID,
	PWM3_ID,
	PWM4_ID,
	PWM5_ID,
}pwm_id_e;


/**
 * @brief  enum variable, represents the PWM mode.
 */
typedef enum{
	PWM_NORMAL_MODE   		= 0x00,
	PWM_COUNT_MODE    		= 0x01,
	PWM_IR_MODE       		= 0x03,
	PWM_IR_FIFO_MODE  		= 0x07,
	PWM_IR_DMA_FIFO_MODE  	= 0x0F,
}pwm_mode_e;


/**
 * @brief  enum variable, represents Selection of PWM pin.
 */
typedef enum{
	PWM_PWM0_PB4 = GPIO_PB4,
	PWM_PWM0_PC0 = GPIO_PC0,
	PWM_PWM0_PE3 = GPIO_PE3,
	PWM_PWM0_N_PD0 = GPIO_PD0,

	PWM_PWM1_PB5 = GPIO_PB5,
	PWM_PWM1_PE1 = GPIO_PE1,
	PWM_PWM1_N_PD1 = GPIO_PD1,

	PWM_PWM2_PB7 = GPIO_PB7,
	PWM_PWM2_PE2 = GPIO_PE2,
	PWM_PWM2_N_PD2 = GPIO_PD2,
	PWM_PWM2_N_PE6 = GPIO_PE6,

	PWM_PWM3_PB1 = GPIO_PB1,
	PWM_PWM3_PE0 = GPIO_PE0,
	PWM_PWM3_N_PD3 = GPIO_PD3,
	PWM_PWM3_N_PE7 = GPIO_PE7,

	PWM_PWM4_PD7 = GPIO_PD7,
	PWM_PWM4_PE4 = GPIO_PE4,
	PWM_PWM4_N_PD4 = GPIO_PD4,

	PWM_PWM5_PB0 = GPIO_PB0,
	PWM_PWM5_PE5 = GPIO_PE5,
	PWM_PWM5_N_PD5 = GPIO_PD5,
}pwm_pin_e;




/**
 * @brief  Select the 32K clock source of pwm.
 */
typedef enum {

	PWM_CLOCK_32K_CHN_NONE = 0x00,
	PWM_CLOCK_32K_CHN_PWM0 = 0x01,
	PWM_CLOCK_32K_CHN_PWM1 = 0x02,
	PWM_CLOCK_32K_CHN_PWM2 = 0x04,
	PWM_CLOCK_32K_CHN_PWM3 = 0x08,
	PWM_CLOCK_32K_CHN_PWM4 = 0x10,
	PWM_CLOCK_32K_CHN_PWM5 = 0x20

}pwm_clk_32k_en_chn_e;






/**
 * @brief     This function servers to set pwm clock frequency, when pwm clock source is pclk.
 * @param[in] pwm_clk_div - variable of the pwm clock.
 *            PWM frequency = System_clock / (pwm_clk_div+1).
 * @return	  none.
 */
static inline void pwm_set_clk(unsigned char pwm_clk_div){

    reg_pwm_clkdiv = pwm_clk_div;
}

/**
 * @brief     This function servers to set pwm clock source is 32K.
 *    	      If you use 32K as the PWM clock source, you can work in suspend mode.
 *    	      but the 32K clock source cannot be divided and can only work in continuous mode and counting mode
 * @param[in] pwm_32K_en_chn - This variable is used to select the specific PWM channel.
 * @return	  none.
 */

static inline void pwm_32k_chn_en(pwm_clk_32k_en_chn_e pwm_32K_en_chn){

	reg_pwm_mode32k= pwm_32K_en_chn;

}


/**
 * @brief     This fuction servers to set pin as pwm0
 * @param[in] pin - selected pin
 * @return	  none.
 */
void pwm_set_pin(pwm_pin_e pin);




/**
 * @brief     This fuction servers to set pwm count status(CMP) time.
 * @param[in] id   - variable of enum to select the pwm number.
 * @param[in] tcmp - variable of the CMP.
 * @return	  none.
 */
static inline void pwm_set_tcmp(pwm_id_e id, unsigned short tcmp)
{
	reg_pwm_cmp(id) = tcmp;
}


/**
 * @brief     This fuction servers to set pwm cycle time.
 * @param[in] id   - variable of enum to select the pwm number.
 * @param[in] tmax - variable of the cycle time.
 * @return	  none.
 */
static inline void pwm_set_tmax(pwm_id_e id, unsigned short tmax){
	reg_pwm_max(id) = tmax;
}


/**
 * @brief     This fuction servers to start the pwm.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_start(pwm_id_e id){
	if(PWM0_ID == id){
		BM_SET(reg_pwm0_enable, BIT(0));
	}
	else{
		BM_SET(reg_pwm_enable, BIT(id));
	}
}


/**
 * @brief     This fuction servers to stop the pwm.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_stop(pwm_id_e id){
	if(PWM0_ID == id){
		BM_CLR(reg_pwm0_enable, BIT(0));
	}
	else{
		BM_CLR(reg_pwm_enable, BIT(id));
	}
}


/**
 * @brief     This fuction servers to revert the PWMx.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_invert_en(pwm_id_e id){
	reg_pwm_invert |= BIT(id);
}


/**
 * @brief     This fuction servers to disable the PWM revert function.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_invert_dis(pwm_id_e id){
	BM_CLR(reg_pwm_invert, BIT(id));
}


/**
 * @brief     This fuction servers to revert the PWMx_N.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_n_invert_en(pwm_id_e id){
	reg_pwm_n_invert |= BIT(id);
}


/**
 * @brief     This fuction servers to disable the PWM revert function.
 * @param[in] id - variable of enum to select the pwm number.
 * @return	  none.
 */
static inline void pwm_n_invert_dis(pwm_id_e id){
	BM_CLR(reg_pwm_n_invert, BIT(id));
}


/**
 * @brief     This fuction servers to enable the pwm polarity.
 * @param[in] id - variable of enum to select the pwm number.
 * @param[in] en: 1 enable. 0 disable.
 * @return	  none.
 */
static inline void pwm_set_polarity_en(pwm_id_e id){
		BM_SET(reg_pwm_pol, BIT(id));
}


/**
 * @brief     This fuction servers to disable the pwm polarity.
 * @param[in] id - variable of enum to select the pwm number.
 * @param[in] en: 1 enable. 0 disable.
 * @return	  none.
 */
static inline void pwm_set_polarity_dis(pwm_id_e id){
		BM_CLR(reg_pwm_pol, BIT(id));
}


/**
 * @brief     This fuction servers to enable the pwm interrupt.
 * @param[in] mask - variable of enum to select the pwm interrupt source.
 * @return	  none.
 */
static inline void pwm_set_irq_mask(pwm_irq_e mask){

	if(mask==FLD_PWM0_IR_FIFO_IRQ)
	{
		BM_SET(reg_pwm_irq_mask(1), BIT(0));
	}
	else
	{
		BM_SET(reg_pwm_irq_mask(0), mask);
	}

}


/**
 * @brief     This fuction servers to disable the pwm interrupt function.
 * @param[in] mask - variable of enum to select the pwm interrupt source.
 * @return	  none.
 */
static inline void pwm_clr_irq_mask(pwm_irq_e mask){

	if(mask==FLD_PWM0_IR_FIFO_IRQ)
	{
		BM_SET(reg_pwm_irq_mask(1), mask>>15);
	}
	else
	{
		BM_SET(reg_pwm_irq_mask(0), mask);
	}

}


/**
 * @brief     This fuction servers to get the pwm interrupt status.
 * @param[in] status - variable of enum to select the pwm interrupt source.
 * @return	  none.
 */
static inline u8 pwm_get_irq_status(pwm_irq_e status){

	if(status==FLD_PWM0_IR_FIFO_IRQ)
	{
		return (reg_pwm_irq_sta(1) & BIT(0));
	}
	else
	{
		return (reg_pwm_irq_sta(0) & status);
	}

}


/**
 * @brief     This fuction servers to clear the pwm interrupt.When a PWM interrupt occurs, the corresponding interrupt flag bit needs to be cleared manually.
 * @param[in] status  - variable of enum to select the pwm interrupt source.
 * @return	  none.
 */
static inline void pwm_clr_irq_status(pwm_irq_e status){

	if(status==FLD_PWM0_IR_FIFO_IRQ)
	{
		BM_SET(reg_pwm_irq_sta(1), BIT(0));
	}
	else
	{
		BM_SET(reg_pwm_irq_sta(0), status);
	}

}


/**
 * @brief     This fuction servers to set pwm0 mode.
 * @param[in] mode - variable of enum to indicates the pwm mode.
 * @return	  none.
 */
static inline void pwm_set_pwm0_mode(pwm_mode_e  mode){
		reg_pwm0_mode = mode;  //only PWM0 has count/IR/fifo IR mode
}


/**
 * @brief     This fuction servers to set pwm cycle time & count status.
 * @param[in] max_tick - variable of the cycle time.
 * @param[in] cmp_tick - variable of the CMP.
 * @return	  none.
 */
static inline void pwm_set_pwm0_tcmp_and_tmax_shadow(unsigned short max_tick, unsigned short cmp_tick)
{
	reg_pwm_tcmp0_shadow = cmp_tick;
	reg_pwm_tmax0_shadow = max_tick;
}


/**
 * @brief     This fuction servers to set the pwm0 pulse number.
 * @param[in] pulse_num - variable of the pwm pulse number.The maximum bits is 14bits.
 * @return	  none.
 */
static inline void pwm_set_pwm0_pulse_num(unsigned short pulse_num){
		reg_pwm0_pulse_num0 = pulse_num;
		reg_pwm0_pulse_num1 = pulse_num>>8;
}


/**
 * @brief     This fuction serves to set trigger level of interrupt for IR FiFo mode
 * @param[in] trig_level - FIFO  num int trigger level.When fifo numbers is less than this value.It's will take effect.
 * @return	  none
 */
static inline void pwm_set_pwm0_ir_fifo_irq_trig_level(unsigned char trig_level)
{
	reg_pwm_ir_fifo_irq_trig_level = trig_level;
}


/**
 * @brief     This fuction serves to clear data in fifo. Only when pwm is in not active mode,
 * 			  it is possible to clear data in fifo.
 * @return	  none
 */
static inline void pwm_clr_pwm0_ir_fifo(void)
{
	reg_pwm_ir_clr_fifo_data |= FLD_PWM0_IR_FIFO_CLR_DATA;
}


/**
 * @brief     This fuction serves to get the number of data in fifo.
 * @return	  the number of data in fifo
 */
static inline unsigned char pwm_get_pwm0_ir_fifo_data_num(void)//????TODO
{
	return (reg_pwm_ir_fifo_data_status & FLD_PWM0_IR_FIFO_DATA_NUM);
}


/**
 * @brief     This fuction serves to determine whether data in fifo is empty.
 * @return	  yes: 1 ,no: 0;
 */
static inline unsigned char pwm_get_pwm0_ir_fifo_is_empty(void)
{
	return (reg_pwm_ir_fifo_data_status & FLD_PWM0_IR_FIFO_EMPTY);
}


/**
 * @brief     This fuction serves to determine whether data in fifo is full.
 * @return	  yes: 1 ,no: 0;
 */
static inline unsigned char pwm_get_pwm0_ir_fifo_is_full(void)
{
	return (reg_pwm_ir_fifo_data_status&FLD_PWM0_IR_FIFO_FULL);
}


/**
 * @brief     This fuction serves to config the pwm's dma wave form.
 * @param[in] pulse_num - the number of pulse.
 * @param[in] shadow_en - whether enable shadow mode.
 * @param[in] carrier_en - must 1 or 0.
 * @return	  none.
 */
static inline unsigned short pwm_cal_pwm0_ir_fifo_cfg_data(unsigned short pulse_num, unsigned char shadow_en, unsigned char carrier_en)
{
	return  ( carrier_en<<15 | (shadow_en<<14) | (pulse_num & 0x3fff) );
}


/**
 * @brief     This fuction serves to write data into FiFo
 * @param[in] pulse_num  - the number of pulse
 * @param[in] use_shadow - determine whether the configuration of shadow cmp and shadow max is used
 * 						   1: use shadow, 0: not use
 * @param[in] carrier_en - enable sending carrier, 1: enable, 0: disable
 * @return	  none
 */
static inline void pwm_set_pwm0_ir_fifo_cfg_data(unsigned short pulse_num, unsigned char use_shadow, unsigned char carrier_en)
{
	static unsigned char index=0;
	unsigned short cfg_data = pwm_cal_pwm0_ir_fifo_cfg_data(pulse_num,use_shadow,carrier_en);
	while(pwm_get_pwm0_ir_fifo_is_full());
	reg_pwm_ir_fifo_dat(index) = cfg_data;
	index++;
	index&=0x01;
}


/**
 * @brief     This function servers to configure DMA channel and some configures.
 * @param[in] chn - to select the DMA channel.
 * @return    none
 */
void pwm_set_dma_config(dma_chn_e chn);


/**
 * @brief     This function servers to configure DMA channel address and length.
 * @param[in] chn - to select the DMA channel.
 * @param[in] buf_addr - the address where DMA need to get data from SRAM.
 * @param[in] len - the length of data in SRAM.
 * @return    none
 */
void pwm_set_dma_buf(dma_chn_e chn,u32 buf_addr,u32 len);


/**
 * @brief     This function servers to enable DMA channel.
 * @param[in] chn - to select the DMA channel.
 * @return    none
 */
void pwm_ir_dma_mode_start(dma_chn_e chn);



/**
 * @brief     This function servers to configure DMA head node.
 * @param[in] chn - to select the DMA channel.
 * @param[in] src_addr - to configure DMA source address.
 * @param[in] data_len - to configure DMA length.
 * @param[in] head_of_list - to configure the address of the next node configure.
 * @return    none
 */
void pwm_set_dma_chain_llp(dma_chn_e chn,u16 * src_addr, u32 data_len,dma_chain_config_t * head_of_list);




/**
 * @brief     This function servers to configure DMA cycle chain node.
 * @param[in] chn - to select the DMA channel.
 * @param[in] config_addr  - to servers to configure the address of the current node.
 * @param[in] llponit - to configure the address of the next node configure.
 * @param[in] src_addr - to configure DMA source address.
 * @param[in] data_len - to configure DMA length.
 * @return    none
 */
void pwm_set_tx_dma_add_list_element(dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llponit ,u16 * src_addr,u32 data_len);



#endif





