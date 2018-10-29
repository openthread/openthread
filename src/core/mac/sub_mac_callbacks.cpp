/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file implements the callbacks from `SubMac` layer into `Mac` or `LinkRaw`.
 */

#define WPP_NAME "sub_mac_callbacks.tmh"

#include "sub_mac.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "mac/mac.hpp"

namespace ot {
namespace Mac {

#if OPENTHREAD_FTD || OPENTHREAD_MTD

void SubMac::Callbacks::ReceiveDone(Frame *aFrame, otError aError)
{
    Mac &mac = *static_cast<Mac *>(this);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    LinkRaw &linkRaw = mac.GetInstance().GetLinkRaw();

    if (linkRaw.IsEnabled())
    {
        linkRaw.InvokeReceiveDone(aFrame, aError);
    }
    else
#endif
    {
        mac.HandleReceivedFrame(aFrame, aError);
    }
}

void SubMac::Callbacks::RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel)
{
    static_cast<Mac *>(this)->RecordCcaStatus(aCcaSuccess, aChannel);
}

void SubMac::Callbacks::RecordFrameTransmitStatus(const Frame &aFrame,
                                                  const Frame *aAckFrame,
                                                  otError      aError,
                                                  uint8_t      aRetryCount,
                                                  bool         aWillRetx)
{
    static_cast<Mac *>(this)->RecordFrameTransmitStatus(aFrame, aAckFrame, aError, aRetryCount, aWillRetx);
}

void SubMac::Callbacks::TransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError)
{
    Mac &mac = *static_cast<Mac *>(this);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    LinkRaw &linkRaw = mac.GetInstance().GetLinkRaw();

    if (linkRaw.IsEnabled())
    {
        linkRaw.InvokeTransmitDone(aFrame, aAckFrame, aError);
    }
    else
#endif
    {
        mac.HandleTransmitDone(aFrame, aAckFrame, aError);
    }
}

void SubMac::Callbacks::EnergyScanDone(int8_t aMaxRssi)
{
    Mac &mac = *static_cast<Mac *>(this);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    LinkRaw &linkRaw = mac.GetInstance().GetLinkRaw();

    if (linkRaw.IsEnabled())
    {
        linkRaw.InvokeEnergyScanDone(aMaxRssi);
    }
    else
#endif
    {
        mac.EnergyScanDone(aMaxRssi);
    }
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
void SubMac::Callbacks::FrameUpdated(Frame &aFrame)
{
    /**
     * This function will be called from interrupt context, it should only read/write data passed in
     * via @p aFrame, but should not read/write any state within OpenThread.
     *
     */

    Mac &mac = *static_cast<Mac *>(this);

    if (aFrame.GetSecurityEnabled())
    {
        const ExtAddress &extAddress = mac.GetExtAddress();

        mac.ProcessTransmitAesCcm(aFrame, &extAddress);
    }
}
#endif

#elif OPENTHREAD_RADIO

void SubMac::Callbacks::ReceiveDone(Frame *aFrame, otError aError)
{
    static_cast<LinkRaw *>(this)->InvokeReceiveDone(aFrame, aError);
}

void SubMac::Callbacks::RecordCcaStatus(bool, uint8_t)
{
}

void SubMac::Callbacks::RecordFrameTransmitStatus(const Frame &, const Frame *, otError, uint8_t, bool)
{
}

void SubMac::Callbacks::TransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError)
{
    static_cast<LinkRaw *>(this)->InvokeTransmitDone(aFrame, aAckFrame, aError);
}

void SubMac::Callbacks::EnergyScanDone(int8_t aMaxRssi)
{
    static_cast<LinkRaw *>(this)->InvokeEnergyScanDone(aMaxRssi);
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
void SubMac::Callbacks::FrameUpdated(Frame &)
{
    /**
     * This function will be called from interrupt context, it should only read/write data passed in
     * via @p aFrame, but should not read/write any state within OpenThread.
     *
     */

    // For now this functionality is not supported in Radio Only mode.
}
#endif

#endif // OPENTHREAD_RADIO

} // namespace Mac
} // namespace ot
