/**
 ****************************************************************************************
 *
 * @file data.c
 *
 * @brief FTDF data send/receive functions
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
 *
 ****************************************************************************************
 */

#include <stdlib.h>

#include <ftdf.h>
#include "internal.h"
#include "regmap.h"

#ifndef FTDF_LITE
void FTDF_processDataRequest(FTDF_DataRequest *dataRequest)
{
#ifndef FTDF_NO_TSCH

    if (FTDF_pib.tschEnabled &&
        FTDF_tschSlotLink->request != dataRequest)
    {
        FTDF_Status status;

        if (dataRequest->dstAddrMode == FTDF_SHORT_ADDRESS &&
            !dataRequest->indirectTX)
        {
            status = FTDF_scheduleTsch((FTDF_MsgBuffer *) dataRequest);

            if (status == FTDF_SUCCESS)
            {
                return;
            }
        }
        else
        {
            status = FTDF_INVALID_PARAMETER;
        }

        FTDF_sendDataConfirm(dataRequest,
                             status,
                             0,
                             0,
                             0,
                             NULL);

        return;
    }

#endif /* FTDF_NO_TSCH */

    int              queue;
    FTDF_AddressMode dstAddrMode = dataRequest->dstAddrMode;
    FTDF_PANId       dstPANId    = dataRequest->dstPANId;
    FTDF_Address     dstAddr     = dataRequest->dstAddr;

    // Search for an existing indirect queue
    for (queue = 0; queue < FTDF_NR_OF_REQ_BUFFERS; queue++)
    {
        if (dstAddrMode == FTDF_SHORT_ADDRESS)
        {
            if (FTDF_txPendingList[ queue ].addrMode == dstAddrMode &&
                FTDF_txPendingList[ queue ].addr.shortAddress == dstAddr.shortAddress)
            {
                break;
            }
        }
        else if (dstAddrMode == FTDF_EXTENDED_ADDRESS)
        {
            if (FTDF_txPendingList[ queue ].addrMode == dstAddrMode &&
                FTDF_txPendingList[ queue ].addr.extAddress == dstAddr.extAddress)
            {
                break;
            }
        }
    }

    if (dataRequest->indirectTX)
    {
        FTDF_Status status = FTDF_SUCCESS;

        if (queue < FTDF_NR_OF_REQ_BUFFERS)
        {
            // Queue request in existing queue
            status = FTDF_queueReqHead((FTDF_MsgBuffer *) dataRequest, &FTDF_txPendingList[ queue ].queue);

            if (status == FTDF_SUCCESS)
            {
                FTDF_addTxPendingTimer((FTDF_MsgBuffer *) dataRequest,
                                       queue,
                                       FTDF_pib.transactionPersistenceTime * FTDF_BASE_SUPERFRAME_DURATION,
                                       FTDF_sendTransactionExpired);
                return;
            }
        }

        if (dstAddrMode != FTDF_EXTENDED_ADDRESS &&
            dstAddrMode != FTDF_SHORT_ADDRESS)
        {
            status = FTDF_INVALID_PARAMETER;
        }

        if (status != FTDF_SUCCESS)
        {
            // Queueing of indirect transfer was not successful
            FTDF_sendDataConfirm(dataRequest,
                                 status,
                                 0,
                                 0,
                                 0,
                                 NULL);

            return;
        }

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
        uint8_t entry, shortAddrIdx;

        if (dstAddrMode == FTDF_SHORT_ADDRESS)
        {
            if (FTDF_fpprGetFreeShortAddress(&entry, &shortAddrIdx) == FTDF_FALSE)
            {
                goto transaction_overflow;
            }
        }
        else if (dstAddrMode == FTDF_EXTENDED_ADDRESS)
        {
            if (FTDF_fpprGetFreeExtAddress(&entry) == FTDF_FALSE)
            {
                goto transaction_overflow;
            }
        }
        else
        {
            status = FTDF_INVALID_PARAMETER;
        }

#endif

        // Search for an empty indirect queue
        for (queue = 0; queue < FTDF_NR_OF_REQ_BUFFERS; queue++)
        {
            if (FTDF_txPendingList[ queue ].addrMode == FTDF_NO_ADDRESS)
            {
                FTDF_txPendingList[ queue ].addrMode = dstAddrMode;
                FTDF_txPendingList[ queue ].PANId    = dstPANId;
                FTDF_txPendingList[ queue ].addr     = dstAddr;

                status                               = FTDF_queueReqHead((FTDF_MsgBuffer *) dataRequest,
                                                                         &FTDF_txPendingList[ queue ].queue);

                if (status == FTDF_SUCCESS)
                {
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO

                    if (dstAddrMode == FTDF_SHORT_ADDRESS)
                    {
                        FTDF_fpprSetShortAddress(entry, shortAddrIdx, dstAddr.shortAddress);
                        FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_TRUE);
                    }
                    else if (dstAddrMode == FTDF_EXTENDED_ADDRESS)
                    {
                        FTDF_fpprSetExtAddress(entry, dstAddr.shortAddress);
                        FTDF_fpprSetExtAddressValid(entry, FTDF_TRUE);
                    }
                    else
                    {
                        ASSERT_WARNING(0);
                    }

#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
                    FTDF_addTxPendingTimer((FTDF_MsgBuffer *) dataRequest,
                                           queue,
                                           FTDF_pib.transactionPersistenceTime * FTDF_BASE_SUPERFRAME_DURATION,
                                           FTDF_sendTransactionExpired);
                    return;
                }
                else
                {
                    break;
                }
            }
        }

        // Did not find an existing or an empty queue
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
transaction_overflow:
#endif
        FTDF_sendDataConfirm(dataRequest, FTDF_TRANSACTION_OVERFLOW,
                             0,
                             0,
                             0,
                             NULL);

        return;
    }

    if (FTDF_reqCurrent == NULL)
    {
        FTDF_reqCurrent = (FTDF_MsgBuffer *) dataRequest;
    }
    else
    {
        if (FTDF_queueReqHead((FTDF_MsgBuffer *) dataRequest, &FTDF_reqQueue) == FTDF_TRANSACTION_OVERFLOW)
        {
            FTDF_sendDataConfirm(dataRequest, FTDF_TRANSACTION_OVERFLOW,
                                 0,
                                 0,
                                 0,
                                 NULL);
        }

        return;
    }

    FTDF_FrameHeader    *frameHeader    = &FTDF_fh;
    FTDF_SecurityHeader *securityHeader = &FTDF_sh;

    frameHeader->frameType = dataRequest->sendMultiPurpose ? FTDF_MULTIPURPOSE_FRAME : FTDF_DATA_FRAME;

    FTDF_Boolean framePending;

    if (queue < FTDF_NR_OF_REQ_BUFFERS)
    {
        framePending = FTDF_TRUE;
    }
    else
    {
        framePending = FTDF_FALSE;
    }

    frameHeader->options =
        (dataRequest->securityLevel > 0 ? FTDF_OPT_SECURITY_ENABLED : 0) |
        (dataRequest->ackTX ? FTDF_OPT_ACK_REQUESTED : 0) |
        (framePending ? FTDF_OPT_FRAME_PENDING : 0) |
        ((dataRequest->frameControlOptions & FTDF_PAN_ID_PRESENT) ? FTDF_OPT_PAN_ID_PRESENT : 0) |
        ((dataRequest->frameControlOptions & FTDF_IES_INCLUDED) ? FTDF_OPT_IES_PRESENT : 0) |
        ((dataRequest->frameControlOptions & FTDF_SEQ_NR_SUPPRESSED) ? FTDF_OPT_SEQ_NR_SUPPRESSED : 0);

    if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled)
    {
        frameHeader->options |= FTDF_OPT_ENHANCED;
    }

    frameHeader->srcAddrMode         = dataRequest->srcAddrMode;
    frameHeader->srcPANId            = FTDF_pib.PANId;
    frameHeader->dstAddrMode         = dataRequest->dstAddrMode;
    frameHeader->dstPANId            = dataRequest->dstPANId;
    frameHeader->dstAddr             = dataRequest->dstAddr;

    securityHeader->securityLevel    = dataRequest->securityLevel;
    securityHeader->keyIdMode        = dataRequest->keyIdMode;
    securityHeader->keyIndex         = dataRequest->keyIndex;
    securityHeader->keySource        = dataRequest->keySource;
    securityHeader->frameCounter     = FTDF_pib.frameCounter;
    securityHeader->frameCounterMode = FTDF_pib.frameCounterMode;

