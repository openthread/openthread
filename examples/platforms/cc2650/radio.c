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

#define IEEE802154_FRAME_TYPE_MASK          0x7     /* (IEEE 802.15.4-2006) PSDU.FCF.frameType */
#define IEEE802154_FRAME_TYPE_ACK           0x2     /* (IEEE 802.15.4-2006) frame type: ACK */
#define IEEE802154_FRAME_PENDING            (1<<4)  /* (IEEE 802.15.4-2006) PSDU.FCF.bFramePending */
#define IEEE802154_ACK_REQUEST              (1<<5)  /* (IEEE 802.15.4-2006) PSDU.FCF.bAR */
#define IEEE802154_DSN_OFFSET               2       /* (IEEE 802.15.4-2006) PSDU.sequenceNumber */

#define IEEE802154_MAC_MIN_BE               1       /* (IEEE 802.15.4-2006) macMinBE */
#define IEEE802154_MAC_MAX_BE               5       /* (IEEE 802.15.4-2006) macMaxBE */
#define IEEE802154_MAC_MAX_CSMA_BACKOFFS    4       /* (IEEE 802.15.4-2006) macMaxCSMABackoffs */
#define IEEE802154_MAC_MAX_FRAMES_RETRIES   3       /* (IEEE 802.15.4-2006) macMaxFrameRetries */

#define IEEE802154_A_UINT_BACKOFF_PERIOD    20      /* (IEEE 802.15.4-2006 7.4.1) MAC constants */
#define IEEE802154_A_TURNAROUND_TIME        12      /* (IEEE 802.15.4-2006 6.4.1) PHY constants */
#define IEEE802154_PHY_SHR_DURATION         10      /* (IEEE 802.15.4-2006 6.4.2) PHY PIB attribute
                                                     * Specifically the O-QPSK PHY */
#define IEEE802154_PHY_SYMBOLS_PER_OCTET    2       /* (IEEE 802.15.4-2006 6.4.2) PHY PIB attribute
                                                     * Specifically the O-QPSK PHY */
#define IEEE802154_MAC_ACK_WAIT_DURATION          \
    (IEEE802154_A_UINT_BACKOFF_PERIOD +           \
     IEEE802154_A_TURNAROUND_TIME +               \
     IEEE802154_PHY_SHR_DURATION +                \
     ( 6 * IEEE802154_PHY_SYMBOLS_PER_OCTET))       /* (IEEE 802.15.4-2006 7.4.2)
                                                     * macAckWaitDuration PIB attribute */
#define IEEE802154_SYMBOLS_PER_SEC          62500   /* (IEEE 802.15.4-2006 6.5.3.2)
                                                     * O-QPSK symbol rate */
#define CC2650_RAT_TICKS_PER_SEC            4000000 /* 4MHz clock */
#define CC2650_INVALID_RSSI                 127
#define CC2650_UNKNOWN_EUI64                      \
                                 0xFFFFFFFFFFFFFFFF /* If the EUI64 read from the ccfg is all ones,
                                                     * then the customer did not set the address */

/* phy state as defined by openthread */
static volatile PhyState sState;
/* phy states not defined by openthread */
static volatile bool cc2650_state_rfc_enabling = false;
static volatile bool cc2650_state_rfc_transmitting = false;
static volatile bool cc2650_state_rfc_edscan = false;

/* TX Power dBm lookup table - values from SmartRF Studio */
typedef struct output_config
{
    int dbm;
    uint16_t value;
} output_config_t;

static const output_config_t output_power[] =
{
    { 5, 0x9330},
    { 4, 0x9324},
    { 3, 0x5a1c},
    { 2, 0x4e18},
    { 1, 0x4214},
    { 0, 0x3161},
    { -3, 0x2558},
    { -6, 0x1d52},
    { -9, 0x194e},
    { -12, 0x144b},
    { -15, 0x0ccb},
    { -18, 0x0cc9},
    { -21, 0x0cc7},
};

#define OUTPUT_CONFIG_COUNT (sizeof(output_power) / sizeof(output_config_t))

/* Max and Min Output Power in dBm */
#define OUTPUT_POWER_MIN     (output_power[OUTPUT_CONFIG_COUNT - 1].dbm)
#define OUTPUT_POWER_MAX     (output_power[0].dbm)
#define OUTPUT_POWER_UNKNOWN 0xFFFF

static output_config_t const *cur_output_power = &(output_power[OUTPUT_CONFIG_COUNT - 1]);

/* Overrides for IEEE 802.15.4, differential mode */
static uint32_t ieee_overrides[] =
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
 * cmd_ieee_rx_ack radio command
 *
 * used to pass data from the radio state ISR to the processing loop
 */
static volatile bool rx_ack_pending_bit = false;

/*
 * number of retry counts left to the currently transmitting frame
 *
 * initialized when a frame is passed to be sent over the air, and decremented
 * by the radio ISR every time the transmit command string fails to receive a
 * corresponding ack
 */
static volatile unsigned int tx_retry_count = 0;

