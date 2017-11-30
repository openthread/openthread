/**
 ****************************************************************************************
 *
 * @file qspi_automode.c
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdk_defs.h"

#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
#include "osal.h"
#include "sys_power_mgr.h"
#endif

#include "hw_cpm.h"

#include "hw_qspi.h"
#include "qspi_automode.h"

/*
 * QSPI controller allows to execute code directly from QSPI flash.
 * When code is executing from flash there is no possibility to reprogram it.
 * To be able to modify flash memory while it is used for code execution it must me assured that
 * during the time needed for erase/write no code is running from flash.
 */

/*
 * Flash specific defines
 */

/* SUS bit delay after SUSPEND command */
#define FLASH_SUS_DELAY                 (20) // in usec

/* Flash page size */
#define FLASH_MAX_WRITE_SIZE            dg_configFLASH_MAX_WRITE_SIZE

/*
 * Use QUAD mode for page write.
 *
 * Note: If the flash does not support QUAD mode or it is not connected for QUAD mode set it to 0
 * (single mode).
 */
#ifndef QUAD_MODE
#define QUAD_MODE                       1
#endif

#ifndef ERASE_IN_AUTOMODE
#define ERASE_IN_AUTOMODE               1
#endif

#ifndef FLASH_FORCE_24BIT_ADDRESSING
#define FLASH_FORCE_24BIT_ADDRESSING    0       // Force 24 bit addressing for devices > 128Mbits
#endif

/*
 * WARNING: The Autodetect mode will greatly increase both the code and the used RetRAM size!!!!
 *          Use with extreme caution!!
 */
#if dg_configFLASH_AUTODETECT == 1
        #define FLASH_AUTODETECT                1       // Enable Flash auto-detection
#else
        #if !defined(dg_configFLASH_MANUFACTURER_ID)
        #error Please define dg_configFLASH_MANUFACTURER_ID !!!
        #endif
        #if !defined(dg_configFLASH_DEVICE_TYPE)
        #error Please define dg_configFLASH_DEVICE_TYPE !!!
        #endif
        #if !defined(dg_configFLASH_DENSITY)
        #error Please define dg_configFLASH_DENSITY !!!
        #endif
        #define FLASH_AUTODETECT                0
#endif

#if FLASH_AUTODETECT == 1
#include "qspi_w25q80ew.h"
#include "qspi_mx25u51245.h"
#include "qspi_gd25lq80b.h"
#else
#ifndef dg_configFLASH_HEADER_FILE
#error Please define macro dg_configFLASH_HEADER_FILE to the header file name that contains the respective implementation
#endif
#include dg_configFLASH_HEADER_FILE
#endif

#if (FLASH_AUTODETECT == 1)

static const qspi_flash_config_t* flash_config_table[] = {
        &flash_w25q80ew_config, // This is the default one
        &flash_mx25u51245_config,
        &flash_gd25lq80b_config,
};

__RETAINED static qspi_flash_config_t flash_autodetect_config;

__RETAINED qspi_flash_config_t * flash_config;

#endif



/*
 * Function definitions
 */

/**
 * \brief Set bus mode to single or QUAD mode.
 *
 * \param[in] mode Can be single (HW_QSPI_BUS_MODE_SINGLE) or quad (HW_QSPI_BUS_MODE_QUAD) mode.
 *
 * \note DUAL mode page program so is not supported by this function.
 */
static inline void qspi_set_bus_mode(HW_QSPI_BUS_MODE mode) __attribute__((always_inline));

static inline void qspi_set_bus_mode(HW_QSPI_BUS_MODE mode)
{
        if (mode == HW_QSPI_BUS_MODE_SINGLE) {
                QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_SINGLE);
                QSPIC->QSPIC_CTRLMODE_REG |=
                        BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO2_OEN, 1) |
                        BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO2_DAT, 1) |
                        BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO3_OEN, 1) |
                        BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO3_DAT, 1);
        } else {
#if QUAD_MODE
                QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_QUAD);
                QSPIC->QSPIC_CTRLMODE_REG &=
                        ~(BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO2_OEN, 1) |
                          BITS32(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_IO3_OEN, 1));
#endif
        }
}

/**
 * \brief Set the mode of the QSPI controller (manual or auto)
 *
 * \param[in] automode True for auto and false for manual mode setting.
 */
static inline void qspi_set_automode(bool automode) __attribute__((always_inline));

static inline void qspi_set_automode(bool automode)
{
        REG_SETF(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_AUTO_MD, automode);
}

