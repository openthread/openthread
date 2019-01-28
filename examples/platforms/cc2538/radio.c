/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "platform-cc2538.h"
#include "common/logging.hpp"
#include "utils/code_utils.h"

enum
{
    IEEE802154_MIN_LENGTH      = 5,
    IEEE802154_MAX_LENGTH      = 127,
    IEEE802154_ACK_LENGTH      = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_ACK  = 0x2,
    IEEE802154_FRAME_PENDING   = 1 << 4,
    IEEE802154_ACK_REQUEST     = 1 << 5,
    IEEE802154_DSN_OFFSET      = 2,
};

enum
{
    CC2538_RSSI_OFFSET  = 73,
    CC2538_CRC_BIT_MASK = 0x80,
    CC2538_LQI_BIT_MASK = 0x7f,
};

enum
{
    CC2538_RECEIVE_SENSITIVITY = -88, // dBm
};

typedef struct TxPowerTable
{
    int8_t  mTxPowerVal;
    uint8_t mTxPowerReg;
} TxPowerTable;

// The transmit power table, the values are from SmartRF Studio 2.4.0
static const TxPowerTable sTxPowerTable[] = {
    {7, 0xFF},   //
    {5, 0xED},   //
    {3, 0xD5},   //
    {1, 0xC5},   //
    {0, 0xB6},   //
    {-1, 0xB0},  //
    {-3, 0xA1},  //
    {-5, 0x91},  //
    {-7, 0x88},  //
    {-9, 0x72},  //
    {-11, 0x62}, //
    {-13, 0x58}, //
    {-15, 0x42}, //
    {-24, 0x00}, //
};

static otRadioFrame sTransmitFrame;
static otRadioFrame sReceiveFrame;
static otError      sTransmitError;
static otError      sReceiveError;

static uint8_t sTransmitPsdu[IEEE802154_MAX_LENGTH];
static uint8_t sReceivePsdu[IEEE802154_MAX_LENGTH];
static uint8_t sChannel = 0;
static int8_t  sTxPower = 0;

static otRadioState sState             = OT_RADIO_STATE_DISABLED;
static bool         sIsReceiverEnabled = false;

void enableReceiver(void)
{
    if (!sIsReceiverEnabled)
    {
        otLogInfoPlat("Enabling receiver", NULL);

        // flush rxfifo
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;

        // enable receiver
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_RXON;
        sIsReceiverEnabled     = true;
    }
}

void disableReceiver(void)
{
    if (sIsReceiverEnabled)
    {
        otLogInfoPlat("Disabling receiver", NULL);

        while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
            ;

        // flush rxfifo
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;

        if (HWREG(RFCORE_XREG_RXENABLE) != 0)
        {
            // disable receiver
            HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_RFOFF;
        }

        sIsReceiverEnabled = false;
    }
}

void setChannel(uint8_t aChannel)
{
    if (sChannel != aChannel)
    {
        bool enabled = false;

        if (sIsReceiverEnabled)
        {
            disableReceiver();
            enabled = true;
        }

        otLogInfoPlat("Channel=%d", aChannel);

        HWREG(RFCORE_XREG_FREQCTRL) = 11 + (aChannel - 11) * 5;
        sChannel                    = aChannel;

        if (enabled)
        {
            enableReceiver();
        }
    }
}

