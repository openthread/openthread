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

#include "fsl_device_registers.h"
#include "fsl_xcvr.h"
#include "openthread-core-kw41z-config.h"
#include <stdint.h>
#include <string.h>

#include <utils/code_utils.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

// clang-format off
#define DOUBLE_BUFFERING             (1)
#define DEFAULT_CHANNEL              (11)
#define DEFAULT_CCA_MODE             (XCVR_CCA_MODE1_c)
#define IEEE802154_ACK_REQUEST       (1 << 5)
#define IEEE802154_MAX_LENGTH        (127)
#define IEEE802154_MIN_LENGTH        (5)
#define IEEE802154_ACK_LENGTH        (IEEE802154_MIN_LENGTH)
#define IEEE802154_FRM_CTL_LO_OFFSET (0)
#define IEEE802154_DSN_OFFSET        (2)
#define IEEE802154_FRM_TYPE_MASK     (0x7)
#define IEEE802154_FRM_TYPE_ACK      (0x2)
#define IEEE802154_TURNAROUND_LEN    (12)
#define IEEE802154_CCA_LEN           (8)
#define IEEE802154_PHY_SHR_LEN       (10)
#define IEEE802154_ACK_WAIT          (54)
#define ZLL_IRQSTS_TMR_ALL_MSK_MASK  (ZLL_IRQSTS_TMR1MSK_MASK | \
                                      ZLL_IRQSTS_TMR2MSK_MASK | \
                                      ZLL_IRQSTS_TMR3MSK_MASK | \
                                      ZLL_IRQSTS_TMR4MSK_MASK )
#define ZLL_DEFAULT_RX_FILTERING     (ZLL_RX_FRAME_FILTER_FRM_VER_FILTER(3) | \
                                      ZLL_RX_FRAME_FILTER_CMD_FT_MASK | \
                                      ZLL_RX_FRAME_FILTER_DATA_FT_MASK | \
                                      ZLL_RX_FRAME_FILTER_ACK_FT_MASK | \
                                      ZLL_RX_FRAME_FILTER_BEACON_FT_MASK)
// clang-format on

typedef enum xcvr_state_tag
{
    XCVR_Idle_c,
    XCVR_RX_c,
    XCVR_TX_c,
    XCVR_CCA_c,
    XCVR_TR_c,
    XCVR_CCCA_c,
} xcvr_state_t;

typedef enum xcvr_cca_type_tag
{
    XCVR_ED_c,        /* energy detect - CCA bit not active, not to be used for T and CCCA sequences */
    XCVR_CCA_MODE1_c, /* energy detect - CCA bit ACTIVE */
    SCVR_CCA_MODE2_c, /* 802.15.4 compliant signal detect - CCA bit ACTIVE */
    XCVR_CCA_MODE3_c  /* 802.15.4 compliant signal detect and energy detect - CCA bit ACTIVE */
} xcvr_cca_type_t;

static otRadioState sState = OT_RADIO_STATE_DISABLED;
static uint16_t     sPanId;
static uint8_t      sExtSrcAddrBitmap[(RADIO_CONFIG_SRC_MATCH_ENTRY_NUM + 7) / 8];
static uint8_t      sChannel;
static int8_t       sMaxED;
static int8_t       sAutoTxPwrLevel = 0;

/* ISR Signaling Flags */
static bool    sTxDone     = false;
static bool    sRxDone     = false;
static bool    sEdScanDone = false;
static otError sTxStatus;

static otRadioFrame sTxFrame;
static otRadioFrame sRxFrame;
static uint8_t      sTxData[OT_RADIO_FRAME_MAX_SIZE];
#if DOUBLE_BUFFERING
static uint8_t sRxData[OT_RADIO_FRAME_MAX_SIZE];
#endif
static otInstance *sInstance = NULL;

