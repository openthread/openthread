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

#include <openthread/config.h>

#include "cc2650_radio.h"
#include <assert.h>
#include <utils/code_utils.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/random.h> /* to seed the CSMA-CA funciton */

#include <driverlib/chipinfo.h>
#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_memmap.h>
#include <inc/hw_prcm.h>

enum
{
    CC2650_RECEIVE_SENSITIVITY = -100, // dBm
};

/* phy state as defined by openthread */
static volatile cc2650_PhyState sState;

static output_config_t const *sCurrentOutputPower = &(rgOutputPower[OUTPUT_CONFIG_COUNT - 1]);

/* Overrides for IEEE 802.15.4, differential mode */
static uint32_t sIEEEOverrides[] = {
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
 * Number of retry counts left to the currently transmitting frame.
 *
 * Initialized when a frame is passed to be sent over the air, and decremented
 * by the radio ISR every time the transmit command string fails to receive a
 * corresponding ack.
 */
static volatile unsigned int sTransmitRetryCount = 0;

/*
 * Offset of the radio timer from the rtc.
 *
 * Used when we start and stop the RAT on enabling and disabling of the rf
 * core.
 */
static uint32_t sRatOffset = 0;

/*
 * Radio command structures that run on the CM0.
 */
// clang-format off
static volatile __attribute__((aligned(4))) rfc_CMD_SYNC_START_RAT_t     sStartRatCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_RADIO_SETUP_t        sRadioSetupCmd;

static volatile __attribute__((aligned(4))) rfc_CMD_FS_POWERDOWN_t       sFsPowerdownCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_SYNC_STOP_RAT_t      sStopRatCmd;

static volatile __attribute__((aligned(4))) rfc_CMD_CLEAR_RX_t           sClearReceiveQueueCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_MOD_FILT_t      sModifyReceiveFilterCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_MOD_SRC_MATCH_t sModifyReceiveSrcMatchCmd;

static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_ED_SCAN_t       sEdScanCmd;

static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_RX_t            sReceiveCmd;

static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_CSMA_t          sCsmacaBackoffCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_TX_t            sTransmitCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_RX_ACK_t        sTransmitRxAckCmd;

static volatile __attribute__((aligned(4))) ext_src_match_data_t         sSrcMatchExtData;
static volatile __attribute__((aligned(4))) short_src_match_data_t       sSrcMatchShortData;
// clang-format on

/*
 * Structure containing radio statistics.
 */
static __attribute__((aligned(4))) rfc_ieeeRxOutput_t sRfStats;

#define RX_BUF_SIZE 144
/* two receive buffers entries with room for 1 max IEEE802.15.4 frame in each */
static uint8_t sRxBuf0[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t sRxBuf1[RX_BUF_SIZE] __attribute__((aligned(4)));

/* The RX Data Queue */
static dataQueue_t sRxDataQueue = {0};

/* openthread data primatives */
static otRadioFrame sTransmitFrame;
static otError      sTransmitError;

static __attribute__((aligned(4))) uint8_t sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE];

static volatile bool sTxCmdChainDone = false;

/*
 * Interrupt handlers forward declared for register functions.
 */
void RFCCPE0IntHandler(void);
void RFCCPE1IntHandler(void);

/**
 * Initialize the RX/TX buffers.
 *
 * Zeros out the receive and transmit buffers and sets up the data structures
 * of the receive queue.
 */
static void rfCoreInitBufs(void)
{
    rfc_dataEntry_t *entry;
    memset(sRxBuf0, 0x00, RX_BUF_SIZE);
    memset(sRxBuf1, 0x00, RX_BUF_SIZE);

    entry               = (rfc_dataEntry_t *)sRxBuf0;
    entry->pNextEntry   = sRxBuf1;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length       = sizeof(sRxBuf0) - sizeof(rfc_dataEntry_t);

    entry               = (rfc_dataEntry_t *)sRxBuf1;
    entry->pNextEntry   = sRxBuf0;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length       = sizeof(sRxBuf1) - sizeof(rfc_dataEntry_t);

    sTransmitFrame.mPsdu   = sTransmitPsdu;
    sTransmitFrame.mLength = 0;
}

/**
 * Initialize the RX command structure.
 *
 * Sets the default values for the receive command structure.
 */
static void rfCoreInitReceiveParams(void)
{
    // clang-format off
    static const rfc_CMD_IEEE_RX_t cReceiveCmd =
    {
        .commandNo                  = CMD_IEEE_RX,
        .status                     = IDLE,
        .pNextOp                    = NULL,
        .startTime                  = 0u,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
        .channel                    = OT_RADIO_CHANNEL_MIN,
        .rxConfig                   =
        {
            .bAutoFlushCrc          = 1,
            .bAutoFlushIgn          = 0,
            .bIncludePhyHdr         = 0,
            .bIncludeCrc            = 0,
            .bAppendRssi            = 1,
            .bAppendCorrCrc         = 1,
            .bAppendSrcInd          = 0,
            .bAppendTimestamp       = 0,
        },
        .frameFiltOpt               =
        {
            .frameFiltEn            = 1,
            .frameFiltStop          = 1,
            .autoAckEn              = 1,
            .slottedAckEn           = 0,
            .autoPendEn             = 0,
            .defaultPend            = 0,
            .bPendDataReqOnly       = 0,
            .bPanCoord              = 0,
            .maxFrameVersion        = 3,
            .bStrictLenFilter       = 1,
        },
        .frameTypes                 =
        {
            .bAcceptFt0Beacon       = 1,
            .bAcceptFt1Data         = 1,
            .bAcceptFt2Ack          = 1,
            .bAcceptFt3MacCmd       = 1,
            .bAcceptFt4Reserved     = 1,
            .bAcceptFt5Reserved     = 1,
            .bAcceptFt6Reserved     = 1,
            .bAcceptFt7Reserved     = 1,
        },
        .ccaOpt                     =
        {
            .ccaEnEnergy            = 1,
            .ccaEnCorr              = 1,
            .ccaEnSync              = 1,
            .ccaCorrOp              = 1,
            .ccaSyncOp              = 0,
            .ccaCorrThr             = 3,
        },
        .ccaRssiThr                 = -90,
        .endTrigger                 =
        {
            .triggerType            = TRIG_NEVER,
        },
        .endTime                    = 0u,
    };
    // clang-format on
    sReceiveCmd = cReceiveCmd;

    sReceiveCmd.pRxQ    = &sRxDataQueue;
    sReceiveCmd.pOutput = &sRfStats;

    sReceiveCmd.numShortEntries = CC2650_SHORTADD_SRC_MATCH_NUM;
    sReceiveCmd.pShortEntryList = (void *)&sSrcMatchShortData;

    sReceiveCmd.numExtEntries = CC2650_EXTADD_SRC_MATCH_NUM;
    sReceiveCmd.pExtEntryList = (uint32_t *)&sSrcMatchExtData;
}

/**
 * Sends the direct abort command to the radio core.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreExecuteAbortCmd(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT)) & 0xFF);
}

/**
 * Sends the direct ping command to the radio core.
 *
 * Check that the Radio core is alive and able to respond to commands.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreExecutePingCmd(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING)) & 0xFF);
}

/**
 * Sends the immediate clear rx queue command to the radio core.
 *
 * Uses the radio core to mark all of the entries in the receive queue as
 * pending. This is used instead of clearing the entries manually to avoid race
 * conditions between the main processor and the radio core.
 *
 * @param [in] aQueue A pointer to the receive queue to be cleared.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreClearReceiveQueue(dataQueue_t *aQueue)
{
    /* memset skipped because sClearReceiveQueueCmd has only 2 members and padding */
    sClearReceiveQueueCmd.commandNo = CMD_CLEAR_RX;
    sClearReceiveQueueCmd.pQueue    = aQueue;

    return (RFCDoorbellSendTo((uint32_t)&sClearReceiveQueueCmd) & 0xFF);
}

