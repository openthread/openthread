/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr.h>
#include <kernel.h>

#include "nrf_cc310_platform_defines.h"
#include "nrf_cc310_platform_mutex.h"
#include "nrf_cc310_platform_abort.h"

/** @brief External reference to the platforms abort APIs
 *  	   This is used in case the mutex functions don't
 * 		   provide return values in their APIs.
 */
extern nrf_cc310_platform_abort_apis_t platform_abort_apis;

/** @brief Definition of mutex for symmetric cryptography
 */
K_MUTEX_DEFINE(sym_mutex_int);

/** @brief Definition of mutex for asymmetric cryptography
 */
K_MUTEX_DEFINE(asym_mutex_int);

/** @brief Definition of mutex for random number generation
*/
K_MUTEX_DEFINE(rng_mutex_int);

/** @brief Definition of mutex for power mode changes
*/
K_MUTEX_DEFINE(power_mutex_int);

/** @brief Arbritary number of mutexes the system suppors
 */
#define NUM_MUTEXES 64

/** @brief Structure definition of the mutex slab
 */
struct k_mem_slab mutex_slab;

/** @brief Definition of buffer used for the mutex slabs
 */
char __aligned(4) mutex_slab_buffer[NUM_MUTEXES * sizeof(struct k_mutex)];

/**@brief Definition of RTOS-independent symmetric cryptography mutex
 * with NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
nrf_cc310_platform_mutex_t sym_mutex =
{
    .mutex = &sym_mutex_int,
    .flags = NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID
};


/**@brief Definition of RTOS-independent asymmetric cryptography mutex
 * with NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
nrf_cc310_platform_mutex_t asym_mutex =
{
    .mutex = &asym_mutex_int,
    .flags = NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID
};


/**@brief Definition of RTOS-independent random number generation mutex
 * with NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
nrf_cc310_platform_mutex_t rng_mutex =
{
    .mutex = &rng_mutex_int,
    .flags = NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID
};


/**@brief Definition of RTOS-independent power management mutex
 * with NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
nrf_cc310_platform_mutex_t power_mutex =
{
    .mutex = &power_mutex_int,
    .flags = NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID
};


/**@brief static function to initialize a mutex
 */
static void mutex_init(nrf_cc310_platform_mutex_t *mutex) {
    int ret;
    struct k_mutex * p_mutex;

    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn(
            "mutex_init called with NULL parameter");
    }

    /* Allocate if this has not been initialized statically */
    if (mutex->flags == NRF_CC310_PLATFORM_MUTEX_MASK_INVALID &&
        mutex->mutex == NULL) {
        /* allocate some memory for the mute*/
        ret = k_mem_slab_alloc(&mutex_slab, &mutex->mutex, K_FOREVER);
        if(ret != 0 || mutex->mutex == NULL)
        {
            /* Allocation failed. Abort all operations */
            platform_abort_apis.abort_fn(
                "Could not allocate mutex before initializing");
        }

        memset(mutex->mutex, 0, sizeof(struct k_mutex));

        /** Set a flag to ensure that mutex is deallocated by the freeing
         * operation
         */
        mutex->flags |= NRF_CC310_PLATFORM_MUTEX_MASK_IS_ALLOCATED;
    }

    p_mutex = (struct k_mutex *)mutex->mutex;
    k_mutex_init(p_mutex);

    /* Set the mask to indicate that the mutex is valid */
    mutex->flags |= NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID;
}


/** @brief Static function to free a mutex
 */
static void mutex_free(nrf_cc310_platform_mutex_t *mutex) {
    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn(
            "mutex_init called with NULL parameter");
    }

    /* Check if we are freeing a mutex that isn't initialized */
    if (mutex->flags == NRF_CC310_PLATFORM_MUTEX_MASK_INVALID) {
        /*Nothing to free*/
        return;
    }

    /* Check if the mutex was allocated or being statically defined */
    if ((mutex->flags & NRF_CC310_PLATFORM_MUTEX_MASK_IS_ALLOCATED) == 0) {
        k_mem_slab_free(&mutex_slab, mutex->mutex);
        mutex->mutex = NULL;
    }
    else {
        memset(mutex->mutex, 0, sizeof(struct k_mutex));
    }

    /* Reset the mutex to invalid state */
    mutex->flags = NRF_CC310_PLATFORM_MUTEX_MASK_INVALID;
}


/** @brief Static function to lock a mutex
 */
static int32_t mutex_lock(nrf_cc310_platform_mutex_t *mutex) {
    int ret;
    struct k_mutex * p_mutex;

    /* Ensure that the mutex param is valid (not NULL) */
    if(mutex == NULL) {
        return NRF_CC310_PLATFORM_ERROR_PARAM_NULL;
    }

    /* Ensure that the mutex has been initialized */
    if (mutex->flags == NRF_CC310_PLATFORM_MUTEX_MASK_INVALID) {
        return NRF_CC310_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED;
    }

    p_mutex = (struct k_mutex *)mutex->mutex;

    ret = k_mutex_lock(p_mutex, K_FOREVER);
    if (ret == 0) {
        return NRF_CC310_PLATFORM_SUCCESS;
    }
    else {
        return NRF_CC310_PLATFORM_ERROR_MUTEX_FAILED;
    }
}


/** @brief Static function to unlock a mutex
 */
static int32_t mutex_unlock(nrf_cc310_platform_mutex_t *mutex) {
    struct k_mutex * p_mutex;

    /* Ensure that the mutex param is valid (not NULL) */
    if(mutex == NULL) {
        return NRF_CC310_PLATFORM_ERROR_PARAM_NULL;
    }

    /* Ensure that the mutex has been initialized */
    if (mutex->flags == NRF_CC310_PLATFORM_MUTEX_MASK_INVALID) {
        return NRF_CC310_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED;
    }

    p_mutex = (struct k_mutex *)mutex->mutex;

    k_mutex_unlock(p_mutex);
    return NRF_CC310_PLATFORM_SUCCESS;
}


/**@brief Constant definition of mutex APIs to set in nrf_cc310_platform
 */
static const nrf_cc310_platform_mutex_apis_t mutex_apis =
{
    .mutex_init_fn = mutex_init,
    .mutex_free_fn = mutex_free,
    .mutex_lock_fn = mutex_lock,
    .mutex_unlock_fn = mutex_unlock
};


/** @brief Constant definition of mutexes to set in nrf_cc310_platform
 */
static const nrf_cc310_platform_mutexes_t mutexes =
{
    .sym_mutex = &sym_mutex,
    .asym_mutex = &asym_mutex,
    .rng_mutex = &rng_mutex,
    .reserved  = NULL,
    .power_mutex = &power_mutex,
};

/** @brief Function to initialize the nrf_cc310_platform mutex APIs
 */
void nrf_cc310_platform_mutex_init(void)
{
    k_mem_slab_init(&mutex_slab,
            mutex_slab_buffer,
            sizeof(struct k_mutex),
            NUM_MUTEXES);

    nrf_cc310_platform_set_mutexes(&mutex_apis, &mutexes);
}