/* Private functions */
static void         rf_abort(void);
static xcvr_state_t rf_get_state(void);
static void         rf_set_channel(uint8_t channel);
static void         rf_set_tx_power(int8_t tx_power);
static uint8_t      rf_lqi_adjust(uint8_t hwLqi);
static int8_t       rf_lqi_to_rssi(uint8_t lqi);
static uint32_t     rf_get_timestamp(void);
static void         rf_set_timeout(uint32_t abs_timeout);
static uint16_t     rf_get_addr_checksum(uint8_t *pAddr, bool ExtendedAddr, uint16_t PanId);
static otError      rf_add_addr_table_entry(uint16_t checksum, bool extendedAddr);
static otError      rf_remove_addr_table_entry(uint16_t checksum);
static otError      rf_remove_addr_table_entry_index(uint8_t index);
static bool         rf_process_rx_frame(void);

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t addrLo;
    uint32_t addrHi;

    if ((RSIM->MAC_LSB == 0xffffffff) && (RSIM->MAC_MSB == 0xff))
    {
        addrLo = SIM->UIDL;
        addrHi = SIM->UIDML;
    }
    else
    {
        addrLo = RSIM->MAC_LSB;
        addrHi = RSIM->MAC_MSB;
    }

    memcpy(aIeeeEui64, &addrLo, sizeof(addrLo));
    memcpy(aIeeeEui64 + sizeof(addrLo), &addrHi, sizeof(addrHi));
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);

    sPanId = aPanId;
    ZLL->MACSHORTADDRS0 &= ~ZLL_MACSHORTADDRS0_MACPANID0_MASK;
    ZLL->MACSHORTADDRS0 |= ZLL_MACSHORTADDRS0_MACPANID0(aPanId);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t addrLo;
    uint32_t addrHi;

    memcpy(&addrLo, aExtAddress->m8, sizeof(addrLo));
    memcpy(&addrHi, aExtAddress->m8 + sizeof(addrLo), sizeof(addrHi));

    ZLL->MACLONGADDRS0_LSB = addrLo;
    ZLL->MACLONGADDRS0_MSB = addrHi;
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    ZLL->MACSHORTADDRS0 &= ~ZLL_MACSHORTADDRS0_MACSHORTADDRS0_MASK;
    ZLL->MACSHORTADDRS0 |= ZLL_MACSHORTADDRS0_MACSHORTADDRS0(aShortAddress);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otEXPECT(!otPlatRadioIsEnabled(aInstance));

    sInstance = aInstance;
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TRCV_MSK_MASK;
    NVIC_ClearPendingIRQ(Radio_1_IRQn);
    NVIC_EnableIRQ(Radio_1_IRQn);

    sState = OT_RADIO_STATE_SLEEP;

exit:
    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otEXPECT(otPlatRadioIsEnabled(aInstance));

    NVIC_DisableIRQ(Radio_1_IRQn);
    rf_abort();
    sState = OT_RADIO_STATE_DISABLED;

exit:
    return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState != OT_RADIO_STATE_DISABLED;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError status = OT_ERROR_NONE;

    otEXPECT_ACTION(((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED)),
                    status = OT_ERROR_INVALID_STATE);

    rf_abort();
    sState = OT_RADIO_STATE_SLEEP;

exit:
    return status;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError status = OT_ERROR_NONE;

    otEXPECT_ACTION(((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED)),
                    status = OT_ERROR_INVALID_STATE);

    sState = OT_RADIO_STATE_RECEIVE;

    /* Check if the channel needs to be changed */
    if ((sChannel != aChannel) || (rf_get_state() != XCVR_RX_c))
    {
        rf_abort();
        /* Set Power level for auto TX */
        rf_set_tx_power(sAutoTxPwrLevel);
        rf_set_channel(aChannel);
        sRxFrame.mChannel = aChannel;

        /* Filter ACK frames during RX sequence */
        ZLL->RX_FRAME_FILTER &= ~(ZLL_RX_FRAME_FILTER_ACK_FT_MASK);
        /* Clear all IRQ flags */
        ZLL->IRQSTS = ZLL->IRQSTS;
        /* Start the RX sequence */
        ZLL->PHY_CTRL |= XCVR_RX_c;

        /* Unmask SEQ interrupt */
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
    }

