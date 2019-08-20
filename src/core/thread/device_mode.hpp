/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for MLE device mode.
 */

#ifndef DEVICE_MODE_HPP_
#define DEVICE_MODE_HPP_

#include "openthread-core-config.h"

#include <openthread/thread.h>

#include "common/string.hpp"
#include "utils/wrap_stdint.h"

namespace ot {
namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 * @brief
 *   This module includes definition for MLE device mode.
 *
 * @{
 *
 */

/**
 * This type represents a MLE device mode.
 *
 */
class DeviceMode
{
public:
    enum
    {
        kModeRxOnWhenIdle      = 1 << 3, ///< If the device has its receiver on when not transmitting.
        kModeSecureDataRequest = 1 << 2, ///< If the device uses link layer security for all data requests.
        kModeFullThreadDevice  = 1 << 1, ///< If the device is an FTD.
        kModeFullNetworkData   = 1 << 0, ///< If the device requires the full Network Data.

        kInfoStringSize = 45, ///< String buffer size used for `ToString()`.
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     *  This structure represents an MLE Mode configuration.
     *
     */
    typedef otLinkModeConfig ModeConfig;

    /**
     * This is the default constructor for `DeviceMode` object.
     *
     */
    DeviceMode(void) {}

    /**
     * This constructor initializes a `DeviceMode` object from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(uint8_t aMode)
        : mMode(aMode)
    {
    }

    /**
     * This constructor initializes a `DeviceMode` object from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(ModeConfig aModeConfig) { Set(aModeConfig); }

    /**
     * This method gets the device mode as a mode TLV bitmask.
     *
     * @returns The device mode as a mode TLV bitmask.
     *
     */
    uint8_t Get(void) const { return mMode; }

    /**
     * This method sets the device mode from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask.
     *
     */
    void Set(uint8_t aMode) { mMode = aMode; }

    /**
     * This method gets the device mode as a mode configuration structure.
     *
     * @param[out] aModeConfig   A reference to a mode configuration structure to output the device mode.
     *
     */
    void Get(ModeConfig &aModeConfig) const;

    /**
     * this method sets the device mode from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration structure.
     *
     */
    void Set(const ModeConfig &aModeConfig);

    /**
     * This method indicates whether or not the device is rx-on-when-idle.
     *
     * @retval TRUE   If the device is rx-on-when-idle (non-sleepy).
     * @retval FALSE  If the device is not rx-on-when-idle (sleepy).
     *
     */
    bool IsRxOnWhenIdle(void) const { return (mMode & kModeRxOnWhenIdle) != 0; }

    /**
     * This method indicates whether or not the device uses secure IEEE 802.15.4 Data Request messages.
     *
     * @retval TRUE   If the device uses secure IEEE 802.15.4 Data Request (data poll) messages.
     * @retval FALSE  If the device uses any IEEE 802.15.4 Data Request (data poll) messages.
     *
     */
    bool IsSecureDataRequest(void) const { return (mMode & kModeSecureDataRequest) != 0; }

    /**
     * This method indicates whether or not the device is a Full Thread Device.
     *
     * @retval TRUE   If the device is Full Thread Device.
     * @retval FALSE  If the device if not Full Thread Device.
     *
     */
    bool IsFullThreadDevice(void) const { return (mMode & kModeFullThreadDevice) != 0; }

    /**
     * This method indicates whether or not the device requests Full Network Data.
     *
     * @retval TRUE   If the device requests Full Network Data.
     * @retval FALSE  If the device does not request Full Network Data (only stable Network Data).
     *
     */
    bool IsFullNetworkData(void) const { return (mMode & kModeFullNetworkData) != 0; }

    /**
     * This method indicates whether or not the device is a Minimal End Device.
     *
     * @retval TRUE   If the device is a Minimal End Device.
     * @retval FALSE  If the device is not a Minimal End Device.
     *
     */
    bool IsMinimalEndDevice(void) const
    {
        return (mMode & (kModeFullThreadDevice | kModeRxOnWhenIdle)) != (kModeFullThreadDevice | kModeRxOnWhenIdle);
    }

    /**
     * This method indicates whether or not the device mode flags are valid.
     *
     * An FTD which is not rx-on-when-idle (is sleepy) is considered invalid.
     *
     * @returns TRUE if , FALSE otherwise.
     * @retval TRUE   If the device mode flags are valid.
     * @retval FALSE  If the device mode flags are not valid.
     *
     */
    bool IsValid(void) const { return !IsFullThreadDevice() || IsRxOnWhenIdle(); }

    /**
     *  This method overloads operator `==` to evaluate whether or not two device modes are equal
     *
     * @param[in]  aOther  The other device mode to compare with.
     *
     * @retval TRUE   If the device modes are equal.
     * @retval FALSE  If the device modes are not equal.
     *
     */
    bool operator==(const DeviceMode &aOther) const { return (mMode == aOther.mMode); }

    /**
     * This method overloads operator `!=` to evaluate whether or not two device modes are not equal.
     *
     * @param[in]  aOther  The other device mode to compare with.
     *
     * @retval TRUE   If the device modes are not equal.
     * @retval FALSE  If the device modes are equal.
     *
     */
    bool operator!=(const DeviceMode &aOther) const { return (mMode != aOther.mMode); }

    /**
     * This method converts the device mode into a human-readable string.
     *
     * @returns An `InfoString` object representing the device mode.
     *
     */
    InfoString ToString(void) const;

private:
    uint8_t mMode;
};

/**
 * @}
 *
 */

} // namespace Mle
} // namespace ot

#endif // DEVICE_MODE_HPP_
