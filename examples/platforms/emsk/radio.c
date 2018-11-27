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
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <string.h>

#include "platform-emsk.h"

#include <openthread/dataset.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include "utils/code_utils.h"

#include "device/device_hal/inc/dev_gpio.h"

#include <stdio.h>
#define DBG(fmt, ...) printf(fmt, ##__VA_ARGS__)

enum
{
    IEEE802154_MIN_LENGTH = 5,
    IEEE802154_MAX_LENGTH = 127,
    IEEE802154_ACK_LENGTH = 5,

    IEEE802154_BROADCAST = 0xffff,

    IEEE802154_FRAME_TYPE_ACK    = 2 << 0,
    IEEE802154_FRAME_TYPE_MACCMD = 3 << 0,
    IEEE802154_FRAME_TYPE_MASK   = 7 << 0,

    IEEE802154_SECURITY_ENABLED  = 1 << 3,
    IEEE802154_FRAME_PENDING     = 1 << 4,
    IEEE802154_ACK_REQUEST       = 1 << 5,
    IEEE802154_PANID_COMPRESSION = 1 << 6,

    IEEE802154_DST_ADDR_NONE  = 0 << 2,
    IEEE802154_DST_ADDR_SHORT = 2 << 2,
    IEEE802154_DST_ADDR_EXT   = 3 << 2,
    IEEE802154_DST_ADDR_MASK  = 3 << 2,

    IEEE802154_SRC_ADDR_NONE  = 0 << 6,
    IEEE802154_SRC_ADDR_SHORT = 2 << 6,
    IEEE802154_SRC_ADDR_EXT   = 3 << 6,
    IEEE802154_SRC_ADDR_MASK  = 3 << 6,

    IEEE802154_DSN_OFFSET     = 2,
    IEEE802154_DSTPAN_OFFSET  = 3,
    IEEE802154_DSTADDR_OFFSET = 5,

    IEEE802154_SEC_LEVEL_MASK = 7 << 0,

    IEEE802154_KEY_ID_MODE_0    = 0 << 3,
    IEEE802154_KEY_ID_MODE_1    = 1 << 3,
    IEEE802154_KEY_ID_MODE_2    = 2 << 3,
    IEEE802154_KEY_ID_MODE_3    = 3 << 3,
    IEEE802154_KEY_ID_MODE_MASK = 3 << 3,

    IEEE802154_MACCMD_DATA_REQ = 4
};

enum
{
    EMSK_RECEIVE_SENSITIVITY = -100, // dBm
};

enum
{
    MRF24J40_RSSI_OFFSET = 90,
    MRF24J40_RSSI_SLOPE  = 5
};

static void radioTransmitMessage(otInstance *aInstance);

static otRadioFrame sTransmitFrame;
static otRadioFrame sReceiveFrame;
static otRadioFrame sAckFrame;

static otError sTransmitError;
static otError sReceiveError;

static uint8_t sTransmitPsdu[IEEE802154_MAX_LENGTH];
static uint8_t sReceivePsdu[IEEE802154_MAX_LENGTH];
static uint8_t sAckPsdu[IEEE802154_MAX_LENGTH];

static otRadioState sState             = OT_RADIO_STATE_DISABLED;
static bool         sIsReceiverEnabled = false;

static volatile uint8_t Mrf24StatusTx  = 0;
static volatile uint8_t Mrf24StatusRx  = 0;
static volatile uint8_t Mrf24StatusSec = 0;

static DEV_SPI_PTR  pmrf_spi_ptr;
static DEV_GPIO_PTR pmrf_gpio_ptr;
static void         RadioIsr(void *ptr);

/* Variables for test */
static uint32_t numInterruptRev   = 0;
static uint32_t numInterruptTrans = 0;
static uint32_t numRadioProcess   = 0;

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
    OT_UNUSED_VARIABLE(aInstance);

    /* should set it manually or preset it in memory */
    aIeeeEui64[0] = 0x00;
    aIeeeEui64[1] = 0x50;
    aIeeeEui64[2] = 0xC2;
    aIeeeEui64[3] = 0xFF;
    aIeeeEui64[4] = 0XFE;
    aIeeeEui64[5] = 0X1D;
    aIeeeEui64[6] = 0X30;
    aIeeeEui64[7] = 0x00;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t pan[2];

    pan[0] = (uint8_t)(panid & 0xFF);
    pan[1] = (uint8_t)(panid >> 8);
    mrf24j40_set_pan(pan);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *address)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* cast to remove const, FIXME: perhaps the bsp library should be updated? */
    mrf24j40_set_eui((uint8_t *)(address->m8));
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t addr[2];

    addr[0] = (uint8_t)(address & 0xFF);
    addr[1] = (uint8_t)(address >> 8);
    mrf24j40_set_short_addr(addr);
}

