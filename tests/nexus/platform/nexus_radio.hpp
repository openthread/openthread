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

#ifndef OT_NEXUS_RADIO_HPP_
#define OT_NEXUS_RADIO_HPP_

#include "instance/instance.hpp"

namespace ot {
namespace Nexus {

struct Radio
{
    static constexpr uint16_t kMaxFrameSize = OT_RADIO_FRAME_MAX_SIZE;

    static constexpr uint16_t kMaxSrcMaatchShort = 80;
    static constexpr uint16_t kMaxSrcMatchExt    = 10;

    static constexpr int8_t kRadioSensetivity = -100;

    using State = otRadioState;

    static constexpr State kStateDisabled = OT_RADIO_STATE_DISABLED;
    static constexpr State kStateSleep    = OT_RADIO_STATE_SLEEP;
    static constexpr State kStateReceive  = OT_RADIO_STATE_RECEIVE;
    static constexpr State kStateTransmit = OT_RADIO_STATE_TRANSMIT;

    struct Frame : public Mac::Frame
    {
        Frame(void);
        explicit Frame(const Frame &aFrame);

        uint8_t mPsduBuffer[kMaxFrameSize];
    };

    bool CanReceiveOnChannel(uint8_t aChannel) const;
    bool Matches(const Mac::Address &aAddress, Mac::PanId aPanId) const;
    bool HasFramePendingFor(const Mac::Address &aAddress) const;

    State                                   mState;
    bool                                    mPromiscuous : 1;
    bool                                    mSrcMatchEnabled : 1;
    uint8_t                                 mChannel;
    Mac::PanId                              mPanId;
    Mac::ShortAddress                       mShortAddress;
    Mac::ExtAddress                         mExtAddress;
    Frame                                   mTxFrame;
    Array<uint16_t, kMaxSrcMaatchShort>     mSrcMatchShortEntries;
    Array<Mac::ExtAddress, kMaxSrcMatchExt> mSrcMatchExtEntries;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_RADIO_HPP_
