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

otError otMqttsnConnectDefault(otInstance *aInstance, otIp6Address mAddress, uint16_t mPort)
{
	Instance &instance = *static_cast<Instance *>(aInstance);
	Mqttsn::MqttsnConfig config;
	config.SetAddress(*static_cast<Ip6::Address *>(mAddress));
	config.SetClientId(MQTTSN_DEFAULT_CLIENT_ID);
	config.SetPort(mPort);
	Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
	return client.Connect(config);
}

otError otMqttsnSetConnectedHandler(otInstance *aInstance, otMqttsnConnectedHandler aHandler, void *aContext)
{
	Instance &instance = *static_cast<Instance *>(aInstance);
	Mqttsn::MqttsnClient &client = instance.Get<Mqttsn::MqttsnClient>();
	return client.SetConnectedCallback(aHandler, aContext);
}

const char* otMqttsnReturnCodeToString(otMqttsnReturnCode aCode)
{
	switch (aCode)
	{
	case kCodeAccepted:
		return "Accepted";
	case kCodeRejectedCongestion:
		return "RejectedCongestion";
	case kCodeRejectedNotSupported:
		return "RejectedNotSupported";
	case kCodeRejectedTopicId:
		return "RejectedTopicId";
	case kCodeTimeout:
		return "Timeout";
	default:
		return "Unknown";
	}
}

#endif

