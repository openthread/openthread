/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file defines flash interface.
 */

#ifndef OT_PLATFORM_FLASH_H
#define OT_PLATFORM_FLASH_H

#include <stdint.h>

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init flash driver.
 *
 * @retval ::kThreadError_None    Init flash driver success.
 * @retval ::kThreadError_Failed  Init flash driver fail.
 */
ThreadError otPlatFlashInit(void);

/**
 * Disable flash driver.
 *
 * @retval ::kThreadError_None    Disable flash driver success.
 * @retval ::kThreadError_Failed  Disable flash driver fail.
 */
ThreadError otPlatFlashDisable(void);

/**
 * Get the base address of flash.
 *
 * @returns The base address of the flash.
 */
uint32_t otPlatFlashGetBaseAddress(void);

/**
 * Get the size of flash.
 *
 * @returns The size of the flash.
 */
uint32_t otPlatFlashGetSize(void);

/**
 * Get the page size of flash.
 *
 * @returns The page size of the flash.
 */
uint32_t otPlatFlashGetPageSize(void);

/**
 * Erase flash pages.
 *
 * @param[in]  aAddress  The start address of the flash to erase.
 * @param[in]  aSize     The size of the flash to erase.
 *
 * @retval ::kThreadError_None           Erase flash success.
 * @retval ::kThreadError_InvalidArgs    Arguments invalid.
 * @retval ::kThreadError_Failed         Erase flash fail.
 */
ThreadError otPlatFlashErasePage(uint32_t aAddress, uint32_t aSize);

/**
 * Write flash.
 *
 * @param[in]  aAddress  The start address of the flash to write.
 * @param[in]  aData     The pointer of the data to write.
 * @param[in]  aSize     The size of the data to write.
 *
 * @returns The actual size of octets write to flash.
 */
uint32_t otPlatFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize);

/**
 * Read flash.
 *
 * @param[in]  aAddress  The start address of the flash to read.
 * @param[in]  aData     The pointer of buffer to store the data.
 * @param[in]  aSize     The size of the data to read.
 *
 * @returns The actual size of octets read to buffer.
 */
uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OT_PLATFORM_FLASH_H
