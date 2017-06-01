/*
 * Copyright (C) 2014, Michele Balistreri
 *
 * Derived from code originally Copyright (C) 2011, Alex Hornung
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Modified by Qiang Gu(Qiang.Gu@synopsys.com) for embARC
 * \version 2017.03
 * \date 2016-07-29
 */

/**
 * \defgroup    BOARD_EMSK_DRV_RF_MRF24J40  EMSK RF-MRF24J40 Driver
 * \ingroup BOARD_EMSK_DRV_RF
 * \brief   EMSK RF MRF24J40 driver
 * \details
 *      Implement the MRF24J40 driver using the DesignWare SPI device driver.
 */

/**
 * \file
 * \ingroup BOARD_EMSK_DRV_RF_MRF24J40
 * \brief   RF MRF24J40 driver
 * \details Implement MRF24J40 driver.
 */

/**
 * \addtogroup  BOARD_EMSK_DRV_RF_MRF24J40
 * @{
 */

#include "board/emsk/pmrf/mrf24j40.h"

#include <stdlib.h>
#include "board/emsk/pmrf/pmrf.h"

/* mrf24j40_read_long_ctrl_reg(), mrf24j40_read_short_ctrl_reg(),
 * mrf24j40_write_long_ctrl_reg() and mrf24j40_write_short_ctrl_reg()
 * is modified for embARC */
/** Read register in long address memory space */
uint8_t mrf24j40_read_long_ctrl_reg(uint16_t addr)
{
    return pmrf_read_long_ctrl_reg(addr);
}

/** Read register in short address memory space */
uint8_t mrf24j40_read_short_ctrl_reg(uint8_t addr)
{
    return pmrf_read_short_ctrl_reg(addr);
}

/** Write register in long address memory space */
void mrf24j40_write_long_ctrl_reg(uint16_t addr, uint8_t value)
{
    pmrf_write_long_ctrl_reg(addr, value);
}

/** Write register in short address memory space */
void mrf24j40_write_short_ctrl_reg(uint8_t addr, uint8_t value)
{
    pmrf_write_short_ctrl_reg(addr, value);
}

/**
 * \brief   Set INTCON: Interrupt Control Register
 * \details Enable TX Normal FIFO transmission interrupt (TXNIE = 0),
 *          RX FIFO reception interrupt (RXIE = 0),
 *          security key request interrupt (SECIE = 0)
 */
void mrf24j40_ie(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_INTCON, ~(MRF24J40_TXNIE | MRF24J40_RXIE | MRF24J40_SECIE));
}

/**
 * \brief   Set SOFTRST: Software Reset Register
 * \details Reset power management circuitry (RSTPWR = 1)
 */
void mrf24j40_pwr_reset(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_SOFTRST, MRF24J40_RSTPWR);
}

/**
 * \brief   Set SOFTRST: Software Reset Register
 * \details Reset baseband circuitry (RSTBB = 1)
 */
void mrf24j40_bb_reset(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_SOFTRST, MRF24J40_RSTBB);
}

/**
 * \brief   Set SOFTRST: Software Reset Register
 * \details Reset MAC circuitry (RSTMAC = 1)
 */
void mrf24j40_mac_reset(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_SOFTRST, MRF24J40_RSTMAC);
}

/**
 * \brief   Set RFCTL: RF Mode Control Register
 * \details Perform RF Reset by setting RFRST = 1 and then RFRST = 0
 */
void mrf24j40_rf_reset(void)
{
    uint8_t old = mrf24j40_read_short_ctrl_reg(MRF24J40_RFCTL);

    mrf24j40_write_short_ctrl_reg(MRF24J40_RFCTL, old | MRF24J40_RFRST);
    mrf24j40_write_short_ctrl_reg(MRF24J40_RFCTL, old & ~MRF24J40_RFRST);
    mrf24j40_delay_ms(2);
}

/**
 * \brief   Get FPSTAT in TXNCON: Transmit Normal FIFO Control Register
 * \details Get status of the frame pending bit in the received Acknowledgement frame
 * \retval 1    Set frame pending bit
 * \retval 0    Clear frame pending bit
 */
uint8_t mrf24j40_get_pending_frame(void)
{
    return (mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON) >> 4) & 0x01;
}

