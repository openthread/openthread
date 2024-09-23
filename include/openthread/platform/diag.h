/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 * @brief
 *   This file defines the platform diag interface.
 */

#ifndef OPENTHREAD_PLATFORM_DIAG_H_
#define OPENTHREAD_PLATFORM_DIAG_H_

#include <stddef.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-factory-diagnostics
 *
 * @brief
 *   This module includes the platform abstraction for diagnostics features.
 *
 * @{
 */

/**
 * Defines the gpio modes.
 */
typedef enum
{
    OT_GPIO_MODE_INPUT  = 0, ///< Input mode without pull resistor.
    OT_GPIO_MODE_OUTPUT = 1, ///< Output mode.
} otGpioMode;

/**
 * Pointer to callback to output platform diag messages.
 *
 * @param[in]  aFormat     The format string.
 * @param[in]  aArguments  The format string arguments.
 * @param[out] aContext    A pointer to the user context.
 */
typedef void (*otPlatDiagOutputCallback)(const char *aFormat, va_list aArguments, void *aContext);

/**
 * Sets the platform diag output callback.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aCallback   A pointer to a function that is called on outputting diag messages.
 * @param[in]  aContext    A pointer to the user context.
 */
void otPlatDiagSetOutputCallback(otInstance *aInstance, otPlatDiagOutputCallback aCallback, void *aContext);

/**
 * Processes a factory diagnostics command line.
 *
 * @param[in]   aInstance       The OpenThread instance for current request.
 * @param[in]   aArgsLength     The number of arguments in @p aArgs.
 * @param[in]   aArgs           The arguments of diagnostics command line.
 *
 * @retval  OT_ERROR_INVALID_ARGS       The command is supported but invalid arguments provided.
 * @retval  OT_ERROR_NONE               The command is successfully process.
 * @retval  OT_ERROR_INVALID_COMMAND    The command is not valid or not supported.
 */
otError otPlatDiagProcess(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);

/**
 * Enables/disables the factory diagnostics mode.
 *
 * @param[in]  aMode  TRUE to enable diagnostics mode, FALSE otherwise.
 */
void otPlatDiagModeSet(bool aMode);

/**
 * Indicates whether or not factory diagnostics mode is enabled.
 *
 * @returns TRUE if factory diagnostics mode is enabled, FALSE otherwise.
 */
bool otPlatDiagModeGet(void);

/**
 * Sets the channel to use for factory diagnostics.
 *
 * @param[in]  aChannel  The channel value.
 */
void otPlatDiagChannelSet(uint8_t aChannel);

/**
 * Sets the transmit power to use for factory diagnostics.
 *
 * @param[in]  aTxPower  The transmit power value.
 */
void otPlatDiagTxPowerSet(int8_t aTxPower);

/**
 * Processes the received radio frame.
 *
 * @param[in]   aInstance   The OpenThread instance for current request.
 * @param[in]   aFrame      The received radio frame.
 * @param[in]   aError      The received radio frame status.
 */
void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError);

/**
 * Processes the alarm event.
 *
 * @param[in]   aInstance   The OpenThread instance for current request.
 */
void otPlatDiagAlarmCallback(otInstance *aInstance);

/**
 * Sets the gpio value.
 *
 * @param[in]  aGpio   The gpio number.
 * @param[in]  aValue  true to set the gpio to high level, or false otherwise.
 *
 * @retval OT_ERROR_NONE             Successfully set the gpio.
 * @retval OT_ERROR_FAILED           A platform error occurred while setting the gpio.
 * @retval OT_ERROR_INVALID_ARGS     @p aGpio is not supported.
 * @retval OT_ERROR_INVALID_STATE    Diagnostic mode was not enabled or @p aGpio is not configured as output.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented or configured on the platform.
 */
otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue);

/**
 * Gets the gpio value.
 *
 * @param[in]   aGpio   The gpio number.
 * @param[out]  aValue  A pointer where to put gpio value.
 *
 * @retval OT_ERROR_NONE             Successfully got the gpio value.
 * @retval OT_ERROR_FAILED           A platform error occurred while getting the gpio value.
 * @retval OT_ERROR_INVALID_ARGS     @p aGpio is not supported or @p aValue is NULL.
 * @retval OT_ERROR_INVALID_STATE    Diagnostic mode was not enabled or @p aGpio is not configured as input.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented or configured on the platform.
 */
otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue);

/**
 * Sets the gpio mode.
 *
 * @param[in]   aGpio   The gpio number.
 * @param[out]  aMode   The gpio mode.
 *
 * @retval OT_ERROR_NONE             Successfully set the gpio mode.
 * @retval OT_ERROR_FAILED           A platform error occurred while setting the gpio mode.
 * @retval OT_ERROR_INVALID_ARGS     @p aGpio or @p aMode is not supported.
 * @retval OT_ERROR_INVALID_STATE    Diagnostic mode was not enabled.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented or configured on the platform.
 */
otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode);

