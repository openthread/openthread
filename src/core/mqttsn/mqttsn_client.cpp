/*
 *  Copyright (c) 2018, Vit Holasek
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

#include <string.h>
#include <stddef.h>
#include "mqttsn_client.hpp"
#include "mqttsn_serializer.hpp"
#include "common/timer.hpp"

/**
 * @file
 *   This file includes implementation of MQTT-SN protocol v1.2 client.
 *
 */

/**
 * Maximal supported MQTT-SN message size in bytes.
 *
 */
#define MAX_PACKET_SIZE 255
/**
 * Minimal MQTT-SN message size in bytes.
 *
 */
#define MQTTSN_MIN_PACKET_LENGTH 2

namespace ot {

namespace Mqttsn {

template <typename CallbackType>
MessageMetadata<CallbackType>::MessageMetadata()
{
    ;
}

template <typename CallbackType>
MessageMetadata<CallbackType>::MessageMetadata(const Ip6::Address &aDestinationAddress, uint16_t aDestinationPort, uint16_t aMessageId, uint32_t aTimestamp, uint32_t aRetransmissionTimeout, uint8_t aRetransmissionCount, CallbackType aCallback, void* aContext)
    : mDestinationAddress(aDestinationAddress)
    , mDestinationPort(aDestinationPort)
    , mMessageId(aMessageId)
    , mTimestamp(aTimestamp)
    , mRetransmissionTimeout(aRetransmissionTimeout)
    , mRetransmissionCount(aRetransmissionCount)
    , mCallback(aCallback)
    , mContext(aContext)
{
    ;
}

template <typename CallbackType>
otError MessageMetadata<CallbackType>::AppendTo(Message &aMessage) const
{
    return aMessage.Append(this, sizeof(*this));
}

template <typename CallbackType>
otError MessageMetadata<CallbackType>::UpdateIn(Message &aMessage) const
{
    aMessage.Write(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    return OT_ERROR_NONE;
}

template <typename CallbackType>
uint16_t MessageMetadata<CallbackType>::ReadFrom(Message &aMessage)
{
    return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
}

template <typename CallbackType>
Message* MessageMetadata<CallbackType>::GetRawMessage(const Message &aMessage) const
{
    return aMessage.Clone(aMessage.GetLength() - GetLength());
}

template <typename CallbackType>
uint16_t MessageMetadata<CallbackType>::GetLength() const
{
    return sizeof(*this);
}

template <typename CallbackType>
WaitingMessagesQueue<CallbackType>::WaitingMessagesQueue(TimeoutCallbackFunc aTimeoutCallback, void* aTimeoutContext, RetransmissionFunc aRetransmissionFunc, void* aRetransmissionContext)
    : mQueue()
    , mTimeoutCallback(aTimeoutCallback)
    , mTimeoutContext(aTimeoutContext)
    , mRetransmissionFunc(aRetransmissionFunc)
    , mRetransmissionContext(aRetransmissionContext)
{
    ;
}

template <typename CallbackType>
WaitingMessagesQueue<CallbackType>::~WaitingMessagesQueue(void)
{
    ForceTimeout();
}

template <typename CallbackType>
otError WaitingMessagesQueue<CallbackType>::EnqueueCopy(const Message &aMessage, uint16_t aLength, const MessageMetadata<CallbackType> &aMetadata)
{
    otError error = OT_ERROR_NONE;
    Message *messageCopy = NULL;

    VerifyOrExit((messageCopy = aMessage.Clone(aLength)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = aMetadata.AppendTo(*messageCopy));
    SuccessOrExit(error = mQueue.Enqueue(*messageCopy));

exit:
    return error;
}

template <typename CallbackType>
otError WaitingMessagesQueue<CallbackType>::Dequeue(Message &aMessage)
{
    otError error = mQueue.Dequeue(aMessage);
    aMessage.Free();
    return error;
}

template <typename CallbackType>
Message* WaitingMessagesQueue<CallbackType>::Find(uint16_t aMessageId, MessageMetadata<CallbackType> &aMetadata)
{
    Message* message = mQueue.GetHead();
    while (message)
    {
        aMetadata.ReadFrom(*message);
        if (aMessageId == aMetadata.mMessageId)
        {
            return message;
        }
        message = message->GetNext();
    }
    return NULL;
}

template <typename CallbackType>
otError WaitingMessagesQueue<CallbackType>::HandleTimer()
{
    otError error = OT_ERROR_NONE;
    Message* message = mQueue.GetHead();
    Message* retransmissionMessage = NULL;
    while (message)
    {
        Message* current = message;
        message = message->GetNext();
        MessageMetadata<CallbackType> metadata;
        metadata.ReadFrom(*current);
        // check if message timed out
        if (metadata.mTimestamp + metadata.mRetransmissionTimeout <= TimerMilli::GetNow().GetValue())
        {
            if (metadata.mRetransmissionCount > 0)
            {
                // Invoke message retransmission and decrement retransmission counter
                if (mRetransmissionFunc)
                {
                    VerifyOrExit((retransmissionMessage = metadata.GetRawMessage(*current)), error = OT_ERROR_NO_BUFS);
                    mRetransmissionFunc(*retransmissionMessage, metadata.mDestinationAddress, metadata.mDestinationPort, mRetransmissionContext);
                    retransmissionMessage->Free();
                }
                metadata.mRetransmissionCount--;
                metadata.mTimestamp = TimerMilli::GetNow().GetValue();
                // Update message metadata
                metadata.UpdateIn(*current);
            }
            else
            {
                // Invoke timeout callback and dequeue message
                if (mTimeoutCallback)
                {
                    mTimeoutCallback(metadata, mTimeoutContext);
                }
                SuccessOrExit(error = Dequeue(*current));
            }
        }
    }
exit:
    return error;
}

template <typename CallbackType>
void WaitingMessagesQueue<CallbackType>::ForceTimeout()
{
    Message* message = mQueue.GetHead();
    while (message)
    {
        Message* current = message;
        message = message->GetNext();
        MessageMetadata<CallbackType> metadata;
        metadata.ReadFrom(*current);
        if (mTimeoutCallback)
        {
            mTimeoutCallback(metadata, mTimeoutContext);
        }
        Dequeue(*current);
    }
}

template <typename CallbackType>
bool WaitingMessagesQueue<CallbackType>::IsEmpty()
{
    return mQueue.GetHead() == NULL;
}

MqttsnClient::MqttsnClient(Instance& instance)
    : InstanceLocator(instance)
    , mSocket(GetInstance().Get<Ip6::Udp>())
    , mConfig()
    , mMessageId(1)
    , mPingReqTime(0)
    , mDisconnectRequested(false)
    , mSleepRequested(false)
    , mTimeoutRaised(false)
    , mClientState(kStateDisconnected)
    , mIsRunning(false)
    , mProcessTask(instance, &MqttsnClient::HandleProcessTask, this)
    , mSubscribeQueue(HandleSubscribeTimeout, this, HandleSubscribeRetransmission, this)
    , mRegisterQueue(HandleRegisterTimeout, this, HandleMessageRetransmission, this)
    , mUnsubscribeQueue(HandleUnsubscribeTimeout, this, HandleMessageRetransmission, this)
    , mPublishQos1Queue(HandlePublishQos1Timeout, this, HandlePublishRetransmission, this)
    , mPublishQos2PublishQueue(HandlePublishQos2PublishTimeout, this, HandlePublishRetransmission, this)
    , mPublishQos2PubrelQueue(HandlePublishQos2PubrelTimeout, this, HandleMessageRetransmission, this)
    , mPublishQos2PubrecQueue(HandlePublishQos2PubrecTimeout, this, HandleMessageRetransmission, this)
    , mConnectQueue(HandleConnectTimeout, this, HandleMessageRetransmission, this)
    , mDisconnectQueue(HandleDisconnectTimeout, this, HandleMessageRetransmission, this)
    , mPingreqQueue(HandlePingreqTimeout, this, HandleMessageRetransmission, this)
    , mConnectedCallback(NULL)
    , mConnectContext(NULL)
    , mPublishReceivedCallback(NULL)
    , mPublishReceivedContext(NULL)
    , mAdvertiseCallback(NULL)
    , mAdvertiseContext(NULL)
    , mSearchGwCallback(NULL)
    , mSearchGwContext(NULL)
    , mDisconnectedCallback(NULL)
    , mDisconnectedContext(NULL)
    , mRegisterReceivedCallback(NULL)
    , mRegisterReceivedContext(NULL)
{
    ;
}

MqttsnClient::~MqttsnClient()
{
    mSocket.Close();
    OnDisconnected();
}

void MqttsnClient::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    Message &message = *static_cast<Message *>(aMessage);
    const Ip6::MessageInfo &messageInfo = *static_cast<const Ip6::MessageInfo *>(aMessageInfo);

    // Read message content
    uint16_t offset = message.GetOffset();
    uint16_t length = message.GetLength() - message.GetOffset();

    unsigned char data[MAX_PACKET_SIZE];

    if (length > MAX_PACKET_SIZE)
    {
        return;
    }

    if (!data)
    {
        return;
    }
    message.Read(offset, length, data);

    otLogDebgMqttsn("UDP message received:");
    otDumpDebgCore("received", data, length);

    // Determine message type
    MessageType messageType;
    if (MessageBase::DeserializeMessageType(data, length, &messageType))
    {
        return;
    }
    otLogDebgMqttsn("Message type: %d", messageType);

    // Handle received message type
    switch (messageType)
    {
    case kTypeConnack:
        client->ConnackReceived(messageInfo, data, length);
        break;
    case kTypeSuback:
        client->SubackReceived(messageInfo, data, length);
        break;
    case kTypePublish:
        client->PublishReceived(messageInfo, data, length);
        break;
    case kTypeAdvertise:
        client->AdvertiseReceived(messageInfo, data, length);
        break;
    case kTypeGwInfo:
        client->GwinfoReceived(messageInfo, data, length);
        break;
    case kTypeRegack:
        client->RegackReceived(messageInfo, data, length);
        break;
    case kTypeRegister:
        client->RegisterReceived(messageInfo, data, length);
        break;
    case kTypePuback:
        client->PubackReceived(messageInfo, data, length);
        break;
    case kTypePubrec:
        client->PubrecReceived(messageInfo, data, length);
        break;
    case kTypePubrel:
        client->PubrelReceived(messageInfo, data, length);
        break;
    case kTypePubcomp:
        client->PubcompReceived(messageInfo, data, length);
        break;
    case kTypeUnsuback:
        client->UnsubackReceived(messageInfo, data, length);
        break;
    case kTypePingreq:
        client->PingreqReceived(messageInfo, data, length);
        break;
    case kTypePingresp:
        client->PingrespReceived(messageInfo, data, length);
        break;
    case kTypeDisconnect:
        client->DisconnectReceived(messageInfo, data, length);
        break;
    default:
        break;
    }
}

void MqttsnClient::ConnackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }

    ConnackMessage connackMessage;
    if (connackMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }
    MessageMetadata<void*> metadata;

    // Check if any waiting connect message queued
    Message *connectMessage = mConnectQueue.Find(0, metadata);
    if (connectMessage)
    {
        mConnectQueue.Dequeue(*connectMessage);

        mClientState = kStateActive;
        if (mConnectedCallback)
        {
            mConnectedCallback(connackMessage.GetReturnCode(), mConnectContext);
        }
    }
}

void MqttsnClient::SubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client must be in active state
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    SubackMessage subackMessage;
    if (subackMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Find waiting message with corresponding ID
    MessageMetadata<otMqttsnSubscribedHandler> metadata;
    Message* subscribeMessage = mSubscribeQueue.Find(subackMessage.GetMessageId(), metadata);
    if (subscribeMessage)
    {
        // Invoke callback and dequeue message
        if (metadata.mCallback)
        {
            metadata.mCallback(subackMessage.GetReturnCode(), subackMessage.GetTopicId(),
                subackMessage.GetQos(), metadata.mContext);
        }
        mSubscribeQueue.Dequeue(*subscribeMessage);
    }
}

void MqttsnClient::PublishReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client must be in active or awake state to receive published messages
    if (mClientState != kStateActive && mClientState != kStateAwake)
    {
        return;
    }
    // Check source IPv6 address
    if (VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PublishMessage publishMessage;
    if (publishMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Filter duplicate QoS level 2 messages
    if (publishMessage.GetQos() == kQos2)
    {
        MessageMetadata<void*> metadata;
        Message* pubrecMessage = mPublishQos2PubrecQueue.Find(publishMessage.GetMessageId(), metadata);
        if (pubrecMessage)
        {
            return;
        }
    }

    ReturnCode code = kCodeRejectedTopicId;
    if (mPublishReceivedCallback)
    {
        // Invoke callback
        code = mPublishReceivedCallback(publishMessage.GetPayload(), publishMessage.GetPayloadLength(),
            publishMessage.GetTopicIdType(), publishMessage.GetTopicId(), publishMessage.GetShortTopicName(),
            mPublishReceivedContext);
    }

    // Handle QoS
    if (publishMessage.GetQos() == kQos0 || publishMessage.GetQos() == kQosm1)
    {
        // On QoS level 0 or -1 do nothing
    }
    else if (publishMessage.GetQos() == kQos1)
    {
        // On QoS level 1  send PUBACK response
        int32_t packetLength = -1;
        Message* responseMessage = NULL;
        unsigned char buffer[MAX_PACKET_SIZE];
        PubackMessage pubackMessage(code, publishMessage.GetTopicId(), publishMessage.GetMessageId());
        if (pubackMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
        {
            return;
        }
        if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE
            || SendMessage(*responseMessage) != OT_ERROR_NONE)
        {
            return;
        }
    }
    else if (publishMessage.GetQos() == kQos2)
    {
        // On QoS level 2 send PUBREC message and wait for PUBREL
        int32_t packetLength = -1;
        Message* responseMessage = NULL;
        unsigned char buffer[MAX_PACKET_SIZE];
        PubrecMessage pubrecMessage(publishMessage.GetMessageId());
        if (pubrecMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
        {
            return;
        }
        if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE ||
            SendMessage(*responseMessage) != OT_ERROR_NONE)
        {
            return;
        }

        const MessageMetadata<void*> &metadata = MessageMetadata<void*>(mConfig.GetAddress(), mConfig.GetPort(),
            publishMessage.GetMessageId(), TimerMilli::GetNow().GetValue(), mConfig.GetRetransmissionTimeout() * 1000,
            mConfig.GetRetransmissionCount(), NULL, NULL);
        // Add message to waiting queue, message with same messageId will not be processed until PUBREL message received
        if (mPublishQos2PubrecQueue.EnqueueCopy(*responseMessage, responseMessage->GetLength(), metadata) != OT_ERROR_NONE)
        {
            return;
        }
    }
}

void MqttsnClient::AdvertiseReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    AdvertiseMessage advertiseMessage;
    if (advertiseMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }
    if (mAdvertiseCallback)
    {
        mAdvertiseCallback(messageInfo.GetPeerAddr(), advertiseMessage.GetGatewayId(),
            advertiseMessage.GetDuration(), mAdvertiseContext);
    }
}

void MqttsnClient::GwinfoReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    GwInfoMessage gwInfoMessage;
    if (gwInfoMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }
    if (mSearchGwCallback)
    {
        Ip6::Address address = (gwInfoMessage.GetHasAddress()) ? gwInfoMessage.GetAddress()
            : messageInfo.GetPeerAddr();
        mSearchGwCallback(address, gwInfoMessage.GetGatewayId(), mSearchGwContext);
    }
}

