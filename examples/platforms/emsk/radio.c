/*
 *  Copyright (c) 2017, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */

#include "openthread/types.h"

#include <utils/code_utils.h>
#include "openthread/platform/radio.h"
#include "platform-emsk.h"

#include "device/device_hal/inc/dev_gpio.h"
#include <string.h>

#include <stdio.h>
#define DBG(fmt, ...)   printf(fmt, ##__VA_ARGS__)

enum
{
    IEEE802154_MIN_LENGTH         = 5,
    IEEE802154_MAX_LENGTH         = 127,
    IEEE802154_ACK_LENGTH         = 5,

    IEEE802154_BROADCAST          = 0xffff,

    IEEE802154_FRAME_TYPE_ACK     = 2 << 0,
    IEEE802154_FRAME_TYPE_MACCMD  = 3 << 0,
    IEEE802154_FRAME_TYPE_MASK    = 7 << 0,

    IEEE802154_SECURITY_ENABLED   = 1 << 3,
    IEEE802154_FRAME_PENDING      = 1 << 4,
    IEEE802154_ACK_REQUEST        = 1 << 5,
    IEEE802154_PANID_COMPRESSION  = 1 << 6,

    IEEE802154_DST_ADDR_NONE      = 0 << 2,
    IEEE802154_DST_ADDR_SHORT     = 2 << 2,
    IEEE802154_DST_ADDR_EXT       = 3 << 2,
    IEEE802154_DST_ADDR_MASK      = 3 << 2,

    IEEE802154_SRC_ADDR_NONE      = 0 << 6,
    IEEE802154_SRC_ADDR_SHORT     = 2 << 6,
    IEEE802154_SRC_ADDR_EXT       = 3 << 6,
    IEEE802154_SRC_ADDR_MASK      = 3 << 6,

    IEEE802154_DSN_OFFSET         = 2,
    IEEE802154_DSTPAN_OFFSET      = 3,
    IEEE802154_DSTADDR_OFFSET     = 5,

    IEEE802154_SEC_LEVEL_MASK     = 7 << 0,

    IEEE802154_KEY_ID_MODE_0      = 0 << 3,
    IEEE802154_KEY_ID_MODE_1      = 1 << 3,
    IEEE802154_KEY_ID_MODE_2      = 2 << 3,
    IEEE802154_KEY_ID_MODE_3      = 3 << 3,
    IEEE802154_KEY_ID_MODE_MASK   = 3 << 3,

    IEEE802154_MACCMD_DATA_REQ    = 4
};

enum
{
    MRF24J40_RSSI_OFFSET = 90,
    MRF24J40_RSSI_SLOPE = 5
};

static void radioTransmitMessage(otInstance *aInstance);

static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static RadioPacket sAckFrame;

static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[IEEE802154_MAX_LENGTH];
static uint8_t sReceivePsdu[IEEE802154_MAX_LENGTH];
static uint8_t sAckPsdu[IEEE802154_MAX_LENGTH];

static PhyState sState = kStateDisabled;
static bool sIsReceiverEnabled = false;

static volatile uint8_t Mrf24StatusTx = 0;
static volatile uint8_t Mrf24StatusRx = 0;
static volatile uint8_t Mrf24StatusSec = 0;

static DEV_SPI_PTR pmrf_spi_ptr;
static DEV_GPIO_PTR pmrf_gpio_ptr;
static void RadioIsr(void *ptr);

/* Variables for test */
static uint32_t numInterruptRev = 0;
static uint32_t numInterruptTrans = 0;
static uint32_t numRadioProcess = 0;
static uint32_t NODE_ID = 1;

static inline bool isSecurityEnabled(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_SECURITY_ENABLED) != 0;
}

static inline bool isAckRequested(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_ACK_REQUEST) != 0;
}

static inline bool isPanIdCompressed(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_PANID_COMPRESSION) != 0;
}

