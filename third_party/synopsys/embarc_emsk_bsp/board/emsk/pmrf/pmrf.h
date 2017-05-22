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
 * \file
 * \ingroup BOARD_EMSK_DRV_PMODRF
 * \brief   header file of Pmod MRF24J40MA driver for EMSK board
 */

/**
 * \addtogroup  BOARD_EMSK_DRV_PMODRF
 * @{
 */

#ifndef _PMRF_H_
#define _PMRF_H_

/** MRF24J40 Functions */

#define mrf24j40_reset_pin(val)     pmrf_reset_pin(val)
#define mrf24j40_wake_pin(val)      pmrf_wake_pin(val)
#define mrf24j40_cs_pin(val)        pmrf_cs_pin(val)

#define mrf24j40_delay_us(val)      pmrf_delay_us(val)
#define mrf24j40_delay_ms(val)      pmrf_delay_ms(val)


/** PMOD RF SPI FREQ & CLK MODE SETTINGS */
#define EMSK_PMRF_0_SPIFREQ     BOARD_SPI_FREQ
#define EMSK_PMRF_0_SPICLKMODE      BOARD_SPI_CLKMODE

/** PMOD RF SPI ID */
#define EMSK_PMRF_0_SPI_ID      DW_SPI_0_ID

/** Use J6 by default, MRF24J40 PIN */
#define EMSK_PMRF_0_SPI_LINE        EMSK_SPI_LINE_0
#define EMSK_PMRF_0_GPIO_ID     EMSK_GPIO_PORT_A
#define MRF24J40_WAKE_PIN       (1 << 30)
#define MRF24J40_RST_PIN        (1 << 29)
#define MRF24J40_INT_PIN        (1 << 28)

#define MRF24J40_WAKE_OFF       (0 << 30)
#define MRF24J40_WAKE_ON        (1 << 30)

#define MRF24J40_RST_LOW        (0 << 29)
#define MRF24J40_RST_HIGH       (1 << 29)

#define MRF24J40_INT_PIN_OFS        (28)

#ifdef __cplusplus
extern "C" {
#endif

extern void pmrf_reset_pin(uint32_t flag);
extern void pmrf_wake_pin(uint32_t flag);
extern void pmrf_cs_pin(uint32_t flag);

extern uint8_t pmrf_read_long_ctrl_reg(uint16_t addr);
extern uint8_t pmrf_read_short_ctrl_reg(uint8_t addr);
extern void pmrf_write_long_ctrl_reg(uint16_t addr, uint8_t value);
extern void pmrf_write_short_ctrl_reg(uint8_t addr, uint8_t value);
extern void pmrf_spi_write(uint8_t data);
extern void pmrf_delay_us(uint32_t us);
extern void pmrf_delay_ms(uint32_t ms);

extern void pmrf_set_key(uint16_t address, uint8_t *key);
extern void pmrf_txpkt_frame_write(uint8_t *frame, int16_t hdr_len, int16_t frame_len);
extern void pmrf_rxpkt_intcb_frame_read(uint8_t *buf, uint8_t length);

extern void pmrf_all_install(void);

extern void pmrf_txfifo_write(uint16_t address, uint8_t *data, uint8_t hdr_len, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* _PMRF_H_ */

/** @} end of group BOARD_EMSK_DRV_PMODRF */