void setTxPower(int8_t aTxPower)
{
    uint8_t i = 0;

    if (sTxPower != aTxPower)
    {
        otLogInfoPlat("TxPower=%d", aTxPower);

        for (i = sizeof(sTxPowerTable) / sizeof(TxPowerTable) - 1; i > 0; i--)
        {
            if (aTxPower < sTxPowerTable[i].mTxPowerVal)
            {
                break;
            }
        }

        HWREG(RFCORE_XREG_TXPOWER) = sTxPowerTable[i].mTxPowerReg;
        sTxPower                   = aTxPower;
    }
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    // EUI64 is in a mixed-endian format.  Split in two halves, each 32-bit
    // half is in little-endian format (machine endian).  However, the
    // most significant part of the EUI64 comes first, so we can't cheat
    // with a uint64_t!
    //
    // See https://e2e.ti.com/support/wireless_connectivity/low_power_rf_tools/f/155/p/307344/1072252

    volatile uint32_t *eui64 = &HWREG(IEEE_EUI64);

    // Read first 32-bits
    uint32_t part = eui64[0];
    for (uint8_t i = 0; i < (OT_EXT_ADDRESS_SIZE / 2); i++)
    {
        aIeeeEui64[3 - i] = part;
        part >>= 8;
    }

    // Read the last 32-bits
    part = eui64[1];
    for (uint8_t i = 0; i < (OT_EXT_ADDRESS_SIZE / 2); i++)
    {
        aIeeeEui64[7 - i] = part;
        part >>= 8;
    }
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogInfoPlat("PANID=%X", aPanid);

    HWREG(RFCORE_FFSM_PAN_ID0) = aPanid & 0xFF;
    HWREG(RFCORE_FFSM_PAN_ID1) = aPanid >> 8;
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogInfoPlat("ExtAddr=%X%X%X%X%X%X%X%X", aAddress->m8[7], aAddress->m8[6], aAddress->m8[5], aAddress->m8[4],
                  aAddress->m8[3], aAddress->m8[2], aAddress->m8[1], aAddress->m8[0]);

    for (int i = 0; i < 8; i++)
    {
        ((volatile uint32_t *)RFCORE_FFSM_EXT_ADDR0)[i] = aAddress->m8[i];
    }
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogInfoPlat("ShortAddr=%X", aAddress);

    HWREG(RFCORE_FFSM_SHORT_ADDR0) = aAddress & 0xFF;
    HWREG(RFCORE_FFSM_SHORT_ADDR1) = aAddress >> 8;
}

