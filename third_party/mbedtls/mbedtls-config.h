/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/memory.h>

#if defined(_WIN32)
#include <stdarg.h>
#include <stdio.h>
// Temporary, until the warnings can get fixed in the mbed library
#pragma warning(disable:4242)  // conversion from '*' to '*', possible loss of data
#pragma warning(disable:4244)  // conversion from '*' to '*', possible loss of data
#pragma warning(disable:4267)  // conversion from '*' to '*', possible loss of data
#endif

#if defined(_KERNEL_MODE)
#define MBEDTLS_PLATFORM_EXIT_MACRO
__inline int windows_kernel_snprintf(char * s, size_t n, const char * format, ...)
{
    va_list argp;
    va_start( argp, format );
    int ret = _vsnprintf_s( s, n, _TRUNCATE, format, argp );
    va_end( argp );
    return ret;
}
#define MBEDTLS_PLATFORM_STD_SNPRINTF windows_kernel_snprintf

#define MBEDTLS_PLATFORM_ZEROIZE_ALT
__inline void mbedtls_platform_zeroize( void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    while ( len-- )
    {
        *p++ = 0;
    }
}
#endif

#ifndef _WIN32
#define MBEDTLS_PLATFORM_SNPRINTF_MACRO snprintf
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE) && !defined(_KERNEL_MODE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CMAC_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECJPAKE_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED
#define MBEDTLS_MD_C
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
#define MBEDTLS_SSL_EXPORT_KEYS
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_TLS_C

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_SRV_C
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_BASE64_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#endif

#if OPENTHREAD_ENABLE_ECDSA
#define MBEDTLS_BASE64_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#endif

#define MBEDTLS_MPI_WINDOW_SIZE            1 /**< Maximum windows size used. */
#define MBEDTLS_MPI_MAX_SIZE              32 /**< Maximum number of bytes for usable MPIs. */
#define MBEDTLS_ECP_MAX_BITS             256 /**< Maximum bit size of groups */
#define MBEDTLS_ECP_WINDOW_SIZE            2 /**< Maximum window size used */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM      0 /**< Enable fixed-point speed-up */
#define MBEDTLS_ENTROPY_MAX_SOURCES        1 /**< Maximum number of sources supported */

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
#define MBEDTLS_PLATFORM_STD_CALLOC      otPlatCAlloc /**< Default allocator to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_FREE        otPlatFree /**< Default free to use, can be undefined */
#else
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define MBEDTLS_SSL_MAX_CONTENT_LEN      900 /**< Maxium fragment length in bytes */
#else
#define MBEDTLS_SSL_MAX_CONTENT_LEN      768 /**< Maxium fragment length in bytes */
#endif

#define MBEDTLS_SSL_CIPHERSUITES         MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8

#if defined(MBEDTLS_USER_CONFIG_FILE)
#include MBEDTLS_USER_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECP_ALT) && !defined(MBEDTLS_ECP_RESTARTABLE)
typedef void mbedtls_ecp_restart_ctx;
#endif

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
