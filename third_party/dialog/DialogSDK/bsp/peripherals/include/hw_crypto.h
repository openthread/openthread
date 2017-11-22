/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup CRYPTO
 * \{
 * \brief Interrupt handling for the crypto engines (AES/HASH, ECC)
 */

/**
 ****************************************************************************************
 *
 * @file hw_crypto.h
 *
 * @brief Interrupt handling API for the AES/Hash and ECC Engines.
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

#include "sdk_defs.h"

/**
 * \brief Crypto engines callback
 *
 * This function type is used for callbacks called when the crypto engines (AES/HASH, ECC)
 * generate an interrupt.
 *
 * \param [in] status The status register (either AES/HASH or ECC) at the time of the interrupt.
 *
 */
typedef void (*hw_crypto_cb)(unsigned int status);

/**
 * \brief Enable interrupt for AES/HASH crypto engine.
 *
 * \param [in] cb A callback to be called when interrupt occurs. It must always be provided.
 *
 * \note AES/HASH engine and ECC engine are common sources of CRYPTO system interrupt. This
 *       function only enables CRYPTO interrupt itself and registers a callback for AES/HASH
 *       related CRYPTO interrupts. In order to fully enable AES/HASH interrupts 
 *       hw_aes_hash_enable_interrupt_source() must also be called
 */
void hw_crypto_enable_aes_hash_interrupt(hw_crypto_cb cb);

/**
 * \brief Enable interrupt for ECC crypto engine.
 *
 * \param [in] cb A callback to be called when interrupt occurs. It must always be provided.
 */
void hw_crypto_enable_ecc_interrupt(hw_crypto_cb cb);

/**
 * \brief Disable interrupt for AES/HASH crypto engine.
 */
void hw_crypto_disable_aes_hash_interrupt(void);

/**
 * \brief Disable interrupt for ECC crypto engine.
 */
void hw_crypto_disable_ecc_interrupt(void);

/**
 * \brief Clear pending interrupt from AES/HASH and ECC crypto engines.
 *
 * \note This function clears the pending CRYPTO interrupt only on the NVI Controller.
 *       Use hw_aes_hash_clear_interrupt_req() and hw_ecc_clear_interrupt_source() to clear the
 *       source of CRYPTO interrupt on the AES/HASH and ECC engines respectively.
 */
static inline void hw_crypto_clear_pending_interrupt(void)
{
        NVIC_ClearPendingIRQ(CRYPTO_IRQn);
}

#endif /* (dg_configUSE_HW_AES_HASH || dg_configUSE_HW_ECC) */
/**
 * \}
 * \}
 * \}
 */

