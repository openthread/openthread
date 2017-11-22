/**
 ****************************************************************************************
 *
 * @file hw_trng.c
 *
 * @brief Implementation of the True Random Number Generator Low Level Driver.
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

#if dg_configUSE_HW_TRNG

#include <hw_trng.h>

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

#define HW_TRNG_FIFO_DEPTH      (32)

static hw_trng_cb trng_cb;

void hw_trng_disable_clk(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_AMBA_REG, TRNG_CLK_ENABLE);
        GLOBAL_INT_RESTORE();
}

void hw_trng_clear_pending(void)
{
        uint32_t dummy __attribute__((unused));
        /* read TRNG_FIFOLVL_REG to clear level-sensitive source. */
        dummy = TRNG->TRNG_FIFOLVL_REG;
        NVIC_ClearPendingIRQ(TRNG_IRQn);
}

void hw_trng_enable(hw_trng_cb callback)
{
        if (callback != NULL) {
                trng_cb = callback;
                hw_trng_clear_pending();
                NVIC_EnableIRQ(TRNG_IRQn);
        }

        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_AMBA_REG, TRNG_CLK_ENABLE);
        GLOBAL_INT_RESTORE();
        REG_SET_BIT(TRNG, TRNG_CTRL_REG, TRNG_ENABLE);
}

void hw_trng_get_numbers(uint32_t* buffer, uint8_t size)
{
        if (size > HW_TRNG_FIFO_DEPTH)
                size = HW_TRNG_FIFO_DEPTH;

        for (int i = 0; i < size ; i++) {
                buffer[i] = hw_trng_get_number();
        }
}

__RETAINED_CODE uint8_t hw_trng_get_fifo_level(void)
{
        return (TRNG->TRNG_FIFOLVL_REG) & (REG_MSK(TRNG, TRNG_FIFOLVL_REG, TRNG_FIFOLVL) |
                                           REG_MSK(TRNG, TRNG_FIFOLVL_REG, TRNG_FIFOFULL));
}

void hw_trng_disable_interrupt(void)
{
        NVIC_DisableIRQ(TRNG_IRQn);
        trng_cb = NULL;
}

void hw_trng_disable(void)
{
        hw_trng_stop();
        hw_trng_disable_interrupt();
        hw_trng_clear_pending();
        hw_trng_disable_clk();
}

void TRNG_Handler(void)
{
        uint32_t dummy __attribute__((unused));

        SEGGER_SYSTEMVIEW_ISR_ENTER();

        if (trng_cb != NULL)
                trng_cb();

        /* read TRNG_FIFOLVL_REG to clear level-sensitive source. */
        dummy = TRNG->TRNG_FIFOLVL_REG;

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

#endif /* dg_configUSE_HW_TRNG */
