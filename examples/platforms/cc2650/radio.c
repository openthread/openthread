/******************************************************************************
 *  Copyright (c) 2016, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1) Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2) Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3) Neither the name of the ORGANIZATION nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
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
 *
 ******************************************************************************/

#include <openthread-types.h>

#include <common/code_utils.hpp>
#include <platform/radio.h>

#include <prcm.h>
#include <hw_prcm.h>
#include <rfc.h>
#include <osc.h>
#include <rf_data_entry.h>
#include <rf_mailbox.h>
#include <rf_common_cmd.h>
#include <rf_ieee_mailbox.h>
#include <rf_ieee_cmd.h>
#include <hw_ints.h>
#include <chipinfo.h>

/* return values from the setup and teardown functions */
#define CC2650_CRC_BIT_MASK (1<<8)
#define CC2650_LQI_BIT_MASK 0x3f

#define RF_CORE_ERROR 1
#define RF_CORE_OK 0

#define IEEE802154_MIN_LENGTH 5
#define IEEE802154_MAX_LENGTH 127
#define IEEE802154_FRAME_TYPE_MASK 0x7
#define IEEE802154_FRAME_TYPE_ACK 0x2
#define IEEE802154_FRAME_PENDING (1 << 4)
#define IEEE802154_ACK_REQUEST (1 << 5)
#define IEEE802154_DSN_OFFSET 2

static PhyState sState;

/* TX Power dBm lookup table - values from SmartRF Studio */
typedef struct output_config {
    int dbm;
    uint16_t value;
} output_config_t;

static const output_config_t output_power[] = {
    {  5, 0x9330},
    {  4, 0x9324},
    {  3, 0x5a1c},
    {  2, 0x4e18},
    {  1, 0x4214},
    {  0, 0x3161},
    { -3, 0x2558},
    { -6, 0x1d52},
    { -9, 0x194e},
    {-12, 0x144b},
    {-15, 0x0ccb},
    {-18, 0x0cc9},
    {-21, 0x0cc7},
};

#define OUTPUT_CONFIG_COUNT (sizeof(output_power) / sizeof(output_config_t))

/* Max and Min Output Power in dBm */
#define OUTPUT_POWER_MIN     (output_power[OUTPUT_CONFIG_COUNT - 1].dbm)
#define OUTPUT_POWER_MAX     (output_power[0].dbm)
#define OUTPUT_POWER_UNKNOWN 0xFFFF

static output_config_t const *cur_output_power = &(output_power[OUTPUT_CONFIG_COUNT - 1]);