exit:
    return status;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aEnable)
    {
        ZLL->SAM_CTRL |= ZLL_SAM_CTRL_SAP0_EN_MASK;
    }
    else
    {
        ZLL->SAM_CTRL &= ~ZLL_SAM_CTRL_SAP0_EN_MASK;
    }
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint16_t checksum = sPanId + aShortAddress;

    return rf_add_addr_table_entry(checksum, false);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint16_t checksum = rf_get_addr_checksum((uint8_t *)aExtAddress->m8, true, sPanId);

    return rf_add_addr_table_entry(checksum, true);
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint16_t checksum = sPanId + aShortAddress;

    return rf_remove_addr_table_entry(checksum);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint16_t checksum = rf_get_addr_checksum((uint8_t *)aExtAddress->m8, true, sPanId);

    return rf_remove_addr_table_entry(checksum);
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t i;

    for (i = 0; i < RADIO_CONFIG_SRC_MATCH_ENTRY_NUM; i++)
    {
        /* Optimization: sExtSrcAddrBitmap[i / 8] & (1 << (i % 8)) */
        if (!(sExtSrcAddrBitmap[i >> 3] & (1 << (i & 7))))
        {
            rf_remove_addr_table_entry_index(i);
        }
    }
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t i;

    for (i = 0; i < RADIO_CONFIG_SRC_MATCH_ENTRY_NUM; i++)
    {
        /* Optimization: sExtSrcAddrBitmap[i / 8] & (1 << (i % 8))*/
        if (sExtSrcAddrBitmap[i >> 3] & (1 << (i & 7)))
        {
            rf_remove_addr_table_entry_index(i);
        }
    }
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTxFrame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError  status = OT_ERROR_NONE;
    uint32_t timeout;

    otEXPECT_ACTION(((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED)),
                    status = OT_ERROR_INVALID_STATE);

    if (rf_get_state() != XCVR_Idle_c)
    {
        rf_abort();
    }

    rf_set_channel(aFrame->mChannel);

    *(uint8_t *)ZLL->PKT_BUFFER_TX = aFrame->mLength;

    /* MKW41Z Reference Manual Section 44.6.2.7 states: "The 802.15.4
     * Link Layer software prepares data to be transmitted, by loading
     * the octets, in order, into the Packet Buffer." */
    for (int i = 0; i < aFrame->mLength - sizeof(uint16_t); i++)
    {
        ((uint8_t *)ZLL->PKT_BUFFER_TX)[1 + i] = sTxFrame.mPsdu[i];
    }

    /* Set CCA mode */
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_CCATYPE_MASK;
    ZLL->PHY_CTRL |= ZLL_PHY_CTRL_CCATYPE(DEFAULT_CCA_MODE);

    /* Clear all IRQ flags */
    ZLL->IRQSTS = ZLL->IRQSTS;

    /* Perform automatic reception of ACK frame, if required */
    if (aFrame->mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] & IEEE802154_ACK_REQUEST)
    {
        /* Permit the reception of ACK frames during TR sequence */
        ZLL->RX_FRAME_FILTER |= (ZLL_RX_FRAME_FILTER_ACK_FT_MASK);
        ZLL->PHY_CTRL |= XCVR_TR_c;
        /* Set ACK wait time-out */
        timeout = rf_get_timestamp();
        timeout += (((XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK) >>
                     XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT) >>
                    4);
        timeout += IEEE802154_CCA_LEN + IEEE802154_TURNAROUND_LEN + IEEE802154_PHY_SHR_LEN +
                   (1 + aFrame->mLength) * OT_RADIO_SYMBOLS_PER_OCTET + IEEE802154_ACK_WAIT;
        rf_set_timeout(timeout);
    }
    else
    {
        ZLL->PHY_CTRL |= XCVR_TX_c;
    }

    sTxStatus = OT_ERROR_NONE;
    sState    = OT_RADIO_STATE_TRANSMIT;
    /* Unmask SEQ interrupt */
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;

    otPlatRadioTxStarted(aInstance, aFrame);

