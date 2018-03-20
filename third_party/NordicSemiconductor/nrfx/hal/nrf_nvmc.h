/**
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef NRF_NVMC_H__
#define NRF_NVMC_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup nrf_nvmc_hal NVMC HAL
 * @{
 * @ingroup nrf_nvmc
 * @brief   Hardware access layer for managing the Non-Volatile Memory Controller (NVMC) peripheral.
 *
 * This driver allows writing to the non-volatile memory (NVM) regions
 * of the chip. In order to write to NVM the controller must be powered
 * on and the relevant page must be erased.
 *
 */


/**
 * @brief Erase a page in flash. This is required before writing to any
 * address in the page.
 *
 * @param address Start address of the page.
 */
void nrf_nvmc_page_erase(uint32_t address);


/**
 * @brief Write a single byte to flash.
 *
 * The function reads the word containing the byte, and then
 * rewrites the entire word.
 *
 * @param address Address to write to.
 * @param value   Value to write.
 */
void nrf_nvmc_write_byte(uint32_t address , uint8_t value);


/**
 * @brief Write a 32-bit word to flash.
 * @param address Address to write to.
 * @param value   Value to write.
 */
void nrf_nvmc_write_word(uint32_t address, uint32_t value);


/**
 * @brief Write consecutive bytes to flash.
 *
 * @param address   Address to write to.
 * @param src       Pointer to data to copy from.
 * @param num_bytes Number of bytes in src to write.
 */
void nrf_nvmc_write_bytes(uint32_t  address, const uint8_t * src, uint32_t num_bytes);


/**
 * @brief Write consecutive words to flash.
 *
 * @param address   Address to write to.
 * @param src       Pointer to data to copy from.
 * @param num_words Number of words in src to write.
 */
void nrf_nvmc_write_words(uint32_t address, const uint32_t * src, uint32_t num_words);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_NVMC_H__