/**
 * Enable/disable frame filtering.
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
 * @param [in] aEnable TRUE: enable frame filtering,
 *                     FALSE: disable frame filtering.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreModifyRxFrameFilter(bool aEnable)
{
    /* memset skipped because sModifyReceiveFilterCmd has only 3 members */
    sModifyReceiveFilterCmd.commandNo = CMD_IEEE_MOD_FILT;
    /* copy current frame filtering and frame types from running RX command */
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameFiltOpt, (void *)&sReceiveCmd.frameFiltOpt,
           sizeof(sModifyReceiveFilterCmd.newFrameFiltOpt));
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameTypes, (void *)&sReceiveCmd.frameTypes,
           sizeof(sModifyReceiveFilterCmd.newFrameTypes));

    sModifyReceiveFilterCmd.newFrameFiltOpt.frameFiltEn = aEnable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveFilterCmd) & 0xFF);
}

/**
 * Enable/disable autoPend feature.
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
 * @param [in] aEnable TRUE: enable autoPend,
 *                     FALSE: disable autoPend.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreModifyRxAutoPend(bool aEnable)
{
    /* memset skipped because sModifyReceiveFilterCmd has only 3 members */
    sModifyReceiveFilterCmd.commandNo = CMD_IEEE_MOD_FILT;
    /* copy current frame filtering and frame types from running RX command */
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameFiltOpt, (void *)&sReceiveCmd.frameFiltOpt,
           sizeof(sModifyReceiveFilterCmd.newFrameFiltOpt));
    memcpy((void *)&sModifyReceiveFilterCmd.newFrameTypes, (void *)&sReceiveCmd.frameTypes,
           sizeof(sModifyReceiveFilterCmd.newFrameTypes));

    sModifyReceiveFilterCmd.newFrameFiltOpt.autoPendEn = aEnable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveFilterCmd) & 0xFF);
}