static inline uint8_t getHeadLength(const uint8_t *frame)
{
    uint8_t length = 0;
    /* Frame Control-2 + Sequence Number-1 */
    length += 2 + 1;

    /* Destination PAN + Address */
    switch (frame[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_SHORT:
        length += sizeof(otPanId) + sizeof(otShortAddress);
        break;

    case IEEE802154_DST_ADDR_EXT:
        length += sizeof(otPanId) + sizeof(otExtAddress);
        break;

    default:
        length = 0;
        goto exit;
    }

    /* Source PAN + Address */
    switch (frame[1] & IEEE802154_SRC_ADDR_MASK)
    {
    case IEEE802154_SRC_ADDR_SHORT:
        if (!isPanIdCompressed(frame))
        {
            length += sizeof(otPanId);
        }

        length += sizeof(otShortAddress);
        break;

    case IEEE802154_SRC_ADDR_EXT:
        if (!isPanIdCompressed(frame))
        {
            length += sizeof(otPanId);
        }

        length += sizeof(otExtAddress);
        break;

    default:
        length = 0;
        goto exit;
    }

exit:
    return length;
}

void enableReceiver(void)
{
    if (!sIsReceiverEnabled)
    {
        mrf24j40_rxfifo_flush();
        /* add code to enable receiver (wakeup) */
        sIsReceiverEnabled = true;
    }
}

void disableReceiver(void)
{
    if (sIsReceiverEnabled)
    {
        mrf24j40_rxfifo_flush();
        /* add code to disable receiver (sleep) */
        sIsReceiverEnabled = false;
    }
}

void setChannel(uint8_t channel)
{
    mrf24j40_set_channel((int16_t)(channel - 11));
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void)aInstance;
    /* should set it manually or preset it in memory */
    aIeeeEui64[0] = 0x00;
    aIeeeEui64[1] = 0x50;
    aIeeeEui64[2] = 0xC2;
    aIeeeEui64[3] = 0x00;
    aIeeeEui64[4] = (NODE_ID >> 24) & 0xff;
    aIeeeEui64[5] = (NODE_ID >> 16) & 0xff;
    aIeeeEui64[6] = (NODE_ID >> 8) & 0xff;
    aIeeeEui64[7] = NODE_ID & 0xff;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    (void)aInstance;
    uint8_t pan[2];

    pan[0] = (uint8_t)(panid & 0xFF);
    pan[1] = (uint8_t)(panid >> 8);
    mrf24j40_set_pan(pan);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *address)
{
    (void)aInstance;
    mrf24j40_set_eui(address);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    (void)aInstance;
    uint8_t addr[2];

    addr[0] = (uint8_t)(address & 0xFF);
    addr[1] = (uint8_t)(address >> 8);
    mrf24j40_set_short_addr(addr);
}

