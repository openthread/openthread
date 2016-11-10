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
 ****************************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <black_orca.h>
#include <hw_qspi.h>

/*
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

/* Erase/Write in progress */
#define W25Q_STATUS1_BUSY_BIT        0
#define W25Q_STATUS1_BUSY_MASK       (1 << W25Q_STATUS1_BUSY_BIT)

#define W25Q_STATUS1_WEL_BIT         1
#define W25Q_STATUS1_WEL_MASK        (1 << W25Q_STATUS1_WEL_BIT)

#define W25Q_WRITE_STATUS_REGISTER   0x01
#define W25Q_PAGE_PROGRAM            0x02
#define W25Q_WRITE_DISABLE           0x04
#define W25Q_READ_STATUS_REGISTER1   0x05
#define W25Q_WRITE_ENABLE            0x06
#define W25Q_SECTOR_ERASE            0x20
#define W25Q_QUAD_PAGE_PROGRAM       0x32
#define W25Q_READ_STATUS_REGISTER2   0x35
#define W25Q_BLOCK_ERASE             0x52
#define W25Q_ERASE_PROGRAM_SUSPEND   0x75
#define W25Q_ERASE_PROGRAM_RESUME    0x7A
#define W25Q_BLOCK_ERASE_64K         0xD8
#define W25Q_CHIP_ERASE              0xC7
#define W25Q_RELEASE_POWER_DOWN      0xAB
#define W25Q_FAST_READ_QUAD          0xEB

#define FLASH_MAX_WRITE_SIZE  dg_configFLASH_MAX_WRITE_SIZE

/*
 * Use QUAD mode for page write.
 * If flash does not support QUAD mode or it is not connected for QUAD mode set it to 0.
 */
#ifndef QUAD_MODE
#define QUAD_MODE 1
#endif

#ifndef ERASE_IN_AUTOMODE
#define ERASE_IN_AUTOMODE 1
#endif

/*
 * Macros to put functions that need to be copied to ram in one section.
 */
#define QSPI_SECTION __attribute__ ((section ("text_retained")))
#define QSPI_SECTION_NOINLINE __attribute__ ((section ("text_retained"), noinline))

QSPI_SECTION static uint8_t flash_read_status_register_1(void);

/*
 * Set bus mode to single or QUAD mode.
 * Flash does not support DUAL page program so it is not supported here.
 */
QSPI_SECTION static inline void qspi_set_bus_mode(HW_QSPI_BUS_MODE mode)
{
    if (mode == HW_QSPI_BUS_MODE_SINGLE)
    {
        QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_SINGLE);
    }
    else
    {
#if QUAD_MODE
        QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_QUAD);
#endif
    }
}

/*
 * Enable CS
 */
QSPI_SECTION static inline void qspi_cs_enable(void)
{
    QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;
}

/*
 * Disable CS
 */
QSPI_SECTION static inline void qspi_cs_disable(void)
{
    QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;
}

/*
 * Write 8 bits to QSPI bus.
 */
QSPI_SECTION static inline void qspi_write8(uint8_t data)
{
    *((volatile uint8_t *) & (QSPIC->QSPIC_WRITEDATA_REG)) = data;
}

/*
 * Write 32 bits to QSPI bus.
 * It creates more efficient transmission then 8 bits write.
 */
QSPI_SECTION static inline void qspi_write32(uint32_t data)
{
    QSPIC->QSPIC_WRITEDATA_REG = data;
}

/*
 * Read byte from QSPI bus.
 */
QSPI_SECTION static inline uint8_t qspi_read8(void)
{
    return *((volatile uint8_t *) & (QSPIC->QSPIC_READDATA_REG));
}

/*
 * Escape continues read mode that could be programmed in auto mode.
 * Without this when controller is switched to manual mode it could treat incoming
 * commands as addresses.
 */
QSPI_SECTION static inline void flash_reset_continuous_mode(void)
{
    qspi_cs_enable();
    qspi_write8(0xFF);
    qspi_cs_disable();
}