/*
 * offset of the radio timer from the rtc
 *
 * used when we start and stop the RAT
 */
uint32_t rat_offset = 0;

/*
 * radio command structures that run on the CM0
 */
static volatile rfc_CMD_SYNC_START_RAT_t cmd_start_rat;
static volatile rfc_CMD_RADIO_SETUP_t cmd_radio_setup;

static volatile rfc_CMD_SYNC_STOP_RAT_t cmd_stop_rat;
static volatile rfc_CMD_FS_POWERDOWN_t cmd_fs_powerdown;

static volatile rfc_CMD_CLEAR_RX_t cmd_clear_rx;
static volatile rfc_CMD_IEEE_MOD_FILT_t cmd_ieee_mod_filt;
static volatile rfc_CMD_IEEE_MOD_SRC_MATCH_t cmd_ieee_mod_src_match;

static volatile rfc_CMD_IEEE_ED_SCAN_t cmd_ieee_ed_scan;

static volatile rfc_CMD_IEEE_RX_t cmd_ieee_rx;

static volatile rfc_CMD_IEEE_CSMA_t cmd_ieee_csma;
static volatile rfc_CMD_IEEE_TX_t cmd_ieee_tx;
static volatile rfc_CMD_IEEE_RX_ACK_t cmd_ieee_rx_ack;

#define CC2650_SRC_MATCH_NONE 0xFF

/*
 * number of extended addresses used for source matching
 */
#define CC2650_EXTADD_SRC_MATCH_NUM 10

/*
 * structure for source matching extended addresses
 */
static volatile struct
{
    uint32_t srcMatchEn[((CC2650_EXTADD_SRC_MATCH_NUM + 31) / 32)];
    uint32_t srcPendEn[((CC2650_EXTADD_SRC_MATCH_NUM + 31) / 32)];
    uint64_t extAddrEnt[CC2650_EXTADD_SRC_MATCH_NUM];
} src_match_ext_data __attribute__((aligned(4)));

/*
 * number of short addresses used for source matching
 */
#define CC2650_SHORTADD_SRC_MATCH_NUM 10

/*
 * structure for source matching short addresses
 */
static volatile struct
{
    uint32_t srcMatchEn[((CC2650_SHORTADD_SRC_MATCH_NUM + 31) / 32)];
    uint32_t srcPendEn[((CC2650_SHORTADD_SRC_MATCH_NUM + 31) / 32)];
    rfc_shortAddrEntry_t extAddrEnt[CC2650_SHORTADD_SRC_MATCH_NUM];
} src_match_short_data __attribute__((aligned(4)));

/* struct containing radio stats */
static rfc_ieeeRxOutput_t rf_stats;

/* size of length in recv struct */
#define DATA_ENTRY_LENSZ_BYTE 1