/**
 * Sends the immediate modify source matching command to the radio core.
 *
 * Uses the radio core to alter the current source matching parameters used by
 * the running RX command. This ensures there is no access fault between the
 * CM3 and CM0, and ensures that the RX command has cohesive view of the data.
 * The CM3 may make alterations to the source matching entries if the entry is
 * marked as disabled.
 *
 * @note An IEEE RX command *must* be running while this command executes.
 *
 * @param [in] aEntryNo The index of the entry to alter.
 * @param [in] aType    TRUE: the entry is a short address,
 *                      FALSE: the entry is an extended address.
 * @param [in] aEnable  Whether the given entry is to be enabled or disabled.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreModifySourceMatchEntry(uint8_t aEntryNo, cc2650_address_t aType, bool aEnable)
{
    /* memset kept to save 60 bytes of text space, gcc can't optimize the
     * following bitfield operation if it doesn't know the fields are zero
     * already.
     */
    memset((void *)&sModifyReceiveSrcMatchCmd, 0, sizeof(sModifyReceiveSrcMatchCmd));

    sModifyReceiveSrcMatchCmd.commandNo = CMD_IEEE_MOD_SRC_MATCH;

    /* we only use source matching for pending data bit, so enabling and
     * pending are the same to us.
     */
    if (aEnable)
    {
        sModifyReceiveSrcMatchCmd.options.bEnable = 1;
        sModifyReceiveSrcMatchCmd.options.srcPend = 1;
    }
    else
    {
        sModifyReceiveSrcMatchCmd.options.bEnable = 0;
        sModifyReceiveSrcMatchCmd.options.srcPend = 0;
    }

    sModifyReceiveSrcMatchCmd.options.entryType = aType;
    sModifyReceiveSrcMatchCmd.entryNo           = aEntryNo;

    return (RFCDoorbellSendTo((uint32_t)&sModifyReceiveSrcMatchCmd) & 0xFF);
}

/**
 * Walks the short address source match list to find an address.
 *
 * @param [in] aAddress The short address to search for.
 *
 * @return The index where the address was found.
 * @retval CC2650_SRC_MATCH_NONE The address was not found.
 */
