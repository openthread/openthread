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

#define PLATFORM_FEM_DEFAULT_PA_PIN                      26  /**< Default Power Amplifier pin. */
#define PLATFORM_FEM_DEFAULT_LNA_PIN                     27  /**< Default Low Noise Amplifier pin. */
#define PLATFORM_FEM_DEFAULT_SET_PPI_CHANNEL             15  /**< Default PPI channel for pin setting. */
#define PLATFORM_FEM_DEFAULT_CLR_PPI_CHANNEL             16  /**< Default PPI channel for pin clearing. */
#define PLATFORM_FEM_DEFAULT_TIMER_MATCH_PPI_GROUP       4   /**< Default PPI channel group used to disable timer match PPI. */
#define PLATFORM_FEM_DEFAULT_RADIO_DISABLED_PPI_GROUP    5   /**< Default PPI channel group used to disable radio disabled PPI. */
#define PLATFORM_FEM_DEFAULT_LNA_GPIOTE_CHANNEL          6   /**< Default LNA GPIOTE channel for FEM control. */
#define PLATFORM_FEM_DEFAULT_PA_GPIOTE_CHANNEL           7   /**< Default PA GPIOTE channel for FEM control. */

// clang-format on

#define PLATFORM_FEM_DEFAULT_CONFIG                                \
    ((PlatformFemConfigParams){                                    \
        .mPaCfg =                                                  \
            {                                                      \
                .mEnable     = 1,                                  \
                .mActiveHigh = 1,                                  \
                .mGpioPin    = PLATFORM_FEM_DEFAULT_PA_PIN,        \
            },                                                     \
        .mLnaCfg =                                                 \
            {                                                      \
                .mEnable     = 1,                                  \
                .mActiveHigh = 1,                                  \
                .mGpioPin    = PLATFORM_FEM_DEFAULT_LNA_PIN,       \
            },                                                     \
        .mPpiChIdClr    = PLATFORM_FEM_DEFAULT_CLR_PPI_CHANNEL,    \
        .mPpiChIdSet    = PLATFORM_FEM_DEFAULT_SET_PPI_CHANNEL,    \
        .mGpiotePaChId  = PLATFORM_FEM_DEFAULT_PA_GPIOTE_CHANNEL,  \
        .mGpioteLnaChId = PLATFORM_FEM_DEFAULT_LNA_GPIOTE_CHANNEL, \
    })

/**
 * @brief Configuration parameters for the PA and LNA.
 */
typedef struct
{
    uint8_t mEnable : 1;     /**< Enable toggling for this amplifier */
    uint8_t mActiveHigh : 1; /**< Set the pin to be active high */
    uint8_t mGpioPin : 6;    /**< The GPIO pin to toggle for this amplifier */
} PlatformFemConfigPaLna;

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
    PlatformFemConfigPaLna mPaCfg;         /**< Power Amplifier configuration */
    PlatformFemConfigPaLna mLnaCfg;        /**< Low Noise Amplifier configuration */
    uint8_t                mPpiChIdSet;    /**< PPI channel used for radio pin setting */
    uint8_t                mPpiChIdClr;    /**< PPI channel used for radio pin clearing */
    uint8_t                mGpiotePaChId;  /**< GPIOTE channel used for radio PA pin toggling */
    uint8_t                mGpioteLnaChId; /**< GPIOTE channel used for radio LNA pin toggling */
} PlatformFemConfigParams;

/**
 * Function used to set parameters of FEM.
 *
 */
void PlatformFemSetConfigParams(const PlatformFemConfigParams *aConfig);

#endif // PLATFORM_FEM_H_
