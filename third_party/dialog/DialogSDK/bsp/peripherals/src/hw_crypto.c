/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup CRYPTO
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_crypto.c
 *
 * @brief Implementation of interrupt handling for the AES/Hash and ECC Engines.
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

#if (dg_configUSE_HW_AES_HASH || dg_configUSE_HW_ECC)

#include "hw_crypto.h"

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

__RETAINED static hw_crypto_cb hw_crypto_aes_hash_cb;
__RETAINED static hw_crypto_cb hw_crypto_ecc_cb;

void hw_crypto_enable_aes_hash_interrupt(hw_crypto_cb cb)
{
        /* A callback for the interrupt must be provided */
        ASSERT_ERROR(cb);
        hw_crypto_aes_hash_cb = cb;
        NVIC_EnableIRQ(CRYPTO_IRQn);
}

void hw_crypto_enable_ecc_interrupt(hw_crypto_cb cb)
{
        /* A callback for the interrupt must be provided */
        ASSERT_ERROR(cb);
        hw_crypto_ecc_cb = cb;
        NVIC_EnableIRQ(CRYPTO_IRQn);
}

void hw_crypto_disable_aes_hash_interrupt(void)
{
        hw_crypto_aes_hash_cb = NULL;
        if (!hw_crypto_ecc_cb) {
                NVIC_DisableIRQ(CRYPTO_IRQn);
        }
}

void hw_crypto_disable_ecc_interrupt(void)
{
        hw_crypto_ecc_cb = NULL;
        if (!hw_crypto_aes_hash_cb) {
                NVIC_DisableIRQ(CRYPTO_IRQn);
        }
}

void CRYPTO_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

        uint32_t status = AES_HASH->CRYPTO_STATUS_REG;

        /* In case both AES/HASH and ECC have triggered an interrupt, first the AES/HASH
           will be served, and then the ISR will be called again since the ECC interrupt
           source is only cleared by reading its status register */
        if (status & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_IRQ_ST_Msk) {
                /* Clear AES/HASH interrupt source */
                AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;

                if (hw_crypto_aes_hash_cb != NULL) {
                        hw_crypto_aes_hash_cb(status);
                }
        } else {
                /* Clear ECC interrupt source */
                status = ECC->ECC_STATUS_REG;

                if (hw_crypto_ecc_cb != NULL) {
                        hw_crypto_ecc_cb(status);
                }
        }

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}


#endif /* (dg_configUSE_HW_AES_HASH || dg_configUSE_HW_ECC) */
/**
 * \}
 * \}
 * \}
 */

