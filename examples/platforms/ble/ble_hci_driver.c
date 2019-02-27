/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements the BLE HCI interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "wsf_types.h"

#include "hci_api.h"
#include "hci_cmd.h"
#include "hci_core.h"
#include "hci_drv.h"
#include "hci_mbed_os_adaptation.h"

#include "ble/ble_hci_driver.h"
#include "common/logging.hpp"
#include "utils/code_utils.h"

#include <assert.h>
#include <openthread/error.h>
#include <openthread/platform/ble-hci.h>
#include <openthread/platform/logging.h>

#if OPENTHREAD_ENABLE_TOBLE

#define BYTES_TO_UINT16(n, p)                                       \
    {                                                               \
        n = (uint16_t)((uint16_t)(p)[0] + ((uint16_t)(p)[1] << 8)); \
        p += 2;                                                     \
    }

#define BYTES_TO_UINT8(n, p)   \
    {                          \
        n = (uint8_t)(*(p)++); \
    }

enum
{
    kTxBufferSize    = 258,
    kHciResetRandCnt = 4,
};

enum
{
    kStateIdle,
    kStateRxHeader,
    kStateRxData,
};

static uint8_t  sTxBuffer[kTxBufferSize];
static uint16_t sTxHead     = 0;
static uint16_t sTxLength   = 0;
static uint16_t sSendLength = 0;

static uint16_t bleHciOutput(uint8_t aType, const uint8_t *aBuf, uint16_t aBufLength);
static void     bleHciHandleSendDone(void);
static void     bleHciHandleResetSequence(uint8_t *pMsg);
static void     bleHciResetSequenceDone();

uint16_t hci_mbed_os_drv_write(uint8_t type, uint16_t len, uint8_t *pData)
{
    return bleHciOutput(type, pData, len);
}

void hci_mbed_os_start_reset_sequence(void)
{
    HciResetCmd();
}

void hci_mbed_os_handle_reset_sequence(uint8_t *msg)
{
    bleHciHandleResetSequence(msg);
}

// ----------------------------------
void otPlatBleHciReceived(uint8_t *aBuf, uint8_t aBufLength)
{
    hciTrSerialRxIncoming(aBuf, aBufLength);
}

void otPlatBleHciSendDone(void)
{
    bleHciHandleSendDone();
}

//-----------------------------------
void bleHciEnable(void)
{
    otPlatBleHciEnable();
}

void bleHciDisable(void)
{
    otPlatBleHciDisable();
}

static void bleHciSend(void)
{
    otEXPECT(sSendLength == 0);

    if (sTxLength > sizeof(sTxBuffer) - sTxHead)
    {
        sSendLength = sizeof(sTxBuffer) - sTxHead;
    }
    else
    {
        sSendLength = sTxLength;
    }

    if (sSendLength > 0)
    {
        otPlatBleHciSend((uint8_t *)(sTxBuffer + sTxHead), sSendLength);
    }

exit:
    return;
}

static uint16_t bleHciOutput(uint8_t aType, const uint8_t *aBuf, uint16_t aBufLength)
{
    uint16_t tail;

    otEXPECT_ACTION(sTxLength + sizeof(aType) + aBufLength <= sizeof(sTxBuffer), aBufLength = 0);

    for (int i = 0; i < (int)(sizeof(aType) + aBufLength); i++)
    {
        tail            = (sTxHead + sTxLength) % sizeof(sTxBuffer);
        sTxBuffer[tail] = (i == 0) ? aType : *aBuf++;
        sTxLength++;
    }

    bleHciSend();

exit:
    if (aBufLength == 0)
    {
        otLogNotePlat("Ble Hci send packet failed: HciType = %02x\r\n", aType);
    }

    return aBufLength;
}

static void bleHciHandleSendDone(void)
{
    sTxHead = (sTxHead + sSendLength) % sizeof(sTxBuffer);
    sTxLength -= sSendLength;
    sSendLength = 0;

    bleHciSend();
}

static void bleHciCoreReadMaxDataLen(void)
{
    // if LE Data Packet Length Extensions is supported by Controller and included
    if ((hciCoreCb.leSupFeat & HCI_LE_SUP_FEAT_DATA_LEN_EXT) && (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_DATA_LEN_EXT))
    {
        HciLeReadMaxDataLen();
    }
    else
    {
        HciLeRandCmd();
    }
}

static void bleHciCoreReadResolvingListSize(void)
{
    // if LL Privacy is supported by Controller and included
    if ((hciCoreCb.leSupFeat & HCI_LE_SUP_FEAT_PRIVACY) && (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_PRIVACY))
    {
        HciLeReadResolvingListSize();
    }
    else
    {
        hciCoreCb.resListSize = 0;
        bleHciCoreReadMaxDataLen();
    }
}

