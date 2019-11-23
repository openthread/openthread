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

#ifndef MQTTSN_CLIENT_HPP_
#define MQTTSN_CLIENT_HPP_

#include "common/locator.hpp"
#include "common/instance.hpp"
#include "common/tasklet.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"
#include <openthread/error.h>
#include <openthread/mqttsn.h>

/**
 * @file
 *   This file includes interface for MQTT-SN protocol v1.2 client.
 *
 */

namespace ot {

namespace Mqttsn {

/**
 * MQTT-SN message return code.
 *
 */
typedef otMqttsnReturnCode ReturnCode;

/**
 * MQTT-SN quality of service level.
 *
 */
typedef otMqttsnQos Qos;

/**
 * Disconnected state reason.
 *
 */
enum DisconnectType
{
    /**
     * Client was disconnected by gateway/broker.
     */
    kServer,
    /**
     * Disconnection was invoked by client.
     */
    kClient,
    /**
     * Client changed state to asleep
     */
    kAsleep,
    /**
     * Communication timeout.
     */
    kTimeout
};

/**
 * Client lifecycle states.
 *
 */
typedef otMqttsnClientState ClientState;

enum
{
    /**
     * Client ID maximal length.
     *
     */
    kCliendIdStringMax = 24,
    /**
     * Long topic name maximal length (with null terminator).
     *
     */
    kMaxTopicNameLength = 50,
    /**
     * Short topic maximal length (with null terminator).
     *
     */
    kShortTopicNameLength = 3
};

/**
 * MQTT-SN topic identificator type.
 */
enum TopicIdType
{
    /**
     * Predefined topic ID.
     *
     */
    kTopicId,
    /**
     * Two character short topic name.
     *
     */
    kShortTopicName,
    /**
     * Long topic name.
     *
     */
    kTopicName
};

/**
 * Topic ID type.
 *
 */
typedef otMqttsnTopicId TopicId;

/**
 * Short topic name string.
 *
 */
typedef String<kShortTopicNameLength> ShortTopicNameString;

/**
 * Long topic name string.
 *
 */
typedef String<kMaxTopicNameLength> TopicNameString;

/**
 * Client ID string.
 *
 */
typedef String<kCliendIdStringMax> ClientIdString;

template <typename CallbackType>
class WaitingMessagesQueue;

/**
 * Message metadata which are stored in waiting messages queue.
 *
 */
template <typename CallbackType>
class MessageMetadata
{
    friend class MqttsnClient;
    friend class WaitingMessagesQueue<CallbackType>;

public:
    /**
     * Default constructor for the object.
     *
     */
    MessageMetadata(void);

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aDestinationAddress     A reference of message destination IPv6 address.
     * @param[in]  aDestinationPort        Message destination port.
     * @param[in]  aMessageId              MQTT-SN Message ID.
     * @param[in]  aTimestamp              Time stamp of message in milliseconds for timeout evaluation.
     * @param[in]  aRetransmissionTimeout  Time in millisecond after which message is message timeout invoked.
     * @param[in]  aCallback               A function pointer for handling message timeout.
     * @param[in]  aContext                Pointer to callback passed to timeout callback.
     *
     */
    MessageMetadata(const Ip6::Address &aDestinationAddress, uint16_t aDestinationPort, uint16_t aMessageId, uint32_t aTimestamp, uint32_t aRetransmissionTimeout, uint8_t aRetransmissionCount, CallbackType aCallback, void* aContext);

    /**
     * Append metadata to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const;

    /**
     * Update metadata of the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully updated the message.
     *
     */
    otError UpdateIn(Message &aMessage) const;

    /**
     * Read metadata from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(Message &aMessage);

    /**
     * Get message without metadata bytes.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns  Returns a pointer to the message or null if invalid input message.
     *
     */
    Message* GetRawMessage(const Message &aMessage) const;