#define RX_BUF_SIZE 144
/* two receive buffers entries with room for 1 max IEEE802.15.4 frame in each */
static uint8_t rx_buf_0[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t rx_buf_1[RX_BUF_SIZE] __attribute__((aligned(4)));

/* The RX Data Queue */
static dataQueue_t rx_data_queue = { 0 };

/* openthread data primatives */
static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[kMaxPHYPacketSize] __attribute__((aligned(4))) ;
static uint8_t sReceivePsdu[kMaxPHYPacketSize] __attribute__((aligned(4))) ;

/**
 * @brief initialize the RX buffers
 *
 * Zeros out the receive buffers and sets up the data structures of the receive
 * queue.
 */
static void rf_core_init_buffers(void)
{
    rfc_dataEntry_t *entry;
    memset(rx_buf_0, 0x00, RX_BUF_SIZE);
    memset(rx_buf_1, 0x00, RX_BUF_SIZE);

    entry = (rfc_dataEntry_t *)rx_buf_0;
    entry->pNextEntry = rx_buf_1;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - sizeof(rfc_dataEntry_t);

    entry = (rfc_dataEntry_t *)rx_buf_1;
    entry->pNextEntry = rx_buf_0;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - sizeof(rfc_dataEntry_t);

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
static void rf_core_init_rx_params(void)
{
    memset((void *)&cmd_ieee_rx, 0x00, sizeof(cmd_ieee_rx));

    cmd_ieee_rx.commandNo = CMD_IEEE_RX;
    cmd_ieee_rx.status = IDLE;
    cmd_ieee_rx.pNextOp = NULL;
    cmd_ieee_rx.startTime = 0x00000000;
    cmd_ieee_rx.startTrigger.triggerType = TRIG_NOW;
    cmd_ieee_rx.condition.rule = COND_NEVER;
    cmd_ieee_rx.channel = kPhyMinChannel;

    cmd_ieee_rx.rxConfig.bAutoFlushCrc = 1;
    cmd_ieee_rx.rxConfig.bAutoFlushIgn = 0;
    cmd_ieee_rx.rxConfig.bIncludePhyHdr = 0;
    cmd_ieee_rx.rxConfig.bIncludeCrc = 0;
    cmd_ieee_rx.rxConfig.bAppendRssi = 1;
    cmd_ieee_rx.rxConfig.bAppendCorrCrc = 1;
    cmd_ieee_rx.rxConfig.bAppendSrcInd = 0;
    cmd_ieee_rx.rxConfig.bAppendTimestamp = 0;

    cmd_ieee_rx.pRxQ = &rx_data_queue;
    cmd_ieee_rx.pOutput = &rf_stats;

    cmd_ieee_rx.frameFiltOpt.frameFiltEn = 1;
    cmd_ieee_rx.frameFiltOpt.frameFiltStop = 1;
    cmd_ieee_rx.frameFiltOpt.autoAckEn = 1;
    cmd_ieee_rx.frameFiltOpt.slottedAckEn = 0;
    cmd_ieee_rx.frameFiltOpt.autoPendEn = 0;
    cmd_ieee_rx.frameFiltOpt.defaultPend = 0;
    cmd_ieee_rx.frameFiltOpt.bPendDataReqOnly = 0;
    cmd_ieee_rx.frameFiltOpt.bPanCoord = 0;
    cmd_ieee_rx.frameFiltOpt.maxFrameVersion = 3;
    cmd_ieee_rx.frameFiltOpt.bStrictLenFilter = 1;

    cmd_ieee_rx.numShortEntries = CC2650_SHORTADD_SRC_MATCH_NUM;
    cmd_ieee_rx.pShortEntryList = (uint32_t *)&src_match_short_data;

    cmd_ieee_rx.numExtEntries = CC2650_EXTADD_SRC_MATCH_NUM;
    cmd_ieee_rx.pExtEntryList = (uint32_t *)&src_match_ext_data;

    /* Receive all frame types */
    cmd_ieee_rx.frameTypes.bAcceptFt0Beacon = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt1Data = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt2Ack = 0;
    cmd_ieee_rx.frameTypes.bAcceptFt3MacCmd = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt4Reserved = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt5Reserved = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt6Reserved = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt7Reserved = 1;

    /* Configure CCA settings */
    cmd_ieee_rx.ccaOpt.ccaEnEnergy = 1;
    cmd_ieee_rx.ccaOpt.ccaEnCorr = 1;
    cmd_ieee_rx.ccaOpt.ccaEnSync = 1;
    cmd_ieee_rx.ccaOpt.ccaCorrOp = 1;
    cmd_ieee_rx.ccaOpt.ccaSyncOp = 0;
    cmd_ieee_rx.ccaOpt.ccaCorrThr = 3;

    cmd_ieee_rx.ccaRssiThr = -90;

    cmd_ieee_rx.endTrigger.triggerType = TRIG_NEVER;
    cmd_ieee_rx.endTime = 0x00000000;
}

static void rf_core_init_tx_params(void)
{
    uint16_t dummy;
    memset((void *)&cmd_ieee_csma, 0x00, sizeof(cmd_ieee_csma));

    /* initialize the random state with a true random seed for the radio core's
     * psudo rng */
    otPlatRandomSecureGet(sizeof(uint16_t) / sizeof(uint8_t), (uint8_t *) & (cmd_ieee_csma.randomState), &dummy);
    cmd_ieee_csma.commandNo = CMD_IEEE_CSMA;
}

/**
 * @brief sends the direct abort command to the radio core
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_abort(void)
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
static uint_fast8_t rf_core_cmd_ping(void)
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
 * @param [in] queue a pointer to the receive queue to be cleared
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_clear_rx(dataQueue_t *queue)
{
    memset((void *)&cmd_clear_rx, 0, sizeof(cmd_clear_rx));

    cmd_clear_rx.commandNo = CMD_CLEAR_RX;
    cmd_clear_rx.pQueue = queue;

    return (RFCDoorbellSendTo((uint32_t)&cmd_clear_rx) & 0xFF);
}

/**
 * @brief enable/diable filtering
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
 * @param [in] enable TRUE: enable frame filtering, FALSE: disable frame filtering
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_mod_filt(bool enable)
{
    memset((void *)&cmd_ieee_mod_filt, 0, sizeof(cmd_ieee_mod_filt));

    cmd_ieee_mod_filt.commandNo = CMD_IEEE_MOD_FILT;
    memcpy((void *)&cmd_ieee_mod_filt.newFrameFiltOpt, (void *)&cmd_ieee_rx.frameFiltOpt,
           sizeof(cmd_ieee_mod_filt.newFrameFiltOpt));
    memcpy((void *)&cmd_ieee_mod_filt.newFrameTypes, (void *)&cmd_ieee_rx.frameTypes,
           sizeof(cmd_ieee_mod_filt.newFrameTypes));
    cmd_ieee_mod_filt.newFrameFiltOpt.frameFiltEn = enable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_mod_filt) & 0xFF);
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
 * @param [in] enable TRUE: enable autoPend, FALSE: disable autoPend
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_mod_pend(bool enable)
{
    memset((void *)&cmd_ieee_mod_filt, 0, sizeof(cmd_ieee_mod_filt));

    cmd_ieee_mod_filt.commandNo = CMD_IEEE_MOD_FILT;
    memcpy((void *)&cmd_ieee_mod_filt.newFrameFiltOpt, (void *)&cmd_ieee_rx.frameFiltOpt,
           sizeof(cmd_ieee_mod_filt.newFrameFiltOpt));
    memcpy((void *)&cmd_ieee_mod_filt.newFrameTypes, (void *)&cmd_ieee_rx.frameTypes,
           sizeof(cmd_ieee_mod_filt.newFrameTypes));
    cmd_ieee_mod_filt.newFrameFiltOpt.autoPendEn = enable ? 1 : 0;

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_mod_filt) & 0xFF);
}

/**
 * address type for @ref rf_core_cmd_mod_src_match
 */
typedef enum
{
    SHORT_ADDRESS = 1,
    EXT_ADDRESS = 0,
} cc2650_address_type;

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
 * @param [in] entryNo the index of the entry to alter
 * @param [in] type TRUE: the entry is a short address, FALSE: the entry is an extended address
 * @param [in] enable whether the given entry is to be enabled or disabled
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_mod_src_match(uint8_t entryNo, cc2650_address_type type, bool enable)
{
    memset((void *)&cmd_ieee_mod_src_match, 0, sizeof(cmd_ieee_mod_src_match));

    cmd_ieee_mod_src_match.commandNo = CMD_IEEE_MOD_SRC_MATCH;
    /* we only use source matching for pending data bit, so enabling and
     * pending are the same to us.
     */
    cmd_ieee_mod_src_match.options.bEnable = enable ? 1 : 0;
    cmd_ieee_mod_src_match.options.srcPend = enable ? 1 : 0;
    cmd_ieee_mod_src_match.options.entryType = type;

    cmd_ieee_mod_src_match.entryNo = entryNo;

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_mod_src_match) & 0xFF);
}