/**
 * \brief Write to the Flash the contents of a buffer
 *
 * \param[in] wbuf Pointer to the beginning of the buffer
 * \param[in] wlen The number of bytes to be written
 *
 * \note The data are transferred as bytes (8 bits wide). No optimization is done in trying to use
 *       faster access methods (i.e. transfer words instead of bytes whenever it is possible).
 */
__RETAINED_CODE static void qspi_write(const uint8_t *wbuf, size_t wlen)
{
        size_t i;

        hw_qspi_cs_enable();

        for (i = 0; i < wlen; ++i) {
                hw_qspi_write8(wbuf[i]);
        }

        hw_qspi_cs_disable();
}

/**
 * \brief Write an arbitrary number of bytes to the Flash and then read an arbitrary number of bytes
 *       from the Flash in one transaction
 *
 * \param[in] wbuf Pointer to the beginning of the buffer that contains the data to be written
 * \param[in] wlen The number of bytes to be written
 * \param[in] rbuf Pointer to the beginning of the buffer than the read data are stored
 * \param[in] rlen The number of bytes to be read
 *
 * \note The data are transferred as bytes (8 bits wide). No optimization is done in trying to use
 *       faster access methods (i.e. transfer words instead of bytes whenever it is possible).
 */
__RETAINED_CODE static void qspi_transact(const uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen)
{
        size_t i;

        hw_qspi_cs_enable();

        for (i = 0; i < wlen; ++i) {
                hw_qspi_write8(wbuf[i]);
        }

        for (i = 0; i < rlen; ++i) {
                rbuf[i] = hw_qspi_read8();
        }

        hw_qspi_cs_disable();
}


static inline bool flash_erase_program_in_progress(void)
{
        __DBG_QSPI_VOLATILE__ uint8_t status;
        uint8_t cmd[] = { flash_config->read_erase_progress_opcode };

        qspi_transact(cmd, 1, &status, 1);

        return ((status & (1 << flash_config->erase_in_progress_bit)) != 0) == flash_config->erase_in_progress_bit_high_level;
}


static inline bool flash_is_busy(void)
{
        return (flash_read_status_register() & FLASH_STATUS_BUSY_MASK) != 0;
}

/**
 * \brief Exit from continuous mode.
 */
__RETAINED_CODE static void flash_reset_continuous_mode(HW_QSPI_BREAK_SEQ_SIZE break_seq_size)
{
        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_EXIT_CONTINUOUS_MODE);
        if (break_seq_size == HW_QSPI_BREAK_SEQ_SIZE_2B) {
                hw_qspi_write8(CMD_EXIT_CONTINUOUS_MODE);
        }
        hw_qspi_cs_disable();

        while (flash_is_busy());
}

/**
 * \brief Get Device ID when Flash is not in Power Down mode
 *
 * \return uint8_t The Device ID of the Flash
 *
 * \note The function blocks until the Flash executes the command.
 */
__RETAINED_CODE __attribute__((unused)) static uint8_t flash_get_id(void)
{
        uint8_t id;

        hw_qspi_cs_enable();
        hw_qspi_write32(CMD_RELEASE_POWER_DOWN);
        id = hw_qspi_read8();
        hw_qspi_cs_disable();

        while (flash_is_busy());

        return id;
}

/**
 * \brief Set WEL (Write Enable Latch) bit of the Status Register of the Flash
 * \details The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase,
 *       Block Erase, Chip Erase, Write Status Register and Erase/Program Security Registers
 *       instruction. In the case of Write Status Register command, any status bits will be written
 *       as non-volatile bits.
 *
 * \note This function blocks until the Flash has processed the command and it will be repeated if,
 *       for any reason, the command was not successfully executed by the Flash.
 */
__RETAINED_CODE static void flash_write_enable(void)
{
        __DBG_QSPI_VOLATILE__ uint8_t status;
        uint8_t cmd[] = { CMD_WRITE_ENABLE };

        do {
                qspi_write(cmd, 1);
                /* Verify */
                do {
                        status = flash_read_status_register();
                } while (status & FLASH_STATUS_BUSY_MASK);
        } while (!(status & FLASH_STATUS_WEL_MASK));
}

/**
 * \brief Read the Status Register 1 of the Flash
 *
 * \return uint8_t The value of the Status Register 1 of the Flash.
 */
__RETAINED_CODE static uint8_t flash_read_status_register(void)
{
        __DBG_QSPI_VOLATILE__ uint8_t status;
        uint8_t cmd[] = { CMD_READ_STATUS_REGISTER };

        qspi_transact(cmd, 1, &status, 1);

        return status;
}