    /**
     * Get metadata length in bytes.
     *
     * @returns The number of bytes.
     *
     */
    uint16_t GetLength(void) const;

private:
    /**
     * A reference of message destination IPv6 address.
     *
     */
    Ip6::Address mDestinationAddress;
    /**
     * Message destination port.
     *
     */
    uint16_t mDestinationPort;
    /**
     * MQTT-SN Message ID.
     *
     */
    uint16_t mMessageId;
    /**
     * Time stamp of message in milliseconds for timeout evaluation.
     *
     */
    uint32_t mTimestamp;
    /**
     * Time in millisecond after which message is message timeout invoked.
     *
     */
    uint32_t mRetransmissionTimeout;
    /**
     * Message retransmission count.
     *
     */
    uint8_t mRetransmissionCount;
    /**
     * A function pointer for handling message timeout.
     *
     */
    CallbackType mCallback;
    /**
     * A pointer to callback passed to timeout callback.
     *
     */
    void* mContext;
};

/**
 * The class represents the queue which contains messages waiting for acknowledgments from gateway.
 *
 */
template <typename CallbackType>
class WaitingMessagesQueue
{
public:
    /**
     * Declaration of a function pointer which is used as timeout callback.
     *
     * @param[in]  aMetadata  A reference to message metadata.
     * @param[in]  aContext   A poter to timeout callback context object.
     *
     */
    typedef void (*TimeoutCallbackFunc)(const MessageMetadata<CallbackType> &aMetadata, void* aContext);

    /**
     * Declaration of a function pointer for handling message retransmission.
     *
     * @param[in]  aMessage  A reference to message to be resend.
     * @param[in]  aAddress  A reference to destination gateway address.
     * @param[in]  aPort     Destination port.
     * @param[in]  aContext  A pointer to retransmission callback context object.
     */
    typedef void (*RetransmissionFunc)(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext);

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aTimeoutCallback        A function pointer to callback which is invoked on message timeout.
     * @param[in]  aTimeoutContext         A pointer to context passed to timeout callback.
     * @param[in]  aRetransmissionFunc     A function pointer to retransmission function.
     * @param[in]  aRetransmissionContext  A pointer to context passed to retransmission function.
     *
     */
    WaitingMessagesQueue(TimeoutCallbackFunc aTimeoutCallback, void* aTimeoutContext, RetransmissionFunc aRetransmissionFunc, void* aRetransmissionContext);

    /**
     * Default object destructor.
     *
     */
    ~WaitingMessagesQueue(void);

    /**
     * Copy message data and enqueue message to waiting queue.
     *
     * @param[in]  aMessage   A reference to message object to be enqueued.
     * @param[in]  aLength    Message length.
     * @param[in]  aMetadata  A reference to message metadata.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the message.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to copy or enqueue the message.
     *
     */
    otError EnqueueCopy(const Message &aMessage, uint16_t aLength, const MessageMetadata<CallbackType> &aMetadata);

    /**
     * Dequeue specific message from waiting queue.
     *
     * @param[in]  aMessage   A reference to message object to be dequeued.
     *
     * @retval OT_ERROR_NONE       Successfully dequeued the message.
     * @retval OT_ERROR_NOT_FOUND  Message was not found in waiting queue.
     *
     */
    otError Dequeue(Message &aMessage);

    /**
     * Find message by message ID and read message metadata.
     *
     * @param[in]  aMessageId  Message ID.
     * @param[out] aMetadata   A reference to initialized metadata object.
     *
     * @returns  If the message is found the message ID is returned or null otherwise.
     *
     */
    Message* Find(uint16_t aMessageId, MessageMetadata<CallbackType> &aMetadata);

    /**
     * Evaluate queued messages timeout and retransmission.
     *
     * @retval OT_ERROR_NONE  Timeouts were successfully processed.
     *
     */
    otError HandleTimer(void);

    /**
     * Force waiting messages timeout, invoke callback and empty queue.
     *
     */
    void ForceTimeout(void);