/**
 * @brief sends the tx command to the radio core
 *
 * Sends the packet to the radio core to be sent asynchronously.
 *
 * @param [in] psdu a pointer to the data to be sent
 * @note this *must* be 4 byte aligned and not include the FCS, that is
 * calculated in hardware.
 * @param [in] len the length in bytes of data pointed to by psdu.
 *
 * @return the value from the command status register
 * @retval CMDSTA_Done the command completed correctly
 */
static uint_fast8_t rf_core_cmd_ieee_tx(uint8_t *psdu, uint8_t len)
{
    /* no memset for CSMA-CA to preserve the random state */
    memset((void *)&cmd_ieee_tx, 0, sizeof(cmd_ieee_tx));
    /* reset retry count */
    tx_retry_count = 0;

    cmd_ieee_csma.status = IDLE;
    cmd_ieee_csma.macMaxBE = IEEE802154_MAC_MAX_BE;
    cmd_ieee_csma.macMaxCSMABackoffs = IEEE802154_MAC_MAX_CSMA_BACKOFFS;
    cmd_ieee_csma.csmaConfig.initCW = 1;
    cmd_ieee_csma.csmaConfig.bSlotted = 0;
    cmd_ieee_csma.csmaConfig.rxOffMode = 0;
    cmd_ieee_csma.NB = 0;
    cmd_ieee_csma.BE = IEEE802154_MAC_MIN_BE;
    cmd_ieee_csma.remainingPeriods = 0;
    cmd_ieee_csma.endTrigger.triggerType = TRIG_NEVER;
    cmd_ieee_csma.endTime = 0x00000000;
    cmd_ieee_csma.pNextOp = (rfc_radioOp_t *) &cmd_ieee_tx;

    cmd_ieee_tx.commandNo = CMD_IEEE_TX;
    /* no need to look for an ack if the tx operation was stopped */
    cmd_ieee_tx.payloadLen = len;
    cmd_ieee_tx.pPayload = psdu;
    cmd_ieee_tx.startTrigger.triggerType = TRIG_NOW;

    /* always clear the rx_ack and retries commands just incase the status is
     * not IDLE for the interrupt handler */
    memset((void *)&cmd_ieee_rx_ack, 0, sizeof(cmd_ieee_rx_ack));

    if (psdu[0] & IEEE802154_ACK_REQUEST)
    {
        /* setup the receive ack command to follow the tx command */
        cmd_ieee_tx.condition.rule = COND_STOP_ON_FALSE;
        cmd_ieee_tx.pNextOp = (rfc_radioOp_t *) &cmd_ieee_rx_ack;

        cmd_ieee_rx_ack.commandNo = CMD_IEEE_RX_ACK;
        cmd_ieee_rx_ack.startTrigger.triggerType = TRIG_NOW;
        cmd_ieee_rx_ack.seqNo = psdu[IEEE802154_DSN_OFFSET];
        cmd_ieee_rx_ack.endTrigger.triggerType = TRIG_REL_START;
        cmd_ieee_rx_ack.endTrigger.pastTrig = 1;
        cmd_ieee_rx_ack.condition.rule = COND_NEVER;
        cmd_ieee_rx_ack.pNextOp = NULL;
        /* number of RAT ticks to wait before claiming we haven't received an ack */
        cmd_ieee_rx_ack.endTime = ((IEEE802154_MAC_ACK_WAIT_DURATION * CC2650_RAT_TICKS_PER_SEC) / IEEE802154_SYMBOLS_PER_SEC);
    }
    else
    {
        /* we are not looking for an ack, no retries because we won't know if we need to */
        cmd_ieee_tx.condition.rule = COND_NEVER;
        cmd_ieee_tx.pNextOp = NULL;
    }

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_csma) & 0xFF);
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
static uint_fast8_t rf_core_cmd_ieee_rx(void)
{
    cmd_ieee_rx.status = IDLE;
    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_rx) & 0xFF);
}

