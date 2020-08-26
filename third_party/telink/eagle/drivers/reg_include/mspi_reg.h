/********************************************************************************************************
 * @file     mspi_reg.h
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
#ifndef MSPI_REG_H
#define MSPI_REG_H
#include "../../common/bit.h"
#include "../sys.h"
/*******************************      MSPI registers: 0x140100      ******************************/
#define reg_mspi_data REG_ADDR8(0x140100)
#define reg_mspi_fm REG_ADDR8(0x140101)
enum
{
    FLD_MSPI_RD_TRIG_EN = BIT(0),
    FLD_MSPI_RD_MODE    = BIT(1),
    FLD_MSPI_DATA_LINE  = BIT_RNG(2, 3),
    FLD_MSPI_CSN        = BIT(4),
};
#define reg_mspi_status REG_ADDR8(0x140102)
enum
{
    FLD_MSPI_BUSY = BIT(0),
};

#define reg_mspi_fm1 REG_ADDR8(0x140103)
enum
{
    FLD_MSPI_TIMEOUT_CNT = BIT_RNG(0, 2),
    FLD_MSPI_CS2SCL_CNT  = BIT_RNG(3, 4),
    FLD_MSPI_CS2CS_CNT   = BIT_RNG(5, 7),
};

#define reg_mspi_set_l REG_ADDR8(0x140104)
enum
{
    FLD_MSPI_MULTIBOOT_ADDR_OFFSET = BIT_RNG(0, 2),
};

#define reg_mspi_set_h REG_ADDR8(0x140105)
enum
{
    FLD_MSPI_PROGRAM_SPACE_SIZE = BIT_RNG(0, 6),
};

#define reg_mspi_cmd_ahb REG_ADDR8(0x140106)
enum
{
    FLD_MSPI_RD_CMD = BIT_RNG(0, 7),
};

#define reg_mspi_fm_ahb REG_ADDR8(0x140107)
enum
{
    FLD_MSPI_DUMMY     = BIT_RNG(0, 3),
    FLD_MSPI_DAT_LINE  = BIT_RNG(4, 5),
    FLD_MSPI_ADDR_LINE = BIT(6),
    FLD_MSPI_CMD_LINE  = BIT(7),
};

#endif
