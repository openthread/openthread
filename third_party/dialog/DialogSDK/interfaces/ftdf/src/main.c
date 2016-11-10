/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main FTDF functions
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************
 */

#include <ftdf.h>
#include <string.h>

#include "internal.h"
#include "regmap.h"
#include "ftdfInfo.h"

void FTDF_getReleaseInfo(char **lmacRelName, char **lmacBuildTime, char **umacRelName, char **umacBuildTime)
{
    static char        lrelName[ 16 ];
    static char        lbldTime[ 16 ];
    static char        urelName[ 16 ];
    static char        ubldTime[ 16 ];
    uint8_t            i;

    volatile uint32_t *ptr    = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_REL_NAME);
    uint32_t          *chrPtr = (uint32_t *)lrelName;

    for (i = 0; i < FTDF_ON_OFF_REGMAP_REL_NAME_NUM; i++)
    {
        *chrPtr++ = *ptr++;
    }

    ptr    = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_BUILDTIME);
    chrPtr = (uint32_t *)lbldTime;

    for (i = 0; i < FTDF_ON_OFF_REGMAP_BUILDTIME_NUM; i++)
    {
        *chrPtr++ = *ptr++;
    }

    const char *umacVPtr = FTDF_getUmacRelName();

    for (i = 0; i < 16; i++)
    {
        urelName[ i ] = umacVPtr[ i ];

        if (umacVPtr[ i ] == '\0')
        {
            break;
        }
    }

    umacVPtr = FTDF_getUmacBuildTime();

    for (i = 0; i < 16; i++)
    {
        ubldTime[ i ] = umacVPtr[ i ];

        if (umacVPtr[ i ] == '\0')
        {
            break;
        }
    }

    lrelName[ 15 ] = '\0';
    lbldTime[ 15 ] = '\0';
    urelName[ 15 ] = '\0';
    ubldTime[ 15 ] = '\0';

    *lmacRelName   = lrelName;
    *lmacBuildTime = lbldTime;
    *umacRelName   = urelName;
    *umacBuildTime = ubldTime;
}

void FTDF_confirmLmacInterrupt(void)
{
    volatile uint32_t *ftdfCm = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CM);
    *ftdfCm = 0;
}

void FTDF_eventHandler(void)
{
    volatile uint32_t ftdfCe = *FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CE);

    if (ftdfCe & FTDF_MSK_RX_CE)
    {
        FTDF_processRxEvent();
    }

    if (ftdfCe & FTDF_MSK_TX_CE)
    {
        FTDF_processTxEvent();
    }

    if (ftdfCe & FTDF_MSK_SYMBOL_TMR_CE)
    {
        FTDF_processSymbolTimerEvent();
    }

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL

    if (FTDF_pib.leEnabled)
    {
        FTDF_Time curTime = FTDF_GET_FIELD(ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL);
        FTDF_Time delta   = curTime - FTDF_rzTime;

        if (delta < 0x80000000)
        {
            // RZ has passed check if a send frame is pending
            if (FTDF_sendFramePending != 0xfffe)
            {
                FTDF_Time   wakeupStartTime;
                FTDF_Period wakeupPeriod;

                FTDF_criticalVar();
                FTDF_enterCritical();

                FTDF_getWakeupParams(FTDF_sendFramePending, &wakeupStartTime, &wakeupPeriod);

                FTDF_txInProgress = FTDF_TRUE;
                FTDF_SET_FIELD(ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, wakeupStartTime);
                FTDF_SET_FIELD(ON_OFF_REGMAP_MACWUPERIOD, wakeupPeriod);

                volatile uint32_t *txFlagSet = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_TX_FLAG_SET);
                *txFlagSet           |= ((1 << FTDF_TX_DATA_BUFFER) | (1 << FTDF_TX_WAKEUP_BUFFER));

                FTDF_sendFramePending = 0xfffe;

                FTDF_exitCritical();
            }

            if (FTDF_txInProgress == FTDF_FALSE)
            {
                FTDF_setCslSampleTime();
            }
        }
    }

#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
    volatile uint32_t *ftdfCm = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CM);
    *ftdfCm = FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE;
}