/**
 * \brief   Set RXFLUSH in RXFLUSH: Receive FIFO Flush Register
 * \details Reset RXFIFO address pointer to zero
 */
void mrf24j40_rxfifo_flush(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_RXFLUSH, (mrf24j40_read_short_ctrl_reg(MRF24J40_RXFLUSH) | MRF24J40__RXFLUSH));
}

/**
 * \brief   Set CHANNEL in RFCON0: RF Control 0 Register
 * \details Set channel number
 * \param  ch   Channel number, 0000 = Channel 11 (2405 MHz) as default
 */
void mrf24j40_set_channel(int16_t ch)
{
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON0, MRF24J40_CHANNEL(ch) | MRF24J40_RFOPT(0x03));
    mrf24j40_rf_reset();
}

/**
 * \brief   Set PROMI in RXMCR: Receive MAC Control Register
 * \details Set promiscuous mode
 * \param  crc_check    1 = Discard packet when there is a MAC address mismatch,
 *          illegal frame type, dPAN/sPAN or MAC short address mismatch,
 *          0 = Receive all packet types with good CRC
 */
void mrf24j40_set_promiscuous(bool crc_check)
{
    uint8_t w = mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR);

    if (!crc_check)
    {
        w |= MRF24J40_PROMI;
    }
    else
    {
        w &= (~MRF24J40_PROMI);
    }

    mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, w);
}

/**
 * \brief   Set PANCOORD in RXMCR: Receive MAC Control Register
 * \details Set device as PAN coordinator
 */
void mrf24j40_set_coordinator(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) | MRF24J40_PANCOORD);
}

/**
 * \brief   Clear PANCOORD in RXMCR: Receive MAC Control Register
 * \details Device is not set as PAN coordinator
 */
void mrf24j40_clear_coordinator(void)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) & ~MRF24J40_PANCOORD);
}

/**
 * \brief   Set PAN ID in PANIDL and PANIDH
 * \param  *pan Pointer of PAN ID, pan[0] for low byte and pan[1] for high byte
 */
void mrf24j40_set_pan(uint8_t *pan)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_PANIDL, pan[0]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_PANIDH, pan[1]);
}

/**
 * \brief   Set short address in SADRL and SADRH
 * \param  *addr    Pointer of short address, addr[0] for low byte and addr[1] for high byte
 */
void mrf24j40_set_short_addr(uint8_t *addr)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_SADRL, addr[0]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_SADRH, addr[1]);
}

/**
 * \brief   Set extended address from EADR0 to EADR7
 * \param  *addr    Pointer of extended address, eui[0] to eui[7]
 */
/* Modified for embARC */
void mrf24j40_set_eui(uint8_t *eui)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR0, eui[0]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR1, eui[1]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR2, eui[2]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR3, eui[3]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR4, eui[4]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR5, eui[5]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR6, eui[6]);
    mrf24j40_write_short_ctrl_reg(MRF24J40_EADR7, eui[7]);
}

/**
 * \brief   Set associated coordinator short address in ASSOSADR0 and ASSOSADR1
 * \param  *addr    Pointer of associated coordinator short address
 */
/* Modified for embARC */
void mrf24j40_set_coordinator_short_addr(uint8_t *addr)
{
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOSADR0, addr[0]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOSADR1, addr[1]);
}

/**
 * \brief   Set associated coordinator extended address from ASSOEADR0 to ASSOEADR7
 * \param  *addr    Pointer of associated coordinator extended address, eui[0] to eui[7]
 */
/* Modified for embARC */
void mrf24j40_set_coordinator_eui(uint8_t *eui)
{
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR0, eui[0]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR1, eui[1]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR2, eui[2]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR3, eui[3]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR4, eui[4]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR5, eui[5]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR6, eui[6]);
    mrf24j40_write_long_ctrl_reg(MRF24J40_ASSOEADR7, eui[7]);
}

/**
 * \brief   Set key in address for SECKTXNFIFO and SECKRXFIFO
 * \param  address  TX normal FIFO security key (0x280) and RX FIFO security key (0x2B0)
 * \param  *key     Pointer of security key (16 bytes)
 */
/* Modified for embARC */
void mrf24j40_set_key(uint16_t address, uint8_t *key)
{
    pmrf_set_key(address, key);
}