static void bleHciResetSequenceDone()
{
    hci_mbed_os_signal_reset_sequence_done();
}

static void bleHciHandleResetSequence(uint8_t *pMsg)
{
    uint16_t       opcode;
    static uint8_t randCnt;

    if (*pMsg == HCI_CMD_CMPL_EVT)
    {
        pMsg += HCI_EVT_HDR_LEN;       // skip HCI event header
        pMsg++;                        // skip Num_HCI_Command_Packets
        BYTES_TO_UINT16(opcode, pMsg); // skip Command_Opcode
        pMsg++;                        // skip Status

        switch (opcode)
        {
        case HCI_OPCODE_RESET:
            randCnt = 0;
            HciSetEventMaskCmd((uint8_t *)hciEventMask);
            break;

        case HCI_OPCODE_SET_EVENT_MASK:
            HciLeSetEventMaskCmd((uint8_t *)hciLeEventMask);
            break;

        case HCI_OPCODE_LE_SET_EVENT_MASK:
            HciSetEventMaskPage2Cmd((uint8_t *)hciEventMaskPage2);
            break;

        case HCI_OPCODE_SET_EVENT_MASK_PAGE2:
            HciReadBdAddrCmd();
            break;

        case HCI_OPCODE_READ_BD_ADDR:
        {
            BdaCpy(hciCoreCb.bdAddr, pMsg);
            HciLeReadBufSizeCmd();
            break;
        }

        case HCI_OPCODE_LE_SET_RAND_ADDR:
            HciLeReadBufSizeCmd();
            break;

        case HCI_OPCODE_LE_READ_BUF_SIZE:
            BYTES_TO_UINT16(hciCoreCb.bufSize, pMsg);
            BYTES_TO_UINT8(hciCoreCb.numBufs, pMsg);

            // initialize ACL buffer accounting
            hciCoreCb.availBufs = hciCoreCb.numBufs;

            HciLeReadSupStatesCmd();
            break;

        case HCI_OPCODE_LE_READ_SUP_STATES:
            memcpy(hciCoreCb.leStates, pMsg, HCI_LE_STATES_LEN);

            HciLeReadWhiteListSizeCmd();
            break;

        case HCI_OPCODE_LE_READ_WHITE_LIST_SIZE:
            BYTES_TO_UINT8(hciCoreCb.whiteListSize, pMsg);

            HciLeReadLocalSupFeatCmd();
            break;

        case HCI_OPCODE_LE_READ_LOCAL_SUP_FEAT:
            BYTES_TO_UINT16(hciCoreCb.leSupFeat, pMsg);

            bleHciCoreReadResolvingListSize();
            break;

        case HCI_OPCODE_LE_READ_RES_LIST_SIZE:
            BYTES_TO_UINT8(hciCoreCb.resListSize, pMsg);

            bleHciCoreReadMaxDataLen();
            break;

        case HCI_OPCODE_LE_READ_MAX_DATA_LEN:
        {
            uint16_t maxTxOctets;
            uint16_t maxTxTime;

            BYTES_TO_UINT16(maxTxOctets, pMsg);
            BYTES_TO_UINT16(maxTxTime, pMsg);

            /* use Controller's maximum supported payload octets and packet duration times
             * for transmission as Host's suggested values for maximum transmission number
             * of payload octets and maximum packet transmission time for new connections.
             */
            HciLeWriteDefDataLen(maxTxOctets, maxTxTime);
        }
        break;
        case HCI_OPCODE_LE_WRITE_DEF_DATA_LEN:
            if (hciCoreCb.extResetSeq)
            {
                (*hciCoreCb.extResetSeq)(pMsg, opcode);
            }
            else
            {
                hciCoreCb.maxAdvDataLen  = 0;
                hciCoreCb.numSupAdvSets  = 0;
                hciCoreCb.perAdvListSize = 0;

                HciLeRandCmd();
            }
            break;

        case HCI_OPCODE_LE_READ_MAX_ADV_DATA_LEN:
        case HCI_OPCODE_LE_READ_NUM_SUP_ADV_SETS:
        case HCI_OPCODE_LE_READ_PER_ADV_LIST_SIZE:
            if (hciCoreCb.extResetSeq)
            {
                (*hciCoreCb.extResetSeq)(pMsg, opcode);
            }
            break;

        case HCI_OPCODE_LE_RAND:
            // check if need to send second rand command
            if (randCnt < (kHciResetRandCnt - 1))
            {
                randCnt++;
                HciLeRandCmd();
            }
            else
            {
                // last command in sequence; set resetting state and call callback
                bleHciResetSequenceDone();
            }
            break;

        default:
            break;
        }
    }
}

#endif // OPENTHREAD_ENABLE_TOBLE
