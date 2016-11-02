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
 * @file radio.cpp
 * Platform abstraction for radio communication.
 */

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/code_utils.hpp>
#include <platform/alarm.h>
#include <platform/radio.h>
#include <openthread.h>
#include "platform-da15100.h"

#define DEBUG_LOG_ENABLE (0)

#include <ftdf.h>
#include <ad_ftdf.h>
#include "ad_ftdf_phy_api.h"
#include "hw_rf.h"
#include <global_io.h>
#include <regmap.h>
#include "black_orca.h"
#include "core_cm0.h"
#include "internal.h"
#include <hw_uart.h>

typedef struct __attribute__((packed)) RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[kMaxPHYPacketSize];
} RadioMessage;

static PhyState s_state = kStateDisabled;
static RadioMessage s_receive_message;
static RadioMessage s_transmit_message;
static RadioMessage s_ack_message;
static RadioPacket s_receive_frame;
static RadioPacket s_transmit_frame;
static RadioPacket s_ack_frame;
static bool s_data_pending = false;
static bool SendFrameDone = false;
static uint8_t s_extended_address[OT_EXT_ADDRESS_SIZE];
static uint16_t s_short_address;
static uint16_t s_panid;

//static int s_sockfd;

static bool s_promiscuous = false;

static uint8_t sleep_init_delay =0;
static bool SED = false;
static uint32_t sleep_const_delay;

static otInstance *First_Instace;

#define RF_MODE_BLE             0

void disable_interrupt() {
    NVIC_DisableIRQ(FTDF_GEN_IRQn);
}

void enable_interrupt() {
    NVIC_ClearPendingIRQ(FTDF_GEN_IRQn);
    NVIC_EnableIRQ(FTDF_GEN_IRQn);
}


void phy_power_init() {
    // Set the radio_ldo voltage to 1.4 Volts
    REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_SETVDD, 2);
    // Switch on the RF LDO
    REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_ENABLE, 1);
    hw_rf_poweron();
}


void ad_ftdf_init_phy_api_V2(void) {
    NVIC_ClearPendingIRQ(FTDF_WAKEUP_IRQn);
    NVIC_EnableIRQ(FTDF_WAKEUP_IRQn);

    NVIC_ClearPendingIRQ(FTDF_GEN_IRQn);
    NVIC_EnableIRQ(FTDF_GEN_IRQn);

    volatile uint32_t* lmacReset = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_LMACRESET);
    *lmacReset = MSK_R_FTDF_ON_OFF_REGMAP_LMACRESET;

    volatile uint32_t* controlStatus = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_LMAC_CONTROL_STATUS);
    while ((*controlStatus & MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP) == 0) {}

    volatile uint32_t* wakeupTimerEnableStatus = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS);
    FTDF_SET_FIELD(ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE, 0);
    while (*wakeupTimerEnableStatus & MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS) {}

    FTDF_SET_FIELD(ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE, 1);
    while ((*wakeupTimerEnableStatus & MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS) == 0) {}
}

void ad_ftdf_init_lmac() {
    FTDF_SET_FIELD(ON_OFF_REGMAP_CCAIDLEWAIT, 192);
    volatile uint32_t* txFlagClear = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_TX_FLAG_CLEAR);
    *txFlagClear = MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR;

    volatile uint32_t* phyParams = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_PHY_PARAMETERS_2);
    *phyParams = (FTDF_PHYTXSTARTUP << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP) |
                 (FTDF_PHYTXLATENCY << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY) |
                 (FTDF_PHYTXFINISH << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH) |
                 (FTDF_PHYTRXWAIT << OFF_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT);

    phyParams = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_PHY_PARAMETERS_3);
    *phyParams = (FTDF_PHYRXSTARTUP << OFF_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP) |
                 (FTDF_PHYRXLATENCY << OFF_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY) |
                 (FTDF_PHYENABLE << OFF_F_FTDF_ON_OFF_REGMAP_PHYENABLE);

    volatile uint32_t* ftdfCm = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CM);
    *ftdfCm = FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE;

    volatile uint32_t* rxMask = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_RX_MASK);
    *rxMask = MSK_R_FTDF_ON_OFF_REGMAP_RX_MASK;

    volatile uint32_t* lmacMask = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_LMAC_MASK);
    *lmacMask = MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M;

    volatile uint32_t* lmacCtrlMask = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_LMAC_CONTROL_MASK);
    *lmacCtrlMask = MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M |
                    MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M |
                    MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M;

    volatile uint32_t* txFlagClearM;
    txFlagClearM   = FTDF_GET_FIELD_ADDR_INDEXED(ON_OFF_REGMAP_TX_FLAG_CLEAR_M, FTDF_TX_DATA_BUFFER);
    *txFlagClearM |= MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M;
    txFlagClearM   = FTDF_GET_FIELD_ADDR_INDEXED(ON_OFF_REGMAP_TX_FLAG_CLEAR_M, FTDF_TX_WAKEUP_BUFFER);
    *txFlagClearM |= MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M;
}