/* Overrides for IEEE 802.15.4, differential mode */
static uint32_t ieee_overrides[] = {
    0x00354038, /* Synth: Set RTRIM (POTAILRESTRIM) to 5 */
    0x4001402D, /* Synth: Correct CKVD latency setting (address) */
    0x00608402, /* Synth: Correct CKVD latency setting (value) */
    //  0x4001405D, /* Synth: Set ANADIV DIV_BIAS_MODE to PG1 (address) */
    //  0x1801F800, /* Synth: Set ANADIV DIV_BIAS_MODE to PG1 (value) */
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
 * offset of the radio timer from the rtc
 * 
 * used when we start and stop the RAT
 */
uint32_t rat_offset = 0;

/*
 * rx command that runs in the background on the CM0
 */
static volatile rfc_CMD_IEEE_RX_t cmd_ieee_rx;

/*
 * struct containing radio stats
 */
static rfc_ieeeRxOutput_t rf_stats;

/* size of length in recv struct */
#define DATA_ENTRY_LENSZ_NONE 0
#define DATA_ENTRY_LENSZ_BYTE 1
#define DATA_ENTRY_LENSZ_WORD 2 /* 2 bytes */

#define RX_BUF_SIZE 144
/* Four receive buffers entries with room for 1 IEEE802.15.4 frame in each */
static uint8_t rx_buf_0[RX_BUF_SIZE] __attribute__ ((aligned (4)));
static uint8_t rx_buf_1[RX_BUF_SIZE] __attribute__ ((aligned (4)));
// static uint8_t rx_buf_2[RX_BUF_SIZE] __attribute__ ((aligned (4)));
// static uint8_t rx_buf_3[RX_BUF_SIZE] __attribute__ ((aligned (4)));

/* The RX Data Queue */
static dataQueue_t rx_data_queue = { 0 };

/* openthread data primatives */
static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[kMaxPHYPacketSize] __attribute__ ((aligned (4))) ;
/* XXX: can we point the ot stack to one of the buffers above? */
static uint8_t sReceivePsdu[kMaxPHYPacketSize] __attribute__ ((aligned (4))) ;

static void init_buffers(void)
{
    rfc_dataEntry_t *entry;

    memset(rx_buf_0, 0, RX_BUF_SIZE);
    memset(rx_buf_1, 0, RX_BUF_SIZE);
    // memset(rx_buf_2, 0, RX_BUF_SIZE);
    // memset(rx_buf_3, 0, RX_BUF_SIZE);

    entry = (rfc_dataEntry_t *)rx_buf_0;
    entry->pNextEntry = rx_buf_1;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - 8;

    entry = (rfc_dataEntry_t *)rx_buf_1;
    entry->pNextEntry = rx_buf_0;
    // entry->pNextEntry = rx_buf_2;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - 8;

    /*
    entry = (rfc_dataEntry_t *)rx_buf_2;
    entry->pNextEntry = rx_buf_3;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - 8;

    entry = (rfc_dataEntry_t *)rx_buf_3;
    entry->pNextEntry = rx_buf_0;
    entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
    entry->length = sizeof(rx_buf_0) - 8;
    */

    sTransmitFrame.mPsdu = sTransmitPsdu;
    sTransmitFrame.mLength = 0;

    sReceiveFrame.mPsdu = sReceivePsdu;
    sReceiveFrame.mLength = 0;
}

static void init_rf_params(void)
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
    cmd_ieee_rx.frameFiltOpt.frameFiltStop = 0;
    cmd_ieee_rx.frameFiltOpt.autoAckEn = 1;
    cmd_ieee_rx.frameFiltOpt.slottedAckEn = 0;
    cmd_ieee_rx.frameFiltOpt.autoPendEn = 1;
    cmd_ieee_rx.frameFiltOpt.defaultPend = 1;
    cmd_ieee_rx.frameFiltOpt.bPendDataReqOnly = 0;
    cmd_ieee_rx.frameFiltOpt.bPanCoord = 0;
    cmd_ieee_rx.frameFiltOpt.maxFrameVersion = 3;
    cmd_ieee_rx.frameFiltOpt.bStrictLenFilter = 1;

    /* Receive all frame types */
    cmd_ieee_rx.frameTypes.bAcceptFt0Beacon = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt1Data = 1;
    cmd_ieee_rx.frameTypes.bAcceptFt2Ack = 1;
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

    cmd_ieee_rx.numExtEntries = 0x00;
    cmd_ieee_rx.numShortEntries = 0x00;
    cmd_ieee_rx.pExtEntryList = 0;
    cmd_ieee_rx.pShortEntryList = 0;

    cmd_ieee_rx.endTrigger.triggerType = TRIG_NEVER;
    cmd_ieee_rx.endTime = 0x00000000;
}

uint_fast16_t rf_core_cmd_radio_setup()
{
    volatile rfc_CMD_RADIO_SETUP_t radio_setup_cmd;
    uint16_t doorbell_ret;

    /* Create radio setup command */
    memset((void *)&radio_setup_cmd, 0, sizeof(radio_setup_cmd));

    radio_setup_cmd.commandNo = CMD_RADIO_SETUP;
    radio_setup_cmd.condition.rule = COND_NEVER;

    /* initally set the radio tx power to the max */
    radio_setup_cmd.txPower = cur_output_power->value;
    radio_setup_cmd.pRegOverride = ieee_overrides;
    radio_setup_cmd.mode = 1;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&radio_setup_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }

    /* NOTE: order is important here */
    while(radio_setup_cmd.status == IDLE 
    		|| radio_setup_cmd.status == PENDING
			|| radio_setup_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
    	;
    }

    return radio_setup_cmd.status;
}