/**
 * Gets the gpio mode.
 *
 * @param[in]   aGpio   The gpio number.
 * @param[out]  aMode   A pointer where to put gpio mode.
 *
 * @retval OT_ERROR_NONE             Successfully got the gpio mode.
 * @retval OT_ERROR_FAILED           Mode returned by the platform is not implemented in OpenThread or a platform error
 *                                   occurred while getting the gpio mode.
 * @retval OT_ERROR_INVALID_ARGS     @p aGpio is not supported or @p aMode is NULL.
 * @retval OT_ERROR_INVALID_STATE    Diagnostic mode was not enabled.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented or configured on the platform.
 */
otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode);

/**
 * Set the radio raw power setting for diagnostics module.
 *
 * @param[in] aInstance               The OpenThread instance structure.
 * @param[in] aRawPowerSetting        A pointer to the raw power setting byte array.
 * @param[in] aRawPowerSettingLength  The length of the @p aRawPowerSetting.
 *
 * @retval OT_ERROR_NONE             Successfully set the raw power setting.
 * @retval OT_ERROR_INVALID_ARGS     The @p aRawPowerSetting is NULL or the @p aRawPowerSettingLength is too long.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This method is not implemented.
 */
otError otPlatDiagRadioSetRawPowerSetting(otInstance    *aInstance,
                                          const uint8_t *aRawPowerSetting,
                                          uint16_t       aRawPowerSettingLength);

/**
 * Get the radio raw power setting for diagnostics module.
 *
 * @param[in]      aInstance               The OpenThread instance structure.
 * @param[out]     aRawPowerSetting        A pointer to the raw power setting byte array.
 * @param[in,out]  aRawPowerSettingLength  On input, a pointer to the size of @p aRawPowerSetting.
 *                                         On output, a pointer to the length of the raw power setting data.
 *
 * @retval OT_ERROR_NONE             Successfully set the raw power setting.
 * @retval OT_ERROR_INVALID_ARGS     The @p aRawPowerSetting or @p aRawPowerSettingLength is NULL or
 *                                   @aRawPowerSettingLength is too short.
 * @retval OT_ERROR_NOT_FOUND        The raw power setting is not set.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This method is not implemented.
 */
otError otPlatDiagRadioGetRawPowerSetting(otInstance *aInstance,
                                          uint8_t    *aRawPowerSetting,
                                          uint16_t   *aRawPowerSettingLength);

/**
 * Enable/disable the platform layer to use the raw power setting set by `otPlatDiagRadioSetRawPowerSetting()`.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aEnable   TRUE to enable or FALSE to disable the raw power setting.
 *
 * @retval OT_ERROR_NONE             Successfully enabled/disabled the raw power setting.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This method is not implemented.
 */
otError otPlatDiagRadioRawPowerSettingEnable(otInstance *aInstance, bool aEnable);

/**
 * Start/stop the platform layer to transmit continuous carrier wave.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aEnable   TRUE to enable or FALSE to disable the platform layer to transmit continuous carrier wave.
 *
 * @retval OT_ERROR_NONE             Successfully enabled/disabled .
 * @retval OT_ERROR_INVALID_STATE    The radio was not in the Receive state.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This method is not implemented.
 */
otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable);

/**
 * Start/stop the platform layer to transmit stream of characters.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aEnable   TRUE to enable or FALSE to disable the platform layer to transmit stream.
 *
 * @retval OT_ERROR_NONE             Successfully enabled/disabled.
 * @retval OT_ERROR_INVALID_STATE    The radio was not in the Receive state.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented.
 */
otError otPlatDiagRadioTransmitStream(otInstance *aInstance, bool aEnable);

/**
 * Get the power settings for the given channel.
 *
 * @param[in]      aInstance               The OpenThread instance structure.
 * @param[in]      aChannel                The radio channel.
 * @param[out]     aTargetPower            The target power in 0.01 dBm.
 * @param[out]     aActualPower            The actual power in 0.01 dBm.
 * @param[out]     aRawPowerSetting        A pointer to the raw power setting byte array.
 * @param[in,out]  aRawPowerSettingLength  On input, a pointer to the size of @p aRawPowerSetting.
 *                                         On output, a pointer to the length of the raw power setting data.
 *
 * @retval  OT_ERROR_NONE             Successfully got the target power.
 * @retval  OT_ERROR_INVALID_ARGS     The @p aChannel is invalid, @aTargetPower, @p aActualPower, @p aRawPowerSetting or
 *                                    @p aRawPowerSettingLength is NULL or @aRawPowerSettingLength is too short.
 * @retval  OT_ERROR_NOT_FOUND        The power settings for the @p aChannel was not found.
 * @retval  OT_ERROR_NOT_IMPLEMENTED  This method is not implemented.
 */
otError otPlatDiagRadioGetPowerSettings(otInstance *aInstance,
                                        uint8_t     aChannel,
                                        int16_t    *aTargetPower,
                                        int16_t    *aActualPower,
                                        uint8_t    *aRawPowerSetting,
                                        uint16_t   *aRawPowerSettingLength);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_DIAG_H_
