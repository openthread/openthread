/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup MEMORY
 *
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file qspi_w25q80ew.h
 *
 * @brief QSPI flash driver for the Winbond W25Q80EW
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************
 */

#ifndef _QSPI_W32Q80EW_H_
#define _QSPI_W32Q80EW_H_

#ifndef WINBOND_ID
#define WINBOND_ID      0xEF
#endif

// Device type using command 0x9F
#define W25Q80EW        0x60

#ifndef W25Q_8Mb_SIZE
#define W25Q_8Mb_SIZE   0x14
#endif

#if (FLASH_AUTODETECT == 1) || (dg_configFLASH_MANUFACTURER_ID == WINBOND_ID && \
        dg_configFLASH_DEVICE_TYPE == W25Q80EW && dg_configFLASH_DENSITY == W25Q_8Mb_SIZE)

#include "qspi_common.h"

#include "qspi_winbond.h"

static void flash_w25q80ew_sys_clock_cfg(sys_clk_t sys_clk);
static uint8_t flash_w25q80ew_get_dummy_bytes(void);

static const qspi_flash_config_t flash_w25q80ew_config = {
        .manufacturer_id               = WINBOND_ID,
        .device_type                   = W25Q80EW,
        .device_density                = W25Q_8Mb_SIZE,
        .is_suspended                  = flash_w25q_is_suspended,
        .initialize                    = flash_w25q_initialize,
        .deactivate_command_entry_mode = flash_w25q_deactivate_command_entry_mode,
        .sys_clk_cfg                   = flash_w25q80ew_sys_clock_cfg,
        .get_dummy_bytes               = flash_w25q80ew_get_dummy_bytes,
        .break_seq_size                = HW_QSPI_BREAK_SEQ_SIZE_1B,
        .address_size                  = HW_QSPI_ADDR_SIZE_24,
        .page_program_opcode           = CMD_QUAD_PAGE_PROGRAM,
        .quad_page_program_address     = false,
        .erase_opcode                  = CMD_SECTOR_ERASE,
        .erase_suspend_opcode          = W25Q_ERASE_PROGRAM_SUSPEND,
        .erase_resume_opcode           = W25Q_ERASE_PROGRAM_RESUME,
        .read_erase_progress_opcode    = CMD_READ_STATUS_REGISTER,
        .erase_in_progress_bit         = FLASH_STATUS_BUSY_BIT,
        .erase_in_progress_bit_high_level = true,
        .send_once                     = 1,
        .extra_byte                    = 0xA0,
        .ucode_wakeup                  = {w25q_ucode_wakeup, sizeof(w25q_ucode_wakeup)},
        .power_down_delay              = W25Q_POWER_DOWN_DELAY_US,
        .release_power_down_delay      = W25Q_RELEASE_POWER_DOWN_DELAY_US,
};

#if (FLASH_AUTODETECT == 0)
        const qspi_flash_config_t* const flash_config = &flash_w25q80ew_config;
#endif

__RETAINED_CODE static void flash_w25q80ew_sys_clock_cfg(sys_clk_t sys_clk)
{

}

__RETAINED_CODE static uint8_t flash_w25q80ew_get_dummy_bytes(void)
{
        return 2;
}
#endif

#endif /* _QSPI_W32Q80EW_H_ */
/**
 * \}
 * \}
 * \}
 */