    /**
     * Determine if is the queue empty.
     *
     * @returns  Returns true if the queue is empty or false otherwise.
     */
    bool IsEmpty(void);

private:
    MessageQueue mQueue;
    TimeoutCallbackFunc mTimeoutCallback;
    void* mTimeoutContext;
    RetransmissionFunc mRetransmissionFunc;
    void* mRetransmissionContext;
};

/**
 * This class contains MQTT-SN connection parameters.
 *
 */
class MqttsnConfig
{
public:

    /**
     * Default constructor for the object.
     *
     */
    MqttsnConfig(void)
        : mAddress()
        , mPort()
        , mClientId()
        , mKeepAlive(30)
        , mCleanSession()
        , mRetransmissionTimeout(10)
        , mRetransmissionCount(3)
    {
        ;
    }

    /**
     * Get gateway IPv6 address.
     *
     * @returns A reference to IPv6 address.
     *
     */
    const Ip6::Address &GetAddress()
    {
        return mAddress;
    }

    /**
     * Set gateway IPv6 address.
     *
     * @param[in]  aAddress  A reference to gateway IPv6 address.
     *
     */
    void SetAddress(const Ip6::Address &aAddress)
    {
        mAddress = aAddress;
    }

    /**
     * Get gateway interface port number.
     *
     * @returns Gateway port number.
     *
     */
    uint16_t GetPort()
    {
        return mPort;
    }

    /**
     * Set gateway interface port number.
     *
     * @param[in]  aPort  Gateway port number.
     *
     */
    void SetPort(uint16_t aPort)
    {
        mPort = aPort;
    }

    /**
     * Get client ID.
     *
     * @returns Cliend ID string.
     *
     */
    const ClientIdString &GetClientId()
    {
        return mClientId;
    }

    /**
     * Set client ID.
     *
     * @param[in]  aClientId  A pointer to client ID string.
     *
     */
    void SetClientId(const char* aClientId)
    {
        mClientId.Set("%s", aClientId);
    }

    /**
     * Get keepalive period in seconds.
     *
     * @returns Keepalive time in seconds.
     *
     */
    int16_t GetKeepAlive()
    {
        return mKeepAlive;
    }

    /**
     * Set keepalive period in seconds.
     *
     * @param[in]  Keepalive time in seconds.
     *
     */
    void SetKeepAlive(int16_t aDuration)
    {
        mKeepAlive = aDuration;
    }

    /**
     * Get clean session flag.
     *
     * @returns Clean session flag.
     *
     */
    bool GetCleanSession()
    {
        return mCleanSession;
    }

    /**
     * Set clean session flag.
     *
     * @param[in]  aCleanSession  Clean session flag.
     *
     */
    void SetCleanSession(bool aCleanSession)
    {
        mCleanSession = aCleanSession;
    }

    /**
     * Get retransmission timeout in milliseconds.
     *
     * @returns Retransmission timeout in milliseconds.
     *
     */
    uint32_t GetRetransmissionTimeout()
    {
        return mRetransmissionTimeout;
    }

    /**
     * Set retransmission timeout in milliseconds.
     *
     * @param[in]  aTimeout  Retransmission timeout value in milliseconds.
     *
     */
    void SetRetransmissionTimeout(uint32_t aTimeout)
    {
        mRetransmissionTimeout = aTimeout;
    }

    /**
     * Get retransmission count.
     *
     * @returns Retransmission count.
     *
     */
    uint8_t GetRetransmissionCount()
    {
        return mRetransmissionCount;
    }

