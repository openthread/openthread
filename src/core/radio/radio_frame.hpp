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
 *   This file includes definitions for a radio frame.
 */

#ifndef OT_CORE_RADIO_RADIO_FRAME_HPP_
#define OT_CORE_RADIO_RADIO_FRAME_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "common/as_core_type.hpp"
#include "mac/mac_types.hpp"
#include "radio/radio_types.hpp"

namespace ot {
namespace Radio {

/**
 * @addtogroup core-radio
 *
 * @{
 */

/**
 * Represents a radio frame.
 */
class Frame : public otRadioFrame
{
public:
    /**
     * Indicates whether the frame is empty (no payload).
     *
     * @retval TRUE   The frame is empty (no PSDU payload).
     * @retval FALSE  The frame is not empty.
     */
    bool IsEmpty(void) const { return (mLength == 0); }

    /**
     * Returns the frame length (PSDU length).
     *
     * @returns The frame length.
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Sets the frame length.
     *
     * @param[in]  aLength  The frame length.
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

    /**
     * Returns the channel used for transmission or reception.
     *
     * @returns The channel used for transmission or reception.
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * Returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     */
    uint8_t *GetPsdu(void) { return mPsdu; }

    /**
     * Returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     */
    const uint8_t *GetPsdu(void) const { return mPsdu; }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    /**
     * Gets the radio link type of the frame.
     *
     * @returns Frame's radio link type.
     */
    Type GetRadioType(void) const { return static_cast<Type>(mRadioType); }

    /**
     * Sets the radio link type of the frame.
     *
     * @param[in] aRadioType  A radio link type.
     */
    void SetRadioType(Type aRadioType) { mRadioType = static_cast<uint8_t>(aRadioType); }
#endif

    /**
     * Returns the maximum transmission unit size (MTU).
     *
     * @returns The maximum transmission unit (MTU).
     */
    uint16_t GetMtu(void) const
#if !OPENTHREAD_CONFIG_MULTI_RADIO && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        return k154MtuSize;
    }
#else
        ;
#endif

    /**
     * Returns the FCS size.
     *
     * @returns The FCS size.
     */
    uint8_t GetFcsSize(void) const
#if !OPENTHREAD_CONFIG_MULTI_RADIO && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        return k154FcsSize;
    }
#else
        ;
#endif

protected:
    static constexpr uint16_t k154MtuSize = OT_RADIO_FRAME_MAX_SIZE;
    static constexpr uint8_t  k154FcsSize = sizeof(uint16_t);
};

/**
 * Defines a CRTP base class providing property accessors for a received radio frame.
 *
 * @tparam RxFrameType  The `RxFrame` subclass type.
 */
template <typename RxFrameType> class RxFrameProperties
{
public:
    /**
     * Returns the RSSI in dBm used for reception.
     *
     * @returns The RSSI in dBm used for reception.
     */
    int8_t GetRssi(void) const { return AsFrame().mInfo.mRxInfo.mRssi; }

    /**
     * Sets the RSSI in dBm used for reception.
     *
     * @param[in]  aRssi  The RSSI in dBm used for reception.
     */
    void SetRssi(int8_t aRssi) { AsFrame().mInfo.mRxInfo.mRssi = aRssi; }

    /**
     * Returns the receive Link Quality Indicator.
     *
     * @returns The receive Link Quality Indicator.
     */
    uint8_t GetLqi(void) const { return AsFrame().mInfo.mRxInfo.mLqi; }

    /**
     * Sets the receive Link Quality Indicator.
     *
     * @param[in]  aLqi  The receive Link Quality Indicator.
     */
    void SetLqi(uint8_t aLqi) { AsFrame().mInfo.mRxInfo.mLqi = aLqi; }

    /**
     * Indicates whether or not the received frame is acknowledged with frame pending set.
     *
     * @retval TRUE   This frame is acknowledged with frame pending set.
     * @retval FALSE  This frame is acknowledged with frame pending not set.
     */
    bool IsAckedWithFramePending(void) const { return AsFrame().mInfo.mRxInfo.mAckedWithFramePending; }

    /**
     * Returns the timestamp when the frame was received.
     *
     * The value SHALL be the time of the local radio clock in
     * microseconds when the end of the SFD (or equivalently: the start
     * of the first symbol of the PHR) was present at the local antenna,
     * see the definition of a "symbol boundary" in IEEE 802.15.4-2020,
     * section 6.5.2 or equivalently the RMARKER definition in section
     * 6.9.1 (albeit both unrelated to OT).
     *
     * The time is relative to the local radio clock as defined by
     * `Radio::GetNow()`.
     *
     * @returns The timestamp in microseconds.
     */
    const Time64 &GetTimestamp(void) const { return AsFrame().mInfo.mRxInfo.mTimestamp; }

private:
    Frame       &AsFrame(void) { return *static_cast<RxFrameType *>(this); }
    const Frame &AsFrame(void) const { return *static_cast<const RxFrameType *>(this); }
};

