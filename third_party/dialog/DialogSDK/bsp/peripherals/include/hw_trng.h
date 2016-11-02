/**
 *****************************************************************************************
 *
 * @file hw_trng.h
 *
 * @brief Definition of API for the True Random Number Generator Low Level Driver.
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
 *****************************************************************************************
 */

#if dg_configUSE_HW_TRNG

#include <black_orca.h>

/**
 * \brief TRNG callback.
 *
 * This function is called by the TRNG driver when the interrupt is fired.
 *
 * \note If the TRNG is not needed anymore the hw_trng_disable function should be called in the
 * callback function to save power.
 *
 */
typedef void (*hw_trng_cb)(void);

/**
 * \brief TRNG enable.
 *
 * This function enables the TRNG. If callback is not NULL it will be called when a TRNG interrupt
 * occurs. The interrupt is triggered when the TRNG FIFO is full.
 *
 * \param [in] callback The callback function that is called when a TRNG interrupt occurs.
 *
 * \note If the amount of random numbers needed is less than the contents of the FIFO it is faster
 * and more power efficient to use a wait loop in combination with the hw_trng_get_fifo_level
 * function. If the FIFO has the required level use the hw_trng_get_numbers function to get the
 * random numbers.
 *
 */
void hw_trng_enable(hw_trng_cb callback);

/**
 * \brief TRNG get numbers.
 *
 * This function reads the random number from the TRNG FIFO.
 *
 * \param [in] buffer The buffer to write the numbers to.
 * \param [in] size The size of the buffer (max 32).
 *
 * \note Do not forget to disable the TRNG after reading the amount of numbers needed to save
 * power.
 *
 */
void hw_trng_get_numbers(uint32_t* buffer, uint8_t size);

/**
 * \brief TRNG get FIFO level.
 *
 * This function returns the current level of the TRNG FIFO.
 *
 * \return The current level of the TRNG FIFO.
 *
 */
uint8_t hw_trng_get_fifo_level(void);

/**
 * \brief TRNG disable.
 *
 * This function disables the TRNG and its interrupt.
 *
 */
void hw_trng_disable(void);

#endif /* dg_configUSE_HW_TRNG */
