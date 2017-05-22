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
#include "MKW41Z4.h"
#include "fsl_xcvr.h"
#include "fsl_edma.h"
#include "fsl_dmamux.h"
#include "dma_capture.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PKT_RAM_SIZE_16B_WORDS  (544) /* Number of 16bit entries in each Packet RAM */
#define SIGN_EXTND_12_16(x)     ((x) | (((x) & 0x800) ? 0xF000 : 0x0))
#define SIGN_EXTND_5_8(x)       ((x) | (((x) & 0x10) ? 0xE0 : 0x0))

const dma_capture_lut_t dma_table [DMA_PAGE_MAX] = 
{
  /* DMA_PAGE               OSR */
    {DMA_PAGE_IDLE,         0xF},
    {DMA_PAGE_RXDIGIQ,      0x4},
    {DMA_PAGE_RXDIGI,       0x2},
    {DMA_PAGE_RXDIGQ,       0x2},
    {DMA_PAGE_RAWADCIQ,     0x2},
    {DMA_PAGE_RAWADCI,      0x1},
    {DMA_PAGE_RAWADCQ,      0x1},
    {DMA_PAGE_DCESTIQ,      0x4},
    {DMA_PAGE_DCESTI,       0x2},
    {DMA_PAGE_DCESTQ,       0x2},
    {DMA_PAGE_RXINPH,       0x1},
    {DMA_PAGE_DEMOD_HARD,   0xF},
    {DMA_PAGE_DEMOD_SOFT,   0xF},
    {DMA_PAGE_DEMOD_DATA,   0xF},
    {DMA_PAGE_DEMOD_CFO_PH, 0xF},  
};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Initialize to an invalid value. */
uint8_t osr_temp = 0xF;
edma_handle_t g_EDMA_Handle;
volatile bool g_Transfer_Done = false;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* User callback function for EDMA transfer. */
void EDMA_Callback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    if (transferDone)
    {
        g_Transfer_Done = true;
    }
}

/*******************************************************************************
 * Code
 ******************************************************************************/
void dma_init(void)
{   
    edma_config_t dmaConfig;
    
    /* Configure DMAMUX */
    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, 0, 11);
    DMAMUX_EnableChannel(DMAMUX0, 0);
    
    /* Configure EDMA transfer */
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DMA0, &dmaConfig);
    EDMA_CreateHandle(&g_EDMA_Handle, DMA0, 0);
    EDMA_SetCallback(&g_EDMA_Handle, EDMA_Callback, NULL);
    
    /* Turns on clocking to DMA/DBG blocks */
    XCVR_RX_DIG->RX_DIG_CTRL |= XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_MASK;
    /* Save current osr config */
    osr_temp = (XCVR_RX_DIG->RX_DIG_CTRL & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK) >> XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_SHIFT;

    /* Some external code must perform the RX warmup request */
}

dmaStatus_t dma_wait_for_complete(void)
{
    dmaStatus_t status = DMA_SUCCESS;
    
    /* Wait for EDMA transfer finish */
    while (g_Transfer_Done != true)
    {
        /* Timeout */
        if (XCVR_MISC->DMA_CTRL & XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK)
        {
            /* Clear flag */
            XCVR_MISC->DMA_CTRL |= XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK;

            /* Capture error */
            status = DMA_CAPTURE_NOT_COMPLETE;
            
            break;
        }
    }
    
    /* Release DMA module */
    dma_release();
    
    return status;
}

dmaStatus_t dma_poll_capture_status(void)
{  
  dmaStatus_t status = DMA_SUCCESS;
  
  if (g_Transfer_Done != true)
  {
      /* Timeout */
      if (XCVR_MISC->DMA_CTRL & XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK)
      {
          /* Clear flag */
          XCVR_MISC->DMA_CTRL |= XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK;
          /* Release DMA module */
          dma_release();
          /* Capture error */
          status = DMA_CAPTURE_NOT_COMPLETE;
      }
  }
  
  return status;
}