void cc2538RadioInit(void)
{
    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu   = sTransmitPsdu;
    sReceiveFrame.mLength  = 0;
    sReceiveFrame.mPsdu    = sReceivePsdu;

    // enable clock
    HWREG(SYS_CTRL_RCGCRFC) = SYS_CTRL_RCGCRFC_RFC0;
    HWREG(SYS_CTRL_SCGCRFC) = SYS_CTRL_SCGCRFC_RFC0;
    HWREG(SYS_CTRL_DCGCRFC) = SYS_CTRL_DCGCRFC_RFC0;

    // Table 23-7.
    HWREG(RFCORE_XREG_AGCCTRL1)              = 0x15;
    HWREG(RFCORE_XREG_TXFILTCFG)             = 0x09;
    HWREG(ANA_REGS_BASE + ANA_REGS_O_IVCTRL) = 0x0b;

    HWREG(RFCORE_XREG_CCACTRL0)  = 0xf8;
    HWREG(RFCORE_XREG_FIFOPCTRL) = IEEE802154_MAX_LENGTH;

    HWREG(RFCORE_XREG_FRMCTRL0) = RFCORE_XREG_FRMCTRL0_AUTOCRC | RFCORE_XREG_FRMCTRL0_AUTOACK;

    // default: SRCMATCH.SRC_MATCH_EN(1), SRCMATCH.AUTOPEND(1),
    // SRCMATCH.PEND_DATAREQ_ONLY(1), RFCORE_XREG_FRMCTRL1_PENDING_OR(0)

    HWREG(RFCORE_XREG_TXPOWER) = sTxPowerTable[0].mTxPowerReg;
    sTxPower                   = sTxPowerTable[0].mTxPowerVal;

    otLogInfoPlat("Initialized", NULL);
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    if (!otPlatRadioIsEnabled(aInstance))
    {
        otLogDebgPlat("State=OT_RADIO_STATE_SLEEP", NULL);
        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    if (otPlatRadioIsEnabled(aInstance))
    {
        otLogDebgPlat("State=OT_RADIO_STATE_DISABLED", NULL);
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
        otLogDebgPlat("State=OT_RADIO_STATE_SLEEP", NULL);
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
        otLogDebgPlat("State=OT_RADIO_STATE_RECEIVE", NULL);

        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_RECEIVE;
        setChannel(aChannel);
        sReceiveFrame.mChannel = aChannel;
        enableReceiver();
    }

    return error;
}

static void setupTransmit(otRadioFrame *aFrame)
{
    int i;

    // wait for current TX operation to complete, if any.
    while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    // flush txfifo
    HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHTX;
    HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHTX;

    // frame length
    HWREG(RFCORE_SFR_RFDATA) = aFrame->mLength;

    // frame data
    for (i = 0; i < aFrame->mLength; i++)
    {
        HWREG(RFCORE_SFR_RFDATA) = aFrame->mPsdu[i];
    }

    setChannel(aFrame->mChannel);
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        int i;

        error          = OT_ERROR_NONE;
        sState         = OT_RADIO_STATE_TRANSMIT;
        sTransmitError = OT_ERROR_NONE;

        setupTransmit(aFrame);

        // Set up a counter to inform us if we get stuck.
        i = 1000000;

        // Wait for radio to enter receive state.
        while ((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_RX_ACTIVE) == 0)
        {
            // Count down the cycles, and emit a message if we get to zero.
            // Ideally, we should never get there!
            if (i)
            {
                i--;
            }
            else
            {
                otLogCritPlat("Radio is stuck!!! FSMSTAT0=0x%08x FSMSTAT1=0x%08x RFERRF=0x%08x",
                              HWREG(RFCORE_XREG_FSMSTAT0), HWREG(RFCORE_XREG_FSMSTAT1), HWREG(RFCORE_SFR_RFERRF));
                i = 1000000;
            }

            // Ensure we haven't overflowed the RX buffer in the mean time, as this
            // will cause a deadlock here otherwise.  Similarly, if we see an aborted
            // RX, handle that here too to prevent deadlock.
            if (HWREG(RFCORE_SFR_RFERRF) & (RFCORE_SFR_RFERRF_RXOVERF | RFCORE_SFR_RFERRF_RXABO))
            {
                if (HWREG(RFCORE_SFR_RFERRF) & RFCORE_SFR_RFERRF_RXOVERF)
                {
                    otLogCritPlat("RX Buffer Overflow detected", NULL);
                }

                if (HWREG(RFCORE_SFR_RFERRF) & RFCORE_SFR_RFERRF_RXABO)
                {
                    otLogCritPlat("Aborted RX detected", NULL);
                }

                // Flush the RX buffer
                HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
                HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
            }

            // Check for idle state.  After flushing the RX buffer, we may wind up here.
            if (!(HWREG(RFCORE_XREG_FSMSTAT1) & (RFCORE_XREG_FSMSTAT1_TX_ACTIVE | RFCORE_XREG_FSMSTAT1_RX_ACTIVE)))
            {
                otLogCritPlat("Idle state detected", NULL);

                // In this case, the state of our driver mis-matches our state.  So force
                // matters by clearing our channel variable and calling setChannel.  This
                // should bring our radio into the RX state, which should allow us to go
                // into TX.
                sChannel = 0;
                setupTransmit(aFrame);
            }
        }

        // wait for valid rssi
        while ((HWREG(RFCORE_XREG_RSSISTAT) & RFCORE_XREG_RSSISTAT_RSSI_VALID) == 0)
            ;

        otEXPECT_ACTION(((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_CCA) &&
                         !((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_SFD))),
                        sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE);

        // begin transmit
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_TXON;

        otPlatRadioTxStarted(aInstance, aFrame);

        while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
            ;

        otLogDebgPlat("Transmitted %d bytes", aFrame->mLength);
    }

exit:
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

    return (HWREG(RFCORE_XREG_FRMFILT0) & RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN) == 0;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogInfoPlat("PromiscuousMode=%d", aEnable ? 1 : 0);

    if (aEnable)
    {
        HWREG(RFCORE_XREG_FRMFILT0) &= ~RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN;
    }
    else
    {
        HWREG(RFCORE_XREG_FRMFILT0) |= RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN;
    }
}