#ifndef FTDF_NO_TSCH

    if (FTDF_pib.tschEnabled)
    {
        frameHeader->SN = FTDF_processTschSN((FTDF_MsgBuffer *)dataRequest,
                                             FTDF_pib.DSN,
                                             &dataRequest->requestSN);
    }
    else
#endif /* FTDF_NO_TSCH */
    {
        frameHeader->SN = FTDF_pib.DSN;
    }

    FTDF_Octet *txPtr = (FTDF_Octet *) FTDF_GET_REG_ADDR(RETENTION_RAM_TX_FIFO) +
                        (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

    // Skip PHY header (= MAC length)
    txPtr++;

    FTDF_DataLength msduLength = dataRequest->msduLength;

    txPtr           = FTDF_addFrameHeader(txPtr,
                                          frameHeader,
                                          msduLength);

    txPtr = FTDF_addSecurityHeader(txPtr,
                                   securityHeader);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)

    if (dataRequest->frameControlOptions & FTDF_IES_INCLUDED)
    {
        txPtr = FTDF_addIes(txPtr,
                            dataRequest->headerIEList,
                            dataRequest->payloadIEList,
                            dataRequest->msduLength);
    }

#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

    FTDF_Status status = FTDF_sendFrame(FTDF_pib.currentChannel,
                                        frameHeader,
                                        securityHeader,
                                        txPtr,
                                        dataRequest->msduLength,
                                        dataRequest->msdu);

    if (status != FTDF_SUCCESS)
    {
        FTDF_sendDataConfirm(dataRequest, status,
                             0,
                             0,
                             0,
                             NULL);

        FTDF_reqCurrent = NULL;

        return;
    }

    FTDF_nrOfRetries = 0;

    if (frameHeader->SN == FTDF_pib.DSN)
    {
        FTDF_pib.DSN++;
    }
}