void MqttsnClient::RegackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }

    RegackMessage regackMessage;
    if (regackMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }
    // Find waiting message with corresponding ID
    MessageMetadata<RegisterCallbackFunc> metadata;
    Message* registerMessage = mRegisterQueue.Find(regackMessage.GetMessageId(), metadata);
    if (!registerMessage)
    {
        return;
    }
    // Invoke callback and dequeue message
    if (metadata.mCallback)
    {
        metadata.mCallback(regackMessage.GetReturnCode(), regackMessage.GetTopicId(), metadata.mContext);
    }
    mRegisterQueue.Dequeue(*registerMessage);
}

void MqttsnClient::RegisterReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    int32_t packetLength = -1;
    Message* responseMessage = NULL;
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }

    RegisterMessage registerMessage;
    if (registerMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Invoke register callback
    ReturnCode code = kCodeRejectedTopicId;
    if (mRegisterReceivedCallback)
    {
        code = mRegisterReceivedCallback(registerMessage.GetTopicId(), registerMessage.GetTopicName(), mRegisterReceivedContext);
    }

    // Send REGACK response message
    RegackMessage regackMessage(code, registerMessage.GetTopicId(), registerMessage.GetMessageId());
    if (regackMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
    {
        return;
    }
    if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE ||
        SendMessage(*responseMessage) != OT_ERROR_NONE)
    {
        return;
    }
}