ThreadError otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    (void)aInstance;

    s_panid = panid;
    FTDF_setValue(FTDF_PIB_PAN_ID, &panid);
    return kThreadError_None;
}

ThreadError otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *address)
{
    (void)aInstance;

    for (unsigned i = 0; i < sizeof(s_extended_address); i++)
    {
        s_extended_address[i] = address[i];
    }

    FTDF_setValue( FTDF_PIB_EXTENDED_ADDRESS,  s_extended_address);
    return kThreadError_None;
}

ThreadError otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    (void)aInstance;

    s_short_address = address;
    FTDF_setValue(FTDF_PIB_SHORT_ADDRESS, &address);
    return kThreadError_None;
}


void da15100RadioInit(void)
{
    /* Wake up power domains */
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);
    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0);
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
    while (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP) == 0x0);

    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_ENABLE, 1);
    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_DIV, 0);

    phy_power_init();
    hw_rf_system_init();
    hw_rf_set_recommended_settings();
    hw_rf_iff_calibration();

    hw_rf_modulation_gain_calibration(RF_MODE_BLE);
    hw_rf_dc_offset_calibration();

    // ad_ftdf_init_phy_api();
    FTDF_pib.CCAMode=2;
    ad_ftdf_init_phy_api_V2();
    ad_ftdf_init_lmac();


}


ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    First_Instace=aInstance;
    (void)aInstance;
    ThreadError error = kThreadError_None;
    uint16_t DefaultChannel = 11;
    uint8_t value = 1;

    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &value); //Wake();
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &DefaultChannel); //SetChannel(channel);

    uint32_t max_retries = 0;
    FTDF_setValue(FTDF_PIB_MAX_FRAME_RETRIES, &max_retries);

    FTDF_Bitmap32 options =  FTDF_TRANSPARENT_ENABLE_FCS_GENERATION;
    options |= FTDF_TRANSPARENT_WAIT_FOR_ACK;
    options |= FTDF_TRANSPARENT_AUTO_ACK;

    FTDF_enableTransparentMode(FTDF_TRUE, options);
    otPlatRadioSetPromiscuous(aInstance, false);

    s_receive_frame.mPsdu = s_receive_message.mPsdu;
    s_transmit_frame.mPsdu = s_transmit_message.mPsdu;
    s_ack_frame.mPsdu = s_ack_message.mPsdu;
    s_state = kStateReceive;
    return error;
}

ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    (void)aInstance;
    s_state = kStateDisabled;
    return kThreadError_None;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return (s_state != kStateDisabled) ? true : false;
}

ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_Busy;
    (void)aInstance;


    if(s_state==kStateReceive && sleep_init_delay==0)
    {
        sleep_init_delay=otPlatAlarmGetNow();
        return kThreadError_None; //error;
    }
    else if((otPlatAlarmGetNow()-sleep_init_delay)<3000)
        return kThreadError_None; //error;

    if (s_state == kStateSleep || s_state == kStateReceive)
    {
        error = kThreadError_None;
        SED=true;
        sleep_const_delay=otPlatAlarmGetNow();
    }

    return error;
}

ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;

#if DEBUG_LOG_ENABLE
    hw_uart_write_buffer(HW_UART1, "\nR", 2);
#endif
    VerifyOrExit(s_state != kStateDisabled, error = kThreadError_Busy);
    s_state = kStateReceive;
    s_receive_frame.mChannel = aChannel;

    uint32_t phyAckAttr;
    phyAckAttr = 0x08 | ((aChannel - 11) & 0xf) << 4 | (0 & 0x3) << 8;
    FTDF_SET_FIELD(ON_OFF_REGMAP_PHYRXATTR, (((aChannel - 11) & 0xf) << 4));
    FTDF_SET_FIELD(ON_OFF_REGMAP_PHYACKATTR, phyAckAttr);

    FTDF_SET_FIELD(ON_OFF_REGMAP_RXALWAYSON, 1);
    FTDF_SET_FIELD(ON_OFF_REGMAP_RXENABLE, 1);