void FTDF_sendDataConfirm(FTDF_DataRequest  *dataRequest,
                          FTDF_Status        status,
                          FTDF_Time          timestamp,
                          FTDF_SN            DSN,
                          FTDF_NumOfBackoffs numOfBackoffs,
                          FTDF_IEList       *ackPayload)
{
    FTDF_REL_DATA_BUFFER(dataRequest->msdu);

    FTDF_DataConfirm *dataConfirm = (FTDF_DataConfirm *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_DataConfirm));

    dataConfirm->msgId         = FTDF_DATA_CONFIRM;
    dataConfirm->msduHandle    = dataRequest->msduHandle;
    dataConfirm->status        = status;
    dataConfirm->timestamp     = timestamp;
    dataConfirm->numOfBackoffs = numOfBackoffs;
    dataConfirm->DSN           = DSN;
    dataConfirm->ackPayload    = ackPayload;

    if (FTDF_reqCurrent == (FTDF_MsgBuffer *)dataRequest)
    {
        FTDF_reqCurrent = NULL;
    }

    FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) dataRequest);
    FTDF_RCV_MSG((FTDF_MsgBuffer *) dataConfirm);
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
    FTDF_fpFsmClearPending();
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
    FTDF_processNextRequest();
}

void FTDF_sendDataIndication(FTDF_FrameHeader    *frameHeader,
                             FTDF_SecurityHeader *securityHeader,
                             FTDF_IEList         *payloadIEList,
                             FTDF_DataLength      msduLength,
                             FTDF_Octet          *msdu,
                             FTDF_LinkQuality     mpduLinkQuality,
                             FTDF_Time            timestamp)
{
    FTDF_DataIndication *dataIndication = (FTDF_DataIndication *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_DataIndication));
    FTDF_Octet          *msduBuf        = FTDF_GET_DATA_BUFFER(msduLength);
    FTDF_Octet          *bufPtr         = msduBuf;

    int                  n;

    for (n = 0; n < msduLength; n++)
    {
        *msduBuf++ = *msdu++;
    }

    dataIndication->msgId           = FTDF_DATA_INDICATION;
    dataIndication->srcAddrMode     = frameHeader->srcAddrMode;
    dataIndication->srcPANId        = frameHeader->srcPANId;
    dataIndication->srcAddr         = frameHeader->srcAddr;
    dataIndication->dstAddrMode     = frameHeader->dstAddrMode;
    dataIndication->dstPANId        = frameHeader->dstPANId;
    dataIndication->dstAddr         = frameHeader->dstAddr;
    dataIndication->msduLength      = msduLength;
    dataIndication->msdu            = bufPtr;
    dataIndication->mpduLinkQuality = mpduLinkQuality;
    dataIndication->DSN             = frameHeader->SN;
    dataIndication->timestamp       = timestamp;
    dataIndication->securityLevel   = securityHeader->securityLevel;
    dataIndication->keyIdMode       = securityHeader->keyIdMode;
    dataIndication->keyIndex        = securityHeader->keyIndex;
    dataIndication->payloadIEList   = payloadIEList;

    if (dataIndication->keyIdMode == 0x2)
    {
        for (n = 0; n < 4; n++)
        {
            dataIndication->keySource[ n ] = securityHeader->keySource[ n ];
        }
    }
    else if (dataIndication->keyIdMode == 0x3)
    {
        for (n = 0; n < 8; n++)
        {
            dataIndication->keySource[ n ] = securityHeader->keySource[ n ];
        }
    }

    FTDF_RCV_MSG((FTDF_MsgBuffer *) dataIndication);
}