void emskRadioInit(void)
{
    DEV_GPIO_BIT_ISR isr;
    DEV_GPIO_INT_CFG int_cfg;

    int32_t ercd;
    uint32_t temp;

    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu = sTransmitPsdu;
    sReceiveFrame.mLength = 0;
    sReceiveFrame.mPsdu = sReceivePsdu;
    sAckFrame.mLength = 0;
    sAckFrame.mPsdu = sAckPsdu;

    pmrf_spi_ptr = spi_get_dev(EMSK_PMRF_0_SPI_ID);
    ercd = pmrf_spi_ptr->spi_open(DEV_MASTER_MODE, EMSK_PMRF_0_SPIFREQ);

    if ((ercd != E_OK) && (ercd != E_OPNED))
    {
        DBG("PmodRF2 SPI open error\r\n");
    }

    pmrf_spi_ptr->spi_control(SPI_CMD_SET_CLK_MODE, CONV2VOID(EMSK_PMRF_0_SPICLKMODE));

    /*MRF24J40 wakepin:output, rstpin:output, INT_PIN:input, interrupt */
    pmrf_gpio_ptr = gpio_get_dev(EMSK_PMRF_0_GPIO_ID);
    ercd = pmrf_gpio_ptr->gpio_open(MRF24J40_WAKE_PIN | MRF24J40_RST_PIN);

    if ((ercd != E_OK) && (ercd != E_OPNED))
    {
        DBG("PmodRF2 CRTL port open error");
    }

    if (ercd == E_OPNED)
    {
        pmrf_gpio_ptr->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT, (void *)(MRF24J40_WAKE_PIN | MRF24J40_RST_PIN));
        pmrf_gpio_ptr->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT, (void *)MRF24J40_INT_PIN);
    }

    pmrf_gpio_ptr->gpio_control(GPIO_CMD_DIS_BIT_INT, (void *)MRF24J40_INT_PIN);

    temp = MRF24J40_INT_PIN;
    int_cfg.int_bit_mask = temp;
    int_cfg.int_bit_type = GPIO_INT_BITS_EDGE_TRIG(temp);
    int_cfg.int_bit_polarity = GPIO_INT_BITS_POL_FALL_EDGE(temp);
    int_cfg.int_bit_debounce = GPIO_INT_BITS_DIS_DEBOUNCE(temp);
    pmrf_gpio_ptr->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, (void *)(&int_cfg));

    isr.int_bit_ofs = MRF24J40_INT_PIN_OFS;
    isr.int_bit_handler = RadioIsr;
    pmrf_gpio_ptr->gpio_control(GPIO_CMD_SET_BIT_ISR, (void *)(&isr));

    /* interrupt for mrf24j40 is enabled at the end of the init process */
    DBG("MRF24J40 Init Started\r\n");
    mrf24j40_initialize();
    DBG("MRF24J40 Init Finished\r\n");

    pmrf_gpio_ptr->gpio_control(GPIO_CMD_ENA_BIT_INT, (void *)MRF24J40_INT_PIN);

}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return (sState != kStateDisabled) ? true : false;
}

ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    if (!otPlatRadioIsEnabled(aInstance))
    {
        sState = kStateSleep;
    }

    return kThreadError_None;
}

ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    if (otPlatRadioIsEnabled(aInstance))
    {
        sState = kStateDisabled;
    }

    return kThreadError_None;
}

ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_InvalidState;
    (void)aInstance;

    if (sState == kStateSleep || sState == kStateReceive)
    {
        error = kThreadError_None;
        sState = kStateSleep;
        disableReceiver();
    }

    return error;
}

ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    ThreadError error = kThreadError_InvalidState;
    (void)aInstance;

    if (sState != kStateDisabled)
    {
        error = kThreadError_None;
        sState = kStateReceive;
        setChannel(aChannel);
        sReceiveFrame.mChannel = aChannel;
        enableReceiver();
    }

    return error;
}

ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket)
{
    ThreadError error = kThreadError_InvalidState;
    (void)aInstance;
    (void)aPacket;

    if (sState == kStateReceive)
    {
        error = kThreadError_None;
        sState = kStateTransmit;
    }

    return error;

}

RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;
    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;
    return kRadioCapsNone;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;
    return (bool)(mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) & MRF24J40_PROMI);
}

// should be checked again with CC2538
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    mrf24j40_set_promiscuous(~aEnable);
}

void readFrame(void)
{

    /* readBuffer
     * 1 bit -- 5 to 127 bits -- 1 bit -- 1bit
     * Frame Length -- PSDU (Header + Data Payload + FCS) -- LQI -- RSSI
     */
    uint8_t readBuffer[MRF24J40_RXFIFO_SIZE];
    uint8_t readPlqi = 0;
    uint8_t readRssi = 0;

    uint16_t length;
    int16_t i;

    memset(readBuffer, 0, MRF24J40_RXFIFO_SIZE);

    otEXPECT_ACTION(sState == kStateReceive || sState == kStateTransmit, ;);
    otEXPECT_ACTION(Mrf24StatusRx, ;);

    if (Mrf24StatusRx == 1)
    {
        Mrf24StatusRx = 0;
    }

    if (Mrf24StatusSec == 1)
    {
        Mrf24StatusSec = 0;
    }

    /* Read length */
    length = (uint16_t)mrf24j40_rxpkt_intcb(readBuffer, &readPlqi, &readRssi);

    otEXPECT_ACTION(IEEE802154_MIN_LENGTH <= length && length <= IEEE802154_MAX_LENGTH, ;);

    /* Read PSDU */
    memcpy(sReceiveFrame.mPsdu, readBuffer, length - 2);

    sReceiveFrame.mPower = (int8_t)(readRssi / MRF24J40_RSSI_SLOPE) - MRF24J40_RSSI_OFFSET;
    sReceiveFrame.mLength = (uint8_t) length;
    sReceiveFrame.mLqi = readPlqi;

exit:
    return;

}