/**
 * \brief Write the Status Register 1 of the Flash
 *
 * \param[in] value The value to be written.
 *
 * \note This function blocks until the Flash has processed the command. No verification that the
 *        value has been actually written is done though. It is up to the caller to decide whether
 *        such verification is needed or not and execute it on its own.
 */
__RETAINED_CODE __attribute__((unused)) static void flash_write_status_register(uint8_t value)
{
        uint8_t cmd[2] = { CMD_WRITE_STATUS_REGISTER, value };

        qspi_write(cmd, 2);

        /* Wait for the Flash to process the command */
        while (flash_is_busy());
}

/**
 * \brief Fast copy of a buffer to a FIFO
 * \details Implementation of a fast copy of the contents of a buffer to a FIFO in assembly. All
 *        addresses are word aligned.
 *
 * \param[in] start Pointer to the beginning of the buffer
 * \param[in] end Pointer to the end of the buffer
 * \param[in] Pointer to the FIFO
 *
 * \warning No validity checks are made! It is the responsibility of the caller to make sure that
 *        sane values are passed to this function.
 */
static inline
void fast_write_to_fifo32(uint32_t start, uint32_t end, uint32_t dest) __attribute__((always_inline));

static inline void fast_write_to_fifo32(uint32_t start, uint32_t end, uint32_t dest)
{
        __asm volatile(   "copy:                                  \n"
                          "       ldmia %[start]!, {r3}           \n"
                          "       str r3, [%[dest]]               \n"
                          "       cmp %[start], %[end]            \n"
                          "       blt copy                        \n"
                          :
                          :                                                         /* output */
                          [start] "l" (start), [end] "r" (end), [dest] "l" (dest) : /* inputs (%0, %1, %2) */
                          "r3");                                              /* registers that are destroyed */
}

/**
 * \brief Write data (up to 1 page) to Flash
 *
 * \param[in] addr The address of the Flash where the data will be written. It may be anywhere in a
 *        page.
 * \param[in] buf Pointer to the beginning of the buffer that contains the data to be written.
 * \param[in] size The number of bytes to be written.
 *
 * \return size_t The number of bytes written.
 *
 * \warning The boundary of the page where addr belongs to, will not be crossed! The caller should
 *        issue another flash_write_page() call in order to write the remaining data to the next
 *        page.
 */
__RETAINED_CODE static size_t flash_write_page(uint32_t addr, const uint8_t *buf, uint16_t size)
{
        size_t i = 0;
        size_t odd = ((uint32_t) buf) & 3;
        size_t size_aligned32;
        size_t tmp;

        DBG_SET_HIGH(FLASH_DEBUG, FLASHDBG_PAGE_PROG);

        flash_write_enable();

        /* Reduce max write size, that can reduce interrupt latency time */
        if (size > FLASH_MAX_WRITE_SIZE) {
                size = FLASH_MAX_WRITE_SIZE;
        }

        /* Make sure write will not cross page boundary */
        tmp = 256 - (addr & 0xFF);
        if (size > tmp) {
                size = tmp;
        }

        hw_qspi_cs_enable();

        if (flash_config->address_size == HW_QSPI_ADDR_SIZE_32) {
                hw_qspi_write8(flash_config->page_program_opcode);
#if QUAD_MODE
                if (flash_config->quad_page_program_address == true) {
                        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
                }
#endif
                hw_qspi_write32((addr >> 24) | ((addr >> 8) & 0xFF00) | ((addr << 8) & 0xFF0000) | (addr << 24));
#if QUAD_MODE
                if (flash_config->quad_page_program_address == false) {
                        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
                }
#endif
        }
        else {
                if (flash_config->quad_page_program_address == true) {
                        hw_qspi_write8(flash_config->page_program_opcode);
#if QUAD_MODE
                        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif
                        hw_qspi_write16(((addr >> 16) & 0xFF) | (addr & 0xFF00));
                        hw_qspi_write8(addr & 0xFF);
                }
                else {
                        hw_qspi_write32(CMD_QUAD_PAGE_PROGRAM | ((addr >> 8) & 0xFF00) | ((addr << 8) & 0xFF0000) | (addr << 24));
#if QUAD_MODE
                        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif
                }
        }

        if (odd) {
                odd = 4 - odd;
                for (i = 0; i < odd && i < size; ++i) {
                        hw_qspi_write8(buf[i]);
                }
        }

        size_aligned32 = ((size - i) & ~0x3);

        if (size_aligned32) {
                fast_write_to_fifo32((uint32_t)(buf + i), (uint32_t)(buf + i + size_aligned32),
                        (uint32_t)&(QSPIC->QSPIC_WRITEDATA_REG));
                i += size_aligned32;
        }

        for (; i < size; i++) {
                hw_qspi_write8(buf[i]);
        }

        hw_qspi_cs_disable();

        DBG_SET_LOW(FLASH_DEBUG, FLASHDBG_PAGE_PROG);

#if QUAD_MODE
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);
#endif

        return i;
}