void FTDF_processPollRequest(FTDF_PollRequest *pollRequest)
{
    if (FTDF_reqCurrent == NULL)
    {
        FTDF_reqCurrent = (FTDF_MsgBuffer *) pollRequest;
    }
    else
    {
        if (FTDF_queueReqHead((FTDF_MsgBuffer *) pollRequest, &FTDF_reqQueue) == FTDF_TRANSACTION_OVERFLOW)
        {
            FTDF_sendPollConfirm(pollRequest, FTDF_TRANSACTION_OVERFLOW);
        }

        return;
    }

    FTDF_FrameHeader    *frameHeader    = &FTDF_fh;
    FTDF_SecurityHeader *securityHeader = &FTDF_sh;

    frameHeader->frameType = FTDF_MAC_COMMAND_FRAME;
    frameHeader->options   =
        (pollRequest->securityLevel > 0 ? FTDF_OPT_SECURITY_ENABLED : 0) | FTDF_OPT_ACK_REQUESTED;

    if (FTDF_pib.shortAddress < 0xfffe)
    {
        frameHeader->srcAddrMode          = FTDF_SHORT_ADDRESS;
        frameHeader->srcAddr.shortAddress = FTDF_pib.shortAddress;
    }
    else
    {
        frameHeader->srcAddrMode        = FTDF_EXTENDED_ADDRESS;
        frameHeader->srcAddr.extAddress = FTDF_pib.extAddress;
    }

    frameHeader->srcPANId            = FTDF_pib.PANId;
    frameHeader->dstAddrMode         = pollRequest->coordAddrMode;
    frameHeader->dstPANId            = pollRequest->coordPANId;
    frameHeader->dstAddr             = pollRequest->coordAddr;

    securityHeader->securityLevel    = pollRequest->securityLevel;
    securityHeader->keyIdMode        = pollRequest->keyIdMode;
    securityHeader->keyIndex         = pollRequest->keyIndex;
    securityHeader->keySource        = pollRequest->keySource;
    securityHeader->frameCounter     = FTDF_pib.frameCounter;
    securityHeader->frameCounterMode = FTDF_pib.frameCounterMode;

    FTDF_Octet *txPtr = (FTDF_Octet *) FTDF_GET_REG_ADDR(RETENTION_RAM_TX_FIFO) +
                        (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

    // Skip PHY header (= MAC length)
    txPtr++;

    frameHeader->SN = FTDF_pib.DSN;

    txPtr           = FTDF_addFrameHeader(txPtr,
                                          frameHeader,
                                          1);

    txPtr = FTDF_addSecurityHeader(txPtr,
                                   securityHeader);

    *txPtr++ = FTDF_COMMAND_DATA_REQUEST;

    FTDF_Status status = FTDF_sendFrame(FTDF_pib.currentChannel,
                                        frameHeader,
                                        securityHeader,
                                        txPtr,
                                        0,
                                        NULL);

    if (status != FTDF_SUCCESS)
    {
        FTDF_sendPollConfirm(pollRequest, status);

        return;
    }

    FTDF_nrOfRetries = 0;
    FTDF_pib.DSN++;
}

void FTDF_sendPollConfirm(FTDF_PollRequest *pollRequest, FTDF_Status status)
{
    FTDF_PollConfirm *pollConfirm = (FTDF_PollConfirm *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_PollConfirm));

    pollConfirm->msgId  = FTDF_POLL_CONFIRM;
    pollConfirm->status = status;

    if (FTDF_reqCurrent == (FTDF_MsgBuffer *)pollRequest)
    {
        FTDF_reqCurrent = NULL;
    }

    FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) pollRequest);
    FTDF_RCV_MSG((FTDF_MsgBuffer *) pollConfirm);

    FTDF_processNextRequest();
}

