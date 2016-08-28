/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <openthread-types.h>
#include <openthread-config.h>

#include <common/code_utils.hpp>
#include <platform/radio.h>
#include <platform/diag.h>
#include "platform-cc2538.h"

enum
{
    IEEE802154_MIN_LENGTH = 5,
    IEEE802154_MAX_LENGTH = 127,
    IEEE802154_ACK_LENGTH = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_ACK = 0x2,
    IEEE802154_FRAME_PENDING = 1 << 4,
    IEEE802154_ACK_REQUEST = 1 << 5,
    IEEE802154_DSN_OFFSET = 2,
};

enum
{
    CC2538_RSSI_OFFSET = 73,
    CC2538_CRC_BIT_MASK = 0x80,
    CC2538_LQI_BIT_MASK = 0x7f,
};

static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[IEEE802154_MAX_LENGTH];
static uint8_t sReceivePsdu[IEEE802154_MAX_LENGTH];

static PhyState sState = kStateDisabled;
static bool sIsReceiverEnabled = false;

void enableReceiver(void)
{
    if (!sIsReceiverEnabled)
    {
        // flush rxfifo
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHRX;

        // enable receiver
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_RXON;
        sIsReceiverEnabled = true;
    }
}

void disableReceiver(void)
{
    if (sIsReceiverEnabled)
    {
        while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE);

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

void setChannel(uint8_t channel)
{
    HWREG(RFCORE_XREG_FREQCTRL) = 11 + (channel - 11) * 5;
}

ThreadError otPlatRadioSetPanId(uint16_t panid)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        HWREG(RFCORE_FFSM_PAN_ID0) = panid & 0xFF;
        HWREG(RFCORE_FFSM_PAN_ID1) = panid >> 8;
        error = kThreadError_None;
    }

    return error;
}

ThreadError otPlatRadioSetExtendedAddress(uint8_t *address)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        int i;

        for (i = 0; i < 8; i++)
        {
            ((volatile uint32_t *)RFCORE_FFSM_EXT_ADDR0)[i] = address[i];
        }

        error = kThreadError_None;
    }

    return error;
}

ThreadError otPlatRadioSetShortAddress(uint16_t address)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        HWREG(RFCORE_FFSM_SHORT_ADDR0) = address & 0xFF;
        HWREG(RFCORE_FFSM_SHORT_ADDR1) = address >> 8;
        error = kThreadError_None;
    }

    return error;
}

void cc2538RadioInit(void)
{
    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu = sTransmitPsdu;
    sReceiveFrame.mLength = 0;
    sReceiveFrame.mPsdu = sReceivePsdu;

    // enable clock
    HWREG(SYS_CTRL_RCGCRFC) = SYS_CTRL_RCGCRFC_RFC0;
    HWREG(SYS_CTRL_SCGCRFC) = SYS_CTRL_SCGCRFC_RFC0;
    HWREG(SYS_CTRL_DCGCRFC) = SYS_CTRL_DCGCRFC_RFC0;

    // Table 23-7.
    HWREG(RFCORE_XREG_AGCCTRL1) = 0x15;
    HWREG(RFCORE_XREG_TXFILTCFG) = 0x09;
    HWREG(ANA_REGS_BASE + ANA_REGS_O_IVCTRL) = 0x0b;

    HWREG(RFCORE_XREG_CCACTRL0) = 0xf8;
    HWREG(RFCORE_XREG_FIFOPCTRL) = IEEE802154_MAX_LENGTH;

    HWREG(RFCORE_XREG_FRMCTRL0) = RFCORE_XREG_FRMCTRL0_AUTOCRC | RFCORE_XREG_FRMCTRL0_AUTOACK;
    HWREG(RFCORE_XREG_FRMCTRL1) |= RFCORE_XREG_FRMCTRL1_PENDING_OR;

    // disable source address matching
    HWREG(RFCORE_XREG_SRCMATCH) = 0;
}

ThreadError otPlatRadioEnable(void)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateSleep || sState == kStateDisabled)
    {
        error = kThreadError_None;
        sState = kStateSleep;
    }

    return error;
}

ThreadError otPlatRadioDisable(void)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateDisabled || sState == kStateSleep)
    {
        error = kThreadError_None;
        sState = kStateDisabled;
    }

    return error;
}