static uint8_t rfCoreFindShortSrcMatchIdx(const uint16_t aAddress)
{
    uint8_t i;
    uint8_t ret = CC2650_SRC_MATCH_NONE;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if (sSrcMatchShortData.extAddrEnt[i].shortAddr == aAddress)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/**
 * Walks the short address source match list to find an empty slot.
 *
 * @return The index of an unused address slot.
 * @retval CC2650_SRC_MATCH_NONE No unused slots available.
 */
static uint8_t rfCoreFindEmptyShortSrcMatchIdx(void)
{
    uint8_t i;
    uint8_t ret = CC2650_SRC_MATCH_NONE;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if ((sSrcMatchShortData.srcMatchEn[i / 32] & (1 << (i % 32))) == 0u)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/**
 * Walks the extended address source match list to find an address.
 *
 * @param [in] aAddress The extended address to search for.
 *
 * @return The index where the address was found.
 * @retval CC2650_SRC_MATCH_NONE The address was not found.
 */
static uint8_t rfCoreFindExtSrcMatchIdx(const uint64_t *aAddress)
{
    uint8_t i;
    uint8_t ret = CC2650_SRC_MATCH_NONE;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if (sSrcMatchExtData.extAddrEnt[i] == *aAddress)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/**
 * Walks the extended address source match list to find an empty slot.
 *
 * @return The index of an unused address slot.
 * @retval CC2650_SRC_MATCH_NONE No unused slots available.
 */
static uint8_t rfCoreFindEmptyExtSrcMatchIdx(void)
{
    uint8_t i;
    uint8_t ret = CC2650_SRC_MATCH_NONE;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if ((sSrcMatchExtData.srcMatchEn[i / 32] & (1 << (i % 32))) != 0u)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/**
 * Sends the tx command to the radio core.
 *
 * Sends the packet to the radio core to be sent asynchronously.
 *
 * @note @ref aPsdu *must* be 4 byte aligned and not include the FCS.
 *
 * @param [in] aPsdu A pointer to the data to be sent.
 * @param [in] aLen  The length in bytes of data pointed to by PSDU.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreSendTransmitCmd(uint8_t *aPsdu, uint8_t aLen)
{
    // clang-format off
    static const rfc_CMD_IEEE_CSMA_t cCsmacaBackoffCmd =
    {
        .commandNo                  = CMD_IEEE_CSMA,
        .status                     = IDLE,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_ALWAYS,
        },
        .macMaxBE                   = IEEE802154_MAC_MAX_BE,
        .macMaxCSMABackoffs         = IEEE802154_MAC_MAX_CSMA_BACKOFFS,
        .csmaConfig                 =
        {
            .initCW                 = 1,
            .bSlotted               = 0,
            .rxOffMode              = 0,
        },
        .NB                         = 0,
        .BE                         = IEEE802154_MAC_MIN_BE,
        .remainingPeriods           = 0,
        .endTrigger                 =
        {
            .triggerType            = TRIG_NEVER,
        },
        .endTime                    = 0x00000000,
    };
    static const rfc_CMD_IEEE_TX_t cTransmitCmd =
    {
        .commandNo                  = CMD_IEEE_TX,
        .status                     = IDLE,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
        .pNextOp                    = NULL,
    };
    static const rfc_CMD_IEEE_RX_ACK_t cTransmitRxAckCmd =
    {
        .commandNo                  = CMD_IEEE_RX_ACK,
        .status                     = IDLE,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .endTrigger                 =
        {
            .triggerType            = TRIG_REL_START,
            .pastTrig               = 1,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
        .pNextOp                    = NULL,
        /* number of RAT ticks to wait before claiming we haven't received an ack */
        .endTime                    = ((IEEE802154_MAC_ACK_WAIT_DURATION * CC2650_RAT_TICKS_PER_SEC) / IEEE802154_SYMBOLS_PER_SEC),
    };
    // clang-format on

    /* reset retry count */
    sTransmitRetryCount = 0;

    sCsmacaBackoffCmd = cCsmacaBackoffCmd;
    /* initialize the random state with a true random seed for the radio core's
     * psudo rng */
    sCsmacaBackoffCmd.randomState = otPlatRandomGet();
    sCsmacaBackoffCmd.pNextOp     = (rfc_radioOp_t *)&sTransmitCmd;

    sTransmitCmd = cTransmitCmd;
    /* no need to look for an ack if the tx operation was stopped */
    sTransmitCmd.payloadLen = aLen;
    sTransmitCmd.pPayload   = aPsdu;

    if (aPsdu[0] & IEEE802154_ACK_REQUEST)
    {
        /* setup the receive ack command to follow the tx command */
        sTransmitCmd.condition.rule = COND_STOP_ON_FALSE;
        sTransmitCmd.pNextOp        = (rfc_radioOp_t *)&sTransmitRxAckCmd;

        sTransmitRxAckCmd       = cTransmitRxAckCmd;
        sTransmitRxAckCmd.seqNo = aPsdu[IEEE802154_DSN_OFFSET];
    }

    return (RFCDoorbellSendTo((uint32_t)&sCsmacaBackoffCmd) & 0xFF);
}

/**
 * Sends the rx command to the radio core.
 *
 * Sends the pre-built receive command to the radio core. This sets up the
 * radio to receive packets according to the settings in the global rx command.
 *
 * @note This function does not alter any of the parameters of the rx command.
 * It is only concerned with sending the command to the radio core. See @ref
 * otPlatRadioSetPanId for an example of how the rx settings are set changed.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command completed correctly.
 */
static uint_fast8_t rfCoreSendReceiveCmd(void)
{
    sReceiveCmd.status = IDLE;
    return (RFCDoorbellSendTo((uint32_t)&sReceiveCmd) & 0xFF);
}

static uint_fast8_t rfCoreSendEdScanCmd(uint8_t aChannel, uint16_t aDurration)
{
    // clang-format off
    static const rfc_CMD_IEEE_ED_SCAN_t cEdScanCmd =
    {
        .commandNo                  = CMD_IEEE_ED_SCAN,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
        .ccaOpt                     =
        {
            .ccaEnEnergy            = 1,
            .ccaEnCorr              = 1,
            .ccaEnSync              = 1,
            .ccaCorrOp              = 1,
            .ccaSyncOp              = 0,
            .ccaCorrThr             = 3,
        },
        .ccaRssiThr                 = -90,
        .endTrigger                 =
        {
            .triggerType            = TRIG_REL_START,
            .pastTrig               = 1,
        },
    };
    // clang-format on
    sEdScanCmd = cEdScanCmd;

    sEdScanCmd.channel = aChannel;

    /* durration is in ms */
    sEdScanCmd.endTime = aDurration * (CC2650_RAT_TICKS_PER_SEC / 1000);

    return (RFCDoorbellSendTo((uint32_t)&sEdScanCmd) & 0xFF);
}

/**
 * Enables the cpe0 and cpe1 radio interrupts.
 *
 * Enables the @ref IRQ_LAST_COMMAND_DONE and @ref IRQ_LAST_FG_COMMAND_DONE to
 * be handled by the @ref RFCCPE0IntHandler interrupt handler.
 */
static void rfCoreSetupInt(void)
{
    bool interruptsWereDisabled;

    otEXPECT(PRCMRfReady());

    interruptsWereDisabled = IntMasterDisable();

    /* Set all interrupt channels to CPE0 channel, error to CPE1 */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = IRQ_INTERNAL_ERROR;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = IRQ_LAST_COMMAND_DONE | IRQ_LAST_FG_COMMAND_DONE;

    IntRegister(INT_RFC_CPE_0, RFCCPE0IntHandler);
    IntRegister(INT_RFC_CPE_1, RFCCPE1IntHandler);
    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntEnable(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_1);

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

exit:
    return;
}

/**
 * Disables and clears the cpe0 and cpe1 radio interrupts.
 */
static void rfCoreStopInt(void)
{
    bool interruptsWereDisabled;

    interruptsWereDisabled = IntMasterDisable();

    /* clear and disable interrupts */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;

    IntUnregister(INT_RFC_CPE_0);
    IntUnregister(INT_RFC_CPE_1);
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
 * Sets the mode for the radio core to IEEE 802.15.4
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
 * Turns on the radio core.
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
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command was received.
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
        ;

    PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();

    while (!PRCMLoadGet())
        ;

    rfCoreSetupInt();

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) =
        (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M | RFC_PWR_PWMCLKEN_CPERAM_M);

    /* Send ping (to verify RFCore is ready and alive) */
    return rfCoreExecutePingCmd();
}

/**
 * Turns off the radio core.
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
        ;

    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);

    while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_OFF)
        ;

    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_RCOSC_HF)
    {
        /* Request to switch to the RC osc for low power mode. */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_RCOSC_HF);
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }
}

