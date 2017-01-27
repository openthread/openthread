/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <openthread-types.h>

#include <assert.h>
#include <common/code_utils.hpp>
#include <cc2650_radio.h>
#include <platform/radio.h>
#include <platform/random.h> /* to seed the CSMA-CA funciton */

#include <driverlib/prcm.h>
#include <inc/hw_prcm.h>
#include <inc/hw_memmap.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_ccfg.h>
#include <driverlib/rfc.h>
#include <driverlib/osc.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/chipinfo.h>

/* phy state as defined by openthread */
static volatile cc2650_PhyState sState;

static output_config_t const *sCurrentOutputPower = &(rgOutputPower[OUTPUT_CONFIG_COUNT - 1]);

/* Overrides for IEEE 802.15.4, differential mode */
static uint32_t sIEEEOverrides[] =
{
    0x00354038, /* Synth: Set RTRIM (POTAILRESTRIM) to 5 */
    0x4001402D, /* Synth: Correct CKVD latency setting (address) */
    0x00608402, /* Synth: Correct CKVD latency setting (value) */
    0x000784A3, /* Synth: Set FREF = 3.43 MHz (24 MHz / 7) */
    0xA47E0583, /* Synth: Set loop bandwidth after lock to 80 kHz (K2) */
    0xEAE00603, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB) */
    0x00010623, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB) */
    0x002B50DC, /* Adjust AGC DC filter */
    0x05000243, /* Increase synth programming timeout */
    0x002082C3, /* Increase synth programming timeout */
    0xFFFFFFFF, /* End of override list */
};

/*
 * status of the pending bit of the last ack packet found by the
 * sTransmitRxAckCmd radio command
 *
 * used to pass data from the radio state ISR to the processing loop
 */
static volatile bool sReceivedAckPendingBit = false;

/*
 * number of retry counts left to the currently transmitting frame
 *
 * initialized when a frame is passed to be sent over the air, and decremented
 * by the radio ISR every time the transmit command string fails to receive a
 * corresponding ack
 */
static volatile unsigned int sTransmitRetryCount = 0;

/*
 * offset of the radio timer from the rtc
 *
 * used when we start and stop the RAT
 */
static uint32_t sRatOffset = 0;

/*
 * radio command structures that run on the CM0
 */
static volatile rfc_CMD_SYNC_START_RAT_t sStartRatCmd;
static volatile rfc_CMD_RADIO_SETUP_t sRadioSetupCmd;

static volatile rfc_CMD_SYNC_STOP_RAT_t sStopRatCmd;
static volatile rfc_CMD_FS_POWERDOWN_t sFsPowerdownCmd;

static volatile rfc_CMD_CLEAR_RX_t sClearReceiveQueueCmd;
static volatile rfc_CMD_IEEE_MOD_FILT_t sModifyReceiveFilterCmd;
static volatile rfc_CMD_IEEE_MOD_SRC_MATCH_t sModifyReceiveSrcMatchCmd;

static volatile rfc_CMD_IEEE_ED_SCAN_t sEdScanCmd;

static volatile rfc_CMD_IEEE_RX_t sReceiveCmd;

static volatile rfc_CMD_IEEE_CSMA_t sCsmacaBackoffCmd;
static volatile rfc_CMD_IEEE_TX_t sTransmitCmd;
static volatile rfc_CMD_IEEE_RX_ACK_t sTransmitRxAckCmd;

static volatile ext_src_match_data_t sSrcMatchExtData;
static volatile short_src_match_data_t sSrcMatchShortData;

/* struct containing radio stats */
static rfc_ieeeRxOutput_t sRfStats;