    /**
     * Set retransmission count.
     *
     * @param[in]  aTimeout  Retransmission count.
     *
     */
    void SetRetransmissionCount(uint8_t aCount)
    {
        mRetransmissionCount = aCount;
    }

private:
    Ip6::Address mAddress;
    uint16_t mPort;
    ClientIdString mClientId;
    uint16_t mKeepAlive;
    bool mCleanSession;
    uint32_t mRetransmissionTimeout;
    uint8_t mRetransmissionCount;
};

/**
 * The class representing MQTT-SN protocol client.
 *
 */
class MqttsnClient: public InstanceLocator
{
public:
    /**
     * Declaration of function for callback invoked when publish message received.
     *
     * @param[in]  aPayload         A pointer to PUBLISH message payload byte array.
     * @param[in]  aPayloadLength   PUBLISH message payload length.
     * @param[in]  aTopicIdType     Topic ID type. Possible values are kTopicId or kShortTopicName.
     * @param[in]  aTopicId         PUBLISH message topic ID. It is set only when aTopicIdType is kTopicId.
     * @param[in]  aShortTopicName  PUBLISH message short topic name. It is set only when aTopicIdType is kShortTopicName.
     * @param[in]  aContext         A pointer to publish received callback context object.
     *
     * @returns  Code to be send in response PUBACK message. Timeout value is not relevant.
     */
    typedef ReturnCode (*PublishReceivedCallbackFunc)(const uint8_t* aPayload, int32_t aPayloadLength, TopicIdType aTopicIdType, TopicId aTopicId, ShortTopicNameString aShortTopicName, void* aContext);

    /**
     * Declaration of function for advertise callback.
     *
     * @param[in]  aAddress    A reference to advertised gateway IPv6 address.
     * @param[in]  aGatewayId  Advertised gateway ID.
     * @param[in]  aDuration   Advertise message duration parameter.
     * @param[in]  aContext    A pointer to advertise callback context object.
     *
     */
    typedef void (*AdvertiseCallbackFunc)(const Ip6::Address &aAddress, uint8_t aGatewayId, uint32_t aDuration, void* aContext);

    /**
     * Declaration of function for search gateway callback.
     *
     * @param[in]  aAddress  A reference to IPv6 address of discovered gateway.
     * @param[in]  aAddress  Discovered gateway ID.
     * @param[in]  aContext  A pointer to search gateway context object.
     *
     */
    typedef void (*SearchGwCallbackFunc)(const Ip6::Address &aAddress, uint8_t aGatewayId, void* aContext);

    /**
     * Declaration of function for register callback.
     *
     * @param[in]  aCode     REGACK return code or -1 when subscription timed out.
     * @param[in]  aTopicId  Registered topic ID.
     * @param[in]  aContext  A pointer to register callback context object.
     *
     */
    typedef void (*RegisterCallbackFunc)(ReturnCode aCode, TopicId aTopicId, void* aContext);

    /**
     * Declaration of function for callback invoked when register message received.
     *
     * @param[in]  aTopicId    Registered topic ID.
     * @param[in]  aTopicName  Long topic name string mapped to registered topic.
     * @param[in]  aContext    A pointer to register received callback context object.
     *
     * @returns  Code to be send in response REGACK message. Timeout value is not relevant.
     *
     */
    typedef ReturnCode (*RegisterReceivedCallbackFunc)(TopicId aTopicId, const TopicNameString &aTopicName, void* aContext);

    /**
     * Declaration of function for publish callback. It is invoked only when quality of service level is 1 or 2.
     *
     * @param[in]  aCode     Publish response code or -1 when publish timed out.
     * @param[in]  aTopicId  Topic ID of published message. It is set to 0 when timed out or short topic name is used.
     * @param[in]  aContext  A pointer to publish callback context object.
     *
     */
    typedef void (*PublishCallbackFunc)(ReturnCode aCode, void* aContext);

    /**
     * Declaration of function for unsubscribe callback.
     *
     * @param[in]  aCode     UNSUBACK response code or -1 when publish timed out.
     * @param[in]  aContext  A pointer to unsubscribe callback context object.
     *
     */
    typedef void (*UnsubscribeCallbackFunc)(ReturnCode aCode, void* aContext);