uint_fast16_t rf_core_cmd_start_rat(void)
{
    volatile rfc_CMD_SYNC_START_RAT_t start_rat_cmd;
    uint16_t doorbell_ret;

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    memset((void *)&start_rat_cmd, 0, sizeof(start_rat_cmd));

    start_rat_cmd.commandNo = CMD_SYNC_START_RAT;
    start_rat_cmd.condition.rule = COND_NEVER;

    /* copy the value and send back */
    start_rat_cmd.rat0 = rat_offset;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&start_rat_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }

    /* NOTE: order is important here */
    while(start_rat_cmd.status == IDLE 
    		|| start_rat_cmd.status == PENDING
			|| start_rat_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
    	;
    }

    return start_rat_cmd.status;
}

uint_fast16_t rf_core_cmd_stop_rat(void)
{
    volatile rfc_CMD_SYNC_STOP_RAT_t stop_rat_cmd;
    uint16_t doorbell_ret;

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    memset((void *)&stop_rat_cmd, 0, sizeof(stop_rat_cmd));

    stop_rat_cmd.commandNo = CMD_SYNC_STOP_RAT;
    stop_rat_cmd.condition.rule = COND_NEVER;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&stop_rat_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }

    /* NOTE: order is important here */
    while(stop_rat_cmd.status == IDLE 
    		|| stop_rat_cmd.status == PENDING
			|| stop_rat_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
    	;
    }
    rat_offset = stop_rat_cmd.rat0;
    return stop_rat_cmd.status;
}

uint_fast8_t rf_core_cmd_set_tx_power(int dBm)
{
    volatile rfc_CMD_SET_TX_POWER_t tx_power_cmd;
    unsigned int i;
    ASSERT(dBm <= OUTPUT_POWER_MAX
            && dBm >= OUTPUT_POWER_MIN);

    memset((void *)&tx_power_cmd, 0, sizeof(tx_power_cmd));
    for(i=0; i < OUTPUT_CONFIG_COUNT
            && output_power[i].dbm != dBm; i++)
    {
        ;
    }
    if(i != OUTPUT_CONFIG_COUNT)
    {
        cur_output_power = &(output_power[i]);
    }

    tx_power_cmd.commandNo = CMD_SET_TX_POWER;
    tx_power_cmd.txPower = cur_output_power->value;

    /* immediate command, we don't need to wait */
    return (RFCDoorbellSendTo((uint32_t)&tx_power_cmd) & 0xFF);
}

uint_fast8_t rf_core_cmd_abort()
{
    /* direct command, we don't need to wait */
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT)) & 0xFF);
}

uint_fast8_t rf_core_cmd_abort_fg()
{
    /* direct command, we don't need to wait */
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_IEEE_ABORT_FG)) & 0xFF);
}

uint_fast8_t rf_core_cmd_ping()
{
    /* direct command, we don't need to wait */
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING)) & 0xFF);
}

uint_fast8_t rf_core_cmd_stop()
{
    /* direct command, we don't need to wait */
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_STOP)) & 0xFF);
}

uint_fast8_t rf_core_cmd_clear_rx(dataQueue_t *queue)
{
    rfc_CMD_CLEAR_RX_t clear_rx_cmd;
    memset((void *)&clear_rx_cmd, 0, sizeof(clear_rx_cmd));

    clear_rx_cmd.commandNo = CMD_CLEAR_RX;
    clear_rx_cmd.pQueue = queue;

    /* immediate command, we don't need to wait */
    return (RFCDoorbellSendTo((uint32_t)&clear_rx_cmd) & 0xFF);
}

