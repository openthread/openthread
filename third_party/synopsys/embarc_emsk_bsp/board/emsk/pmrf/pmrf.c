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
 * \date 2016-07-18
 * \author Qiang Gu(Qiang.Gu@synopsys.com)
--------------------------------------------- */

/**
 * \defgroup    BOARD_EMSK_DRV_PMODRF   EMSK PMOD RF Driver
 * \ingroup BOARD_EMSK_DRIVER
 * \brief   EMSK Pmod RF Driver
 * \details
 *      Realize the EMSK board PMOD RF driver
 */

/**
 * \file
 * \ingroup BOARD_EMSK_DRV_PMODRF
 * \brief   Pmod RF driver for emsk board
 */

/**
 * \addtogroup  BOARD_EMSK_DRV_PMODRF
 * @{
 */
#include <stddef.h>
#include <string.h>

#include "inc/arc/arc.h"
#include "inc/arc/arc_builtin.h"
#include "inc/embARC_toolchain.h"
#include "inc/embARC_error.h"
#include "inc/arc/arc_exception.h"

#include "board/board.h"

#include "device/device_hal/inc/dev_gpio.h"
#include "device/device_hal/inc/dev_spi.h"

#include "board/emsk/pmrf/pmrf.h"

#define EMSK_PMRF_0_SPI_CPULOCK_ENABLE

static DEV_SPI_PTR pmrf_spi_ptr;
static DEV_GPIO_PTR pmrf_gpio_ptr;
static uint32_t cs_line = EMSK_PMRF_0_SPI_LINE;

void pmrf_wake_pin(uint32_t flag)
{
    if (flag == 1)
    {
        pmrf_gpio_ptr->gpio_write(MRF24J40_WAKE_ON, MRF24J40_WAKE_PIN);
    }
    else
    {
        pmrf_gpio_ptr->gpio_write(MRF24J40_WAKE_OFF, MRF24J40_WAKE_PIN);
    }
}

void pmrf_cs_pin(uint32_t flag)
{
    uint32_t cpu_status;

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif

    if (flag == 1)
    {
        pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));

    }
    else
    {
        pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    }

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif
}

void pmrf_reset_pin(uint32_t flag)
{
    if (flag == 1)
    {
        pmrf_gpio_ptr->gpio_write(MRF24J40_RST_HIGH, MRF24J40_RST_PIN);
    }
    else
    {
        pmrf_gpio_ptr->gpio_write(MRF24J40_RST_LOW, MRF24J40_RST_PIN);
    }
}

uint8_t pmrf_read_long_ctrl_reg(uint16_t addr)
{
    uint8_t msg[2];
    uint8_t ret_val;
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;

    msg[0] = (uint8_t)(((addr >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)((addr << 5) & 0xE0);

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, 2);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, &ret_val, 2, 1);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif

    return ret_val;
}

uint8_t pmrf_read_short_ctrl_reg(uint8_t addr)
{
    uint8_t msg;
    uint8_t ret_val;
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;

    msg = (((addr << 1) & 0x7E) | 0);

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, &msg, 0, 1);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, &ret_val, 1, 1);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif

    return ret_val;

}

void pmrf_write_long_ctrl_reg(uint16_t addr, uint8_t value)
{
    uint8_t msg[4];
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;

    msg[0] = (uint8_t)(((addr >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)(((addr << 5) & 0xE0) | (1 << 4));
    msg[2] = value;
    msg[3] = 0x0;   /* have to write 1 more byte, why ?*/

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, 4);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, NULL, 4, 0);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif
}

void pmrf_write_short_ctrl_reg(uint8_t addr, uint8_t value)
{
    DEV_SPI_TRANSFER pmrf_xfer;
    uint8_t msg[3];
    uint32_t cpu_status;

    msg[0] = (((addr << 1) & 0x7E) | 1);
    msg[1] = value;
    msg[2] = 0x0;   /* have to write 1 more byte, why ?*/

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, 3);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, NULL, 3, 0);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif

}

void pmrf_delay_us(uint32_t us)
{
    uint64_t start_us, us_delayed;

    us_delayed = (uint64_t)us;
    start_us = board_get_cur_us();

    while ((board_get_cur_us() - start_us) < us_delayed);
}

void pmrf_delay_ms(uint32_t ms)
{
    board_delay_ms(ms, OSP_DELAY_OS_COMPAT_DISABLE);
}

/* need test */
void pmrf_set_key(uint16_t addr, uint8_t *key)
{
    uint8_t msg[16 + 2];
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;
    uint8_t i;

    msg[0] = (uint8_t)(((addr >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)(((addr << 5) & 0xE0) | (1 << 4));

    for (i = 0; i < 16; i++)
    {
        msg[i + 2] = key[0];
    }

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, 18);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, NULL, 18, 0);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif
}

/* need test */
void pmrf_txpkt_frame_write(uint8_t *frame, int16_t hdr_len, int16_t frame_len)
{
    uint8_t msg[frame_len + 2 + 2];
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;
    uint8_t i = 0;

    msg[0] = (uint8_t)(((MRF24J40_TXNFIFO >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)(((MRF24J40_TXNFIFO << 5) & 0xE0) | (1 << 4));
    msg[2] = (uint8_t)hdr_len;
    msg[3] = (uint8_t)frame_len;

    for (i = 0; i < frame_len; i++)
    {
        msg[i + 4] = *frame;
        frame++;
    }

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, frame_len + 2 + 2);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, NULL, frame_len + 2 + 2, 0);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif
}

/* need test */
void pmrf_rxpkt_intcb_frame_read(uint8_t *buf, uint8_t length)
{
    uint8_t msg[length];
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;

    msg[0] = (uint8_t)((((MRF24J40_RXFIFO + 1) >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)(((MRF24J40_RXFIFO + 1) << 5) & 0xE0);

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, 2);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, buf, 2, length);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);

#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif

}

void pmrf_all_install(void)
{
    pmrf_spi_ptr = spi_get_dev(EMSK_PMRF_0_SPI_ID);
    pmrf_gpio_ptr = gpio_get_dev(EMSK_PMRF_0_GPIO_ID);
}

void pmrf_txfifo_write(uint16_t address, uint8_t *data, uint8_t hdr_len, uint8_t len)
{
    uint8_t msg[len + 4];
    DEV_SPI_TRANSFER pmrf_xfer;
    uint32_t cpu_status;
    uint8_t i = 0;

    msg[0] = (uint8_t)(((address >> 3) & 0x7F) | 0x80);
    msg[1] = (uint8_t)(((address << 5) & 0xE0) | (1 << 4));
    msg[2] = hdr_len;
    msg[3] = len;

    for (i = 0; i < len; i++)
    {
        msg[i + 4] = *(data + i);
    }

    DEV_SPI_XFER_SET_TXBUF(&pmrf_xfer, msg, 0, len + 4);
    DEV_SPI_XFER_SET_RXBUF(&pmrf_xfer, NULL, len + 4, 0);
    DEV_SPI_XFER_SET_NEXT(&pmrf_xfer, NULL);


#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_status = cpu_lock_save();
#endif
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_SEL_DEV, CONV2VOID(cs_line));
    pmrf_spi_ptr->spi_control(SPI_CMD_TRANSFER_POLLING, CONV2VOID(&pmrf_xfer));
    pmrf_spi_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, CONV2VOID(cs_line));
#ifdef EMSK_PMRF_0_SPI_CPULOCK_ENABLE
    cpu_unlock_restore(cpu_status);
#endif

}

/** @} end of group BOARD_EMSK_DRV_PMODRF */