static uint_fast8_t rf_core_cmd_ieee_edscan(uint8_t channel, uint16_t durration)
{
    memset((void *)&cmd_ieee_ed_scan, 0, sizeof(cmd_ieee_ed_scan));

    cmd_ieee_ed_scan.commandNo = CMD_IEEE_ED_SCAN;
    cmd_ieee_ed_scan.startTrigger.triggerType = TRIG_NOW;
    cmd_ieee_ed_scan.condition.rule = COND_NEVER;
    cmd_ieee_ed_scan.channel = channel;

    cmd_ieee_ed_scan.ccaOpt.ccaEnEnergy = 1;
    cmd_ieee_ed_scan.ccaOpt.ccaEnCorr = 1;
    cmd_ieee_ed_scan.ccaOpt.ccaEnSync = 1;
    cmd_ieee_ed_scan.ccaOpt.ccaCorrOp = 1;
    cmd_ieee_ed_scan.ccaOpt.ccaSyncOp = 0;
    cmd_ieee_ed_scan.ccaOpt.ccaCorrThr = 3;

    cmd_ieee_ed_scan.ccaRssiThr = -90;

    cmd_ieee_ed_scan.endTrigger.triggerType = TRIG_REL_START;
    cmd_ieee_ed_scan.endTrigger.pastTrig = 1;
    /* durration is in ms */
    cmd_ieee_ed_scan.endTime = durration * (CC2650_RAT_TICKS_PER_SEC / 1000);

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_ed_scan) & 0xFF);
}

/**
 * @brief enables the cpe0 and cpe1 radio interrupts
 *
 * Enables the @ref IRQ_LAST_COMMAND_DONE and @ref IRQ_LAST_FG_COMMAND_DONE to
 * be handled by the @ref RFCCPE0IntHandler interrupt handler.
 */
static void rf_core_setup_interrupts(void)
{
    bool interrupts_disabled;

    /* We are already turned on by the caller, so this should not happen */
    if (!PRCMRfReady())
    {
        return;
    }

    interrupts_disabled = IntMasterDisable();

    /* Set all interrupt channels to CPE0 channel, error to CPE1 */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = IRQ_INTERNAL_ERROR;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = IRQ_LAST_COMMAND_DONE | IRQ_LAST_FG_COMMAND_DONE;

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntEnable(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_1);

    if (!interrupts_disabled)
    {
        IntMasterEnable();
    }
}

/**
 * @brief disables and clears the cpe0 and cpe1 radio interrupts
 */
static void rf_core_disable_interrupts(void)
{
    bool interrupts_disabled;

    interrupts_disabled = IntMasterDisable();

    /* clear and disable interrupts */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntDisable(INT_RFC_CPE_0);
    IntDisable(INT_RFC_CPE_1);

    if (!interrupts_disabled)
    {
        IntMasterEnable();
    }
}

/**
 * @brief Sets the mode for the radio core to IEEE 802.15.4
 */
