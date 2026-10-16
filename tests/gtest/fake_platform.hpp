/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef OT_GTEST_FAKE_PLATFORM_HPP_
#define OT_GTEST_FAKE_PLATFORM_HPP_

#include "openthread-core-config.h"

#include <map>
#include <set>
#include <vector>

#include <inttypes.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

bool operator<(const otExtAddress &aLeft, const otExtAddress &aRight);

namespace ot {

class FakePlatform
{
public:
    FakePlatform();
    virtual ~FakePlatform();

    static FakePlatform &CurrentPlatform() { return *sPlatform; }
    static otInstance   *CurrentInstance() { return CurrentPlatform().mInstance; }

    /**
     * Run until something happened or timeout.
     *
     * @param aTimeout The timeout in us.
     *
     * @returns the remaining timeout.
     */
    uint64_t Run(uint64_t aTimeoutInUs = 0);

    void GoInUs(uint64_t aTimeoutInUs = 0);

    void GoInMs(uint32_t aTimeoutInMs = 0) { GoInUs(aTimeoutInMs * OT_US_PER_MS); }

    virtual uint64_t GetNow() const { return mNow; }

    /**
     * Returns the radio clock time (`otPlatRadioGetNow()`).
     *
     * The radio clock is a separate time base from the alarm clock (`otPlatAlarmMicroGetNow()`, i.e., `GetNow()`)
     * on the same chip. They differ by `mRadioClockOffset`.
     */
    virtual otRadioTime64 GetRadioNow() const { return mNow + mRadioClockOffset; }

    /**
     * Sets the offset of the radio clock (`otPlatRadioGetNow()`) from the alarm clock (`otPlatAlarmMicroGetNow()`).
     *
     * @param[in] aOffset  The offset in microseconds.
     */
    void SetRadioClockOffset(uint64_t aOffset) { mRadioClockOffset = aOffset; }

    /**
     * Configures the simulated transmit timing used by `Transmit()` (see `TxRecord` for the timeline).
     *
     * When disabled (default), `Transmit()` only signals `otPlatRadioTxStarted()`.
     *
     * The simulated radio does not support `OT_RADIO_CAPS_TRANSMIT_TIMING`, so a timed frame is sent right away. If
     * the end of its SFD would be later than its target TX time, nothing is sent and `otPlatRadioTxDone()` reports
     * `OT_ERROR_ABORT`.
     *
     * TODO: `otPlatRadioReceive()` is assumed to put the radio on the target channel and make it ready to receive
     * immediately. RX ramp-up from sleep and channel retuning, which also take `mRampUpUs` on real radios, are not
     * modeled yet: `SubMac::kTimedTxLeadTime` does not budget for them, so a timed transmission with CCA would always
     * be late when the radio is not already receiving on its channel.
     */
    struct TxTiming
    {
        bool     mEnabled  = false;         ///< Whether to simulate transmit timing.
        uint32_t mCcaUs    = 128;           ///< CCA duration (in usec), 8 symbols.
        uint32_t mRampUpUs = 40;            ///< RX-to-TX ramp-up time before the SHR (in usec).
        otError  mTxError  = OT_ERROR_NONE; ///< Result reported for every transmission.
    };

    /**
     * Represents a transmission simulated with `TxTiming`.
     *
     * All times are absolute radio clock times (see `otPlatRadioGetNow()` and `GetRadioNow()`). The timeline of a
     * transmission of a frame with PSDU length `L` (in bytes) is:
     *
     *     mCalledAt     mCcaEnd                                 mSfdEnd                         mEnd
     *         |---------|-------------|---------------------------|-----------------------------|
     *            CCA       ramp-up        SHR (preamble + SFD)            PHR + PSDU
     *          mCcaUs     mRampUpUs          kShrUs (160)             kUsPerByte * (L + 1)
     *
     * - Without CSMA-CA, there is no CCA and `mCcaEnd == mCalledAt`.
     * - `otPlatRadioTxStarted()` is signaled when the SHR starts and `otPlatRadioTxDone()` at `mEnd`.
     * - If CCA fails (`mTxError` is `OT_ERROR_CHANNEL_ACCESS_FAILURE`), nothing is sent: `otPlatRadioTxDone()` is
     *   signaled at `mCcaEnd`, and `mSfdEnd` and `mEnd` equal `mCcaEnd`.
     * - If a timed frame cannot be sent on time (`mAborted`), nothing is sent: `otPlatRadioTxDone()` is signaled at
     *   `mCalledAt`, and all times equal `mCalledAt`.
     */
    struct TxRecord
    {
        otRadioTime64 mCalledAt;      ///< Time `Transmit()` was called.
        otRadioTime64 mCcaEnd;        ///< Time CCA ends (`mCalledAt` without CCA).
        otRadioTime64 mSfdEnd;        ///< Time the end of SFD is on the air.
        otRadioTime64 mEnd;           ///< Time the transmission ends.
        otRadioTime64 mTargetTime;    ///< Target TX time (valid when `mIsTimed`).
        uint16_t      mLength;        ///< PSDU length (in bytes).
        uint8_t       mChannel;       ///< Channel.
        bool          mIsTimed;       ///< Whether a target TX time was specified.
        bool          mCsmaCaEnabled; ///< Whether CSMA-CA (CCA) was enabled.
        bool          mAborted;       ///< Whether the timed transmission was aborted since it was too late.
    };