exit:
    return error;
}

RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;
    return &s_transmit_frame;
}

ThreadError otPlatRadioTransmit(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;
    (void)aInstance;

    FTDF_Boolean csmaSuppress = 0;
    VerifyOrExit(s_state == kStateReceive, error = kThreadError_Busy);

    ad_ftdf_send_frame_simple(s_transmit_frame.mLength, s_transmit_frame.mPsdu, s_transmit_frame.mChannel, 0, csmaSuppress); //Prio 0 for all.
    s_state = kStateTransmit;
    s_data_pending = false;
#if DEBUG_LOG_ENABLE
    hw_uart_write_buffer(HW_UART1, "\nT", 2);
#endif

exit:
    return error;
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

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    uint8_t value;
    value = aEnable;
    FTDF_setValue(FTDF_PIB_PROMISCUOUS_MODE, &value);
    s_promiscuous = aEnable;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;
    return s_promiscuous;
}

ThreadError otPlatRadioHandleTransmitDone(bool *rxPending)
{
    ThreadError error = kThreadError_None;
    VerifyOrExit(s_state == kStateTransmit, error = kThreadError_InvalidState);

    s_state = kStateReceive;

    if (rxPending != NULL)
    {
        *rxPending = s_data_pending;
    }

exit:
    return error;
}




void FTDF_sendFrameTransparentConfirm( void*         handle,
                                       FTDF_Bitmap32 status )
{
    //   ThreadError transmit_error_ = kThreadError_None;

    (void)handle;
    (void)status;
    volatile uint32_t* txStatus = FTDF_GET_REG_ADDR_INDEXED(RETENTION_RAM_TX_RETURN_STATUS_1, FTDF_TX_DATA_BUFFER);
    if (*txStatus & MSK_F_FTDF_RETENTION_RAM_ACKFAIL) {
        //   transmit_error_ = kThreadError_None;
    } else if (*txStatus & MSK_F_FTDF_RETENTION_RAM_CSMACAFAIL) {
        //   transmit_error_ = kThreadError_Abort;
    } else {
        //   transmit_error_ = kThreadError_None;
    }
    SendFrameDone = true;

#if DEBUG_LOG_ENABLE
    //otPlatRadioHandleTransmitDone(&test);
    hw_uart_write_buffer(HW_UART1, "\nTD", 3);
#endif
}



void da15100RadioProcess(otInstance *aInstance)
{
    bool rxPending;
    uint8_t value=0;
    if(SendFrameDone)
    {
        FTDF_SET_FIELD(ON_OFF_REGMAP_RXENABLE, 1);
        otPlatRadioHandleTransmitDone(&rxPending);
        otPlatRadioTransmitDone(aInstance, false, kThreadError_None);
        SendFrameDone = false;
    }
    if(SED)
    {
        if((otPlatAlarmGetNow()-sleep_const_delay)>100)
        {
            FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &value);
            s_state = kStateSleep;
            SED=false;
        }

    }
}



void FTDF_rcvFrameTransparent( FTDF_DataLength frameLength,
                               FTDF_Octet*     frame,
                               FTDF_Bitmap32   status )
{
    (void)status;
    RadioPacket otRadioPacket;
    //  int8_t        value;
    //  value  = -20;

    otRadioPacket.mPower = -20;           //FIX THIS -actual measurements shall be placed here
    otRadioPacket.mLqi = kPhyNoLqi;

    if( s_receive_frame.mChannel == 0)
        s_receive_frame.mChannel = 11;  //FIX this - dirty fix for channel handling.


    otRadioPacket.mChannel =  s_receive_frame.mChannel;
    otRadioPacket.mLength = frameLength;
    otRadioPacket.mPsdu = frame;

    otPlatRadioReceiveDone(First_Instace, &otRadioPacket, kThreadError_None);
    if (s_state != kStateDisabled)
    {
        s_state = kStateReceive;
    }

}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void)aInstance;
    aIeeeEui64[0] = 0x18;
    aIeeeEui64[1] = 0xb4;
    aIeeeEui64[2] = 0x30;
    aIeeeEui64[3] = 0x00;
    aIeeeEui64[4] = (NODE_ID >> 24) & 0xff;
    aIeeeEui64[5] = (NODE_ID >> 16) & 0xff;
    aIeeeEui64[6] = (NODE_ID >> 8) & 0xff;
    aIeeeEui64[7] = NODE_ID & 0xff;
}