#if 0
uint_fast16_t rf_core_cmd_fs(unsigned int channel, bool txMode)
{
    volatile rfc_CMD_FS_t fs_cmd;
    unsigned int i;
    uint8_t doorbell_ret;
    ASSERT(channel <= CHANNEL_FREQ_MAX
            && channel >= CHANNEL_FREQ_MIN);

    memset((void *)&fs_cmd, 0, sizeof(fs_cmd));
    for(i=0; i < CHANNEL_FREQUENCY_COUNT
            && channel_frequency[i].channel != channel; i++)
    {
        ;
    }

    fs_cmd.commandNo = CMD_FS;
    fs_cmd.status = IDLE;
    fs_cmd.startTime = 0;
    fs_cmd.startTrigger.triggerType = TRIG_NOW;
    fs_cmd.condition.rule = COND_NEVER;
    fs_cmd.frequency = channel_frequency[i].frequency;
    fs_cmd.synthConf.bTxMode = txMode ? 1 : 0;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&fs_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }
    /* NOTE: order is important here */
    while(fs_cmd.status == IDLE 
    		|| fs_cmd.status == PENDING
			|| fs_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
    	;
    }
    return fs_cmd.status;
}
#endif

uint_fast16_t rf_core_cmd_fs_powerdown(void)
{
    volatile rfc_CMD_FS_POWERDOWN_t fs_powerdown_cmd;
    uint16_t doorbell_ret;
    memset((void *)&fs_powerdown_cmd, 0, sizeof(fs_powerdown_cmd));

    fs_powerdown_cmd.commandNo = CMD_FS_POWERDOWN;
    fs_powerdown_cmd.condition.rule = COND_NEVER;

    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&fs_powerdown_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }
    /* NOTE: order is important here */
    while(fs_powerdown_cmd.status == IDLE 
    		|| fs_powerdown_cmd.status == PENDING
			|| fs_powerdown_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
        ;
    }

    return fs_powerdown_cmd.status;
}

uint_fast16_t rf_core_cmd_ieee_tx(uint8_t *psdu, uint8_t len)
{
    volatile rfc_CMD_IEEE_TX_t ieee_tx_cmd;
    uint8_t doorbell_ret;

    memset((void *)&ieee_tx_cmd, 0, sizeof(ieee_tx_cmd));
    ieee_tx_cmd.commandNo = CMD_IEEE_TX;
    ieee_tx_cmd.condition.rule = COND_NEVER;
    ieee_tx_cmd.payloadLen = len;
    ieee_tx_cmd.pPayload = psdu;
    ieee_tx_cmd.startTrigger.triggerType = TRIG_NOW;


    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&ieee_tx_cmd) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }
    /* NOTE: order is important here */
    while(ieee_tx_cmd.status == IDLE 
    		|| ieee_tx_cmd.status == PENDING
			|| ieee_tx_cmd.status == ACTIVE)
    {
        /* TODO: sleep while polling */
        ;
    }

    return ieee_tx_cmd.status;
}

uint_fast16_t rf_core_cmd_ieee_rx()
{
    uint8_t doorbell_ret;
    cmd_ieee_rx.status = IDLE;
    doorbell_ret = (RFCDoorbellSendTo((uint32_t)&cmd_ieee_rx) & 0xFF);
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }
    while(cmd_ieee_rx.status == IDLE
    		|| cmd_ieee_rx.status == PENDING);
    {
        /* TODO: sleep while polling */
        ;
    }
    return cmd_ieee_rx.status;
}

void rf_core_setup_interrupts()
{
    bool interrupts_disabled;

    /* We are already turned on by the caller, so this should not happen */
    if(!PRCMRfReady()) {
        return;
    }

    /* Disable interrupts */
    interrupts_disabled = IntMasterDisable();

    /* Set all interrupt channels to CPE0 channel, error to CPE1 */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = IRQ_INTERNAL_ERROR;

    /* Acknowledge configured interrupts */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = IRQ_RX_NOK;

    /* Clear interrupt flags, active low clear */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntEnable(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_1);

    if(!interrupts_disabled) {
        IntMasterEnable();
    }
}