/**
 * Sends the setup command string to the radio core.
 *
 * Enables the clock line from the RTC to the RF core RAT. Enables the RAT
 * timer and sets up the radio in IEEE mode.
 *
 * @return The value from the command status register.
 * @retval CMDSTA_Done The command was received.
 */
static uint_fast16_t rfCoreSendEnableCmd(void)
{
    uint8_t       doorbellRet;
    bool          interruptsWereDisabled;
    uint_fast16_t ret;

    // clang-format off
    static const rfc_CMD_SYNC_START_RAT_t cStartRatCmd =
    {
        .commandNo                  = CMD_SYNC_START_RAT,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_STOP_ON_FALSE,
        },
    };
    static const rfc_CMD_RADIO_SETUP_t cRadioSetupCmd =
    {
        .commandNo                  = CMD_RADIO_SETUP,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
        .mode                       = 1, // IEEE 802.15.4 mode
    };
    // clang-format on
    /* turn on the clock line to the radio core */
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the rat start command */
    sStartRatCmd         = cStartRatCmd;
    sStartRatCmd.pNextOp = (rfc_radioOp_t *)&sRadioSetupCmd;
    sStartRatCmd.rat0    = sRatOffset;

    /* initialize radio setup command */
    sRadioSetupCmd = cRadioSetupCmd;
    /* initally set the radio tx power to the max */
    sRadioSetupCmd.txPower      = sCurrentOutputPower->value;
    sRadioSetupCmd.pRegOverride = sIEEEOverrides;

    interruptsWereDisabled = IntMasterDisable();

    doorbellRet = (RFCDoorbellSendTo((uint32_t)&sStartRatCmd) & 0xFF);
    otEXPECT_ACTION(CMDSTA_Done == doorbellRet, ret = doorbellRet);

    /* synchronously wait for the CM0 to stop executing */
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00)
        ;

    ret = sRadioSetupCmd.status;

exit:

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    return ret;
}

/**
 * Sends the shutdown command string to the radio core.
 *
 * Powers down the frequency synthesizer and stops the RAT.
 *
 * @note synchronously waits until the command string completes.
 *
 * @return The status of the RAT stop command.
 * @retval DONE_OK The command string executed properly.
 */
static uint_fast16_t rfCoreSendDisableCmd(void)
{
    uint8_t       doorbellRet;
    bool          interruptsWereDisabled;
    uint_fast16_t ret;

    // clang-format off
    static const rfc_CMD_FS_POWERDOWN_t cFsPowerdownCmd =
    {
        .commandNo                  = CMD_FS_POWERDOWN,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_ALWAYS,
        },
    };
    static const rfc_CMD_SYNC_STOP_RAT_t cStopRatCmd =
    {
        .commandNo                  = CMD_SYNC_STOP_RAT,
        .startTrigger               =
        {
            .triggerType            = TRIG_NOW,
        },
        .condition                  = {
            .rule                   = COND_NEVER,
        },
    };
    // clang-format on

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the command to power down the frequency synth */
    sFsPowerdownCmd         = cFsPowerdownCmd;
    sFsPowerdownCmd.pNextOp = (rfc_radioOp_t *)&sStopRatCmd;

    sStopRatCmd = cStopRatCmd;

    interruptsWereDisabled = IntMasterDisable();

    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

    doorbellRet = (RFCDoorbellSendTo((uint32_t)&sFsPowerdownCmd) & 0xFF);
    otEXPECT_ACTION(CMDSTA_Done == doorbellRet, ret = doorbellRet);

    /* synchronously wait for the CM0 to stop */
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00)
        ;

    ret = sStopRatCmd.status;

    if (sStopRatCmd.status == DONE_OK)
    {
        sRatOffset = sStopRatCmd.rat0;
    }

exit:

    if (!interruptsWereDisabled)
    {
        IntMasterEnable();
    }

    return ret;
}

/**
 * Error interrupt handler.
 */
void RFCCPE1IntHandler(void)
{
    /* Clear INTERNAL_ERROR interrupt flag */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x7FFFFFFF;
}

/**
 * Command done handler.
 */
