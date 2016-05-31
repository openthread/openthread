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
 *   This file includes definitions for checksum calculation.
 */

#ifndef CHECKSUM_HPP_
#define CHECKSUM_HPP_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def CalculateChecksum16
 *
 * This method updates a checksum.
 *
 * @param[in]  aChecksum      The checksum value to update.
 * @param[in]  aValue         The 16-bit value to update @p aChecksum with.
 *
 * @returns  The updated checksum.
 *
 */
uint16_t CalculateChecksum16(uint16_t aChecksum, uint16_t aValue);

/**
 * @def CalculateChecksum
 *
 * This method updates a checksum.
 *
 * @param[in]  aChecksum      The checksum value to update.
 * @param[in]  aBuf           A pointer to a buffer.
 * @param[in]  aLength        The number of bytes in @p aBuf.
 *
 * @returns  The updated checksum.
 *
 */
uint16_t CalculateChecksum(uint16_t aChecksum, const void *aBuf, uint16_t aLength);

#ifdef __cplusplus
};
#endif

#endif  // CHECKSUM_HPP_