#define RX_BUF_SIZE 144
/* two receive buffers entries with room for 1 max IEEE802.15.4 frame in each */
static uint8_t sRxBuf0[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t sRxBuf1[RX_BUF_SIZE] __attribute__((aligned(4)));

/* The RX Data Queue */
static dataQueue_t sRxDataQueue = { 0 };

/* openthread data primatives */
static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[kMaxPHYPacketSize] __attribute__((aligned(4))) ;
static uint8_t sReceivePsdu[kMaxPHYPacketSize] __attribute__((aligned(4))) ;

/**
 * @brief initialize the RX/TX buffers
 *
 * Zeros out the receive and transmit buffers and sets up the data structures
 * of the receive queue.
 */
static void rfCoreInitBufs(void)
{
    rfc_dataEntry_t *entry;
    memset(sRxBuf0, 0x00, RX_BUF_SIZE);
    memset(sRxBuf1, 0x00, RX_BUF_SIZE);

    entry = (rfc_dataEntry_t *)sRxBuf0;
    entry->pNextEntry = sRxBuf1;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(sRxBuf0) - sizeof(rfc_dataEntry_t);

    entry = (rfc_dataEntry_t *)sRxBuf1;
    entry->pNextEntry = sRxBuf0;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(sRxBuf0) - sizeof(rfc_dataEntry_t);

    sTransmitFrame.mPsdu = sTransmitPsdu;
    sTransmitFrame.mLength = 0;
    sReceiveFrame.mPsdu = sReceivePsdu;
    sReceiveFrame.mLength = 0;
}

/**
 * @brief initialize the RX command structure
 *
 * Sets the default values for the receive command structure.
 */
static void rfCoreInitReceiveParams(void)
{
    memset((void *)&sReceiveCmd, 0x00, sizeof(sReceiveCmd));

    sReceiveCmd.commandNo = CMD_IEEE_RX;
    sReceiveCmd.status = IDLE;
    sReceiveCmd.pNextOp = NULL;
    sReceiveCmd.startTime = 0x00000000;
    sReceiveCmd.startTrigger.triggerType = TRIG_NOW;
    sReceiveCmd.condition.rule = COND_NEVER;
    sReceiveCmd.channel = kPhyMinChannel;

    sReceiveCmd.rxConfig.bAutoFlushCrc = 1;
    sReceiveCmd.rxConfig.bAutoFlushIgn = 0;
    sReceiveCmd.rxConfig.bIncludePhyHdr = 0;
    sReceiveCmd.rxConfig.bIncludeCrc = 0;
    sReceiveCmd.rxConfig.bAppendRssi = 1;
    sReceiveCmd.rxConfig.bAppendCorrCrc = 1;
    sReceiveCmd.rxConfig.bAppendSrcInd = 0;
    sReceiveCmd.rxConfig.bAppendTimestamp = 0;

    sReceiveCmd.pRxQ = &sRxDataQueue;
    sReceiveCmd.pOutput = &sRfStats;

    sReceiveCmd.frameFiltOpt.frameFiltEn = 1;
    sReceiveCmd.frameFiltOpt.frameFiltStop = 1;
    sReceiveCmd.frameFiltOpt.autoAckEn = 1;
    sReceiveCmd.frameFiltOpt.slottedAckEn = 0;
    sReceiveCmd.frameFiltOpt.autoPendEn = 0;
    sReceiveCmd.frameFiltOpt.defaultPend = 0;
    sReceiveCmd.frameFiltOpt.bPendDataReqOnly = 0;
    sReceiveCmd.frameFiltOpt.bPanCoord = 0;
    sReceiveCmd.frameFiltOpt.maxFrameVersion = 3;
    sReceiveCmd.frameFiltOpt.bStrictLenFilter = 1;

    sReceiveCmd.numShortEntries = CC2650_SHORTADD_SRC_MATCH_NUM;
    sReceiveCmd.pShortEntryList = (uint32_t *)&sSrcMatchShortData;

    sReceiveCmd.numExtEntries = CC2650_EXTADD_SRC_MATCH_NUM;
    sReceiveCmd.pExtEntryList = (uint32_t *)&sSrcMatchExtData;

    /* Receive all frame types */
    sReceiveCmd.frameTypes.bAcceptFt0Beacon = 1;
    sReceiveCmd.frameTypes.bAcceptFt1Data = 1;
    sReceiveCmd.frameTypes.bAcceptFt2Ack = 0;
    sReceiveCmd.frameTypes.bAcceptFt3MacCmd = 1;
    sReceiveCmd.frameTypes.bAcceptFt4Reserved = 1;
    sReceiveCmd.frameTypes.bAcceptFt5Reserved = 1;
    sReceiveCmd.frameTypes.bAcceptFt6Reserved = 1;
    sReceiveCmd.frameTypes.bAcceptFt7Reserved = 1;

    /* Configure CCA settings */
    sReceiveCmd.ccaOpt.ccaEnEnergy = 1;
    sReceiveCmd.ccaOpt.ccaEnCorr = 1;
    sReceiveCmd.ccaOpt.ccaEnSync = 1;
    sReceiveCmd.ccaOpt.ccaCorrOp = 1;
    sReceiveCmd.ccaOpt.ccaSyncOp = 0;
    sReceiveCmd.ccaOpt.ccaCorrThr = 3;

    sReceiveCmd.ccaRssiThr = -90;

    sReceiveCmd.endTrigger.triggerType = TRIG_NEVER;
    sReceiveCmd.endTime = 0x00000000;
}

static void rfCoreInitTransmitParams(void)
{
    uint16_t dummy;
    memset((void *)&sCsmacaBackoffCmd, 0x00, sizeof(sCsmacaBackoffCmd));

    /* initialize the random state with a true random seed for the radio core's
     * psudo rng */
    otPlatRandomSecureGet(sizeof(uint16_t) / sizeof(uint8_t), (uint8_t *) & (sCsmacaBackoffCmd.randomState), &dummy);
    sCsmacaBackoffCmd.commandNo = CMD_IEEE_CSMA;
}

/**
 * @brief sends the direct abort command to the radio core
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreExecuteAbortCmd(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT)) & 0xFF);
}

/**
 * @brief sends the direct ping command to the radio core
 *
 * Check that the Radio core is alive and able to respond to commands.
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreExecutePingCmd(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING)) & 0xFF);
}

/**
 * @brief sends the immediate clear rx queue command to the radio core
 *
 * Uses the radio core to mark all of the entries in the receive queue as
 * pending. This is used instead of clearing the entries manually to avoid race
 * conditions between the main processor and the radio core.
 *
 * @param [in] aQueue a pointer to the receive queue to be cleared
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreClearReceiveQueue(dataQueue_t *aQueue)
{
    memset((void *)&sClearReceiveQueueCmd, 0, sizeof(sClearReceiveQueueCmd));

    sClearReceiveQueueCmd.commandNo = CMD_CLEAR_RX;
    sClearReceiveQueueCmd.pQueue = aQueue;

    return (RFCDoorbellSendTo((uint32_t)&sClearReceiveQueueCmd) & 0xFF);
}

/**
 * @brief enable/disable filtering
 *
 * Uses the radio core to alter the current running RX command filtering
 * options. This ensures there is no access fault between the CM3 and CM0 for
 * the RX command.
 *
 * This function leaves the type of frames to be filtered the same as the
 * receive command.
 *
 * @note An IEEE RX command *must* be running while this command executes.
 *
 * @param [in] aEnable TRUE: enable frame filtering, FALSE: disable frame filtering
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreModifyRxFrameFilter(bool aEnable)
{
    memset((void *)&sModifyReceiveFilterCmd, 0, sizeof(sModifyReceiveFilterCmd));

    sModifyReceiveFilterCmd.commandNo = CMD_IEEE_MOD_FILT;
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameFiltOpt, (void *)&sReceiveCmd.frameFiltOpt,
           sizeof(sModifyReceiveFilterCmd.newFrameFiltOpt));
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameTypes, (void *)&sReceiveCmd.frameTypes,
           sizeof(sModifyReceiveFilterCmd.newFrameTypes));
    sModifyReceiveFilterCmd.newFrameFiltOpt.frameFiltEn = aEnable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveFilterCmd) & 0xFF);
}

/**
 * @brief enable/disable autoPend
 *
 * Uses the radio core to alter the current running RX command filtering
 * options. This ensures there is no access fault between the CM3 and CM0 for
 * the RX command.
 *
 * This function leaves the type of frames to be filtered the same as the
 * receive command.
 *
 * @note An IEEE RX command *must* be running while this command executes.
 *
 * @param [in] aEnable TRUE: enable autoPend, FALSE: disable autoPend
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreModifyRxAutoPend(bool aEnable)
{
    memset((void *)&sModifyReceiveFilterCmd, 0, sizeof(sModifyReceiveFilterCmd));

    sModifyReceiveFilterCmd.commandNo = CMD_IEEE_MOD_FILT;
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameFiltOpt, (void *)&sReceiveCmd.frameFiltOpt,
           sizeof(sModifyReceiveFilterCmd.newFrameFiltOpt));
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameTypes, (void *)&sReceiveCmd.frameTypes,
           sizeof(sModifyReceiveFilterCmd.newFrameTypes));
    sModifyReceiveFilterCmd.newFrameFiltOpt.autoPendEn = aEnable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveFilterCmd) & 0xFF);
}

/**
 * @brief sends the immediate modify source matching command to the radio core
 *
 * Uses the radio core to alter the current source matching parameters used by
 * the running RX command. This ensures there is no access fault between the
 * CM3 and CM0, and ensures that the RX command has cohesive view of the data.
 * The CM3 may make alterations to the source matching entries if the entry is
 * marked as disabled.
 *
 * @note An IEEE RX command *must* be running while this command executes.
 *
 * @param [in] aEntryNo the index of the entry to alter
 * @param [in] aType TRUE: the entry is a short address, FALSE: the entry is an extended address
 * @param [in] aEnable whether the given entry is to be enabled or disabled
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreModifySourceMatchEntry(uint8_t aEntryNo, cc2650_address_t aType, bool aEnable)
{
    memset((void *)&sModifyReceiveSrcMatchCmd, 0, sizeof(sModifyReceiveSrcMatchCmd));

    sModifyReceiveSrcMatchCmd.commandNo = CMD_IEEE_MOD_SRC_MATCH;

    /* we only use source matching for pending data bit, so enabling and
     * pending are the same to us.
     */
    if (aEnable)
    {
        sModifyReceiveSrcMatchCmd.options.bEnable = 1;
        sModifyReceiveSrcMatchCmd.options.srcPend = 1;
        sModifyReceiveSrcMatchCmd.options.entryType = aType;
    }
    else
    {
        sModifyReceiveSrcMatchCmd.options.bEnable = 0;
        sModifyReceiveSrcMatchCmd.options.srcPend = 0;
        sModifyReceiveSrcMatchCmd.options.entryType = aType;
    }

    sModifyReceiveSrcMatchCmd.entryNo = aEntryNo;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveSrcMatchCmd) & 0xFF);
}