void readFrame(otInstance *aInstance)
{
    uint8_t length;
    uint8_t crcCorr;
    int     i;

    otEXPECT(sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT);
    otEXPECT((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_FIFOP) != 0);

    // read length
    length = HWREG(RFCORE_SFR_RFDATA);
    otEXPECT(IEEE802154_MIN_LENGTH <= length && length <= IEEE802154_MAX_LENGTH);

    if (otPlatRadioGetPromiscuous(aInstance))
    {
        // Timestamp
        sReceiveFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
        sReceiveFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
    }

    // read psdu
    for (i = 0; i < length - 2; i++)
    {
        sReceiveFrame.mPsdu[i] = HWREG(RFCORE_SFR_RFDATA);
    }

    sReceiveFrame.mInfo.mRxInfo.mRssi = (int8_t)HWREG(RFCORE_SFR_RFDATA) - CC2538_RSSI_OFFSET;
    crcCorr                           = HWREG(RFCORE_SFR_RFDATA);

    if (crcCorr & CC2538_CRC_BIT_MASK)
    {
        sReceiveFrame.mLength            = length;
        sReceiveFrame.mInfo.mRxInfo.mLqi = crcCorr & CC2538_LQI_BIT_MASK;
    }
    else
    {
        // resets rxfifo
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;

        otLogDebgPlat("Dropping %d received bytes (Invalid CRC)", length);
    }

    // check for rxfifo overflow
    if ((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_FIFOP) != 0 &&
        (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_FIFO) == 0)
    {
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
    }

exit:
    return;
}

void cc2538RadioProcess(otInstance *aInstance)
{
    readFrame(aInstance);

    if ((sState == OT_RADIO_STATE_RECEIVE && sReceiveFrame.mLength > 0) ||
        (sState == OT_RADIO_STATE_TRANSMIT && sReceiveFrame.mLength > IEEE802154_ACK_LENGTH))
    {
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
        }
        else
#endif
        {
            // signal MAC layer for each received frame if promiscuous is enabled
            // otherwise only signal MAC layer for non-ACK frame
            if (((HWREG(RFCORE_XREG_FRMFILT0) & RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN) == 0) ||
                (sReceiveFrame.mLength > IEEE802154_ACK_LENGTH))
            {
                otLogDebgPlat("Received %d bytes", sReceiveFrame.mLength);
                otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
            }
        }
    }

    if (sState == OT_RADIO_STATE_TRANSMIT)
    {
        if (sTransmitError != OT_ERROR_NONE || (sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
        {
            if (sTransmitError != OT_ERROR_NONE)
            {
                otLogDebgPlat("Transmit failed ErrorCode=%d", sTransmitError);
            }

            sState = OT_RADIO_STATE_RECEIVE;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, sTransmitError);
            }
            else
#endif
            {
                otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
            }
        }
        else if (sReceiveFrame.mLength == IEEE802154_ACK_LENGTH &&
                 (sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK &&
                 (sReceiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET]))
        {
            sState = OT_RADIO_STATE_RECEIVE;

            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveFrame, sTransmitError);
        }
    }

    sReceiveFrame.mLength = 0;
}

void RFCoreRxTxIntHandler(void)
{
    HWREG(RFCORE_SFR_RFIRQF0) = 0;
}

void RFCoreErrIntHandler(void)
{
    HWREG(RFCORE_SFR_RFERRF) = 0;
}

uint32_t getSrcMatchEntriesEnableStatus(bool aShort)
{
    uint32_t  status = 0;
    uint32_t *addr   = aShort ? (uint32_t *)RFCORE_XREG_SRCSHORTEN0 : (uint32_t *)RFCORE_XREG_SRCEXTEN0;

    for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_ENABLE_STATUS_SIZE; i++)
    {
        status |= HWREG(addr++) << (i * 8);
    }

    return status;
}

