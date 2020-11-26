/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc3xx_platform_ctr_drbg nrf_cc3xx_platform ctr_drbg generation APIs
 * @ingroup nrf_cc3xx_platform
 * @{
 * @brief The nrf_cc3xx_platform_ctr_drbg APIs provide PRNG seeded by TRNG in
 *        accordance with <em>NIST SP 800-90A: Recommendation for Random Number
 *        Generation Using Deterministic Random Bit Generators</em>
 *        The generation of TRNG/PRNG data is using Arm CryptoCell cc3xx
 *        hardware acceleration.
 *
 * The pre-built APIs are based on mbedtls_entropy and mbedtls_ctr_drbg
 * but doesn't require setting up memory allocation before use.
 */
#ifndef NRF_CC3XX_PLATFORM_CTR_DRBG_H__
#define NRF_CC3XX_PLATFORM_CTR_DRBG_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf_cc3xx_platform_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**@brief Macro holding size of the opaque ctr_drbg context type
 *
 * This corresponds to a structure with the combined size of
 * mbedtls_entropy_context and mbedtls_ctr_drbg_context in Arm CryptoCell
 * code base which is sized differently than in vanilla mbed TLS software.
 */
#define NRF_CC3XX_PLATFORM_ENTROPY_SIZE_WORDS (110)

/**@brief Opaque type for the context required for ctr_drbg generation
 *
 * Note: This opaque type contains contexts for mbed TLS
 * entropy generation (TRNG) and ctr_drbg (PRNG).
 */
typedef struct nrf_cc3xx_platform_ctr_drbg_context_t
{
    uint32_t        is_initialized;									//!< Mask indicating if the ctr_drbg context has been initialized.
    uint32_t 		buffer[NRF_CC3XX_PLATFORM_ENTROPY_SIZE_WORDS];	//!< Internal buffer for the built-in entropy and ctrl_drbg contexts.
} nrf_cc3xx_platform_ctr_drbg_context_t;


/**@brief Function that initializes an ctr_drbg context
 *
 * @param[in,out]	context             Pointer to structure holding the ctr_drbg
 *                                      context which must be used for subsequent
 *                                      calls to generate random data.
 * @param[in]       pers_string         Personalization string used for the
 *                                      CTR_DRBG_Instantiate_algorithm.
 * @param[in]       pers_string_len     Length of the personalization string,
 *                                      which may be zero.
 *
 * @retval NRF_CC3XX_PLATFORM_ERROR_PARAM_NULL Context was NULL.
 * @return 0 on success, otherwise a non-zero failure from mbedtls_ctrl_drbg_seed.
 */
int nrf_cc3xx_platform_ctr_drbg_init(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context,
    const uint8_t * pers_string,
    size_t pers_string_len);


/**@brief Function that deintializes an ctr_drbg context
 *
 * Param[in,out] context    Pointer to structure holding ctr_drbg context which
 *                          is to be deinitialized.
 */
int nrf_cc3xx_platform_ctr_drbg_free(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context);


/**@brief Function to enable prediction resistance
 *
 * If prediction resistance is enabled, TRNG is gathered at the beginning of
 * every call to @ref nrf_cc3xx_platform_ctr_drbg_get and
 * @ref nrf_cc3xx_platform_ctr_drbg_get_with_add. This leads to a higher power
 * draw and longer execution time.
 *
 * @note Before calling this api the context to must be initialized by calling
 *       @ref nrf_cc3xx_platform_ctr_drbg_init
 *
 * @note The default configuration is to have prediction resistance turned off.
 *
 * @param[in,out]   context     Pointer to a structure holding the ctr_drbg
 *                              context.
 * @param[in]       pr_enabled  Enables prediction resistance if true, otherwise
 *                              false (default).
 *
 * @retval NRF_CC3XX_PLATFORM_ERROR_PARAM_NULL Context was NULL.
 * @return 0 on success, otherwise a non-zero failure.
 */
int nrf_cc3xx_platform_ctr_drbg_set_pr(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context,
    bool pr_enabled);


/**@brief Function to change the reseed interval
 *
 * This API controls when the ctr_drbg is automatically reseeded
 *
 * @note Before calling this api the context to must be initialized by calling
 *       @ref nrf_cc3xx_platform_ctr_drbg_init.
 *
 * @note Changing the reseed interval is optional.
 *
 * @param[in,out]   context     Pointer to a structure holding the ctr_drbg
 *                              context.
 * @param[in]       interval    New reeseed interval value.
 *
 * @retval NRF_CC3XX_PLATFORM_ERROR_PARAM_NULL Context was NULL.
 * @return 0 on success, otherwise a non-zero failure according to the API
 *         mbedtls_ctrl_drbg_reseed.
 */
int nrf_cc3xx_platform_ctr_drbg_set_reseed_interval(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context, int interval);


/**@brief Function to do a manual reseed of ctr_drbg (using TRNG)
 *
 * @note Calling this API is optional as the APIs
 *       @ref nrf_cc3xx_platform_ctr_drbg_get and
 *       @ref nrf_cc3xx_platform_ctr_drbg_get_with_add functions will reseed
 *       automatically according to the reseed interval in the built-in
 *       mbedtls_ctr_drbg context.
 *
 * @note Before calling this api the context to must be initialized by calling
 *       @ref nrf_cc3xx_platform_ctr_drbg_init.
 *
 * @note This API is only usable if @ref nrf_cc3xx_platform_init was run
 *       prior to calling it.
 *
 * @note This API is unneccesary if ctr_drbg is executed with prediction
 *       resistance turned on.
 *
 * @param[in,out]  context     Pointer to a structure holding the ctr_drbg
 *                             context.
 * @param[in]      additional  Optional additional input to use for
 *                             CTR_DRBG_Reseed_function.
 * @param[in]      add_len     Length of the additional input, may be zero.
 *
 * @retval NRF_CC3XX_PLATFORM_ERROR_PARAM_NULL  Context was NULL.
 * @return 0 on success, otherwise a non-zero failure according to the API
 *         mbedtls_ctrl_drbg_seed.
 */
int nrf_cc3xx_platform_ctr_drbg_reseed(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context,
    const uint8_t *additional,
    size_t add_len);


/** @brief Function to get PRNG using ctr_drbg and an additional string of data
 *
 * This function will calculate PRNG using HW accelarated AES CTR_DRBG with a
 * 16-byte key and reseed with TRNG using ARM CryptoCEll cc3xx HW according
 * to a reseed interval.
 *
 * @details This function calculates random numbers using PRNG seeded by TRNG
 *          as defined in <em>NIST SP 800-90A: Recommendation for Random Number
 *          Generation Using Deterministic Random Bit Generators</em>. The
 *          random numbers are generated using Arm CryptoCell cc3xx hardware
 *          acceleration.
 *
 * @note Before calling this api the context to must be initialized by calling
 *       @ref nrf_cc3xx_platform_ctr_drbg_init.
 *
 * @note This API is only usable if @ref nrf_cc3xx_platform_init was run
 *       prior to calling it.
 *
 * @param[in,out]   context     Pointer to structure holding the ctr_drbg context.
 * @param[in]       buffer      Pointer to buffer to hold PRNG data.
 * @param[in]       len         Length of PRNG to get.
 * @param[out]      olen        Length reported out.
 * @param[in]       additional  Additional input to use with CTR_DRBG_Generate_algorithm.
 * @param[in]       add_len     Length of CTR_DRBG additional input.
 *
 * @retval NRF_CC3XX_PLATFORM_ERROR_PARAM_NULL  Context was NULL.
 * @return 0 on success, otherwise a non-zero failure according to the API
 *         mbedtls_ctrl_drbg_get_with_add.
 */
int nrf_cc3xx_platform_ctr_drbg_get_with_add(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context,
    uint8_t *buffer,
    size_t len,
    size_t *olen,
    const uint8_t *additional,
    size_t add_len);


/** @brief Function to get PRNG data using ctr_drbg
 *
 * @details This function calculates random numbers using PRNG seeded by TRNG as
 *          defined in <em>NIST SP 800-90A: Recommendation for Random Number
 *          Generation Using Deterministic Random Bit Generators</em>. The
 *          random numbers are generated using Arm CryptoCell cc3xx hardware
 *          acceleration.
 *
 * @note Before calling this api the context to must be initialized by calling
 *       @ref nrf_cc3xx_platform_ctr_drbg_init.
 *
 * @note This API is only usable if @ref nrf_cc3xx_platform_init was run
 *       prior to calling it.
 *
 * @param[in,out]   context     Pointer to structure holding the ctr_drbg context.
 * @param[in]       buffer      Pointer to buffer to hold PRNG data.
 * @param[in]       length      Length of PRNG to get.
 * @param[out]      olen        Length reported out.
 *
 * @return 0 on success, otherwise a non-zero failure  according to the API
 *         mbedtls_ctr_drbg_random.
 */
int nrf_cc3xx_platform_ctr_drbg_get(
    nrf_cc3xx_platform_ctr_drbg_context_t * const context,
    uint8_t *buffer,
    size_t length,
    size_t* olen);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC3XX_PLATFORM_CTR_DRBG_H__ */

/** @} */