void FTDF_processPurgeRequest(FTDF_PurgeRequest *purgeRequest)
{
    FTDF_Handle msduHandle = purgeRequest->msduHandle;
    FTDF_Status status     = FTDF_INVALID_HANDLE;

    int         n;

    for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++)
    {
        FTDF_MsgBuffer *request = FTDF_dequeueByHandle(msduHandle, &FTDF_txPendingList[ n ].queue);

        if (request)
        {
            FTDF_DataRequest *dataRequest = (FTDF_DataRequest *) request;

            if (dataRequest->indirectTX == FTDF_TRUE)
            {
                FTDF_removeTxPendingTimer(request);
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO

                if (FTDF_txPendingList[ n ].addrMode == FTDF_SHORT_ADDRESS)
                {
                    uint8_t entry, shortAddrIdx;
                    FTDF_Boolean found = FTDF_fpprLookupShortAddress(
                                             FTDF_txPendingList[ n ].addr.shortAddress, &entry,
                                             &shortAddrIdx);
                    ASSERT_WARNING(found);
                    FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_FALSE);
                }
                else if (FTDF_txPendingList[ n ].addrMode  == FTDF_EXTENDED_ADDRESS)
                {
                    uint8_t entry;
                    FTDF_Boolean found = FTDF_fpprLookupExtAddress(
                                             FTDF_txPendingList[ n ].addr.extAddress, &entry);
                    ASSERT_WARNING(found);
                    FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);
                }
                else
                {
                    ASSERT_WARNING(0);
                }

#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

                if (FTDF_isQueueEmpty(&FTDF_txPendingList[ n ].queue))
                {
                    FTDF_txPendingList[ n ].addrMode = FTDF_NO_ADDRESS;
                }
            }

            FTDF_REL_DATA_BUFFER(dataRequest->msdu);
            FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) dataRequest);

            status = FTDF_SUCCESS;

            break;
        }
    }

    FTDF_PurgeConfirm *purgeConfirm = (FTDF_PurgeConfirm *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_PurgeConfirm));

    purgeConfirm->msgId      = FTDF_PURGE_CONFIRM;
    purgeConfirm->msduHandle = msduHandle;
    purgeConfirm->status     = status;

    FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) purgeRequest);
    FTDF_RCV_MSG((FTDF_MsgBuffer *) purgeConfirm);
}
#endif /* !FTDF_LITE */

