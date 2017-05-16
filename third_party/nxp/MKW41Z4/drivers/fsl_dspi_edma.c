/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_dspi_edma.h"

/***********************************************************************************************************************
* Definitons
***********************************************************************************************************************/

/*!
* @brief Structure definition for dspi_master_edma_private_handle_t. The structure is private.
*/
typedef struct _dspi_master_edma_private_handle
{
    SPI_Type *base;                    /*!< DSPI peripheral base address. */
    dspi_master_edma_handle_t *handle; /*!< dspi_master_edma_handle_t handle */
} dspi_master_edma_private_handle_t;

/*!
* @brief Structure definition for dspi_slave_edma_private_handle_t. The structure is private.
*/
typedef struct _dspi_slave_edma_private_handle
{
    SPI_Type *base;                   /*!< DSPI peripheral base address. */
    dspi_slave_edma_handle_t *handle; /*!< dspi_master_edma_handle_t handle */
} dspi_slave_edma_private_handle_t;

/***********************************************************************************************************************
* Prototypes
***********************************************************************************************************************/
/*!
* @brief EDMA_DspiMasterCallback after the DSPI master transfer completed by using EDMA.
* This is not a public API.
*/
static void EDMA_DspiMasterCallback(edma_handle_t *edmaHandle,
                                    void *g_dspiEdmaPrivateHandle,
                                    bool transferDone,
                                    uint32_t tcds);

/*!
* @brief EDMA_DspiSlaveCallback after the DSPI slave transfer completed by using EDMA.
* This is not a public API.
*/
static void EDMA_DspiSlaveCallback(edma_handle_t *edmaHandle,
                                   void *g_dspiEdmaPrivateHandle,
                                   bool transferDone,
                                   uint32_t tcds);
/*!
* @brief Get instance number for DSPI module.
*
* This is not a public API and it's extern from fsl_dspi.c.
*
* @param base DSPI peripheral base address
*/
extern uint32_t DSPI_GetInstance(SPI_Type *base);

/***********************************************************************************************************************
* Variables
***********************************************************************************************************************/

/*! @brief Pointers to dspi edma handles for each instance. */
static dspi_master_edma_private_handle_t s_dspiMasterEdmaPrivateHandle[FSL_FEATURE_SOC_DSPI_COUNT];
static dspi_slave_edma_private_handle_t s_dspiSlaveEdmaPrivateHandle[FSL_FEATURE_SOC_DSPI_COUNT];

/***********************************************************************************************************************
* Code
***********************************************************************************************************************/

