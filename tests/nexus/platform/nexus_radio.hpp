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

#ifndef OT_NEXUS_PLATFORM_NEXUS_RADIO_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_RADIO_HPP_

#include "mac_frame.h"
#include "instance/instance.hpp"

namespace ot {
namespace Nexus {

struct Radio
{
    static constexpr uint16_t kMaxFrameSize = OT_RADIO_FRAME_MAX_SIZE;

    static constexpr uint16_t kMaxSrcMatchShort = 80;
    static constexpr uint16_t kMaxSrcMatchExt   = 10;

    static constexpr int8_t kRadioSensitivity = -100;

    static constexpr uint8_t kEnhAckProbingDataMaxLen = 2;
    static constexpr uint8_t kLinkMetricsMax          = 255;
    static constexpr uint8_t kLinkMetricsScale        = 130;
    static constexpr int16_t kRssiOffset              = 130;

    using State = otRadioState;

    static constexpr State kStateDisabled = OT_RADIO_STATE_DISABLED;
    static constexpr State kStateSleep    = OT_RADIO_STATE_SLEEP;
    static constexpr State kStateReceive  = OT_RADIO_STATE_RECEIVE;
    static constexpr State kStateTransmit = OT_RADIO_STATE_TRANSMIT;

    struct Frame : public Mac::TxFrame
    {
        Frame(void);
        explicit Frame(const Frame &aFrame);

        /**
         * Updates the frame with the proper IEEE 802.15.4 FCS.
         */
        void UpdateFcs(void);

        uint8_t mPsduBuffer[kMaxFrameSize];
    };

    struct LinkMetricsInfo
    {
        bool Matches(Mac::ShortAddress aShortAddress) const { return mShortAddress == aShortAddress; }
        bool Matches(const Mac::ExtAddress &aExtAddress) const { return mExtAddress == aExtAddress; }

        Mac::ShortAddress mShortAddress;
        Mac::ExtAddress   mExtAddress;
        otLinkMetrics     mMetrics;
    };

    Radio(void);
    void Reset(void);
    bool CanReceiveOnChannel(uint8_t aChannel) const;
    bool Matches(const Mac::Address &aAddress, Mac::PanId aPanId) const;
    bool HasFramePendingFor(const Mac::Address &aAddress) const;

    Error   ConfigureEnhAckProbing(Mac::ShortAddress      aShortAddress,
                                   const Mac::ExtAddress *aExtAddress,
                                   otLinkMetrics          aMetrics);
    uint8_t GenerateEnhAckProbingData(const Mac::Address &aAddress, uint8_t aLqi, int8_t aRssi, uint8_t *aData) const;

    State                                   mState;
    bool                                    mPromiscuous : 1;
    bool                                    mSrcMatchEnabled : 1;
    bool                                    mMacFrameCounterReset : 1;
    uint8_t                                 mChannel;
    Mac::PanId                              mPanId;
    Mac::ShortAddress                       mShortAddress;
    Mac::ExtAddress                         mExtAddress;
    Frame                                   mTxFrame;
    otRadioIeInfo                           mTxIeInfo;
    otRadioContext                          mRadioContext;
    Array<uint16_t, kMaxSrcMatchShort>      mSrcMatchShortEntries;
    Array<Mac::ExtAddress, kMaxSrcMatchExt> mSrcMatchExtEntries;

    Array<LinkMetricsInfo, OPENTHREAD_CONFIG_MLE_LINK_METRICS_MAX_SERIES_SUPPORTED> mLinkMetricsEntries;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_RADIO_HPP_