uint8_t rf_core_set_modesel()
{
    uint8_t rv = RF_CORE_ERROR;

    if(ChipInfo_ChipFamilyIsCC26xx()) {
        if(ChipInfo_SupportsBLE() == true &&
                ChipInfo_SupportsIEEE_802_15_4() == true) {
            /* CC2650 */
            HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE5;
            rv = RF_CORE_OK;
        } else if(ChipInfo_SupportsBLE() == false &&
                ChipInfo_SupportsIEEE_802_15_4() == true) {
            /* CC2630 */
            HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE2;
            rv = RF_CORE_OK;
        }
    } else if(ChipInfo_ChipFamilyIsCC13xx()) {
        if(ChipInfo_SupportsBLE() == false &&
                ChipInfo_SupportsIEEE_802_15_4() == false) {
            /* CC1310 */
            HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE3;
            rv = RF_CORE_OK;
        }
    }

    return rv;
}

ThreadError rf_core_update_rx(bool clearQueue)
{
    if(!PRCMRfReady())
    {
        /* The whole rf core is off, we don't need to restart if it is not
         * started.
         */
        return kThreadError_None;
    }

    if(sState != kStateReceive)
    {
        /* The change will take effect the next time we are put into one of
         * these states.
         */
        return kThreadError_None;
    }

    if(rf_core_cmd_abort() != CMDSTA_Done)
    {
        return kThreadError_Failed;
    }

    if(clearQueue && rf_core_cmd_clear_rx(&rx_data_queue) != CMDSTA_Done)
    {
        sState = kStateSleep;
        return kThreadError_Failed;
    }

    if(rf_core_cmd_ieee_rx() != ACTIVE)
    {
        sState = kStateSleep;
        return kThreadError_Failed;
    }
    return kThreadError_None;
}

uint8_t rf_core_power_on(void)
{
    bool interrupts_disabled;
    /*
     * Request the HF XOSC as the source for the HF clock. Needed before we can
     * use the FS. This will only request, it will _not_ perform the switch.
     */
    if(OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
        /*
         * Request to switch to the crystal to enable radio operation. It takes a
         * while for the XTAL to be ready so instead of performing the actual
         * switch, we do other stuff while the XOSC is getting ready.
         */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_XOSC_HF);
    }

    rf_core_set_modesel();

    /* Set of RF Core data queue. Circular buffer, no last entry */
    rx_data_queue.pCurrEntry = rx_buf_0;
    rx_data_queue.pLastEntry = NULL;

    init_buffers();

    /*
     * Trigger a switch to the XOSC, so that we can subsequently use the RF FS
     * This will block until the XOSC is actually ready, but give how we
     * requested it early on, this won't be too long a wait.
     * This should be done before starting the RAT.
     */
    if(OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }

    interrupts_disabled = IntMasterDisable();

    /* Enable RF Core power domain */
    PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_ON)
    {
        ;
    }

    PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();
    while(!PRCMLoadGet())
    {
        ;
    }

    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;
    IntEnable(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_1);

    if(!interrupts_disabled) {
        IntMasterEnable();
    }

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = (RFC_PWR_PWMCLKEN_RFC_M
      | RFC_PWR_PWMCLKEN_CPE_M | RFC_PWR_PWMCLKEN_CPERAM_M);

    /* Send ping (to verify RFCore is ready and alive) */
    if(rf_core_cmd_ping() != CMDSTA_Done) {
        return RF_CORE_ERROR;
    }

    return RF_CORE_OK;
}

