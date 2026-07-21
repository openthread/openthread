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
namespace Radio {

const uint8_t kSupportedChannelPages[kNumChannelPages] = {
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

uint32_t ChannelMaskForPage(uint8_t aChannelPage)
{
    uint32_t mask = 0;

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    if (aChannelPage == kChannelPage0)
    {
        mask = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    if (aChannelPage == kChannelPage2)
    {
        mask = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK;
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
    if (aChannelPage == OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE)
    {
        mask = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MASK;
    }
#endif
    return mask;
}

bool IsCslChannelValid(uint8_t aCslChannel)
{
    return ((aCslChannel == 0) ||
            ((kChannelMin == aCslChannel) || ((kChannelMin < aCslChannel) && (aCslChannel <= kChannelMax))));
}

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_RADIO
void Radio::Init(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    Mac::ExtAddress allZeroExtAddress;
    Mac::KeyTrio    emptyKeyTrio;

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
    allZeroExtAddress.Clear();
    SetExtendedAddress(allZeroExtAddress);
    SetShortAddress(Mac::kShortAddrInvalid);

    emptyKeyTrio.Clear();
    SetMode1MacKeys(emptyKeyTrio);
    SetMacFrameCounter(0);

    SetPromiscuous(false);
    SetRxOnWhenIdle(true);
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
}
#endif // OPENTHREAD_RADIO

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void Radio::SetExtendedAddress(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress address;

    address.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    otPlatRadioSetExtendedAddress(GetInstancePtr(), &address);

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitExtendedAddress(address);
#endif
}

void Radio::SetShortAddress(Mac::ShortAddress aShortAddress)
{
    otPlatRadioSetShortAddress(GetInstancePtr(), aShortAddress);

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitShortAddress(aShortAddress);
#endif
}

Error Radio::AddSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress address;

    address.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    return otPlatRadioAddSrcMatchExtEntry(GetInstancePtr(), &address);
}

Error Radio::ClearSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress address;

    address.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    return otPlatRadioClearSrcMatchExtEntry(GetInstancePtr(), &address);
}

Error Radio::Transmit(Mac::TxFrame &aFrame)
{
#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitTransmit(aFrame);
#endif

    return otPlatRadioTransmit(GetInstancePtr(), &aFrame);
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

uint32_t Radio::CalculateBusTransferTime(uint16_t aFrameSize) const
{
    uint32_t busSpeed     = GetBusSpeed();
    uint32_t transferTime = 0;

    if (busSpeed != 0)
    {
        transferTime = DivideAndRoundUp<uint32_t>(aFrameSize * kBitsPerByte * Time::kOneSecondInUsec, busSpeed);
    }

    transferTime += GetBusLatency();

    return transferTime;
}

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
inline uint64_t UintSafeMinus(uint64_t aLhs, uint64_t aRhs) { return aLhs > aRhs ? (aLhs - aRhs) : 0; }

Statistics::Statistics(void)
    : mStatus(kDisabled)
{
    ResetTime();
}

void Statistics::RecordStateChange(Status aStatus)
{
    UpdateTime();
    mStatus = aStatus;
}

void Statistics::HandleReceiveAt(uint32_t aDurationUs)
{
    // The actual rx time of ReceiveAt cannot be obtained from software level. This is a workaround.
    if (mStatus == kSleep)
    {
        mTimeStats.mRxTime += aDurationUs;
    }
}

void Statistics::RecordTxDone(Error aError, uint16_t aPsduLength)
{
    if (aError == kErrorNone || aError == kErrorNoAck)
    {
        uint32_t txTimeUs    = (aPsduLength + kPhyHeaderSize) * kSymbolsPerOctet * kSymbolTime;
        uint32_t rxAckTimeUs = (Mac::Frame::GetImmAckLength() + kPhyHeaderSize) * kPhyUsPerByte;

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

void Statistics::RecordRxDone(Error aError)
{
    uint32_t ackTimeUs;

    VerifyOrExit(aError == kErrorNone);

    UpdateTime();
    // Currently we cannot know the actual length of ACK. So assume the ACK is an immediate ACK.
    ackTimeUs = (Mac::Frame::GetImmAckLength() + kPhyHeaderSize) * kPhyUsPerByte;
    mTimeStats.mTxTime += ackTimeUs;
    if (mStatus == kReceive)
    {
        mTimeStats.mRxTime = UintSafeMinus(mTimeStats.mRxTime, ackTimeUs);
    }

exit:
    return;
}

const Statistics::TimeStats &Statistics::GetStats(void)
{
    UpdateTime();

    return mTimeStats;
}

void Statistics::ResetTime(void)
{
    ClearAllBytes(mTimeStats);
    mLastUpdateTime = TimerMicro::GetNow();
}

void Statistics::UpdateTime(void)
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

} // namespace Radio
} // namespace ot