/**
 * @brief walks the short address source match list to find an address
 *
 * @param [in] address the short address to search for
 *
 * @return the index where the address was found
 * @retval CC2650_SRC_MATCH_NONE the address was not found
 */
static uint8_t rfCoreFindShortSrcMatchIdx(const uint16_t aAddress)
{
    uint8_t i;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if (sSrcMatchShortData.extAddrEnt[i].shortAddr == aAddress)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * @brief walks the short address source match list to find an empty slot
 *
 * @return the index of an unused address slot
 * @retval CC2650_SRC_MATCH_NONE no unused slots available
 */
static uint8_t rfCoreFindEmptyShortSrcMatchIdx(void)
{
    uint8_t i;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if ((sSrcMatchShortData.srcMatchEn[i / 32] & (1 << (i % 32))) == 0u)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * @brief walks the ext address source match list to find an address
 *
 * @param [in] address the ext address to search for
 *
 * @return the index where the address was found
 * @retval CC2650_SRC_MATCH_NONE the address was not found
 */
static uint8_t rfCoreFindExtSrcMatchIdx(const uint64_t *aAddress)
{
    uint8_t i;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if (sSrcMatchExtData.extAddrEnt[i] == *aAddress)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * @brief walks the ext address source match list to find an empty slot
 *
 * @return the index of an unused address slot
 * @retval CC2650_SRC_MATCH_NONE no unused slots available
 */
static uint8_t rfCoreFindEmptyExtSrcMatchIdx(void)
{
    uint8_t i;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if ((sSrcMatchExtData.srcMatchEn[i / 32] & (1 << (i % 32))) != 0u)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * @brief sends the tx command to the radio core
 *
 * Sends the packet to the radio core to be sent asynchronously.
 *
 * @param [in] aPsdu a pointer to the data to be sent
 * @note this *must* be 4 byte aligned and not include the FCS, that is
 * calculated in hardware.
 * @param [in] aLen the length in bytes of data pointed to by psdu.
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreSendTransmitCmd(uint8_t *aPsdu, uint8_t aLen)
{
    /* no memset for CSMA-CA to preserve the random state */
    memset((void *)&sTransmitCmd, 0, sizeof(sTransmitCmd));
    /* reset retry count */
    sTransmitRetryCount = 0;

    sCsmacaBackoffCmd.status = IDLE;
    sCsmacaBackoffCmd.macMaxBE = IEEE802154_MAC_MAX_BE;
    sCsmacaBackoffCmd.macMaxCSMABackoffs = IEEE802154_MAC_MAX_CSMA_BACKOFFS;
    sCsmacaBackoffCmd.csmaConfig.initCW = 1;
    sCsmacaBackoffCmd.csmaConfig.bSlotted = 0;
    sCsmacaBackoffCmd.csmaConfig.rxOffMode = 0;
    sCsmacaBackoffCmd.NB = 0;
    sCsmacaBackoffCmd.BE = IEEE802154_MAC_MIN_BE;
    sCsmacaBackoffCmd.remainingPeriods = 0;
    sCsmacaBackoffCmd.endTrigger.triggerType = TRIG_NEVER;
    sCsmacaBackoffCmd.endTime = 0x00000000;
    sCsmacaBackoffCmd.pNextOp = (rfc_radioOp_t *) &sTransmitCmd;

    sTransmitCmd.commandNo = CMD_IEEE_TX;
    /* no need to look for an ack if the tx operation was stopped */
    sTransmitCmd.payloadLen = aLen;
    sTransmitCmd.pPayload = aPsdu;
    sTransmitCmd.startTrigger.triggerType = TRIG_NOW;

    /* always clear the rx_ack and retries commands just incase the status is
     * not IDLE for the interrupt handler */
    memset((void *)&sTransmitRxAckCmd, 0, sizeof(sTransmitRxAckCmd));

    if (aPsdu[0] & IEEE802154_ACK_REQUEST)
    {
        /* setup the receive ack command to follow the tx command */
        sTransmitCmd.condition.rule = COND_STOP_ON_FALSE;
        sTransmitCmd.pNextOp = (rfc_radioOp_t *) &sTransmitRxAckCmd;

        sTransmitRxAckCmd.commandNo = CMD_IEEE_RX_ACK;
        sTransmitRxAckCmd.startTrigger.triggerType = TRIG_NOW;
        sTransmitRxAckCmd.seqNo = aPsdu[IEEE802154_DSN_OFFSET];
        sTransmitRxAckCmd.endTrigger.triggerType = TRIG_REL_START;
        sTransmitRxAckCmd.endTrigger.pastTrig = 1;
        sTransmitRxAckCmd.condition.rule = COND_NEVER;
        sTransmitRxAckCmd.pNextOp = NULL;
        /* number of RAT ticks to wait before claiming we haven't received an ack */
        sTransmitRxAckCmd.endTime = ((IEEE802154_MAC_ACK_WAIT_DURATION * CC2650_RAT_TICKS_PER_SEC) /
                                     IEEE802154_SYMBOLS_PER_SEC);
    }
    else
    {
        /* we are not looking for an ack, no retries because we won't know if we need to */
        sTransmitCmd.condition.rule = COND_NEVER;
        sTransmitCmd.pNextOp = NULL;
    }

    return (RFCDoorbellSendTo((uint32_t)&sCsmacaBackoffCmd) & 0xFF);
}

/**
 * @brief sends the rx command to the radio core
 *
 * Sends the pre-built receive command to the radio core. This sets up the
 * radio to receive packets according to the settings in the global rx command.
 *
 * @note This function does not alter any of the parameters of the rx command.
 * It is only concerned with sending the command to the radio core. See @ref
 * otPlatRadioSetPanId for an example of how the rx settings are set changed.
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rfCoreSendReceiveCmd(void)
{
    sReceiveCmd.status = IDLE;
    return (RFCDoorbellSendTo((uint32_t)&sReceiveCmd) & 0xFF);
}

static uint_fast8_t rfCoreSendEdScanCmd(uint8_t aChannel, uint16_t aDurration)
{
    memset((void *)&sEdScanCmd, 0, sizeof(sEdScanCmd));

    sEdScanCmd.commandNo = CMD_IEEE_ED_SCAN;
    sEdScanCmd.startTrigger.triggerType = TRIG_NOW;
    sEdScanCmd.condition.rule = COND_NEVER;
    sEdScanCmd.channel = aChannel;

    sEdScanCmd.ccaOpt.ccaEnEnergy = 1;
    sEdScanCmd.ccaOpt.ccaEnCorr = 1;
    sEdScanCmd.ccaOpt.ccaEnSync = 1;
    sEdScanCmd.ccaOpt.ccaCorrOp = 1;
    sEdScanCmd.ccaOpt.ccaSyncOp = 0;
    sEdScanCmd.ccaOpt.ccaCorrThr = 3;

    sEdScanCmd.ccaRssiThr = -90;

    sEdScanCmd.endTrigger.triggerType = TRIG_REL_START;
    sEdScanCmd.endTrigger.pastTrig = 1;
    /* durration is in ms */
    sEdScanCmd.endTime = aDurration * (CC2650_RAT_TICKS_PER_SEC / 1000);

    return (RFCDoorbellSendTo((uint32_t)&sEdScanCmd) & 0xFF);
}

/**
 * @brief enables the cpe0 and cpe1 radio interrupts
 *
 * Enables the @ref IRQ_LAST_COMMAND_DONE and @ref IRQ_LAST_FG_COMMAND_DONE to
 * be handled by the @ref RFCCPE0IntHandler interrupt handler.
 */
static void rfCoreSetupInt(void)
{
    bool interruptsWereDisabled;

    /* We are already turned on by the caller, so this should not happen */
    if (!PRCMRfReady())
    {
        return;
    }

    interruptsWereDisabled = IntMasterDisable();

    /* Set all interrupt channels to CPE0 channel, error to CPE1 */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = IRQ_INTERNAL_ERROR;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = IRQ_LAST_COMMAND_DONE | IRQ_LAST_FG_COMMAND_DONE;

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntEnable(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_1);

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }
}

/**
 * @brief disables and clears the cpe0 and cpe1 radio interrupts
 */
static void rfCoreStopInt(void)
{
    bool interruptsWereDisabled;

    interruptsWereDisabled = IntMasterDisable();

    /* clear and disable interrupts */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntDisable(INT_RFC_CPE_0);
    IntDisable(INT_RFC_CPE_1);

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }
}

/**
 * @brief Sets the mode for the radio core to IEEE 802.15.4
 */
static void rfCoreSetModeSelect(void)
{
    switch (ChipInfo_GetChipType())
    {
    case CHIP_TYPE_CC2650:
        HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE5;
        break;

    case CHIP_TYPE_CC2630:
        HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE2;
        break;

    default:
        /* This code must be run on a valid cc26xx chip */
        assert(false);
        break;
    }
}

/**
 * @brief turns on the radio core
 *
 * Sets up the power and resources for the radio core.
 * - switches the high frequency clock to the xosc crystal
 * - sets the mode for the radio core to IEEE 802.15.4
 * - initializes the rx buffers and command
 * - powers on the radio core power domain
 * - enables the radio core power domain
 * - sets up the interrupts
 * - sends the ping command to the radio core to make sure it is running
 *
 * @return the value from the ping command to the radio core
 * @retval CMDSTA_Done the radio core is alive and responding
 */
static uint_fast8_t rfCorePowerOn(void)
{
    bool interruptsWereDisabled;

    /* Request the HF XOSC as the source for the HF clock. Needed before we can
     * use the FS. This will only request, it will _not_ perform the switch.
     */
    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF)
    {
        /* Request to switch to the crystal to enable radio operation. It takes a
         * while for the XTAL to be ready so instead of performing the actual
         * switch, we do other stuff while the XOSC is getting ready.
         */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_XOSC_HF);
    }

    rfCoreSetModeSelect();

    /* Set of RF Core data queue. Circular buffer, no last entry */
    sRxDataQueue.pCurrEntry = sRxBuf0;
    sRxDataQueue.pLastEntry = NULL;

    rfCoreInitBufs();

    /*
     * Trigger a switch to the XOSC, so that we can subsequently use the RF FS
     * This will block until the XOSC is actually ready, but give how we
     * requested it early on, this won't be too long a wait.
     * This should be done before starting the RAT.
     */
    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF)
    {
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }

    interruptsWereDisabled = IntMasterDisable();

    /* Enable RF Core power domain */
    PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);

    while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_ON)
    {
        ;
    }

    PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();

    while (!PRCMLoadGet())
    {
        ;
    }

    rfCoreSetupInt();

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M |
                                                       RFC_PWR_PWMCLKEN_CPERAM_M);

    /* Send ping (to verify RFCore is ready and alive) */
    return rfCoreExecutePingCmd();
}

