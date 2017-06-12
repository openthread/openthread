/**
 ****************************************************************************************
 *
 * @file ftdf.c
 *
 * @brief FTDF FreeRTOS Adapter
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
#ifdef CONFIG_USE_FTDF

#ifdef FTDF_PHY_API

#include <string.h>
#include <stdbool.h>

#include "sdk_defs.h"
#include "ad_ftdf.h"
#include "ad_ftdf_phy_api.h"
#include "internal.h"
#if FTDF_DBG_BUS_ENABLE
#include "hw_gpio.h"
#endif /* FTDF_DBG_BUS_ENABLE */

#ifndef OS_FREERTOS
static unsigned long uxCriticalNesting = 0xaaaaaaaa;

void vPortEnterCritical(void)
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
    __asm volatile("dsb");
    __asm volatile("isb");
}

/*-----------------------------------------------------------*/

void vPortExitCritical(void)
{

    uxCriticalNesting--;

    if (uxCriticalNesting == 0)
    {
        portENABLE_INTERRUPTS();
    }
}
#endif /* !OS_FREERTOS */

/**
 * @brief ftdf_gen_irq interrupt service routine
 */
void FTDF_GEN_Handler(void)
{
    FTDF_confirmLmacInterrupt();
    FTDF_eventHandler();
}
/**
 * @brief ftdf_wakup_irq interrupt service routine
 */
void FTDF_WAKEUP_Handler(void)
{
    ad_ftdf_wake_up_async();
}

void ad_ftdf_setExtAddress(FTDF_ExtAddress address)
{
    uExtAddress = address;
}

FTDF_ExtAddress ad_ftdf_getExtAddress(void)
{
    return uExtAddress;
}

void ad_ftdf_wakeUpReady(void)
{
}

FTDF_Status ad_ftdf_send_frame_simple(FTDF_DataLength    frameLength,
                                      FTDF_Octet        *frame,
                                      FTDF_ChannelNumber channel,
                                      FTDF_PTI           pti,
                                      FTDF_Boolean       csmaSuppress)
{
    FTDF_criticalVar();
    FTDF_enterCritical();

    if (FTDF_txInProgress == FTDF_TRUE)
    {
        FTDF_exitCritical();
        return FTDF_TRANSPARENT_OVERFLOW;
    }

    FTDF_txInProgress = FTDF_TRUE;
    FTDF_exitCritical();

    if (sleep_status == BLOCK_SLEEPING)
    {
        /* Wake-up the block */
        ad_ftdf_wake_up_async();
#if FTDF_DBG_BUS_ENABLE
        FTDF_checkDbgMode();
#endif /* FTDF_DBG_BUS_ENABLE */
        sleep_status = BLOCK_ACTIVE;
    }

    return FTDF_sendFrameSimple(frameLength, frame, channel, pti, csmaSuppress);
}


void ad_ftdf_sleep_when_possible(FTDF_Boolean allow_deferred_sleep)
{
    sleep_when_possible(allow_deferred_sleep, 0);
}

void ad_ftdf_wake_up(void)
{
    if (sleep_status == BLOCK_SLEEPING)
    {
        /* Wake-up the block */
        ad_ftdf_wake_up_async();
#if FTDF_DBG_BUS_ENABLE
        FTDF_checkDbgMode();
#endif /* FTDF_DBG_BUS_ENABLE */
        sleep_status = BLOCK_ACTIVE;
    }

}

void ad_ftdf_init_phy_api(void)
{
    NVIC_ClearPendingIRQ(FTDF_WAKEUP_IRQn);
    NVIC_EnableIRQ(FTDF_WAKEUP_IRQn);

    //NVIC_ClearPendingIRQ(FTDF_GEN_IRQn);    // Disable ftdf interrupts
    //NVIC_EnableIRQ(FTDF_GEN_IRQn);          // ftdf will be handled by poiling
    sleep_status = BLOCK_ACTIVE;
#ifndef OS_FREERTOS
    uxCriticalNesting = 0;
#endif

    FTDF_reset(1);
}
#endif /* FTDF_PHY_API */
#endif