static void rf_core_set_modesel(void)
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
static uint_fast8_t rf_core_power_on(void)
{
    bool interrupts_disabled;

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

    rf_core_set_modesel();

    /* Set of RF Core data queue. Circular buffer, no last entry */
    rx_data_queue.pCurrEntry = rx_buf_0;
    rx_data_queue.pLastEntry = NULL;

    rf_core_init_buffers();

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

    interrupts_disabled = IntMasterDisable();

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

    rf_core_setup_interrupts();

    if (!interrupts_disabled)
    {
        IntMasterEnable();
    }

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M |
                                                       RFC_PWR_PWMCLKEN_CPERAM_M);

    /* Send ping (to verify RFCore is ready and alive) */
    return rf_core_cmd_ping();
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
static void rf_core_power_off(void)
{
    rf_core_disable_interrupts();

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
static uint_fast8_t rf_core_cmds_enable(void)
{
    /* turn on the clock line to the radio core */
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the rat start command */
    memset((void *)&cmd_start_rat, 0x00, sizeof(cmd_start_rat));
    cmd_start_rat.commandNo = CMD_SYNC_START_RAT;
    cmd_start_rat.condition.rule = COND_STOP_ON_FALSE;
    cmd_start_rat.pNextOp = (rfc_radioOp_t *) &cmd_radio_setup;
    cmd_start_rat.rat0 = rat_offset;

    /* initialize radio setup command */
    memset((void *)&cmd_radio_setup, 0, sizeof(cmd_radio_setup));
    cmd_radio_setup.commandNo = CMD_RADIO_SETUP;
    cmd_radio_setup.condition.rule = COND_NEVER;
    /* initally set the radio tx power to the max */
    cmd_radio_setup.txPower = cur_output_power->value;
    cmd_radio_setup.pRegOverride = ieee_overrides;
    cmd_radio_setup.mode = 1;

    return (RFCDoorbellSendTo((uint32_t)&cmd_start_rat) & 0xFF);
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
static uint_fast16_t rf_core_cmds_disable(void)
{
    uint8_t doorbell_ret;
    bool interrupts_disabled;

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the command to power down the frequency synth */
    memset((void *)&cmd_fs_powerdown, 0, sizeof(cmd_fs_powerdown));
    cmd_fs_powerdown.commandNo = CMD_FS_POWERDOWN;
    cmd_fs_powerdown.condition.rule = COND_ALWAYS;
    cmd_fs_powerdown.pNextOp = (rfc_radioOp_t *)&cmd_stop_rat;

    memset((void *)&cmd_stop_rat, 0, sizeof(cmd_stop_rat));
    cmd_stop_rat.commandNo = CMD_SYNC_STOP_RAT;
    cmd_stop_rat.condition.rule = COND_NEVER;

    interrupts_disabled = IntMasterDisable();

    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&cmd_fs_powerdown) & 0xFF);

    if (doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }

    /* synchronously wait for the CM0 to stop */
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00)
    {
        ;
    }

    if (!interrupts_disabled)
    {
        IntMasterEnable();
    }

    if (cmd_stop_rat.status == DONE_OK)
    {
        rat_offset = cmd_stop_rat.rat0;
    }

    return cmd_stop_rat.status;
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

        if (sState == kStateSleep)
        {
            if (cmd_radio_setup.status == DONE_OK)
            {
                cc2650_state_rfc_enabling = false;
            }
            else
            {
                rf_core_power_off();
                sState = kStateDisabled;
            }
        }
        else if (sState == kStateReceive)
        {
            if (cc2650_state_rfc_edscan)
            {
                sState = kStateSleep;
            }
            else if (cmd_ieee_rx.status != ACTIVE && cmd_ieee_rx.status != IEEE_SUSPENDED)
            {
                /* the rx command was probably aborted to change the channel */
                sState = kStateSleep;
            }
        }
    }

    if (HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_FG_COMMAND_DONE)
    {
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_FG_COMMAND_DONE;

        if (sState == kStateTransmit)
        {
            if (cmd_ieee_rx_ack.status == IEEE_DONE_TIMEOUT)
            {
                if (tx_retry_count < IEEE802154_MAC_MAX_FRAMES_RETRIES)
                {
                    /* re-submit the tx command chain */
                    tx_retry_count++;
                    RFCDoorbellSendTo((uint32_t)&cmd_ieee_csma);
                }
                else
                {
                    /* signal polling function we are done transmitting, we failed to send the packet */
                    cc2650_state_rfc_transmitting = false;
                    sTransmitError = kThreadError_NoAck;
                }
            }
            else if (cmd_ieee_rx_ack.status != IDLE)
            {
                /* signal polling function we are done transmitting */
                cc2650_state_rfc_transmitting = false;

                /* we were looking for an ack, and the TX command didn't fail */
                switch (cmd_ieee_rx_ack.status)
                {
                case IEEE_DONE_ACK:
                    rx_ack_pending_bit = false;
                    sTransmitError = kThreadError_None;
                    break;

                case IEEE_DONE_ACKPEND:
                    rx_ack_pending_bit = true;
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
                cc2650_state_rfc_transmitting = false;

                /* The TX command was either stopped or we are not looking for
                 * an ack */
                switch (cmd_ieee_tx.status)
                {
                case IEEE_DONE_OK:
                    rx_ack_pending_bit = false;
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
    rf_core_init_rx_params();
    /* Populate the CSMA parameters */
    rf_core_init_tx_params();

    sState = kStateDisabled;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if (sState == kStateDisabled && !cc2650_state_rfc_enabling)
    {
        VerifyOrExit(rf_core_power_on() == CMDSTA_Done, error = kThreadError_Failed);
        cc2650_state_rfc_enabling = true;
        sState = kStateSleep;
        VerifyOrExit(rf_core_cmds_enable() == CMDSTA_Done, error = kThreadError_Failed);
    }

exit:

    if (error == kThreadError_Failed)
    {
        rf_core_power_off();
        sState = kStateDisabled;
    }

    return error;
}

/**
 * Function documented in platform/radio.h
 */
bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return (sState != kStateDisabled);
}

ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;

    if (sState == kStateDisabled)
    {
        error = kThreadError_None;
    }

    if (sState == kStateSleep)
    {
        rf_core_cmds_disable();
        /* we don't want to fail if this command string doesn't work, just turn
         * off the whole core
         */
        rf_core_power_off();
        sState = kStateDisabled;
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

    if (sState == kStateSleep && !cc2650_state_rfc_enabling)
    {
        sState = kStateReceive;
        cc2650_state_rfc_edscan = true;
        VerifyOrExit(rf_core_cmd_ieee_edscan(aScanChannel, aScanDuration) == CMDSTA_Done, error = kThreadError_Failed);
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

    if (sState == kStateSleep && !cc2650_state_rfc_enabling)
    {
        sState = kStateReceive;

        /* initialize the receive command
         * XXX: no memset here because we assume init has been called and we
         *      may have changed some values in the rx command
         */
        cmd_ieee_rx.channel = aChannel;
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error = kThreadError_Failed);
        error = kThreadError_None;
    }
    else if (sState == kStateReceive && !cc2650_state_rfc_edscan)
    {
        if (cmd_ieee_rx.status == ACTIVE && cmd_ieee_rx.channel == aChannel)
        {
            /* we are already running on the right channel */
            sState = kStateReceive;
            error = kThreadError_None;
        }
        else
        {
            /* we have either not fallen back into our receive command or
             * we are running on the wrong channel. Either way assume the
             * caller correctly called us and abort all running commands.
             */
            VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, error = kThreadError_Failed);

            /* any frames in the queue will be for the old channel */
            VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, error = kThreadError_Failed);

            cmd_ieee_rx.channel = aChannel;
            VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error = kThreadError_Failed);

            sState = kStateReceive;
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

    if (sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if (sState == kStateReceive)
    {
        if (rf_core_cmd_abort() != CMDSTA_Done)
        {
            error = kThreadError_Busy;
            return error;
        }

        sState = kStateSleep;
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

    if (sState == kStateReceive)
    {
        /*
         * This is the easiest way to setup the frequency synthesizer.
         * And we are supposed to fall into the receive state afterwards.
         */
        error = otPlatRadioReceive(aInstance, aPacket->mChannel);

        if (error == kThreadError_None)
        {
            sState = kStateTransmit;

            /* removing 2 bytes of CRC placeholder because we generate that in hardware */
            cc2650_state_rfc_transmitting = true;
            VerifyOrExit(rf_core_cmd_ieee_tx(aPacket->mPsdu, aPacket->mLength - 2) == CMDSTA_Done, error = kThreadError_Failed);
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
    return rf_stats.maxRssi;
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

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        SuccessOrExit(rf_core_cmd_mod_pend(aEnable) == CMDSTA_Done);
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        cmd_ieee_rx.frameFiltOpt.autoPendEn = aEnable ? 1 : 0;
    }

exit:
    return;
}

/**
 * @brief walks the short address source match list to find an address
 *
 * @param [in] address the short address to search for
 *
 * @return the index where the address was found
 * @retval CC2650_SRC_MATCH_NONE the address was not found
 */
static uint8_t rfcore_src_match_short_find_idx(const uint16_t address)
{
    uint8_t i;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if (src_match_short_data.extAddrEnt[i].shortAddr == address)
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
static uint8_t rfcore_src_match_short_find_empty(void)
{
    uint8_t i;

    for (i = 0; i < CC2650_SHORTADD_SRC_MATCH_NUM; i++)
    {
        if ((src_match_short_data.srcMatchEn[i / 32] & (1 << (i % 32))) == 0u)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx = rfcore_src_match_short_find_idx(aShortAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        VerifyOrExit((idx = rfcore_src_match_short_find_empty()) != CC2650_SRC_MATCH_NONE, error = kThreadError_NoBufs);
        src_match_short_data.extAddrEnt[idx].shortAddr = aShortAddress;
        src_match_short_data.extAddrEnt[idx].panId = cmd_ieee_rx.localPanID;
    }

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rf_core_cmd_mod_src_match(idx, SHORT_ADDRESS, true) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        src_match_short_data.srcPendEn[idx / 32] |= (1 << (idx % 32));
        src_match_short_data.srcMatchEn[idx / 32] |= (1 << (idx % 32));
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
    VerifyOrExit((idx = rfcore_src_match_short_find_idx(aShortAddress)) != CC2650_SRC_MATCH_NONE,
                 error = kThreadError_NoAddress);

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rf_core_cmd_mod_src_match(idx, SHORT_ADDRESS, false) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        src_match_short_data.srcPendEn[idx / 32] &= ~(1 << (idx % 32));
        src_match_short_data.srcMatchEn[idx / 32] &= ~(1 << (idx % 32));
    }

exit:
    return error;
}

/**
 * @brief walks the ext address source match list to find an address
 *
 * @param [in] address the ext address to search for
 *
 * @return the index where the address was found
 * @retval CC2650_SRC_MATCH_NONE the address was not found
 */
static uint8_t rfcore_src_match_ext_find_idx(const uint64_t *address)
{
    uint8_t i;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if (src_match_ext_data.extAddrEnt[i] == *address)
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
static uint8_t rfcore_src_match_ext_find_empty(void)
{
    uint8_t i;

    for (i = 0; i < CC2650_EXTADD_SRC_MATCH_NUM; i++)
    {
        if ((src_match_ext_data.srcMatchEn[i / 32] & (1 << (i % 32))) != 0u)
        {
            return i;
        }
    }

    return CC2650_SRC_MATCH_NONE;
}

/**
 * Function documented in platform/radio.h
 */
ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;
    uint8_t idx = rfcore_src_match_ext_find_idx((uint64_t *)aExtAddress);

    if (idx == CC2650_SRC_MATCH_NONE)
    {
        /* the entry does not exist already, add it */
        VerifyOrExit((idx = rfcore_src_match_ext_find_empty()) != CC2650_SRC_MATCH_NONE, error = kThreadError_NoBufs);
        src_match_ext_data.extAddrEnt[idx] = *((uint64_t *)aExtAddress);
    }

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rf_core_cmd_mod_src_match(idx, EXT_ADDRESS, true) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        src_match_ext_data.srcPendEn[idx / 32] |= (1 << (idx % 32));
        src_match_ext_data.srcMatchEn[idx / 32] |= (1 << (idx % 32));
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
    VerifyOrExit((idx = rfcore_src_match_ext_find_idx((uint64_t *)aExtAddress)) != CC2650_SRC_MATCH_NONE,
                 error = kThreadError_NoAddress);

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        VerifyOrExit(rf_core_cmd_mod_src_match(idx, EXT_ADDRESS, false) == CMDSTA_Done, error = kThreadError_Failed);
    }
    else
    {
        /* we are not running, so we must update the values ourselves */
        src_match_ext_data.srcPendEn[idx] = 0u;
        src_match_ext_data.srcMatchEn[idx] = 0u;
        src_match_ext_data.srcPendEn[idx / 32] &= ~(1 << (idx % 32));
        src_match_ext_data.srcMatchEn[idx / 32] &= ~(1 << (idx % 32));
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
    return cmd_ieee_rx.frameFiltOpt.frameFiltEn == 0;
}

/**
 * Function documented in platform/radio.h
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    if (cmd_ieee_rx.status == ACTIVE || cmd_ieee_rx.status == IEEE_SUSPENDED)
    {
        /* we have a running or backgrounded rx command */
        /* if we are promiscuous, then frame filtering should be disabled */
        SuccessOrExit(rf_core_cmd_mod_filt(!aEnable) == CMDSTA_Done);
        /* XXX should we dump any queued messages ? */
    }
    else
    {
        /* if we are promiscuous, then frame filtering should be disabled */
        cmd_ieee_rx.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
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
    (void)aInstance;

    /* The IEEE MAC address can be stored two places. We check the Customer
     * Configuration was not set before defaulting to the Factory
     * Configuration.
     */
    eui64 = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);

    if (*((uint64_t *)eui64) == CC2650_UNKNOWN_EUI64)
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
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    (void)aInstance;

    /* XXX: if the pan id is the broadcast pan id (0xFFFF) the auto ack will
     * not work. This is due to the design of the CM0 and follows IEEE 802.15.4
     */
    if (sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, ;);
        cmd_ieee_rx.localPanID = panid;
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, ;);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = kStateReceive;
    }
    else if (sState != kStateTransmit)
    {
        cmd_ieee_rx.localPanID = panid;
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
void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *address)
{
    (void)aInstance;

    /* XXX: assuming little endian format */
    if (sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, ;);
        cmd_ieee_rx.localExtAddr = *((uint64_t *)(address));
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, ;);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = kStateReceive;
    }
    else if (sState != kStateTransmit)
    {
        cmd_ieee_rx.localExtAddr = *((uint64_t *)(address));
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
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    (void)aInstance;

    if (sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, ;);
        cmd_ieee_rx.localShortAddr = address;
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, ;);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, ;);
        /* the interrupt from abort changed our state to sleep */
        sState = kStateReceive;
    }
    else if (sState != kStateTransmit)
    {
        cmd_ieee_rx.localShortAddr = address;
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
    rfc_dataEntryGeneral_t *startEntry = (rfc_dataEntryGeneral_t *)rx_data_queue.pCurrEntry;
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
                sReceiveFrame.mChannel = cmd_ieee_rx.channel;
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
    if (sState == kStateSleep && cc2650_state_rfc_edscan)
    {
        if (cmd_ieee_ed_scan.status == IEEE_DONE_OK)
        {
            otPlatRadioEnergyScanDone(aInstance, cmd_ieee_ed_scan.maxRssi);
        }
        else
        {
            otPlatRadioEnergyScanDone(aInstance, CC2650_INVALID_RSSI);
        }
    }

    if (sState == kStateReceive || sState == kStateTransmit)
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

    if (sState == kStateTransmit && (!cc2650_state_rfc_transmitting || sTransmitError != kThreadError_None))
    {
        /* we are not looking for an ACK packet, or failed */
        sState = kStateReceive;
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, rx_ack_pending_bit, sTransmitError);
        }
        else
#endif /* OPENTHREAD_ENABLE_DIAG */
        {
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, rx_ack_pending_bit, sTransmitError);
        }
    }

    return 0;
}