exit:
    return status;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (ZLL->LQI_AND_RSSI & ZLL_LQI_AND_RSSI_RSSI_MASK) >> ZLL_LQI_AND_RSSI_RSSI_SHIFT;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_ENERGY_SCAN;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (ZLL->PHY_CTRL & ZLL_PHY_CTRL_PROMISCUOUS_MASK) == ZLL_PHY_CTRL_PROMISCUOUS_MASK;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aEnable)
    {
        ZLL->PHY_CTRL |= ZLL_PHY_CTRL_PROMISCUOUS_MASK;
        /* FRM_VER[11:8] = b1111. Any FrameVersion accepted */
        ZLL->RX_FRAME_FILTER |= (ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_MASK | ZLL_RX_FRAME_FILTER_NS_FT_MASK);
    }
    else
    {
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_PROMISCUOUS_MASK;
        /* FRM_VER[11:8] = b0011. Accept FrameVersion 0 and 1 packets, reject all others */
        ZLL->RX_FRAME_FILTER = ZLL_DEFAULT_RX_FILTERING;
    }
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError  status = OT_ERROR_NONE;
    uint32_t timeout;

    otEXPECT_ACTION(((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED)),
                    status = OT_ERROR_INVALID_STATE);

    if (rf_get_state() != XCVR_Idle_c)
    {
        rf_abort();
    }

    sMaxED = -128;
    rf_set_channel(aScanChannel);
    /* Set CCA type to ED - Energy Detect */
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_CCATYPE_MASK;
    ZLL->PHY_CTRL |= ZLL_PHY_CTRL_CCATYPE(XCVR_ED_c);
    /* Clear all IRQ flags */
    ZLL->IRQSTS = ZLL->IRQSTS;
    /* Start ED sequence */
    ZLL->PHY_CTRL |= XCVR_CCA_c;
    /* Unmask SEQ interrupt */
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
    /* Set Scan time-out */
    timeout = rf_get_timestamp();
    timeout += (aScanDuration * 1000) / OT_RADIO_SYMBOL_TIME;
    rf_set_timeout(timeout);

exit:
    return status;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sAutoTxPwrLevel;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAutoTxPwrLevel = aPower;

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return -100;
}

/*************************************************************************************************/

static void rf_abort(void)
{
    /* Mask SEQ interrupt */
    ZLL->PHY_CTRL |= ZLL_PHY_CTRL_SEQMSK_MASK;

    /* Disable timer trigger (for scheduled XCVSEQ) */
    if (ZLL->PHY_CTRL & ZLL_PHY_CTRL_TMRTRIGEN_MASK)
    {
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMRTRIGEN_MASK;

        /* give the FSM enough time to start if it was triggered */
        while ((XCVR_MISC->XCVR_CTRL & XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK) == 0)
        {
        }
    }

    /* If XCVR is not idle, abort current SEQ */
    if (ZLL->PHY_CTRL & ZLL_PHY_CTRL_XCVSEQ_MASK)
    {
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_XCVSEQ_MASK;

        /* wait for Sequence Idle (if not already) */
        while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK)
        {
        }
    }

    /* Stop timers */
    ZLL->PHY_CTRL &= ~(ZLL_PHY_CTRL_TMR2CMP_EN_MASK | ZLL_PHY_CTRL_TMR3CMP_EN_MASK);
    /* Clear all PP IRQ bits to avoid unexpected interrupts( do not change TMR1 and TMR4 IRQ status ) */
    ZLL->IRQSTS &= ~(ZLL_IRQSTS_TMR1IRQ_MASK | ZLL_IRQSTS_TMR4IRQ_MASK);
}

static xcvr_state_t rf_get_state(void)
{
    return (xcvr_state_t)((ZLL->PHY_CTRL & ZLL_PHY_CTRL_XCVSEQ_MASK) >> ZLL_PHY_CTRL_XCVSEQ_SHIFT);
}

static void rf_set_channel(uint8_t channel)
{
    if (sChannel != channel)
    {
        ZLL->CHANNEL_NUM0 = channel;
        sChannel          = channel;
    }
}