void radioTransmitMessage(otInstance *aInstance)
{
    (void)aInstance;
    uint8_t header_len = 0;

    sTransmitError = kThreadError_None;
    setChannel(sTransmitFrame.mChannel);

    uint8_t reg = mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON);

    header_len = getHeadLength(sTransmitFrame.mPsdu);

    if (isAckRequested(sTransmitFrame.mPsdu))
    {
        reg |= MRF24J40_TXNACKREQ;
    }
    else
    {
        reg &= ~(MRF24J40_TXNACKREQ);
    }

    if (isSecurityEnabled(sTransmitFrame.mPsdu))
    {
        reg |= MRF24J40_TXNSECEN;
    }
    else
    {
        reg &= ~(MRF24J40_TXNSECEN);
        mrf24j40_write_short_ctrl_reg(MRF24J40_TXNCON, mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON) & (~MRF24J40_TXNSECEN));
    }

    mrf24j40_txfifo_write(MRF24J40_TXNFIFO, sTransmitFrame.mPsdu, header_len, (sTransmitFrame.mLength - 2));

    mrf24j40_write_short_ctrl_reg(MRF24J40_TXNCON, reg | MRF24J40_TXNTRIG);

    int16_t tx_timeout = 500;
    Mrf24StatusTx = 0;

    while ((tx_timeout > 0) && (Mrf24StatusTx != 1))
    {
        mrf24j40_delay_ms(1);
        tx_timeout--;

        if (tx_timeout <= 0)
        {
            DBG("Radio Transmit Timeout!!!!!!!!!!!!\r\n");
        }
    }
}

void emskRadioProcess(otInstance *aInstance)
{
    numRadioProcess++;

    readFrame();
    uint8_t reg = mrf24j40_read_short_ctrl_reg(MRF24J40_TXSTAT);

    if (reg & MRF24J40_TXNSTAT)
    {
        DBG("TX MAC Timeout!!!!!!\r\n");

        if (reg & MRF24J40_CCAFAIL)
        {
            DBG("Channel busy!!!!!!\r\n");
        }
    }

    if ((sState == kStateReceive) && (sReceiveFrame.mLength > 0))
    {
        otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
    }

    if (sState == kStateTransmit)
    {
        radioTransmitMessage(aInstance);

        if (sTransmitError != kThreadError_None || (sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
        {
            sState = kStateReceive;
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, false, sTransmitError);
        }
        else if (Mrf24StatusTx == 1)
        {
            Mrf24StatusTx = 0;
            sState = kStateReceive;
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame,
                                    (mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON) & MRF24J40_FPSTAT) != 0,
                                    sTransmitError);
        }
    }

    sReceiveFrame.mLength = 0;

}

/**
 * \brief isr routine of mrf24j40
 * \param[in] ptr pointer transferred frome interrupt entry
 */
static void RadioIsr(void *ptr)
{
    uint8_t int_status = 0;

    int_status = pmrf_read_short_ctrl_reg(MRF24J40_INTSTAT);

    /* a frame is received */
    if (int_status & MRF24J40_RXIF)
    {
        numInterruptRev++;
        Mrf24StatusRx = 1;
    }

    /* a frame is transmitted */
    if (int_status & MRF24J40_TXNIF)
    {
        switch (mrf24j40_txpkt_intcb())
        {
        case MRF24J40_EBUSY:
            /* Channel busy */
            break;

        case MRF24J40_EIO:
            /* Channel idle */
            break;

        case 0:
            numInterruptTrans++;
            Mrf24StatusTx = 1;
            break;
        }
    }

    /* a frame with security key request is received */
    if (int_status & MRF24J40_SECIF)
    {
        Mrf24StatusSec = 1;
        mrf24j40_sec_intcb(false);
    }

}

/* CC2538 supports source address matching for low power consumption
   and this is not supported in MRF24J40 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    (void)aEnable;
}

ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;
    (void)aExtAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;
    (void)aExtAddress;
    return kThreadError_None;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void)aInstance;
}

ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;
    return kThreadError_NotImplemented;
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    // TODO: Create a proper implementation for this driver.
    (void)aInstance;
    (void)aPower;
}