void dma_release(void)
{   
    /* Restart flag */
    g_Transfer_Done = false;
    
    /* Disable DMA */
    XCVR_MISC->DMA_CTRL &= ~XCVR_CTRL_DMA_CTRL_DMA_PAGE_MASK; 
    
    /* Gasket bypass disable */
    RSIM->CONTROL &= ~(RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_MASK | 
                       RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_MASK); 
    
    /* Restore previous osr config (if any) */
    if (osr_temp != 0xF)
    {
        XCVR_RX_DIG->RX_DIG_CTRL &= ~XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK;
        XCVR_RX_DIG->RX_DIG_CTRL |= XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(osr_temp);
        osr_temp = 0xF;
    }
}

#if RADIO_IS_GEN_3P0
dmaStatus_t dma_start_capture(uint8_t dbg_page, uint16_t buffer_sz_bytes, void * result_buffer, dmaStartTriggerType start_trig)
{
	
}
#else
dmaStatus_t dma_capture(uint8_t dma_page, uint16_t buffer_sz_bytes, uint32_t * result_buffer)
{
    dmaStatus_t status = DMA_SUCCESS;
    
    /* Some external code must perform the RX warmup request after the d_init() call. */
    if (result_buffer == NULL)
    {
        status = DMA_FAIL_NULL_POINTER;
    }
    else
    {
        if (buffer_sz_bytes > (4096))
        {
            status = DMA_FAIL_SAMPLE_NUM_LIMIT;
        }
        else
        {
            uint32_t temp = XCVR_MISC->DMA_CTRL & ~XCVR_CTRL_DMA_CTRL_DMA_PAGE_MASK;
            edma_transfer_config_t transferConfig;
            
            /* Wait for Network Address Match to start capturing */
//            while (!((XCVR_MISC->DMA_CTRL & XCVR_CTRL_D  MA_CTRL_DMA_TRIGGERRED_MASK))); 
            
            switch (dma_page)
            {
            case DMA_PAGE_RXDIGIQ:
            case DMA_PAGE_RAWADCIQ:
            case DMA_PAGE_DCESTIQ:
            case DMA_PAGE_RXINPH:
            case DMA_PAGE_DEMOD_CFO_PH:
                
                /* Set OSR */
                XCVR_RX_DIG->RX_DIG_CTRL &= ~XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK;
                XCVR_RX_DIG->RX_DIG_CTRL |= XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(dma_table[dma_page].osr); 
                
                /* Single request mode */
                XCVR_MISC->DMA_CTRL |= XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_MASK;
                XCVR_MISC->DMA_CTRL &= ~XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_MASK;
                
                /* Gasket bypass disable */
                RSIM->CONTROL &= ~(RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_MASK | 
                                   RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_MASK); 
                break;
                                
            case DMA_PAGE_DEMOD_HARD:
            case DMA_PAGE_DEMOD_SOFT:
            case DMA_PAGE_DEMOD_DATA:
                /* Data rate is too low to use Single Req mode */
                
                /* Request per sample mode */
                XCVR_MISC->DMA_CTRL &= XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_MASK;
                XCVR_MISC->DMA_CTRL |= XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_MASK;
                
                /* Gasket bypass enable */
                RSIM->CONTROL |= (RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_MASK | 
                                  RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_MASK);              
                break;
                
            case DMA_PAGE_IDLE:
            default:    
                /* Illegal capture page request */
                status = DMA_FAIL_PAGE_ERROR;
                break;
            }
            
            if (status == DMA_SUCCESS)
            {
                /* Select DMA_page */
                XCVR_MISC->DMA_CTRL =  temp | XCVR_CTRL_DMA_CTRL_DMA_PAGE(dma_page);
                
                /* EDMA transfer config */
                EDMA_PrepareTransfer(&transferConfig, /* Config */
                                     (uint32_t *)&XCVR_MISC->DMA_DATA, /* Source address */
                                     sizeof(uint32_t), /* Source address width(bytes) */
                                     result_buffer, /* Destination address */
                                     sizeof(*result_buffer), /* Destination address width */
                                     sizeof(uint32_t), /* Transfer bytes per channel request */
                                     buffer_sz_bytes, /* Bytes to be transferred */
                                     kEDMA_PeripheralToMemory); /* Transfer type */
                
                EDMA_SubmitTransfer(&g_EDMA_Handle, &transferConfig);                                  
                EDMA_StartTransfer(&g_EDMA_Handle);
            }
        }
    }
    
    return status;	
}
#endif /* RADIO_IS_GEN_3P0 */