ThreadError otPlatRadioSleep(void)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateSleep || sState == kStateReceive)
    {
        error = kThreadError_None;
        sState = kStateSleep;
        disableReceiver();
    }

    return error;
}

ThreadError otPlatRadioReceive(uint8_t aChannel)
{
    ThreadError error = kThreadError_Busy;

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

ThreadError otPlatRadioTransmit(void)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateReceive)
    {
        int i;

        error = kThreadError_None;
        sState = kStateTransmit;
        sTransmitError = kThreadError_None;

        while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE);

        // flush txfifo
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHTX;
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_FLUSHTX;

        // frame length
        HWREG(RFCORE_SFR_RFDATA) = sTransmitFrame.mLength;

        // frame data
        for (i = 0; i < sTransmitFrame.mLength; i++)
        {
            HWREG(RFCORE_SFR_RFDATA) = sTransmitFrame.mPsdu[i];
        }

        setChannel(sTransmitFrame.mChannel);

        while ((HWREG(RFCORE_XREG_FSMSTAT1) & 1) == 0);

        // wait for valid rssi
        while ((HWREG(RFCORE_XREG_RSSISTAT) & RFCORE_XREG_RSSISTAT_RSSI_VALID) == 0);

        VerifyOrExit(HWREG(RFCORE_XREG_FSMSTAT1) & (RFCORE_XREG_FSMSTAT1_CCA | RFCORE_XREG_FSMSTAT1_SFD),
                     sTransmitError = kThreadError_ChannelAccessFailure);

        // begin transmit
        HWREG(RFCORE_SFR_RFST) = RFCORE_SFR_RFST_INSTR_TXON;

        while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE);
    }

exit:
    return error;
}

RadioPacket *otPlatRadioGetTransmitBuffer(void)
{
    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(void)
{
    return 0;
}

otRadioCaps otPlatRadioGetCaps(void)
{
    return kRadioCapsNone;
}

bool otPlatRadioGetPromiscuous(void)
{
    return (HWREG(RFCORE_XREG_FRMFILT0) & RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN) == 0;
}

void otPlatRadioSetPromiscuous(bool aEnable)
{
    if (aEnable)
    {
        HWREG(RFCORE_XREG_FRMFILT0) &= ~RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN;
    }
    else
    {
        HWREG(RFCORE_XREG_FRMFILT0) |= RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN;
    }
}

void readFrame(void)
{
    uint8_t length;
    uint8_t crcCorr;
    int i;

    VerifyOrExit(sState == kStateReceive || sState == kStateTransmit, ;);
    VerifyOrExit((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_FIFOP) != 0, ;);

    // read length
    length = HWREG(RFCORE_SFR_RFDATA);
    VerifyOrExit(IEEE802154_MIN_LENGTH <= length && length <= IEEE802154_MAX_LENGTH, ;);

    // read psdu
    for (i = 0; i < length - 2; i++)
    {
        sReceiveFrame.mPsdu[i] = HWREG(RFCORE_SFR_RFDATA);
    }

    sReceiveFrame.mPower = (int8_t)HWREG(RFCORE_SFR_RFDATA) - CC2538_RSSI_OFFSET;
    crcCorr = HWREG(RFCORE_SFR_RFDATA);

    if (crcCorr & CC2538_CRC_BIT_MASK)
    {
        sReceiveFrame.mLength = length;
        sReceiveFrame.mLqi = crcCorr & CC2538_LQI_BIT_MASK;
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

void cc2538RadioProcess(void)
{
    readFrame();

    if ((sState == kStateReceive) && (sReceiveFrame.mLength > 0))
    {
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(&sReceiveFrame, sReceiveError);
        }
        else
#endif
        {
            otPlatRadioReceiveDone(&sReceiveFrame, sReceiveError);
        }
    }

    if (sState == kStateTransmit)
    {
        if (sTransmitError != kThreadError_None || (sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
        {
            sState = kStateReceive;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(false, sTransmitError);
            }
            else
#endif
            {
                otPlatRadioTransmitDone(false, sTransmitError);
            }
        }
        else if (sReceiveFrame.mLength == IEEE802154_ACK_LENGTH &&
                 (sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK &&
                 (sReceiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET]))
        {
            sState = kStateReceive;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0, sTransmitError);
            }
            else
#endif
            {
                otPlatRadioTransmitDone((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0, sTransmitError);
            }
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