void MqttsnClient::PubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address.
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PubackMessage pubackMessage;
    if (pubackMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Process QoS level 1 message
    // Find message waiting for acknowledge
    MessageMetadata<PublishCallbackFunc> metadata;
    Message* publishMessage = mPublishQos1Queue.Find(pubackMessage.GetMessageId(), metadata);
    if (publishMessage)
    {
        // Invoke confirmation callback
        if (metadata.mCallback)
        {
            metadata.mCallback(pubackMessage.GetReturnCode(), metadata.mContext);
        }
        // Dequeue waiting message
        mPublishQos1Queue.Dequeue(*publishMessage);
        return;
    }
    // May be QoS level 2 message error response
    publishMessage = mPublishQos2PublishQueue.Find(pubackMessage.GetMessageId(), metadata);
    if (publishMessage)
    {
        // Invoke confirmation callback
        if (metadata.mCallback)
        {
            metadata.mCallback(pubackMessage.GetReturnCode(), metadata.mContext);
        }
        // Dequeue waiting message
        mPublishQos2PublishQueue.Dequeue(*publishMessage);
        return;
    }

    // May be QoS level 0 message error response - it is not handled
}

void MqttsnClient::PubrecReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    int32_t packetLength = -1;
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PubrecMessage pubrecMessage;
    if (pubrecMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Process QoS level 2 message
    // Find message waiting for receive acknowledge
    MessageMetadata<PublishCallbackFunc> metadata;
    Message* publishMessage = mPublishQos2PublishQueue.Find(pubrecMessage.GetMessageId(), metadata);
    if (!publishMessage)
    {
        return;
    }

    // Send PUBREL message
    PubrelMessage pubrelMessage(metadata.mMessageId);
    if (pubrelMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
    {
        return;
    }
    Message* responseMessage = NULL;
    if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE ||
        SendMessage(*responseMessage) != OT_ERROR_NONE)
    {
        return;
    }

    // Enqueue PUBREL message and wait for PUBCOMP
    const MessageMetadata<PublishCallbackFunc> &pubrelMetadata = MessageMetadata<PublishCallbackFunc>(mConfig.GetAddress(),
        mConfig.GetPort(), metadata.mMessageId, TimerMilli::GetNow().GetValue(), mConfig.GetRetransmissionTimeout() * 1000,
        mConfig.GetRetransmissionCount(), metadata.mCallback, this);
    if (mPublishQos2PubrelQueue.EnqueueCopy(*responseMessage, responseMessage->GetLength(), pubrelMetadata) != OT_ERROR_NONE)
    {
        return;
    }

    // Dequeue waiting PUBLISH message
    mPublishQos2PublishQueue.Dequeue(*publishMessage);
}

void MqttsnClient::PubrelReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    int32_t packetLength = -1;
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PubrelMessage pubrelMessage;
    if (pubrelMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Process QoS level 2 PUBREL message
    // Find PUBREC message waiting for receive acknowledge
    MessageMetadata<void*> metadata;
    Message* pubrecMessage = mPublishQos2PubrecQueue.Find(pubrelMessage.GetMessageId(), metadata);
    if (!pubrecMessage)
    {
        return;
    }
    // Send PUBCOMP message
    PubcompMessage pubcompMessage(metadata.mMessageId);
    if (pubcompMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
    {
        return;
    }
    Message* responseMessage = NULL;
    if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE ||
        SendMessage(*responseMessage) != OT_ERROR_NONE)
    {
        return;
    }

    // Dequeue waiting message
    mPublishQos2PubrecQueue.Dequeue(*pubrecMessage);
}

void MqttsnClient::PubcompReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PubcompMessage pubcompMessage;
    if (pubcompMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Process QoS level 2 PUBCOMP message
    // Find PUBREL message waiting for receive acknowledge
    MessageMetadata<PublishCallbackFunc> metadata;
    Message* pubrelMessage = mPublishQos2PubrelQueue.Find(pubcompMessage.GetMessageId(), metadata);
    if (!pubrelMessage)
    {
        return;
    }
    // Invoke confirmation callback
    if (metadata.mCallback)
    {
        metadata.mCallback(kCodeAccepted, metadata.mContext);
    }
    // Dequeue waiting message
    mPublishQos2PubrelQueue.Dequeue(*pubrelMessage);
}

void MqttsnClient::UnsubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }

    UnsubackMessage unsubackMessage;
    if (unsubackMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }
    // Find unsubscription message waiting for confirmation
    MessageMetadata<UnsubscribeCallbackFunc> metadata;
    Message* unsubscribeMessage = mUnsubscribeQueue.Find(unsubackMessage.GetMessageId(), metadata);
    if (!unsubscribeMessage)
    {
        return;
    }
    // Invoke unsubscribe confirmation callback
    if (metadata.mCallback)
    {
        metadata.mCallback(kCodeAccepted, metadata.mContext);
    }
    // Dequeue waiting message
    mUnsubscribeQueue.Dequeue(*unsubscribeMessage);
}

void MqttsnClient::PingreqReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    Message* responseMessage = NULL;
    int32_t packetLength = -1;
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        return;
    }

    PingreqMessage pingreqMessage;
    if (pingreqMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Send PINGRESP message
    PingrespMessage pingrespMessage;
    if (pingrespMessage.Serialize(buffer, MAX_PACKET_SIZE, &packetLength) != OT_ERROR_NONE)
    {
        return;
    }
    if (NewMessage(&responseMessage, buffer, packetLength) != OT_ERROR_NONE ||
        SendMessage(*responseMessage, messageInfo.GetPeerAddr(), mConfig.GetPort()) != OT_ERROR_NONE)
    {
        return;
    }
}

void MqttsnClient::PingrespReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }
    PingrespMessage pingrespMessage;
    if (pingrespMessage.Deserialize(data, length) != OT_ERROR_NONE)
    {
        return;
    }

    // Check if any waiting connect message queued
    MessageMetadata<void*> metadata;
    Message *pingreqMessage = mPingreqQueue.Find(0, metadata);
    if (pingreqMessage == NULL)
    {
        return;
    }
    mPingreqQueue.Dequeue(*pingreqMessage);

    // If the client is awake PINRESP message put it into sleep again
    if (mClientState == kStateAwake)
    {
        mClientState = kStateAsleep;
        if (mDisconnectedCallback)
        {
            mDisconnectedCallback(kAsleep, mDisconnectedContext);
        }
    }
}