/**
 * \brief Erase a sector of the Flash
 *
 * \param[in] addr The address of the sector to be erased.
 *
 * \note This function blocks until the Flash has processed the command.
 */
__RETAINED_CODE __attribute__((unused)) static void flash_erase_sector(uint32_t addr)
{
        flash_write_enable();

        if (flash_config->address_size == HW_QSPI_ADDR_SIZE_32) {
                uint8_t cmd[5] = { flash_config->erase_opcode, addr >> 24, addr >> 16, addr >>  8, addr };
                qspi_write(cmd, 5);
        }
        else {
                uint8_t cmd[4] = { flash_config->erase_opcode, addr >> 16, addr >>  8, addr };
                qspi_write(cmd, 4);
        }


        /* Wait for the Flash to process the command */
        while (flash_erase_program_in_progress());
}

/**
 * \brief Check if the Flash can accept commands
 *
 * \return bool True if the Flash is not busy else false.
 *
 */
__RETAINED_CODE static bool qspi_writable(void)
{
        bool writable;

        /*
         * From now on QSPI may not be available, turn off interrupts.
         */
        GLOBAL_INT_DISABLE();

        /*
         * Turn on command entry mode.
         */
        flash_activate_command_entry_mode();

        /*
         * Check if flash is ready.
         */
        writable = !(flash_is_busy());

        /*
         * Restore auto mode.
         */
        flash_deactivate_command_entry_mode();

        /*
         * Let other code to be executed including QSPI one.
         */
        GLOBAL_INT_RESTORE();

        return writable;
}

/**
 * \brief Get the status of an Erase when it is done automatically by the QSPI controller
 *
 * \return uint8_t The status of the Erase
 *        0: No Erase
 *        1: Pending erase request
 *        2: Erase procedure is running
 *        3: Suspended Erase procedure
 *        4: Finishing the Erase procedure
 */
__RETAINED_CODE __attribute__((unused)) static uint8_t qspi_get_erase_status(void)
{
        QSPIC->QSPIC_CHCKERASE_REG = 0;

        return HW_QSPIC_REG_GETF(ERASECTRL, ERS_STATE);
}

/**
 * \brief Get the address size used by the QSPI controller
 *
 * \return The address size used (HW_QSPI_ADDR_SIZE_24 or HW_QSPI_ADDR_SIZE_32).
 */
static inline HW_QSPI_ADDR_SIZE qspi_get_address_size(void) __attribute__((always_inline));

static inline HW_QSPI_ADDR_SIZE qspi_get_address_size(void)
{
        return (HW_QSPI_ADDR_SIZE)HW_QSPIC_REG_GETF(CTRLMODE, USE_32BA);
}

__RETAINED_CODE void flash_activate_command_entry_mode(void)
{
        /*
         * Turn off auto mode to allow write.
         */
        qspi_set_automode(false);

        /*
         * Switch to single mode for command entry.
         */
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

        /*
         * Exit continuous mode (QPI mode), after this the flash will interpret commands again.
         */
        if (flash_config->send_once != 0) {
                flash_reset_continuous_mode(flash_config->break_seq_size);
        }
}


__RETAINED_CODE void flash_deactivate_command_entry_mode(void)
{
        flash_config->deactivate_command_entry_mode();

#if QUAD_MODE
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif
        qspi_set_automode(true);
}

#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
__RETAINED_CODE void flash_erase_sector_manual_mode(uint32_t addr)
{
        /*
         * Turn on command entry mode.
         */
        flash_activate_command_entry_mode();

        /*
         * Issue the erase sector command.
         */
        flash_write_enable();

        if (flash_config->address_size == HW_QSPI_ADDR_SIZE_32) {
                uint8_t cmd[5] = { flash_config->erase_opcode, addr >> 24, addr >> 16, addr >>  8, addr };
                qspi_write(cmd, 5);
        }
        else {
                uint8_t cmd[4] = { flash_config->erase_opcode, addr >> 16, addr >>  8, addr };
                qspi_write(cmd, 4);
        }

        /*
         * Flash stays in manual mode.
         */
}

