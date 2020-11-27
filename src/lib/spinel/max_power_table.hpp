/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#ifndef OT_LIB_SPINEL_MAX_POWER_TABLE_HPP_
#define OT_LIB_SPINEL_MAX_POWER_TABLE_HPP_

#include "core/radio/radio.hpp"

namespace ot {
namespace Spinel {

class MaxPowerTable
{
public:
    static const int8_t kPowerDefault = 30;   ///< Default power 1 watt (30 dBm).
    static const int8_t kPowerNone    = 0x7f; ///< Power not specified

    MaxPowerTable(void)
    {
        for (size_t i = 0; i <= Radio::kChannelMax - Radio::kChannelMin; i++)
        {
            mPowerTable[i]       = kPowerNone;
            mChannelSupported[i] = true;
            mChannelPreferred[i] = true;
        }

        memset(mPowerTable, kPowerNone, sizeof(mPowerTable));
    }

    /**
     * This method gets the max supported transmit power of channel @p aChannel.
     *
     * @params[in]  aChannel    The radio channel number.
     *
     * @returns The max supported transmit power in dBm.
     *
     */
    int8_t GetTransmitPower(uint8_t aChannel) const { return mPowerTable[aChannel - Radio::kChannelMin]; }

    /**
     * This method sets the max supported transmit power of channel @p aChannel.
     *
     * @params[in]  aChannel    The radio channel number.
     * @params[in]  aPower      The max supported transmit power in dBm.
     *
     */
    void SetTransmitPower(uint8_t aChannel, int8_t aPower) { mPowerTable[aChannel - Radio::kChannelMin] = aPower; }

    /**
     * This method sets whether a channel is supported
     *
     * @params[in]  aChannel    The radio channel number.
     * @params[in]  aSupported  Whether a channel is supported.
     *
     */
    void SetChannelSupported(uint8_t aChannel, bool aSupported)
    {
        mChannelSupported[aChannel - Radio::kChannelMin] = aSupported;
    }

    /**
     * This method sets whether a channel is preferred
     *
     * @params[in]  aChannel    The radio channel number.
     * @params[in]  aSupported  Whether a channel is supported.
     *
     */
    void SetChannelPreferred(uint8_t aChannel, bool aPreferred)
    {
        mChannelPreferred[aChannel - Radio::kChannelMin] = aPreferred;
    }

    /**
     * This method gets the supported channel masks.
     *
     */
    uint32_t GetSupportedChannelMask(void) const
    {
        uint32_t channelMask = 0;

        for (uint8_t i = Radio::kChannelMin; i <= Radio::kChannelMax; ++i)
        {
            if (mChannelSupported[i - Radio::kChannelMin])
            {
                channelMask |= (1 << i);
            }
        }

        return channelMask;
    }

    /**
     * This method gets the preferred channel masks.
     *
     */
    uint32_t GetPreferredChannelMask(void) const
    {
        uint32_t channelMask = 0;

        for (uint8_t i = Radio::kChannelMin; i <= Radio::kChannelMax; ++i)
        {
            if (mChannelPreferred[i - Radio::kChannelMin])
            {
                channelMask |= (1 << i);
            }
        }

        return channelMask;
    }

private:
    int8_t mPowerTable[Radio::kChannelMax - Radio::kChannelMin + 1];
    bool   mChannelSupported[Radio::kChannelMax - Radio::kChannelMin + 1];
    bool   mChannelPreferred[Radio::kChannelMax - Radio::kChannelMin + 1];
};

} // namespace Spinel
} // namespace ot

#endif // OT_LIB_SPINEL_MAX_POWER_TABLE_HPP_
