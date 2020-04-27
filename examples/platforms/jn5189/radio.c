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

/* Openthread configuration */
#include "openthread-core-jn5189-config.h"

/* memcpy */
#include "string.h"

/* uMac, MMAC, Radio */
#include "BbcAndPhyRegs.h"
#include "MMAC.h"
#include "MicroSpecific_arm_sdk2.h"
#include "radio_jn518x.h"

/* Openthread general */
#include <utils/code_utils.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

extern void BOARD_LedDongleToggle(void);

/* Defines */
#define BIT_SET(arg, posn) ((arg) |= (1ULL << (posn)))
#define BIT_CLR(arg, posn) ((arg) &= ~(1ULL << (posn)))
#define BIT_TST(arg, posn) (!!((arg) & (1ULL << (posn))))

#define ALL_FFs_BYTE (0xFF)

#define JN5189_RADIO_MIN_TX_POWER_DBM (-30)
#define JN5189_RADIO_MAX_TX_POWER_DBM (15)
#define JN5189_RADIO_RX_SENSITIVITY_DBM (-100)
#define JN5189_RADIO_DEFAULT_CHANNEL (11)

#define US_PER_SYMBOL (16) /* Duration of a single symbol in [us] */
#define SYMBOLS_TO_US(symbols) ((symbols)*US_PER_SYMBOL)
#define US_TO_MILI_DIVIDER (1000)

#define MAX_FP_ADDRS (10)     /* max number of frame pending children */
#define JN5189_RX_BUFFERS (8) /* max number of RX buffers */

/* check IEEE Std. 802.15.4 - 2015: Table 8-81 - MAC sublayer constants */
#define MAC_TX_ATTEMPTS (4)
#define MAC_TX_CSMA_MIN_BE (3)
#define MAC_TX_CSMA_MAX_BE (5)
#define MAC_TX_CSMA_MAX_BACKOFFS (4)

/* Structures */
typedef struct
{
    uint16_t macAddress;
    uint16_t panId;
} fpNeighShortAddr;

typedef struct
{
    uint32_t u32L;
    uint32_t u32H;
} extMacAddr;

typedef struct
{
    extMacAddr extAddr;
    uint16_t   panId;
} fpNeighExtAddr;

typedef struct
{
    tsRxFrameFormat *buffer[JN5189_RX_BUFFERS];
    uint8_t          head;
    uint8_t          tail;
    bool             isFull;
} rxRingBuffer;

typedef enum
{
    kFcfSize             = sizeof(uint16_t),
    kDsnSize             = sizeof(uint8_t),
    kSecurityControlSize = sizeof(uint8_t),
    kFrameCounterSize    = sizeof(uint32_t),
    kKeyIndexSize        = sizeof(uint8_t),

    kMacFcfLowOffset = 0, /* Offset of FCF first byte inside Mac Hdr */
    kMacFrameDataReq = 4,

    kFcfTypeBeacon       = 0,
    kFcfTypeMacData      = 1,
    kFcfTypeAck          = 2,
    kFcfTypeMacCommand   = 3,
    kFcfMacFrameTypeMask = 7 << 0,

    kFcfAckRequest        = 1 << 5,
    kFcfPanidCompression  = 1 << 6,
    kFcfSeqNbSuppresssion = 1 << 8,
    kFcfDstAddrNone       = 0 << 10,
    kFcfDstAddrShort      = 2 << 10,
    kFcfDstAddrExt        = 3 << 10,
    kFcfDstAddrMask       = 3 << 10,
    kFcfSrcAddrNone       = 0 << 14,
    kFcfSrcAddrShort      = 2 << 14,
    kFcfSrcAddrExt        = 3 << 14,
    kFcfSrcAddrMask       = 3 << 14,

    kSecLevelMask            = 7 << 0,
    kFrameCounterSuppression = 1 << 5,

    kKeyIdMode0    = 0 << 3,
    kKeyIdMode1    = 1 << 3,
    kKeyIdMode2    = 2 << 3,
    kKeyIdMode3    = 3 << 3,
    kKeyIdModeMask = 3 << 3,

    kKeySourceSizeMode0 = 0,
    kKeySourceSizeMode1 = 0,
    kKeySourceSizeMode2 = 4,
    kKeySourceSizeMode3 = 8,
} macHdr;

typedef enum
{
    macToOtFrame, /* RX */
    otToMacFrame, /* TX */
} frameConversionType;

/* Private functions declaration */
static void    JN5189ISR(uint32_t u32IntBitmap);
static void    JN5189ProcessMacHeader(tsRxFrameFormat *aRxFrame);
static void    JN5189ProcessRxFrames(otInstance *aInstance);
static void    JN5189ProcessTxFrame(otInstance *aInstance);
static bool    JN5189CheckIfFpRequired(tsRxFrameFormat *aRxFrame);
static bool_t  JN5189IsDataReq(tsRxFrameFormat *aRxFrame);
static otError JN5189FrameConversion(tsRxFrameFormat *   aMacFormatFrame,
                                     otRadioFrame *      aOtFrame,
                                     frameConversionType convType);