uint8_t rf_core_power_off(void)
{
    bool interrupts_disabled;
    interrupts_disabled = IntMasterDisable();

    /* clear and disable interrupts */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;

    if(rf_core_cmd_fs_powerdown() != DONE_OK)
    {
        /* continue anyway */
    }

    PRCMDomainDisable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();
    while(!PRCMLoadGet())
    {
        ;
    }

    /* Enable RF Core power domain */
    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_OFF)
    {
        ;
    }

    /*
     * Request the HF RCOSC as the source for the HF clock. Used to save power
     * from the XOSC. This will only request, it will _not_ perform the switch.
     */
    if(OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_RCOSC_HF) {
        /*
         * Request to switch to the RC osc for low power mode.
         */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_RCOSC_HF);
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }

    IntPendClear(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_1);
    IntDisable(INT_RFC_CPE_0);
    IntDisable(INT_RFC_CPE_1);

    if(!interrupts_disabled) {
        IntMasterEnable();
    }

    return RF_CORE_OK;
}

uint8_t rf_core_wakeup(void)
{
    if(rf_core_cmd_start_rat() != DONE_OK)
    {
        return RF_CORE_ERROR;
    }

    rf_core_setup_interrupts();

    if(rf_core_cmd_radio_setup() != DONE_OK)
    {
        return RF_CORE_ERROR;
    }
    return RF_CORE_OK;
}

uint8_t rf_core_sleep(void)
{
    if(rf_core_cmd_abort() != CMDSTA_Done)
    {
        return RF_CORE_ERROR;
    }

    if(rf_core_cmd_fs_powerdown() != DONE_OK)
    {
        return RF_CORE_ERROR;
    }

    if(rf_core_cmd_stop_rat() != DONE_OK)
    {
        return RF_CORE_ERROR;
    }
    return RF_CORE_OK;
}

void RFCCPE1IntHandler(void)
{
    if(!PRCMRfReady()) {
    }

    /* Clear INTERNAL_ERROR interrupt flag */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x7FFFFFFF;
}

void RFCCPE0IntHandler(void)
{
    if(!PRCMRfReady()) {
    }

    if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_RX_ENTRY_DONE) {
        /* Clear the RX_ENTRY_DONE interrupt flag */
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFF7FFFFF;
        /* XXX: notifiy polling function there is a frame ?? */
    }

    if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_RX_NOK) {
        /* Clear the RX_NOK interrupt flag */
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFFFDFFFF;
    }

    if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) &
            (IRQ_LAST_FG_COMMAND_DONE | IRQ_LAST_COMMAND_DONE)) {
        /* Clear the two TX-related interrupt flags */
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFFFFFFF5;
        /* XXX: we are usually polling for a command being done */
    }
}

void cc2650RadioInit(void)
{
    /* Populate the RX parameters data structure with default values */
    init_rf_params();

    sState = kStateDisabled;
}

ThreadError otPlatRadioEnable(void)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if(sState == kStateDisabled)
    {
        if(rf_core_power_on() == RF_CORE_OK)
        {
            sState = kStateSleep;
            error = kThreadError_None;
        }
        else
        {
            sState = kStateDisabled;
            rf_core_power_off();
            error = kThreadError_Failed;
        }
    }
    return error;
}

ThreadError otPlatRadioDisable(void)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateDisabled)
    {
        error = kThreadError_None;
    }

    if(sState == kStateSleep)
    {
        if(rf_core_power_off() == RF_CORE_OK)
        {
            sState = kStateDisabled;
            error = kThreadError_None;
        }
    }
    return error;
}

ThreadError otPlatRadioSleep(void)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if(sState == kStateReceive)
    {
        if(rf_core_sleep() != RF_CORE_OK)
        {
            sState = kStateSleep;
            error = kThreadError_None;
        }
    }
    
    return error;
}

int8_t otPlatRadioGetNoiseFloor(void)
{
    /* TODO: is this meant to be the largest observed RSSI? */
    return 0;
}

otRadioCaps otPlatRadioGetCaps(void)
{
    return kRadioCapsNone;
}

bool otPlatRadioGetPromiscuous(void)
{
    /* we are promiscuous if we are not filtering */
    return cmd_ieee_rx.frameFiltOpt.frameFiltEn == 0;
}