#ifndef FTDF_PHY_API
void FTDF_sndMsg(FTDF_MsgBuffer *msgBuf)
{
    switch (msgBuf->msgId)
    {
#ifndef FTDF_LITE

    case FTDF_DATA_REQUEST:
        FTDF_processDataRequest((FTDF_DataRequest *) msgBuf);
        break;

    case FTDF_PURGE_REQUEST:
        FTDF_processPurgeRequest((FTDF_PurgeRequest *) msgBuf);
        break;

    case FTDF_ASSOCIATE_REQUEST:
        FTDF_processAssociateRequest((FTDF_AssociateRequest *) msgBuf);
        break;

    case FTDF_ASSOCIATE_RESPONSE:
        FTDF_processAssociateResponse((FTDF_AssociateResponse *) msgBuf);
        break;

    case FTDF_DISASSOCIATE_REQUEST:
        FTDF_processDisassociateRequest((FTDF_DisassociateRequest *) msgBuf);
        break;
#endif /* !FTDF_LITE */

    case FTDF_GET_REQUEST:
        FTDF_processGetRequest((FTDF_GetRequest *) msgBuf);
        break;

    case FTDF_SET_REQUEST:
        FTDF_processSetRequest((FTDF_SetRequest *) msgBuf);
        break;
#ifndef FTDF_LITE

    case FTDF_ORPHAN_RESPONSE:
        FTDF_processOrphanResponse((FTDF_OrphanResponse *) msgBuf);
        break;
#endif /* !FTDF_LITE */

    case FTDF_RESET_REQUEST:
        FTDF_processResetRequest((FTDF_ResetRequest *) msgBuf);
        break;

    case FTDF_RX_ENABLE_REQUEST:
        FTDF_processRxEnableRequest((FTDF_RxEnableRequest *) msgBuf);
        break;
#ifndef FTDF_LITE

    case FTDF_SCAN_REQUEST:
        FTDF_processScanRequest((FTDF_ScanRequest *) msgBuf);
        break;

    case FTDF_START_REQUEST:
        FTDF_processStartRequest((FTDF_StartRequest *) msgBuf);
        break;

    case FTDF_POLL_REQUEST:
        FTDF_processPollRequest((FTDF_PollRequest *) msgBuf);
        break;
#ifndef FTDF_NO_TSCH

    case FTDF_SET_SLOTFRAME_REQUEST:
        FTDF_processSetSlotframeRequest((FTDF_SetSlotframeRequest *) msgBuf);
        break;

    case FTDF_SET_LINK_REQUEST:
        FTDF_processSetLinkRequest((FTDF_SetLinkRequest *) msgBuf);
        break;

    case FTDF_TSCH_MODE_REQUEST:
        FTDF_processTschModeRequest((FTDF_TschModeRequest *) msgBuf);
        break;

    case FTDF_KEEP_ALIVE_REQUEST:
        FTDF_processKeepAliveRequest((FTDF_KeepAliveRequest *) msgBuf);
        break;
#endif /* FTDF_NO_TSCH */

    case FTDF_BEACON_REQUEST:
        FTDF_processBeaconRequest((FTDF_BeaconRequest *) msgBuf);
        break;
#endif /* !FTDF_LITE */

    case FTDF_TRANSPARENT_ENABLE_REQUEST:
        FTDF_enableTransparentMode(((FTDF_TransparentEnableRequest *) msgBuf)->enable,
                                   ((FTDF_TransparentEnableRequest *) msgBuf)->options);
        FTDF_REL_MSG_BUFFER(msgBuf);
        break;

    case FTDF_TRANSPARENT_REQUEST:
        FTDF_processTransparentRequest((FTDF_TransparentRequest *) msgBuf);
        break;

    case FTDF_SLEEP_REQUEST:
        FTDF_SLEEP_CALLBACK(((FTDF_SleepRequest *)msgBuf)->sleepTime);
        FTDF_REL_MSG_BUFFER(msgBuf);
        break;
#ifndef FTDF_LITE

    case FTDF_REMOTE_REQUEST:
        FTDF_processRemoteRequest((FTDF_RemoteRequest *)msgBuf);
        break;
#endif /* !FTDF_LITE */
#if FTDF_DBG_BUS_ENABLE

    case FTDF_DBG_MODE_SET_REQUEST:
        FTDF_setDbgMode(((FTDF_DbgModeSetRequest *) msgBuf)->dbgMode);
        FTDF_REL_MSG_BUFFER(msgBuf);
        break;
#endif /* FTDF_DBG_BUS_ENABLE */

    default:
        // Silenty release the message buffer
        FTDF_REL_MSG_BUFFER(msgBuf);
        break;
    }
}

void FTDF_sendFrameTransparentConfirm(void         *handle,
                                      FTDF_Bitmap32 status)
{
    FTDF_TransparentConfirm *confirm =
        (FTDF_TransparentConfirm *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_TransparentConfirm));

    confirm->msgId  = FTDF_TRANSPARENT_CONFIRM;
    confirm->handle = handle;
    confirm->status = status;

    FTDF_RCV_MSG((FTDF_MsgBuffer *) confirm);
}

void FTDF_rcvFrameTransparent(FTDF_DataLength frameLength,
                              FTDF_Octet     *frame,
                              FTDF_Bitmap32   status)
{
    FTDF_TransparentIndication *indication =
        (FTDF_TransparentIndication *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_TransparentIndication));

    indication->msgId       = FTDF_TRANSPARENT_INDICATION;
    indication->frameLength = frameLength;
    indication->status      = status;
    indication->frame       = FTDF_GET_DATA_BUFFER(frameLength);
    memcpy(indication->frame, frame, frameLength);

    FTDF_RCV_MSG((FTDF_MsgBuffer *) indication);
}
#endif /* !FTDF_PHY_API */