/**
 * Defines a CRTP base class providing property accessors for a transmitted radio frame.
 *
 * @tparam TxFrameType  The `TxFrame` subclass type.
 */
template <typename TxFrameType> class TxFrameProperties
{
public:
    /**
     * Sets the channel on which to send the frame.
     *
     * It also sets the `RxChannelAfterTxDone` to the same channel.
     *
     * @param[in]  aChannel  The channel used for transmission.
     */
    void SetChannel(uint8_t aChannel)
    {
        AsFrame().mChannel = aChannel;
        SetRxChannelAfterTxDone(aChannel);
    }

    /**
     * Sets TX power to send the frame.
     *
     * @param[in]  aTxPower  The tx power used for transmission.
     */
    void SetTxPower(int8_t aTxPower) { AsFrame().mInfo.mTxInfo.mTxPower = aTxPower; }

    /**
     * Gets the RX channel after frame TX is done.
     *
     * @returns The RX channel after frame TX is done.
     */
    uint8_t GetRxChannelAfterTxDone(void) const { return AsFrame().mInfo.mTxInfo.mRxChannelAfterTxDone; }

    /**
     * Sets the RX channel after frame TX is done.
     *
     * @param[in] aChannel   The RX channel after frame TX is done.
     */
    void SetRxChannelAfterTxDone(uint8_t aChannel) { AsFrame().mInfo.mTxInfo.mRxChannelAfterTxDone = aChannel; }

    /**
     * Returns the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access
     *          failure.
     */
    uint8_t GetMaxCsmaBackoffs(void) const { return AsFrame().mInfo.mTxInfo.mMaxCsmaBackoffs; }

    /**
     * Sets the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxCsmaBackoffs  The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring
     *                               a channel access failure.
     */
    void SetMaxCsmaBackoffs(uint8_t aMaxCsmaBackoffs) { AsFrame().mInfo.mTxInfo.mMaxCsmaBackoffs = aMaxCsmaBackoffs; }

    /**
     * Returns the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of retries allowed after a transmission failure.
     */
    uint8_t GetMaxFrameRetries(void) const { return AsFrame().mInfo.mTxInfo.mMaxFrameRetries; }

    /**
     * Sets the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxFrameRetries  The maximum number of retries allowed after a transmission failure.
     */
    void SetMaxFrameRetries(uint8_t aMaxFrameRetries) { AsFrame().mInfo.mTxInfo.mMaxFrameRetries = aMaxFrameRetries; }

    /**
     * Indicates whether or not the frame is a retransmission.
     *
     * @retval TRUE   Frame is a retransmission
     * @retval FALSE  This is a new frame and not a retransmission of an earlier frame.
     */
    bool IsARetransmission(void) const { return AsFrame().mInfo.mTxInfo.mIsARetx; }

    /**
     * Sets the retransmission flag attribute.
     *
     * @param[in]  aIsARetx  TRUE if frame is a retransmission of an earlier frame, FALSE otherwise.
     */
    void SetIsARetransmission(bool aIsARetx) { AsFrame().mInfo.mTxInfo.mIsARetx = aIsARetx; }

    /**
     * Indicates whether or not CSMA-CA is enabled.
     *
     * @retval TRUE   CSMA-CA is enabled.
     * @retval FALSE  CSMA-CA is not enabled is not enabled.
     */
    bool IsCsmaCaEnabled(void) const { return AsFrame().mInfo.mTxInfo.mCsmaCaEnabled; }

    /**
     * Sets the CSMA-CA enabled attribute.
     *
     * @param[in]  aCsmaCaEnabled  TRUE if CSMA-CA must be enabled for this packet, FALSE otherwise.
     */
    void SetCsmaCaEnabled(bool aCsmaCaEnabled) { AsFrame().mInfo.mTxInfo.mCsmaCaEnabled = aCsmaCaEnabled; }

    /**
     * Returns the key used for frame encryption and authentication (AES CCM).
     *
     * @returns The key.
     */
    const Mac::KeyMaterial &GetAesKey(void) const { return AsCoreType(AsFrame().mInfo.mTxInfo.mAesKey); }

    /**
     * Sets the key used for frame encryption and authentication (AES CCM).
     *
     * @param[in]  aAesKey  The key.
     */
    void SetAesKey(const Mac::KeyMaterial &aAesKey) { AsFrame().mInfo.mTxInfo.mAesKey = &aAesKey; }

    /**
     * Indicates whether or not the frame has security processed.
     *
     * @retval TRUE   The frame already has security processed.
     * @retval FALSE  The frame does not have security processed.
     */
    bool IsSecurityProcessed(void) const { return AsFrame().mInfo.mTxInfo.mIsSecurityProcessed; }

    /**
     * Sets the security processed flag attribute.
     *
     * @param[in]  aIsSecurityProcessed  TRUE if the frame already has security processed.
     */
    void SetIsSecurityProcessed(bool aIsSecurityProcessed)
    {
        AsFrame().mInfo.mTxInfo.mIsSecurityProcessed = aIsSecurityProcessed;
    }

    /**
     * Indicates whether or not the frame contains the CSL IE.
     *
     * @retval TRUE   The frame contains the CSL IE.
     * @retval FALSE  The frame does not contain the CSL IE.
     */
    bool IsCslIePresent(void) const { return AsFrame().mInfo.mTxInfo.mCslPresent; }

    /**
     * Sets the CSL IE present flag.
     *
     * @param[in]  aCslPresent  TRUE if the frame contains the CSL IE.
     */
    void SetCslIePresent(bool aCslPresent) { AsFrame().mInfo.mTxInfo.mCslPresent = aCslPresent; }

    /**
     * Indicates whether or not the frame header is updated.
     *
     * @retval TRUE   The frame already has the header updated.
     * @retval FALSE  The frame does not have the header updated.
     */
    bool IsHeaderUpdated(void) const { return AsFrame().mInfo.mTxInfo.mIsHeaderUpdated; }

    /**
     * Sets the header updated flag attribute.
     *
     * @param[in]  aIsHeaderUpdated  TRUE if the frame header is updated.
     */
    void SetIsHeaderUpdated(bool aIsHeaderUpdated) { AsFrame().mInfo.mTxInfo.mIsHeaderUpdated = aIsHeaderUpdated; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Sets the Time IE offset.
     *
     * @param[in]  aOffset  The Time IE offset, 0 means no Time IE.
     */
    void SetTimeIeOffset(uint8_t aOffset) { AsFrame().mInfo.mTxInfo.mIeInfo->mTimeIeOffset = aOffset; }

    /**
     * Gets the Time IE offset.
     *
     * @returns The Time IE offset, 0 means no Time IE.
     */
    uint8_t GetTimeIeOffset(void) const { return AsFrame().mInfo.mTxInfo.mIeInfo->mTimeIeOffset; }

    /**
     * Sets the offset to network time.
     *
     * @param[in]  aNetworkTimeOffset  The offset to network time.
     */
    void SetNetworkTimeOffset(int64_t aNetworkTimeOffset)
    {
        AsFrame().mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset = aNetworkTimeOffset;
    }

    /**
     * Sets the time sync sequence.
     *
     * @param[in]  aTimeSyncSeq  The time sync sequence.
     */
    void SetTimeSyncSeq(uint8_t aTimeSyncSeq) { AsFrame().mInfo.mTxInfo.mIeInfo->mTimeSyncSeq = aTimeSyncSeq; }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    /**
     * Gets the TX delay field for the frame.
     *
     * @returns The delay time for the TX frame in microseconds.
     */
    uint32_t GetTxDelay(void) const { return AsFrame().mInfo.mTxInfo.mTxDelay; }

    /**
     * Set TX delay field for the frame.
     *
     * @param[in]    aTxDelay    The delay time for the TX frame.
     */
    void SetTxDelay(uint32_t aTxDelay) { AsFrame().mInfo.mTxInfo.mTxDelay = aTxDelay; }

    /**
     * Gets the TX delay base time field for the frame.
     *
     * @returns The delay base time for the TX frame as a `Time32`.
     */
    Time32 GetTxDelayBaseTime(void) const { return AsFrame().mInfo.mTxInfo.mTxDelayBaseTime; }

    /**
     * Set TX delay base time field for the frame.
     *
     * @param[in]    aTxDelayBaseTime    The delay base time for the TX frame.
     */
    void SetTxDelayBaseTime(Time32 aTxDelayBaseTime) { AsFrame().mInfo.mTxInfo.mTxDelayBaseTime = aTxDelayBaseTime; }
#endif

private:
    Frame       &AsFrame(void) { return *static_cast<TxFrameType *>(this); }
    const Frame &AsFrame(void) const { return *static_cast<const TxFrameType *>(this); }
};

/**
 * @}
 */

} // namespace Radio
} // namespace ot

#endif // OT_CORE_RADIO_RADIO_FRAME_HPP_
