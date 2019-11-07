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

/**
 * @file
 *  This file includes definitions for the Joiner Router role.
 */

#ifndef JOINER_ROUTER_HPP_
#define JOINER_ROUTER_HPP_

#include "openthread-core-config.h"

#include "coap/coap.hpp"
#include "coap/coap_message.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/udp6.hpp"
#include "thread/key_manager.hpp"

namespace ot {

namespace MeshCoP {

class JoinerRouter : public InstanceLocator
{
public:
    /**
     * This constructor initializes the Joiner Router object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit JoinerRouter(Instance &aInstance);

    /**
     * This method returns the Joiner UDP Port.
     *
     * @returns The Joiner UDP Port number .
     *
     */
    uint16_t GetJoinerUdpPort(void);

    /**
     * This method sets the Joiner UDP Port.
     *
     * @param[in]  The Joiner UDP Port number.
     *
     */
    void SetJoinerUdpPort(uint16_t aJoinerUdpPort);

private:
    enum
    {
        kDelayJoinEnt = 50, ///< milliseconds
    };

    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleRelayTransmit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleRelayTransmit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleJoinerEntrustResponse(void *               aContext,
                                            otMessage *          aMessage,
                                            const otMessageInfo *aMessageInfo,
                                            otError              aResult);
    void HandleJoinerEntrustResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    otError DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo, const JoinerRouterKekTlv &aKek);
    void    SendDelayedJoinerEntrust(void);
    otError SendJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Ip6::UdpSocket mSocket;
    Coap::Resource mRelayTransmit;

    TimerMilli   mTimer;
    MessageQueue mDelayedJoinEnts;

    Notifier::Callback mNotifierCallback;

    uint16_t mJoinerUdpPort;

    bool mIsJoinerPortConfigured : 1;
    bool mExpectJoinEntRsp : 1;
};

/**
 * This class implements functionality required for delaying JOIN_ENT.ntf messages.
 *
 */
class DelayedJoinEntHeader
{
public:
    /**
     * This method initializes the object with specific values.
     *
     * @param[in]  aSendTime     Time when the message shall be sent.
     * @param[in]  aMessageInfo  IPv6 address of the message destination.
     * @param[in]  aKek          A pointer to the KEK.
     *
     */
    void Init(TimeMilli aSendTime, Ip6::MessageInfo &aMessageInfo, const uint8_t *aKek)
    {
        mSendTime    = aSendTime;
        mMessageInfo = aMessageInfo;
        memcpy(&mKek, aKek, sizeof(mKek));
    }

    /**
     * This method appends delayed response header to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) { return aMessage.Append(this, sizeof(*this)); }

    /**
     * This method reads delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     */
    void ReadFrom(const Message &aMessage)
    {
        uint16_t length = aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
        assert(length == sizeof(*this));
        OT_UNUSED_VARIABLE(length);
    }

    /**
     * This method removes delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     */
    static void RemoveFrom(Message &aMessage)
    {
        otError error = aMessage.SetLength(aMessage.GetLength() - sizeof(DelayedJoinEntHeader));
        assert(error == OT_ERROR_NONE);
        OT_UNUSED_VARIABLE(error);
    }

    /**
     * This method returns a time when the message shall be sent.
     *
     * @returns  A time when the message shall be sent.
     *
     */
    TimeMilli GetSendTime(void) const { return mSendTime; }

    /**
     * This method returns a destination of the delayed message.
     *
     * @returns  A destination of the delayed message.
     *
     */
    const Ip6::MessageInfo *GetMessageInfo(void) const { return &mMessageInfo; }

    /**
     * This method returns a pointer to the KEK that should be used to send the delayed message.
     *
     * @returns  A pointer to the KEK.
     *
     */
    const uint8_t *GetKek(void) const { return mKek; }

private:
    Ip6::MessageInfo mMessageInfo;                    ///< Message info of the message to send.
    TimeMilli        mSendTime;                       ///< Time when the message shall be sent.
    uint8_t          mKek[KeyManager::kMaxKeyLength]; ///< KEK used by MAC layer to encode this message.
};

} // namespace MeshCoP
} // namespace ot

#endif // JOINER_ROUTER_HPP_