#ifdef FTDF_PHY_API
FTDF_Status FTDF_sendFrameSimple(FTDF_DataLength    frameLength,
                                 FTDF_Octet        *frame,
                                 FTDF_ChannelNumber channel,
                                 FTDF_PTI           pti,
                                 FTDF_Boolean       csmaSuppress)
{

    FTDF_Octet *fp = frame;

    if (frameLength > 127 ||
        FTDF_transparentMode != FTDF_TRUE)
    {
        return FTDF_INVALID_PARAMETER;
    }

    FTDF_Octet *txPtr = (FTDF_Octet *) FTDF_GET_REG_ADDR(RETENTION_RAM_TX_FIFO) +
                        (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

    /* This data copy could/should be optimized using DMA */
    *txPtr++ = frameLength;

    int         n;

    for (n = 0; n < frameLength; n++)
    {
        *txPtr++ = *fp++;
    }

    FTDF_criticalVar();
    FTDF_enterCritical();
    FTDF_nrOfRetries = 0;
    FTDF_exitCritical();
    FTDF_sendTransparentFrame(frameLength,
                              frame,
                              channel,
                              pti,
                              csmaSuppress);

    return FTDF_SUCCESS;
}



#else /* !FTDF_PHY_API */
void FTDF_processTransparentRequest(FTDF_TransparentRequest *transparentRequest)
{
    FTDF_DataLength frameLength = transparentRequest->frameLength;

    if (frameLength > 127 ||
        FTDF_transparentMode != FTDF_TRUE)
    {
        FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparentRequest->handle,
                                            FTDF_INVALID_PARAMETER);

        FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) transparentRequest);

        return;
    }

    if (FTDF_reqCurrent == NULL)
    {
        FTDF_reqCurrent = (FTDF_MsgBuffer *) transparentRequest;
    }
    else
    {
#ifndef FTDF_LITE

        if (FTDF_queueReqHead((FTDF_MsgBuffer *) transparentRequest, &FTDF_reqQueue) == FTDF_TRANSACTION_OVERFLOW)
        {
#endif /* !FTDF_LITE */
            FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparentRequest->handle,
                                                FTDF_TRANSPARENT_OVERFLOW);

            FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) transparentRequest);
#ifndef FTDF_LITE
        }

#endif /* !FTDF_LITE */
        return;
    }

    FTDF_Octet *txPtr = (FTDF_Octet *) FTDF_GET_REG_ADDR(RETENTION_RAM_TX_FIFO) +
                        (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

    *txPtr++ = frameLength;

    FTDF_Octet *frame = transparentRequest->frame;

    int         n;

    for (n = 0; n < frameLength; n++)
    {
        *txPtr++ = *frame++;
    }

    FTDF_sendTransparentFrame(frameLength,
                              transparentRequest->frame,
                              transparentRequest->channel,
                              transparentRequest->pti,
                              transparentRequest->cmsaSuppress);
    FTDF_nrOfRetries = 0;
}


void FTDF_sendFrameTransparent(FTDF_DataLength    frameLength,
                               FTDF_Octet        *frame,
                               FTDF_ChannelNumber channel,
                               FTDF_PTI           pti,
                               FTDF_Boolean       cmsaSuppress,
                               void              *handle)
{
    FTDF_TransparentRequest *transparentRequest =
        (FTDF_TransparentRequest *) FTDF_GET_MSG_BUFFER(sizeof(FTDF_TransparentRequest));

    transparentRequest->msgId        = FTDF_TRANSPARENT_REQUEST;
    transparentRequest->frameLength  = frameLength;
    transparentRequest->frame        = frame;
    transparentRequest->channel      = channel;
    transparentRequest->pti          = pti;
    transparentRequest->cmsaSuppress = cmsaSuppress;
    transparentRequest->handle       = handle;

    FTDF_processTransparentRequest(transparentRequest);
}

#endif /* FTDF_PHY_API */
