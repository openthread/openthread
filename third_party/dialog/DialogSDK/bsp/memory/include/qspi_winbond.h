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
 * @file qspi_winbond.h
 *
 * @brief QSPI flash driver for Winbond flashes - common code
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
 *
 ****************************************************************************************
 */

#ifndef _QSPI_WINBOND_H_
#define _QSPI_WINBOND_H_

#define WINBOND_ID                      0xEF

#if (FLASH_AUTODETECT == 1) || (dg_configFLASH_MANUFACTURER_ID == WINBOND_ID)
#include "qspi_common.h"

#define W25Q_ERASE_PROGRAM_SUSPEND      0x75
#define W25Q_ERASE_PROGRAM_RESUME       0x7A

#define W25Q_WRITE_STATUS_REGISTER2     0x31
#define W25Q_PAGE_PROGRAM               0x02
#define W25Q_WRITE_ENABLE_NON_VOL       0x50
#define W25Q_READ_STATUS_REGISTER2      0x35
#define W25Q_BLOCK_ERASE_64K            0xD8
#define W25Q_FAST_READ_QPI              0x0B
#define W25Q_READ_DEVICE_ID_SINGLE      0x90    // Requires single mode for the command entry!
#define W25Q_READ_DEVICE_ID_DUAL        0x92    // Requires dual mode for the command entry!
#define W25Q_READ_DEVICE_ID_QUAD        0x94
#define W25Q_READ_UNIQUE_ID             0x4B    // Requires single mode for the command entry!
#define W25Q_READ_SFDP_REG              0x5A    // Requires single mode for the command entry!
#define W25Q_ERASE_SECURITY_REGS        0x44    // Requires single mode for the command entry!
#define W25Q_PROGR_SECURITY_REGS        0x42    // Requires single mode for the command entry!
#define W25Q_READ_SECURITY_REGS         0x48    // Requires single mode for the command entry!
#define W25Q_ENTER_QPI_MODE             0x38    // Requires single mode for the command entry!
#define W25Q_EXIT_QPI_MODE              0xFF    // Requires quad mode for the command entry!

/* Suspend */
#define W25Q_STATUS2_SUS_BIT            7
#define W25Q_STATUS2_SUS_MASK           (1 << W25Q_STATUS2_SUS_BIT)

/* QPI Enable */
#define W25Q_STATUS2_QE_BIT             1
#define W25Q_STATUS2_QE_MASK            (1 << W25Q_STATUS2_QE_BIT)

// Flash power up/down timings
#define W25Q_POWER_DOWN_DELAY_US          3
#define W25Q_RELEASE_POWER_DOWN_DELAY_US  3
#define W25Q_POWER_UP_DELAY_US            10

#if (dg_configFLASH_POWER_OFF == 1)
/**
 * \brief uCode for handling the QSPI FLASH activation from power off.
 */
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * Delay 10usec
         * 0x01   // CMD_NBYTES = 0, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0xA0   // CMD_WT_CNT_LS = 160 --> 10000 / 62.5 = 160 = 10usec
         * 0x00   // CMD_WT_CNT_MS = 0
         * Exit from Fast Read mode
         * 0x09   // CMD_NBYTES = 1, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0x00   // CMD_WT_CNT_LS = 0
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0xFF   // Enable Reset
         * (up to 16 words)
         */
        const uint32_t w25q_ucode_wakeup[] = {
                0x09000001 | (((uint16_t)(W25Q_POWER_UP_DELAY_US*1000/62.5) & 0xFFFF) << 8),
                0x00FF0000,
        };
#elif (dg_configFLASH_POWER_DOWN == 1)
/**
 * \brief uCode for handling the QSPI FLASH release from power-down.
 */
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * 0x09   // CMD_NBYTES = 1, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0x30   // CMD_WT_CNT_LS = 3000 / 62.5 = 48 // 3usec
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0xAB   // Release Power Down
         * (up to 16 words)
         */
        const uint32_t w25q_ucode_wakeup[] = {
                0xAB000009 | (((uint16_t)(W25Q_RELEASE_POWER_DOWN_DELAY_US*1000/62.5) & 0xFFFF) << 8),
        };
