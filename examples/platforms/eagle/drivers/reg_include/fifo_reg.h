/********************************************************************************************************
 * @file     fifo_reg.h
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
#ifndef FIFO_REG_H
#define FIFO_REG_H

#include "../sys.h"
#include "../../common/bit.h"



/*******************************      fifo registers: 0x800      ******************************/

#define reg_fifo0_data			REG_ADDR32(0x800)
#define reg_fifo1_data			REG_ADDR32(0x900)
#define reg_fifo2_data			REG_ADDR32(0xa00)

/*******************************      dfifo registers: 0xb00      ******************************/

#define reg_dfifo0_addr			REG_ADDR16(0xb00)
#define reg_dfifo0_size			REG_ADDR8(0xb02)
#define reg_dfifo0_addHi		REG_ADDR8(0xb03)  //default 0x04, no need set

#define reg_dfifo1_addr			REG_ADDR16(0xb04)
#define reg_dfifo1_size			REG_ADDR8(0xb06)
#define reg_dfifo1_addHi		REG_ADDR8(0xb07)  //default 0x04, no need set

//misc channel only use dfifo2
#define reg_dfifo2_addr			REG_ADDR16(0xb08)
#define reg_dfifo2_size			REG_ADDR8(0xb0a)
#define reg_dfifo2_addHi		REG_ADDR8(0xb0b)  //default 0x04, no need set

#define reg_dfifo_audio_addr		reg_dfifo0_addr
#define reg_dfifo_audio_size		reg_dfifo0_size

#define reg_dfifo_misc_chn_addr		reg_dfifo2_addr
#define reg_dfifo_misc_chn_size		reg_dfifo2_size


#define reg_dfifo0_l_level		REG_ADDR8(0xb0c)  //dfifo0  low int threshold(wptr - rptr)
#define reg_dfifo0_h_level		REG_ADDR8(0xb0d)  //dfifo0 high int threshold(wptr - rptr)
#define reg_dfifo1_h_level		REG_ADDR8(0xb0e)  //dfifo1 high int threshold(wptr - rptr)
#define reg_dfifo2_h_level		REG_ADDR8(0xb0f)  //dfifo2 high int threshold(wptr - rptr)


#define	reg_dfifo_mode	REG_ADDR8(0xb10)
enum{
	FLD_AUD_DFIFO0_IN 		= BIT(0),
	FLD_AUD_DFIFO1_IN 		= BIT(1),
	FLD_AUD_DFIFO2_IN 		= BIT(2),
	FLD_AUD_DFIFO0_OUT 		= BIT(3),
	FLD_AUD_DFIFO0_L_INT	= BIT(4),
	FLD_AUD_DFIFO0_H_INT	= BIT(5),
	FLD_AUD_DFIFO1_H_INT	= BIT(6),
	FLD_AUD_DFIFO2_H_INT	= BIT(7),
};

#define	reg_dfifo_ain			REG_ADDR8(0xb11)
enum{
	FLD_AUD_SAMPLE_TIME_CONFIG        = BIT_RNG(0,1),
	FLD_AUD_FIFO0_INPUT_SELECT     	  = BIT_RNG(2,3),
	FLD_AUD_FIFO1_INPUT_SELECT     	  = BIT_RNG(4,5),
	FLD_AUD_MIC_LEFT_CHN_SELECT       = BIT(6),
	FLD_AUD_MIC_RIGHT_CHN_SELECT      = BIT(7),
};

enum{  //core_b11<3:2>  audio input select
	AUDIO_FIFO0_INPUT_SELECT_USB 	        = 0x00,
	AUDIO_FIFO0_INPUT_SELECT_I2S 	        = 0x01,
	AUDIO_FIFO0_INPUT_SELECT_16BIT 	        = 0x02,
	AUDIO_FIFO0_INPUT_SELECT_20BIT 	        = 0x03,
};

enum{  //core_b11<5:4>  audio input select
	AUDIO_FIFO1_INPUT_SELECT_USB 	        = 0x00,
	AUDIO_FIFO1_INPUT_SELECT_I2S 	        = 0x01,
	AUDIO_FIFO1_INPUT_SELECT_16BIT 	        = 0x02,
	AUDIO_FIFO1_INPUT_SELECT_20BIT 	        = 0x03,
};


#define	reg_mic_ctrl			REG_ADDR8(0xb12)
enum{
	FLD_AUD_MIC_VOL_CONTROL           = BIT_RNG(0,5),
	FLD_AUD_MIC_MONO_EN               = BIT(6),
	FLD_AUD_AMIC_DMIC_SELECT          = BIT(7),
};

enum{
	MIC_VOL_CONTROL_m48DB = 		0x00,
	MIC_VOL_CONTROL_m42DB = 		0x04,
	MIC_VOL_CONTROL_m36DB = 		0x08,
	MIC_VOL_CONTROL_m30DB = 		0x0c,
	MIC_VOL_CONTROL_m24DB = 		0x10,
	MIC_VOL_CONTROL_m18DB = 		0x14,
	MIC_VOL_CONTROL_m16DB = 		0x15,
	MIC_VOL_CONTROL_m12DB = 		0x18,
	MIC_VOL_CONTROL_m6DB  = 	   	0x1c,
	MIC_VOL_CONTROL_0DB   = 	    0x20,
	MIC_VOL_CONTROL_6DB   =  		0x24,
	MIC_VOL_CONTROL_12DB  = 		0x28,
	MIC_VOL_CONTROL_18DB  = 		0x2c,
	MIC_VOL_CONTROL_24DB  = 		0x30,
	MIC_VOL_CONTROL_30DB  = 		0x34,
	MIC_VOL_CONTROL_36DB  = 		0x38,
	MIC_VOL_CONTROL_42DB  =     	0x3c,
};


#define reg_set_filter_para     REG_ADDR8(0xb80)

#define reg_dfifo_dec_ratio	    REG_ADDR8(0xb8a)

#define reg_codec_dec_en	    REG_ADDR8(0xb8b)
enum{
	FLD_AUD_CODEC_DEC_EN	=   BIT(0),
};

#define reg_dfifo_irq_status	REG_ADDR8(0xb13)
enum{
	FLD_AUD_DFIFO0_L_IRQ	= BIT(4),
	FLD_AUD_DFIFO0_H_IRQ	= BIT(5),
	FLD_AUD_DFIFO1_L_IRQ	= BIT(6),
	FLD_AUD_DFIFO1_H_IRQ	= BIT(7),
};
#define reg_dfifo0_rptr			REG_ADDR16(0xb14)
#define reg_dfifo0_wptr			REG_ADDR16(0xb16)

#define reg_dfifo1_rptr			REG_ADDR16(0xb18)
#define reg_dfifo1_wptr			REG_ADDR16(0xb1a)

#define reg_dfifo2_rptr			REG_ADDR16(0xb1c)
#define reg_dfifo2_wptr			REG_ADDR16(0xb1e)


#define reg_audio_wptr			reg_dfifo0_wptr

#define reg_dfifo0_num			REG_ADDR16(0xb20)
#define reg_dfifo1_num			REG_ADDR16(0xb24)
#define reg_dfifo2_num			REG_ADDR16(0xb28)

#define reg_dfifo_manual_mode		REG_ADDR8(0xb2c)
enum{
	FLD_DFIFO_MANUAL_MODE_EN	= BIT(0),
};

#define reg_dfifo0_man_dat		REG_ADDR32(0xb30)



#endif
