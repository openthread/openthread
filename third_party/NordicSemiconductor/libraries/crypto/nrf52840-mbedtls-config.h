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

#ifndef NRF52840_MBEDTLS_CONFIG_H_
#define NRF52840_MBEDTLS_CONFIG_H_

#include <openthread/config.h>

#ifndef DISABLE_CC310
#define MBEDTLS_AES_ALT
#define MBEDTLS_ECP_ALT
#define MBEDTLS_SHA256_ALT
#endif // DISABLE_CC310

#ifdef MBEDTLS_THREADING
#define MBEDTLS_THREADING_C
#define MBEDTLS_THREADING_ALT
#endif // MBEDTLS_THREADING

#if defined(__ICCARM__)
    _Pragma("diag_suppress=Pe550")
#endif

#if defined(__CC_ARM)
    _Pragma("diag_suppress=550")
    _Pragma("diag_suppress=68")
#endif

/**
 * @def NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
 *
 * Define as 1 to enable AES usage in interrupt context and AES-256, by introducing a software AES under platform layer.
 *
 * @note This feature must be enabled to support AES-256 used by Commissioner and Joiner, and AES usage in interrupt context
 *       used by Header IE related features.
 *
 */
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE || OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#define NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT 1
#else
#define NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT 0
#endif

#endif // NRF52840_MBEDTLS_CONFIG_H_