void otPlatRadioSetPromiscuous(bool aEnable)
{
    /* if we are promiscuous, disable frame filtering */
    cmd_ieee_rx.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
    rf_core_update_rx(false);
}

ThreadError otPlatRadioSetPanId(uint16_t panid)
{
    /* XXX: if the pan id is the broadcast pan id (0xFFFF) the auto ack will
     * not work. This is due to the design of the CM0 and follows IEEE 802.15.4
     */
    cmd_ieee_rx.localPanID = panid;
    return rf_core_update_rx(true);
}

ThreadError otPlatRadioSetExtendedAddress(uint8_t *address)
{
    int i;
    uint8_t *rxAddr = (uint8_t *)(&(cmd_ieee_rx.localExtAddr));
    for(i=0; i<8; i++)
    {
        rxAddr[i] = address[i];
    }
    return rf_core_update_rx(true);
}

ThreadError otPlatRadioSetShortAddress(uint16_t address)
{
    cmd_ieee_rx.localShortAddr = address;
    return rf_core_update_rx(true);
}

void readFrame(void)
{
    uint8_t crcCorr;
    uint8_t rssi;
    rfc_dataEntryGeneral_t *startEntry =
        (rfc_dataEntryGeneral_t *)rx_data_queue.pCurrEntry;
    rfc_dataEntryGeneral_t *curEntry = startEntry;

    /* loop through receive queue */
    do
    {
        uint8_t *payload = &(curEntry->data);
        if(sReceiveFrame.mLength == 0
            && curEntry->status == DATA_ENTRY_FINISHED)
        {
            uint8_t len = payload[0];
            /* 
             * get the information appended to the end of the frame.
             * This array access looks like it is a fencepost error, but the
             * first byte is the number of bytes that follow.
             */
            crcCorr = payload[len];
            rssi = payload[len - 1];
            if(!(crcCorr & CC2650_CRC_BIT_MASK)
                    && (len - 2) < kMaxPHYPacketSize) 
            {
                sReceiveFrame.mLength = len;
                memcpy(sReceiveFrame.mPsdu, &(payload[1]), len - 2);
                sReceiveFrame.mChannel = cmd_ieee_rx.channel;
                sReceiveFrame.mPower = rssi;
                sReceiveFrame.mLqi = crcCorr & CC2650_LQI_BIT_MASK;

                sReceiveError = kThreadError_None;
            }
            else
            {
                sReceiveError = kThreadError_FcsErr;
            }
            curEntry->status = DATA_ENTRY_PENDING;
            break;
        }
        else if(curEntry->status == DATA_ENTRY_UNFINISHED)
        {
            curEntry->status = DATA_ENTRY_PENDING;
        }
        curEntry = (rfc_dataEntryGeneral_t *)(curEntry->pNextEntry);
    }
    while(curEntry != startEntry);

    return;
}