static void rf_set_tx_power(int8_t tx_power)
{
    if (tx_power > 2)
    {
        ZLL->PA_PWR = 30;
    }
    else if (tx_power > 1)
    {
        ZLL->PA_PWR = 24;
    }
    else if (tx_power > -1)
    {
        ZLL->PA_PWR = 18;
    }
    else if (tx_power > -3)
    {
        ZLL->PA_PWR = 14;
    }
    else if (tx_power > -4)
    {
        ZLL->PA_PWR = 12;
    }
    else if (tx_power > -6)
    {
        ZLL->PA_PWR = 10;
    }
    else if (tx_power > -8)
    {
        ZLL->PA_PWR = 8;
    }
    else if (tx_power > -11)
    {
        ZLL->PA_PWR = 6;
    }
    else if (tx_power > -14)
    {
        ZLL->PA_PWR = 4;
    }
    else if (tx_power > -20)
    {
        ZLL->PA_PWR = 2;
    }
    else
    {
        ZLL->PA_PWR = 0;
    }
}

static uint16_t rf_get_addr_checksum(uint8_t *pAddr, bool ExtendedAddr, uint16_t PanId)
{
    uint16_t checksum;

    /* Short address */
    checksum = PanId;
    checksum += *pAddr++;
    checksum += (uint16_t)(*pAddr++) << 8;

    if (ExtendedAddr)
    {
        /* Extended address */
        checksum += *pAddr++;
        checksum += ((uint16_t)(*pAddr++)) << 8;
        checksum += *pAddr++;
        checksum += ((uint16_t)(*pAddr++)) << 8;
        checksum += *pAddr++;
        checksum += ((uint16_t)(*pAddr++)) << 8;
    }

    return checksum;
}

static otError rf_add_addr_table_entry(uint16_t checksum, bool extendedAddr)
{
    uint8_t index;
    otError status;

    /* Find first free index */
    ZLL->SAM_TABLE = ZLL_SAM_TABLE_FIND_FREE_IDX_MASK;

    while (ZLL->SAM_TABLE & ZLL_SAM_TABLE_SAM_BUSY_MASK)
    {
    }

    index = (ZLL->SAM_FREE_IDX & ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_MASK) >> ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_SHIFT;

    otEXPECT_ACTION((index < RADIO_CONFIG_SRC_MATCH_ENTRY_NUM), status = OT_ERROR_NO_BUFS);

    /* Insert the checksum at the index found */
    ZLL->SAM_TABLE = ((uint32_t)index << ZLL_SAM_TABLE_SAM_INDEX_SHIFT) |
                     ((uint32_t)checksum << ZLL_SAM_TABLE_SAM_CHECKSUM_SHIFT) | ZLL_SAM_TABLE_SAM_INDEX_WR_MASK |
                     ZLL_SAM_TABLE_SAM_INDEX_EN_MASK;

    if (extendedAddr)
    {
        /* Optimization: sExtSrcAddrBitmap[index / 8] |= 1 << (index % 8); */
        sExtSrcAddrBitmap[index >> 3] |= 1 << (index & 7);
    }

    status = OT_ERROR_NONE;

exit:
    return status;
}

static otError rf_remove_addr_table_entry(uint16_t checksum)
{
    otError  status = OT_ERROR_NO_ADDRESS;
    uint32_t i, temp;

    /* Search for an entry to match the provided checksum */
    for (i = 0; i < RADIO_CONFIG_SRC_MATCH_ENTRY_NUM; i++)
    {
        ZLL->SAM_TABLE = i << ZLL_SAM_TABLE_SAM_INDEX_SHIFT;
        /* Read checksum located at the specified index */
        temp = (ZLL->SAM_TABLE & ZLL_SAM_TABLE_SAM_CHECKSUM_MASK) >> ZLL_SAM_TABLE_SAM_CHECKSUM_SHIFT;

        if (temp == checksum)
        {
            /* Remove the entry from the table */
            status = rf_remove_addr_table_entry_index(i);
            break;
        }
    }

    return status;
}

