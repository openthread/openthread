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

#include "radio.hpp"

#include "instance/instance.hpp"

namespace ot {

const uint8_t Radio::kSupportedChannelPages[kNumChannelPages] = {
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    kChannelPage0,
#endif
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    kChannelPage2,
#endif
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
    OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE,
#endif
};

#if OPENTHREAD_RADIO
void Radio::Init(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    SuccessOrAssert(ResetCsl());
#endif

    EnableSrcMatch(false);
    ClearSrcMatchShortEntries();
    ClearSrcMatchExtEntries();

    if (IsEnabled())
    {
        SuccessOrAssert(Sleep());
        SuccessOrAssert(Disable());
    }

    SetPanId(Mac::kPanIdBroadcast);
    SetExtendedAddress(Mac::ExtAddress{});
    SetShortAddress(Mac::kShortAddrInvalid);
    SetMacKey(0, 0, Mac::KeyMaterial{}, Mac::KeyMaterial{}, Mac::KeyMaterial{});
    SetMacFrameCounter(0);

    SetPromiscuous(false);
    SetRxOnWhenIdle(true);
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
}
#endif // OPENTHREAD_RADIO

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void Radio::SetExtendedAddress(const Mac::ExtAddress &aExtAddress)
{
    otPlatRadioSetExtendedAddress(GetInstancePtr(), &aExtAddress);

#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitExtendedAddress(aExtAddress);
#endif
}

void Radio::SetShortAddress(Mac::ShortAddress aShortAddress)
{
    otPlatRadioSetShortAddress(GetInstancePtr(), aShortAddress);

#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitShortAddress(aShortAddress);
#endif
}

Error Radio::Transmit(Mac::TxFrame &aFrame)
{
#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitTransmit(aFrame);
#endif

    return otPlatRadioTransmit(GetInstancePtr(), &aFrame);
}
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
inline uint64_t UintSafeMinus(uint64_t aLhs, uint64_t aRhs) { return aLhs > aRhs ? (aLhs - aRhs) : 0; }

RadioStatistics::RadioStatistics(void)
    : mStatus(kDisabled)
{
    ResetTime();
}

void RadioStatistics::RecordStateChange(Status aStatus)
{
    UpdateTime();
    mStatus = aStatus;
}

void RadioStatistics::HandleReceiveAt(uint32_t aDurationUs)
{
    // The actual rx time of ReceiveAt cannot be obtained from software level. This is a workaround.
    if (mStatus == kSleep)
    {
        mTimeStats.mRxTime += aDurationUs;
    }
}

void RadioStatistics::RecordTxDone(otError aError, uint16_t aPsduLength)
{
    if (aError == kErrorNone || aError == kErrorNoAck)
    {
        uint32_t txTimeUs = (aPsduLength + Mac::Frame::kPhyHeaderSize) * Radio::kSymbolsPerOctet * Radio::kSymbolTime;
        uint32_t rxAckTimeUs = (Mac::Frame::kImmAckLength + Mac::Frame::kPhyHeaderSize) * Radio::kPhyUsPerByte;

        UpdateTime();
        mTimeStats.mTxTime += txTimeUs;

        if (mStatus == kReceive)
        {
            mTimeStats.mRxTime = UintSafeMinus(mTimeStats.mRxTime, txTimeUs);
        }
        else if (mStatus == kSleep)
        {
            mTimeStats.mSleepTime = UintSafeMinus(mTimeStats.mSleepTime, txTimeUs);
            if (aError == kErrorNone)
            {
                mTimeStats.mRxTime += rxAckTimeUs;
                mTimeStats.mSleepTime = UintSafeMinus(mTimeStats.mSleepTime, rxAckTimeUs);
            }
        }
    }
}

void RadioStatistics::RecordRxDone(otError aError)
{
    uint32_t ackTimeUs;

    VerifyOrExit(aError == kErrorNone);

    UpdateTime();
    // Currently we cannot know the actual length of ACK. So assume the ACK is an immediate ACK.
    ackTimeUs = (Mac::Frame::kImmAckLength + Mac::Frame::kPhyHeaderSize) * Radio::kPhyUsPerByte;
    mTimeStats.mTxTime += ackTimeUs;
    if (mStatus == kReceive)
    {
        mTimeStats.mRxTime = UintSafeMinus(mTimeStats.mRxTime, ackTimeUs);
    }

exit:
    return;
}

const otRadioTimeStats &RadioStatistics::GetStats(void)
{
    UpdateTime();

    return mTimeStats;
}

void RadioStatistics::ResetTime(void)
{
    mTimeStats.mDisabledTime = 0;
    mTimeStats.mSleepTime    = 0;
    mTimeStats.mRxTime       = 0;
    mTimeStats.mTxTime       = 0;
    mLastUpdateTime          = TimerMicro::GetNow();
}

void RadioStatistics::UpdateTime(void)
{
    TimeMicro nowTime     = TimerMicro::GetNow();
    uint32_t  timeElapsed = nowTime - mLastUpdateTime;

    switch (mStatus)
    {
    case kSleep:
        mTimeStats.mSleepTime += timeElapsed;
        break;
    case kReceive:
        mTimeStats.mRxTime += timeElapsed;
        break;
    case kDisabled:
        mTimeStats.mDisabledTime += timeElapsed;
        break;
    }
    mLastUpdateTime = nowTime;
}

#endif // OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)

} // namespace ot
