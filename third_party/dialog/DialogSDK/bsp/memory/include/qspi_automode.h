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
 * @file qspi_automode.h
 *
 * @brief Access QSPI flash when running in auto mode
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

#ifndef QSPI_AUTOMODE_H_
#define QSPI_AUTOMODE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sdk_defs.h>
#include <hw_qspi.h>
#include "hw_cpm.h"
/*
 * Debug options
 */
#if __DBG_QSPI_ENABLED
#define __DBG_QSPI_VOLATILE__           volatile
#pragma message "Automode: Debugging is on!"
#else
#define __DBG_QSPI_VOLATILE__
#endif

/*
 * Defines (generic)
 */

/* Macros to put functions that need to be copied to ram in one section (retained) */
typedef struct qspi_ucode_s {
       const uint32_t *code;
       uint8_t size;
} qspi_ucode_t;

/*
 * Flash specific defines
 */

/**
 * \brief SUS bit delay after SUSPEND command (in μsec)
 *
 */
#define FLASH_SUS_DELAY                 (20)


/**
 * \brief Get size of RAM buffer needed for code to modify QSPI flash.
 *
 * Called must allocate buffer at least of this size and pass it qspi_set_code_buffer() to allow
 * flash modification.
 *
 * \return size of buffer needed
 *
 */
DEPRECATED
uint32_t qspi_automode_get_code_buffer_size(void);

/**
 * \brief Set buffer that will be used for code that modifies flash.
 *
 * This function should be called with buffer allocated for flash manipulation code.
 * Size of this buffer should be at least the value returned from qspi_get_code_buffer_size().
 * If function is called with NULL all further calls to qspi_write_flash_page() or
 * qspi_erase_flash_sector() will crash.
 * To preserve memory it is quite wise to allocate buffer before erase/write, update flash and
 * then call this function with NULL and free memory.
 *
 * \param [in] ram buffer for code that will be used for erase and write.
 *
 */
DEPRECATED_MSG("Function does not need to be called")
void qspi_automode_set_code_buffer(void *ram);

/**
 * \brief Write flash memory
 *
 * This function allows to write up to page size of data to flash.
 * If size is greater than page size, flash can wrap data and overwrite content of page.
 * It's possible to write less then page size.
 * Memory should be erased before.
 *
 * \note: Do not pass buf pointing to QSPI mapped memory.
 *
 * \param [in] addr offset in flash to write data to
 * \param [in] buf pointer to data to write
 * \param [in] size number of bytes to write
 *
 * return number of bytes written
 *
 */
size_t qspi_automode_write_flash_page(uint32_t addr, const uint8_t *buf, size_t size);

/**
 * \brief Erase flash sector
 *
 * \param [in] addr starting offset of sector
 */
void qspi_automode_erase_flash_sector(uint32_t addr);

/**
 * \brief Erase whole chip
 */
void qspi_automode_erase_chip(void);

/**
 * \brief Read flash memory
 *
 * \param [in] addr starting offset
 * \param [out] buf buffer to read data to
 * \param [in] len number of bytes to read
 *
 * \returns number of bytes read
 */
size_t qspi_automode_read(uint32_t addr, uint8_t *buf, size_t len);

/**
 * \brief Get address of flash
 *
 * \param [in] addr starting offset
 *
 * \returns address in CPU address space where data is located
 */
static inline const void *qspi_automode_addr(uint32_t addr) __attribute__((always_inline));

static inline const void *qspi_automode_addr(uint32_t addr)
{
        return (const void *) (MEMORY_QSPIF_BASE + addr);
}

/**
 * \brief Power up flash
 */
__RETAINED_CODE void qspi_automode_flash_power_up(void);

/**
 * \brief Set QSPI Flash into power down mode
 */
__RETAINED_CODE void qspi_automode_flash_power_down(void);

/**
 * \brief Init QSPI controller
 */
__RETAINED_CODE bool qspi_automode_init(void);