    /**
     * Declaration of function for disconnection callback.
     *
     * @param[in]  aType     Disconnection reason.
     * @param[in]  aContext  A pointer to disconnection callback context object.
     *
     */
    typedef void (*DisconnectedCallbackFunc)(DisconnectType aType, void* aContext);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    MqttsnClient(Instance &aInstance);

    /**
     * Default object destructor.
     *
     */
    ~MqttsnClient(void);

    /**
     * Start MQTT-SN service and start connection and listening.
     *
     * @param[in]  aPort  MQTT-SN client listening port.
     *
     * @retval OT_ERROR_NONE  Successfully started the service.
     *
     */
    otError Start(uint16_t aPort);

    /**
     * Stop MQTT-SN service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the service.
     *
     */
    otError Stop(void);

    /**
     * Process service workers.
     *
     * @retval OT_ERROR_NONE  Successfully processed.
     *
     */
    otError Process(void);

    /**
     * Establish MQTT-SN connection with gateway.
     *
     * @param[in]  aConfig  A reference to configuration object with connection parameters.
     *
     * @retval OT_ERROR_NONE           Connection message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid connection parameters.
     * @retval OT_ERROR_INVALID_STATE  The client is in invalid state. It must be disconnected before new connection establishment.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Connect(const MqttsnConfig &aConfig);

    /**
     * Subscribe to the topic by topic name string.
     *
     * @param[in]  aTopicName         A pointer to long topic name string.
     * @param[in]  aIsShortTopicName  Set to true when subscribing by long topic name or false in case of short topic name.
     * @param[in]  aQos               Quality of service level to be subscribed.
     * @param[in]  aCallback          A function pointer to callback which is invoked when subscription is acknowledged.
     * @param[in]  aContext           A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Subscription message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid subscription parameters.
     * @retval OT_ERROR_INVALID_STATE  The client cannot connect in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Subscribe(const char* aTopicName, bool aIsShortTopicName, Qos aQos, otMqttsnSubscribedHandler aCallback, void* aContext);

    /**
     * Subscribe to the topic by topic ID.
     *
     * @param[in]  aTopicId   Topic ID to subscribe to.
     * @param[in]  aCallback  A function pointer to callback which is invoked when subscription is acknowledged.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Subscription message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid subscription parameters.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     */
    otError Subscribe(TopicId aTopicId, Qos aQos, otMqttsnSubscribedHandler aCallback, void* aContext);