__RETAINED_CODE size_t flash_program_page_manual_mode(uint32_t addr, const uint8_t *buf, uint16_t size)
{
        size_t written = flash_write_page(addr, buf, size);

        /*
         * Flash stays in manual mode.
         */

        return written;
}

__RETAINED_CODE bool qspi_check_program_erase_in_progress(void)
{
        return flash_is_busy();
}

__RETAINED_CODE bool qspi_check_and_suspend(void)
{
        uint8_t cmd[] = { flash_config->erase_suspend_opcode };
        bool am = hw_qspi_get_automode();
        bool ret = true;

        if (am) {
                /*
                 * Turn on command entry mode.
                 */
                flash_activate_command_entry_mode();
        }

        /*
         * Suspend action.
         */
        DBG_SET_HIGH(FLASH_DEBUG, FLASHDBG_SUSPEND_ACTION);

        /*
         * Check if an operation is ongoing.
         */
        while (flash_erase_program_in_progress()) {
                qspi_write(cmd, 1);
        }

        hw_cpm_delay_usec(FLASH_SUS_DELAY);  // Wait for SUS bit to be updated

        DBG_SET_LOW(FLASH_DEBUG, FLASHDBG_SUSPEND_ACTION);

        if (flash_config->is_suspended() == false) {
                ret = false;
        }

        /*
         * Restore auto mode.
         */
        flash_deactivate_command_entry_mode();

        return ret;
}

__RETAINED_CODE void qspi_resume(void)
{
        uint8_t cmd[] = { flash_config->erase_resume_opcode };

        do {
                /*
                 * Turn on command entry mode.
                 */
                flash_activate_command_entry_mode();

                /*
                 * Check if suspended.
                 */
                if (flash_config->is_suspended() == false) {
                        break;
                }

                /*
                 * Wait for flash to become ready again.
                 */
                do {
                        /*
                         * Resume action.
                         */
                        qspi_write(cmd, 1);

                        /*
                         * Check if SUS bit is cleared.
                         */
                } while (flash_config->is_suspended());
        } while (0);

        /*
         * Flash stays in manual mode.
         */
}
#endif

#if (ERASE_IN_AUTOMODE == 1)
/**
 * \brief Erase sector (via CPM background processing or using the QSPI controller)
 *
 * \details This function will execute a Flash sector erase operation. The operation will either be
 *        carried out immediately (dg_configDISABLE_BACKGROUND_FLASH_OPS is set to 1) or it will be
 *        deferred to be executed by the CPM when the system becomes idle (when
 *        dg_configDISABLE_BACKGROUND_FLASH_OPS is set to 0, default value). In the latter case, the
 *        caller will block until the CPM completes the registered erase operation.
 *
 * \param[in] addr The address of the sector to be erased.
 */
__RETAINED_CODE static void qspi_erase_sector(uint32_t addr)
{
#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
        OS_TASK handle;
        void *op;

        handle = OS_GET_CURRENT_TASK();

        /* Instruct CPM to erase this sector */
        if (pm_register_qspi_operation(handle, addr, NULL, 0, &op)) {
                /* Block until erase is completed */
                OS_TASK_SUSPEND(handle);
                OS_FREE(op);
        }
        else {
                /* The PM has not started yet... */

#endif
                /* Wait for previous erase to end */
                while (qspi_get_erase_status() != 0) {
                }

                if (qspi_get_address_size() == HW_QSPI_ADDR_SIZE_32) {
                        addr >>= 12;
                } else {
                        addr >>= 4;
                }

                /* Setup erase block page */
                HW_QSPIC_REG_SETF(ERASECTRL, ERS_ADDR, addr);

                /* Fire erase */
                HW_QSPIC_REG_SETF(ERASECTRL, ERASE_EN, 1);
#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
        }
#endif
}
#endif /* (ERASE_IN_AUTOMODE == 1)  */

/**
 * \brief Write data to a page in the Flash (via CPM background processing or using the QSPI
 *        controller)
 *
 * \details This function will execute a Flash program page operation. The operation will either be
 *        carried out immediately (dg_configDISABLE_BACKGROUND_FLASH_OPS is set to 1) or it will be
 *        deferred to be executed by the CPM when the system becomes idle (when
 *        dg_configDISABLE_BACKGROUND_FLASH_OPS is set to 0, default value). In the latter case, the
 *        caller will block until the CPM completes the registered page program operation.
 *
 * \param[in] addr The address of the Flash where the data will be written. It may be anywhere in a
 *        page.
 * \param[in] buf Pointer to the beginning of the buffer that contains the data to be written.
 * \param[in] size The number of bytes to be written.
 *
 * \return size_t The number of bytes written.
 *
 * \warning The caller should ensure that buf does not point to QSPI mapped memory.
 */