/**
 * @brief turns off the radio core
 *
 * Switches off the power and resources for the radio core.
 * - disables the interrupts
 * - disables the radio core power domain
 * - powers off the radio core power domain
 * - switches the high frequency clock to the rcosc to save power
 */
static void rfCorePowerOff(void)
{
    rfCoreStopInt();

    PRCMDomainDisable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();

    while (!PRCMLoadGet())
    {
        ;
    }

    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);

    while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_OFF)
    {
        ;
    }

    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_RCOSC_HF)
    {
        /* Request to switch to the RC osc for low power mode. */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_RCOSC_HF);
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }
}

/**
 * @brief sends the setup command string to the radio core
 *
 * Enables the clock line from the RTC to the RF core RAT. Enables the RAT
 * timer and sets up the radio in IEEE mode.
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command was received
 */
static uint_fast16_t rfCoreSendEnableCmd(void)
{
    uint8_t ret;
    bool interruptsWereDisabled;
    /* turn on the clock line to the radio core */
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the rat start command */
    memset((void *)&sStartRatCmd, 0x00, sizeof(sStartRatCmd));
    sStartRatCmd.commandNo = CMD_SYNC_START_RAT;
    sStartRatCmd.condition.rule = COND_STOP_ON_FALSE;
    sStartRatCmd.pNextOp = (rfc_radioOp_t *) &sRadioSetupCmd;
    sStartRatCmd.rat0 = sRatOffset;

    /* initialize radio setup command */
    memset((void *)&sRadioSetupCmd, 0, sizeof(sRadioSetupCmd));
    sRadioSetupCmd.commandNo = CMD_RADIO_SETUP;
    sRadioSetupCmd.condition.rule = COND_NEVER;
    /* initally set the radio tx power to the max */
    sRadioSetupCmd.txPower = sCurrentOutputPower->value;
    sRadioSetupCmd.pRegOverride = sIEEEOverrides;
    sRadioSetupCmd.mode = 1;

    interruptsWereDisabled = IntMasterDisable();

    if ((ret = (RFCDoorbellSendTo((uint32_t)&sStartRatCmd) & 0xFF)) != CMDSTA_Done)
    {
        if (!interruptsWereDisabled)
        {
            IntMasterEnable();
        }
        return ret;
    }

    /* synchronously wait for the CM0 to stop executing */
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00)
    {
        ;
    }

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    return sRadioSetupCmd.status;
}

