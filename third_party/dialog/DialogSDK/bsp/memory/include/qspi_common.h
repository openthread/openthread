/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup MEMORY
 *
 * \brief QSPI flash driver framework
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file qspi_common.h
 *
 * @brief QSPI flash driver common definitions
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

#ifndef _QSPI_COMMON_H_
#define _QSPI_COMMON_H_

#include "qspi_automode.h"

/*
 * Flash Commands
 *
 * Note: Default command issuing mode is single mode! If commands specific to other modes have to be
 * issued then the mode must be changed!
 */
#define CMD_WRITE_STATUS_REGISTER       0x01
#define CMD_WRITE_DISABLE               0x04
#define CMD_READ_STATUS_REGISTER        0x05
#define CMD_WRITE_ENABLE                0x06
#define CMD_SECTOR_ERASE                0x20
#define CMD_QUAD_PAGE_PROGRAM           0x32
#define CMD_QUAD_IO_PAGE_PROGRAM        0x38
#define CMD_BLOCK_ERASE                 0x52
#define CMD_CHIP_ERASE                  0xC7
#define CMD_FAST_READ_QUAD              0xEB
#define CMD_READ_JEDEC_ID               0x9F
#define CMD_EXIT_CONTINUOUS_MODE        0xFF
#define CMD_RELEASE_POWER_DOWN          0xAB
#define CMD_ENTER_POWER_DOWN            0xB9

#define CMD_FAST_READ_QUAD_4B           0xEC
#define CMD_SECTOR_ERASE_4B             0x21
#define CMD_QUAD_PAGE_PROGRAM_4B        0x34
#define CMD_QUAD_IO_PAGE_PROGRAM_4B     0x3E

/* Erase/Write in progress */
#define FLASH_STATUS_BUSY_BIT           0
#define FLASH_STATUS_BUSY_MASK          (1 << FLASH_STATUS_BUSY_BIT)

/* WE Latch bit */
#define FLASH_STATUS_WEL_BIT            1
#define FLASH_STATUS_WEL_MASK           (1 << FLASH_STATUS_WEL_BIT)

typedef bool (*is_suspended_cb_t)(void);
typedef void (*initialize_cb_t)(uint8_t device_type, uint8_t device_density);
typedef void (*deactivate_command_entry_mode_cb_t)(void);
typedef void (*sys_clk_cfg_cb_t)(sys_clk_t type);
typedef uint8_t (*get_dummy_bytes_cb_t)(void);

/**
 * \brief QSPI Flash configuration structure
 *
 * This struct is used to define a driver for a specific QSPI flash.
 *
 * \note The struct instance must be declared as static const for this to work
 */
typedef struct qspi_flash_config {
        // Function pointers
        initialize_cb_t initialize;            /**< Flash-specific initialization */
        is_suspended_cb_t is_suspended;        /**< Check if flash is in erase/program
                                                    suspend state */
        deactivate_command_entry_mode_cb_t
                deactivate_command_entry_mode; /**< Perform extra steps needed when command
                                                    entry mode is deactivated */
        sys_clk_cfg_cb_t sys_clk_cfg;          /**< Perform Flash configuration when system clock
                                                    is changed (e.g. change dummy bytes or QSPIC
                                                    clock divider */
        get_dummy_bytes_cb_t get_dummy_bytes;  /**< Return the number of dummy bytes currently
                                                    needed (they may e.g. change when the clock
                                                    changes) */
        uint8_t manufacturer_id;               /**< The Flash JEDEC vendor ID (Cmd 0x9F, 1st byte)
                                                    This (and the device_type/device_density)
                                                    are needed for flash autodetection */
        uint8_t device_type;                   /**< The Flash JEDEC device type (Cmd 0x9F, 2nd byte) */
        uint8_t device_density;                /**< The Flash JEDEC device type (Cmd 0x9F, 3rd byte) */
        uint8_t erase_opcode;                  /**< The Flash erase opcode to use */
        uint8_t erase_suspend_opcode;          /**< The Flash erase suspend opcode to use */
        uint8_t erase_resume_opcode;           /**< The Flash erase resume opcode to use */
        uint8_t page_program_opcode;           /**< The Flash page program opcode to use */
        bool quad_page_program_address;        /**< If true, the address will be transmitted in
                                                    QUAD mode when writing a page. Otherwise, it
                                                    will be transmitted in single mode */
        uint8_t read_erase_progress_opcode;    /**< The opcode to use to check if erase is in progress
                                                    (Usually the Read Status Reg opcode (0x5)) */

        uint8_t erase_in_progress_bit;         /**< The bit to check when reading the erase progress */
        bool erase_in_progress_bit_high_level; /**< The active state (true: high, false: low) of the bit
                                                    above */
        uint8_t send_once;                     /**< If set to 1, the "Performance mode" (or burst, or
                                                    continuous; differs per vendor) will be used for read
                                                    accesses. In this mode, the read opcode is only sent
                                                    once, and subsequent accesses only transfer the address */
        uint8_t extra_byte;                    /**< The extra byte to transmit, when in "Performance mode"
                                                    (send once is 1), that tells the flash that it should
                                                    stay in this continuous, performance mode */
        HW_QSPI_ADDR_SIZE address_size;        /**< Whether the flash works in 24- or 32-bit addressing mode */
        HW_QSPI_BREAK_SEQ_SIZE break_seq_size; /**< Whether the break sequence, that puts flash out of the
                                                    continuous mode, is one or two bytes long (the break byte is
                                                    0xFF) */
        qspi_ucode_t ucode_wakeup;             /**< The QSPIC microcode to use to setup the flash on wakeup. This
                                                    is automatically used by the QSPI Controller after wakeup, and
                                                    before CPU starts code execution. This is different based on
                                                    whether flash was active, in deep power down or completely off
                                                    while the system was sleeping */
        uint16_t power_down_delay;             /**< This is the time, in usec, needed for the flash to go to power
                                                    down, after the Power Down command is issued */
        uint16_t release_power_down_delay;     /**< This is the time, in usec, needed for the flash to exit the power
                                                    down mode, after the Release Power Down command is issued */
} qspi_flash_config_t;

extern const qspi_flash_config_t* flash_config_init;
#if (FLASH_AUTODETECT == 1)
extern qspi_flash_config_t *flash_config;
#else
extern const qspi_flash_config_t * const flash_config;
#endif

static void qspi_write(const uint8_t *wbuf, size_t wlen);
static void qspi_transact(const uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen);
static uint8_t flash_read_status_register(void);
static void flash_write_status_register(uint8_t value);
static void flash_write_enable(void);
static void qspi_automode_set_dummy_bytes_count(uint8_t count);
static inline bool flash_erase_program_in_progress(void) __attribute__((always_inline));
static inline bool flash_is_busy(void) __attribute__((always_inline));

#endif /* _QSPI_COMMON_H_ */
/**
 * \}
 * \}
 * \}
 */