static otError rf_remove_addr_table_entry_index(uint8_t index)
{
    otError status = OT_ERROR_NONE;

    otEXPECT_ACTION(index < RADIO_CONFIG_SRC_MATCH_ENTRY_NUM, status = OT_ERROR_NO_ADDRESS);

    ZLL->SAM_TABLE = ((uint32_t)0xFFFF << ZLL_SAM_TABLE_SAM_CHECKSUM_SHIFT) |
                     ((uint32_t)index << ZLL_SAM_TABLE_SAM_INDEX_SHIFT) | ZLL_SAM_TABLE_SAM_INDEX_INV_MASK |
                     ZLL_SAM_TABLE_SAM_INDEX_WR_MASK;

    /* Clear bitmap */
    /* Optimization: sExtSrcAddrBitmap[index / 8] &= ~(1 << (index % 8)); */
    sExtSrcAddrBitmap[index >> 3] &= ~(1 << (index & 7));

exit:
    return status;
}

static uint8_t rf_lqi_adjust(uint8_t hwLqi)
{
    if (hwLqi >= 220)
    {
        hwLqi = 255;
    }
    else
    {
        hwLqi = (51 * hwLqi) / 44;
    }

    return hwLqi;
}

static int8_t rf_lqi_to_rssi(uint8_t lqi)
{
    int32_t rssi = (36 * lqi - 9836) / 109;

    return (int8_t)rssi;
}

static uint32_t rf_get_timestamp(void)
{
    return ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT;
}

static void rf_set_timeout(uint32_t abs_timeout)
{
    uint32_t irqSts;

    /* Disable TMR3 compare */
    ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR3CMP_EN_MASK;
    /* Set time-out value */
    ZLL->T3CMP = abs_timeout;
    /* Aknowledge and unmask TMR3 IRQ */
    irqSts = ZLL->IRQSTS & ZLL_IRQSTS_TMR_ALL_MSK_MASK;
    irqSts &= ~ZLL_IRQSTS_TMR3MSK_MASK;
    irqSts |= ZLL_IRQSTS_TMR3IRQ_MASK;
    ZLL->IRQSTS = irqSts;
    /* Enable TMR3 compare */
    ZLL->PHY_CTRL |= ZLL_PHY_CTRL_TMR3CMP_EN_MASK;
}

static bool rf_process_rx_frame(void)
{
    uint8_t temp;
    bool    status = true;

    /* Get Rx length */
    temp = (ZLL->IRQSTS & ZLL_IRQSTS_RX_FRAME_LENGTH_MASK) >> ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT;

    /* Check if frame is valid */
    otEXPECT_ACTION((IEEE802154_MIN_LENGTH <= temp) && (temp <= IEEE802154_MAX_LENGTH), status = false);

    if (otPlatRadioGetPromiscuous(sInstance))
    {
        // Timestamp
        sRxFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
        sRxFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
    }

    sRxFrame.mLength = temp;
    temp             = (ZLL->LQI_AND_RSSI & ZLL_LQI_AND_RSSI_LQI_VALUE_MASK) >> ZLL_LQI_AND_RSSI_LQI_VALUE_SHIFT;
    sRxFrame.mInfo.mRxInfo.mLqi  = rf_lqi_adjust(temp);
    sRxFrame.mInfo.mRxInfo.mRssi = rf_lqi_to_rssi(sRxFrame.mInfo.mRxInfo.mLqi);
#if DOUBLE_BUFFERING

    for (temp = 0; temp < sRxFrame.mLength - 2; temp++)
    {
        sRxData[temp] = ((uint8_t *)ZLL->PKT_BUFFER_RX)[temp];
    }

#endif

exit:
    return status;
}

