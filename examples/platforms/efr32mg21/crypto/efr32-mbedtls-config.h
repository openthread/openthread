/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
#include "em_se.h"

/**
 * \def MBEDTLS_AES_ALT
 *
 * Enable hardware acceleration for the AES block cipher
 *
 * Module:  sl_crypto/src/crypto_aes.c for devices with CRYPTO
 *          sl_crypto/src/aes_aes.c for devices with AES
 *
 * See MBEDTLS_AES_C for more information.
 */
#define MBEDTLS_AES_ALT

/**
 * \def MBEDTLS_ENTROPY_HARDWARE_ALT
 *
 * Integrate the provided default entropy source into the mbed
 * TLS entropy infrastructure.
 *
 */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* Turn on all hardware acceleration provided by the EFR32xG21's built-in SE */
#define MBEDTLS_SHA1_ALT
#define MBEDTLS_SHA1_PROCESS_ALT
#define MBEDTLS_SHA256_ALT
#define MBEDTLS_SHA256_PROCESS_ALT
#define MBEDTLS_SHA512_ALT
#define MBEDTLS_SHA512_PROCESS_ALT

#define MBEDTLS_CCM_ALT
#define MBEDTLS_CMAC_ALT

/* Turning on ECC acceleration is dependant on not requiring curve25519 when
 * running on EFR32xG21A devices */
#if (defined(_SILICON_LABS_SECURITY_FEATURE) &&                                   \
     (_SILICON_LABS_SECURITY_FEATURE == _SILICON_LABS_SECURITY_FEATURE_VAULT)) || \
    !defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
#define MBEDTLS_ECDH_GEN_PUBLIC_ALT
#define MBEDTLS_ECDSA_GENKEY_ALT
#define MBEDTLS_ECDH_COMPUTE_SHARED_ALT
#define MBEDTLS_ECDSA_SIGN_ALT
#define MBEDTLS_ECDSA_VERIFY_ALT
/* Incompatibility in header files between mbedTLS version in OT and GSDK means
 * we can't turn on EC-JPAKE acceleration on EFR32xG21 just yet. Will be fixed
 * for the next GSDK update */
//#define MBEDTLS_ECJPAKE_ALT
#endif /* EFR32xG21B or curve25519 not enabled */

#endif // EFR32_MBEDTLS_CONFIG_H
