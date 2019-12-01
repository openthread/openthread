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

#ifndef MQTTSN_SERIALIZER_HPP_
#define MQTTSN_SERIALIZER_HPP_

#include <stdint.h>

#include "net/ip6_address.hpp"
#include "mqttsn_client.hpp"

/**
 * @file
 *   This file includes declaration of MQTT-SN protocol v1.2 messages serializers.
 *
 */

namespace ot {

namespace Mqttsn {

enum MessageType
{
    kTypeAdvertise,
    kTypeSearchGw,
    kTypeGwInfo,
    kTypeReserved1,
    kTypeConnect,
    kTypeConnack,
    kTypeWillTopicReq,
    kTypeWillTopic,
    kTypeWillMsqReq,
    kTypeWillMsg,
    kTypeRegister,
    kTypeRegack,
    kTypePublish,
    kTypePuback,
    kTypePubcomp,
    kTypePubrec,
    kTypePubrel,
    kTypeReserved2,
    kTypeSubscribe,
    kTypeSuback,
    kTypeUnsubscribe,
    kTypeUnsuback,
    kTypePingreq,
    kTypePingresp,
    kTypeDisconnect,
    kTypeReserved3,
    kTypeWillTopicUpd,
    kTypeWillTopicResp,
    kTypeWillMsqUpd,
    kTypeWillMsgResp,
    kTypeEncapsulated = 0xfe
};

class MessageBase
{
protected:
    MessageBase(MessageType aMessageType)
        : mMessageType(aMessageType)
    {
        ;
    }

public:
    MessageType GetMessageType() { return mMessageType; };

    void SetMessageType(MessageType aMessageType) { mMessageType = aMessageType; };

    virtual otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const = 0;

    virtual otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength) = 0;

    static otError DeserializeMessageType(const uint8_t* aBuffer, int32_t aBufferLength, MessageType* aMessageType);

private:
    MessageType mMessageType;
};

class AdvertiseMessage : public MessageBase
{
public:
    AdvertiseMessage()
        : MessageBase(kTypeAdvertise)
    {
        ;
    }

    AdvertiseMessage (uint8_t aGatewayId, uint16_t aDuration)
        : MessageBase(kTypeAdvertise)
        , mGatewayId(aGatewayId)
        , mDuration(aDuration)
    {
        ;
    }

    uint8_t GetGatewayId() const { return mGatewayId; }

    void SetGatewayId(uint8_t aGatewayId) { mGatewayId = aGatewayId; }

    uint16_t GetDuration() const { return mDuration; }

    void SetDuration(uint16_t aDuration) { mDuration = aDuration; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint8_t mGatewayId;
    uint16_t mDuration;
};

class SearchGwMessage : public MessageBase
{
public:
    SearchGwMessage()
        : MessageBase(kTypeSearchGw)
    {
        ;
    }

    SearchGwMessage (uint8_t aRadius)
        : MessageBase(kTypeSearchGw)
        , mRadius(aRadius)
    {
        ;
    }

    uint8_t GetRadius() const { return mRadius; }

    void SetRadius(uint8_t aRadius) { mRadius = aRadius; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint8_t mRadius;
};

class GwInfoMessage : public MessageBase
{
public:
    GwInfoMessage()
        : MessageBase(kTypeGwInfo)
    {
        ;
    }

    GwInfoMessage (uint8_t aGatewayId, bool aHasAddress, const Ip6::Address &aAddress)
        : MessageBase(kTypeGwInfo)
        , mGatewayId(aGatewayId)
        , mHasAddress(aHasAddress)
        , mAddress(aAddress)
    {
        ;
    }

    uint8_t GetGatewayId() const { return mGatewayId; }

    void SetGatewayId(uint8_t aGatewayId) { mGatewayId = aGatewayId; }

    bool GetHasAddress() { return mHasAddress; }

    void SetHasAddress(bool aHasAddress) { mHasAddress = aHasAddress; }

    const Ip6::Address &GetAddress() const { return mAddress; }

    void SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint8_t mGatewayId;
    bool mHasAddress;
    Ip6::Address mAddress;
};

class ConnectMessage : public MessageBase
{
public:
    ConnectMessage()
        : MessageBase(kTypeConnect)
    {
        ;
    }