    static constexpr uint32_t kUsPerByte = 32;  ///< 2.4 GHz O-QPSK: 2 symbols of 16 usec per byte.
    static constexpr uint32_t kShrUs     = 160; ///< SHR: 4 preamble bytes and 1 SFD byte.

    /**
     * Returns the airtime of a frame: SHR, PHR, and PSDU, i.e. `16 * (2 * aPsduLength + 12)` usec.
     */
    static uint32_t GetAirtimeUs(uint16_t aPsduLength) { return kUsPerByte * (aPsduLength + 6); }

    /**
     * Converts an alarm clock time (see `GetNow()`) to the radio clock time.
     */
    otRadioTime64 ToRadioTime(uint64_t aTime) const { return aTime + mRadioClockOffset; }

    /**
     * Converts a radio clock time to the alarm clock time (see `GetNow()`).
     */
    uint64_t ToAlarmTime(otRadioTime64 aTime) const { return aTime - mRadioClockOffset; }

    TxTiming              mTxTiming;
    std::vector<TxRecord> mTxRecords;

    virtual void StartMilliAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMilliAlarm();
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    virtual void StartMicroAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMicroAlarm();
#endif

    uint8_t               GetReceiveChannel(void) const { return mChannel; }
    virtual otRadioFrame *GetTransmitBuffer() { return &mTransmitFrame; }
    virtual otError       Transmit(otRadioFrame *aFrame);
    virtual otError       ReceiveAt(uint8_t aChannel, uint32_t aStart, uint32_t aDuration)
    {
        mReceiveAtChannel = aChannel;
        mReceiveAtStart   = mNow + aStart;
        mReceiveAtEnd     = mReceiveAtStart + aDuration;

        return OT_ERROR_NONE;
    }

    virtual otError Receive(uint8_t aChannel)
    {
        mChannel    = aChannel;
        mIsSleeping = false;
        return OT_ERROR_NONE;
    }

    virtual otError Sleep(void)
    {
        mIsSleeping = true;
        return OT_ERROR_NONE;
    }

    bool IsSleeping(void) const { return mIsSleeping; }

