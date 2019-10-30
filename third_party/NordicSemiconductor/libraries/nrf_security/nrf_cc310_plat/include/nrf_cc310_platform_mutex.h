/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc310_platform_mutex nrf_cc310_platform mutex APIs
 * @ingroup nrf_cc310_platform
 * @{
 * @brief The nrf_cc310_platform_mutex APIs provides RTOS integration for mutex
 *        usage in nrf_cc310_platform and dependent libraries.
 */
#ifndef NRF_CC310_PLATFORM_MUTEX_H__
#define NRF_CC310_PLATFORM_MUTEX_H__

#include <stdint.h>
#include <stddef.h>

#include "nrf_cc310_platform_abort.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NRF_CC310_PLATFORM_MUTEX_MASK_INVALID        (0)         /*!< Mask indicating that the mutex is invalid (not initialized or allocated). */
#define NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID       (1<<0)      /*!< Mask value indicating that the mutex is valid for use. */
#define NRF_CC310_PLATFORM_MUTEX_MASK_IS_ALLOCATED   (1<<1)      /*!< Mask value indicating that the mutex is allocated and requires deallocation once freed. */

/** @brief Type definition of architecture neutral mutex type */
typedef struct nrf_cc310_platform_mutex
{
    void * 		mutex;
    uint32_t	flags;

} nrf_cc310_platform_mutex_t;

/** @brief Type definition of function pointer to initialize a mutex
 *
 * Calling this function pointer should initialize a previously uninitialized
 * mutex or do nothing if the mutex is already initialized.
 *
 * @note Initialization may not imply memory allocation, as this can be done
 *       using static allocation through other APIs in the RTOS.
 *
 * @param[in]   mutex   Pointer to a mutex to initialize.
 */
typedef void (*nrf_cc310_platform_mutex_init_fn_t)(nrf_cc310_platform_mutex_t *mutex);


/** @brief Type definition of function pointer to free a mutex
 *
 * Calling this function pointer should free a mutex.
 *
 * @note If the RTOS does not provide an API to free the mutex it is advised
 *       to reset the mutex to an initialized state with no owner.
 *
 * @param[in]   mutex   Pointer to a mutex to free.
 */
typedef void (*nrf_cc310_platform_mutex_free_fn_t)(nrf_cc310_platform_mutex_t *mutex);


/** @brief Type definition of function pointer to lock a mutex
 *
 * Calling this function pointer should lock a mutex.
 *
 * @param[in]   mutex   Pointer to a mutex to lock.
 */
typedef int (*nrf_cc310_platform_mutex_lock_fn_t)(nrf_cc310_platform_mutex_t *mutex);


/** @brief Type definition of function pointer to unlock a mutex
 *
 * Calling this function pointer should unlock a mutex.
 *
 * @param[in]   mutex   Pointer to a mutex to unlock.
 */
typedef int (*nrf_cc310_platform_mutex_unlock_fn_t)(nrf_cc310_platform_mutex_t *mutex);


/**@brief Type definition of structure holding platform mutex APIs
 */
typedef struct nrf_cc310_platform_mutex_apis_t
{
    /* The platform mutex init function */
    nrf_cc310_platform_mutex_init_fn_t 		mutex_init_fn;

    /* The platform mutex free function */
    nrf_cc310_platform_mutex_free_fn_t		mutex_free_fn;

    /* The platform lock function */
    nrf_cc310_platform_mutex_lock_fn_t	 	mutex_lock_fn;

    /* The platform unlock function */
    nrf_cc310_platform_mutex_unlock_fn_t 	mutex_unlock_fn;
} nrf_cc310_platform_mutex_apis_t;


/** @brief Type definition of structure to platform hw mutexes
 */
typedef struct nrf_cc310_platform_mutexes_t
{
    /* Mutex for symmetric operations. */
    void * sym_mutex;

    /* Mutex for asymetric operations. */
    void * asym_mutex;

    /* Mutex for rng operations. */
    void * rng_mutex;

    /* Mutex reserved for future use. */
    void * reserved;

    /* Mutex for power mode changes */
    void * power_mutex;
} nrf_cc310_platform_mutexes_t;


/**@brief External reference to structure holding the currently set platform
 * mutexe APIs.
 */
extern nrf_cc310_platform_mutex_apis_t 	platform_mutex_apis;


/**@brief External reference to currently set platform hw mutexes */
extern nrf_cc310_platform_mutexes_t	platform_mutexes;


/** @brief Function to set platform mutex APIs and mutexes
 *
 * @param[in] apis              Structure holding the mutex APIs.
 * @param[in] mutexes           Structure holding the mutexes.
 */
void nrf_cc310_platform_set_mutexes(
    nrf_cc310_platform_mutex_apis_t const * const apis,
    nrf_cc310_platform_mutexes_t const * const mutexes);


/** @brief Function to initialize RTOS thread-safe mutexes
 *
 * This function must be implemented to set the platform mutex APIS,
 * and platform mutexes.
 *
 * @note This function must be called once before calling
 * @ref nrf_cc310_platform_init or @ref nrf_cc310_platform_init_no_rng.
 *
 * @note This function is not expected to be thread-safe.
 */
void nrf_cc310_platform_mutex_init(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC310_PLATFORM_MUTEX_H__ */