void emskRadioInit(void)
{
    DEV_GPIO_BIT_ISR isr;
    DEV_GPIO_INT_CFG int_cfg;

    int32_t  ercd;
    uint32_t temp;

    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu   = sTransmitPsdu;
    sReceiveFrame.mLength  = 0;
    sReceiveFrame.mPsdu    = sReceivePsdu;
    sAckFrame.mLength      = 0;
    sAckFrame.mPsdu        = sAckPsdu;

    pmrf_spi_ptr = spi_get_dev(EMSK_PMRF_0_SPI_ID);
    ercd         = pmrf_spi_ptr->spi_open(DEV_MASTER_MODE, EMSK_PMRF_0_SPIFREQ);

    if ((ercd != E_OK) && (ercd != E_OPNED))
    {
        DBG("PmodRF2 SPI open error\r\n");
    }

    pmrf_spi_ptr->spi_control(SPI_CMD_SET_CLK_MODE, CONV2VOID(EMSK_PMRF_0_SPICLKMODE));

    /*MRF24J40 wakepin:output, rstpin:output, INT_PIN:input, interrupt */
    pmrf_gpio_ptr = gpio_get_dev(EMSK_PMRF_0_GPIO_ID);
    ercd          = pmrf_gpio_ptr->gpio_open(MRF24J40_WAKE_PIN | MRF24J40_RST_PIN);

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

    temp                     = MRF24J40_INT_PIN;
    int_cfg.int_bit_mask     = temp;
    int_cfg.int_bit_type     = GPIO_INT_BITS_EDGE_TRIG(temp);
    int_cfg.int_bit_polarity = GPIO_INT_BITS_POL_FALL_EDGE(temp);
    int_cfg.int_bit_debounce = GPIO_INT_BITS_DIS_DEBOUNCE(temp);
    pmrf_gpio_ptr->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, (void *)(&int_cfg));

    isr.int_bit_ofs     = MRF24J40_INT_PIN_OFS;
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
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    if (!otPlatRadioIsEnabled(aInstance))
    {
        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    if (otPlatRadioIsEnabled(aInstance))
    {
        sState = OT_RADIO_STATE_DISABLED;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_SLEEP || sState == OT_RADIO_STATE_RECEIVE)
    {
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_SLEEP;
        disableReceiver();
    }

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState != OT_RADIO_STATE_DISABLED)
    {
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_RECEIVE;
        setChannel(aChannel);
        sReceiveFrame.mChannel = aChannel;
        enableReceiver();
    }

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_TRANSMIT;
    }

    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_NONE;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (bool)(mrf24j40_read_short_ctrl_reg(MRF24J40_RXMCR) & MRF24J40_PROMI);
}

// should be checked again with CC2538
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    mrf24j40_set_promiscuous(~aEnable);
}

void readFrame(otInstance *aInstance)
{
    /* readBuffer
     * 1 bit -- 5 to 127 bits -- 1 bit -- 1bit
     * Frame Length -- PSDU (Header + Data Payload + FCS) -- LQI -- RSSI
     */
    uint8_t readBuffer[MRF24J40_RXFIFO_SIZE];
    uint8_t readPlqi = 0;
    uint8_t readRssi = 0;

    uint16_t length;
    int16_t  i;

    memset(readBuffer, 0, MRF24J40_RXFIFO_SIZE);

    otEXPECT_ACTION(sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT, ;);
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

    if (otPlatRadioGetPromiscuous(aInstance))
    {
        // Timestamp
        sReceiveFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
        sReceiveFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
    }

    /* Read PSDU */
    memcpy(sReceiveFrame.mPsdu, readBuffer, length - 2);

    sReceiveFrame.mInfo.mRxInfo.mRssi = (int8_t)(readRssi / MRF24J40_RSSI_SLOPE) - MRF24J40_RSSI_OFFSET;
    sReceiveFrame.mLength             = (uint8_t)length;
    sReceiveFrame.mInfo.mRxInfo.mLqi  = readPlqi;

exit:
    return;
}

void radioTransmitMessage(otInstance *aInstance)
{
    uint8_t header_len = 0;

    sTransmitError = OT_ERROR_NONE;
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
        mrf24j40_write_short_ctrl_reg(MRF24J40_TXNCON,
                                      mrf24j40_read_short_ctrl_reg(MRF24J40_TXNCON) & (~MRF24J40_TXNSECEN));
    }

    mrf24j40_txfifo_write(MRF24J40_TXNFIFO, sTransmitFrame.mPsdu, header_len, (sTransmitFrame.mLength - 2));

    mrf24j40_write_short_ctrl_reg(MRF24J40_TXNCON, reg | MRF24J40_TXNTRIG);

    otPlatRadioTxStarted(aInstance, &sTransmitFrame);

    int16_t tx_timeout = 500;
    Mrf24StatusTx      = 0;

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

    readFrame(aInstance);
    uint8_t reg = mrf24j40_read_short_ctrl_reg(MRF24J40_TXSTAT);

    if (reg & MRF24J40_TXNSTAT)
    {
        DBG("TX MAC Timeout!!!!!!\r\n");

        if (reg & MRF24J40_CCAFAIL)
        {
            DBG("Channel busy!!!!!!\r\n");
        }
    }

    if ((sState == OT_RADIO_STATE_RECEIVE) && (sReceiveFrame.mLength > 0))
    {
        otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
    }

    if (sState == OT_RADIO_STATE_TRANSMIT)
    {
        radioTransmitMessage(aInstance);

        if (sTransmitError != OT_ERROR_NONE || (sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
        {
            sState = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
        }
        else if (Mrf24StatusTx == 1)
        {
            Mrf24StatusTx = 0;
            sState        = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveFrame, sTransmitError);
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
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);

    return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);

    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);

    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);

    return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    // TODO: Create a proper implementation for this driver.
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    // TODO: Create a proper implementation for this driver.
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);

    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return EMSK_RECEIVE_SENSITIVITY;
}