    virtual otError SettingsGet(uint16_t aKey, uint16_t aIndex, uint8_t *aValue, uint16_t *aValueLength) const;
    virtual otError SettingsSet(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
    virtual otError SettingsAdd(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
    virtual otError SettingsDelete(uint16_t aKey, int aIndex);
    virtual void    SettingsWipe();

    virtual void     FlashInit();
    virtual void     FlashErase(uint8_t aSwapIndex);
    virtual void     FlashRead(uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize) const;
    virtual void     FlashWrite(uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize);
    virtual uint32_t FlashGetSwapSize() const { return kFlashSwapSize; }

    virtual uint64_t GetEui64() const { return 0; }

    virtual void SrcMatchEnable(bool aEnabled) { mSrcMatchEnabled = aEnabled; }
    virtual bool SrcMatchIsEnabled() const { return mSrcMatchEnabled; }
    virtual void SrcMatchAddShortEntry(uint16_t aShortAddr) { mSrcMatchShortAddrs.insert(aShortAddr); }
    virtual void SrcMatchClearShortEntry(uint16_t aShortAddr) { mSrcMatchShortAddrs.erase(aShortAddr); }
    virtual bool SrcMatchHasShortEntry(uint16_t aShortAddr) const { return mSrcMatchShortAddrs.count(aShortAddr) != 0; }
    virtual void SrcMatchAddExtEntry(const otExtAddress &aExtAddr) { mSrcMatchExtAddrs.insert(aExtAddr); }
    virtual void SrcMatchClearExtEntry(const otExtAddress &aExtAddr) { mSrcMatchExtAddrs.erase(aExtAddr); }
    virtual bool SrcMatchHasExtEntry(const otExtAddress &aExtAddr) const
    {
        return mSrcMatchExtAddrs.count(aExtAddr) != 0;
    }
    virtual void   SrcMatchClearShortEntries(void) { mSrcMatchShortAddrs.clear(); }
    virtual size_t SrcMatchCountShortEntries(void) const { return mSrcMatchShortAddrs.size(); }
    virtual void   SrcMatchClearExtEntries(void) { mSrcMatchExtAddrs.clear(); }
    virtual size_t SrcMatchCountExtEntries(void) const { return mSrcMatchExtAddrs.size(); }

    // Records what the RCP was told, so a test can assert which channel got
    // which power rather than only that something was sent.
    static constexpr int8_t kNoMaxTxPower = INT8_MIN;

    virtual otError ChannelMaxTxPowerSet(uint8_t aChannel, int8_t aPower)
    {
        otError error = mChannelMaxTxPowerError;

        if (error == OT_ERROR_NONE)
        {
            mChannelMaxTxPower[aChannel] = aPower;
        }

        return error;
    }

    // Makes the fake RCP refuse the property, the way a firmware without
    // `SPINEL_PROP_PHY_CHAN_MAX_POWER` does.
    virtual void   ChannelMaxTxPowerFailWith(otError aError) { mChannelMaxTxPowerError = aError; }
    virtual int8_t ChannelMaxTxPowerGet(uint8_t aChannel) const
    {
        auto it = mChannelMaxTxPower.find(aChannel);
        return (it == mChannelMaxTxPower.end()) ? kNoMaxTxPower : it->second;
    }
    virtual void   ChannelMaxTxPowerClear(void) { mChannelMaxTxPower.clear(); }
    virtual size_t ChannelMaxTxPowerCount(void) const { return mChannelMaxTxPower.size(); }

protected:
    void ProcessSchedules(uint64_t &aTimeout);

    static constexpr uint64_t kAlarmStop = 0xffffffffffffffffUL;

    static constexpr uint32_t kFlashSwapSize = 2048;
    static constexpr uint32_t kFlashSwapNum  = 2;

    static FakePlatform *sPlatform;

    otInstance *mInstance = nullptr;

    uint64_t mNow = 0;

    uint64_t mRadioClockOffset = 0; // Offset of the radio clock from the alarm clock.
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    uint64_t mMicroAlarmStart = kAlarmStop;
#endif
    uint64_t mMilliAlarmStart = kAlarmStop;

    uint64_t mReceiveAtStart = kAlarmStop;
    uint64_t mReceiveAtEnd   = kAlarmStop;

    uint64_t      mTxStartAt   = kAlarmStop;
    uint64_t      mTxDoneAt    = kAlarmStop;
    otRadioFrame *mTxFrame     = nullptr;
    otError       mTxDoneError = OT_ERROR_NONE;

    template <uint64_t FakePlatform::*T> void HandleSchedule();

    otRadioFrame mTransmitFrame{};
    uint8_t      mTransmitBuffer[OT_RADIO_FRAME_MAX_SIZE]{};
    uint8_t      mChannel          = 0;
    bool         mIsSleeping       = true;
    uint8_t      mReceiveAtChannel = 0;

    uint8_t mFlash[kFlashSwapSize * kFlashSwapNum];

    std::map<uint32_t, std::vector<std::vector<uint8_t>>> mSettings;

    bool                      mSrcMatchEnabled = false;
    std::set<uint16_t>        mSrcMatchShortAddrs;
    std::set<otExtAddress>    mSrcMatchExtAddrs;
    std::map<uint8_t, int8_t> mChannelMaxTxPower;
    otError                   mChannelMaxTxPowerError = OT_ERROR_NONE;
};

template <> inline void FakePlatform::HandleSchedule<&FakePlatform::mMilliAlarmStart>()
{
    otPlatAlarmMilliFired(mInstance);
}

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
template <> inline void FakePlatform::HandleSchedule<&FakePlatform::mMicroAlarmStart>()
{
    otPlatAlarmMicroFired(mInstance);
}
#endif

} // namespace ot

#endif // OT_GTEST_FAKE_PLATFORM_HPP_