void Radio_1_IRQHandler(void)
{
    xcvr_state_t state     = rf_get_state();
    uint32_t     irqStatus = ZLL->IRQSTS;
    int8_t       temp;

    ZLL->IRQSTS = irqStatus;

    /* TMR3 IRQ - time-out */
    if ((irqStatus & ZLL_IRQSTS_TMR3IRQ_MASK) && (!(irqStatus & ZLL_IRQSTS_TMR3MSK_MASK)))
    {
        /* Stop TMR3 */
        ZLL->IRQSTS = irqStatus | ZLL_IRQSTS_TMR3MSK_MASK;
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR3CMP_EN_MASK;

        if (state == XCVR_CCA_c)
        {
            rf_abort();
            sEdScanDone = true;
        }
        else if ((state == XCVR_TR_c) && !(irqStatus & ZLL_IRQSTS_RXIRQ_MASK))
        {
            rf_abort();
            sState    = OT_RADIO_STATE_RECEIVE;
            sTxStatus = OT_ERROR_NO_ACK;
            sTxDone   = true;
        }
    }

    /* Sequence done IRQ */
    if ((!(ZLL->PHY_CTRL & ZLL_PHY_CTRL_SEQMSK_MASK)) && (irqStatus & ZLL_IRQSTS_SEQIRQ_MASK))
    {
        /* Cleanup */
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_XCVSEQ_MASK;
        ZLL->PHY_CTRL |= ZLL_PHY_CTRL_SEQMSK_MASK;

        switch (state)
        {
        case XCVR_RX_c:
            sRxDone = rf_process_rx_frame();
            break;

        case XCVR_TR_c:
            /* Stop TMR3 */
            ZLL->IRQSTS = irqStatus | ZLL_IRQSTS_TMR3MSK_MASK;
            ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR3CMP_EN_MASK;

            if ((ZLL->PHY_CTRL & ZLL_PHY_CTRL_CCABFRTX_MASK) && (irqStatus & ZLL_IRQSTS_CCA_MASK))
            {
                sTxStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            }
            else if (!(irqStatus & ZLL_IRQSTS_RXIRQ_MASK) || (rf_process_rx_frame() == false) ||
                     (sRxFrame.mLength != IEEE802154_ACK_LENGTH) ||
                     ((sRxFrame.mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] & IEEE802154_FRM_TYPE_MASK) !=
                      IEEE802154_FRM_TYPE_ACK) ||
                     (sRxFrame.mPsdu[IEEE802154_DSN_OFFSET] != sTxFrame.mPsdu[IEEE802154_DSN_OFFSET]))
            {
                sTxStatus = OT_ERROR_NO_ACK;
            }

            sState  = OT_RADIO_STATE_RECEIVE;
            sTxDone = true;
            break;

        case XCVR_TX_c:
            if ((ZLL->PHY_CTRL & ZLL_PHY_CTRL_CCABFRTX_MASK) && (irqStatus & ZLL_IRQSTS_CCA_MASK))
            {
                sTxStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            }

            sState  = OT_RADIO_STATE_RECEIVE;
            sTxDone = true;
            break;

        case XCVR_CCA_c:
            temp = (ZLL->LQI_AND_RSSI & ZLL_LQI_AND_RSSI_CCA1_ED_FNL_MASK) >> ZLL_LQI_AND_RSSI_CCA1_ED_FNL_SHIFT;

            if (temp > sMaxED)
            {
                sMaxED = temp;
            }

            if (!sEdScanDone)
            {
                /* Restart ED */
                while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK)
                {
                }

                ZLL->IRQSTS = (ZLL->IRQSTS & ZLL_IRQSTS_TMR_ALL_MSK_MASK) | ZLL_IRQSTS_SEQIRQ_MASK;
                ZLL->PHY_CTRL |= XCVR_CCA_c;
                ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
            }

            break;

        default:
            rf_abort();
            break;
        }
    }

    if ((sState == OT_RADIO_STATE_RECEIVE) && (rf_get_state() == XCVR_Idle_c))
    {
        /* Restart RX */
        while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK)
        {
        }

        /* Filter ACK frames during RX sequence*/
        ZLL->RX_FRAME_FILTER &= ~(ZLL_RX_FRAME_FILTER_ACK_FT_MASK);
        ZLL->IRQSTS = ZLL->IRQSTS;
        ZLL->PHY_CTRL |= XCVR_RX_c;
        ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
    }
}