static void    JN5189Copy(uint8_t *aFieldValue, uint8_t **aPsdu, uint8_t copySize, frameConversionType convType);
static void    JN5189ResetRxRingBuffer(rxRingBuffer *aRxRing);
static void    JN5189PushRxRingBuffer(rxRingBuffer *aRxRing, tsRxFrameFormat *aRxFrame);
static tsRxFrameFormat *JN5189PopRxRingBuffer(rxRingBuffer *aRxRing);
static bool             JN5189IsEmptyRxRingBuffer(rxRingBuffer *aRxRing);
static tsRxFrameFormat *JN5189GetFrame(tsRxFrameFormat *aRxFrame, uint8_t *aRxFrameIndex);
static void             JN5189EnableReceive(bool_t isNewFrameNeeded);
static void             JN5189RestartRx();

/* Private variables declaration */
static otRadioState sState = OT_RADIO_STATE_DISABLED;
static otInstance * sInstance;    /* Saved OT Instance */
static int8_t       sTxPwrLevel;  /* Default power is 0 dBm */
static uint8_t      sChannel = 0; /* Default channel - must be invalid so it
                                     updates the first time it is set */
static bool_t    sIsFpEnabled;    /* Frame Pending enabled? */
static uint16_t  sPanId;          /* PAN ID currently in use */
static uint16_t  sShortAddress;
static tsExtAddr sExtAddress;
static uint64_t  sCustomExtAddr = 0;

static fpNeighShortAddr sFpShortAddr[MAX_FP_ADDRS]; /* Frame Pending short addresses array */
static uint16_t         sFpShortAddrMask;           /* Mask - sFpShortAddr valid entries */

static fpNeighExtAddr sFpExtAddr[MAX_FP_ADDRS]; /* Frame Pending extended addresses array */
static uint16_t       sFpExtAddrMask;           /* Mask - sFpExtAddr is valid */

static rxRingBuffer     sRxRing;                       /* Receive Ring Buffer */
static tsRxFrameFormat  sRxFrame[JN5189_RX_BUFFERS];   /* RX Buffers */
static tsRxFrameFormat *sRxFrameInProcess;             /* RX Frame currently in processing */
static bool_t           sIsRxDisabled;                 /* TRUE if RX was disabled due to no RX bufs */
static uint8_t          sRxFrameIndex;                 /* Index tracking the sRxFrame array */
static teRxOption       sRxOpt = E_MMAC_RX_START_NOW | /* RX Options */
                           E_MMAC_RX_ALIGN_NORMAL | E_MMAC_RX_USE_AUTO_ACK | E_MMAC_RX_NO_MALFORMED |
                           E_MMAC_RX_NO_FCS_ERROR | E_MMAC_RX_ADDRESS_MATCH;

tsRxFrameFormat        sTxMacFrame;                      /* TX Frame */
static tsRxFrameFormat sRxAckFrame;                      /* Frame used for keeping the ACK */
static otRadioFrame    sRxOtFrame;                       /* Used for TX/RX frame conversion */
static uint8           sRxData[OT_RADIO_FRAME_MAX_SIZE]; /* mPsdu buffer for sRxOtFrame */

static bool         sRadioInitForLp    = FALSE;
static bool         sPromiscuousEnable = FALSE;
static bool         sTxDone;                          /* TRUE if a TX frame was sent into the air */
static otError      sTxStatus;                        /* Status of the latest TX operation */
static otRadioFrame sTxOtFrame;                       /* OT TX Frame to be send */
static uint8_t      sTxData[OT_RADIO_FRAME_MAX_SIZE]; /* mPsdu buffer for sTxOtFrame */

/* Stub functions for controlling low power mode */
WEAK void App_AllowDeviceToSleep();
WEAK void App_DisallowDeviceToSleep();
/* Stub functions for controlling LEDs on OT RCP USB dongle */
WEAK void BOARD_LedDongleToggle();

/**
 * Stub function used for controlling low power mode
 *
 */
WEAK void App_AllowDeviceToSleep()
{
}

/**
 * Stub function used for controlling low power mode
 *
 */
WEAK void App_DisallowDeviceToSleep()
{
}

/**
 * Stub functions for controlling LEDs on OT RCP USB dongle
 *
 */
WEAK void BOARD_LedDongleToggle()
{
}

void App_SetCustomEui64(uint8_t *aIeeeEui64)
{
    memcpy((uint8_t *)&sCustomExtAddr, aIeeeEui64, sizeof(sCustomExtAddr));
}

void JN5189RadioInit(void)
{
    /* RX initialization */
    memset(sRxFrame, 0, sizeof(tsRxFrameFormat) * JN5189_RX_BUFFERS);
    sRxFrameIndex = 0;

    /* TX initialization */
    sTxOtFrame.mPsdu = sTxData;
    sRxOtFrame.mPsdu = sRxData;
}

