/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc310_platform_defines nrf_cc310_platform shared defines
 * @ingroup nrf_cc310_platform
 * @{
 * @brief nrf_cc310_platform shared defines and return codes.
 */
#ifndef NRF_CC310_PLATFORM_DEFINES_H__
#define NRF_CC310_PLATFORM_DEFINES_H__

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Definition of max number of entropy bits to gather for CTR_DRBG
 */
#define NRF_CC310_PLATFORM_ENTROPY_MAX_GATHER              (144)


/** @brief Definition of max count of concurrent usage
 *
 *  @note The max value will never be reached.
 */
#define NRF_CC310_PLATFORM_USE_COUNT_MAX                    (10)


#define NRF_CC310_PLATFORM_SUCCESS                          (0)
#define NRF_CC310_PLATFORM_ERROR_PARAM_NULL                 (-0x7001)
#define NRF_CC310_PLATFORM_ERROR_INTERNAL                   (-0x7002)
#define NRF_CC310_PLATFORM_ERROR_RNG_TEST_FAILED            (-0x7003)
#define NRF_CC310_PLATFORM_ERROR_HW_VERSION_FAILED          (-0x7004)
#define NRF_CC310_PLATFORM_ERROR_PARAM_WRITE_FAILED         (-0x7005)
#define NRF_CC310_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED      (-0x7016)
#define NRF_CC310_PLATFORM_ERROR_MUTEX_FAILED               (-0x7017)
#define NRF_CC310_PLATFORM_ERROR_ENTROPY_NOT_INITIALIZED    (-0x7018)
#define NRF_CC310_PLATFORM_ERROR_ENTROPY_TRNG_TOO_LONG      (-0x7019)


#ifdef __cplusplus
}
#endif

#endif /* NRF_CC310_PLATFORM_DEFINES_H__ */

/** @} */