void kw41zRadioInit(void)
{
    XCVR_Init(ZIGBEE_MODE, DR_500KBPS);

    /* Disable all timers, enable AUTOACK and CCA before TX, mask all interrupts */
    ZLL->PHY_CTRL = ZLL_PHY_CTRL_CCATYPE(DEFAULT_CCA_MODE) | //
                    ZLL_PHY_CTRL_CCABFRTX_MASK |             //
                    ZLL_PHY_CTRL_TSM_MSK_MASK |              //
                    ZLL_PHY_CTRL_WAKE_MSK_MASK |             //
                    ZLL_PHY_CTRL_CRC_MSK_MASK |              //
                    ZLL_PHY_CTRL_PLL_UNLOCK_MSK_MASK |       //
                    ZLL_PHY_CTRL_FILTERFAIL_MSK_MASK |       //
                    ZLL_PHY_CTRL_RX_WMRK_MSK_MASK |          //
                    ZLL_PHY_CTRL_CCAMSK_MASK |               //
                    ZLL_PHY_CTRL_RXMSK_MASK |                //
                    ZLL_PHY_CTRL_TXMSK_MASK |                //
                    ZLL_PHY_CTRL_SEQMSK_MASK |               //
                    ZLL_PHY_CTRL_AUTOACK_MASK |              //
                    ZLL_PHY_CTRL_TRCV_MSK_MASK;

    /* Clear all IRQ flags and disable all timer interrupts */
    ZLL->IRQSTS = ZLL->IRQSTS;

    /*  Frame Filtering
    FRM_VER[7:6] = b11. Accept FrameVersion 0 and 1 packets, reject all others */
    ZLL->RX_FRAME_FILTER = ZLL_DEFAULT_RX_FILTERING;

    /* Set prescaller to obtain 1 symbol (16us) timebase */
    ZLL->TMR_PRESCALE = 0x05;

    /* Set CCA threshold to -75 dBm */
    ZLL->CCA_LQI_CTRL &= ~ZLL_CCA_LQI_CTRL_CCA1_THRESH_MASK;
    ZLL->CCA_LQI_CTRL |= ZLL_CCA_LQI_CTRL_CCA1_THRESH(-75);

    /* Adjust LQI compensation */
    ZLL->CCA_LQI_CTRL &= ~ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_MASK;
    ZLL->CCA_LQI_CTRL |= ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP(96);

    /* Adjust ACK delay to fulfill the 802.15.4 turnaround requirements */
    ZLL->ACKDELAY &= ~ZLL_ACKDELAY_ACKDELAY_MASK;
    ZLL->ACKDELAY |= ZLL_ACKDELAY_ACKDELAY(-8);

    /* Clear HW indirect queue */
    ZLL->SAM_TABLE |= ZLL_SAM_TABLE_INVALIDATE_ALL_MASK;

    rf_set_channel(DEFAULT_CHANNEL);
    rf_set_tx_power(0);

    sTxFrame.mLength = 0;
    sTxFrame.mPsdu   = sTxData;
    sRxFrame.mLength = 0;
#if DOUBLE_BUFFERING
    sRxFrame.mPsdu = sRxData;
#else
    sRxFrame.mPsdu = (uint8_t *)ZLL->PKT_BUFFER_RX;
#endif
}

void kw41zRadioProcess(otInstance *aInstance)
{
    if (sTxDone)
    {
        if (sTxFrame.mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] & IEEE802154_ACK_REQUEST)
        {
            otPlatRadioTxDone(aInstance, &sTxFrame, &sRxFrame, sTxStatus);
        }
        else
        {
            otPlatRadioTxDone(aInstance, &sTxFrame, NULL, sTxStatus);
        }

        sTxDone = false;
    }

    if (sRxDone)
    {
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, &sRxFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otPlatRadioReceiveDone(aInstance, &sRxFrame, OT_ERROR_NONE);
        }

        sRxDone = false;
    }

    if (sEdScanDone)
    {
        otPlatRadioEnergyScanDone(aInstance, sMaxED);
        sEdScanDone = false;
    }
}
