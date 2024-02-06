/*
 * Test driver for AEAD entry points.
 */
/*  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PSA_CRYPTO_DRIVERS) && defined(PSA_CRYPTO_DRIVER_TEST)
#include "psa_crypto_aead.h"

#include "test/drivers/aead.h"

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1)
#include "libtestdriver1/library/psa_crypto_aead.h"
#endif

mbedtls_test_driver_aead_hooks_t
    mbedtls_test_driver_aead_hooks = MBEDTLS_TEST_DRIVER_AEAD_INIT;

psa_status_t mbedtls_test_transparent_aead_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *plaintext, size_t plaintext_length,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length)
{
    mbedtls_test_driver_aead_hooks.hits++;

    if (mbedtls_test_driver_aead_hooks.forced_status != PSA_SUCCESS) {
        mbedtls_test_driver_aead_hooks.driver_status =
            mbedtls_test_driver_aead_hooks.forced_status;
    } else {
#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
        defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_AEAD)
        mbedtls_test_driver_aead_hooks.driver_status =
            libtestdriver1_mbedtls_psa_aead_encrypt(
                (const libtestdriver1_psa_key_attributes_t *) attributes,
                key_buffer, key_buffer_size,
                alg,
                nonce, nonce_length,
                additional_data, additional_data_length,
                plaintext, plaintext_length,
                ciphertext, ciphertext_size, ciphertext_length);
#elif defined(MBEDTLS_PSA_BUILTIN_AEAD)
        mbedtls_test_driver_aead_hooks.driver_status =
            mbedtls_psa_aead_encrypt(
                attributes, key_buffer, key_buffer_size,
                alg,
                nonce, nonce_length,
                additional_data, additional_data_length,
                plaintext, plaintext_length,
                ciphertext, ciphertext_size, ciphertext_length);
#else
        (void) attributes;
        (void) key_buffer;
        (void) key_buffer_size;
        (void) alg;
        (void) nonce;
        (void) nonce_length;
        (void) additional_data;
        (void) additional_data_length;
        (void) plaintext;
        (void) plaintext_length;
        (void) ciphertext;
        (void) ciphertext_size;
        (void) ciphertext_length;
        mbedtls_test_driver_aead_hooks.driver_status = PSA_ERROR_NOT_SUPPORTED;
#endif
    }
    return mbedtls_test_driver_aead_hooks.driver_status;
}

psa_status_t mbedtls_test_transparent_aead_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *ciphertext, size_t ciphertext_length,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length)
{
    mbedtls_test_driver_aead_hooks.hits++;

    if (mbedtls_test_driver_aead_hooks.forced_status != PSA_SUCCESS) {
        mbedtls_test_driver_aead_hooks.driver_status =
            mbedtls_test_driver_aead_hooks.forced_status;
    } else {
#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
        defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_AEAD)
        mbedtls_test_driver_aead_hooks.driver_status =
            libtestdriver1_mbedtls_psa_aead_decrypt(
                (const libtestdriver1_psa_key_attributes_t *) attributes,
                key_buffer, key_buffer_size,
                alg,
                nonce, nonce_length,
                additional_data, additional_data_length,
                ciphertext, ciphertext_length,
                plaintext, plaintext_size, plaintext_length);
#elif defined(MBEDTLS_PSA_BUILTIN_AEAD)
        mbedtls_test_driver_aead_hooks.driver_status =
            mbedtls_psa_aead_decrypt(
                attributes, key_buffer, key_buffer_size,
                alg,
                nonce, nonce_length,
                additional_data, additional_data_length,
                ciphertext, ciphertext_length,
                plaintext, plaintext_size, plaintext_length);
#else
        (void) attributes;
        (void) key_buffer;
        (void) key_buffer_size;
        (void) alg;
        (void) nonce;
        (void) nonce_length;
        (void) additional_data;
        (void) additional_data_length;
        (void) ciphertext;
        (void) ciphertext_length;
        (void) plaintext;
        (void) plaintext_size;
        (void) plaintext_length;
        mbedtls_test_driver_aead_hooks.driver_status = PSA_ERROR_NOT_SUPPORTED;
#endif
    }
    return mbedtls_test_driver_aead_hooks.driver_status;
}

#endif /* MBEDTLS_PSA_CRYPTO_DRIVERS && PSA_CRYPTO_DRIVER_TEST */
