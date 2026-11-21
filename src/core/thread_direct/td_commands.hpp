/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for Thread Direct Command Frames.
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_COMMANDS_HPP_
#define OT_CORE_THREAD_DIRECT_TD_COMMANDS_HPP_

#include "openthread-core-config.h"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "common/clearable.hpp"
#include "common/error.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"

namespace ot {
namespace ThreadDirect {

/**
 * Defines utility helpers shared across Thread Direct MAC command classes.
 */
class CommandUtils
{
protected:
    CommandUtils(void) = delete;

    static Error ParseCommandHeader(uint8_t aSubCmdId, FrameData &aPayloadData);
    static Error AppendCommandHeader(uint8_t aSubCmdId, FrameBuilder &aBuilder);
};

/**
 * Defines constants and payload structures for the Thread Direct Wake Command.
 */
class WakeCommand : private CommandUtils
{
public:
    static constexpr uint8_t kSubCmdId = 0x01; ///< Wake Command Sub-ID.

    static constexpr uint8_t kDefaultRetryInterval = 1; ///< Default Retry Interval.
    static constexpr uint8_t kDefaultRetryCount    = 8; ///< Default Retry Count.

    /**
     * Represents the Wake Command payload information.
     */
    struct Info : public Clearable<Info>
    {
        /**
         * Validates the Wake Command `Info`.
         *
         * Checks that `mRetryInterval` and `mRetryCount` are within valid ranges when `mHasConnParams` is set, and
         * that `mChannel` is a valid radio channel when `mHasChannel` is set.
         *
         * @retval TRUE   The Wake Command info is valid.
         * @retval FALSE  One or more Wake Command info fields are invalid.
         */
        bool IsValid(void) const;

        /**
         * Parses the Wake Command payload from a given `FrameData`.
         *
         * Parses the fields as-is from @p aPayloadData without validating their values (callers can use `IsValid()` to
         * check validity).
         *
         * @param[in] aPayloadData  The frame payload data starting at the MAC Command ID.
         *
         * @retval kErrorNone   Successfully parsed the Wake Command payload.
         * @retval kErrorParse  Failed to parse the command header or a required parameter was truncated.
         */
        Error ParseFrom(const FrameData &aPayloadData);

        /**
         * Encodes the Wake Command payload into a `FrameBuilder`.
         *
         * Encodes the header and any present fields as-is without calling `IsValid()`.
         *
         * @param[in,out] aBuilder  The frame builder to append the Wake Command to.
         *
         * @retval kErrorNone    Successfully encoded the Wake Command.
         * @retval kErrorNoBufs  Insufficient space remaining in @p aBuilder.
         */
        Error PrepareIn(FrameBuilder &aBuilder) const;

        bool    mHasRendezvousTime : 1; ///< Whether Rendezvous Time is present.
        bool    mHasConnParams : 1;     ///< Whether Connection Parameters are present.
        bool    mHasChannel : 1;        ///< Whether Channel is present.
        bool    mHasActionSelect : 1;   ///< Whether Action Select is present.
        uint8_t mRendezvousTime;        ///< Rendezvous Time (in units of 625 us) when `mHasRendezvousTime`.
        uint8_t mRetryInterval;         ///< Retry Interval when `mHasConnParams`.
        uint8_t mRetryCount;            ///< Retry Count when `mHasConnParams`.
        uint8_t mChannel;               ///< RX channel when `mHasChannel`.
        uint8_t mActionSelect;          ///< Action Select when `mHasActionSelect`.
    };

private:
    static constexpr uint8_t kFlagRendezvousTime = (1 << 7);
    static constexpr uint8_t kFlagConnParams     = (1 << 6);
    static constexpr uint8_t kFlagChannel        = (1 << 5);
    static constexpr uint8_t kFlagActionSelect   = (1 << 4);

    static constexpr uint8_t kRetryIntervalMask = (0xf << 4);
    static constexpr uint8_t kRetryCountMask    = (0xf << 0);

    static constexpr uint8_t kMinRetryInterval = 1;
    static constexpr uint8_t kMaxRetryInterval = 15;
    static constexpr uint8_t kMaxRetryCount    = 15;
};

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_COMMANDS_HPP_
