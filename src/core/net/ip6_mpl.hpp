/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IP6_MPL_HPP_
#define IP6_MPL_HPP_

/**
 * @file
 *   This file includes definitions for MPL.
 */

#include <openthread-types.h>
#include <common/message.hpp>
#include <common/timer.hpp>
#include <net/ip6.hpp>

namespace Thread {
namespace Ip6 {

/**
 * @addtogroup core-ip6-mpl
 *
 * @brief
 *   This module includes definitions for MPL.
 *
 * @{
 *
 */

/**
 * This class implements MPL header generation and parsing.
 *
 */
class OptionMpl: public OptionHeader
{
public:
    enum
    {
        kType = 0x6d,    /* 01 1 01101 */
    };

    /**
     * This method initializes the MPL header.
     *
     */
    void Init() {
        OptionHeader::SetType(kType);
        OptionHeader::SetLength(sizeof(*this) - sizeof(OptionHeader));
    }

    /**
     * MPL Seed lengths.
     */
    enum SeedLength
    {
        kSeedLength0  = 0 << 6,  ///< 0-byte MPL Seed Length.
        kSeedLength2  = 1 << 6,  ///< 2-byte MPL Seed Length.
        kSeedLength8  = 2 << 6,  ///< 8-byte MPL Seed Length.
        kSeedLength16 = 3 << 6,  ///< 16-byte MPL Seed Length.
    };

    /**
     * This method returns the MPL Seed Length value.
     *
     * @returns The MPL Seed Length value.
     *
     */
    SeedLength GetSeedLength() { return static_cast<SeedLength>(mControl & kSeedLengthMask); }

    /**
     * This method sets the MPL Seed Length value.
     *
     * @param[in]  aSeedLength  The MPL Seed Length.
     *
     */
    void SetSeedLength(SeedLength aSeedLength) { mControl = (mControl & ~kSeedLengthMask) | aSeedLength; }

    /**
     * This method indicates whether or not the MPL M flag is set.
     *
     * @retval TRUE   If the MPL M flag is set.
     * @retval FALSE  If the MPL M flag is not set.
     *
     */
    bool IsMaxFlagSet() { return mControl & kMaxFlag; }

    /**
     * This method clears the MPL M flag.
     *
     */
    void ClearMaxFlag() { mControl &= ~kMaxFlag; }

    /**
     * This method sets the MPL M flag.
     *
     */
    void SetMaxFlag() { mControl |= kMaxFlag; }

    /**
     * This method returns the MPL Sequence value.
     *
     * @returns The MPL Sequence value.
     *
     */
    uint8_t GetSequence() const { return mSequence; }

    /**
     * This method sets the MPL Sequence value.
     *
     * @param[in]  aSequence  The MPL Sequence value.
     */
    void SetSequence(uint8_t aSequence) { mSequence = aSequence; }

    /**
     * This method returns the MPL Seed value.
     *
     * @returns The MPL Seed value.
     *
     */
    uint16_t GetSeed() const { return HostSwap16(mSeed); }

    /**
     * This method sets the MPL Seed value.
     *
     * @param[in]  aSeed  The MPL Seed value.
     */
    void SetSeed(uint16_t aSeed) { mSeed = HostSwap16(aSeed); }

private:
    enum
    {
        kSeedLengthMask = 3 << 6,
        kMaxFlag = 1 << 5,
    };
    uint8_t  mControl;
    uint8_t  mSequence;
    uint16_t mSeed;
} __attribute__((packed));

/**
 * This class implements MPL message processing.
 *
 */
class Mpl
{
public:
    /**
     * This constructor initializes the MPL object.
     *
     */
    Mpl(void);

    /**
     * This method initializes the MPL option.
     *
     * @param[in]  aOption  A reference to the MPL header to initialize.
     * @param[in]  aSeed    The MPL Seed value to use.
     *
     */
    void InitOption(OptionMpl &aOption, uint16_t aSeed);

    /**
     * This method processes an MPL option.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None  Successfully processed the MPL option.
     * @retval kThreadError_Drop  The MPL message is a duplicate and should be dropped.
     *
     */
    ThreadError ProcessOption(const Message &aMessage);

private:
    enum
    {
        kNumEntries = OPENTHREAD_CONFIG_MPL_CACHE_ENTRIES,
        kLifetime = OPENTHREAD_CONFIG_MPL_CACHE_ENTRY_LIFETIME,
    };

    static void HandleTimer(void *context);
    void HandleTimer();

    Timer mTimer;
    uint8_t mSequence;

    struct MplEntry
    {
        uint16_t mSeed;
        uint8_t mSequence;
        uint8_t mLifetime;
    };
    MplEntry mEntries[kNumEntries];
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_IP6_MPL_HPP_