/**
 * @brief sends the shutdown command string to the radio core
 *
 * Powers down the frequency synthesizer and stops the RAT.
 *
 * @note synchronously waits until the command string completes.
 *
 * @return the status of the RAT stop command
 * @retval DONE_OK the command string executed properly
 */
static uint_fast16_t rfCoreSendDisableCmd(void)
{
    uint8_t doorbell_ret;
    bool interruptsWereDisabled;

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the command to power down the frequency synth */
    memset((void *)&sFsPowerdownCmd, 0, sizeof(sFsPowerdownCmd));
    sFsPowerdownCmd.commandNo = CMD_FS_POWERDOWN;
    sFsPowerdownCmd.condition.rule = COND_ALWAYS;
    sFsPowerdownCmd.pNextOp = (rfc_radioOp_t *)&sStopRatCmd;

    memset((void *)&sStopRatCmd, 0, sizeof(sStopRatCmd));
    sStopRatCmd.commandNo = CMD_SYNC_STOP_RAT;
    sStopRatCmd.condition.rule = COND_NEVER;

    interruptsWereDisabled = IntMasterDisable();

    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&sFsPowerdownCmd) & 0xFF);

    if (doorbell_ret != CMDSTA_Done)
    {
        if (!interruptsWereDisabled)
        {
            IntMasterEnable();
        }
        return doorbell_ret;
    }

    /* synchronously wait for the CM0 to stop */
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00)
    {
        ;
    }

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    if (sStopRatCmd.status == DONE_OK)
    {
        sRatOffset = sStopRatCmd.rat0;
    }

    return sStopRatCmd.status;
}

