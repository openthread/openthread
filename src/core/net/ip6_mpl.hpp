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

#ifndef IP6_MPL_HPP_
#define IP6_MPL_HPP_

/**
 * @file
 *   This file includes definitions for MPL.
 */

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-mpl
 *
 * @brief
 *   This module includes definitions for MPL.
 *
 * @{
 */

/**
 * Implements MPL header generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class MplOption : public Option
{
public:
    static constexpr uint8_t kType    = 0x6d;                 ///< MPL option type - 01 1 01101
    static constexpr uint8_t kMinSize = (2 + sizeof(Option)); ///< Minimum size (num of bytes) of `MplOption`

    /**
     * MPL Seed Id Lengths.
     */
    enum SeedIdLength : uint8_t
    {
        kSeedIdLength0  = 0 << 6, ///< 0-byte MPL Seed Id Length.
        kSeedIdLength2  = 1 << 6, ///< 2-byte MPL Seed Id Length.
        kSeedIdLength8  = 2 << 6, ///< 8-byte MPL Seed Id Length.
        kSeedIdLength16 = 3 << 6, ///< 16-byte MPL Seed Id Length.
    };

    /**
     * Initializes the MPL Option.
     *
     * The @p aSeedIdLength MUST be either `kSeedIdLength0` or `kSeedIdLength2`. Other values are not supported.
     *
     * @param[in] aSeedIdLength   The MPL Seed Id Length.
     */
    void Init(SeedIdLength aSeedIdLength);

    /**
     * Returns the MPL Seed Id Length value.
     *
     * @returns The MPL Seed Id Length value.
     */
    SeedIdLength GetSeedIdLength(void) const { return static_cast<SeedIdLength>(mControl & kSeedIdLengthMask); }

    /**
     * Indicates whether or not the MPL M flag is set.
     *
     * @retval TRUE   If the MPL M flag is set.
     * @retval FALSE  If the MPL M flag is not set.
     */
    bool IsMaxFlagSet(void) const { return (mControl & kMaxFlag) != 0; }

    /**
     * Clears the MPL M flag.
     */
    void ClearMaxFlag(void) { mControl &= ~kMaxFlag; }

    /**
     * Sets the MPL M flag.
     */
    void SetMaxFlag(void) { mControl |= kMaxFlag; }

    /**
     * Returns the MPL Sequence value.
     *
     * @returns The MPL Sequence value.
     */
    uint8_t GetSequence(void) const { return mSequence; }

    /**
     * Sets the MPL Sequence value.
     *
     * @param[in]  aSequence  The MPL Sequence value.
     */
    void SetSequence(uint8_t aSequence) { mSequence = aSequence; }

    /**
     * Returns the MPL Seed Id value.
     *
     * @returns The MPL Seed Id value.
     */
    uint16_t GetSeedId(void) const { return BigEndian::HostSwap16(mSeedId); }

    /**
     * Sets the MPL Seed Id value.
     *
     * @param[in]  aSeedId  The MPL Seed Id value.
     */
    void SetSeedId(uint16_t aSeedId) { mSeedId = BigEndian::HostSwap16(aSeedId); }

private:
    static constexpr uint8_t kSeedIdLengthMask = 3 << 6;
    static constexpr uint8_t kMaxFlag          = 1 << 5;

    uint8_t  mControl;
    uint8_t  mSequence;
    uint16_t mSeedId;
} OT_TOOL_PACKED_END;

/**
 * Implements MPL message processing.
 */
class Mpl : public InstanceLocator, private NonCopyable
{
    friend class ot::TimeTicker;

public:
    /**
     * Initializes the MPL object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Mpl(Instance &aInstance);

    /**
     * Initializes the MPL option.
     *
     * @param[in]  aOption   A reference to the MPL header to initialize.
     * @param[in]  aAddress  A reference to the IPv6 Source Address.
     */
    void InitOption(MplOption &aOption, const Address &aAddress);

    /**
     * Processes an MPL option. When the MPL module acts as an MPL Forwarder
     * it disseminates MPL Data Message using Trickle timer expirations. When acts as an
     * MPL Seed it allows to send the first MPL Data Message directly, then sets up Trickle
     * timer expirations for subsequent retransmissions.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aOffsetRange  The offset range in @p aMessage to read the MPL option.
     * @param[in]  aAddress      A reference to the IPv6 Source Address.
     * @param[out] aReceive      Set to FALSE if the MPL message is a duplicate and must not
     *                           go through the receiving process again, untouched otherwise.
     *
     * @retval kErrorNone  Successfully processed the MPL option.
     * @retval kErrorDrop  The MPL message is a duplicate and should be dropped.
     */
    Error ProcessOption(Message &aMessage, const OffsetRange &aOffsetRange, const Address &aAddress, bool &aReceive);

#if OPENTHREAD_FTD
    /**
     * Returns a reference to the buffered message set.
     *
     * @returns A reference to the buffered message set.
     */
    const MessageQueue &GetBufferedMessageSet(void) const { return mBufferedMessageSet; }
#endif

private:
    static constexpr uint16_t kNumSeedEntries      = OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES;
    static constexpr uint32_t kSeedEntryLifetime   = OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME;
    static constexpr uint32_t kSeedEntryLifetimeDt = 1000;
    static constexpr uint8_t  kDataMessageInterval = 64;

    struct SeedEntry
    {
        uint16_t mSeedId;
        uint8_t  mSequence;
        uint8_t  mLifetime;
    };

    void  HandleTimeTick(void);
    Error UpdateSeedSet(uint16_t aSeedId, uint8_t aSequence);

    SeedEntry mSeedSet[kNumSeedEntries];
    uint8_t   mSequence;

#if OPENTHREAD_FTD
    static constexpr uint8_t kChildRetransmissions  = 0; // MPL retransmissions for Children.
    static constexpr uint8_t kRouterRetransmissions = 2; // MPL retransmissions for Routers.

    struct Metadata : public Message::FooterData<Metadata>
    {
        void GenerateNextTransmissionTime(TimeMilli aCurrentTime, uint8_t aInterval);

        TimeMilli mTransmissionTime;
        uint16_t  mSeedId;
        uint8_t   mSequence;
        uint8_t   mTransmissionCount;
        uint8_t   mIntervalOffset;
    };

    uint8_t DetermineMaxRetransmissions(void) const;
    void    HandleRetransmissionTimer(void);
    void    AddBufferedMessage(Message &aMessage, uint16_t aSeedId, uint8_t aSequence);

    using RetxTimer = TimerMilliIn<Mpl, &Mpl::HandleRetransmissionTimer>;

    MessageQueue mBufferedMessageSet;
    RetxTimer    mRetransmissionTimer;
#endif // OPENTHREAD_FTD
};

/**
 * @}
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_MPL_HPP_