void MqttsnClient::DisconnectReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length)
{
    DisconnectMessage disconnectMessage;
    if (disconnectMessage.Deserialize(data, length))
    {
        return;
    }

    // Check source IPv6 address
    if (!VerifyGatewayAddress(messageInfo))
    {
        return;
    }

    // Check if the waiting disconnect message is queued
    MessageMetadata<void*> metadata;
    Message *waitingMessage = mDisconnectQueue.Find(0, metadata);
    if (waitingMessage != NULL)
    {
        mDisconnectQueue.Dequeue(*waitingMessage);
    }

    // Handle disconnection behavior depending on client state
    DisconnectType reason = kServer;
    switch (mClientState)
    {
    case kStateActive:
    case kStateAwake:
    case kStateAsleep:
        if (mDisconnectRequested)
        {
            // Regular disconnect
            mClientState = kStateDisconnected;
            reason = kServer;
        }
        else if (mSleepRequested)
        {
            // Sleep state was requested - go asleep
            mClientState = kStateAsleep;
            reason = kAsleep;
        }
        else
        {
            // Disconnected by gateway
            mClientState = kStateDisconnected;
            reason = kServer;
        }
        break;
    default:
        break;
    }
    OnDisconnected();

    // Invoke disconnected callback
    if (mDisconnectedCallback)
    {
        mDisconnectedCallback(reason, mDisconnectedContext);
    }
}

void MqttsnClient::HandleProcessTask(Tasklet &aTasklet)
{
    otError error = aTasklet.GetOwner<MqttsnClient>().Process();
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMqttsn("Process task failed: %s", otThreadErrorToString(error));
    }
}

otError MqttsnClient::Start(uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    Ip6::SockAddr sockaddr;
    sockaddr.mPort = aPort;

    // Open UDP socket
    SuccessOrExit(error = mSocket.Open(MqttsnClient::HandleUdpReceive, this));
    // Start listening on configured port
    SuccessOrExit(error = mSocket.Bind(sockaddr));

    // Enqueue process task which will handle message queues etc.
    SuccessOrExit(error = mProcessTask.Post());
    mIsRunning = true;
exit:
    return error;
}

otError MqttsnClient::Stop()
{
    mIsRunning = false;
    otError error = mSocket.Close();
    // Disconnect client if it is not disconnected already
    mClientState = kStateDisconnected;
    if (mClientState != kStateDisconnected && mClientState != kStateLost)
    {
        OnDisconnected();
        if (mDisconnectedCallback)
        {
            mDisconnectedCallback(kClient, mDisconnectedContext);
        }
    }
    return error;
}

otError MqttsnClient::Process()
{
    otError error = OT_ERROR_NONE;
    uint32_t now = TimerMilli::GetNow().GetValue();

    if (mIsRunning)
    {
        // Enqueue again if client running
        SuccessOrExit(error = mProcessTask.Post());
    }

    // Process keep alive and send periodical PINGREQ message
    if (mClientState == kStateActive && mPingReqTime != 0 && mPingReqTime <= now)
    {
        SuccessOrExit(error = PingGateway());
    }

    // Handle pending messages timeouts
    SuccessOrExit(error = mSubscribeQueue.HandleTimer());
    SuccessOrExit(error = mRegisterQueue.HandleTimer());
    SuccessOrExit(error = mUnsubscribeQueue.HandleTimer());
    SuccessOrExit(error = mPublishQos1Queue.HandleTimer());
    SuccessOrExit(error = mPublishQos2PublishQueue.HandleTimer());
    SuccessOrExit(error = mPublishQos2PubrelQueue.HandleTimer());

exit:
    // Handle timeout
    if (mTimeoutRaised && mClientState == kStateActive)
    {
        mClientState = kStateLost;
        OnDisconnected();
        if (mDisconnectedCallback)
        {
            mDisconnectedCallback(kTimeout, mDisconnectedContext);
        }
    }
    // Only enqueue process when client connected
    if (mClientState != kStateDisconnected && mClientState != kStateLost)
    {
        mProcessTask.Post();
    }
    return error;
}

otError MqttsnClient::Connect(const MqttsnConfig &aConfig)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    ConnectMessage connectMessage(mConfig.GetCleanSession(), false, mConfig.GetKeepAlive(), mConfig.GetClientId().AsCString());
    unsigned char buffer[MAX_PACKET_SIZE];

    // Cannot connect in active state (already connected)
    if (mClientState == kStateActive || !mConnectQueue.IsEmpty())
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }
    mConfig = aConfig;

    // Serialize and send CONNECT message
    SuccessOrExit(error = connectMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    SuccessOrExit(error = mConnectQueue.EnqueueCopy(*message, message->GetLength(),
            MessageMetadata<void*>(mConfig.GetAddress(), mConfig.GetPort(), 0, TimerMilli::GetNow().GetValue(),
                mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), NULL, NULL)));

    mDisconnectRequested = false;
    mSleepRequested = false;

    // Set next keepalive PINGREQ time
    mPingReqTime = TimerMilli::GetNow().GetValue() + mConfig.GetKeepAlive() * 700;
exit:
    return error;
}