/**
 * error interrupt handler
 */
void RFCCPE1IntHandler(void)
{
    if (!PRCMRfReady())
    {
    }

    /* Clear INTERNAL_ERROR interrupt flag */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x7FFFFFFF;
}

/**
 * command done handler
 */
void RFCCPE0IntHandler(void)
{
    if (HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE)
    {
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

        if (sState == cc2650_stateReceive)
        {
            if (sReceiveCmd.status != ACTIVE && sReceiveCmd.status != IEEE_SUSPENDED)
            {
                /* the rx command was probably aborted to change the channel */
                sState = cc2650_stateSleep;
            }
        }
        else if (sState == cc2650_stateEdScan)
        {
            sState = cc2650_stateSleep;
        }
    }

    if (HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_FG_COMMAND_DONE)
    {
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_FG_COMMAND_DONE;

        if (sState == cc2650_stateTransmit)
        {
            if (sTransmitRxAckCmd.status == IEEE_DONE_TIMEOUT)
            {
                if (sTransmitRetryCount < IEEE802154_MAC_MAX_FRAMES_RETRIES)
                {
                    /* re-submit the tx command chain */
                    sTransmitRetryCount++;
                    RFCDoorbellSendTo((uint32_t)&sCsmacaBackoffCmd);
                }
                else
                {
                    /* signal polling function we are done transmitting, we failed to send the packet */
                    sState = cc2650_stateTransmitComplete;
                    sTransmitError = kThreadError_NoAck;
                }
            }
            else if (sTransmitRxAckCmd.status != IDLE)
            {
                /* signal polling function we are done transmitting */
                sState = cc2650_stateTransmitComplete;

                /* we were looking for an ack, and the TX command didn't fail */
                switch (sTransmitRxAckCmd.status)
                {
                case IEEE_DONE_ACK:
                    sReceivedAckPendingBit = false;
                    sTransmitError = kThreadError_None;
                    break;

                case IEEE_DONE_ACKPEND:
                    sReceivedAckPendingBit = true;
                    sTransmitError = kThreadError_None;
                    break;

                case IEEE_DONE_TIMEOUT:
                    sTransmitError = kThreadError_NoAck;
                    break;

                default:
                    sTransmitError = kThreadError_Failed;
                    break;
                }
            }
            else
            {
                /* signal polling function we are done transmitting */
                sState = cc2650_stateTransmitComplete;

                /* The TX command was either stopped or we are not looking for
                 * an ack */
                switch (sTransmitCmd.status)
                {
                case IEEE_DONE_OK:
                    sReceivedAckPendingBit = false;
                    sTransmitError = kThreadError_None;
                    break;

                case IEEE_DONE_TIMEOUT:
                    sTransmitError = kThreadError_ChannelAccessFailure;
                    break;

                case IEEE_ERROR_NO_SETUP:
                case IEEE_ERROR_NO_FS:
                case IEEE_ERROR_SYNTH_PROG:
                    sTransmitError = kThreadError_InvalidState;
                    break;

                case IEEE_ERROR_TXUNF:
                    sTransmitError = kThreadError_NoBufs;
                    break;

                default:
                    sTransmitError = kThreadError_Error;
                    break;
                }
            }
        }
    }
}

/**
 * Function documented in platform-cc2650.h
 */
void cc2650RadioInit(void)
{
    /* Populate the RX parameters data structure with default values */
    rfCoreInitReceiveParams();
    /* Populate the CSMA parameters */
    rfCoreInitTransmitParams();

    sState = cc2650_stateDisabled;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == cc2650_stateSleep)
    {
        error = kThreadError_None;
    }

    if (sState == cc2650_stateDisabled)
    {
        VerifyOrExit(rfCorePowerOn() == CMDSTA_Done, error = kThreadError_Failed);
        VerifyOrExit(rfCoreSendEnableCmd() == DONE_OK, error = kThreadError_Failed);
        sState = cc2650_stateSleep;
    }

