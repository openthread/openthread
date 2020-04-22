/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

/**
 * @file
 *   This file includes front end module platform-specific functions.
 *
 */

#ifndef PLATFORM_FEM_H_
#define PLATFORM_FEM_H_

#include <stdint.h>

// clang-format off

/**
 * @brief Configuration parameters for the Front End Module.
 */
#define PLATFORM_FEM_DEFAULT_PA_PIN                      26  /**< Default Power Amplifier pin. */
#define PLATFORM_FEM_DEFAULT_LNA_PIN                     27  /**< Default Low Noise Amplifier pin. */
#define PLATFORM_FEM_DEFAULT_PDN_PIN                     28  /**< Default Power Down pin. */
#define PLATFORM_FEM_DEFAULT_SET_PPI_CHANNEL             15  /**< Default PPI channel for pin setting. */
#define PLATFORM_FEM_DEFAULT_CLR_PPI_CHANNEL             16  /**< Default PPI channel for pin clearing. */
#define PLATFORM_FEM_DEFAULT_PDN_PPI_CHANNEL             14  /**< Default PPI channel for Power Down control. */
#define PLATFORM_FEM_DEFAULT_TIMER_MATCH_PPI_GROUP       4   /**< Default PPI channel group used to disable timer match PPI. */
#define PLATFORM_FEM_DEFAULT_RADIO_DISABLED_PPI_GROUP    5   /**< Default PPI channel group used to disable radio disabled PPI. */
#define PLATFORM_FEM_DEFAULT_PA_GPIOTE_CHANNEL           6   /**< Default PA GPIOTE channel for FEM control. */
#define PLATFORM_FEM_DEFAULT_LNA_GPIOTE_CHANNEL          7   /**< Default LNA GPIOTE channel for FEM control. */
#define PLATFORM_FEM_DEFAULT_PDN_GPIOTE_CHANNEL          5   /**< Default PDN GPIOTE channel for FEM control. */

/**
 * @brief Configuration parameters for the Front End Module timings and gain.
 */
#define PLATFORM_FEM_PA_TIME_IN_ADVANCE_US  13 /**< Default time in microseconds when PA GPIO is activated before the radio is ready for transmission. */
#define PLATFORM_FEM_LNA_TIME_IN_ADVANCE_US 13 /**< Default time in microseconds when LNA GPIO is activated before the radio is ready for reception. */
#define PLATFORM_FEM_PDN_SETTLE_US          18 /**< Default the time between activating the PDN and asserting the RX_EN/TX_EN. */
#define PLATFORM_FEM_TRX_HOLD_US            5  /**< Default the time between deasserting the RX_EN/TX_EN and deactivating PDN. */
#define PLATFORM_FEM_PA_GAIN_DB             0  /**< Default PA gain. Ignored if the amplifier is not supporting this feature. */
#define PLATFORM_FEM_LNA_GAIN_DB            0  /**< Default LNA gain. Ignored if the amplifier is not supporting this feature. */

// clang-format on

#define PLATFORM_FEM_DEFAULT_CONFIG                                     \
    ((PlatformFemConfigParams){                                         \
        .mFemPhyCfg =                                                   \
            {                                                           \
                .mPaTimeGapUs  = PLATFORM_FEM_PA_TIME_IN_ADVANCE_US,    \
                .mLnaTimeGapUs = PLATFORM_FEM_LNA_TIME_IN_ADVANCE_US,   \
                .mPdnSettleUs  = PLATFORM_FEM_PDN_SETTLE_US,            \
                .mTrxHoldUs    = PLATFORM_FEM_TRX_HOLD_US,              \
                .mPaGainDb     = PLATFORM_FEM_PA_GAIN_DB,               \
                .mLnaGainDb    = PLATFORM_FEM_LNA_GAIN_DB,              \
            },                                                          \
        .mPaCfg =                                                       \
            {                                                           \
                .mEnable     = 1,                                       \
                .mActiveHigh = 1,                                       \
                .mGpioPin    = PLATFORM_FEM_DEFAULT_PA_PIN,             \
                .mGpioteChId = PLATFORM_FEM_DEFAULT_PA_GPIOTE_CHANNEL,  \
            },                                                          \
        .mLnaCfg =                                                      \
            {                                                           \
                .mEnable     = 1,                                       \
                .mActiveHigh = 1,                                       \
                .mGpioPin    = PLATFORM_FEM_DEFAULT_LNA_PIN,            \
                .mGpioteChId = PLATFORM_FEM_DEFAULT_LNA_GPIOTE_CHANNEL, \
            },                                                          \
        .mPdnCfg =                                                      \
            {                                                           \
                .mEnable     = 1,                                       \
                .mActiveHigh = 1,                                       \
                .mGpioPin    = PLATFORM_FEM_DEFAULT_PDN_PIN,            \
                .mGpioteChId = PLATFORM_FEM_DEFAULT_PDN_GPIOTE_CHANNEL, \
            },                                                          \
        .mPpiChIdClr = PLATFORM_FEM_DEFAULT_CLR_PPI_CHANNEL,            \
        .mPpiChIdSet = PLATFORM_FEM_DEFAULT_SET_PPI_CHANNEL,            \
        .mPpiChIdPdn = PLATFORM_FEM_DEFAULT_PDN_PPI_CHANNEL,            \
    })

/**
 * @brief Configuration parameters for FEM PHY.
 */
typedef struct
{
    uint32_t mPaTimeGapUs;
    uint32_t mLnaTimeGapUs;
    uint32_t mPdnSettleUs;
    uint32_t mTrxHoldUs;
    uint8_t  mPaGainDb;
    uint8_t  mLnaGainDb;
} PlatformFemPhyConfig;

/**
 * @brief Configuration parameters for the PA and LNA.
 */
typedef struct
{
    uint8_t mEnable : 1;     /**< Enable toggling for this amplifier */
    uint8_t mActiveHigh : 1; /**< Set the pin to be active high */
    uint8_t mGpioPin : 6;    /**< The GPIO pin to toggle for this amplifier */
    uint8_t mGpioteChId;     /**< The GPIOTE Channel ID used for toggling pins */
} PlatformFemPinConfig;

/**
 * @brief PA & LNA GPIO toggle configuration
 *
 * This option configures the nRF 802.15.4 radio driver to toggle pins when the radio
 * is active for use with a power amplifier and/or a low noise amplifier.
 *
 * Toggling the pins is achieved by using two PPI channels and a GPIOTE channel. The hardware channel IDs are provided
 * by the application and should be regarded as reserved as long as any PA/LNA toggling is enabled.
 *
 * @note Changing this configuration while the radio is in use may have undefined
 *       consequences and must be avoided by the application.
 */
typedef struct
{
    PlatformFemPhyConfig mFemPhyCfg;  /**< Front End Module Physical layer configuration */
    PlatformFemPinConfig mPaCfg;      /**< Power Amplifier configuration */
    PlatformFemPinConfig mLnaCfg;     /**< Low Noise Amplifier configuration */
    PlatformFemPinConfig mPdnCfg;     /**< Power Down configuration */
    uint8_t              mPpiChIdSet; /**< PPI channel to be used for setting pins */
    uint8_t              mPpiChIdClr; /**< PPI channel to be used for clearing pins */
    uint8_t              mPpiChIdPdn; /**< PPI channel to handle PDN pin */
} PlatformFemConfigParams;

/**
 * Function used to set parameters of FEM.
 *
 */
void PlatformFemSetConfigParams(const PlatformFemConfigParams *aConfig);

#endif // PLATFORM_FEM_H_