__RETAINED_CODE static size_t write_page(uint32_t addr, const uint8_t *buf, uint16_t size)
{
        size_t written;

#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
        OS_TASK handle;
        void *op;

        handle = OS_GET_CURRENT_TASK();

        /* Instruct CPM to program this sector */
        if (pm_register_qspi_operation(handle, addr, buf, &size, &op)) {
                /* Block until program is completed */
                OS_TASK_SUSPEND(handle);
                OS_FREE(op);
                written = size;
        }
        else {
#endif
                /*
                 * From now on QSPI may not be available, turn of interrupts.
                 */
                GLOBAL_INT_DISABLE();

                /*
                 * Turn on command entry mode.
                 */
                flash_activate_command_entry_mode();

                /*
                 * Write data into the page of the Flash.
                 */
                written = flash_write_page(addr, buf, size);

                /* Wait for the Flash to process the command */
                while (flash_erase_program_in_progress());

                /*
                 * Restore auto mode.
                 */
                flash_deactivate_command_entry_mode();

                /*
                 * Let other code to be executed including QSPI one.
                 */
                GLOBAL_INT_RESTORE();
#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 0)
        }
#endif

        return written;
}

/**
 * \brief Erase a sector of the Flash
 *
 * \details The time and the way that the operation will be carried out depends on the following
 *        settings:
 *        ERASE_IN_AUTOMODE = 0: the command is issued immediately in manual mode
 *        ERASE_IN_AUTOMODE = 1:
 *              dg_configDISABLE_BACKGROUND_FLASH_OPS = 0: the operation is executed manually by the
 *                      CPM when the system becomes idle
 *              dg_configDISABLE_BACKGROUND_FLASH_OPS = 1: the operation is executed automatically
 *                      by the QSPI controller.
 *
 * \param[in] addr The address of the sector to be erased.
 */
__RETAINED_CODE static void erase_sector(uint32_t addr)
{
#if ERASE_IN_AUTOMODE
        /*
         * Erase sector in automode
         */
        qspi_erase_sector(addr);

        /*
         * Wait for erase to finish
         */
        while (qspi_get_erase_status()) {
        }
#else
        /*
         * From now on QSPI may not be available, turn of interrupts.
         */
        GLOBAL_INT_DISABLE();

        /*
         * Turn off auto mode to allow write.
         */
        qspi_set_automode(false);

        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

        /*
         * Exit continuous mode, after this the flash will interpret commands again.
         */
        flash_reset_continuous_mode(flash_config->break_seq_size);

        /*
         * Execute erase command.
         */
        flash_erase_sector(addr);

        /*
         * Restore auto mode.
         */
        flash_deactivate_command_entry_mode();

        /*
         * Let other code to be executed including QSPI one.
         */
        GLOBAL_INT_RESTORE();
#endif
}

uint32_t qspi_automode_get_code_buffer_size(void)
{
        /* Return 1 in case some code will pass this value to memory allocation function */
        return 1;
}

void qspi_automode_set_code_buffer(void *ram)
{
        (void) ram; /* Unused */
}

bool qspi_automode_writable(void)
{
        return qspi_writable();
}

size_t qspi_automode_write_flash_page(uint32_t addr, const uint8_t *buf, size_t size)
{
        while (!qspi_automode_writable()) {
        }

        return write_page(addr, buf, size);
}

void qspi_automode_erase_flash_sector(uint32_t addr)
{
        while (!qspi_automode_writable()) {
        }

        erase_sector(addr);
}

void qspi_automode_erase_chip(void)
{
        flash_activate_command_entry_mode();

        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_WRITE_ENABLE);
        hw_qspi_cs_disable();

        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_CHIP_ERASE);
        hw_qspi_cs_disable();

        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_READ_STATUS_REGISTER);
        while (hw_qspi_read8() & FLASH_STATUS_BUSY_MASK);
        hw_qspi_cs_disable();

        flash_deactivate_command_entry_mode();
}

size_t qspi_automode_read(uint32_t addr, uint8_t *buf, size_t len)
{
        memcpy(buf, (void *)(MEMORY_QSPIF_BASE + addr), len);

        return len;
}

