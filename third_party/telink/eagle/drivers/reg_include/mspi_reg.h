/********************************************************************************************************
 * @file	mspi_reg.h
 *
 * @brief	This is the header file for B91
 *
 * @author	Z.W.H
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
#pragma once

#include "../sys.h"
#include "../../common/bit.h"

/*******************************      MSPI registers: 0x140100      ******************************/
#define reg_mspi_data       REG_ADDR8(0x140100)

#define reg_mspi_fm       	REG_ADDR8(0x140101)
enum{
	FLD_MSPI_RD_TRIG_EN		=  BIT(0),
	FLD_MSPI_RD_MODE		=  BIT(1),
	FLD_MSPI_DATA_LINE		=  BIT_RNG(2,3),
	FLD_MSPI_CSN			=  BIT(4),
};

#define reg_mspi_status     REG_ADDR8(0x140102)
enum{
	FLD_MSPI_BUSY			=  BIT(0),
};

#define reg_mspi_fm1		REG_ADDR8(0x140103)
enum{
	FLD_MSPI_TIMEOUT_CNT	=  BIT_RNG(0,2),
	FLD_MSPI_CS2SCL_CNT		=  BIT_RNG(3,4),
	FLD_MSPI_CS2CS_CNT		=  BIT_RNG(5,7),
};

#define reg_mspi_set_l		REG_ADDR8(0x140104)
enum{
	FLD_MSPI_MULTIBOOT_ADDR_OFFSET	=  BIT_RNG(0,2),
};

#define reg_mspi_set_h   	REG_ADDR8(0x140105)
enum{
	FLD_MSPI_PROGRAM_SPACE_SIZE		=  BIT_RNG(0,6),
};

#define reg_mspi_cmd_ahb   	REG_ADDR8(0x140106)
enum{
	FLD_MSPI_RD_CMD			=  BIT_RNG(0,7),
};

#define reg_mspi_fm_ahb   	REG_ADDR8(0x140107)
enum{
	FLD_MSPI_DUMMY			=  BIT_RNG(0,3),
	FLD_MSPI_DAT_LINE		=  BIT_RNG(4,5),
	FLD_MSPI_ADDR_LINE		=  BIT(6),
	FLD_MSPI_CMD_LINE		=  BIT(7),
};