#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
/**
 * \brief Check if a program or sector erase operation is in progress
 *
 * \return bool True if the BUSY bit is set else false.
 *
 * \warning This function checks the value of the BUSY bit in the Status Register 1 of the Flash. It
 *        is the responsibility of the caller to call the function in the right context. The
 *        function must be called with interrupts disabled.
 *
 */
__RETAINED_CODE bool qspi_check_program_erase_in_progress(void);

/**
 * \brief Suspend a Flash program or erase operation
 *
 * \details This function will try to suspend an ongoing program or erase procedure. The SUS bit is
 *        checked and the actual status is returned to the caller. Note that the program or erase
 *        procedure may have been completed before the suspend command is processed by the Flash. In
 *        this case the SUS bit will be left to 0.
 *
 * \return bool True if the procedure has been suspended. False if the procedure was not suspended
 *        successfully or had finished before the suspend command was processed by the Flash.
 *
 * \warning After the call to this function, the QSPI controller is set to auto mode and the Flash
 *        access to quad mode (if QUAD_MODE is 1). The function must be called with interrupts
 *        disabled.
 *
 */
__RETAINED_CODE bool qspi_check_and_suspend(void);

/**
 * \brief Resume a Flash program or sector erase operation
 *
 * \warning After the call to this function, the QSPI controller is set to manual mode and the Flash
 *        access to single mode. The function must be called with interrupts disabled.
 *
 */
__RETAINED_CODE void qspi_resume(void);

/**
 * \brief Erase a sector of the Flash in manual mode
 *
 * \param[in] addr The address of the sector to be erased.
 *
 * \warning This function does not block until the Flash has processed the command! The QSPI
 *        controller is left to manual mode after the call to this function. The function must be
 *        called with interrupts disabled.
 *
 */
__RETAINED_CODE void flash_erase_sector_manual_mode(uint32_t addr);

/**
 * \brief Program data into a page of the Flash in manual mode
 *
 * \param[in] addr The address of the Flash where the data will be written. It may be anywhere in a
 *        page.
 * \param[in] buf Pointer to the beginning of the buffer that contains the data to be written.
 * \param[in] len The number of bytes to be written.
 *
 * \return size_t The number of bytes written.
 *
 * \warning The boundary of the page where addr belongs to, will not be crossed! The caller should
 *        issue another flash_program_page_manual_mode() call in order to write the remaining data
 *        to the next page. The QSPI controller is left to manual mode after the call to this
 *        function. The function must be called with interrupts disabled.
 *
 */
__RETAINED_CODE size_t flash_program_page_manual_mode(uint32_t addr, const uint8_t *buf,
                                                      uint16_t len);
#endif

/**
 * \brief Activate Flash command entry mode
 *
 * \note After the call to this function, the QSPI controller is set to manual mode and the Flash
 *        access to single mode.
 *
 * \warning The function must be called with interrupts disabled.
 *
 */
__RETAINED_CODE void flash_activate_command_entry_mode(void);

/**
 * \brief Deactivate Flash command entry mode
 *
 * \note After the call to this function, the QSPI controller is set to auto mode and the Flash
 *        access to quad mode (if QUAD_MODE is 1).
 *
 * \warning The function must be called with interrupts disabled.
 *
 */
__RETAINED_CODE void flash_deactivate_command_entry_mode(void);

/**
 * \brief Configure Flash and QSPI controller for system clock frequency
 *
 * This function is used to change the Flash configuration of the QSPI controller
 * to work with the system clock frequency defined in sys_clk. Dummy clock
 * cycles could be changed here to support higher clock frequencies.
 * QSPI controller clock divider could also be changed if the Flash
 * maximum frequency is smaller than the system clock frequency.
 * This function must be called before changing system clock frequency.
 *
 * \param [in] sys_clk System clock frequency
 *
 */
__RETAINED_CODE void qspi_automode_sys_clock_cfg(sys_clk_t sys_clk);

/**
 * \brief Get ucode required for wake-up sequence
 * \return qspi_ucode_t Pointer to the structure containing the ucode and ucode size
 */
const qspi_ucode_t *qspi_automode_get_ucode(void);

#endif /* QSPI_AUTOMODE_H_ */
/**
 * \}
 * \}
 * \}
 */