void RFCCPE0IntHandler(void)
{
    if (HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE)
    {
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

        if (sState == cc2650_stateReceive && sReceiveCmd.status != ACTIVE && sReceiveCmd.status != IEEE_SUSPENDED)
        {
            /* the rx command was probably aborted to change the channel */
            sState = cc2650_stateSleep;
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
            if (sTransmitCmd.pPayload[0] & IEEE802154_ACK_REQUEST)
            {
                /* we are looking for an ack */
                switch (sTransmitRxAckCmd.status)
                {
                case IEEE_DONE_TIMEOUT:
                    if (sTransmitRetryCount < IEEE802154_MAC_MAX_FRAMES_RETRIES)
                    {
                        /* re-submit the tx command chain */
                        sTransmitRetryCount++;
                        RFCDoorbellSendTo((uint32_t)&sCsmacaBackoffCmd);
                    }
                    else
                    {
                        sTransmitError = OT_ERROR_NO_ACK;
                        /* signal polling function we are done transmitting, we failed to send the packet */
                        sTxCmdChainDone = true;
                    }

                    break;

                case IEEE_DONE_ACK:
                    sTransmitError = OT_ERROR_NONE;
                    /* signal polling function we are done transmitting */
                    sTxCmdChainDone = true;
                    break;

                case IEEE_DONE_ACKPEND:
                    sTransmitError = OT_ERROR_NONE;
                    /* signal polling function we are done transmitting */
                    sTxCmdChainDone = true;
                    break;

                default:
                    sTransmitError = OT_ERROR_FAILED;
                    /* signal polling function we are done transmitting */
                    sTxCmdChainDone = true;
                    break;
                }
            }
            else
            {
                /* The TX command was either stopped or we are not looking for
                 * an ack */
                switch (sTransmitCmd.status)
                {
                case IEEE_DONE_OK:
                    sTransmitError = OT_ERROR_NONE;
                    break;

                case IEEE_DONE_TIMEOUT:
                    sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
                    break;

                case IEEE_ERROR_NO_SETUP:
                case IEEE_ERROR_NO_FS:
                case IEEE_ERROR_SYNTH_PROG:
                    sTransmitError = OT_ERROR_INVALID_STATE;
                    break;

                case IEEE_ERROR_TXUNF:
                    sTransmitError = OT_ERROR_NO_BUFS;
                    break;

                default:
                    sTransmitError = OT_ERROR_FAILED;
                    break;
                }

                /* signal polling function we are done transmitting */
                sTxCmdChainDone = true;
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

    sState = cc2650_stateDisabled;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioEnable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateSleep)
    {
        error = OT_ERROR_NONE;
    }
    else if (sState == cc2650_stateDisabled)
    {
        otEXPECT_ACTION(rfCorePowerOn() == CMDSTA_Done, error = OT_ERROR_FAILED);
        otEXPECT_ACTION(rfCoreSendEnableCmd() == DONE_OK, error = OT_ERROR_FAILED);
        sState = cc2650_stateSleep;
    }

exit:

    if (error == OT_ERROR_FAILED)
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
    OT_UNUSED_VARIABLE(aInstance);
    return (sState != cc2650_stateDisabled);
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateDisabled)
    {
        error = OT_ERROR_NONE;
    }
    else if (sState == cc2650_stateSleep)
    {
        rfCoreSendDisableCmd();
        /* we don't want to fail if this command string doesn't work, just turn
         * off the whole core
         */
        rfCorePowerOff();
        sState = cc2650_stateDisabled;
        error  = OT_ERROR_NONE;
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateSleep)
    {
        sState = cc2650_stateEdScan;
        otEXPECT_ACTION(rfCoreSendEdScanCmd(aScanChannel, aScanDuration) == CMDSTA_Done, error = OT_ERROR_FAILED);
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sCurrentOutputPower->dbm;

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    unsigned int           i;
    output_config_t const *powerCfg = &(rgOutputPower[0]);

    for (i = 1; i < OUTPUT_CONFIG_COUNT; i++)
    {
        if (rgOutputPower[i].dbm >= aPower)
        {
            powerCfg = &(rgOutputPower[i]);
        }
        else
        {
            break;
        }
    }

    sCurrentOutputPower = powerCfg;

    return OT_ERROR_NONE;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateSleep)
    {
        sState = cc2650_stateReceive;

        /* initialize the receive command
         * XXX: no memset here because we assume init has been called and we
         *      may have changed some values in the rx command
         */
        sReceiveCmd.channel = aChannel;
        otEXPECT_ACTION(rfCoreSendReceiveCmd() == CMDSTA_Done, error = OT_ERROR_FAILED);
        error = OT_ERROR_NONE;
    }
    else if (sState == cc2650_stateReceive)
    {
        if (sReceiveCmd.status == ACTIVE && sReceiveCmd.channel == aChannel)
        {
            /* we are already running on the right channel */
            sState = cc2650_stateReceive;
            error  = OT_ERROR_NONE;
        }
        else
        {
            /* we have either not fallen back into our receive command or
             * we are running on the wrong channel. Either way assume the
             * caller correctly called us and abort all running commands.
             */
            otEXPECT_ACTION(rfCoreExecuteAbortCmd() == CMDSTA_Done, error = OT_ERROR_FAILED);

            /* any frames in the queue will be for the old channel */
            otEXPECT_ACTION(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done, error = OT_ERROR_FAILED);

            sReceiveCmd.channel = aChannel;
            otEXPECT_ACTION(rfCoreSendReceiveCmd() == CMDSTA_Done, error = OT_ERROR_FAILED);

            sState = cc2650_stateReceive;
            error  = OT_ERROR_NONE;
        }
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateSleep)
    {
        error = OT_ERROR_NONE;
    }
    else if (sState == cc2650_stateReceive)
    {
        if (rfCoreExecuteAbortCmd() != CMDSTA_Done)
        {
            error = OT_ERROR_BUSY;
        }
        else
        {
            sState = cc2650_stateSleep;
        }
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sTransmitFrame;
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_BUSY;

    if (sState == cc2650_stateReceive)
    {
        sState = cc2650_stateTransmit;

        /* removing 2 bytes of CRC placeholder because we generate that in hardware */
        otEXPECT_ACTION(rfCoreSendTransmitCmd(aFrame->mPsdu, aFrame->mLength - 2) == CMDSTA_Done,
                        error = OT_ERROR_FAILED);
        error           = OT_ERROR_NONE;
        sTransmitError  = OT_ERROR_NONE;
        sTxCmdChainDone = false;
        otPlatRadioTxStarted(aInstance, aFrame);
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRfStats.maxRssi;
}

/**
 * Function documented in platform/radio.h
 */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_TRANSMIT_RETRIES;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        rfCoreModifyRxAutoPend(aEnable);
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        sReceiveCmd.frameFiltOpt.autoPendEn = aEnable ? 1 : 0;
    }
}

/**
 * Function documented in platform/radio.h
 */
otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    uint8_t idx   = rfCoreFindShortSrcMatchIdx(aShortAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        otEXPECT_ACTION((idx = rfCoreFindEmptyShortSrcMatchIdx()) != CC2650_SRC_MATCH_NONE, error = OT_ERROR_NO_BUFS);
        sSrcMatchShortData.extAddrEnt[idx].shortAddr = aShortAddress;
        sSrcMatchShortData.extAddrEnt[idx].panId     = sReceiveCmd.localPanID;
    }

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        otEXPECT_ACTION(rfCoreModifySourceMatchEntry(idx, SHORT_ADDRESS, true) == CMDSTA_Done, error = OT_ERROR_FAILED);
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
otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    uint8_t idx;

    otEXPECT_ACTION((idx = rfCoreFindShortSrcMatchIdx(aShortAddress)) != CC2650_SRC_MATCH_NONE,
                    error = OT_ERROR_NO_ADDRESS);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        otEXPECT_ACTION(rfCoreModifySourceMatchEntry(idx, SHORT_ADDRESS, false) == CMDSTA_Done,
                        error = OT_ERROR_FAILED);
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
otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    uint8_t idx   = rfCoreFindExtSrcMatchIdx((uint64_t *)aExtAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        otEXPECT_ACTION((idx = rfCoreFindEmptyExtSrcMatchIdx()) != CC2650_SRC_MATCH_NONE, error = OT_ERROR_NO_BUFS);
        sSrcMatchExtData.extAddrEnt[idx] = *((uint64_t *)aExtAddress);
    }

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        otEXPECT_ACTION(rfCoreModifySourceMatchEntry(idx, EXT_ADDRESS, true) == CMDSTA_Done, error = OT_ERROR_FAILED);
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
otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    uint8_t idx;

    otEXPECT_ACTION((idx = rfCoreFindExtSrcMatchIdx((uint64_t *)aExtAddress)) != CC2650_SRC_MATCH_NONE,
                    error = OT_ERROR_NO_ADDRESS);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        otEXPECT_ACTION(rfCoreModifySourceMatchEntry(idx, EXT_ADDRESS, false) == CMDSTA_Done, error = OT_ERROR_FAILED);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        sSrcMatchExtData.srcPendEn[idx / 32] &= ~(1 << (idx % 32));
        sSrcMatchExtData.srcMatchEn[idx / 32] &= ~(1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        unsigned int i;

        for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
        {
            /* we have a running or backgrounded rx command */
            otEXPECT(rfCoreModifySourceMatchEntry(i, SHORT_ADDRESS, false) == CMDSTA_Done);
        }
    }
    else
    {
        /* we are not running, so we can erase them ourselves */
        memset((void *)&sSrcMatchShortData, 0, sizeof(sSrcMatchShortData));
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        unsigned int i;

        for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
        {
            /* we have a running or backgrounded rx command */
            otEXPECT(rfCoreModifySourceMatchEntry(i, EXT_ADDRESS, false) == CMDSTA_Done);
        }
    }
    else
    {
        /* we are not running, so we can erase them ourselves */
        memset((void *)&sSrcMatchExtData, 0, sizeof(sSrcMatchExtData));
    }

exit:
    return;
}