QSPI_SECTION uint8_t flash_release_power_down(void)
{
    uint8_t id;

    qspi_cs_enable();
    qspi_write32(0x000000AB);
    id = qspi_read8();
    qspi_cs_disable();

    while (flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    return id;
}

/*
 * Get auto mode state of controller.
 */
QSPI_SECTION static inline bool qspi_get_automode(void)
{
    return REG_GETF(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_AUTO_MD);
}

/*
 * Set auto or manual mode
 */
QSPI_SECTION static inline void qspi_set_automode(bool automode)
{
    REG_SETF(QSPIC, QSPIC_CTRLMODE_REG, QSPIC_AUTO_MD, automode);
}

/*
 * Write bytes on QSPI bus.
 */
QSPI_SECTION static void qspi_write(const uint8_t *wbuf, size_t wlen)
{
    size_t i;

    qspi_cs_enable();

    for (i = 0; i < wlen; ++i)
    {
        qspi_write8(wbuf[i]);
    }

    qspi_cs_disable();
}

/*
 * Do write then read transaction on QSPI bus.
 */
QSPI_SECTION static void qspi_transact(const uint8_t *wbuf, size_t wlen, uint8_t *rbuf,
                                       size_t rlen)
{
    size_t i;

    qspi_cs_enable();

    for (i = 0; i < wlen; ++i)
    {
        qspi_write8(wbuf[i]);
    }

    for (i = 0; i < rlen; ++i)
    {
        rbuf[i] = qspi_read8();
    }

    qspi_cs_disable();
}


/*
 * Enable write to flash.
 * Must be called before every write and erase.
 */
QSPI_SECTION static void flash_write_enable(void)
{
    uint8_t status;
    uint8_t cmd[] = { W25Q_WRITE_ENABLE };

    do
    {
        qspi_write(cmd, 1);

        do
        {
            status = flash_read_status_register_1();
        }
        while (status & W25Q_STATUS1_BUSY_MASK);
    }
    while (!(status & W25Q_STATUS1_WEL_MASK));
}

QSPI_SECTION static size_t flash_write_page(uint32_t addr, const uint8_t *buf, uint16_t size)
{
    size_t i = 0;
    size_t odd = ((uint32_t) buf) & 3;

    flash_write_enable();

    qspi_cs_enable();

    qspi_write32((QUAD_MODE ? W25Q_QUAD_PAGE_PROGRAM : W25Q_PAGE_PROGRAM) |
                 ((addr >> 8) & 0xFF00) | ((addr << 8) & 0xFF0000) | (addr << 24));

    /* Reduce max write size, that can reduce interrupt latency time */
    if (size > FLASH_MAX_WRITE_SIZE)
    {
        size = FLASH_MAX_WRITE_SIZE;
    }

    /* Make sure write will not cross page boundary */
    if ((addr & 0xFF) + size > 256)
    {
        size = 256 - ((addr + 256) & 0xFF);
    }

#if QUAD_MODE
    qspi_set_bus_mode(HW_QSPI_BUS_MODE_QUAD);
#endif

    if (odd)
    {
        odd = 4 - odd;

        for (i = 0; i < odd && i < size; ++i)
        {
            qspi_write8(buf[i]);
        }
    }

    for (; i + 3 < size; i += 4)
    {
        qspi_write32(*((uint32_t *) &buf[i]));
    }

    for (; i < size; i++)
    {
        qspi_write8(*((uint8_t *) &buf[i]));
    }

#if QUAD_MODE
    qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);
#endif

    qspi_cs_disable();

    return i;
}

/*
 * Read flash status register.
 * It is important not to go to auto mode before flash finishes write or erase.
 */
QSPI_SECTION static uint8_t flash_read_status_register_1(void)
{
    uint8_t status;
    uint8_t cmd[] = { W25Q_READ_STATUS_REGISTER1 };

    qspi_transact(cmd, 1, &status, 1);

    return status;
}

#if !ERASE_IN_AUTOMODE
/*
 * Erase flash sector 4KB command.
 */
QSPI_SECTION static void flash_erase_sector(uint32_t addr)
{
    uint8_t cmd[4] = { W25Q_SECTOR_ERASE, addr >> 16, addr >>  8, addr };

    flash_write_enable();

    qspi_write(cmd, 4);
}
#endif

