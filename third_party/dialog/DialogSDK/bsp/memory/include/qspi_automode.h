/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup MEMORY
 * 
 * \brief QSPI flash access when running in auto mode
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
 ****************************************************************************************
 */

#ifndef QSPI_AUTOMODE_H_
#define QSPI_AUTOMODE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <black_orca.h>
#include <hw_qspi.h>

/**
 * QSPI controller allows to execute code directly from QSPI flash.
 * When code is executing from flash there is no possibility to reprogram it.
 * To be able to modify flash memory while it is used for code execution it must me assured that
 * for time needed for erase/write no code is running from flash.
 * To achieve this code in this file allows to copy programming functions to RAM and execute it
 * from there.
 * Great flexibility is achieved by putting all code needed for flash modification in one section
 * that will be copied to RAM.
 * Code in this section will not access any other functions or constant variables (that could reside
 * in flash).
 */

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
static inline const void *qspi_automode_addr(uint32_t addr)
{
        return (const void *) (MEMORY_QSPIF_BASE + addr);
}

/**
 * \brief Power up flash
 */
void qspi_automode_flash_power_up(void);

/**
 * \brief Init QSPI controller
 */
bool qspi_automode_init(void);

#endif /* QSPI_AUTOMODE_H_ */

/**
 * \}
 * \}
 * \}
 */
