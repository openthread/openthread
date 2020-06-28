/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FSL_RNG_H_
#define __FSL_RNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fsl_common.h"

/*!
 * @addtogroup jn_rng
 * @{
 */

/*! @file */

/**
 * RNG return status types
 */

/**
 * RNG operating modes
 */
typedef enum _trng_mode
{
    trng_UpdateOnce  = 0x1, /*!< TRNG update once & disable */
    trng_FreeRunning = 0x2, /*!< TRNG updates continuously */
} trng_mode_t;

typedef struct _trng_config
{
    uint8_t shift4x;   /*!< Used to add precision to clock ratio & entropy refill - range from 0 to 4 */
    uint8_t clock_sel; /*!< Internal clock on which to compute statistics */
                       /*!< 0 - XOR results from all clocks */
                       /*!< 1 - First clock */
                       /*!< 2 - Second clock */
    trng_mode_t mode;  /*!< TRNG mode select */
} trng_config_t;

/*!
 * @brief Gets Default config of TRNG.
 *
 * This function initializes the TRNG configuration structure.
 *
 * @param userConfig Pointer to TRNG configuration structure
 */
status_t TRNG_GetDefaultConfig(trng_config_t *userConfig);

/*!
 * @brief Initializes the TRNG.
 *
 * This function initializes the TRNG.
 *
 * @param base TRNG base address
 * @param userConfig The configuration of TRNG
 * @return  kStatus_Success - Success
 *          kStatus_InvalidArgument - Invalid parameter
 */
status_t TRNG_Init(RNG_Type *base, const trng_config_t *userConfig);

/*!
 * @brief Shuts down the TRNG.
 *
 * This function shuts down the TRNG.
 *
 * @param base TRNG base address
 */
void TRNG_Deinit(RNG_Type *base);

/*!
 * @brief Gets random data.
 *
 * This function gets random data from the TRNG.
 *
 * @param base TRNG base address
 * @param data pointer to user buffer to be filled by random data
 * @param data_size size of data in bytes
 * @return TRNG status
 */
status_t TRNG_GetRandomData(RNG_Type *base, void *data, size_t data_size);

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __RNG_JN518X_H_*/