void JN5189RadioProcess(otInstance *aInstance)
{
    JN5189ProcessRxFrames(aInstance);
    JN5189ProcessTxFrame(aInstance);
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (0 == sCustomExtAddr)
    {
        tsExtAddr euiAddr;
        vMMAC_GetMacAddress(&euiAddr);

        memcpy(aIeeeEui64, &euiAddr.u32L, sizeof(uint32_t));
        memcpy(aIeeeEui64 + sizeof(uint32_t), &euiAddr.u32H, sizeof(uint32_t));
    }
    else
    {
        memcpy(aIeeeEui64, (uint8_t *)&sCustomExtAddr, sizeof(sCustomExtAddr));
    }
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);

    sPanId = aPanId;
    vMMAC_SetRxPanId(aPanId);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aExtAddress)
    {
        memcpy(&sExtAddress.u32L, aExtAddress->m8, sizeof(uint32_t));
        memcpy(&sExtAddress.u32H, aExtAddress->m8 + sizeof(uint32_t), sizeof(uint32_t));
        vMMAC_SetRxExtendedAddr(&sExtAddress);
    }
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    sShortAddress = aShortAddress;
    vMMAC_SetRxShortAddr(aShortAddress);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    JN5189ResetRxRingBuffer(&sRxRing);
    sRxFrameIndex = 0;
    vMMAC_Enable();
    vMMAC_EnableInterrupts(JN5189ISR);
    vMMAC_ConfigureInterruptSources(E_MMAC_INT_TX_COMPLETE | E_MMAC_INT_RX_HEADER | E_MMAC_INT_RX_COMPLETE);
    vMMAC_ConfigureRadio();
    vMMAC_SetTxParameters(MAC_TX_ATTEMPTS, MAC_TX_CSMA_MIN_BE, MAC_TX_CSMA_MAX_BE, MAC_TX_CSMA_MAX_BACKOFFS);

    if (sRadioInitForLp)
    {
        /* Re-set modem settings after low power exit */
        vMMAC_SetChannelAndPower(sChannel, sTxPwrLevel);
        vMMAC_SetRxExtendedAddr(&sExtAddress);
        vMMAC_SetRxPanId(sPanId);
        vMMAC_SetRxShortAddr(sShortAddress);
    }

    sTxOtFrame.mLength = 0;
    sRxOtFrame.mLength = 0;

    sInstance = aInstance;
    sState    = OT_RADIO_STATE_SLEEP;

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_INVALID_STATE;

    otEXPECT(otPlatRadioIsEnabled(aInstance));

    JN5189ResetRxRingBuffer(&sRxRing);
    sRxFrameIndex = 0;
    vMMAC_Disable();
    sState = OT_RADIO_STATE_DISABLED;
    error  = OT_ERROR_NONE;

exit:
    return error;
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

    /* The radio has been init and configuration should be restored in otPlatRadioEnable when
       exiting low power */
    sRadioInitForLp = TRUE;

    sState = OT_RADIO_STATE_SLEEP;
    vMMAC_RadioToOffAndWait();
    App_AllowDeviceToSleep();

exit:
    return status;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError      error            = OT_ERROR_NONE;
    bool_t       isNewFrameNeeded = TRUE;
    otRadioState tempState        = sState;

    otEXPECT_ACTION(((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED)),
                    error = OT_ERROR_INVALID_STATE);

    App_DisallowDeviceToSleep();

    /* Check if the channel needs to be changed */
    if (sChannel != aChannel)
    {
        sChannel = aChannel;

        /* The state is set to sleep to prevent a lockup caused by an RX interrup firing during
         * the radio off command called inside set channel and power */
        sState = OT_RADIO_STATE_SLEEP;
        vMMAC_SetChannelAndPower(sChannel, sTxPwrLevel);
        sState = tempState;
    }

    if (OT_RADIO_STATE_RECEIVE != sState)
    {
        sState = OT_RADIO_STATE_RECEIVE;
    }
    else
    {
        /* this might happen when the channel is switched
         * in the middle of a receive operation */
        isNewFrameNeeded = FALSE;
    }
    JN5189EnableReceive(isNewFrameNeeded);