exit:

    if (error == kThreadError_Failed)
    {
        rfCorePowerOff();
        sState = cc2650_stateDisabled;
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return (sState != cc2650_stateDisabled);
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == cc2650_stateDisabled)
    {
        error = kThreadError_None;
    }

    if (sState == cc2650_stateSleep)
    {
        rfCoreSendDisableCmd();
        /* we don't want to fail if this command string doesn't work, just turn
         * off the whole core
         */
        rfCorePowerOff();
        sState = cc2650_stateDisabled;
        error = kThreadError_None;
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == cc2650_stateSleep)
    {
        sState = cc2650_stateEdScan;
        VerifyOrExit(rfCoreSendEdScanCmd(aScanChannel, aScanDuration) == CMDSTA_Done, error = kThreadError_Failed);
        error = kThreadError_None;
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == cc2650_stateSleep)
    {
        sState = cc2650_stateReceive;

        /* initialize the receive command
         * XXX: no memset here because we assume init has been called and we
         *      may have changed some values in the rx command
         */
        sReceiveCmd.channel = aChannel;
        VerifyOrExit(rfCoreSendReceiveCmd() == CMDSTA_Done, error = kThreadError_Failed);
        error = kThreadError_None;
    }
    else if (sState == cc2650_stateReceive)
    {
        if (sReceiveCmd.status == ACTIVE && sReceiveCmd.channel == aChannel)
        {
            /* we are already running on the right channel */
            sState = cc2650_stateReceive;
            error = kThreadError_None;
        }
        else
        {
            /* we have either not fallen back into our receive command or
             * we are running on the wrong channel. Either way assume the
             * caller correctly called us and abort all running commands.
             */
            VerifyOrExit(rfCoreExecuteAbortCmd() == CMDSTA_Done, error = kThreadError_Failed);

            /* any frames in the queue will be for the old channel */
            VerifyOrExit(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done, error = kThreadError_Failed);

            sReceiveCmd.channel = aChannel;
            VerifyOrExit(rfCoreSendReceiveCmd() == CMDSTA_Done, error = kThreadError_Failed);

            sState = cc2650_stateReceive;
            error = kThreadError_None;
        }
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == cc2650_stateSleep)
    {
        error = kThreadError_None;
    }

    if (sState == cc2650_stateReceive)
    {
        if (rfCoreExecuteAbortCmd() != CMDSTA_Done)
        {
            error = kThreadError_Busy;
            return error;
        }

        sState = cc2650_stateSleep;
        return error;
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;
    return &sTransmitFrame;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket)
{
    ThreadError error = kThreadError_Busy;

    if (sState == cc2650_stateReceive)
    {
        /*
         * This is the easiest way to setup the frequency synthesizer.
         * And we are supposed to fall into the receive state afterwards.
         */
        error = otPlatRadioReceive(aInstance, aPacket->mChannel);

        if (error == kThreadError_None)
        {
            sState = cc2650_stateTransmit;

            /* removing 2 bytes of CRC placeholder because we generate that in hardware */
            VerifyOrExit(rfCoreSendTransmitCmd(aPacket->mPsdu, aPacket->mLength - 2) == CMDSTA_Done, error = kThreadError_Failed);
            error = kThreadError_None;
        }
    }

exit:
    sTransmitError = error;
    return error;
}

