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

#ifndef OPENTHREAD_MQTTSN_H_
#define OPENTHREAD_MQTTSN_H_

#include <openthread/instance.h>
#include <openthread/ip6.h>

#define OT_DEFAULT_MQTTSN_PORT 1883

#ifndef OPENTHREAD_CONFIG_MQTTSN_ENABLE
#define OPENTHREAD_CONFIG_MQTTSN_ENABLE 0
#endif

/**
 * MQTT-SN message return code.
 *
 */
enum otMqttsnReturnCode
{
    kCodeAccepted = 0,
    kCodeRejectedCongestion = 1,
    kCodeRejectedTopicId = 2,
    kCodeRejectedNotSupported = 3,
    /**
     * Pending message timed out. this value is not returned by gateway.
     */
    kCodeTimeout = -1,
};

/**
 * This structure contains MQTT-SN connection parameters.
 *
 */
typedef struct otMqttsnConfig {
	/**
	 * Gateway IPv6 address.
	 */
	otIp6Address mAddress;
	/**
	 * Gateway interface port number.
	 */
    uint16_t mPort;
    /**
     * Client id string.
     */
    char *mClientId;
    /**
     * Keepalive period in seconds.
     */
    uint16_t mKeepAlive;
    /**
     * Clean session flag.
     */
    bool mCleanSession;
    /**
     * Retransmission timeout in milliseconds.
     */
    uint32_t mRetransmissionTimeout;
    /**
     * Retransmission count.
     */
    uint8_t mRetransmissionCount;
} otMqttsnConfig;


/**
 * Declaration of function for connection callback.
 *
 * @param[in]  aCode     CONNACK return code value or -1 when connection establishment timed out.
 * @param[in]  aContext  A pointer to connection callback context object.
 *
 */
typedef void (*otMqttsnConnectedHandler)(otMqttsnReturnCode aCode, void *aContext);

otError otMqttsnStart(otInstance *aInstance, uint16_t aPort);

otError otMqttsnStop(otInstance *aInstance);

otError otMqttsnConnect(otInstance *aInstance, const otMqttsnConfig *aConfig);

otError otMqttsnConnectDefault(otInstance *aInstance, otIp6Address mAddress, uint16_t mPort);

otError otMqttsnSetConnectedHandler(otInstance *aInstance, otMqttsnConnectedHandler aHandler, void *aContext);

const char* otMqttsnReturnCodeToString(otMqttsnReturnCode aCode);

#endif /* OPENTHREAD_MQTTSN_H_ */