exit:
    return error;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsFpEnabled = aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NO_BUFS;
    uint8_t idx   = 0;

    for (; idx < MAX_FP_ADDRS; idx++)
    {
        if (!BIT_TST(sFpShortAddrMask, idx))
        {
            sFpShortAddr[idx].panId      = sPanId;
            sFpShortAddr[idx].macAddress = aShortAddress;
            BIT_SET(sFpShortAddrMask, idx);
            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NO_BUFS;
    uint8_t idx   = 0;

    for (; idx < MAX_FP_ADDRS; idx++)
    {
        if (!BIT_TST(sFpExtAddrMask, idx))
        {
            sFpExtAddr[idx].panId = sPanId;
            memcpy(&sFpExtAddr[idx].extAddr.u32L, aExtAddress->m8, sizeof(uint32_t));
            memcpy(&sFpExtAddr[idx].extAddr.u32H, aExtAddress->m8 + sizeof(uint32_t), sizeof(uint32_t));
            BIT_SET(sFpExtAddrMask, idx);
            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NO_ADDRESS;
    uint8_t idx   = 0;

    for (; idx < MAX_FP_ADDRS; idx++)
    {
        if (BIT_TST(sFpShortAddrMask, idx) && (sFpShortAddr[idx].macAddress == aShortAddress))
        {
            BIT_CLR(sFpShortAddrMask, idx);
            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NO_ADDRESS;
    uint8_t idx   = 0;

    for (; idx < MAX_FP_ADDRS; idx++)
    {
        if (BIT_TST(sFpExtAddrMask, idx) && !memcmp(&sFpExtAddr[idx].extAddr.u32L, aExtAddress->m8, sizeof(uint32_t)) &&
            !memcmp(&sFpExtAddr[idx].extAddr.u32H, aExtAddress->m8 + sizeof(uint32_t), sizeof(uint32_t)))
        {
            BIT_CLR(sFpExtAddrMask, idx);
            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sFpShortAddrMask = 0;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sFpExtAddrMask = 0;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTxOtFrame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError    error    = OT_ERROR_NONE;
    teTxOption eOptions = E_MMAC_TX_START_NOW | E_MMAC_TX_USE_AUTO_ACK;

    otEXPECT_ACTION(OT_RADIO_STATE_RECEIVE == sState, error = OT_ERROR_INVALID_STATE);

    /* go to TX state */
    sState    = OT_RADIO_STATE_TRANSMIT;
    sTxStatus = OT_ERROR_NONE;

    /* set tx channel */
    if (sChannel != aFrame->mChannel)
    {
        vMMAC_SetChannelAndPower(aFrame->mChannel, sTxPwrLevel);
    }

    if (aFrame->mInfo.mTxInfo.mCsmaCaEnabled)
    {
        eOptions |= E_MMAC_TX_USE_CCA;
    }

    JN5189FrameConversion(&sTxMacFrame, aFrame, otToMacFrame);

    /* stop rx is handled by uMac tx function */
    vMMAC_StartMacTransmit(&sTxMacFrame.sFrameBody, eOptions);

    /* Set RX buffer pointer for ACK */
    vREG_BbcWrite(REG_BBC_RXBUFAD, (uint32)&sRxAckFrame.sFrameBody);

    otPlatRadioTxStarted(aInstance, aFrame);

exit:
    return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    int8_t  rssidBm       = 127;
    int16_t rssiValSigned = 0;
    bool_t  stateChanged  = FALSE;

    /* in RCP designs, the RSSI function is called while the radio is in
     * OT_RADIO_STATE_RECEIVE. Turn off the radio before reading RSSI,
     * otherwise we may end up waiting until a packet is received
     * (in i16Radio_JN518x_GetRSSI, while loop)
     */

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        sState = OT_RADIO_STATE_SLEEP;
        vMMAC_RadioToOffAndWait();
        stateChanged = TRUE;
    }

    rssiValSigned = i16Radio_Jn518x_GetRSSI(0, FALSE, NULL);

    if (stateChanged)
    {
        sState = OT_RADIO_STATE_RECEIVE;
        JN5189EnableReceive(TRUE);
    }

    rssiValSigned = i16Radio_Jn518x_BoundRssiValue(rssiValSigned);

    /* RSSI reported by radio is in 1/4 dBm step,
     * meaning values are 4 times larger than real dBm value.
     */
    rssidBm = (int8_t)(rssiValSigned >> 2);

    return rssidBm;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPromiscuousEnable;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sPromiscuousEnable != aEnable)
    {
        sPromiscuousEnable = aEnable;

        if (aEnable)
        {
            sRxOpt &= ~E_MMAC_RX_ADDRESS_MATCH;
        }
        else
        {
            sRxOpt |= E_MMAC_RX_ADDRESS_MATCH;
        }
    }
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError status = OT_ERROR_NOT_IMPLEMENTED;

    return status;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);

    *aPower = i8Radio_GetTxPowerLevel_dBm();
    return OT_ERROR_NONE;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    otRadioState tempState = sState;
    sState                 = OT_RADIO_STATE_SLEEP;

    /* trim the values to the radio capabilities */
    if (aPower < JN5189_RADIO_MIN_TX_POWER_DBM)
    {
        aPower = JN5189_RADIO_MIN_TX_POWER_DBM;
    }
    else if (aPower > JN5189_RADIO_MAX_TX_POWER_DBM)
    {
        aPower = JN5189_RADIO_MAX_TX_POWER_DBM;
    }

    /* save for later use */
    sTxPwrLevel = aPower;

    /* The state is set to sleep to prevent a lockup caused by an RX interrup firing during
     * the radio off command called inside set channel and power */
    if (0 != sChannel)
    {
        vMMAC_SetChannelAndPower(sChannel, aPower);
    }
    else
    {
        /* if the channel has not yet been initialized use JN5189_RADIO_DEFAULT_CHANNEL as default */
        vMMAC_SetChannelAndPower(JN5189_RADIO_DEFAULT_CHANNEL, aPower);
    }
    sState = tempState;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return JN5189_RADIO_RX_SENSITIVITY_DBM;
}

/**
 * Interrupt service routine (e.g.: TX/RX/MAC HDR received)
 *
 * @param[in] u32IntBitmap  Bitmap telling which interrupt fired
 *
 */
static void JN5189ISR(uint32_t u32IntBitmap)
{
    tsRxFrameFormat *pRxFrame = NULL;

    switch (sState)
    {
    case OT_RADIO_STATE_RECEIVE:

        /* no rx errors */
        if (0 == u32MMAC_GetRxErrors())
        {
            if (u32IntBitmap & E_MMAC_INT_RX_HEADER)
            {
                /* go back one index from current frame index */
                pRxFrame = &sRxFrame[(sRxFrameIndex + JN5189_RX_BUFFERS - 1) % JN5189_RX_BUFFERS];

                /* FP processing first */
                JN5189ProcessMacHeader(pRxFrame);

                /* RX interrupt fired so it's safe to consume the frame */
                JN5189PushRxRingBuffer(&sRxRing, pRxFrame);

                if (0 == (pRxFrame->sFrameBody.u16FCF & kFcfAckRequest))
                {
                    JN5189EnableReceive(TRUE);
                }
            }
            else if (u32IntBitmap & E_MMAC_INT_RX_COMPLETE)
            {
                JN5189EnableReceive(TRUE);
            }
        }
        else
        {
            /* restart RX and keep same buffer as data received contains errors */
            JN5189EnableReceive(FALSE);
        }

        BOARD_LedDongleToggle();
        break;
    case OT_RADIO_STATE_TRANSMIT:

        if (u32IntBitmap & E_MMAC_INT_TX_COMPLETE)
        {
            uint32_t txErrors = u32MMAC_GetTxErrors();
            sTxDone           = TRUE;

            if (txErrors & REG_BBC_TXSTAT_CCAE_MASK)
            {
                sTxStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            }
            else if (txErrors & REG_BBC_TXSTAT_ACKE_MASK)
            {
                sTxStatus = OT_ERROR_NO_ACK;
            }
            else if (txErrors & REG_BBC_TXSTAT_OOTE_MASK)
            {
                sTxStatus = OT_ERROR_ABORT;
            }
            else if ((txErrors & REG_BBC_TXSTAT_TXPCTO_MASK) || (txErrors & REG_BBC_TXSTAT_TXTO_MASK))
            {
                /* The JN518x has a TXTO timeout that we are using to catch and cope with the curious
                hang-up issue */
                vMMAC_AbortRadio();

                /* Describe failure as a CCA failure for onward processing */
                sTxStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            }

            /* go to RX and restore channel */
            if (sChannel != sTxOtFrame.mChannel)
            {
                vMMAC_SetChannelAndPower(sChannel, sTxPwrLevel);
            }

            BOARD_LedDongleToggle();
            sState = OT_RADIO_STATE_RECEIVE;
            JN5189EnableReceive(TRUE);
        }
        break;

    default:
        break;
    }
}
/**
 * Process the MAC Header of the latest received packet
 * We are in interrupt context - we need to compute the FP
 * while the hardware generates the ACK.
 *
 * @param[in] aRxFrame     Pointer to the latest received MAC packet
 *
 */
static void JN5189ProcessMacHeader(tsRxFrameFormat *aRxFrame)
{
    /* check if frame pending processing is required */
    if (aRxFrame && sIsFpEnabled)
    {
        /* Intra-PAN bit set? */
        if ((kFcfPanidCompression & aRxFrame->sFrameBody.u16FCF) &&
            (kFcfDstAddrNone != (aRxFrame->sFrameBody.u16FCF & kFcfDstAddrMask)))
        {
            /* Read destination PAN ID into source PAN ID: they are the same */
            aRxFrame->sFrameBody.u16SrcPAN = aRxFrame->sFrameBody.u16DestPAN;
        }

        if (JN5189IsDataReq(aRxFrame))
        {
            vREG_BbcWrite(REG_BBC_TXPEND, JN5189CheckIfFpRequired(aRxFrame));
        }
        else
        {
            /* Make sure this is set to 0 when we are not dealing with a data request */
            aRxFrame->sFrameBody.u16Unused = 0;
        }
    }
}

/**
 * Check if aRxFrame is a MAC Data Request Command
 *
 * @param[in] aRxFrame     Pointer to the latest received MAC packet
 *
 * @return TRUE 	aRxFrame is a MAC Data Request Frame
 * @return FALSE	aRxFrame is not a MAC Data Request Frame
 */
static bool_t JN5189IsDataReq(tsRxFrameFormat *aRxFrame)
{
    bool_t  isDataReq = FALSE;
    uint8_t offset    = 0;

    if (kFcfTypeMacCommand == (aRxFrame->sFrameBody.u16FCF & kFcfMacFrameTypeMask))
    {
        uint8_t secControlField = aRxFrame->sFrameBody.uPayload.au8Byte[0];

        if (secControlField & kSecLevelMask)
        {
            offset += kSecurityControlSize;
        }

        if (!(secControlField & kFrameCounterSuppression))
        {
            offset += kFrameCounterSize;
        }

        switch (secControlField & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            offset += kKeySourceSizeMode0;
            break;

        case kKeyIdMode1:
            offset += kKeySourceSizeMode1 + kKeyIndexSize;
            break;

        case kKeyIdMode2:
            offset += kKeySourceSizeMode2 + kKeyIndexSize;
            break;

        case kKeyIdMode3:
            offset += kKeySourceSizeMode3 + kKeyIndexSize;
            break;
        }

        if (kMacFrameDataReq == aRxFrame->sFrameBody.uPayload.au8Byte[offset])
        {
            isDataReq = TRUE;
        }
    }

    return isDataReq;
}

/**
 * Check if Frame Pending was requested by aPsRxFrame
 * We are in interrupt context.
 *
 * @param[in] aRxFrame  Pointer to a Data Request Frame
 *
 * @return    TRUE 	    Frame Pending bit should be set in the reply
 * @return    FALSE	    Frame Pending bit shouldn't be set in the reply
 *
 */
static bool JN5189CheckIfFpRequired(tsRxFrameFormat *aRxFrame)
{
    bool     isFpRequired = FALSE;
    uint16_t panId        = aRxFrame->sFrameBody.u16SrcPAN;
    uint8_t  idx          = 0;

    if (kFcfSrcAddrShort == (aRxFrame->sFrameBody.u16FCF & kFcfSrcAddrMask))
    {
        uint16_t shortAddr = aRxFrame->sFrameBody.uSrcAddr.u16Short;

        for (idx = 0; idx < MAX_FP_ADDRS; idx++)
        {
            if (BIT_TST(sFpShortAddrMask, idx) && (sFpShortAddr[idx].macAddress == shortAddr) &&
                sFpShortAddr[idx].panId == panId)
            {
                isFpRequired = TRUE;
                break;
            }
        }
    }
    else
    {
        for (idx = 0; idx < MAX_FP_ADDRS; idx++)
        {
            if (BIT_TST(sFpExtAddrMask, idx) &&
                (sFpExtAddr[idx].extAddr.u32L == aRxFrame->sFrameBody.uSrcAddr.sExt.u32L) &&
                (sFpExtAddr[idx].extAddr.u32H == aRxFrame->sFrameBody.uSrcAddr.sExt.u32H) &&
                sFpExtAddr[idx].panId == panId)
            {
                isFpRequired = TRUE;
                break;
            }
        }
    }
    /* use the unsued filed to store if the frame was ack'ed with FP and report this back to OT stack */
    aRxFrame->sFrameBody.u16Unused = isFpRequired;

    return isFpRequired;
}

/**
 * Process RX frames in process context and call the upper layer call-backs
 *
 * @param[in] aInstance  Pointer to OT instance
 */
static void JN5189ProcessRxFrames(otInstance *aInstance)
{
    tsRxFrameFormat *pRxMacFormatFrame = NULL;
    uint32_t         savedInterrupts;

    while ((pRxMacFormatFrame = JN5189PopRxRingBuffer(&sRxRing)) != NULL)
    {
        if (OT_ERROR_NONE == JN5189FrameConversion(pRxMacFormatFrame, &sRxOtFrame, macToOtFrame))
        {
            otPlatRadioReceiveDone(aInstance, &sRxOtFrame, OT_ERROR_NONE);
        }
        else
        {
            otPlatRadioReceiveDone(aInstance, NULL, OT_ERROR_ABORT);
        }
        memset(pRxMacFormatFrame, 0, sizeof(tsRxFrameFormat));

        MICRO_DISABLE_AND_SAVE_INTERRUPTS(savedInterrupts);
        sRxFrameInProcess = NULL;
        if (sIsRxDisabled)
        {
            JN5189EnableReceive(TRUE);
        }
        MICRO_RESTORE_INTERRUPTS(savedInterrupts);
    }
}

/**
 * Process TX frame in process context and call the upper layer call-backs
 *
 * @param[in] aInstance  Pointer to OT instance
 */
static void JN5189ProcessTxFrame(otInstance *aInstance)
{
    if (sTxDone)
    {
        sTxDone = FALSE;
        if ((sTxOtFrame.mPsdu[kMacFcfLowOffset] & kFcfAckRequest) && (OT_ERROR_NONE == sTxStatus))
        {
            JN5189FrameConversion(&sRxAckFrame, &sRxOtFrame, macToOtFrame);
            otPlatRadioTxDone(aInstance, &sTxOtFrame, &sRxOtFrame, sTxStatus);
        }
        else
        {
            otPlatRadioTxDone(aInstance, &sTxOtFrame, NULL, sTxStatus);
        }
    }
}

/**
 * aMacFormatFrame <-> aOtFrame bidirectional conversion
 *
 * @param[in] aMacFrameFormat  Pointer to a MAC Format frame
 * @param[in] aOtFrame         Pointer to OpenThread Frame
 * @param[in] convType         Conversion direction
 *
 * @return    OT_ERROR_NONE      No conversion error
 * @return    OT_ERROR_PARSE     Conversion failed due to parsing error
 */
static otError JN5189FrameConversion(tsRxFrameFormat *   aMacFormatFrame,
                                     otRadioFrame *      aOtFrame,
                                     frameConversionType convType)
{
    tsMacFrame *pMacFrame         = &aMacFormatFrame->sFrameBody;
    uint8_t *   pSavedStartRxPSDU = aOtFrame->mPsdu;
    uint8_t *   pPsdu             = aOtFrame->mPsdu;
    uint16_t    aFcf              = 0;
    otError     error             = OT_ERROR_NONE;

    /* frame control field */
    JN5189Copy((uint8_t *)&pMacFrame->u16FCF, &pPsdu, kFcfSize, convType);
    aFcf = pMacFrame->u16FCF;

    /* sequence number */
    if (0 == (aFcf & kFcfSeqNbSuppresssion))
    {
        JN5189Copy(&pMacFrame->u8SequenceNum, &pPsdu, kDsnSize, convType);
    }

    /* destination Pan Id + address */
    switch (aFcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrNone:
        break;

    case kFcfDstAddrShort:
        JN5189Copy((uint8_t *)&pMacFrame->u16DestPAN, &pPsdu, sizeof(otPanId), convType);
        JN5189Copy((uint8_t *)&pMacFrame->uDestAddr.u16Short, &pPsdu, sizeof(otShortAddress), convType);
        break;

    case kFcfDstAddrExt:
        JN5189Copy((uint8_t *)&pMacFrame->u16DestPAN, &pPsdu, sizeof(otPanId), convType);
        JN5189Copy((uint8_t *)&pMacFrame->uDestAddr.sExt.u32L, &pPsdu, sizeof(uint32_t), convType);
        JN5189Copy((uint8_t *)&pMacFrame->uDestAddr.sExt.u32H, &pPsdu, sizeof(uint32_t), convType);
        break;

    default:
        error = OT_ERROR_PARSE;
        otEXPECT(false);
    }

    /* Source Pan Id */
    if ((aFcf & kFcfSrcAddrMask) != kFcfSrcAddrNone && (aFcf & kFcfPanidCompression) == 0)
    {
        JN5189Copy((uint8_t *)&pMacFrame->u16SrcPAN, &pPsdu, sizeof(otPanId), convType);
    }

    /* Source Address */
    switch (aFcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        JN5189Copy((uint8_t *)&pMacFrame->uSrcAddr.u16Short, &pPsdu, sizeof(otShortAddress), convType);
        break;

    case kFcfSrcAddrExt:
        JN5189Copy((uint8_t *)&pMacFrame->uSrcAddr.sExt.u32L, &pPsdu, sizeof(uint32_t), convType);
        JN5189Copy((uint8_t *)&pMacFrame->uSrcAddr.sExt.u32H, &pPsdu, sizeof(uint32_t), convType);
        break;

    default:
        error = OT_ERROR_PARSE;
        otEXPECT(false);
    }

    if (convType == otToMacFrame)
    {
        pMacFrame->u8PayloadLength = aOtFrame->mLength - (pPsdu - pSavedStartRxPSDU) - kFcfSize;
    }
    else
    {
        aOtFrame->mInfo.mRxInfo.mAckedWithFramePending = (bool)aMacFormatFrame->sFrameBody.u16Unused;
        aOtFrame->mInfo.mRxInfo.mLqi                   = aMacFormatFrame->u8LinkQuality;
        aOtFrame->mInfo.mRxInfo.mRssi                  = i8Radio_Jn518x_GetLastPacketRSSI();
        aOtFrame->mChannel                             = sChannel;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#error Time sync requires the timestamp of SFD rather than that of rx done!
#else
        if (otPlatRadioGetPromiscuous(sInstance))
#endif
        {
            aOtFrame->mInfo.mRxInfo.mTimestamp = otPlatAlarmMilliGetNow() * 1000;
        }

        aOtFrame->mLength = pPsdu + (pMacFrame->u8PayloadLength) - pSavedStartRxPSDU + sizeof(pMacFrame->u16FCS);
    }
    JN5189Copy((uint8_t *)&pMacFrame->uPayload, &pPsdu, pMacFrame->u8PayloadLength, convType);

exit:
    return error;
}

/**
 * Helper function for aMacFrame <-> aOtFrame bidirectional conversion
 * Copies from/to PSDU to/from specific field
 *
 * @param[in] aFieldValue     Pointer to field Value which needs to be copied/filled
 * @param[in] aPsdu           Pointer to PSDU part which needs to be copied/filled
 * @param[in] copySize        Bytes copied
 * @param[in] convType        Conversion Type
 *
 * @return    OT_ERROR_NONE      No conversion error
 * @return    OT_ERROR_PARSE     Conversion failed due to parsing error
 */
static void JN5189Copy(uint8_t *aFieldValue, uint8_t **aPsdu, uint8_t copySize, frameConversionType convType)
{
    if (convType == macToOtFrame)
    {
        memcpy(*aPsdu, aFieldValue, copySize);
    }
    else
    {
        memcpy(aFieldValue, *aPsdu, copySize);
    }

    (*aPsdu) += copySize;
}

/**
 * Function used to init/reset an RX Ring Buffer
 *
 * @param[in] aRxRing         Pointer to an RX Ring Buffer
 */
static void JN5189ResetRxRingBuffer(rxRingBuffer *aRxRing)
{
    aRxRing->head   = 0;
    aRxRing->tail   = 0;
    aRxRing->isFull = FALSE;
}

/**
 * Function used to push the address of a received frame to the RX Ring buffer.
 * In case the ring buffer is full, the oldest address is overwritten.
 *
 * @param[in] aRxRing             Pointer to the RX Ring Buffer
 * @param[in] rxFrame             The address for a received frame
 */
static void JN5189PushRxRingBuffer(rxRingBuffer *aRxRing, tsRxFrameFormat *aRxFrame)
{
    aRxRing->buffer[aRxRing->head] = aRxFrame;
    if (aRxRing->isFull)
    {
        aRxRing->tail = (aRxRing->tail + 1) % JN5189_RX_BUFFERS;
    }

    aRxRing->head   = (aRxRing->head + 1) % JN5189_RX_BUFFERS;
    aRxRing->isFull = (aRxRing->head == aRxRing->tail);
}

/**
 * Function used to pop the address of a received frame from the RX Ring buffer
 * Process Context: the consumer will pop frames with the interrupts disabled
 *                  to make sure the interrupt context(ISR) doesn't push in
 *                  the middle of a pop.
 *
 * @param[in] aRxRing           Pointer to the RX Ring Buffer
 *
 * @return    tsRxFrameFormat   Pointer to a received frame
 * @return    NULL              In case the RX Ring buffer is empty
 */
static tsRxFrameFormat *JN5189PopRxRingBuffer(rxRingBuffer *aRxRing)
{
    tsRxFrameFormat *rxFrame = NULL;
    uint32_t         savedInterrupts;

    MICRO_DISABLE_AND_SAVE_INTERRUPTS(savedInterrupts);
    if (!JN5189IsEmptyRxRingBuffer(aRxRing))
    {
        rxFrame         = aRxRing->buffer[aRxRing->tail];
        aRxRing->isFull = FALSE;
        aRxRing->tail   = (aRxRing->tail + 1) % JN5189_RX_BUFFERS;
    }
    MICRO_RESTORE_INTERRUPTS(savedInterrupts);

    return rxFrame;
}

/**
 * Function used to check if an RX Ring buffer is empty
 *
 * @param[in] aRxRing           Pointer to the RX Ring Buffer
 *
 * @return    TRUE              RX Ring Buffer is not empty
 * @return    FALSE             RX Ring Buffer is empty
 */
static bool JN5189IsEmptyRxRingBuffer(rxRingBuffer *aRxRing)
{
    return (!aRxRing->isFull && (aRxRing->head == aRxRing->tail));
}

/**
 * Function used to get the next frame from aRxFrame pointed by aRxFrameIndex.
 *
 * This is the address where the BBC should DMA a received frame. Once the
 * reception is complete (the RX interrupt fires) this address is added to the
 * ring buffer and the frame can be consumed by the process context.
 *
 * @param[in] aRxFrame            Pointer to an array of tsRxFrameFormat
 * @param[in] aRxFrameIndex       Pointer to the current index in aRxFrame array
 *
 * @return    tsRxFrameFormat     Pointer to a tsRxFrameFormat
 */
static tsRxFrameFormat *JN5189GetFrame(tsRxFrameFormat *aRxFrame, uint8_t *aRxFrameIndex)
{
    tsRxFrameFormat *frame = NULL;

    frame = &aRxFrame[*aRxFrameIndex];
    if (frame != sRxFrameInProcess)
    {
        *aRxFrameIndex = (*aRxFrameIndex + 1) % JN5189_RX_BUFFERS;
    }
    else
    {
        /* this can happen only if the RX buffer is full and the
         * process context is interrupted right in the middle of
         * starting to process a frame. In this case, wait for
         * the process context to finish the processing then
         * re-enable RX
         */

        sIsRxDisabled = TRUE;
        frame         = NULL;
    }

    return frame;
}

/**
 * Function used to enable the receiving of a frame
 *
 * @param[in] isNewFrameNeeded FALSE in case the current RX buffer can be
 *                             used for a new RX operation.
 *
 */
static void JN5189EnableReceive(bool_t isNewFrameNeeded)
{
    tsRxFrameFormat *pRxFrame = NULL;

    if (isNewFrameNeeded)
    {
        if ((pRxFrame = JN5189GetFrame(sRxFrame, &sRxFrameIndex)) != NULL)
        {
            vREG_BbcWrite(REG_BBC_RXBUFAD, (uint32)(&pRxFrame->sFrameBody));
            JN5189RestartRx();
        }
    }
    else
    {
        JN5189RestartRx();
    }
}

/**
 * Function used for MMAC-RX Restart
 *
 */
static void JN5189RestartRx()
{
    vREG_BbcWrite(REG_BBC_RXPROM, ((uint32)sRxOpt >> 8) & ALL_FFs_BYTE);
    vMMAC_RxCtlUpdate(((uint32)sRxOpt) & ALL_FFs_BYTE);
}