    /**
     * Register to topic with long topic name and obtain related topic ID.
     *
     * @param[in]  aTopicName  A pointer to long topic name string.
     * @param[in]  aCallback   A function pointer to callback invoked when registration is acknowledged.
     * @param[in]  aContext    A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Registration message successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Register(const char* aTopicName, otMqttsnRegisteredHandler aCallback, void* aContext);

    /**
     * Publish message to the topic with specific short topic name.
     *
     * @param[in]  aData            A pointer to byte array to be send as message payload.
     * @param[in]  aLength          Length of message payload data.
     * @param[in]  aQos             Message quality of service level.
     * @param[in]  aShortTopicName  A pointer to short topic name string of target topic.
     * @param[in]  aCallback        A function pointer to callback invoked when registration is acknowledged.
     * @param[in]  aContext         A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Publish message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid publish parameters. Short topic name must have one or two characters.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Publish(const uint8_t* aData, int32_t aLength, Qos aQos, const char* aShortTopicName, PublishCallbackFunc aCallback, void* aContext);

    /**
     * Publish message to the topic with specific topic ID.
     *
     * @param[in]  aData      A pointer to byte array to be send as message payload.
     * @param[in]  aLength    Length of message payload data.
     * @param[in]  aQos       Message quality of service level.
     * @param[in]  aTopicId   Topic ID of target topic.
     * @param[in]  aCallback  A function pointer to callback invoked when registration is acknowledged.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Publish message successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Publish(const uint8_t* aData, int32_t aLength, Qos aQos, TopicId aTopicId, PublishCallbackFunc aCallback, void* aContext);

    /**
     * Publish message to the topic with specific short topic name with QoS level -1. No connection or subscription is required.
     *
     * @param[in]  aData            A pointer to byte array to be send as message payload.
     * @param[in]  aLength          Length of message payload data.
     * @param[in]  aShortTopicName  A pointer to short topic name string of target topic.
     * @param[in]  aAddress         Gateway address.
     * @param[in]  aPort            Gateway port.
     *
     * @retval OT_ERROR_NONE           Publish message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid publish parameters. Short topic name must have one or two characters.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError PublishQosm1(const uint8_t* aData, int32_t aLength, const char* aShortTopicName, Ip6::Address aAddress, uint16_t aPort);

    /**
     * Publish message to the topic with specific topic ID with QoS level -1. No connection or subscription is required.
     *
     * @param[in]  aData      A pointer to byte array to be send as message payload.
     * @param[in]  aLength    Length of message payload data.
     * @param[in]  aTopicId   Topic ID of target topic.
     * @param[in]  aAddress   Gateway address.
     * @param[in]  aPort      Gateway port.
     *
     * @retval OT_ERROR_NONE           Publish message successfully queued.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError PublishQosm1(const uint8_t* aData, int32_t aLength, TopicId aTopicId, Ip6::Address aAddress, uint16_t aPort);

    /**
     * Unsubscribe from the topic with specific short topic name.
     *
     * @param[in]  aShortTopicName  A pointer to short topic name string of target topic.
     * @param[in]  aCallback        A function pointer to callback invoked when unsubscription is acknowledged.
     * @param[in]  aContext         A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Unsubscribe message successfully queued.
     * @retval OT_ERROR_INVALID_ARGS   Invalid unsubscribe parameters. Short topic name must have one or two characters.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Unsubscribe(const char* aShortTopicName, UnsubscribeCallbackFunc aCallback, void* aContext);

    /**
     * Unsubscribe from the topic with specific topic ID.
     *
     * @param[in]  aTopicId   A pointer to short topic name string of target topic.
     * @param[in]  aCallback  A function pointer to callback invoked when unsubscription is acknowledged.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE           Unsubscribe message successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to unsubscribe.
     *
     */
    otError Unsubscribe(TopicId aTopicId, UnsubscribeCallbackFunc aCallback, void* aContext);

    /**
     * Disconnect MQTT-SN client from gateway.
     *
     * @retval OT_ERROR_NONE           Disconnection message successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in relevant state. It must be asleep, awake or active.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Disconnect(void);

    /**
     * Put the client into asleep state or change sleep duration. Client must be awaken or reconnected before duration time passes.
     *
     * @param[in]  aDuration  Duration time for which will the client stay in asleep state.
     *
     * @retval OT_ERROR_NONE           Sleep request successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in relevant state. It must be asleep, awake or active.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Sleep(uint16_t aDuration);

    /**
     * Awake the client and receive pending messages.
     *
     * @param[in]  aTimeout  Timeout in milliseconds for staying in awake state. PINGRESP message must be received before timeout time passes.
     *
     * @retval OT_ERROR_NONE           Awake request successfully queued.
     * @retval OT_ERROR_INVALID_STATE  The client is not in relevant state. It must be asleep or awake.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError Awake(uint32_t aTimeout);

    /**
     * Search for gateway with multicast message.
     *
     * @param[in]  aMulticastAddress  A reference to multicast IPv6 address.
     * @param[in]  aPort              Gateway port number.
     * @param[in]  aRadius            Message hop limit (time to live)
     *
     * @retval OT_ERROR_NONE           Search gateway request successfully queued.
     * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
     *
     */
    otError SearchGateway(const Ip6::Address &aMulticastAddress, uint16_t aPort, uint8_t aRadius);

    /**
     * Get current MQTT-SN client state.
     *
     * @returns Current client state.
     *
     */
    ClientState GetState(void);

