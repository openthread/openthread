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

#include <assert.h>
#include <common/code_utils.hpp>
#include <platform/radio.h>

#include <driverlib/prcm.h>
#include <inc/hw_prcm.h>
#include <driverlib/rfc.h>
#include <driverlib/osc.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/chipinfo.h>

#define CC2650_CRC_BIT_MASK (1<<8)
#define CC2650_LQI_BIT_MASK 0x3f

#define IEEE802154_MIN_LENGTH 5
#define IEEE802154_MAX_LENGTH 127
#define IEEE802154_FRAME_TYPE_MASK 0x7
#define IEEE802154_FRAME_TYPE_ACK 0x2
#define IEEE802154_FRAME_PENDING (1 << 4)
#define IEEE802154_ACK_REQUEST (1 << 5)
#define IEEE802154_DSN_OFFSET 2

/* phy state as defined by openthread */
static volatile PhyState sState;
/* phy states not defined by openthread */
static volatile bool enabaling = false;
static volatile bool transmitting = false;

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
 * radio commands that run on the CM0
 */
static volatile rfc_CMD_SYNC_START_RAT_t cmd_start_rat;
static volatile rfc_CMD_SYNC_STOP_RAT_t cmd_stop_rat;
static volatile rfc_CMD_RADIO_SETUP_t cmd_radio_setup;
static volatile rfc_CMD_FS_POWERDOWN_t cmd_fs_powerdown;
static volatile rfc_CMD_IEEE_RX_t cmd_ieee_rx;
static volatile rfc_CMD_IEEE_TX_t cmd_ieee_tx;
static volatile rfc_CMD_CLEAR_RX_t cmd_clear_rx;

/* struct containing radio stats */
static rfc_ieeeRxOutput_t rf_stats;

/* size of length in recv struct */
#define DATA_ENTRY_LENSZ_BYTE 1

#define RX_BUF_SIZE 144
/* two receive buffers entries with room for 1 max IEEE802.15.4 frame in each */
static uint8_t rx_buf_0[RX_BUF_SIZE] __attribute__ ((aligned (4)));
static uint8_t rx_buf_1[RX_BUF_SIZE] __attribute__ ((aligned (4)));

/* The RX Data Queue */
static dataQueue_t rx_data_queue = { 0 };

/* openthread data primatives */
static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitError;
static ThreadError sReceiveError;

static uint8_t sTransmitPsdu[kMaxPHYPacketSize] __attribute__ ((aligned (4))) ;
static uint8_t sReceivePsdu[kMaxPHYPacketSize] __attribute__ ((aligned (4))) ;

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

static uint_fast8_t rf_core_cmd_abort(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT)) & 0xFF);
}

static uint_fast8_t rf_core_cmd_ping(void)
{
    return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING)) & 0xFF);
}

static uint_fast8_t rf_core_cmd_clear_rx(dataQueue_t *queue)
{
    memset((void *)&cmd_clear_rx, 0, sizeof(cmd_clear_rx));

    cmd_clear_rx.commandNo = CMD_CLEAR_RX;
    cmd_clear_rx.pQueue = queue;

    return (RFCDoorbellSendTo((uint32_t)&cmd_clear_rx) & 0xFF);
}

static uint_fast8_t rf_core_cmd_ieee_tx(uint8_t *psdu, uint8_t len)
{
    memset((void *)&cmd_ieee_tx, 0, sizeof(cmd_ieee_tx));
    cmd_ieee_tx.commandNo = CMD_IEEE_TX;
    cmd_ieee_tx.condition.rule = COND_NEVER;
    cmd_ieee_tx.payloadLen = len;
    cmd_ieee_tx.pPayload = psdu;
    cmd_ieee_tx.startTrigger.triggerType = TRIG_NOW;

    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_tx) & 0xFF);
}

static uint_fast8_t rf_core_cmd_ieee_rx(void)
{
    cmd_ieee_rx.status = IDLE;
    return (RFCDoorbellSendTo((uint32_t)&cmd_ieee_rx) & 0xFF);
}

static void rf_core_setup_interrupts(void)
{
    bool interrupts_disabled;

    /* We are already turned on by the caller, so this should not happen */
    if(!PRCMRfReady()) {
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

    if(!interrupts_disabled) {
        IntMasterEnable();
    }
}

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

    if(!interrupts_disabled) {
        IntMasterEnable();
    }
}