void DSPI_MasterTransferCreateHandleEDMA(SPI_Type *base,
                                         dspi_master_edma_handle_t *handle,
                                         dspi_master_edma_transfer_callback_t callback,
                                         void *userData,
                                         edma_handle_t *edmaRxRegToRxDataHandle,
                                         edma_handle_t *edmaTxDataToIntermediaryHandle,
                                         edma_handle_t *edmaIntermediaryToTxRegHandle)
{
    assert(handle);
    assert(edmaRxRegToRxDataHandle);
    assert(edmaTxDataToIntermediaryHandle);
    assert(edmaIntermediaryToTxRegHandle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    uint32_t instance = DSPI_GetInstance(base);

    s_dspiMasterEdmaPrivateHandle[instance].base = base;
    s_dspiMasterEdmaPrivateHandle[instance].handle = handle;

    handle->callback = callback;
    handle->userData = userData;

    handle->edmaRxRegToRxDataHandle = edmaRxRegToRxDataHandle;
    handle->edmaTxDataToIntermediaryHandle = edmaTxDataToIntermediaryHandle;
    handle->edmaIntermediaryToTxRegHandle = edmaIntermediaryToTxRegHandle;
}

status_t DSPI_MasterTransferEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle, dspi_transfer_t *transfer)
{
    assert(handle);
    assert(transfer);

    /* If the transfer count is zero, then return immediately.*/
    if (transfer->dataSize == 0)
    {
        return kStatus_InvalidArgument;
    }

    /* If both send buffer and receive buffer is null */
    if ((!(transfer->txData)) && (!(transfer->rxData)))
    {
        return kStatus_InvalidArgument;
    }

    /* Check that we're not busy.*/
    if (handle->state == kDSPI_Busy)
    {
        return kStatus_DSPI_Busy;
    }

    handle->state = kDSPI_Busy;

    uint32_t instance = DSPI_GetInstance(base);
    uint16_t wordToSend = 0;
    uint8_t dummyData = DSPI_DUMMY_DATA;
    uint8_t dataAlreadyFed = 0;
    uint8_t dataFedMax = 2;

    uint32_t rxAddr = DSPI_GetRxRegisterAddress(base);
    uint32_t txAddr = DSPI_MasterGetTxRegisterAddress(base);

    edma_tcd_t *softwareTCD = (edma_tcd_t *)((uint32_t)(&handle->dspiSoftwareTCD[1]) & (~0x1FU));

    edma_transfer_config_t transferConfigA;
    edma_transfer_config_t transferConfigB;
    edma_transfer_config_t transferConfigC;

    handle->txBuffIfNull = ((uint32_t)DSPI_DUMMY_DATA << 8) | DSPI_DUMMY_DATA;

    dspi_command_data_config_t commandStruct;
    DSPI_StopTransfer(base);
    DSPI_FlushFifo(base, true, true);
    DSPI_ClearStatusFlags(base, kDSPI_AllStatusFlag);

    commandStruct.whichPcs =
        (dspi_which_pcs_t)(1U << ((transfer->configFlags & DSPI_MASTER_PCS_MASK) >> DSPI_MASTER_PCS_SHIFT));
    commandStruct.isEndOfQueue = false;
    commandStruct.clearTransferCount = false;
    commandStruct.whichCtar =
        (dspi_ctar_selection_t)((transfer->configFlags & DSPI_MASTER_CTAR_MASK) >> DSPI_MASTER_CTAR_SHIFT);
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterPcsContinuous);
    handle->command = DSPI_MasterGetFormattedCommand(&(commandStruct));

    commandStruct.isEndOfQueue = true;
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterActiveAfterTransfer);
    handle->lastCommand = DSPI_MasterGetFormattedCommand(&(commandStruct));

    handle->bitsPerFrame = ((base->CTAR[commandStruct.whichCtar] & SPI_CTAR_FMSZ_MASK) >> SPI_CTAR_FMSZ_SHIFT) + 1;

    if ((base->MCR & SPI_MCR_DIS_RXF_MASK) || (base->MCR & SPI_MCR_DIS_TXF_MASK))
    {
        handle->fifoSize = 1;
    }
    else
    {
        handle->fifoSize = FSL_FEATURE_DSPI_FIFO_SIZEn(base);
    }
    handle->txData = transfer->txData;
    handle->rxData = transfer->rxData;
    handle->remainingSendByteCount = transfer->dataSize;
    handle->remainingReceiveByteCount = transfer->dataSize;
    handle->totalByteCount = transfer->dataSize;

    /* If using a shared RX/TX DMA request, then this limits the amount of data we can transfer
    * due to the linked channel. The max bytes is 511 if 8-bit/frame or 1022 if 16-bit/frame
    */
    uint32_t limited_size = 0;
    if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        limited_size = 32767u;
    }
    else
    {
        limited_size = 511u;
    }

    if (handle->bitsPerFrame > 8)
    {
        if (transfer->dataSize > (limited_size << 1u))
        {
            handle->state = kDSPI_Idle;
            return kStatus_DSPI_OutOfRange;
        }
    }
    else
    {
        if (transfer->dataSize > limited_size)
        {
            handle->state = kDSPI_Idle;
            return kStatus_DSPI_OutOfRange;
        }
    }

    /*The data size should be even if the bitsPerFrame is greater than 8 (that is 2 bytes per frame in dspi) */
    if ((handle->bitsPerFrame > 8) && (transfer->dataSize & 0x1))
    {
        handle->state = kDSPI_Idle;
        return kStatus_InvalidArgument;
    }

    DSPI_DisableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    EDMA_SetCallback(handle->edmaRxRegToRxDataHandle, EDMA_DspiMasterCallback,
                     &s_dspiMasterEdmaPrivateHandle[instance]);

    /*
    (1)For DSPI instances with shared RX/TX DMA requests: Rx DMA request -> channel_A -> channel_B-> channel_C.
    channel_A minor link to channel_B , channel_B minor link to channel_C.

    Already pushed 1 or 2 data in SPI_PUSHR , then start the DMA tansfer.
    channel_A:SPI_POPR to rxData,
    channel_B:next txData to handle->command (low 16 bits),
    channel_C:handle->command (32 bits) to SPI_PUSHR, and use the scatter/gather to transfer the last data
    (handle->lastCommand to SPI_PUSHR).

    (2)For DSPI instances with separate RX and TX DMA requests:
    Rx DMA request -> channel_A
    Tx DMA request -> channel_C -> channel_B .
    channel_C major link to channel_B.
    So need prepare the first data in "intermediary"  before the DMA
    transfer and then channel_B is used to prepare the next data to "intermediary"

    channel_A:SPI_POPR to rxData,
    channel_C: handle->command (32 bits) to SPI_PUSHR,
    channel_B: next txData to handle->command (low 16 bits), and use the scatter/gather to prepare the last data
    (handle->lastCommand to handle->Command).
    */

    /*If dspi has separate dma request , prepare the first data in "intermediary" .
    else (dspi has shared dma request) , send first 2 data if there is fifo or send first 1 data if there is no fifo*/
    if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        /* For DSPI instances with separate RX/TX DMA requests, we'll use the TX DMA request to
        * trigger the TX DMA channel and RX DMA request to trigger the RX DMA channel
        */

        /*Prepare the firt data*/
        if (handle->bitsPerFrame > 8)
        {
            /* If it's the last word */
            if (handle->remainingSendByteCount <= 2)
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    ++handle->txData; /* increment to next data byte */
                    wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                }
                else
                {
                    wordToSend = ((uint32_t)dummyData << 8) | dummyData;
                }
                handle->lastCommand = (handle->lastCommand & 0xffff0000U) | wordToSend;
                handle->command = handle->lastCommand;
            }
            else /* For all words except the last word , frame > 8bits */
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    ++handle->txData; /* increment to next data byte */
                    wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                    ++handle->txData; /* increment to next data byte */
                }
                else
                {
                    wordToSend = ((uint32_t)dummyData << 8) | dummyData;
                }
                handle->command = (handle->command & 0xffff0000U) | wordToSend;
            }
        }
        else /* Optimized for bits/frame less than or equal to one byte. */
        {
            if (handle->txData)
            {
                wordToSend = *(handle->txData);
                ++handle->txData; /* increment to next data word*/
            }
            else
            {
                wordToSend = dummyData;
            }

            if (handle->remainingSendByteCount == 1)
            {
                handle->lastCommand = (handle->lastCommand & 0xffff0000U) | wordToSend;
                handle->command = handle->lastCommand;
            }
            else
            {
                handle->command = (handle->command & 0xffff0000U) | wordToSend;
            }
        }
    }

    else /*dspi has shared dma request*/

    {
        /* For DSPI instances with shared RX/TX DMA requests, we'll use the RX DMA request to
        * trigger ongoing transfers and will link to the TX DMA channel from the RX DMA channel.
        */

        /* If bits/frame is greater than one byte */
        if (handle->bitsPerFrame > 8)
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag)
            {
                if (handle->remainingSendByteCount <= 2)
                {
                    if (handle->txData)
                    {
                        wordToSend = *(handle->txData);
                        ++handle->txData;
                        wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                    }
                    else
                    {
                        wordToSend = ((uint32_t)dummyData << 8) | dummyData;
                    }
                    handle->remainingSendByteCount = 0;
                    base->PUSHR = (handle->lastCommand & 0xffff0000U) | wordToSend;
                }
                /* For all words except the last word */
                else
                {
                    if (handle->txData)
                    {
                        wordToSend = *(handle->txData);
                        ++handle->txData;
                        wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                        ++handle->txData;
                    }
                    else
                    {
                        wordToSend = ((uint32_t)dummyData << 8) | dummyData;
                    }
                    handle->remainingSendByteCount -= 2;
                    base->PUSHR = (handle->command & 0xffff0000U) | wordToSend;
                }

                /* Try to clear the TFFF; if the TX FIFO is full this will clear */
                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

                dataAlreadyFed += 2;

                /* exit loop if send count is zero, else update local variables for next loop */
                if ((handle->remainingSendByteCount == 0) || (dataAlreadyFed == (dataFedMax * 2)))
                {
                    break;
                }
            } /* End of TX FIFO fill while loop */
        }
        else /* Optimized for bits/frame less than or equal to one byte. */
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag)
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    ++handle->txData;
                }
                else
                {
                    wordToSend = dummyData;
                }

                if (handle->remainingSendByteCount == 1)
                {
                    base->PUSHR = (handle->lastCommand & 0xffff0000U) | wordToSend;
                }
                else
                {
                    base->PUSHR = (handle->command & 0xffff0000U) | wordToSend;
                }

                /* Try to clear the TFFF; if the TX FIFO is full this will clear */
                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

                --handle->remainingSendByteCount;

                dataAlreadyFed++;

                /* exit loop if send count is zero, else update local variables for next loop */
                if ((handle->remainingSendByteCount == 0) || (dataAlreadyFed == dataFedMax))
                {
                    break;
                }
            } /* End of TX FIFO fill while loop */
        }
    }

    /***channel_A *** used for carry the data from Rx_Data_Register(POPR) to User_Receive_Buffer(rxData)*/
    EDMA_ResetChannel(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel);

    transferConfigA.srcAddr = (uint32_t)rxAddr;
    transferConfigA.srcOffset = 0;

    if (handle->rxData)
    {
        transferConfigA.destAddr = (uint32_t) & (handle->rxData[0]);
        transferConfigA.destOffset = 1;
    }
    else
    {
        transferConfigA.destAddr = (uint32_t) & (handle->rxBuffIfNull);
        transferConfigA.destOffset = 0;
    }

    transferConfigA.destTransferSize = kEDMA_TransferSize1Bytes;

    if (handle->bitsPerFrame <= 8)
    {
        transferConfigA.srcTransferSize = kEDMA_TransferSize1Bytes;
        transferConfigA.minorLoopBytes = 1;
        transferConfigA.majorLoopCounts = handle->remainingReceiveByteCount;
    }
    else
    {
        transferConfigA.srcTransferSize = kEDMA_TransferSize2Bytes;
        transferConfigA.minorLoopBytes = 2;
        transferConfigA.majorLoopCounts = handle->remainingReceiveByteCount / 2;
    }

    /* Store the initially configured eDMA minor byte transfer count into the DSPI handle */
    handle->nbytes = transferConfigA.minorLoopBytes;

    EDMA_SetTransferConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                           &transferConfigA, NULL);
    EDMA_EnableChannelInterrupts(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                 kEDMA_MajorInterruptEnable);

    /***channel_B *** used for carry the data from User_Send_Buffer to "intermediary" because the SPIx_PUSHR should
    write the 32bits at once time . Then use channel_C to carry the "intermediary" to SPIx_PUSHR. Note that the
    SPIx_PUSHR upper 16 bits are the "command" and the low 16bits are data */

    EDMA_ResetChannel(handle->edmaTxDataToIntermediaryHandle->base, handle->edmaTxDataToIntermediaryHandle->channel);

    /*Calculate the last data : handle->lastCommand*/
    if (((handle->remainingSendByteCount > 0) && (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))) ||
        ((((handle->remainingSendByteCount > 1) && (handle->bitsPerFrame <= 8)) ||
          ((handle->remainingSendByteCount > 2) && (handle->bitsPerFrame > 8))) &&
         (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))))
    {
        if (handle->txData)
        {
            uint32_t bufferIndex = 0;

            if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
            {
                if (handle->bitsPerFrame <= 8)
                {
                    bufferIndex = handle->remainingSendByteCount - 1;
                }
                else
                {
                    bufferIndex = handle->remainingSendByteCount - 2;
                }
            }
            else
            {
                bufferIndex = handle->remainingSendByteCount;
            }

            if (handle->bitsPerFrame <= 8)
            {
                handle->lastCommand = (handle->lastCommand & 0xffff0000U) | handle->txData[bufferIndex - 1];
            }
            else
            {
                handle->lastCommand = (handle->lastCommand & 0xffff0000U) |
                                      ((uint32_t)handle->txData[bufferIndex - 1] << 8) |
                                      handle->txData[bufferIndex - 2];
            }
        }
        else
        {
            if (handle->bitsPerFrame <= 8)
            {
                wordToSend = dummyData;
            }
            else
            {
                wordToSend = ((uint32_t)dummyData << 8) | dummyData;
            }
            handle->lastCommand = (handle->lastCommand & 0xffff0000U) | wordToSend;
        }
    }

    /*For DSPI instances with separate RX and TX DMA requests: use the scatter/gather to prepare the last data
     * (handle->lastCommand) to handle->Command*/
    if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        transferConfigB.srcAddr = (uint32_t) & (handle->lastCommand);
        transferConfigB.destAddr = (uint32_t) & (handle->command);
        transferConfigB.srcTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigB.destTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigB.srcOffset = 0;
        transferConfigB.destOffset = 0;
        transferConfigB.minorLoopBytes = 4;
        transferConfigB.majorLoopCounts = 1;

        EDMA_TcdReset(softwareTCD);
        EDMA_TcdSetTransferConfig(softwareTCD, &transferConfigB, NULL);
    }

    /*User_Send_Buffer(txData) to intermediary(handle->command)*/
    if (((((handle->remainingSendByteCount > 2) && (handle->bitsPerFrame <= 8)) ||
          ((handle->remainingSendByteCount > 4) && (handle->bitsPerFrame > 8))) &&
         (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))) ||
        (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base)))
    {
        if (handle->txData)
        {
            transferConfigB.srcAddr = (uint32_t)(handle->txData);
            transferConfigB.srcOffset = 1;
        }
        else
        {
            transferConfigB.srcAddr = (uint32_t)(&handle->txBuffIfNull);
            transferConfigB.srcOffset = 0;
        }

        transferConfigB.destAddr = (uint32_t)(&handle->command);
        transferConfigB.destOffset = 0;

        transferConfigB.srcTransferSize = kEDMA_TransferSize1Bytes;

        if (handle->bitsPerFrame <= 8)
        {
            transferConfigB.destTransferSize = kEDMA_TransferSize1Bytes;
            transferConfigB.minorLoopBytes = 1;

            if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
            {
                transferConfigB.majorLoopCounts = handle->remainingSendByteCount - 2;
            }
            else
            {
                /*Only enable channel_B minorlink to channel_C , so need to add one count due to the last time is
                majorlink , the majorlink would not trigger the channel_C*/
                transferConfigB.majorLoopCounts = handle->remainingSendByteCount + 1;
            }
        }
        else
        {
            transferConfigB.destTransferSize = kEDMA_TransferSize2Bytes;
            transferConfigB.minorLoopBytes = 2;
            if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
            {
                transferConfigB.majorLoopCounts = handle->remainingSendByteCount / 2 - 2;
            }
            else
            {
                /*Only enable channel_B minorlink to channel_C , so need to add one count due to the last time is
                * majorlink*/
                transferConfigB.majorLoopCounts = handle->remainingSendByteCount / 2 + 1;
            }
        }

        if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
        {
            EDMA_SetTransferConfig(handle->edmaTxDataToIntermediaryHandle->base,
                                   handle->edmaTxDataToIntermediaryHandle->channel, &transferConfigB, softwareTCD);
            EDMA_EnableAutoStopRequest(handle->edmaIntermediaryToTxRegHandle->base,
                                       handle->edmaIntermediaryToTxRegHandle->channel, false);
        }
        else
        {
            EDMA_SetTransferConfig(handle->edmaTxDataToIntermediaryHandle->base,
                                   handle->edmaTxDataToIntermediaryHandle->channel, &transferConfigB, NULL);
        }
    }
    else
    {
        EDMA_SetTransferConfig(handle->edmaTxDataToIntermediaryHandle->base,
                               handle->edmaTxDataToIntermediaryHandle->channel, &transferConfigB, NULL);
    }

    /***channel_C ***carry the "intermediary" to SPIx_PUSHR. used the edma Scatter Gather function on channel_C to
    handle the last data */

    EDMA_ResetChannel(handle->edmaIntermediaryToTxRegHandle->base, handle->edmaIntermediaryToTxRegHandle->channel);

    /*For DSPI instances with shared RX/TX DMA requests: use the scatter/gather to prepare the last data
     * (handle->lastCommand) to SPI_PUSHR*/
    if (((1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base)) && (handle->remainingSendByteCount > 0)))
    {
        transferConfigC.srcAddr = (uint32_t) & (handle->lastCommand);
        transferConfigC.destAddr = (uint32_t)txAddr;
        transferConfigC.srcTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigC.destTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigC.srcOffset = 0;
        transferConfigC.destOffset = 0;
        transferConfigC.minorLoopBytes = 4;
        transferConfigC.majorLoopCounts = 1;

        EDMA_TcdReset(softwareTCD);
        EDMA_TcdSetTransferConfig(softwareTCD, &transferConfigC, NULL);
    }

    if (((handle->remainingSendByteCount > 1) && (handle->bitsPerFrame <= 8)) ||
        ((handle->remainingSendByteCount > 2) && (handle->bitsPerFrame > 8)) ||
        (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base)))
    {
        transferConfigC.srcAddr = (uint32_t)(&(handle->command));
        transferConfigC.destAddr = (uint32_t)txAddr;

        transferConfigC.srcTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigC.destTransferSize = kEDMA_TransferSize4Bytes;
        transferConfigC.srcOffset = 0;
        transferConfigC.destOffset = 0;
        transferConfigC.minorLoopBytes = 4;
        if (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
        {
            if (handle->bitsPerFrame <= 8)
            {
                transferConfigC.majorLoopCounts = handle->remainingSendByteCount - 1;
            }
            else
            {
                transferConfigC.majorLoopCounts = handle->remainingSendByteCount / 2 - 1;
            }

            EDMA_SetTransferConfig(handle->edmaIntermediaryToTxRegHandle->base,
                                   handle->edmaIntermediaryToTxRegHandle->channel, &transferConfigC, softwareTCD);
        }
        else
        {
            transferConfigC.majorLoopCounts = 1;

            EDMA_SetTransferConfig(handle->edmaIntermediaryToTxRegHandle->base,
                                   handle->edmaIntermediaryToTxRegHandle->channel, &transferConfigC, NULL);
        }

        EDMA_EnableAutoStopRequest(handle->edmaIntermediaryToTxRegHandle->base,
                                   handle->edmaIntermediaryToTxRegHandle->channel, false);
    }
    else
    {
        EDMA_SetTransferConfig(handle->edmaIntermediaryToTxRegHandle->base,
                               handle->edmaIntermediaryToTxRegHandle->channel, &transferConfigC, NULL);
    }

    /*Start the EDMA channel_A , channel_B , channel_C transfer*/
    EDMA_StartTransfer(handle->edmaRxRegToRxDataHandle);
    EDMA_StartTransfer(handle->edmaTxDataToIntermediaryHandle);
    EDMA_StartTransfer(handle->edmaIntermediaryToTxRegHandle);

    /*Set channel priority*/
    uint8_t channelPriorityLow = handle->edmaRxRegToRxDataHandle->channel;
    uint8_t channelPriorityMid = handle->edmaTxDataToIntermediaryHandle->channel;
    uint8_t channelPriorityHigh = handle->edmaIntermediaryToTxRegHandle->channel;
    uint8_t t = 0;
    if (channelPriorityLow > channelPriorityMid)
    {
        t = channelPriorityLow;
        channelPriorityLow = channelPriorityMid;
        channelPriorityMid = t;
    }

    if (channelPriorityLow > channelPriorityHigh)
    {
        t = channelPriorityLow;
        channelPriorityLow = channelPriorityHigh;
        channelPriorityHigh = t;
    }

    if (channelPriorityMid > channelPriorityHigh)
    {
        t = channelPriorityMid;
        channelPriorityMid = channelPriorityHigh;
        channelPriorityHigh = t;
    }
    edma_channel_Preemption_config_t preemption_config_t;
    preemption_config_t.enableChannelPreemption = true;
    preemption_config_t.enablePreemptAbility = true;
    preemption_config_t.channelPriority = channelPriorityLow;

    if (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_SetChannelPreemptionConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                        &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityMid;
        EDMA_SetChannelPreemptionConfig(handle->edmaTxDataToIntermediaryHandle->base,
                                        handle->edmaTxDataToIntermediaryHandle->channel, &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityHigh;
        EDMA_SetChannelPreemptionConfig(handle->edmaIntermediaryToTxRegHandle->base,
                                        handle->edmaIntermediaryToTxRegHandle->channel, &preemption_config_t);
    }
    else
    {
        EDMA_SetChannelPreemptionConfig(handle->edmaIntermediaryToTxRegHandle->base,
                                        handle->edmaIntermediaryToTxRegHandle->channel, &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityMid;
        EDMA_SetChannelPreemptionConfig(handle->edmaTxDataToIntermediaryHandle->base,
                                        handle->edmaTxDataToIntermediaryHandle->channel, &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityHigh;
        EDMA_SetChannelPreemptionConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                        &preemption_config_t);
    }

    /*Set the channel link.*/
    if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        /*if there is Tx DMA request , carry the 32bits data (handle->command) to PUSHR first , then link to channelB
        to prepare the next 32bits data (txData to handle->command) */
        if (handle->remainingSendByteCount > 1)
        {
            EDMA_SetChannelLink(handle->edmaIntermediaryToTxRegHandle->base,
                                handle->edmaIntermediaryToTxRegHandle->channel, kEDMA_MajorLink,
                                handle->edmaTxDataToIntermediaryHandle->channel);
        }

        DSPI_EnableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);
    }
    else
    {
        if (handle->remainingSendByteCount > 0)
        {
            EDMA_SetChannelLink(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                kEDMA_MinorLink, handle->edmaTxDataToIntermediaryHandle->channel);

            EDMA_SetChannelLink(handle->edmaTxDataToIntermediaryHandle->base,
                                handle->edmaTxDataToIntermediaryHandle->channel, kEDMA_MinorLink,
                                handle->edmaIntermediaryToTxRegHandle->channel);
        }

        DSPI_EnableDMA(base, kDSPI_RxDmaEnable);
    }

    DSPI_StartTransfer(base);

    return kStatus_Success;
}

