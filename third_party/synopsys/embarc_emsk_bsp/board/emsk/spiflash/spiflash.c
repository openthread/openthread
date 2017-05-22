/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \version 2017.03
 * \date 2014-07-17
 * \author Huaqi Fang(Huaqi.Fang@synopsys.com)
--------------------------------------------- */

/**
 * \defgroup    BOARD_EMSK_DRV_SPIFLASH EMSK SPI Flash Driver
 * \ingroup BOARD_EMSK_DRIVER
 * \brief   EMSK On-board SPI Flash Driver
 * \details
 *      Realize driver for on-board W25Q128BV spi flash using Designware spi driver.
 */

/**
 * \file
 * \ingroup BOARD_EMSK_DRV_SPIFLASH
 * \brief   emsk on-board W25Q128BV spi flash driver
 */

/**
 * \addtogroup  BOARD_EMSK_DRV_SPIFLASH
 * @{
 */
#include "board/emsk/spiflash/spiflash.h"

#include "inc/embARC_error.h"
#include "board/emsk/emsk.h"
#include "device/device_hal/inc/dev_spi.h"
#include "inc/arc/arc_exception.h"

/**
 * \name    SPI Flash Commands
 * @{
 */
#define RDID    0x9F    /*!<read chip ID */
#define RDSR    0x05    /*!< read status register */
#define WRSR    0x01    /*!< write status register */
#define WREN    0x06    /*!< write enablewaitDeviceReady */
#define WRDI    0x04    /*!< write disable */
#define READ    0x03    /*!< read data bytes */
#define SE  0x20    /*!< sector erase */
#define PP  0x02    /*!< page program */
/** @} end of name */

#define SPI_LINE_SFLASH     BOARD_SFLASH_SPI_LINE
#define SPI_ID_SFLASH       BOARD_SFLASH_SPI_ID

#define SPI_FLASH_FREQ      (BOARD_SPI_FREQ)
#define SPI_FLASH_CLKMODE   (BOARD_SPI_CLKMODE)

#define SPI_FLASH_NOT_VALID (0xFFFFFFFF)

static DEV_SPI *spi_flash;
static uint32_t cs_flash = SPI_LINE_SFLASH;

#define SPIFLASH_CHECK_EXP(EXPR, ERROR_CODE)        CHECK_EXP(EXPR, ercd, ERROR_CODE, error_exit)

/**
 * \brief   spi flash spi send command to operate spi flash
 * \param[in]   xfer    spi transfer that need to transfer to spi device
 * \retval  0   success
 * \retval  -1  fail
 */
Inline int32_t spi_send_cmd(DEV_SPI_TRANSFER *xfer)
{
    uint32_t cpu_status;
    int32_t ercd = 0;

    cpu_status = cpu_lock_save();

    /* select device */
    ercd = spi_flash->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_flash));
    SPIFLASH_CHECK_EXP(ercd == E_OK, -1);
    ercd = spi_flash->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(xfer));
    /* deselect device */
    spi_flash->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_flash));
    SPIFLASH_CHECK_EXP(ercd == E_OK, -1);

    cpu_unlock_restore(cpu_status);

    if (ercd == E_OK) { ercd = 0; }

error_exit:
    return ercd;
}

/**
 * \brief   init spi flash related interface
 * \retval  0   success
 * \retval  -1  fail
 */
int32_t flash_init(void)
{
    int32_t ercd = 0;

    spi_flash = spi_get_dev(SPI_ID_SFLASH);
    ercd = spi_flash->spi_open(DEV_MASTER_MODE, SPI_FLASH_FREQ);
    spi_flash->spi_control(SPI_CMD_MST_SET_FREQ, CONV2VOID(SPI_FLASH_FREQ));
    spi_flash->spi_control(SPI_CMD_SET_DUMMY_DATA, CONV2VOID(0xFF));
    spi_flash->spi_control(SPI_CMD_SET_CLK_MODE, CONV2VOID(SPI_FLASH_CLKMODE));

    if (ercd == E_OK) { ercd = 0; }

error_exit:
    return ercd;
}

int32_t flash_change_freq(uint32_t freq)
{
    spi_flash = spi_get_dev(SPI_ID_SFLASH);
    int32_t ercd = spi_flash->spi_control(SPI_CMD_MST_SET_FREQ, CONV2VOID(freq));
    return ercd;
}

/**
 * \brief   read spi flash identification ID
 * \return  the id of the spi flash
 */
uint32_t flash_read_id(void)
{
    uint32_t id = 0;
    uint8_t local_buf[4];
    DEV_SPI_TRANSFER cmd_xfer;

    local_buf[0] = RDID;

    DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 1);
    DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, local_buf, 1, 3);
    DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

    if (spi_send_cmd(&cmd_xfer) == 0)
    {
        id = (local_buf[0] << 16) | (local_buf[1] << 8) | local_buf[2];
    }
    else
    {
        id = SPI_FLASH_NOT_VALID;
    }

    return id;
}

/**
 * \brief   read the status of spi flash
 * \return  current status of spi flash
 */
uint32_t flash_read_status(void)
{

    uint8_t local_buf[2];
    DEV_SPI_TRANSFER cmd_xfer;

    local_buf[0] = RDSR;

    DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 1);
    DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, local_buf, 1, 1);
    DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

    if (spi_send_cmd(&cmd_xfer) == 0)
    {
        return (uint32_t)local_buf[0];
    }
    else
    {
        return SPI_FLASH_NOT_VALID;
    }
}

/**
 * \brief   read data from flash
 * \param[in]   address     read start address of spi flash
 * \param[in]   size        read size of spi flash
 * \param[out]  data        data to store the return data
 *
 * \retval  -1      failed in read operation
 * \retval  >=0     data size of data read
 */