static void rf_core_set_modesel(void)
{
    switch(ChipInfo_GetChipType())
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

static uint_fast8_t rf_core_power_on(void)
{
    bool interrupts_disabled;
    /* Request the HF XOSC as the source for the HF clock. Needed before we can
     * use the FS. This will only request, it will _not_ perform the switch.
     */
    if(OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
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

    rf_core_setup_interrupts();

    if(!interrupts_disabled) {
        IntMasterEnable();
    }

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = (RFC_PWR_PWMCLKEN_RFC_M
      | RFC_PWR_PWMCLKEN_CPE_M | RFC_PWR_PWMCLKEN_CPERAM_M);

    /* Send ping (to verify RFCore is ready and alive) */
    return rf_core_cmd_ping();
}

static void rf_core_power_off(void)
{
    rf_core_disable_interrupts();

    PRCMDomainDisable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();
    while(!PRCMLoadGet())
    {
        ;
    }

    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_OFF)
    {
        ;
    }

    if(OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_RCOSC_HF) {
        /* Request to switch to the RC osc for low power mode. */
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_RCOSC_HF);
        /* Switch the HF clock source (cc26xxware executes this from ROM) */
        OSCHfSourceSwitch();
    }
}

static uint_fast8_t rf_core_cmds_enable(void)
{
    /* turn on the clock line to the radio core */
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initialize the rat start command */
    memset((void *)&cmd_start_rat, 0x00, sizeof(cmd_start_rat));
    cmd_start_rat.commandNo = CMD_SYNC_START_RAT;
    cmd_start_rat.condition.rule = COND_STOP_ON_FALSE;
    cmd_start_rat.pNextOp =(rfc_radioOp_t *)&(cmd_radio_setup);
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

static uint_fast16_t rf_core_cmds_disable(void)
{
    uint8_t doorbell_ret;
    bool interrupts_disabled;

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

    /* initiialize the command to power down the frequency synth */
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
    if(doorbell_ret != CMDSTA_Done)
    {
        return doorbell_ret;
    }

    /* syncronously wait for the CM0 to stop */
    while((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) &
                IRQ_LAST_COMMAND_DONE) == 0x00)
    {
        ;
    }

    if(!interrupts_disabled) {
        IntMasterEnable();
    }

    if(cmd_stop_rat.status == DONE_OK)
    {
        rat_offset = cmd_stop_rat.rat0;
    }
    return cmd_stop_rat.status;
}

/* error interrupt handler */
void RFCCPE1IntHandler(void)
{
    if(!PRCMRfReady()) {
    }

    /* Clear INTERNAL_ERROR interrupt flag */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x7FFFFFFF;
}

/* command done handler */
void RFCCPE0IntHandler(void)
{
    if(sState == kStateSleep)
    {
        if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE)
        {
            if(cmd_radio_setup.status == DONE_OK)
            {
                enabaling = false;
                HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;
            }
            else
            {
                rf_core_power_off();
                sState = kStateDisabled;
            }
        }
    }
    else if(sState == kStateReceive)
    {
        /* the rx command was probably aborted to change the channel */
        sState = kStateSleep;
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;
    }
    else if(sState == kStateTransmit)
    {
        switch(cmd_ieee_tx.status)
        {
            case IDLE:
            case PENDING:
            case ACTIVE:
                break;
            case IEEE_DONE_OK:
                transmitting = false;
                sTransmitError = kThreadError_None;
                break;
            case IEEE_DONE_TIMEOUT:
                transmitting = false;
                sTransmitError = kThreadError_ChannelAccessFailure;
                break;
            case IEEE_ERROR_NO_SETUP:
            case IEEE_ERROR_NO_FS:
            case IEEE_ERROR_SYNTH_PROG:
                transmitting = false;
                sTransmitError = kThreadError_InvalidState;
                break;
            case IEEE_ERROR_TXUNF:
                transmitting = false;
                sTransmitError = kThreadError_NoBufs;
                break;
            default:
                transmitting = false;
                sTransmitError = kThreadError_Error;
        }
        /* we let the process function move us back to receive */
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_FG_COMMAND_DONE;
    }
}

void cc2650RadioInit(void)
{
    /* Populate the RX parameters data structure with default values */
    rf_core_init_rx_params();

    sState = kStateDisabled;
}

ThreadError otPlatRadioEnable(void)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if(sState == kStateDisabled && !enabaling)
    {
        VerifyOrExit(rf_core_power_on() == CMDSTA_Done, error = kThreadError_Failed);
        enabaling = true;
        sState = kStateSleep;
        VerifyOrExit(rf_core_cmds_enable() == CMDSTA_Done, error = kThreadError_Failed);
    }

exit:
    if(error == kThreadError_Failed)
    {
        rf_core_power_off();
        sState = kStateDisabled;
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

ThreadError otPlatRadioReceive(uint8_t aChannel)
{
    ThreadError error = kThreadError_Busy;

    if(sState == kStateSleep && !enabaling)
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
    else if(sState == kStateReceive || sState == kStateTransmit)
    {
        if(cmd_ieee_rx.status == ACTIVE && cmd_ieee_rx.channel == aChannel)
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
            VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done,
                    error = kThreadError_Failed);

            cmd_ieee_rx.channel = aChannel;
            VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error = kThreadError_Failed);

            sState = kStateReceive;
            error = kThreadError_None;
        }
    }