int8_t findSrcMatchShortEntry(const uint16_t aShortAddress)
{
    int8_t    entry = -1;
    uint16_t  shortAddr;
    uint32_t  bitMask;
    uint32_t *addr   = NULL;
    uint32_t  status = getSrcMatchEntriesEnableStatus(true);

    for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_SHORT_ENTRIES; i++)
    {
        bitMask = 0x00000001 << i;

        if ((status & bitMask) == 0)
        {
            continue;
        }

        addr = (uint32_t *)RFCORE_FFSM_SRCADDRESS_TABLE + (i * RFCORE_XREG_SRCMATCH_SHORT_ENTRY_OFFSET);

        shortAddr = HWREG(addr + 2);
        shortAddr |= HWREG(addr + 3) << 8;

        if ((shortAddr == aShortAddress))
        {
            entry = i;
            break;
        }
    }

    return entry;
}

int8_t findSrcMatchExtEntry(const otExtAddress *aExtAddress)
{
    int8_t    entry = -1;
    uint32_t  bitMask;
    uint32_t *addr   = NULL;
    uint32_t  status = getSrcMatchEntriesEnableStatus(false);

    for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_EXT_ENTRIES; i++)
    {
        uint8_t j = 0;
        bitMask   = 0x00000001 << 2 * i;

        if ((status & bitMask) == 0)
        {
            continue;
        }

        addr = (uint32_t *)RFCORE_FFSM_SRCADDRESS_TABLE + (i * RFCORE_XREG_SRCMATCH_EXT_ENTRY_OFFSET);

        for (j = 0; j < sizeof(otExtAddress); j++)
        {
            if (HWREG(addr + j) != aExtAddress->m8[j])
            {
                break;
            }
        }

        if (j == sizeof(otExtAddress))
        {
            entry = i;
            break;
        }
    }

    return entry;
}

void setSrcMatchEntryEnableStatus(bool aShort, uint8_t aEntry, bool aEnable)
{
    uint8_t   entry          = aShort ? aEntry : (2 * aEntry);
    uint8_t   index          = entry / 8;
    uint32_t *addrEn         = aShort ? (uint32_t *)RFCORE_XREG_SRCSHORTEN0 : (uint32_t *)RFCORE_XREG_SRCEXTEN0;
    uint32_t *addrAutoPendEn = aShort ? (uint32_t *)RFCORE_FFSM_SRCSHORTPENDEN0 : (uint32_t *)RFCORE_FFSM_SRCEXTPENDEN0;
    uint32_t  bitMask        = 0x00000001;

    if (aEnable)
    {
        HWREG(addrEn + index) |= (bitMask) << (entry % 8);
        HWREG(addrAutoPendEn + index) |= (bitMask) << (entry % 8);
    }
    else
    {
        HWREG(addrEn + index) &= ~((bitMask) << (entry % 8));
        HWREG(addrAutoPendEn + index) &= ~((bitMask) << (entry % 8));
    }
}

