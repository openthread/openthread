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
 *   This file defines the context structure for crypto apis.
 */

#ifndef CRYPTO_CONTEXT_H_
#define CRYPTO_CONTEXT_H_

#ifndef OPEN_THREAD_DRIVER
#include <stdint.h>
#endif

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MBEDTLS_AES_CONTEXT
#define MBEDTLS_AES_CONTEXT

/**
 * \brief          AES context structure
 *
 * \note           buf is able to hold 32 extra bytes, which can be used:
 *                 - for alignment purposes if VIA padlock is used, and/or
 *                 - to simplify key expansion in the 256-bit case by
 *                 generating an extra round key
 */
typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
}
mbedtls_aes_context;

#endif // MBEDTLS_AES_CONTEXT

#ifndef MBEDTLS_MD_CONTEXT
#define MBEDTLS_MD_CONTEXT

/**
 * Opaque struct defined in md_internal.h
 */
typedef struct mbedtls_md_info_t mbedtls_md_info_t;

/**
 * Generic message digest context.
 */
typedef struct {
    /** Information about the associated message digest */
    const mbedtls_md_info_t *md_info;

    /** Digest-specific context */
    void *md_ctx;

    /** HMAC part of the context */
    void *hmac_ctx;
} mbedtls_md_context_t;

#endif // MBEDTLS_MD_CONTEXT

/**
 * @addtogroup core-security
 *
 * @{
 *
 */
    
/**
 * @def MBED_MEMORY_BUF_SIZE
 *
 * The size of the memory buffer used by mbedtls.
 *
 */
#define MBED_MEMORY_BUF_SIZE  512

/**
 * This type represents all the static / global variables used by OpenThread allocated in one place.
 */
typedef struct otCryptoContext
{
    //
    // Variables
    //
    
    bool mIsInitialized;
    unsigned char mMemoryBuf[MBED_MEMORY_BUF_SIZE];

    mbedtls_aes_context mAesContext;
    mbedtls_md_context_t mSha256Context;

} otCryptoContext;

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // CRYPTO_CONTEXT_H_