exit:
    return error;
}

ThreadError otPlatRadioSleep(void)
{
    ThreadError error = kThreadError_None;

    if(sState == kStateSleep)
    {
        error = kThreadError_None;
    }

    if(sState == kStateReceive)
    {
        if(rf_core_cmd_abort() != CMDSTA_Done)
        {
            error = kThreadError_Busy;
            return error;
        }

        sState = kStateSleep;
        return error;
    }
    
    return error;
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
            transmitting = true;
            VerifyOrExit(rf_core_cmd_ieee_tx(sTransmitFrame.mPsdu,
                        sTransmitFrame.mLength - 2) == CMDSTA_Done, error =
                    kThreadError_Failed);
            error = kThreadError_None;
        }
    }

exit:
    sTransmitError = error;
    return error;
}

int8_t otPlatRadioGetRssi(void)
{
    /* XXX: is this meant to be the largest or last observed RSSI? */
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
    if(aEnable != (cmd_ieee_rx.frameFiltOpt.frameFiltEn == 0))
    {
        if(sState == kStateReceive)
        {
            SuccessOrExit(rf_core_cmd_abort() == CMDSTA_Done);
            /* if we are promiscuous, disable frame filtering */
            cmd_ieee_rx.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
            SuccessOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done);
        }
        else if(sState != kStateTransmit)
        {
            /* if we are promiscuous, disable frame filtering */
            cmd_ieee_rx.frameFiltOpt.frameFiltEn = aEnable ? 0 : 1;
        }
    }
exit:
    return;
}

ThreadError otPlatRadioSetPanId(uint16_t panid)
{
    ThreadError error = kThreadError_None;
    /* XXX: if the pan id is the broadcast pan id (0xFFFF) the auto ack will
     * not work. This is due to the design of the CM0 and follows IEEE 802.15.4
     */
    if(sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, error=kThreadError_Failed);
        cmd_ieee_rx.localPanID = panid;
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, error=kThreadError_Failed);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error=kThreadError_Failed);
    }
    else if(sState != kStateTransmit)
    {
        cmd_ieee_rx.localPanID = panid;
    }
exit:
    return error;
}

ThreadError otPlatRadioSetExtendedAddress(uint8_t *address)
{
    /* XXX: assuming little endian format */
    ThreadError error = kThreadError_None;
    if(sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, error=kThreadError_Failed);
        cmd_ieee_rx.localExtAddr = *((uint64_t *)(address));
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, error=kThreadError_Failed);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error=kThreadError_Failed);
    }
    else if(sState != kStateTransmit)
    {
        cmd_ieee_rx.localExtAddr = *((uint64_t *)(address));
    }
exit:
    return error;
}

ThreadError otPlatRadioSetShortAddress(uint16_t address)
{
    ThreadError error = kThreadError_None;
    if(sState == kStateReceive)
    {
        VerifyOrExit(rf_core_cmd_abort() == CMDSTA_Done, error=kThreadError_Failed);
        cmd_ieee_rx.localShortAddr = address;
        VerifyOrExit(rf_core_cmd_clear_rx(&rx_data_queue) == CMDSTA_Done, error=kThreadError_Failed);
        VerifyOrExit(rf_core_cmd_ieee_rx() == CMDSTA_Done, error=kThreadError_Failed);
    }
    else if(sState != kStateTransmit)
    {
        cmd_ieee_rx.localShortAddr = address;
    }
exit:
    return error;
}

static void readFrame(void)
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
            /* get the information appended to the end of the frame.
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
    if((sState == kStateTransmit && !transmitting)
            && (((sTransmitPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
                || (sTransmitError != kThreadError_None)))
    {
        /* we are not looking for an ACK packet, or failed */
        sState = kStateReceive;
#if OPENTHREAD_ENABLE_DIAG
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(false, sTransmitError);
        }
        else
#endif /* OPENTHREAD_ENABLE_DIAG */
        {
            otPlatRadioTransmitDone(false, sTransmitError);
        }
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
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0, sTransmitError);
            }
            else
#endif /* OPENTHREAD_ENABLE_DIAG */
            {
                otPlatRadioTransmitDone((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0, sTransmitError);
            }
        }
        else if (sState == kStateReceive && sReceiveFrame.mLength > 0)
        {
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioReceiveDone(&sReceiveFrame, sReceiveError);
            }
            else
#endif /* OPENTHREAD_ENABLE_DIAG */
            {
                otPlatRadioReceiveDone(&sReceiveFrame, sReceiveError);
            }
        }
        sReceiveFrame.mLength = 0;
    }
    return 0;
}