static void EDMA_DspiMasterCallback(edma_handle_t *edmaHandle,
                                    void *g_dspiEdmaPrivateHandle,
                                    bool transferDone,
                                    uint32_t tcds)
{
    assert(edmaHandle);
    assert(g_dspiEdmaPrivateHandle);

    dspi_master_edma_private_handle_t *dspiEdmaPrivateHandle;

    dspiEdmaPrivateHandle = (dspi_master_edma_private_handle_t *)g_dspiEdmaPrivateHandle;

    DSPI_DisableDMA((dspiEdmaPrivateHandle->base), kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    dspiEdmaPrivateHandle->handle->state = kDSPI_Idle;

    if (dspiEdmaPrivateHandle->handle->callback)
    {
        dspiEdmaPrivateHandle->handle->callback(dspiEdmaPrivateHandle->base, dspiEdmaPrivateHandle->handle,
                                                kStatus_Success, dspiEdmaPrivateHandle->handle->userData);
    }
}

void DSPI_MasterTransferAbortEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle)
{
    assert(handle);

    DSPI_StopTransfer(base);

    DSPI_DisableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    EDMA_AbortTransfer(handle->edmaRxRegToRxDataHandle);
    EDMA_AbortTransfer(handle->edmaTxDataToIntermediaryHandle);
    EDMA_AbortTransfer(handle->edmaIntermediaryToTxRegHandle);

    handle->state = kDSPI_Idle;
}