int8_t findSrcMatchAvailEntry(bool aShort)
{
    int8_t   entry = -1;
    uint32_t bitMask;
    uint32_t shortEnableStatus = getSrcMatchEntriesEnableStatus(true);
    uint32_t extEnableStatus   = getSrcMatchEntriesEnableStatus(false);

    otLogDebgPlat("Short enable status: 0x%x", shortEnableStatus);
    otLogDebgPlat("Ext enable status: 0x%x", extEnableStatus);

    if (aShort)
    {
        bitMask = 0x00000001;

        for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_SHORT_ENTRIES; i++)
        {
            if ((extEnableStatus & bitMask) == 0)
            {
                if ((shortEnableStatus & bitMask) == 0)
                {
                    entry = i;
                    break;
                }
            }

            if (i % 2 == 1)
            {
                extEnableStatus = extEnableStatus >> 2;
            }

            shortEnableStatus = shortEnableStatus >> 1;
        }
    }
    else
    {
        bitMask = 0x00000003;

        for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_EXT_ENTRIES; i++)
        {
            if (((extEnableStatus | shortEnableStatus) & bitMask) == 0)
            {
                entry = i;
                break;
            }

            extEnableStatus   = extEnableStatus >> 2;
            shortEnableStatus = shortEnableStatus >> 2;
        }
    }

    return entry;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogInfoPlat("EnableSrcMatch=%d", aEnable ? 1 : 0);

    if (aEnable)
    {
        // only set FramePending when ack for data poll if there are queued messages
        // for entries in the source match table.
        HWREG(RFCORE_XREG_FRMCTRL1) &= ~RFCORE_XREG_FRMCTRL1_PENDING_OR;
    }
    else
    {
        // set FramePending for all ack.
        HWREG(RFCORE_XREG_FRMCTRL1) |= RFCORE_XREG_FRMCTRL1_PENDING_OR;
    }
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError   error = OT_ERROR_NONE;
    int8_t    entry = findSrcMatchAvailEntry(true);
    uint32_t *addr  = (uint32_t *)RFCORE_FFSM_SRCADDRESS_TABLE;

    otLogDebgPlat("Add ShortAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0, error = OT_ERROR_NO_BUFS);

    addr += (entry * RFCORE_XREG_SRCMATCH_SHORT_ENTRY_OFFSET);

    HWREG(addr++) = HWREG(RFCORE_FFSM_PAN_ID0);
    HWREG(addr++) = HWREG(RFCORE_FFSM_PAN_ID1);
    HWREG(addr++) = aShortAddress & 0xFF;
    HWREG(addr++) = aShortAddress >> 8;

    setSrcMatchEntryEnableStatus(true, (uint8_t)(entry), true);

exit:
    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError   error = OT_ERROR_NONE;
    int8_t    entry = findSrcMatchAvailEntry(false);
    uint32_t *addr  = (uint32_t *)RFCORE_FFSM_SRCADDRESS_TABLE;

    otLogDebgPlat("Add ExtAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0, error = OT_ERROR_NO_BUFS);

    addr += (entry * RFCORE_XREG_SRCMATCH_EXT_ENTRY_OFFSET);

    for (uint8_t i = 0; i < sizeof(otExtAddress); i++)
    {
        HWREG(addr++) = aExtAddress->m8[i];
    }

    setSrcMatchEntryEnableStatus(false, (uint8_t)(entry), true);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int8_t  entry = findSrcMatchShortEntry(aShortAddress);

    otLogDebgPlat("Clear ShortAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0, error = OT_ERROR_NO_ADDRESS);

    setSrcMatchEntryEnableStatus(true, (uint8_t)(entry), false);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int8_t  entry = findSrcMatchExtEntry(aExtAddress);

    otLogDebgPlat("Clear ExtAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0, error = OT_ERROR_NO_ADDRESS);

    setSrcMatchEntryEnableStatus(false, (uint8_t)(entry), false);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t *addrEn         = (uint32_t *)RFCORE_XREG_SRCSHORTEN0;
    uint32_t *addrAutoPendEn = (uint32_t *)RFCORE_FFSM_SRCSHORTPENDEN0;

    otLogDebgPlat("Clear ShortAddr entries", NULL);

    for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_ENABLE_STATUS_SIZE; i++)
    {
        HWREG(addrEn++)         = 0;
        HWREG(addrAutoPendEn++) = 0;
    }
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t *addrEn         = (uint32_t *)RFCORE_XREG_SRCEXTEN0;
    uint32_t *addrAutoPendEn = (uint32_t *)RFCORE_FFSM_SRCEXTPENDEN0;

    otLogDebgPlat("Clear ExtAddr entries", NULL);

    for (uint8_t i = 0; i < RFCORE_XREG_SRCMATCH_ENABLE_STATUS_SIZE; i++)
    {
        HWREG(addrEn++)         = 0;
        HWREG(addrAutoPendEn++) = 0;
    }
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
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sTxPower;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    setTxPower(aPower);
    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return CC2538_RECEIVE_SENSITIVITY;
}