otError MqttsnClient::Subscribe(const char* aTopicName, bool aIsShortTopicName, Qos aQos, otMqttsnSubscribedHandler aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Ip6::MessageInfo messageInfo;
    Message *message = NULL;
    int32_t topicNameLength = strlen(aTopicName);
    SubscribeMessage subscribeMessage;
    VerifyOrExit(topicNameLength > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(topicNameLength < kMaxTopicNameLength, error = OT_ERROR_INVALID_ARGS);
    // Topic length must be 1 or 2
    VerifyOrExit(!aIsShortTopicName || length <= 2, error = OT_ERROR_INVALID_ARGS);
    subscribeMessage = aIsShortTopicName ?
        SubscribeMessage(false, aQos, mMessageId, kShortTopicName, 0, aTopicName, "")
        : SubscribeMessage(false, aQos, mMessageId, kTopicName, 0, "", aTopicName);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Topic subscription is possible only for QoS levels 1, 2, 3
    if (aQos != kQos0 || aQos != kQos1 || aQos != kQos2)
    {
        error = OT_ERROR_INVALID_ARGS;
        goto exit;
    }

    // Serialize and send SUBSCRIBE message
    SuccessOrExit(error = subscribeMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    // Enqueue message to waiting queue - waiting for SUBACK
    SuccessOrExit(error = mSubscribeQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<otMqttsnSubscribedHandler>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Subscribe(TopicId aTopicId, Qos aQos, otMqttsnSubscribedHandler aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Ip6::MessageInfo messageInfo;
    Message *message = NULL;
    SubscribeMessage subscribeMessage(false, aQos, mMessageId, kShortTopicName, aTopicId, "", "");
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Topic subscription is possible only for QoS levels 1, 2, 3
    if (aQos != kQos0 || aQos != kQos1 || aQos != kQos2)
    {
        error = OT_ERROR_INVALID_ARGS;
        goto exit;
    }

    // Serialize and send SUBSCRIBE message
    SuccessOrExit(error = subscribeMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    // Enqueue message to waiting queue - waiting for SUBACK
    SuccessOrExit(error = mSubscribeQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<otMqttsnSubscribedHandler>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Register(const char* aTopicName, RegisterCallbackFunc aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    RegisterMessage registerMessage(0, mMessageId, aTopicName);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send REGISTER message
    SuccessOrExit(error = registerMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));
    // Enqueue message to waiting queue - waiting for REGACK
    SuccessOrExit(error = mRegisterQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<RegisterCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Publish(const uint8_t* aData, int32_t aLength, Qos aQos, const char* aShortTopicName, PublishCallbackFunc aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    int32_t topicNameLength = strlen(aShortTopicName);
    PublishMessage publishMessage;
    // Topic length must be 1 or 2
    VerifyOrExit(topicNameLength > 0 && topicNameLength <= 2, error = OT_ERROR_INVALID_ARGS);
    publishMessage = PublishMessage(false, false, aQos, mMessageId, kShortTopicName, 0, aShortTopicName, aData, aLength);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send PUBLISH message
    SuccessOrExit(error = publishMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));
    if (aQos == kQos1)
    {
        // If QoS level 1 enqueue message to waiting queue - waiting for PUBACK
        SuccessOrExit(error = mPublishQos1Queue.EnqueueCopy(*message, message->GetLength(),
            MessageMetadata<PublishCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
                mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    }
    if (aQos == kQos2)
    {
        // If QoS level 1 enqueue message to waiting queue - waiting for PUBREC
        SuccessOrExit(error = mPublishQos2PublishQueue.EnqueueCopy(*message, message->GetLength(),
            MessageMetadata<PublishCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
                mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    }
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Publish(const uint8_t* aData, int32_t aLength, Qos aQos, TopicId aTopicId, PublishCallbackFunc aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    PublishMessage publishMessage(false, false, aQos, mMessageId, kTopicId, aTopicId, "", aData, aLength);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send PUBLISH message
    SuccessOrExit(error = publishMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));
    if (aQos == kQos1)
    {
        // If QoS level 1 enqueue message to waiting queue - waiting for PUBACK
        SuccessOrExit(error = mPublishQos1Queue.EnqueueCopy(*message, message->GetLength(),
            MessageMetadata<PublishCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
                mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    }
    if (aQos == kQos2)
    {
        // If QoS level 1 enqueue message to waiting queue - waiting for PUBREC
        SuccessOrExit(error = mPublishQos2PublishQueue.EnqueueCopy(*message, message->GetLength(),
            MessageMetadata<PublishCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
                mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    }
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::PublishQosm1(const uint8_t* aData, int32_t aLength, const char* aShortTopicName, Ip6::Address aAddress, uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    PublishMessage publishMessage;
    int32_t topicNameLength = strlen(aShortTopicName);
    VerifyOrExit(topicNameLength > 0 && topicNameLength <= 2, error = OT_ERROR_INVALID_ARGS);
    publishMessage = PublishMessage(false, false, kQosm1, mMessageId, kShortTopicName, 0, aShortTopicName, aData, aLength);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Serialize and send PUBLISH message
    SuccessOrExit(error = publishMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message, aAddress, aPort));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::PublishQosm1(const uint8_t* aData, int32_t aLength, TopicId aTopicId, Ip6::Address aAddress, uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    PublishMessage publishMessage(false, false, kQosm1, mMessageId, kTopicId, aTopicId, "", aData, aLength);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Serialize and send PUBLISH message
    SuccessOrExit(error = publishMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message, aAddress, aPort));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Unsubscribe(const char* aShortTopicName, UnsubscribeCallbackFunc aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    int32_t topicNameLength = strlen(aShortTopicName);
    UnsubscribeMessage unsubscribeMessage;
    // Topic length must be 1 or 2
    VerifyOrExit(topicNameLength > 0 && topicNameLength <= 2, error = OT_ERROR_INVALID_ARGS);
    unsubscribeMessage = UnsubscribeMessage(mMessageId, kShortTopicName, 0, aShortTopicName);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send UNSUBSCRIBE message
    SuccessOrExit(error = unsubscribeMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));
    // Enqueue message to waiting queue - waiting for UNSUBACK
    SuccessOrExit(error = mUnsubscribeQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<UnsubscribeCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Unsubscribe(TopicId aTopicId, UnsubscribeCallbackFunc aCallback, void* aContext)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    UnsubscribeMessage unsubscribeMessage(mMessageId, kTopicId, aTopicId, "");
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client state must be active
    if (mClientState != kStateActive)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send UNSUBSCRIBE message
    SuccessOrExit(error = unsubscribeMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));
    // Enqueue message to waiting queue - waiting for UNSUBACK
    SuccessOrExit(error = mUnsubscribeQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<UnsubscribeCallbackFunc>(mConfig.GetAddress(), mConfig.GetPort(), mMessageId, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), aCallback, aContext)));
    mMessageId++;

exit:
    return error;
}

otError MqttsnClient::Disconnect()
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    DisconnectMessage disconnectMessage(0);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client must be connected
    if ((mClientState != kStateActive && mClientState != kStateAwake
        && mClientState != kStateAsleep) || !mDisconnectQueue.IsEmpty())
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send DISCONNECT message
    SuccessOrExit(error = disconnectMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    SuccessOrExit(error = mDisconnectQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<void*>(mConfig.GetAddress(), mConfig.GetPort(), 0, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, 0, NULL, NULL)));

    // Set flag for regular disconnect request and wait for DISCONNECT message from gateway
    mDisconnectRequested = true;

exit:
    return error;
}

otError MqttsnClient::Sleep(uint16_t aDuration)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    DisconnectMessage disconnectMessage(aDuration);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Client must be connected
    if ((mClientState != kStateActive && mClientState != kStateAwake && mClientState != kStateAsleep)
        || !mDisconnectQueue.IsEmpty())
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Serialize and send DISCONNECT message
    SuccessOrExit(error = disconnectMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    SuccessOrExit(error = mDisconnectQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<void*>(mConfig.GetAddress(), mConfig.GetPort(), 0, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, 0, NULL, NULL)));

    // Set flag for sleep request and wait for DISCONNECT message from gateway
    mSleepRequested = true;

exit:
    return error;
}

otError MqttsnClient::Awake(uint32_t aTimeout)
{
	OT_UNUSED_VARIABLE(aTimeout);
    otError error = OT_ERROR_NONE;
    // Awake is possible only when the client is in
    if (mClientState != kStateAwake && mClientState != kStateAsleep)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // Send PINGEQ message
    SuccessOrExit(error = PingGateway());

    // Set awake state and wait for any PUBLISH messages
    mClientState = kStateAwake;
exit:
    return error;
}

otError MqttsnClient::SearchGateway(const Ip6::Address &aMulticastAddress, uint16_t aPort, uint8_t aRadius)
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    SearchGwMessage searchGwMessage(aRadius);
    unsigned char buffer[MAX_PACKET_SIZE];

    // Serialize and send SEARCHGW message
    SuccessOrExit(error = searchGwMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message, aMulticastAddress, aPort, aRadius));

exit:
    return error;
}

ClientState MqttsnClient::GetState()
{
    return mClientState;
}

otError MqttsnClient::SetConnectedCallback(otMqttsnConnectedHandler aCallback, void* aContext)
{
    mConnectedCallback = aCallback;
    mConnectContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::SetPublishReceivedCallback(PublishReceivedCallbackFunc aCallback, void* aContext)
{
    mPublishReceivedCallback = aCallback;
    mPublishReceivedContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::SetAdvertiseCallback(AdvertiseCallbackFunc aCallback, void* aContext)
{
    mAdvertiseCallback = aCallback;
    mAdvertiseContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::SetSearchGwCallback(SearchGwCallbackFunc aCallback, void* aContext)
{
    mSearchGwCallback = aCallback;
    mSearchGwContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::SetDisconnectedCallback(DisconnectedCallbackFunc aCallback, void* aContext)
{
    mDisconnectedCallback = aCallback;
    mDisconnectedContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::SetRegisterReceivedCallback(RegisterReceivedCallbackFunc aCallback, void* aContext)
{
    mRegisterReceivedCallback = aCallback;
    mRegisterReceivedContext = aContext;
    return OT_ERROR_NONE;
}

otError MqttsnClient::NewMessage(Message **aMessage, unsigned char* aBuffer, int32_t aLength)
{
    otError error = OT_ERROR_NONE;
    Message *message = NULL;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->Append(aBuffer, aLength));
    *aMessage = message;

exit:
    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
    return error;
}

otError MqttsnClient::SendMessage(Message &aMessage)
{
    return SendMessage(aMessage, mConfig.GetAddress(), mConfig.GetPort());
}

otError MqttsnClient::SendMessage(Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort)
{
    return SendMessage(aMessage, aAddress, aPort, 0);
}

otError MqttsnClient::SendMessage(Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, uint8_t aHopLimit)
{
    otError error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;

    messageInfo.SetHopLimit(aHopLimit);
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(aPort);
    messageInfo.SetIsHostInterface(false);

    otLogDebgMqttsn("Sending message to %s[:%u]", messageInfo.GetPeerAddr().ToString().AsCString(), messageInfo.GetPeerPort());
    SuccessOrExit(error = mSocket.SendTo(aMessage, messageInfo));

exit:
    if (error != OT_ERROR_NONE)
    {
        aMessage.Free();
    }
    return error;
}

otError MqttsnClient::PingGateway()
{
    otError error = OT_ERROR_NONE;
    int32_t length = -1;
    Message* message = NULL;
    PingreqMessage pingreqMessage(mConfig.GetClientId().AsCString());
    unsigned char buffer[MAX_PACKET_SIZE];

    if (mClientState != kStateActive && mClientState != kStateAwake)
    {
        error = OT_ERROR_INVALID_STATE;
        goto exit;
    }

    // There is already pingreq message waiting
    if (!mConnectQueue.IsEmpty())
    {
        goto exit;
    }

    // Serialize and send PINGREQ message
    SuccessOrExit(error = pingreqMessage.Serialize(buffer, MAX_PACKET_SIZE, &length));
    SuccessOrExit(error = NewMessage(&message, buffer, length));
    SuccessOrExit(error = SendMessage(*message));

    SuccessOrExit(error = mPingreqQueue.EnqueueCopy(*message, message->GetLength(),
        MessageMetadata<void*>(mConfig.GetAddress(), mConfig.GetPort(), 0, TimerMilli::GetNow().GetValue(),
            mConfig.GetRetransmissionTimeout() * 1000, mConfig.GetRetransmissionCount(), NULL, NULL)));

    mPingReqTime = TimerMilli::GetNow().GetValue() + mConfig.GetKeepAlive() * 700;

exit:
    return error;
}

void MqttsnClient::OnDisconnected()
{
    mDisconnectRequested = false;
    mSleepRequested = false;
    mTimeoutRaised = false;
    mPingReqTime = 0;

    mSubscribeQueue.ForceTimeout();
    mRegisterQueue.ForceTimeout();
    mUnsubscribeQueue.ForceTimeout();
    mPublishQos1Queue.ForceTimeout();
    mPublishQos2PublishQueue.ForceTimeout();
    mPublishQos2PubrelQueue.ForceTimeout();
}

bool MqttsnClient::VerifyGatewayAddress(const Ip6::MessageInfo &aMessageInfo)
{
    return aMessageInfo.GetPeerAddr() == mConfig.GetAddress()
        && aMessageInfo.GetPeerPort() == mConfig.GetPort();
}

void MqttsnClient::HandleSubscribeTimeout(const MessageMetadata<otMqttsnSubscribedHandler> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, 0, kQos0, aMetadata.mContext);
}

void MqttsnClient::HandleRegisterTimeout(const MessageMetadata<RegisterCallbackFunc> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, 0, aMetadata.mContext);
}

void MqttsnClient::HandleUnsubscribeTimeout(const MessageMetadata<UnsubscribeCallbackFunc> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, aMetadata.mContext);
}

void MqttsnClient::HandlePublishQos1Timeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, aMetadata.mContext);
}

void MqttsnClient::HandlePublishQos2PublishTimeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, aMetadata.mContext);
}

void MqttsnClient::HandlePublishQos2PubrelTimeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    client->mTimeoutRaised = true;
    aMetadata.mCallback(kCodeTimeout, aMetadata.mContext);
}

void MqttsnClient::HandlePublishQos2PubrecTimeout(const MessageMetadata<void*> &aMetadata, void* aContext)
{
    OT_UNUSED_VARIABLE(aMetadata);
    OT_UNUSED_VARIABLE(aContext);
}

void MqttsnClient::HandleConnectTimeout(const MessageMetadata<void*> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    OT_UNUSED_VARIABLE(aMetadata);
    OT_UNUSED_VARIABLE(aContext);
    client->mTimeoutRaised = true;
    if (client->mConnectedCallback)
    {
        client->mConnectedCallback(kCodeTimeout, client->mConnectContext);
    }
}

void MqttsnClient::HandleDisconnectTimeout(const MessageMetadata<void*> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    OT_UNUSED_VARIABLE(aMetadata);
    OT_UNUSED_VARIABLE(aContext);
    client->mTimeoutRaised = true;
}

void MqttsnClient::HandlePingreqTimeout(const MessageMetadata<void*> &aMetadata, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    OT_UNUSED_VARIABLE(aMetadata);
    OT_UNUSED_VARIABLE(aContext);
    client->mTimeoutRaised = true;
}

void MqttsnClient::HandleMessageRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    Message* retransmissionMessage = aMessage.Clone(aMessage.GetLength());
    if (retransmissionMessage != NULL)
    {
        client->SendMessage(*retransmissionMessage, aAddress, aPort);
    }
}

void MqttsnClient::HandlePublishRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    unsigned char buffer[MAX_PACKET_SIZE];
    PublishMessage publishMessage;

    // Read message content
    uint16_t offset = aMessage.GetOffset();
    int32_t length = aMessage.GetLength() - aMessage.GetOffset();

    unsigned char data[MAX_PACKET_SIZE];

    if (length > MAX_PACKET_SIZE || !data)
    {
        return;
    }
    aMessage.Read(offset, length, data);
    if (publishMessage.Deserialize(buffer, length) != OT_ERROR_NONE)
    {
        return;
    }
    // Set DUP flag
    publishMessage.SetDupFlag(true);
    if (publishMessage.Serialize(buffer, MAX_PACKET_SIZE, &length) != OT_ERROR_NONE)
    {
        return;
    }
    Message* retransmissionMessage = NULL;
    if (client->NewMessage(&retransmissionMessage, buffer, length) != OT_ERROR_NONE)
    {
        return;
    }
    client->SendMessage(*retransmissionMessage, aAddress, aPort);
}

void MqttsnClient::HandleSubscribeRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext)
{
    MqttsnClient* client = static_cast<MqttsnClient*>(aContext);
    unsigned char buffer[MAX_PACKET_SIZE];
    SubscribeMessage subscribeMessage;

    // Read message content
    uint16_t offset = aMessage.GetOffset();
    int32_t length = aMessage.GetLength() - aMessage.GetOffset();

    unsigned char data[MAX_PACKET_SIZE];

    if (length > MAX_PACKET_SIZE || !data)
    {
        return;
    }
    aMessage.Read(offset, length, data);
    if (subscribeMessage.Deserialize(buffer, length) != OT_ERROR_NONE)
    {
        return;
    }
    // Set DUP flag
    subscribeMessage.SetDupFlag(true);
    if (subscribeMessage.Serialize(buffer, MAX_PACKET_SIZE, &length) != OT_ERROR_NONE)
    {
        return;
    }
    Message* retransmissionMessage = NULL;
    if (client->NewMessage(&retransmissionMessage, buffer, length) != OT_ERROR_NONE)
    {
        return;
    }
    client->SendMessage(*retransmissionMessage, aAddress, aPort);
}

}

}