status_t DSPI_MasterTransferGetCountEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kDSPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    size_t bytes;

    bytes = (uint32_t)handle->nbytes * EDMA_GetRemainingMajorLoopCount(handle->edmaRxRegToRxDataHandle->base,
                                                                       handle->edmaRxRegToRxDataHandle->channel);

    *count = handle->totalByteCount - bytes;

    return kStatus_Success;
}

void DSPI_SlaveTransferCreateHandleEDMA(SPI_Type *base,
                                        dspi_slave_edma_handle_t *handle,
                                        dspi_slave_edma_transfer_callback_t callback,
                                        void *userData,
                                        edma_handle_t *edmaRxRegToRxDataHandle,
                                        edma_handle_t *edmaTxDataToTxRegHandle)
{
    assert(handle);
    assert(edmaRxRegToRxDataHandle);
    assert(edmaTxDataToTxRegHandle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    uint32_t instance = DSPI_GetInstance(base);

    s_dspiSlaveEdmaPrivateHandle[instance].base = base;
    s_dspiSlaveEdmaPrivateHandle[instance].handle = handle;

    handle->callback = callback;
    handle->userData = userData;

    handle->edmaRxRegToRxDataHandle = edmaRxRegToRxDataHandle;
    handle->edmaTxDataToTxRegHandle = edmaTxDataToTxRegHandle;
}

status_t DSPI_SlaveTransferEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle, dspi_transfer_t *transfer)
{
    assert(handle);
    assert(transfer);

    /* If send/receive length is zero */
    if (transfer->dataSize == 0)
    {
        return kStatus_InvalidArgument;
    }

    /* If both send buffer and receive buffer is null */
    if ((!(transfer->txData)) && (!(transfer->rxData)))
    {
        return kStatus_InvalidArgument;
    }

    /* Check that we're not busy.*/
    if (handle->state == kDSPI_Busy)
    {
        return kStatus_DSPI_Busy;
    }

    handle->state = kDSPI_Busy;

    uint32_t instance = DSPI_GetInstance(base);
    uint8_t whichCtar = (transfer->configFlags & DSPI_SLAVE_CTAR_MASK) >> DSPI_SLAVE_CTAR_SHIFT;
    handle->bitsPerFrame =
        (((base->CTAR_SLAVE[whichCtar]) & SPI_CTAR_SLAVE_FMSZ_MASK) >> SPI_CTAR_SLAVE_FMSZ_SHIFT) + 1;

    /* If using a shared RX/TX DMA request, then this limits the amount of data we can transfer
    * due to the linked channel. The max bytes is 511 if 8-bit/frame or 1022 if 16-bit/frame
    */
    uint32_t limited_size = 0;
    if (1 == FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        limited_size = 32767u;
    }
    else
    {
        limited_size = 511u;
    }

    if (handle->bitsPerFrame > 8)
    {
        if (transfer->dataSize > (limited_size << 1u))
        {
            handle->state = kDSPI_Idle;
            return kStatus_DSPI_OutOfRange;
        }
    }
    else
    {
        if (transfer->dataSize > limited_size)
        {
            handle->state = kDSPI_Idle;
            return kStatus_DSPI_OutOfRange;
        }
    }

    /*The data size should be even if the bitsPerFrame is greater than 8 (that is 2 bytes per frame in dspi) */
    if ((handle->bitsPerFrame > 8) && (transfer->dataSize & 0x1))
    {
        handle->state = kDSPI_Idle;
        return kStatus_InvalidArgument;
    }

    EDMA_SetCallback(handle->edmaRxRegToRxDataHandle, EDMA_DspiSlaveCallback, &s_dspiSlaveEdmaPrivateHandle[instance]);

    /* Store transfer information */
    handle->txData = transfer->txData;
    handle->rxData = transfer->rxData;
    handle->remainingSendByteCount = transfer->dataSize;
    handle->remainingReceiveByteCount = transfer->dataSize;
    handle->totalByteCount = transfer->dataSize;

    uint16_t wordToSend = 0;
    uint8_t dummyData = DSPI_DUMMY_DATA;
    uint8_t dataAlreadyFed = 0;
    uint8_t dataFedMax = 2;

    uint32_t rxAddr = DSPI_GetRxRegisterAddress(base);
    uint32_t txAddr = DSPI_SlaveGetTxRegisterAddress(base);

    edma_transfer_config_t transferConfigA;
    edma_transfer_config_t transferConfigC;

    DSPI_StopTransfer(base);

    DSPI_FlushFifo(base, true, true);
    DSPI_ClearStatusFlags(base, kDSPI_AllStatusFlag);

    DSPI_DisableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    DSPI_StartTransfer(base);

    /*if dspi has separate dma request , need not prepare data first .
    else (dspi has shared dma request) , send first 2 data into fifo if there is fifo or send first 1 data to
    slaveGetTxRegister if there is no fifo*/
    if (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        /* For DSPI instances with shared RX/TX DMA requests, we'll use the RX DMA request to
        * trigger ongoing transfers and will link to the TX DMA channel from the RX DMA channel.
        */
        /* If bits/frame is greater than one byte */
        if (handle->bitsPerFrame > 8)
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag)
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    ++handle->txData; /* Increment to next data byte */

                    wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                    ++handle->txData; /* Increment to next data byte */
                }
                else
                {
                    wordToSend = ((uint32_t)dummyData << 8) | dummyData;
                }
                handle->remainingSendByteCount -= 2; /* decrement remainingSendByteCount by 2 */
                base->PUSHR_SLAVE = wordToSend;

                /* Try to clear the TFFF; if the TX FIFO is full this will clear */
                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

                dataAlreadyFed += 2;

                /* Exit loop if send count is zero, else update local variables for next loop */
                if ((handle->remainingSendByteCount == 0) || (dataAlreadyFed == (dataFedMax * 2)))
                {
                    break;
                }
            } /* End of TX FIFO fill while loop */
        }
        else /* Optimized for bits/frame less than or equal to one byte. */
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag)
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    /* Increment to next data word*/
                    ++handle->txData;
                }
                else
                {
                    wordToSend = dummyData;
                }

                base->PUSHR_SLAVE = wordToSend;

                /* Try to clear the TFFF; if the TX FIFO is full this will clear */
                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                /* Decrement remainingSendByteCount*/
                --handle->remainingSendByteCount;

                dataAlreadyFed++;

                /* Exit loop if send count is zero, else update local variables for next loop */
                if ((handle->remainingSendByteCount == 0) || (dataAlreadyFed == dataFedMax))
                {
                    break;
                }
            } /* End of TX FIFO fill while loop */
        }
    }

    /***channel_A *** used for carry the data from Rx_Data_Register(POPR) to User_Receive_Buffer*/
    if (handle->remainingReceiveByteCount > 0)
    {
        EDMA_ResetChannel(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel);

        transferConfigA.srcAddr = (uint32_t)rxAddr;
        transferConfigA.srcOffset = 0;

        if (handle->rxData)
        {
            transferConfigA.destAddr = (uint32_t) & (handle->rxData[0]);
            transferConfigA.destOffset = 1;
        }
        else
        {
            transferConfigA.destAddr = (uint32_t) & (handle->rxBuffIfNull);
            transferConfigA.destOffset = 0;
        }

        transferConfigA.destTransferSize = kEDMA_TransferSize1Bytes;

        if (handle->bitsPerFrame <= 8)
        {
            transferConfigA.srcTransferSize = kEDMA_TransferSize1Bytes;
            transferConfigA.minorLoopBytes = 1;
            transferConfigA.majorLoopCounts = handle->remainingReceiveByteCount;
        }
        else
        {
            transferConfigA.srcTransferSize = kEDMA_TransferSize2Bytes;
            transferConfigA.minorLoopBytes = 2;
            transferConfigA.majorLoopCounts = handle->remainingReceiveByteCount / 2;
        }

        /* Store the initially configured eDMA minor byte transfer count into the DSPI handle */
        handle->nbytes = transferConfigA.minorLoopBytes;

        EDMA_SetTransferConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                               &transferConfigA, NULL);
        EDMA_EnableChannelInterrupts(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                     kEDMA_MajorInterruptEnable);
    }

    if (handle->remainingSendByteCount > 0)
    {
        /***channel_C *** used for carry the data from User_Send_Buffer to Tx_Data_Register(PUSHR_SLAVE)*/
        EDMA_ResetChannel(handle->edmaTxDataToTxRegHandle->base, handle->edmaTxDataToTxRegHandle->channel);

        transferConfigC.destAddr = (uint32_t)txAddr;
        transferConfigC.destOffset = 0;

        if (handle->txData)
        {
            transferConfigC.srcAddr = (uint32_t)(&(handle->txData[0]));
            transferConfigC.srcOffset = 1;
        }
        else
        {
            transferConfigC.srcAddr = (uint32_t)(&handle->txBuffIfNull);
            transferConfigC.srcOffset = 0;
            if (handle->bitsPerFrame <= 8)
            {
                handle->txBuffIfNull = DSPI_DUMMY_DATA;
            }
            else
            {
                handle->txBuffIfNull = (DSPI_DUMMY_DATA << 8) | DSPI_DUMMY_DATA;
            }
        }

        transferConfigC.srcTransferSize = kEDMA_TransferSize1Bytes;

        if (handle->bitsPerFrame <= 8)
        {
            transferConfigC.destTransferSize = kEDMA_TransferSize1Bytes;
            transferConfigC.minorLoopBytes = 1;
            transferConfigC.majorLoopCounts = handle->remainingSendByteCount;
        }
        else
        {
            transferConfigC.destTransferSize = kEDMA_TransferSize2Bytes;
            transferConfigC.minorLoopBytes = 2;
            transferConfigC.majorLoopCounts = handle->remainingSendByteCount / 2;
        }

        EDMA_SetTransferConfig(handle->edmaTxDataToTxRegHandle->base, handle->edmaTxDataToTxRegHandle->channel,
                               &transferConfigC, NULL);

        EDMA_StartTransfer(handle->edmaTxDataToTxRegHandle);
    }

    EDMA_StartTransfer(handle->edmaRxRegToRxDataHandle);

    /*Set channel priority*/
    uint8_t channelPriorityLow = handle->edmaRxRegToRxDataHandle->channel;
    uint8_t channelPriorityHigh = handle->edmaTxDataToTxRegHandle->channel;
    uint8_t t = 0;

    if (channelPriorityLow > channelPriorityHigh)
    {
        t = channelPriorityLow;
        channelPriorityLow = channelPriorityHigh;
        channelPriorityHigh = t;
    }

    edma_channel_Preemption_config_t preemption_config_t;
    preemption_config_t.enableChannelPreemption = true;
    preemption_config_t.enablePreemptAbility = true;
    preemption_config_t.channelPriority = channelPriorityLow;

    if (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_SetChannelPreemptionConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                        &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityHigh;
        EDMA_SetChannelPreemptionConfig(handle->edmaTxDataToTxRegHandle->base, handle->edmaTxDataToTxRegHandle->channel,
                                        &preemption_config_t);
    }
    else
    {
        EDMA_SetChannelPreemptionConfig(handle->edmaTxDataToTxRegHandle->base, handle->edmaTxDataToTxRegHandle->channel,
                                        &preemption_config_t);

        preemption_config_t.channelPriority = channelPriorityHigh;
        EDMA_SetChannelPreemptionConfig(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                        &preemption_config_t);
    }

    /*Set the channel link.
    For DSPI instances with shared RX/TX DMA requests: Rx DMA request -> channel_A -> channel_C.
    For DSPI instances with separate RX and TX DMA requests:
    Rx DMA request -> channel_A
    Tx DMA request -> channel_C */
    if (1 != FSL_FEATURE_DSPI_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        if (handle->remainingSendByteCount > 0)
        {
            EDMA_SetChannelLink(handle->edmaRxRegToRxDataHandle->base, handle->edmaRxRegToRxDataHandle->channel,
                                kEDMA_MinorLink, handle->edmaTxDataToTxRegHandle->channel);
        }
        DSPI_EnableDMA(base, kDSPI_RxDmaEnable);
    }
    else
    {
        DSPI_EnableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);
    }

    return kStatus_Success;
}