/**
 * \brief   Perform hardware Reset by asserting RESET pin
 * \details The MRF24J40 will be released from Reset approximately 250 us after the RESET pin is released.
 */
void mrf24j40_hard_reset()
{
    mrf24j40_reset_pin(0);

    mrf24j40_delay_us(500);
    mrf24j40_reset_pin(1);
    mrf24j40_delay_us(500);
}

/**
 * \brief   Initialize MRF24J40
 */
/* Modified for embARC */
void mrf24j40_initialize()
{

    uint8_t softrst_status;
    uint8_t rf_state;

    pmrf_all_install();

    //mrf24j40_cs_pin(1);
    //mrf24j40_wake_pin(1);
    mrf24j40_wake_pin(1);

    mrf24j40_hard_reset();
    mrf24j40_write_short_ctrl_reg(MRF24J40_SOFTRST, (MRF24J40_RSTPWR | MRF24J40_RSTBB | MRF24J40_RSTMAC));
    mrf24j40_delay_us(500);

    do
    {
        softrst_status = mrf24j40_read_short_ctrl_reg(MRF24J40_SOFTRST);
    }
    while ((softrst_status & (MRF24J40_RSTPWR | MRF24J40_RSTBB | MRF24J40_RSTMAC)) != 0);

    mrf24j40_write_short_ctrl_reg(MRF24J40_PACON2, MRF24J40_FIFOEN | MRF24J40_TXONTS(0x18));
    mrf24j40_write_short_ctrl_reg(MRF24J40_TXSTBL, MRF24J40_RFSTBL(9) | MRF24J40_MSIFS(5));
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON0, MRF24J40_CHANNEL(0) | MRF24J40_RFOPT(0x03));
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON1, MRF24J40_VCOOPT(0x02)); // Initialize VCOOPT = 0x02
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON2, MRF24J40_PLLEN);
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON6, (MRF24J40_TXFIL | MRF24J40__20MRECVR));
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON7, MRF24J40_SLPCLKSEL(0x02));
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON8, MRF24J40_RFVCO);
    mrf24j40_write_long_ctrl_reg(MRF24J40_SLPCON1, MRF24J40_SLPCLKDIV(1) | MRF24J40_CLKOUTDIS);

    mrf24j40_write_short_ctrl_reg(MRF24J40_RXFLUSH, (MRF24J40_WAKEPAD | MRF24J40_WAKEPOL));

    mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) & (~MRF24J40_NOACKRSP));
    // mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) | MRF24J40_PANCOORD);
    // mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) | (MRF24J40_NOACKRSP));
    // mrf24j40_write_short_ctrl_reg(MRF24J40_ACKTMOUT, mrf24j40_read_short_ctrl_reg(MRF24J40_ACKTMOUT) | 0x7f);
    mrf24j40_write_short_ctrl_reg(MRF24J40_TXMCR, 0b00011100);
    // mrf24j40_set_coordinator();
    mrf24j40_write_short_ctrl_reg(MRF24J40_ORDER, 0xFF);

    mrf24j40_write_short_ctrl_reg(MRF24J40_BBREG1, 0x0);
    mrf24j40_write_short_ctrl_reg(MRF24J40_BBREG2, MRF24J40_CCAMODE(0x02) | MRF24J40_CCASTH(0x00));
    mrf24j40_write_short_ctrl_reg(MRF24J40_CCAEDTH, 0x60);
    mrf24j40_write_short_ctrl_reg(MRF24J40_BBREG6, MRF24J40_RSSIMODE2);

    // TURNTIME Defaule value: 0x4 and TURNTIME=0x3
    mrf24j40_write_short_ctrl_reg(MRF24J40_TXTIME, 0x30);

    //mrf24j40_write_short_ctrl_reg(MRF24J40_TXPEND, mrf24j40_read_short_ctrl_reg(MRF24J40_TXPEND) & MRF24J40_FPACK);

    //mrf24j40_write_long_ctrl_reg(MRF24J40_SLPCON0, MRF24J40_SLPCLKEN);

    // mrf24j40_write_short_ctrl_reg(MRF24J40_ACKTMOUT, mrf24j40_read_short_ctrl_reg(MRF24J40_ACKTMOUT) | MRF24J40_DRPACK);

    // mrf24j40_write_short_ctrl_reg(MRF24J40_RXMCR, mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) | MRF24J40_PROMI);

    mrf24j40_ie();
    mrf24j40_write_long_ctrl_reg(MRF24J40_RFCON3, 0x0);

    mrf24j40_rf_reset();

    do
    {
        rf_state = mrf24j40_read_long_ctrl_reg(MRF24J40_RFSTATE);
    }
    while (((rf_state >> 5) & 0x05) != 0x05);

    mrf24j40_rxfifo_flush();

}

