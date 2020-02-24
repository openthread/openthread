/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc310_platform_abort nrf_cc310_platform abort APIs
 * @ingroup nrf_cc310_platform
 * @{
 * @brief The nrf_cc310_platform_entropy APIs provides callbacks to abort
 *        from nrf_cc310_platform and/or dependent libraries.
 */
#ifndef NRF_CC310_PLATFORM_ABORT_H__
#define NRF_CC310_PLATFORM_ABORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Type definition of handle used for abort
 *
 * This handle could point to the thread or task to abort or any other
 * static memory required for aborting the on-going cryptographic routine(s).
 */
typedef void* nrf_cc310_platform_abort_handle_t;


/** @brief Type definition of platform abort function
 *
 * @note This function pointer will be used when the nrf_cc310_platform
 *       and/or dependent libraries raises an error that can't be recovered.
 */
typedef void (*nrf_cc310_platform_abort_fn_t)(char const * const reason);


/** @brief Type definition of structure holding platform abort APIs
 */
typedef struct nrf_cc310_platform_abort_apis_t
{
    nrf_cc310_platform_abort_handle_t abort_handle;   //!< Handle to use when crypto operations are aborted.
    nrf_cc310_platform_abort_fn_t     abort_fn;       //!< Function to use when crypto operations are aborted.

} nrf_cc310_platform_abort_apis_t;


/** @brief External reference to the platform abort APIs
 */
extern nrf_cc310_platform_abort_apis_t  platform_abort_apis;


/** @brief Function to set platform abort APIs
 *
 * @param[in]   apis    Pointer to platform APIs.
 */
void nrf_cc310_platform_set_abort(
    nrf_cc310_platform_abort_apis_t const * const apis);


/** @brief Function to initialize platform abort APIs
 *
 * @note This function must be called once before calling
 * @ref nrf_cc310_platform_init or @ref nrf_cc310_platform_init_no_rng.
 *
 * @note This function is not expected to be thread-safe.
 */
void nrf_cc310_platform_abort_init(void);


#ifdef __cplusplus
}
#endif

#endif /* NRF_CC310_PLATFORM_ABORT_H__ */

/** @} */
