/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 
#ifndef EFR32_MBEDTLS_CONFIG_H
#define EFR32_MBEDTLS_CONFIG_H

#include "em_device.h"

/**
 * \def MBEDTLS_AES_ALT
 *
 * Enable hardware acceleration for the AES block cipher
 *
 * Module:  sl_crypto/src/sl_aes.c
 *          or
 *          sl_crypto/src/slcl_aes.c if MBEDTLS_SLCL_PLUGINS is defined.
 *
 * See MBEDTLS_AES_C for more information.
 */
#define MBEDTLS_AES_ALT

/**
 * \def MBEDTLS_ECP_DEVICE_ALT
 * \def MBEDTLS_ECP_DEVICE_ALT
 * \def MBEDTLS_ECP_DOUBLE_JAC_ALT
 * \def MBEDTLS_ECP_DEVICE_ADD_MIXED_ALT
 * \def MBEDTLS_ECP_NORMALIZE_JAC_ALT
 * \def MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT
 * \def MBEDTLS_MPI_MODULAR_DIVISION_ALT
 *
 * Enable hardware acceleration for the elliptic curve over GF(p) library.
 *
 * Module:  sl_crypto/src/sl_ecp.c
 *          or
 *          sl_crypto/src/slcl_ecp.c if MBEDTLS_SLCL_PLUGINS is defined.
 * Caller:  library/ecp.c
 *          library/ecdh.c
 *          library/ecdsa.c
 *          library/ecjpake.c
 *
 * Requires: MBEDTLS_BIGNUM_C, MBEDTLS_ECP_C and at least one
 * MBEDTLS_ECP_DP_XXX_ENABLED and (CRYPTO_COUNT > 0)
 */
#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)
#define MBEDTLS_ECP_DEVICE_ALT
#define MBEDTLS_ECP_DOUBLE_JAC_ALT
#define MBEDTLS_ECP_DEVICE_ADD_MIXED_ALT
#define MBEDTLS_ECP_NORMALIZE_JAC_ALT
#define MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT
#define MBEDTLS_MPI_MODULAR_DIVISION_ALT
#endif

/**
 * \def MBEDTLS_SHA256_ALT
 *
 * Enable hardware acceleration for the SHA-224 and SHA-256 cryptographic
 * hash algorithms.
 *
 * Module:  sl_crypto/src/sl_sha256.c
 *          or
 *          sl_crypto/src/slcl_sha256.c if MBEDTLS_SLCL_PLUGINS is defined.
 * Caller:  library/entropy.c
 *          library/mbedtls_md.c
 *          library/ssl_cli.c
 *          library/ssl_srv.c
 *          library/ssl_tls.c
 *
 * Requires: MBEDTLS_SHA256_C and (CRYPTO_COUNT > 0)
 * See MBEDTLS_SHA256_C for more information.
 */
#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)
//#define MBEDTLS_SHA256_ALT
#endif

#endif // EFR32_MBEDTLS_CONFIG_H