/**
 * \brief   Set low-current Sleep mode
 */
void mrf24j40_sleep()
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_WAKECON, MRF24J40_IMMWAKE);

    uint8_t r = mrf24j40_read_short_ctrl_reg(MRF24J40_SLPACK);
    mrf24j40_wake_pin(0);

    mrf24j40_pwr_reset();
    mrf24j40_write_short_ctrl_reg(MRF24J40_SLPACK, r | MRF24J40__SLPACK);
}

/**
 * \brief   Set wake-up from Sleep mode
 */
void mrf24j40_wakeup()
{
    mrf24j40_wake_pin(1);
    mrf24j40_rf_reset();
}

/**
 * \brief   Enable and transmit frame in TX Normal FIFO
 * \param  *frame   Pointer of the TX frame
 * \param  hdr_len  Header Legnth of the transmission packet
 * \param  sec_hdr_len  Security header Legnth of the transmission packet
 * \param  payload_len  Data payload of the transmission packet
 */
/* Modified for embARC */
void mrf24j40_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len)
{
    int16_t frame_len = hdr_len + sec_hdr_len + payload_len;

    uint8_t w = mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON);
    w &= ~(MRF24J40_TXNSECEN);

    if (IEEE_802_15_4_HAS_SEC(frame[0]))
    {
        w |= MRF24J40_TXNSECEN;
    }

    if (IEEE_802_15_4_WANTS_ACK(frame[0]))
    {
        w |= MRF24J40_TXNACKREQ;
    }

    pmrf_txpkt_frame_write(frame, hdr_len, frame_len);

    mrf24j40_write_short_ctrl_reg(MRF24J40_TXNCON, w | MRF24J40_TXNTRIG);
}

/**
 * \brief   Set RXCIPHER and TXNCIPHER in SECCON0: Security Control 0 Register
 * \param  rxcipher RX FIFO security suite select bits
 * \param  txcipher TX normal FIFO security suite select bits
 * \details 111 = AES-CBC-MAC-32
 *      110 = AES-CBC-MAC-64
 *      101 = AES-CBC-MAC-128
 *      100 = AES-CMM-32
 *      011 = AES-CMM-64
 *      010 = AES-CMM-128
 *      001 = AES-CTR
 *      000 = None (default)
 */
void mrf24j40_set_cipher(uint8_t rxcipher, uint8_t txcipher)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_SECCON0, MRF24J40_RXCIPHER(rxcipher) | MRF24J40_TXNCIPHER(txcipher));
}

/**
 * \brief   Read SECDECERR in RXSR: RX MAC Status Register
 * \details Read security decryption error bit
 * \retval 0    Security decryption error occurred
 * \retval 1    Security decryption error did not occur
 */
// Should be tested
bool mrf24j40_rx_sec_fail()
{
    bool rx_sec_fail = (mrf24j40_read_short_ctrl_reg(MRF24J40_RXSR) >> 2) & 0x01;
    mrf24j40_write_short_ctrl_reg(MRF24J40_RXSR, 0x00);
    return rx_sec_fail;
}

/**
 * \brief   Set SECIGNORE/SECSTART in SECCON0: Security Control 0 Register
 * \details Set RX security decryption ignore/start bit
 * \param  accept   1: start decryption process, 0: ignore decryption process
 */
void mrf24j40_sec_intcb(bool accept)
{
    uint8_t w = mrf24j40_read_short_ctrl_reg(MRF24J40_SECCON0);

    w |= accept ? MRF24J40_SECSTART : MRF24J40_SECIGNORE;
    mrf24j40_write_short_ctrl_reg(MRF24J40_SECCON0, w);
}