#else
/**
 * \brief uCode for handling the QSPI FLASH exit from the "Continuous Read Mode".
 */
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * 0x25   // CMD_NBYTES = 4, CMD_TX_MD = 2 (Quad), CMD_VALID = 1
         * 0x00   // CMD_WT_CNT_LS = 0
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0x55   // Clocks 0-1 (A23-16)
         * 0x55   // Clocks 2-3 (A15-8)
         * 0x55   // Clocks 4-5 (A7-0)
         * 0x55   // Clocks 6-7 (M7-0) : M5-4 != '10' ==> Disable "Continuous Read Mode"
         * (up to 16 words)
         */
        const uint32_t w25q_ucode_wakeup[] = {
                0x55000025,
                0x00555555,
        };
#endif

static bool flash_w25q_is_suspended(void);
static void flash_w25q_initialize(uint8_t device_type, uint8_t device_density);
static void flash_w25q_deactivate_command_entry_mode(void);

/**
 * \brief Enable volatile writes to Status Register bits
 * \details When this command is issued, any writes to any of the Status Registers of the Flash are
 *        done as volatile writes. This command is valid only when the Write Status Register command
 *        follows.
 *
 * \note This function blocks until the Flash has processed the command.
 */
static inline void flash_w25q_wre_volatile(void) __attribute__((always_inline)) __attribute__((unused));
static inline void flash_w25q_wre_volatile(void)
{
        uint8_t cmd[] = { W25Q_WRITE_ENABLE_NON_VOL };

        qspi_write(cmd, 1);

        /* Verify */
        while (flash_is_busy());
}

static inline uint8_t flash_w25q_read_status_register_2(void) __attribute__((always_inline));
static inline uint8_t flash_w25q_read_status_register_2(void)
{
        __DBG_QSPI_VOLATILE__ uint8_t status;
        uint8_t cmd[] = { W25Q_READ_STATUS_REGISTER2 };

        qspi_transact(cmd, 1, &status, 1);

        return status;
}

/**
 * \brief Write the Status Register 2 of the Flash
 *
 * \param[in] value The value to be written.
 *
 * \note This function blocks until the Flash has processed the command. No verification that the
 *        value has been actually written is done though. It is up to the caller to decide whether
 *        such verification is needed or not and execute it on its own.
 */
static inline void flash_w25q_write_status_register_2(uint8_t value) __attribute__((always_inline));
static inline void flash_w25q_write_status_register_2(uint8_t value)
{
        uint8_t cmd[] = { W25Q_WRITE_STATUS_REGISTER2, value };

        qspi_write(cmd, 2);

        /* Wait for the Flash to process the command */
        while (flash_is_busy());
}

static inline void flash_w25q_enable_quad_mode(void) __attribute__((always_inline));
static inline void flash_w25q_enable_quad_mode(void)
{
        uint8_t status;

        status = flash_w25q_read_status_register_2();
        if (!(status & W25Q_STATUS2_QE_MASK)) {
                flash_write_enable();
                flash_w25q_write_status_register_2(status | W25Q_STATUS2_QE_MASK);
        }
}

__RETAINED_CODE static bool flash_w25q_is_suspended(void)
{
        __DBG_QSPI_VOLATILE__ uint8_t status;

        status = flash_w25q_read_status_register_2();
        return (status & W25Q_STATUS2_SUS_MASK) != 0;
}

__RETAINED_CODE static void flash_w25q_initialize(uint8_t device_type, uint8_t device_density)
{
        flash_activate_command_entry_mode();

        flash_w25q_enable_quad_mode();

        flash_deactivate_command_entry_mode();
}

__RETAINED_CODE static void flash_w25q_deactivate_command_entry_mode(void)
{
}

#endif

#endif /* _QSPI_WINBOND_H_ */
/**
 * \}
 * \}
 * \}
 */
