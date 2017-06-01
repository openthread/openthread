/*
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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
#ifndef _DMA_CAPTURE_H_
#define _DMA_CAPTURE_H_

#include "fsl_common.h"

#if RADIO_IS_GEN_3P0
#include "K32W042S1M2_M0P.h"
#else
#include "fsl_device_registers.h"
#endif /* RADIO_IS_GEN_3P0 */

/*!
 * @addtogroup xcvr
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Page definitions */
#define DMA_PAGE_IDLE           (0x00)
#define DMA_PAGE_RXDIGIQ        (0x01)
#define DMA_PAGE_RXDIGI         (0x02)
#define DMA_PAGE_RXDIGQ         (0x03)
#define DMA_PAGE_RAWADCIQ       (0x04)
#define DMA_PAGE_RAWADCI        (0x05)
#define DMA_PAGE_RAWADCQ        (0x06)
#define DMA_PAGE_DCESTIQ        (0x07)
#define DMA_PAGE_DCESTI         (0x08)
#define DMA_PAGE_DCESTQ         (0x09)
#define DMA_PAGE_RXINPH         (0x0A)
#define DMA_PAGE_DEMOD_HARD     (0x0B)
#define DMA_PAGE_DEMOD_SOFT     (0x0C)
#define DMA_PAGE_DEMOD_DATA     (0x0D)
#define DMA_PAGE_DEMOD_CFO_PH   (0x0E)

#define DMA_PAGE_MAX   (0x0F)

typedef struct
{
    uint8_t dma_page;
    uint8_t osr;
} dma_capture_lut_t;

typedef enum _dmaStatus
{
    DMA_SUCCESS = 0,
    DMA_FAIL_SAMPLE_NUM_LIMIT = 1,
    DMA_FAIL_PAGE_ERROR = 2,
    DMA_FAIL_NULL_POINTER = 3,
    DMA_INVALID_TRIG_SETTING = 4,
    DMA_FAIL_NOT_ENOUGH_SAMPLES = 5,
    DMA_CAPTURE_NOT_COMPLETE = 6,     /* Not an error response, but an indication that capture isn't complete for status polling. */
} dmaStatus_t; 

#if RADIO_IS_GEN_3P0
typedef enum dmaStartTriggerType
{
    NO_DMA_START_TRIG = 0,
    START_DMA_ON_FSK_PREAMBLE_FOUND = 1,
    START_DMA_ON_FSK_AA_MATCH = 2,
    START_DMA_ON_ZBDEMOD_PREAMBLE_FOUND = 3,
    START_DMA_ON_ZBDEMOD_SFD_MATCH = 4,
    START_DMA_ON_AGC_DCOC_GAIN_CHG = 5,
    START_DMA_ON_TSM_RX_DIG_EN = 6,
    START_ODMA_N_TSM_SPARE2_EN = 7,
    INVALID_DMA_START_TRIG = 8
} dmaStartTriggerType;
#endif /* RADIO_IS_GEN_3P0 */

/*! *********************************************************************************
* \brief  This function prepares for sample capture to system RAM using DMA.
*
* \return None.
*
* \details 
* This routine assumes that some other functions in the calling routine both set
* the channel and force RX warmup before calling the appropriate capture routine.
* This routine may corrupt the OSR value of the system but restores it in the ::dma_release() routine.
***********************************************************************************/
void dma_init(void);

/*! *********************************************************************************
* \brief  This function performs a blocking wait for completion of the capture of transceiver data to the system RAM.
*
* \return Status of the request, DMA_SUCCESS if capture is complete.
*
* \details 
* This function performs a wait loop for capture completion and may take an indeterminate amount of time for 
* some capture trigger types.
* Blocking wait for capture completion, no matter what trigger type.
***********************************************************************************/
dmaStatus_t dma_wait_for_complete(void);

/*! *********************************************************************************
* \brief  This function polls the state of the capture of transceiver data to the transceiver packet RAM.
*
* \return Status of the request, DMA_SUCCESS if capture is complete, DMA_CAPTURE_NOT_COMPLETE if not complete.
*
* \details
* Non-blocking completion check, just reads the current status of the capure.
***********************************************************************************/
dmaStatus_t dma_poll_capture_status(void);

/*! *********************************************************************************
* \brief  This function performs any state restoration at the completion of DMA capture to system RAM.
* 
* \details
* Because DMA capture only works at specific OSR values the DMA capture routines must force the OSR to the
* supported value when capturing data. This forced OSR value must be restored after DMA operations are complete.
***********************************************************************************/
void dma_release(void);

#if RADIO_IS_GEN_3P0
/*! *********************************************************************************
* \brief  This function initiates the capture of transceiver data to the system RAM.
*
* \param[in] dbg_page - The page selector (DBG_PAGE).
* \param[in] buffer_sz_bytes - The size of the output buffer (in bytes)
* \param[in] result_buffer - The pointer to the output buffer of a size large enough for the samples.
* \param[in] dbg_start_trigger - The trigger to start acquisition (must be "no trigger" if a stop trigger is enabled).
*
* \return Status of the request.
*
* \details 
* This function starts the process of capturing data to the system RAM. Depending upon the start  trigger
* settings, the actual capture process can take an indeterminate amount of time. Other APIs are provided to 
* perform a blocking wait for completion or allow polling for completion of the capture.
* After any capture has completed, data will be left in system RAM.
***********************************************************************************/
dmaStatus_t dma_start_capture(uint8_t dbg_page, uint16_t buffer_sz_bytes, void * result_buffer, dmaStartTriggerType start_trig);

#else /* Gen 2.0 version */
/*! *********************************************************************************
* \brief  This function captures transceiver data to the transceiver packet RAM.
*
* \param[in] dma_page - The page selector (DBG_PAGE).
* \param[in] buffer_sz_bytes - The size of the output buffer (in bytes)
* \param[in] result_buffer - The pointer to the output buffer of a size large enough for the samples.
*
* \return None.
*
* \details 
* The capture to system RAM . The samples will be 
* copied to the buffer pointed to by result_buffer parameter until buffer_sz_bytes worth of data have
* been copied. Data will be copied 
* NOTE: This routine has a slight hazard of getting stuck waiting for the specified number of butes when RX has
* not been enabled or RX ends before the specified byte count is achieved (such as when capturing packet data ). It is 
* intended to be used with manually triggered RX where RX data will continue as long as needed.
***********************************************************************************/
dmaStatus_t dma_capture(uint8_t dma_page, uint16_t buffer_sz_bytes, uint32_t * result_buffer);
#endif /* RADIO_IS_GEN_3P0 */

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _DMA_CAPTURE_H_ */