/**
 * Function documented in platform/radio.h
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    /* we are promiscuous if we are not filtering */
    return sReceiveCmd.frameFiltOpt.frameFiltEn == 0;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sReceiveCmd.status == ACTIVE || sReceiveCmd.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        /* if we are promiscuous, then frame filtering should be disabled */
        rfCoreModifyRxFrameFilter(!aEnable);
        /* XXX should we dump any queued messages ? */
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        sReceiveCmd.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
    }
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    uint8_t *    eui64;
    unsigned int i;

    OT_UNUSED_VARIABLE(aInstance);

    /* The IEEE MAC address can be stored two places. We check the Customer
     * Configuration was not set before defaulting to the Factory
     * Configuration.
     */
    eui64 = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);

    for (i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        if (eui64[i] != CC2650_UNKNOWN_EUI64)
        {
            break;
        }
    }

    if (i >= OT_EXT_ADDRESS_SIZE)
    {
        /* The ccfg address was all 0xFF, switch to the fcfg */
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
 * valid way to return that error since the function prototype was changed.
 */
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* XXX: if the pan id is the broadcast pan id (0xFFFF) the auto ack will
     * not work. This is due to the design of the CM0 and follows IEEE 802.15.4
     */
    if (sState == cc2650_stateReceive)
    {
        otEXPECT(rfCoreExecuteAbortCmd() == CMDSTA_Done);
        sReceiveCmd.localPanID = aPanid;
        otEXPECT(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done);
        otEXPECT(rfCoreSendReceiveCmd() == CMDSTA_Done);
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
 * valid way to return that error since the function prototype was changed.
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* XXX: assuming little endian format */
    if (sState == cc2650_stateReceive)
    {
        otEXPECT(rfCoreExecuteAbortCmd() == CMDSTA_Done);
        sReceiveCmd.localExtAddr = *((uint64_t *)(aAddress));
        otEXPECT(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done);
        otEXPECT(rfCoreSendReceiveCmd() == CMDSTA_Done);
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
 * valid way to return that error since the function prototype was changed.
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sState == cc2650_stateReceive)
    {
        otEXPECT(rfCoreExecuteAbortCmd() == CMDSTA_Done);
        sReceiveCmd.localShortAddr = aAddress;
        otEXPECT(rfCoreClearReceiveQueue(&sRxDataQueue) == CMDSTA_Done);
        otEXPECT(rfCoreSendReceiveCmd() == CMDSTA_Done);
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

static void cc2650RadioProcessTransmitDone(otInstance *  aInstance,
                                           otRadioFrame *aTransmitFrame,
                                           otRadioFrame *aAckFrame,
                                           otError       aTransmitError)
{
#if OPENTHREAD_ENABLE_DIAG

    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioTransmitDone(aInstance, aTransmitFrame, aTransmitError);
    }
    else
#endif /* OPENTHREAD_ENABLE_DIAG */
    {
        otPlatRadioTxDone(aInstance, aTransmitFrame, aAckFrame, aTransmitError);
    }
}

static void cc2650RadioProcessReceiveDone(otInstance *aInstance, otRadioFrame *aReceiveFrame, otError aReceiveError)
{
#if OPENTHREAD_ENABLE_DIAG

    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioReceiveDone(aInstance, aReceiveFrame, aReceiveError);
    }
    else
#endif /* OPENTHREAD_ENABLE_DIAG */
    {
        otPlatRadioReceiveDone(aInstance, aReceiveFrame, aReceiveError);
    }
}

static void cc2650RadioProcessReceiveQueue(otInstance *aInstance)
{
    rfc_ieeeRxCorrCrc_t *   crcCorr;
    rfc_dataEntryGeneral_t *curEntry, *startEntry;
    uint8_t                 rssi;

    startEntry = (rfc_dataEntryGeneral_t *)sRxDataQueue.pCurrEntry;
    curEntry   = startEntry;

    /* loop through receive queue */
    do
    {
        uint8_t *payload = &(curEntry->data);

        if (curEntry->status == DATA_ENTRY_FINISHED)
        {
            uint8_t      len;
            otError      receiveError;
            otRadioFrame receiveFrame;

            /* get the information appended to the end of the frame.
             * This array access looks like it is a fencepost error, but the
             * first byte is the number of bytes that follow.
             */
            len     = payload[0];
            crcCorr = (rfc_ieeeRxCorrCrc_t *)&payload[len];
            rssi    = payload[len - 1];

            if (crcCorr->status.bCrcErr == 0 && (len - 2) < OT_RADIO_FRAME_MAX_SIZE)
            {
                if (otPlatRadioGetPromiscuous(aInstance))
                {
                    // TODO: Propagate CM0 timestamp
                    receiveFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
                    receiveFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
                }

                receiveFrame.mLength             = len;
                receiveFrame.mPsdu               = &(payload[1]);
                receiveFrame.mChannel            = sReceiveCmd.channel;
                receiveFrame.mInfo.mRxInfo.mRssi = rssi;
                receiveFrame.mInfo.mRxInfo.mLqi  = crcCorr->status.corr;

                receiveError = OT_ERROR_NONE;
            }
            else
            {
                receiveError = OT_ERROR_FCS;
            }

            if ((receiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK)
            {
                if (receiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET])
                {
                    sState = cc2650_stateReceive;
                    cc2650RadioProcessTransmitDone(aInstance, &sTransmitFrame, &receiveFrame, receiveError);
                }
            }
            else
            {
                cc2650RadioProcessReceiveDone(aInstance, &receiveFrame, receiveError);
            }

            curEntry->status = DATA_ENTRY_PENDING;
            break;
        }
        else if (curEntry->status == DATA_ENTRY_UNFINISHED)
        {
            curEntry->status = DATA_ENTRY_PENDING;
        }

        curEntry = (rfc_dataEntryGeneral_t *)(curEntry->pNextEntry);
    } while (curEntry != startEntry);
}

/**
 * Function documented in platform-cc2650.h
 */
void cc2650RadioProcess(otInstance *aInstance)
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

    if (sState == cc2650_stateReceive || sState == cc2650_stateTransmit)
    {
        cc2650RadioProcessReceiveQueue(aInstance);
    }

    if (sTxCmdChainDone)
    {
        if (sState == cc2650_stateTransmit)
        {
            /* we are not looking for an ACK packet, or failed */
            sState = cc2650_stateReceive;
            cc2650RadioProcessTransmitDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
        }

        sTransmitError  = OT_ERROR_NONE;
        sTxCmdChainDone = false;
    }
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return CC2650_RECEIVE_SENSITIVITY;
}