    ConnectMessage (bool aCleanSessionFlag, bool aWillFlag, uint16_t aDuration, const char* aClientId)
        : MessageBase(kTypeConnect)
        , mCleanSessionFlag(aCleanSessionFlag)
        , mWillFlag(aWillFlag)
        , mDuration(aDuration)
        , mClientId("%s", aClientId)
    {
        ;
    }

    bool GetCleanSessionFlag() const { return mCleanSessionFlag; }

    void SetCleanSessionFlag(bool aCleanSessionFlag) { mCleanSessionFlag = aCleanSessionFlag; }

    bool GetWillFlag() const { return mWillFlag; }

    void SetWillFlag(bool aWillFlag) { mWillFlag = aWillFlag; }

    uint16_t GetDuration() const { return mDuration; }

    void SetDuration(uint16_t aDuration) { mDuration = aDuration; }

    const ClientIdString &GetClientId() const { return mClientId; }

    void SetClientId(const char* aClientId) { mClientId.Set("%s", aClientId); }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    bool mCleanSessionFlag;
    bool mWillFlag;
    uint16_t mDuration;
    ClientIdString mClientId;
};

class ConnackMessage : public MessageBase
{
public:
    ConnackMessage()
        : MessageBase(kTypeConnack)
    {
        ;
    }

    ConnackMessage (ReturnCode aReturnCode)
        : MessageBase(kTypeConnack)
        , mReturnCode(aReturnCode)
    {
        ;
    }

    ReturnCode GetReturnCode() const { return mReturnCode; }

    void SetReturnCode(ReturnCode aReturnCode) { mReturnCode = aReturnCode; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    ReturnCode mReturnCode;
};

class RegisterMessage : public MessageBase
{
public:
    RegisterMessage()
        : MessageBase(kTypeRegister)
    {
        ;
    }

    RegisterMessage (TopicId aTopicId, uint16_t aMessageId, const char* aTopicName)
        : MessageBase(kTypeRegister)
        , mTopicId(aTopicId)
        , mMessageId(aMessageId)
        , mTopicName("%s", aTopicName)
    {
        ;
    }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    const TopicNameString &GetTopicName() const { return mTopicName; }

    void SetTopicName(const char* aTopicName) { mTopicName.Set("%s", aTopicName); }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    TopicId mTopicId;
    uint16_t mMessageId;
    TopicNameString mTopicName;
};

class RegackMessage : public MessageBase
{
public:
    RegackMessage()
        : MessageBase(kTypeRegack)
    {
        ;
    }

    RegackMessage (ReturnCode aReturnCode, TopicId aTopicId, uint16_t aMessageId)
        : MessageBase(kTypeRegack)
        , mReturnCode(aReturnCode)
        , mTopicId(aTopicId)
        , mMessageId(aMessageId)
    {
        ;
    }

    ReturnCode GetReturnCode() const { return mReturnCode; }

    void SetReturnCode(ReturnCode aReturnCode) { mReturnCode = aReturnCode; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    ReturnCode mReturnCode;
    TopicId mTopicId;
    uint16_t mMessageId;
};

class PublishMessage : public MessageBase
{
public:
    PublishMessage()
        : MessageBase(kTypePublish)
    {
        ;
    }

    PublishMessage(bool aDupFlag, bool aRetainedFlag, Qos aQos, uint16_t aMessageId, TopicIdType aTopicIdType, TopicId aTopicId, const char* aShortTopicName, const uint8_t* aPayload, int32_t aPayloadLength)
        : MessageBase(kTypePublish)
        , mDupFlag(aDupFlag)
        , mRetainedFlag(aRetainedFlag)
        , mQos(aQos)
        , mMessageId(aMessageId)
        , mTopicIdType(aTopicIdType)
        , mTopicId(aTopicId)
        , mShortTopicName("%s", aShortTopicName)
        , mPayload(aPayload)
        , mPayloadLength(aPayloadLength)
    {
        ;
    }

    bool GetDupFlag() const { return mDupFlag; }

    void SetDupFlag(bool aDupFlag) { mDupFlag = aDupFlag; }

    bool GetRetainedFlag() { return mRetainedFlag; }

    void SetRetainedFlag(bool aRetainedFlag) { mRetainedFlag = aRetainedFlag; }

