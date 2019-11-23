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
#include <openthread/mqttsn.h>
#include "mqttsn/mqttsn_client.hpp"

#if OPENTHREAD_CONFIG_MQTTSN_ENABLE

#define MQTTSN_DEFAULT_CLIENT_ID "openthread"

using namespace ot;

otError otMqttsnStart(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    return instance.Get<Mqttsn::MqttsnClient>().Start(aPort);
}

otError otMqttsnStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    return instance.Get<Mqttsn::MqttsnClient>().Stop();
}

otMqttsnClientState otMqttsnGetState(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    return instance.Get<Mqttsn::MqttsnClient>().GetState();
}

otError otMqttsnConnect(otInstance *aInstance, const otMqttsnConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    if (aConfig == NULL)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    Mqttsn::MqttsnConfig config;
    config.SetAddress(*static_cast<Ip6::Address *>(aConfig->mAddress));
    config.SetCleanSession(aConfig->mCleanSession);
    config.SetClientId(aConfig->mClientId);
    config.SetKeepAlive(aConfig->mKeepAlive);
    config.SetPort(aConfig->mPort);
    config.SetRetransmissionCount(aConfig->mRetransmissionCount);
    config.SetRetransmissionTimeout(aConfig->mRetransmissionTimeout);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Connect(config);
}

otError otMqttsnConnectDefault(otInstance *aInstance, otIp6Address aAddress, uint16_t mPort) {
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnConfig config;
    config.SetAddress(static_cast<Ip6::Address>(aAddress));
    config.SetClientId(MQTTSN_DEFAULT_CLIENT_ID);
    config.SetPort(mPort);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Connect(config);
}

otError otMqttsnSubscribe(otInstance *aInstance, const char *aTopicName, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Subscribe(aTopicName, false, aQos, aHandler, aContext);
}

otError otMqttsnSubscribeShort(otInstance *aInstance, const char *aShortTopicName, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Subscribe(aShortTopicName, true, aQos, aHandler, aContext);
}

otError otMqttsnSubscribeTopicId(otInstance *aInstance, otMqttsnTopicId aTopicId, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Subscribe(aTopicId, aQos, aHandler, aContext);
}

otError otMqttsnRegister(otInstance *aInstance, const char* aTopicName, otMqttsnRegisteredHandler aHandler, void* aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Register(aTopicName, aHandler, aContext);
}

otError otMqttsnPublish(otInstance *aInstance, const uint8_t* aData, int32_t aLength, otMqttsnQos aQos, otMqttsnTopicId aTopicId, otMqttsnPublishedHandler aHandler, void* aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Publish(aData, aLength, aQos, aTopicId, aHandler, aContext);
}

otError otMqttsnPublishShort(otInstance *aInstance, const uint8_t* aData, int32_t aLength, otMqttsnQos aQos, const char* aShortTopicName, otMqttsnPublishedHandler aHandler, void* aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Publish(aData, aLength, aQos, aShortTopicName, aHandler, aContext);
}

otError otMqttsnPublishQosm1(otInstance *aInstance, const uint8_t* aData, int32_t aLength, otMqttsnTopicId aTopicId, otIp6Address aAddress, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.PublishQosm1(aData, aLength, aTopicId, static_cast<Ip6::Address>(aAddress), aPort);
}

otError otMqttsnPublishQosm1Short(otInstance *aInstance, const uint8_t* aData, int32_t aLength, const char* aShortTopicName, otIp6Address aAddress, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.PublishQosm1(aData, aLength, aShortTopicName, static_cast<Ip6::Address>(aAddress), aPort);
}

otError otMqttsnUnsubscribe(otInstance *aInstance, otMqttsnTopicId aTopicId, otMqttsnUnsubscribedHandler aHandler, void* aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Unsubscribe(aTopicId, aHandler, aContext);
}

otError otMqttsnUnsubscribeShort(otInstance *aInstance, const char* aShortTopicName, otMqttsnUnsubscribedHandler aHandler, void* aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.Unsubscribe(aShortTopicName, aHandler, aContext);
}

otError otMqttsnSetConnectedHandler(otInstance *aInstance, otMqttsnConnectedHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
    return client.SetConnectedCallback(aHandler, aContext);
}

otError otMqttsnReturnCodeToString(otMqttsnReturnCode aCode, const char** aCodeString)
{
    switch (aCode)
    {
    case kCodeAccepted:
        *aCodeString = "Accepted";
        break;
    case kCodeRejectedCongestion:
        *aCodeString = "RejectedCongestion";
        break;
    case kCodeRejectedNotSupported:
        *aCodeString = "RejectedNotSupported";
        break;
    case kCodeRejectedTopicId:
        *aCodeString = "RejectedTopicId";
        break;
    case kCodeTimeout:
        *aCodeString = "Timeout";
        break;
    default:
        return OT_ERROR_INVALID_ARGS;
    }
    return OT_ERROR_NONE;
}

otError otMqttsnStringToQos(const char* aQosString, otMqttsnQos *aQos)
{
    if (strcmp(aQosString, "0") == 0)
    {
        *aQos = kQos0;
    }
    else if (strcmp(aQosString, "1") == 0)
    {
        *aQos = kQos1;
    }
    else if (strcmp(aQosString, "2") == 0)
    {
        *aQos = kQos2;
    }
    else if (strcmp(aQosString, "-1") == 0)
    {
        *aQos = kQosm1;
    }
    else
    {
        return OT_ERROR_INVALID_ARGS;
    }
    return OT_ERROR_NONE;
}

otError otMqttsnClientStateToString(otMqttsnClientState aClientState, const char** aClientStateString)
{
    switch (aClientState)
    {
    case kStateDisconnected:
        *aClientStateString = "Disconnected";
        break;
    case kStateActive:
        *aClientStateString = "Active";
        break;
    case kStateAsleep:
        *aClientStateString = "Asleep";
        break;
    case kStateAwake:
        *aClientStateString = "Awake";
        break;
    case kStateLost:
        *aClientStateString = "Lost";
        break;
    default:
        return OT_ERROR_INVALID_ARGS;
    }
    return OT_ERROR_NONE;
}

#endif