/**
 * \brief   Read TX normal FIFO release status bit in TXSTAT: TX MAC Status Register
 * \retval MRF24J40_EBUSY   Failed, channel busy
 * \retval MRF24J40_EIO     Failed, channel idle
 * \retval 0            Succeeded
 */
int16_t mrf24j40_txpkt_intcb(void)
{
    uint8_t stat = mrf24j40_read_short_ctrl_reg(MRF24J40_TXSTAT);

    if (stat & MRF24J40_TXNSTAT)
    {
        if (stat & MRF24J40_CCAFAIL)
        {
            return MRF24J40_EBUSY;
        }
        else
        {
            return MRF24J40_EIO;
        }
    }
    else
    {
        return 0;
    }
}

/**
 * \brief   Send RX packet in RX FIFO
 * \param  *buf     Pointer of RX buffer in RX FIFO
 * \param  *plqi    Pointer of LQI in RX FIFO
 * \param  *prssi   Pointer of RSSI in RX FIFO
 * \returns Frame length in RX FIFO
 */
/* Modified for embARC */
int16_t mrf24j40_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi)
{
    mrf24j40_write_short_ctrl_reg(MRF24J40_BBREG1, mrf24j40_read_short_ctrl_reg(MRF24J40_BBREG1) | MRF24J40_RXDECINV);

    /* Packet Reception
     * Frame Length -- Header -- Data Payload -- FCS -- LQI -- RSSI
     */
    uint8_t flen = mrf24j40_read_long_ctrl_reg(MRF24J40_RXFIFO);

    if (flen > (MRF24J40_RXFIFO_SIZE - 3))
    {
        flen = MRF24J40_RXFIFO_SIZE - 3;
    }

    pmrf_rxpkt_intcb_frame_read(buf, flen);

    uint8_t lqi = mrf24j40_read_long_ctrl_reg(MRF24J40_RXFIFO + flen + 1);
    uint8_t rssi = mrf24j40_read_long_ctrl_reg(MRF24J40_RXFIFO + flen + 2);

    if (plqi != NULL)
    {
        *plqi = lqi;
    }

    if (prssi != NULL)
    {
        *prssi = rssi;
    }

    mrf24j40_rxfifo_flush();
    mrf24j40_write_short_ctrl_reg(MRF24J40_BBREG1, mrf24j40_read_short_ctrl_reg(MRF24J40_BBREG1) & ~MRF24J40_RXDECINV);

    return (int16_t)flen;
}

/**
 * \brief   Read SECIF, TXNIF and RXIF in INTSTAT: Interrupt Status Register
 * \details Read security key request interrupt bit, TX normal FIFO release interrupt bit
 *      and RX FIFO reception interrupt bit
 * \returns Interrupt status
 */
int16_t mrf24j40_int_tasks(void)
{
    int16_t ret = 0;

    uint8_t stat = mrf24j40_read_short_ctrl_reg(MRF24J40_INTSTAT);

    if (stat & MRF24J40_RXIF)
    {
        ret |= MRF24J40_INT_RX;
    }

    if (stat & MRF24J40_TXNIF)
    {
        ret |= MRF24J40_INT_TX;
    }

    if (stat & MRF24J40_SECIF)
    {
        ret |= MRF24J40_INT_SEC;
    }

    return ret;
}

/**
 * \brief   Write TX FIFO
 * \param  address  FIFO Address (TX Normal FIFO)
 * \param  *data    Pointer of data for TX FIFO
 * \param  hdr_len  Header legnth for TX FIFO
 * \param  len      Frame length for TX FIFO
 */
void mrf24j40_txfifo_write(uint16_t address, uint8_t *data, uint8_t hdr_len, uint8_t len)
{
    pmrf_txfifo_write(address, data, hdr_len, len);
}

/**
 * \brief   Set TXNTRIG in TXNCON: Transmit normal FIFO control register
 * \details Transmit frame in TX normal FIFO bit
 */
void mrf24j40_transmit(void)
{
    uint8_t reg = pmrf_read_short_ctrl_reg(MRF24J40_TXNCON) | MRF24J40_TXNTRIG;

    pmrf_write_short_ctrl_reg(MRF24J40_TXNCON, reg);
}

/** @} end of group BOARD_EMSK_DRV_RF_MRF24J40 */