    /**
     * Set callback function invoked when connection acknowledged or timed out.
     *
     * @param[in]  aCallback  A function pointer to connect callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetConnectedCallback(otMqttsnConnectedHandler aCallback, void* aContext);

    /**
     * Set callback function invoked when publish message received from the topic.
     *
     * @param[in]  aCallback  A function pointer to publish received callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetPublishReceivedCallback(PublishReceivedCallbackFunc aCallback, void* aContext);

    /**
     * Set callback function invoked when advertise message received from the gateway.
     *
     * @param[in]  aCallback  A function pointer to advertise callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetAdvertiseCallback(AdvertiseCallbackFunc aCallback, void* aContext);

    /**
     * Set callback function invoked when gateway info received from gateway.
     *
     * @param[in]  aCallback  A function pointer to gateway info received callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetSearchGwCallback(SearchGwCallbackFunc aCallback, void* aContext);

    /**
     * Set callback function invoked when disconnect acknowledged or timed out.
     *
     * @param[in]  aCallback  A function pointer to disconnect callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetDisconnectedCallback(DisconnectedCallbackFunc aCallback, void* aContext);

    /**
     * Set callback function invoked when register acknowledged.
     *
     * @param[in]  aCallback  A function pointer to register callback function.
     * @param[in]  aContext   A pointer to context object passed to callback.
     *
     * @retval OT_ERROR_NONE  Callback function successfully set.
     *
     */
    otError SetRegisterReceivedCallback(RegisterReceivedCallbackFunc aCallback, void* aContext);

protected:
    /**
     * Allocate new message with payload.
     *
     * @param[out]  aMessage  A pointer to message pointer.
     * @param[in]   aBuffer   A pointer to payload byte array.
     * @param[in]   aLength   Payload length in bytes.
     *
     * @retval OT_ERROR_NONE      New message successfully created.
     * @retval OT_ERROR_NO_BUFS   Insufficient available buffers to allocate new message.
     *
     */
    otError NewMessage(Message **aMessage, unsigned char* aBuffer, int32_t aLength);

    /**
     * Send OT message to configured gateway address.
     *
     * @param[in]  aMessage  A reference to message instance.
     *
     * @retval OT_ERROR_NONE      Message successfully enqueued.
     * @retval OT_ERROR_NO_BUFS   Insufficient available buffers to enqueue message.
     *
     */
    otError SendMessage(Message &aMessage);

    /**
     * Send OT message to specific gateway address.
     *
     * @param[in]  aMessage  A reference to message instance.
     * @param[in]  aAddress  A reference to target gateway address.
     * @param[in]  aPort     Target gateway port number.
     *
     * @retval OT_ERROR_NONE      Message successfully enqueued.
     * @retval OT_ERROR_NO_BUFS   Insufficient available buffers to enqueue message.
     *
     */
    otError SendMessage(Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort);

    /**
     * Send OT message to specific gateway address with limited hop limit.
     *
     * @param[in]  aMessage   A reference to message instance.
     * @param[in]  aAddress   A reference to target gateway address.
     * @param[in]  aPort      Target gateway port number.
     * @param[in]  aHopLimit  Datagram hop limit.
     *
     * @retval OT_ERROR_NONE      Message successfully enqueued.
     * @retval OT_ERROR_NO_BUFS   Insufficient available buffers to enqueue message.
     *
     */
    otError SendMessage(Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, uint8_t aHopLimit);

    /**
     * Send PINGREQ message to gateway.
     *
     * @retval OT_ERROR_NONE      PINGREQ message successfully enqueued.
     * @retval OT_ERROR_NO_BUFS   Insufficient available buffers to process.
     *
     */
    otError PingGateway(void);

    /**
     * This method should be called after disconnected or lost to configure client internal state and forcing all messages to time out.
     *
     */
    void OnDisconnected(void);