static void EDMA_DspiSlaveCallback(edma_handle_t *edmaHandle,
                                   void *g_dspiEdmaPrivateHandle,
                                   bool transferDone,
                                   uint32_t tcds)
{
    assert(edmaHandle);
    assert(g_dspiEdmaPrivateHandle);

    dspi_slave_edma_private_handle_t *dspiEdmaPrivateHandle;

    dspiEdmaPrivateHandle = (dspi_slave_edma_private_handle_t *)g_dspiEdmaPrivateHandle;

    DSPI_DisableDMA((dspiEdmaPrivateHandle->base), kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    dspiEdmaPrivateHandle->handle->state = kDSPI_Idle;

    if (dspiEdmaPrivateHandle->handle->callback)
    {
        dspiEdmaPrivateHandle->handle->callback(dspiEdmaPrivateHandle->base, dspiEdmaPrivateHandle->handle,
                                                kStatus_Success, dspiEdmaPrivateHandle->handle->userData);
    }
}

void DSPI_SlaveTransferAbortEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle)
{
    assert(handle);

    DSPI_StopTransfer(base);

    DSPI_DisableDMA(base, kDSPI_RxDmaEnable | kDSPI_TxDmaEnable);

    EDMA_AbortTransfer(handle->edmaRxRegToRxDataHandle);
    EDMA_AbortTransfer(handle->edmaTxDataToTxRegHandle);

    handle->state = kDSPI_Idle;
}

status_t DSPI_SlaveTransferGetCountEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kDSPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    size_t bytes;

    bytes = (uint32_t)handle->nbytes * EDMA_GetRemainingMajorLoopCount(handle->edmaRxRegToRxDataHandle->base,
                                                                       handle->edmaRxRegToRxDataHandle->channel);

    *count = handle->totalByteCount - bytes;

    return kStatus_Success;
}