QSPI_SECTION static inline uint8_t qspi_get_erase_status(void)
{
    QSPIC->QSPIC_CHCKERASE_REG = 0;
    return HW_QSPIC_REG_GETF(ERASECTRL, ERS_STATE);
}

QSPI_SECTION static inline HW_QSPI_ADDR_SIZE qspi_get_address_size(void)
{
    return (HW_QSPI_ADDR_SIZE)HW_QSPIC_REG_GETF(CTRLMODE, USE_32BA);
}

QSPI_SECTION_NOINLINE static bool qspi_writable(void)
{
    bool am = qspi_get_automode();
    bool writable;

    /*
     * From now on QSPI may not be available, turn of interrupts.
     */
    GLOBAL_INT_DISABLE();

    /*
     * Turn off auto mode to allow write.
     */
    qspi_set_automode(false);

    /*
     * Set default mode for commands.
     */
    qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

    /*
     * Exit continues mode, after this flash will interpret commands again.
     */
    flash_reset_continuous_mode();

    /*
     * Check if flash is ready.
     */
    writable = !(flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    /*
     * Restore auto mode.
     */
    qspi_set_automode(am);

    /*
     * Let other code to be executed including QSPI one.
     */
    GLOBAL_INT_RESTORE();

    return writable;
}

/*
 * Erase sector using QSPI controller.
 */
QSPI_SECTION static void qspi_erase_sector(uint32_t addr)
{
    /* Wait for previous erase to end */
    while (qspi_get_erase_status() != 0)
    {
    }

    if (qspi_get_address_size() == HW_QSPI_ADDR_SIZE_32)
    {
        addr >>= 12;
    }
    else
    {
        addr >>= 4;
    }

    /* Setup erase block page */
    HW_QSPIC_REG_SETF(ERASECTRL, ERS_ADDR, addr);
    /* Fire erase */
    HW_QSPIC_REG_SETF(ERASECTRL, ERASE_EN, 1);
}

/*
 * Write page to flash.
 * This function blocks interrupts switches to manual mode, writes page (or less) of data.
 * Then it waits for flash to become ready before switching back to auto mode.
 * Make sure that buf does not point to QSPI mapped memory.
 */
QSPI_SECTION_NOINLINE static size_t write_page(uint32_t addr, const uint8_t *buf, uint16_t size)
{
    bool am = qspi_get_automode();
    size_t written;

    /*
     * From now on QSPI may not be available, turn of interrupts.
     */
    GLOBAL_INT_DISABLE();

    /*
     * Turn off auto mode to allow write.
     */
    qspi_set_automode(false);

    /*
     * Set default mode for commands.
     */
    qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

    /*
     * Exit continues mode, after this flash will interpret commands again.
     */
    flash_reset_continuous_mode();

    while (flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    /*
     * Write page of data.
     */
    written = flash_write_page(addr, buf, size);

    /*
     * Wait for flash to become ready again.
     */
    while (flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    /*
     * Restore auto mode.
     */
    qspi_set_automode(am);

    /*
     * Let other code to be executed including QSPI one.
     */
    GLOBAL_INT_RESTORE();

    return written;
}

/*
 * Erase flash sector.
 * This function blocks interrupts switches to manual mode, erases sector.
 * Then it waits for flash to become ready before switching back to auto mode.
 */
QSPI_SECTION_NOINLINE static void erase_sector(uint32_t addr)
{
#if ERASE_IN_AUTOMODE
    /*
     * Erase sector in automode
     */
    qspi_erase_sector(addr);

    /*
     * Wait for erase to finish
     */
    while (qspi_get_erase_status())
    {
    }

#else
    bool am = qspi_get_automode();

    /*
     * From now on QSPI may not be available, turn of interrupts.
     */
    GLOBAL_INT_DISABLE();

    /*
     * Turn off auto mode to allow write.
     */
    qspi_set_automode(false);

    /*
     * Set default mode for commands.
     */
    qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

    flash_release_power_down();

    /*
     * Exit continues mode, after this flash will interpret commands again.
     */
    flash_reset_continuous_mode();

    while (flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    /*
     * Execute erase command.
     */
    flash_erase_sector(addr);

    /*
     * Wait for flash to become ready again.
     */
    while (flash_read_status_register_1() & W25Q_STATUS1_BUSY_MASK);

    /*
     * Restore auto mode.
     */
    qspi_set_automode(am);

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
    while (!qspi_automode_writable())
    {
    }

    return write_page(addr, buf, size);
}

void qspi_automode_erase_flash_sector(uint32_t addr)
{
    while (!qspi_automode_writable())
    {
    }

    erase_sector(addr);
}

size_t qspi_automode_read(uint32_t addr, uint8_t *buf, size_t len)
{
    memcpy(buf, (void *)(MEMORY_QSPIF_BASE + addr), len);

    return len;
}

void qspi_automode_flash_power_up(void)
{
    if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
    {
        bool am = qspi_get_automode();

        /*
         * Turn off auto mode to allow write.
         */
        qspi_set_automode(false);

        /*
         * Set default mode for commands.
         */
        qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);

        flash_release_power_down();

        /*
         * Restore auto mode.
         */
        qspi_set_automode(am);
    }
}

static const qspi_config qspi_cfg =
{
    HW_QSPI_ADDR_SIZE_24, HW_QSPI_POL_HIGH, HW_QSPI_SAMPLING_EDGE_NEGATIVE
};

bool qspi_automode_init(void)
{
    hw_qspi_enable_clock();

    /*
     * Setup erase instruction that will be sent by QSPI controller to erase sector in automode.
     */
    hw_qspi_set_erase_instruction(W25Q_SECTOR_ERASE, HW_QSPI_BUS_MODE_SINGLE,
                                  HW_QSPI_BUS_MODE_SINGLE, 8, 5);
    /*
     * Setup instruction pair that will temporarily suspend erase operation to allow read.
     */
    hw_qspi_set_suspend_resume_instructions(W25Q_ERASE_PROGRAM_SUSPEND, HW_QSPI_BUS_MODE_SINGLE,
                                            W25Q_ERASE_PROGRAM_RESUME, HW_QSPI_BUS_MODE_SINGLE, 7);

    /*
     * QSPI controller must send write enable before erase, this sets it up.
     */
    hw_qspi_set_write_enable_instruction(W25Q_WRITE_ENABLE, HW_QSPI_BUS_MODE_SINGLE);

    /*
     * Setup instruction that will be used to periodically check erase operation status.
     * Check LSB which is 1 when erase is in progress.
     */
    hw_qspi_set_read_status_instruction(W25Q_READ_STATUS_REGISTER1, HW_QSPI_BUS_MODE_SINGLE,
                                        HW_QSPI_BUS_MODE_SINGLE, W25Q_STATUS1_BUSY_BIT, 1, 20, 0);

    /*
     * This sequence is necessary if flash is working in continuous read mode, when instruction
     * is not sent on every read access just address. Sending 0xFFFF will exit this mode.
     * This sequence is sent only when QSPI is working in automode and decides to send one of
     * instructions above.
     * If flash is working in DUAL bus mode sequence should be 0xFFFF and size should be
     * HW_QSPI_BREAK_SEQ_SIZE_2B.
     */
    hw_qspi_set_break_sequence(0xFFFF, HW_QSPI_BUS_MODE_SINGLE, HW_QSPI_BREAK_SEQ_SIZE_1B, 0);

    /*
     * If application starts from FLASH then bootloader must have set read instruction.
     */
    if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
    {
        hw_qspi_init(&qspi_cfg);
        hw_qspi_set_div(HW_QSPI_DIV_1);
        flash_reset_continuous_mode();
        hw_qspi_set_read_instruction(W25Q_FAST_READ_QUAD, 1, 2, HW_QSPI_BUS_MODE_SINGLE,
                                     HW_QSPI_BUS_MODE_QUAD, HW_QSPI_BUS_MODE_QUAD,
                                     HW_QSPI_BUS_MODE_QUAD);

        hw_qspi_set_extra_byte(0xA0, HW_QSPI_BUS_MODE_QUAD, 1);

        hw_qspi_set_automode(true);
    }

    HW_QSPIC_REG_SETF(BURSTCMDB, CS_HIGH_MIN, 1);

    return true;
}
