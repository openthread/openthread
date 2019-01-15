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
#include "mac/mac_frame.hpp"
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
     * @retval OT_ERROR_NONE    Successfully set the Joiner UDP Port.
     *
     */
    otError SetJoinerUdpPort(uint16_t aJoinerUdpPort);

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
                                            otError              result);
    void HandleJoinerEntrustResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, otError result);

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
     * Default constructor for the object.
     *
     */
    DelayedJoinEntHeader(void) { memset(this, 0, sizeof(*this)); }

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aSendTime     Time when the message shall be sent.
     * @param[in]  aDestination  IPv6 address of the message destination.
     *
     */
    DelayedJoinEntHeader(uint32_t aSendTime, Ip6::MessageInfo &aMessageInfo, const uint8_t *aKek)
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
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(Message &aMessage)
    {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    }

    /**
     * This method removes delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE  Successfully removed the header.
     *
     */
    static otError RemoveFrom(Message &aMessage)
    {
        return aMessage.SetLength(aMessage.GetLength() - sizeof(DelayedJoinEntHeader));
    }

    /**
     * This method returns a time when the message shall be sent.
     *
     * @returns  A time when the message shall be sent.
     *
     */
    uint32_t GetSendTime(void) const { return mSendTime; }

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

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) > 0); }

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) < 0); }

private:
    Ip6::MessageInfo mMessageInfo;                    ///< Message info of the message to send.
    uint32_t         mSendTime;                       ///< Time when the message shall be sent.
    uint8_t          mKek[KeyManager::kMaxKeyLength]; ///< KEK used by MAC layer to encode this message.
};

} // namespace MeshCoP
} // namespace ot

#endif // JOINER_ROUTER_HPP_