int cc2650RadioProcess(void)
{
    if(sState == kStateTransmit
            && (((sTransmitPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
                || (sTransmitError != kThreadError_None)))
    {
        /* we are not looking for an ACK packet, or failed */
        sState = kStateReceive;
        otPlatRadioTransmitDone(false, sTransmitError);
    }
    else if(sState == kStateReceive || sState == kStateTransmit)
    {
        readFrame();

        if (sState == kStateTransmit
                && (sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK
                && (sReceiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET]))
        {
            /* XXX: our RX command only accepts ack packets that are 5 bytes */
            sState = kStateReceive;
            otPlatRadioTransmitDone((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0, sTransmitError);
        }
        else if (sState == kStateReceive && sReceiveFrame.mLength > 0)
        {
            otPlatRadioReceiveDone(&sReceiveFrame, sReceiveError);
        }
        sReceiveFrame.mLength = 0;
    }
    return 0;
}

RadioPacket *otPlatRadioGetTransmitBuffer(void)
{
    return &sTransmitFrame;
}

ThreadError otPlatRadioTransmit(void)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateReceive)
    {
        /*
         * This is the easiest way to setup the frequency synthesizer.
         * And we are supposed to fall into the receive state afterwards.
         */
        error = otPlatRadioReceive(sTransmitFrame.mChannel);
        if(error == kThreadError_None)
        {
            sState = kStateTransmit;

            /* removing 2 bytes of CRC placeholder because we generate that in hardware */
            switch(rf_core_cmd_ieee_tx(sTransmitFrame.mPsdu, sTransmitFrame.mLength - 2))
            {
                case IEEE_DONE_OK:
                    error = kThreadError_None;
                    break;
                case IEEE_DONE_TIMEOUT:
                    error = kThreadError_ChannelAccessFailure;
                    break;
                case IEEE_ERROR_NO_SETUP:
                case IEEE_ERROR_NO_FS:
                case IEEE_ERROR_SYNTH_PROG:
                    error = kThreadError_InvalidState;
                    break;
                case IEEE_ERROR_TXUNF:
                    error = kThreadError_NoBufs;
                    break;
                default:
                    error = kThreadError_Error;
            }
            sTransmitError = error;
        }
    }
    return error;
}

ThreadError otPlatRadioReceive(uint8_t aChannel)
{
    ThreadError error = kThreadError_Busy;

    switch (sState)
    {
        case kStateDisabled:
            break;
        case kStateSleep:
            if(rf_core_wakeup() != RF_CORE_OK)
            {
                error = kThreadError_Failed;
                break;
            }
            sState = kStateReceive;
            /* XXX: fall through */
        case kStateReceive:
            if(cmd_ieee_rx.status == ACTIVE)
            {
                /* we are running a receive already */
                if(cmd_ieee_rx.channel == aChannel)
                {
                    /* we are already on the right channel */
                    error = kThreadError_None;
                    break;
                }
                if(rf_core_cmd_abort() != CMDSTA_Done)
                {
                    error = kThreadError_Failed;
                    break;
                }
                /* any frames in the queue will be for the old channel */
                if(rf_core_cmd_clear_rx(&rx_data_queue) != CMDSTA_Done)
                {
                    error = kThreadError_Failed;
                    break;
                }
                /* make sure that the receive command was aborted */
                while(cmd_ieee_rx.status != IEEE_DONE_ABORT)
                {
                    ;
                }
            }
            else
            {
                if(rf_core_cmd_abort() != CMDSTA_Done)
                {
                    error = kThreadError_Failed;
                    break;
                }
            }
            cmd_ieee_rx.channel = aChannel;
            if(rf_core_cmd_ieee_rx() != ACTIVE)
            {
                error = kThreadError_Failed;
                break;
            }
            error = kThreadError_None;
            break;
        case kStateTransmit:
            if(cmd_ieee_rx.channel == aChannel)
            {
                if(cmd_ieee_rx.status == ACTIVE)
                {
                    /* The command is running on the right channel already */
                    error = kThreadError_None;
                    sState = kStateReceive;
                    break;
                }
                else if(cmd_ieee_rx.status == IEEE_SUSPENDED)
                {
                    /*
                     * The RX command is suspended in the background,
                     * abort the command running in the foreground
                     */
                    if(rf_core_cmd_abort_fg() != CMDSTA_Done)
                    {
                        error = kThreadError_Failed;
                        break;
                    }
                    error = kThreadError_None;
                    sState = kStateReceive;
                    break;
                }
            }
            /* abort all commands running on the radio */
            if(rf_core_cmd_abort() != CMDSTA_Done)
            {
                error = kThreadError_Failed;
                break;
            }
            /* any frames in the queue will be for the old channel */
            if(rf_core_cmd_clear_rx(&rx_data_queue) != CMDSTA_Done)
            {
                error = kThreadError_Failed;
                break;
            }
            cmd_ieee_rx.channel = aChannel;
            if(rf_core_cmd_ieee_rx() != ACTIVE)
            {
                error = kThreadError_Failed;
                break;
            }
            error = kThreadError_None;
            sState = kStateReceive;
            break;
        default:
            break;
    }
    return error;
}