__RETAINED_CODE void qspi_automode_flash_power_up(void)
{
        hw_cpm_delay_usec(flash_config->power_down_delay);

        /* Interrupts must be turned off since the flash goes in manual mode, and
         * code (e.g. for an ISR) cannot be fetched from flash during this time
         */
        GLOBAL_INT_DISABLE();

        // Do not call flash_activate_command_entry_mode(). This function will call
        // flash_reset_continuous_mode which will try to send break sequence to the QSPI Flash
        // which is in power-down mode.
        qspi_set_automode(false);
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_RELEASE_POWER_DOWN);
        hw_qspi_cs_disable();

        flash_deactivate_command_entry_mode();

        /*
         * The flash is in auto mode again. Re-enable the interrupts
         */
        GLOBAL_INT_RESTORE();

        hw_cpm_delay_usec(flash_config->release_power_down_delay);
}

__RETAINED_CODE void qspi_automode_flash_power_down(void)
{
        flash_activate_command_entry_mode();

        hw_qspi_cs_enable();
        hw_qspi_write8(CMD_ENTER_POWER_DOWN);
        hw_qspi_cs_disable();

// Do not call flash_deactivate_command_entry_mode(). This function will call
// flash_config->deactivate_command_entry_mode() which will try to send commands to the QSPI Flash
// which has already been set in power-down mode.

//        flash_deactivate_command_entry_mode();

#if QUAD_MODE
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif
        qspi_set_automode(true);
}

static qspi_config qspi_cfg = {
        HW_QSPI_ADDR_SIZE_24, HW_QSPI_POL_HIGH, HW_QSPI_SAMPLING_EDGE_NEGATIVE
};

/**
 * \brief Read the JEDEC manufacturer ID, device type and device density using command 0x9F
 *
 * \param[in] manufacturer_id Pointer to the variable where the manufacturer ID will be returned
 * \param[in] device_type Pointer to the variable where the device type will be returned
 * \param[in] density Pointer to the variable where the device density will be returned
 */
#if FLASH_AUTODETECT
__RETAINED_CODE static void flash_read_jedec_id(uint8_t *manufacturer_id, uint8_t *device_type, uint8_t *density)
{
        uint8_t cmd[] = { CMD_READ_JEDEC_ID };
        uint8_t buffer[3];

        qspi_set_automode(false);
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

        // reset continuous mode using both one and two break bytes to cover all cases
        flash_reset_continuous_mode(HW_QSPI_BREAK_SEQ_SIZE_2B);
        flash_reset_continuous_mode(HW_QSPI_BREAK_SEQ_SIZE_1B);

        qspi_transact(cmd, 1, buffer, 3);
        *manufacturer_id = buffer[0];
        *device_type = buffer[1];
        *density = buffer[2];

#if QUAD_MODE
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif
        qspi_set_automode(true);
}
#endif

/**
 * \brief Configure dummy bytes in QSPI controller
 *
 * \param[in] count Number of dummy bytes (not including extra byte)
 */
__RETAINED_CODE __attribute__((unused)) static void qspi_automode_set_dummy_bytes_count(uint8_t count)
{
        if (count == 3) {
                HW_QSPIC_REG_SETF(BURSTCMDB, DMY_FORCE, 1);
        } else {
                QSPIC->QSPIC_BURSTCMDB_REG =
                        (QSPIC->QSPIC_BURSTCMDB_REG &
                                ~(REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_FORCE) |
                                        REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM))) |
                                        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM,
                                                dummy_num[count]);
        }
}