/**
 * Function documented in platform/radio.h
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;
    return sRfStats.maxRssi;
}

/**
 * Function documented in platform/radio.h
 */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;
    return kRadioCapsAckTimeout | kRadioCapsEnergyScan | kRadioCapsTransmitRetries;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        SuccessOrExit(rfCoreModifyRxAutoPend(aEnable) == CMDSTA_Done);
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        sReceiveCmd.frameFiltOpt.autoPendEn = aEnable ? 1 : 0;
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx = rfCoreFindShortSrcMatchIdx(aShortAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        VerifyOrExit((idx = rfCoreFindEmptyShortSrcMatchIdx()) != CC2650_SRC_MATCH_NONE, error = kThreadError_NoBufs);
        sSrcMatchShortData.extAddrEnt[idx].shortAddr = aShortAddress;
        sSrcMatchShortData.extAddrEnt[idx].panId = sReceiveCmd.localPanID;
    }

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rfCoreModifySourceMatchEntry(idx, SHORT_ADDRESS, true) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        sSrcMatchShortData.srcPendEn[idx / 32] |= (1 << (idx % 32));
        sSrcMatchShortData.srcMatchEn[idx / 32] |= (1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx;
    VerifyOrExit((idx = rfCoreFindShortSrcMatchIdx(aShortAddress)) != CC2650_SRC_MATCH_NONE,
                 error = kThreadError_NoAddress);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rfCoreModifySourceMatchEntry(idx, SHORT_ADDRESS, false) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        sSrcMatchShortData.srcPendEn[idx / 32] &= ~(1 << (idx % 32));
        sSrcMatchShortData.srcMatchEn[idx / 32] &= ~(1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx = rfCoreFindExtSrcMatchIdx((uint64_t *)aExtAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        VerifyOrExit((idx = rfCoreFindEmptyExtSrcMatchIdx()) != CC2650_SRC_MATCH_NONE, error = kThreadError_NoBufs);
        sSrcMatchExtData.extAddrEnt[idx] = *((uint64_t *)aExtAddress);
    }

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rfCoreModifySourceMatchEntry(idx, EXT_ADDRESS, true) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        sSrcMatchExtData.srcPendEn[idx / 32] |= (1 << (idx % 32));
        sSrcMatchExtData.srcMatchEn[idx / 32] |= (1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx;
    VerifyOrExit((idx = rfCoreFindExtSrcMatchIdx((uint64_t *)aExtAddress)) != CC2650_SRC_MATCH_NONE,
                 error = kThreadError_NoAddress);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rfCoreModifySourceMatchEntry(idx, EXT_ADDRESS, false) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        sSrcMatchExtData.srcPendEn[idx] = 0u;
        sSrcMatchExtData.srcMatchEn[idx] = 0u;
        sSrcMatchExtData.srcPendEn[idx / 32] &= ~(1 << (idx % 32));
        sSrcMatchExtData.srcMatchEn[idx / 32] &= ~(1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;
    /* we are promiscuous if we are not filtering */
    return sReceiveCmd.frameFiltOpt.frameFiltEn == 0;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        /* if we are promiscuous, then frame filtering should be disabled */
        SuccessOrExit(rfCoreModifyRxFrameFilter(!aEnable) == CMDSTA_Done);
        /* XXX should we dump any queued messages ? */
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        sReceiveCmd.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    uint8_t *eui64;
    unsigned int i;
    bool unknown = true;
    (void)aInstance;

    /* The IEEE MAC address can be stored two places. We check the Customer
     * Configuration was not set before defaulting to the Factory
     * Configuration.
     */
    eui64 = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);

    for (i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        if (eui64[i] != CC2650_UNKNOWN_EUI64)
        {
            unknown = false;
        }
    }

    if (unknown)
    {
        eui64 = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
    }

    /* The IEEE MAC address is stored in network byte order (big endian).
     * The caller seems to want the address stored in little endian format,
     * which is backwards of the conventions setup by @ref
     * otPlatRadioSetExtendedAddress. otPlatRadioSetExtendedAddress assumes
     * that the address being passed to it is in network byte order (big
     * endian), so the caller of otPlatRadioSetExtendedAddress must swap the
     * endianness before calling.
     *
     * It may be easier to have the caller of this function store the IEEE
     * address in network byte order (big endian).
     */
    for (i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        aIeeeEui64[i] = eui64[(OT_EXT_ADDRESS_SIZE - 1) - i];
    }
}

/**
 * Function documented in platform/radio.h
 *
 * @note it is entirely possible for this function to fail, but there is no
 * valid way to return that error since the funciton prototype was changed.
 */
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid)
{
    (void)aInstance;

    /* XXX: if the pan id is the broadcast pan id (0xFFFF) the auto ack will
     * not work. This is due to the design of the CM0 and follows IEEE 802.15.4
     */
    if (sState == cc2650_stateReceive)
    {
        VerifyOrExit(rfCoreExecuteAbortCmd() == CMDSTA_Done, ;);
        sReceiveCmd.localPanID = aPanid;
        VerifyOrExit(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done, ;);
        VerifyOrExit(rfCoreSendReceiveCmd() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = cc2650_stateReceive;
    }
    else if (sState != cc2650_stateTransmit)
    {
        sReceiveCmd.localPanID = aPanid;
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 *
 * @note it is entirely possible for this function to fail, but there is no
 * valid way to return that error since the funciton prototype was changed.
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aAddress)
{
    (void)aInstance;

    /* XXX: assuming little endian format */
    if (sState == cc2650_stateReceive)
    {
        VerifyOrExit(rfCoreExecuteAbortCmd() == CMDSTA_Done, ;);
        sReceiveCmd.localExtAddr = *((uint64_t *)(aAddress));
        VerifyOrExit(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done, ;);
        VerifyOrExit(rfCoreSendReceiveCmd() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = cc2650_stateReceive;
    }
    else if (sState != cc2650_stateTransmit)
    {
        sReceiveCmd.localExtAddr = *((uint64_t *)(aAddress));
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 *
 * @note it is entirely possible for this function to fail, but there is no
 * valid way to return that error since the funciton prototype was changed.
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    (void)aInstance;

    if (sState == cc2650_stateReceive)
    {
        VerifyOrExit(rfCoreExecuteAbortCmd() == CMDSTA_Done, ;);
        sReceiveCmd.localShortAddr = aAddress;
        VerifyOrExit(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done, ;);
        VerifyOrExit(rfCoreSendReceiveCmd() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = cc2650_stateReceive;
    }
    else if (sState != cc2650_stateTransmit)
    {
        sReceiveCmd.localShortAddr = aAddress;
    }

exit:
    return;
}

/**
 * @brief search the receive queue for unprocessed messages
 *
 * Loop through the receive queue structure looking for data entries that the
 * radio core has marked as finished. Then place those in @ref sReceiveFrame
 * and mark any errors in @ref sReceiveError.
 */
static void readFrame(void)
{
    rfc_ieeeRxCorrCrc_t *crcCorr;
    uint8_t rssi;
    rfc_dataEntryGeneral_t *startEntry = (rfc_dataEntryGeneral_t *)sRxDataQueue.pCurrEntry;
    rfc_dataEntryGeneral_t *curEntry = startEntry;

    /* loop through receive queue */
    do
    {
        uint8_t *payload = &(curEntry->data);

        if (sReceiveFrame.mLength == 0 && curEntry->status == DATA_ENTRY_FINISHED)
        {
            uint8_t len = payload[0];
            /* get the information appended to the end of the frame.
             * This array access looks like it is a fencepost error, but the
             * first byte is the number of bytes that follow.
             */
            crcCorr = (rfc_ieeeRxCorrCrc_t *)&payload[len];
            rssi = payload[len - 1];

            if (crcCorr->status.bCrcErr == 0 && (len - 2) < kMaxPHYPacketSize)
            {
                sReceiveFrame.mLength = len;
                memcpy(sReceiveFrame.mPsdu, &(payload[1]), len - 2);
                sReceiveFrame.mChannel = sReceiveCmd.channel;
                sReceiveFrame.mPower = rssi;
                sReceiveFrame.mLqi = crcCorr->status.corr;
                sReceiveError = kThreadError_None;
            }
            else
            {
                sReceiveError = kThreadError_FcsErr;
            }

            curEntry->status = DATA_ENTRY_PENDING;
            break;
        }
        else if (curEntry->status == DATA_ENTRY_UNFINISHED)
        {
            curEntry->status = DATA_ENTRY_PENDING;
        }

        curEntry = (rfc_dataEntryGeneral_t *)(curEntry->pNextEntry);
    }
    while (curEntry != startEntry);

    return;
}

/**
 * Function documented in platform-cc2650.h
 */
int cc2650RadioProcess(otInstance *aInstance)
{
    if (sState == cc2650_stateEdScan)
    {
        if (sEdScanCmd.status == IEEE_DONE_OK)
        {
            otPlatRadioEnergyScanDone(aInstance, sEdScanCmd.maxRssi);
        }
        else if (sEdScanCmd.status == ACTIVE)
        {
            otPlatRadioEnergyScanDone(aInstance, CC2650_INVALID_RSSI);
        }
    }

    if (sState == cc2650_stateTransmitComplete || sTransmitError != kThreadError_None)
    {
        /* we are not looking for an ACK packet, or failed */
        sState = cc2650_stateReceive;
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, sReceivedAckPendingBit, sTransmitError);
        }
        else
#endif /* OPENTHREAD_ENABLE_DIAG */
        {
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, sReceivedAckPendingBit, sTransmitError);
        }
    }

    if (sState == cc2650_stateReceive || sState == cc2650_stateTransmit)
    {
        readFrame();

        if (sReceiveFrame.mLength > 0)
        {
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
            }
            else
#endif /* OPENTHREAD_ENABLE_DIAG */
            {
                otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
            }
        }

        sReceiveFrame.mLength = 0;
    }

    return 0;
}