int32_t flash_read(uint32_t address, uint32_t size, void *data)
{

    uint8_t local_buf[4];
    DEV_SPI_TRANSFER cmd_xfer;

    local_buf[0] = READ;
    local_buf[1] = (address >> 16) & 0xff;
    local_buf[2] = (address >> 8) & 0xff;
    local_buf[3] = address  & 0xff;

    DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 4);
    DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, data, 4, size);
    DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

    if (spi_send_cmd(&cmd_xfer) == 0)
    {
        return size;
    }
    else
    {
        return -1;
    }
}

/**
 * \brief   Read status and wait while busy flag is set
 * \retval  0   success
 * \retval  -1  fail
 */
int32_t flash_wait_ready(void)
{
    uint32_t status = 0x01;

    do
    {
        status = flash_read_status();

        if (status == SPI_FLASH_NOT_VALID)
        {
            return -1;
        }
    }
    while (status & 0x01);

    return 0;
}

/**
 * \brief   enable to write flash
 * \retval  0   success
 * \retval  -1  fail
 */
int32_t flash_write_enable(void)
{
    uint8_t local_buf[3];
    DEV_SPI_TRANSFER cmd_xfer;

    uint32_t status = 0;

    do
    {
        local_buf[0] = WREN;

        DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 1);
        DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, NULL, 0, 0);
        DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

        if (spi_send_cmd(&cmd_xfer) != 0)
        {
            return -1;
        }

        status = flash_read_status();

        if (status == SPI_FLASH_NOT_VALID)
        {
            return -1;
        }

        // clear protection bits
        //  Write Protect. and Write Enable.
        if ((status & 0xfc) && (status & 0x02))
        {
            local_buf[0] = WRSR; // write status
            local_buf[1] = 0x00; // write status
            local_buf[2] = 0x00; // write status

            DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 3);
            DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, NULL, 0, 0);
            DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

            if (spi_send_cmd(&cmd_xfer) != 0)
            {
                return -1;
            }

            status = 0;
        }
    }
    while (status != 0x02);

    return 0;
}


/**
 * \brief   flash erase in sectors
 *
 * \param[in]   address     erase start address of spi flash
 * \param[in]   size        erase size
 *
 * \retval  -1      failed in erase operation
 * \retval  >=0     sector count erased
 */
int32_t flash_erase(uint32_t address, uint32_t size)
{
    uint32_t last_address;
    uint32_t count = 0;
    uint8_t local_buf[4];
    DEV_SPI_TRANSFER cmd_xfer;

    // start address of last sector
    last_address = (address + size) & (~(FLASH_SECTOR_SIZE - 1));

    // start address of first sector
    address &= ~(FLASH_SECTOR_SIZE - 1);

    do
    {
        if (flash_write_enable() != 0)
        {
            return -1;
        }

        if (flash_wait_ready() != 0)
        {
            return -1;
        }

        local_buf[0] = SE;
        local_buf[1] = (address >> 16) & 0xff;
        local_buf[2] = (address >> 8) & 0xff;
        local_buf[3] =  address & 0xff;

        DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 4);
        DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, NULL, 0, 0);
        DEV_SPI_XFER_SET_NEXT(&cmd_xfer, NULL);

        if (spi_send_cmd(&cmd_xfer) != 0)
        {
            return -1;
        }

        address += FLASH_SECTOR_SIZE;
        count++;
    }
    while (address <= last_address);

    if (flash_wait_ready() != 0)
    {
        return -1;
    }

    return (int32_t)count;
}

/**
 * \brief   write data to spi flash
 *
 * \param[in]   address start address
 * \param[in]   size    data size
 * \param[in]   data    pointer to data
 *
 * \retval  >=0     written bytes number
 * \retval  <0      error
 */
int32_t flash_write(uint32_t address, uint32_t size, const void *data)
{

    uint8_t local_buf[4];
    DEV_SPI_TRANSFER cmd_xfer;
    DEV_SPI_TRANSFER data_xfer;

    uint32_t first = 0;
    uint32_t size_orig = size;

    if (flash_wait_ready() != 0)
    {
        return -1;
    }

    first = FLASH_PAGE_SIZE - (address & (FLASH_PAGE_SIZE - 1));

    do
    {
        // send write enable command to flash
        if (flash_write_enable() != 0)
        {
            return -1;
        }

        if (flash_wait_ready() != 0)
        {
            return -1;
        }

        first = first < size ? first : size;

        local_buf[0] = PP;
        local_buf[1] = (address >> 16) & 0xff;
        local_buf[2] = (address >> 8) & 0xff;
        local_buf[3] = address  & 0xff;

        DEV_SPI_XFER_SET_TXBUF(&data_xfer, data, 0, first);
        DEV_SPI_XFER_SET_RXBUF(&data_xfer, NULL, 0, 0);
        DEV_SPI_XFER_SET_NEXT(&data_xfer, NULL);

        DEV_SPI_XFER_SET_TXBUF(&cmd_xfer, local_buf, 0, 4);
        DEV_SPI_XFER_SET_RXBUF(&cmd_xfer, NULL, 0, 0);
        DEV_SPI_XFER_SET_NEXT(&cmd_xfer, &data_xfer);

        if (spi_send_cmd(&cmd_xfer) != 0)
        {
            return -1;
        }

        size -= first;
        address += first;
        data += first;
        first = FLASH_PAGE_SIZE;

    }
    while (size);

    if (flash_wait_ready() != 0)
    {
        return -1;
    }

    return (int32_t)(size_orig);
}

/** @} end of group BOARD_EMSK_DRV_SPIFLASH */