__RETAINED_CODE bool qspi_automode_init(void)
{
        uint8_t device_type;
        uint8_t device_density;

        hw_qspi_enable_clock();


#if __DBG_QSPI_ENABLED
        REG_SETF(CRG_TOP, CLK_AMBA_REG, QSPI_DIV, 3);
#endif

#if FLASH_AUTODETECT
        uint8_t manufacturer_id;
        uint8_t i;


        flash_read_jedec_id(&manufacturer_id, &device_type, &device_density);

        const qspi_flash_config_t* flash_config_init = flash_config_table[0]; // default to first entry.

        for (i=0; i< sizeof(flash_config_table)/sizeof(qspi_flash_config_t*); i++) {
                if ( (flash_config_table[i]->manufacturer_id == manufacturer_id) &&
                        (flash_config_table[i]->device_type == device_type) &&
                        (flash_config_table[i]->device_density == device_density)) {
                        flash_config_init = flash_config_table[i];
                        break;
                }
        }
        flash_autodetect_config = *flash_config_init;
        flash_config = &flash_autodetect_config;
#else
        device_type     = dg_configFLASH_DEVICE_TYPE;
        device_density  = dg_configFLASH_DENSITY;
#endif

        /*
         * Copy the selected flash struct from flash into retram
         */
        flash_config->initialize(device_type, device_density);

        uint8_t read_opcode;

        if (flash_config->address_size == HW_QSPI_ADDR_SIZE_32) {
                read_opcode = CMD_FAST_READ_QUAD_4B;
                qspi_cfg.address_size = HW_QSPI_ADDR_SIZE_32; // It will be set when hw_qspi_init() is called
        }
        else {
                read_opcode = CMD_FAST_READ_QUAD;
        }
        /*
         * Setup erase instruction that will be sent by QSPI controller to erase sector in automode.
         */
        hw_qspi_set_erase_instruction(flash_config->erase_opcode, HW_QSPI_BUS_MODE_SINGLE,
                HW_QSPI_BUS_MODE_SINGLE, 15, 5);
        /*
         * Setup instruction pair that will temporarily suspend erase operation to allow read.
         */
        hw_qspi_set_suspend_resume_instructions(flash_config->erase_suspend_opcode, HW_QSPI_BUS_MODE_SINGLE,
                                                flash_config->erase_resume_opcode, HW_QSPI_BUS_MODE_SINGLE, 7);

        /*
         * QSPI controller must send write enable before erase, this sets it up.
         */
        hw_qspi_set_write_enable_instruction(CMD_WRITE_ENABLE, HW_QSPI_BUS_MODE_SINGLE);

        /*
         * Setup instruction that will be used to periodically check erase operation status.
         * Check LSB which is 1 when erase is in progress.
         */
        hw_qspi_set_read_status_instruction(flash_config->read_erase_progress_opcode, HW_QSPI_BUS_MODE_SINGLE,
                HW_QSPI_BUS_MODE_SINGLE, flash_config->erase_in_progress_bit, flash_config->erase_in_progress_bit_high_level?1:0, 20, 0);

        /*
         * This sequence is necessary if flash is working in continuous read mode, when instruction
         * is not sent on every read access just address. Sending 0xFFFF will exit this mode.
         * This sequence is sent only when QSPI is working in automode and decides to send one of
         * instructions above.
         * If flash is working in DUAL bus mode sequence should be 0xFFFF and size should be
         * HW_QSPI_BREAK_SEQ_SIZE_2B.
         */
        hw_qspi_set_break_sequence(0xFFFF, HW_QSPI_BUS_MODE_SINGLE, flash_config->break_seq_size, 0);

        /*
         * If application starts from FLASH then bootloader must have set read instruction.
         */
        if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH) {
                hw_qspi_init(&qspi_cfg);
                hw_qspi_set_div(HW_QSPI_DIV_1);
        }
        flash_activate_command_entry_mode();

        hw_qspi_set_read_instruction(read_opcode, flash_config->send_once, flash_config->get_dummy_bytes(), HW_QSPI_BUS_MODE_SINGLE,
                                     HW_QSPI_BUS_MODE_QUAD, HW_QSPI_BUS_MODE_QUAD,
                                     HW_QSPI_BUS_MODE_QUAD);
        hw_qspi_set_extra_byte(flash_config->extra_byte, HW_QSPI_BUS_MODE_QUAD, 0);
        hw_qspi_set_address_size(flash_config->address_size);

        flash_deactivate_command_entry_mode();

        HW_QSPIC_REG_SETF(BURSTCMDB, CS_HIGH_MIN, 0);

        return true;
}

__RETAINED_CODE void qspi_automode_sys_clock_cfg(sys_clk_t sys_clk)
{
        if ((CRG_TOP->CLK_AMBA_REG & (1 << REG_POS(CRG_TOP, CLK_AMBA_REG, QSPI_ENABLE))) != 0) { // If clock is enabled
                /* Some of the sys_clk_cfg() implementations put the flash in command entry mode, where the flash is
                 * not available for code execution. We must make sure that no interrupt (that may cause a cache miss)
                 * hits during this time.
                 */
                GLOBAL_INT_DISABLE();
                flash_config->sys_clk_cfg(sys_clk);
                GLOBAL_INT_RESTORE();
        }
}

const qspi_ucode_t *qspi_automode_get_ucode(void)
{
        return &flash_config->ucode_wakeup;
}