    /**
     * Compare IPv6 address with configured gateway address.
     *
     * @param[in]  aMessageInfo  A reference to message info.
     *
     * @returns  True if the address is equal to gateway address.
     *
     */
    bool VerifyGatewayAddress(const Ip6::MessageInfo &aMessageInfo);

private:
    void ConnackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void SubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PublishReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void AdvertiseReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void GwinfoReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void RegackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void RegisterReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PubrecReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PubrelReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PubcompReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void UnsubackReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PingreqReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void PingrespReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);
    void DisconnectReceived(const Ip6::MessageInfo &messageInfo, const unsigned char* data, uint16_t length);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static void HandleProcessTask(Tasklet &aTasklet);

    static void HandleSubscribeTimeout(const MessageMetadata<otMqttsnSubscribedHandler> &aMetadata, void* aContext);

    static void HandleRegisterTimeout(const MessageMetadata<otMqttsnRegisteredHandler> &aMetadata, void* aContext);

    static void HandleUnsubscribeTimeout(const MessageMetadata<UnsubscribeCallbackFunc> &aMetadata, void* aContext);

    static void HandlePublishQos1Timeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext);

    static void HandlePublishQos2PublishTimeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext);

    static void HandlePublishQos2PubrelTimeout(const MessageMetadata<PublishCallbackFunc> &aMetadata, void* aContext);

    static void HandlePublishQos2PubrecTimeout(const MessageMetadata<void*> &aMetadata, void* aContext);

    static void HandleConnectTimeout(const MessageMetadata<void*> &aMetadata, void* aContext);

    static void HandleDisconnectTimeout(const MessageMetadata<void*> &aMetadata, void* aContext);

    static void HandlePingreqTimeout(const MessageMetadata<void*> &aMetadata, void* aContext);

    static void HandleMessageRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext);

    static void HandlePublishRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext);

    static void HandleSubscribeRetransmission(const Message &aMessage, const Ip6::Address &aAddress, uint16_t aPort, void* aContext);

    Ip6::UdpSocket mSocket;
    MqttsnConfig mConfig;
    uint16_t mMessageId;
    uint32_t mPingReqTime;
    bool mDisconnectRequested;
    bool mSleepRequested;
    bool mTimeoutRaised;
    ClientState mClientState;
    bool mIsRunning;
    Tasklet mProcessTask;
    WaitingMessagesQueue<otMqttsnSubscribedHandler> mSubscribeQueue;
    WaitingMessagesQueue<otMqttsnRegisteredHandler> mRegisterQueue;
    WaitingMessagesQueue<UnsubscribeCallbackFunc> mUnsubscribeQueue;
    WaitingMessagesQueue<PublishCallbackFunc> mPublishQos1Queue;
    WaitingMessagesQueue<PublishCallbackFunc> mPublishQos2PublishQueue;
    WaitingMessagesQueue<PublishCallbackFunc> mPublishQos2PubrelQueue;
    WaitingMessagesQueue<void*> mPublishQos2PubrecQueue;
    WaitingMessagesQueue<void*> mConnectQueue;
    WaitingMessagesQueue<void*> mDisconnectQueue;
    WaitingMessagesQueue<void*> mPingreqQueue;
    otMqttsnConnectedHandler mConnectedCallback;
    void* mConnectContext;
    PublishReceivedCallbackFunc mPublishReceivedCallback;
    void* mPublishReceivedContext;
    AdvertiseCallbackFunc mAdvertiseCallback;
    void* mAdvertiseContext;
    SearchGwCallbackFunc mSearchGwCallback;
    void* mSearchGwContext;
    DisconnectedCallbackFunc mDisconnectedCallback;
    void* mDisconnectedContext;
    RegisterReceivedCallbackFunc mRegisterReceivedCallback;
    void* mRegisterReceivedContext;
};

}
}

#endif /* MQTTSN_CLIENT_HPP_ */