    Qos GetQos() const { return mQos; }

    void SetQos(Qos qos) { mQos = qos; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    TopicIdType GetTopicIdType() const { return mTopicIdType; }

    void SetTopicIdType(TopicIdType aTopicIdType) { mTopicIdType = aTopicIdType; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    ShortTopicNameString GetShortTopicName() const { return mShortTopicName; }

    void SetShortTopicName(const char* aShortTopicName) { mShortTopicName.Set("%s", aShortTopicName); }

    const uint8_t* GetPayload() const { return mPayload; }

    void SetPayload(const uint8_t* aPayload) { mPayload = aPayload; }

    int32_t GetPayloadLength() { return mPayloadLength; }

    void SetPayloadLenghth(int32_t aPayloadLenght) { mPayloadLength = aPayloadLenght; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    bool mDupFlag;
    bool mRetainedFlag;
    Qos mQos;
    uint16_t mMessageId;
    TopicIdType mTopicIdType;
    TopicId mTopicId;
    ShortTopicNameString mShortTopicName;
    const uint8_t* mPayload;
    int32_t mPayloadLength;
};

class PubackMessage : public MessageBase
{
public:
    PubackMessage()
        : MessageBase(kTypePuback)
    {
        ;
    }

    PubackMessage (ReturnCode aReturnCode, TopicId aTopicId, uint16_t aMessageId)
        : MessageBase(kTypePuback)
        , mReturnCode(aReturnCode)
        , mTopicId(aTopicId)
        , mMessageId(aMessageId)
    {
        ;
    }

    ReturnCode GetReturnCode() const { return mReturnCode; }

    void SetReturnCode(ReturnCode aReturnCode) { mReturnCode = aReturnCode; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    ReturnCode mReturnCode;
    TopicId mTopicId;
    uint16_t mMessageId;
};

class PubcompMessage : public MessageBase
{
public:
    PubcompMessage()
        : MessageBase(kTypePubcomp)
    {
        ;
    }

    PubcompMessage (uint16_t aMessageId)
        : MessageBase(kTypePubcomp)
        , mMessageId(aMessageId)
    {
        ;
    }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mMessageId;
};

class PubrecMessage : public MessageBase
{
public:
    PubrecMessage()
        : MessageBase(kTypePubrec)
    {
        ;
    }

    PubrecMessage (uint16_t aMessageId)
        : MessageBase(kTypePubrec)
        , mMessageId(aMessageId)
    {
        ;
    }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mMessageId;
};

class PubrelMessage : public MessageBase
{
public:
    PubrelMessage()
        : MessageBase(kTypePubrel)
    {
        ;
    }

    PubrelMessage (uint16_t aMessageId)
        : MessageBase(kTypePubrel)
        , mMessageId(aMessageId)
    {
        ;
    }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mMessageId;
};

class SubscribeMessage : public MessageBase
{
public:
    SubscribeMessage()
        : MessageBase(kTypeSubscribe)
    {
        ;
    }

    SubscribeMessage(bool aDupFlag, Qos aQos, uint16_t aMessageId, TopicIdType aTopicIdType, TopicId aTopicId, const char* aShortTopicName, const char* aTopicName)
        : MessageBase(kTypeSubscribe)
        , mDupFlag(aDupFlag)
        , mQos(aQos)
        , mMessageId(aMessageId)
        , mTopicIdType(aTopicIdType)
        , mTopicId(aTopicId)
        , mShortTopicName("%s", aShortTopicName)
        , mTopicName("%s", aTopicName)
    {
        ;
    }

    bool GetDupFlag() const { return mDupFlag; }

    void SetDupFlag(bool aDupFlag) { mDupFlag = aDupFlag; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    TopicIdType GetTopicIdType() const { return mTopicIdType; }

    void SetTopicIdType(TopicIdType aTopicIdType) { mTopicIdType = aTopicIdType; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    const ShortTopicNameString &GetShortTopicName() const { return mShortTopicName; }

    void SetShortTopicName(const char* aShortTopicName) { mShortTopicName.Set("%s", aShortTopicName); }

    const TopicNameString &GetTopicName() const { return mTopicName; }

    void SetTopicName(const char* aTopicName) { mTopicName.Set("%s", aTopicName); }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    bool mDupFlag;
    Qos mQos;
    uint16_t mMessageId;
    TopicIdType mTopicIdType;
    TopicId mTopicId;
    ShortTopicNameString mShortTopicName;
    TopicNameString mTopicName;
};

class SubackMessage : public MessageBase
{
public:
    SubackMessage()
        : MessageBase(kTypeSuback)
    {
        ;
    }

    SubackMessage (ReturnCode aReturnCode, TopicId aTopicId, uint16_t aMessageId)
        : MessageBase(kTypeSuback)
        , mReturnCode(aReturnCode)
        , mTopicId(aTopicId)
        , mMessageId(aMessageId)
    {
        ;
    }

    ReturnCode GetReturnCode() const { return mReturnCode; }

    void SetReturnCode(ReturnCode aReturnCode) { mReturnCode = aReturnCode; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    Qos GetQos() const { return mQos; }

    void SetQos(Qos aQos) { mQos = aQos; }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    ReturnCode mReturnCode;
    TopicId mTopicId;
    Qos mQos;
    uint16_t mMessageId;
};

class UnsubscribeMessage : public MessageBase
{
public:
    UnsubscribeMessage()
        : MessageBase(kTypeUnsubscribe)
    {
        ;
    }

    UnsubscribeMessage (uint16_t aMessageId, TopicIdType aTopicIdType, TopicId aTopicId, const char* aShortTopicName, const char* aTopicName)
        : MessageBase(kTypeUnsubscribe)
        , mMessageId(aMessageId)
        , mTopicIdType(aTopicIdType)
        , mTopicId(aTopicId)
        , mShortTopicName("%s", aShortTopicName)
        , mTopicName("%s", aTopicName)
    {
        ;
    }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    TopicIdType GetTopicIdType() const { return mTopicIdType; }

    void SetTopicIdType(TopicIdType aTopicIdType) { mTopicIdType = aTopicIdType; }

    TopicId GetTopicId() const { return mTopicId; }

    void SetTopicId(TopicId aTopicId) { mTopicId = aTopicId; }

    ShortTopicNameString GetShortTopicName() { return mShortTopicName; }

    void SetShortTopicName(const char* aShortTopicName) { mShortTopicName.Set("%s", aShortTopicName); }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mMessageId;
    TopicIdType mTopicIdType;
    TopicId mTopicId;
    ShortTopicNameString mShortTopicName;
    TopicNameString mTopicName;
};

class UnsubackMessage : public MessageBase
{
public:
    UnsubackMessage()
        : MessageBase(kTypeUnsuback)
    {
        ;
    }

    UnsubackMessage (uint16_t aMessageId)
        : MessageBase(kTypeUnsuback)
        , mMessageId(aMessageId)
    {
        ;
    }

    uint16_t GetMessageId() const { return mMessageId; }

    void SetMessageId(uint16_t aMessageId) { mMessageId = aMessageId; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mMessageId;
};

class PingreqMessage : public MessageBase
{
public:
    PingreqMessage()
        : MessageBase(kTypePingreq)
    {
        ;
    }

    PingreqMessage (const char* aClientId)
        : MessageBase(kTypePingreq)
        , mClientId("%s", aClientId)
    {
        ;
    }

    const ClientIdString &GetClientId() const { return mClientId; }

    void SetClientId(const char* aClientId) { mClientId.Set("%s", aClientId); }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    ClientIdString mClientId;
};

class PingrespMessage : public MessageBase
{
public:
    PingrespMessage()
        : MessageBase(kTypePingresp)
    {
        ;
    }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);
};

class DisconnectMessage : public MessageBase
{
public:
    DisconnectMessage()
        : MessageBase(kTypeDisconnect)
    {
        ;
    }

    DisconnectMessage (uint16_t aDuration)
        : MessageBase(kTypeDisconnect)
        , mDuration(aDuration)
    {
        ;
    }

    uint16_t GetDuration() const { return mDuration; }

    void SetDuration(uint16_t aDuration) { mDuration = aDuration; }

    otError Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const;

    otError Deserialize(const uint8_t* aBuffer, int32_t aBufferLength);

private:
    uint16_t mDuration;
};

}

}

#endif /* MQTTSN_SERIALIZER_HPP_ */
