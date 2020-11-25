/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdint.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "nrf_cc3xx_platform_defines.h"
#include "nrf_cc3xx_platform_mutex.h"


/** @brief external reference to the cc3xx platform mutex APIs*/
extern nrf_cc3xx_platform_mutex_apis_t  platform_mutex_apis;

/** @brief Definition of mutex for symmetric cryptography
 */
static SemaphoreHandle_t sym_mutex_int;

/** @brief Definition of mutex for asymmetric cryptography
 */
static SemaphoreHandle_t asym_mutex_int;

/** @brief Definition of mutex for random number generation
 */
static SemaphoreHandle_t rng_mutex_int;

/** @brief Definition of mutex for power management changes
 */
static SemaphoreHandle_t power_mutex_int;

/** @brief Definition of mutex for heap allocations using buffer_mem_alloc.c
 */
static SemaphoreHandle_t heap_mutex_int;


/**@brief Definition of RTOS-independent symmetric cryptography mutex
 * with NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
static nrf_cc3xx_platform_mutex_t sym_mutex =
{
    .mutex = &sym_mutex_int,
    .flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
};


/**@brief Definition of RTOS-independent asymmetric cryptography mutex
 * with NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
static nrf_cc3xx_platform_mutex_t asym_mutex =
{
    .mutex = &asym_mutex_int,
    .flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
};


/**@brief Definition of RTOS-independent random number generation mutex
 * with NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
static nrf_cc3xx_platform_mutex_t rng_mutex =
{
    .mutex = &rng_mutex_int,
    .flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
};


/**@brief Definition of RTOS-independent power management mutex
 * with NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 * allocation is unneccesary
*/
static nrf_cc3xx_platform_mutex_t power_mutex =
{
    .mutex = &power_mutex_int,
    .flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
};

/** @brief Definition of RTOS-independent heap allocation mutex
 *  with NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID set to indicate that
 *  allocation is unneccesary
 *
 * @note This symbol can't be static as it is referenced in the replacement
 *       file mbemory_buffer_alloc.c inside the heap structure.
 */
nrf_cc3xx_platform_mutex_t heap_mutex =
{
    .mutex = &heap_mutex_int,
    .flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
};


/** @brief Static function to unlock a mutex
 */
static void mutex_init_platform(nrf_cc3xx_platform_mutex_t *mutex)
{
    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn("mutex_init called with NULL parameter");
    }

    if (mutex->flag == NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID)
    {
        mutex->mutex = (void*)xSemaphoreCreateMutex();
        if (mutex->mutex == NULL) {
            platform_abort_apis.abort_fn("Could not create mutex!");
        }
    }

    /* Set the mask to indicate that the mutex is valid */
    mutex->flags |= NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID;
}


/** @brief Static function to free a mutex
 */
static void mutex_free_platform(nrf_cc3xx_platform_mutex_t *mutex)
{
    SemaphoreHandle_t p_mutex;

    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn("mutex_free called with NULL parameter");
    }

    if (mutex->flags == NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID) {
        /*Nothing to free*/
        return;
    }

    p_mutex = (SemaphoreHandle_t) mutex->mutex;

    vSemaphoreDelete(p_mutex);

    /* Reset the mutex to invalid state */
    mutex->flags = NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID;
}


/** @brief Static function to lock a mutex
 */
static int mutex_lock_platform(nrf_cc3xx_platform_mutex_t *mutex)
{
    int ret;
    SemaphoreHandle_t p_mutex;

    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn("mutex_lock called with NULL parameter");
    }

    /* Ensure that the mutex has been initialized */
    if (mutex->flags == NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID) {
        return NRF_CC3XX_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED;
    }

    p_mutex = (SemaphoreHandle_t) mutex->mutex;

    ret = xSemaphoreTake(p_mutex, portMAX_DELAY);
    if (ret == pdTRUE) {
        return NRF_CC3XX_PLATFORM_SUCCESS;
    }
    else {
        return NRF_CC3XX_PLATFORM_ERROR_MUTEX_FAILED;
    }
}


/** @brief Static function to unlock a mutex
 */
static int mutex_unlock_platform(nrf_cc3xx_platform_mutex_t * mutex)
{
    int ret;
    SemaphoreHandle_t p_mutex;

    /* Ensure that the mutex is valid (not NULL) */
    if (mutex == NULL) {
        platform_abort_apis.abort_fn("mutex_unlock called with NULL parameter");
    }

    /* Ensure that the mutex has been initialized */
    if (mutex->flags == NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID) {
        return NRF_CC3XX_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED;
    }

    p_mutex = (SemaphoreHandle_t) mutex->mutex;

    ret = xSemaphoreGive(p_mutex);
    if (ret != pdTRUE) {
        platform_abort_apis.abort_fn("Could not unlock mutex!");
    }

    return NRF_CC3XX_PLATFORM_SUCCESS;
}


/**@brief Constant definition of mutex APIs to set in nrf_cc3xx_platform
 */
const nrf_cc3xx_platform_mutex_apis_t mutex_apis =
{
    .mutex_init_fn = mutex_init_platform,
    .mutex_free_fn = mutex_free_platform,
    .mutex_lock_fn = mutex_lock_platform,
    .mutex_unlock_fn = mutex_unlock_platform
};


/** @brief Constant definition of mutexes to set in nrf_cc3xx_platform
 */
const nrf_cc3xx_platform_mutexes_t mutexes =
{
    .sym_mutex = &sym_mutex,
    .asym_mutex = &asym_mutex,
    .rng_mutex = &rng_mutex,
    .reserved = NULL,
    .power_mutex = &power_mutex
};

/** @brief Function to initiialize the nrf_cc3xx_platform mutex APIs.
 */
void nrf_cc3xx_platform_mutex_init(void)
{
    nrf_cc3xx_platform_set_mutexes(&mutex_apis, &mutexes);
}
