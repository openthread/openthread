/**
 * \file mbedtls_sha256.h
 *
 * \brief SHA-224 and SHA-256 cryptographic hash function
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *  Copyright 2017 NXP. Not a Contribution
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

// Regular implementation
//

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_FREESCALE_LTC_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context ltc_hash_ctx_t

#elif defined(MBEDTLS_FREESCALE_LPC_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context sha_ctx_t

#elif defined(MBEDTLS_FREESCALE_CAAM_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context caam_hash_ctx_t

#elif defined(MBEDTLS_FREESCALE_CAU3_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context cau3_hash_ctx_t  

#elif defined(MBEDTLS_FREESCALE_DCP_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context dcp_hash_ctx_t

#elif defined(MBEDTLS_FREESCALE_HASHCRYPT_SHA256)

/**
 * \brief          SHA-256 context structure
 */
#define mbedtls_sha256_context hashcrypt_hash_ctx_t

#endif /* MBEDTLS_FREESCALE_LTC_SHA256 */

#ifdef __cplusplus
}
#endif

#endif /* sha256_alt.h */
