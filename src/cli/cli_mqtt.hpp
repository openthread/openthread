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

#ifndef CLI_MQTT_HPP_
#define CLI_MQTT_HPP_

#if OPENTHREAD_CONFIG_MQTTSN_ENABLE

#include <openthread/error.h>
#include <openthread/mqttsn.h>

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI CoAP server and client.
 *
 */
class Mqtt
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit Mqtt(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

private:
    struct Command
    {
        const char *mName;
        otError (Mqtt::*mCommand)(int argc, char *argv[]);
    };

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessStart(int argc, char *argv[]);
    otError ProcessStop(int argc, char *argv[]);
    otError ProcessConnect(int argc, char *argv[]);
    otError ProcessSubscribe(int argc, char *argv[]);
    otError ProcessState(int argc, char *argv[]);
    otError ProcessRegister(int argc, char *argv[]);
    otError ProcessPublish(int argc, char *argv[]);
    otError ProcessUnsubscribe(int argc, char *argv[]);
    otError ProcessDisconnect(int argc, char *argv[]);
    otError ProcessSleep(int argc, char *argv[]);
    otError ProcessAwake(int argc, char *argv[]);
    otError ProcessSearchgw(int argc, char *argv[]);

    static void HandleConnected(otMqttsnReturnCode aCode, void *aContext);
    void        HandleConnected(otMqttsnReturnCode aCode);
    static void HandleSubscribed(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, otMqttsnQos aQos, void* aContext);
    void        HandleSubscribed(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, otMqttsnQos aQos);
    static void HandleRegistered(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, void* aContext);
    void        HandleRegistered(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId);
    static void HandlePublished(otMqttsnReturnCode aCode, void* aContext);
    void        HandlePublished(otMqttsnReturnCode aCode);
    static void HandleUnsubscribed(otMqttsnReturnCode aCode, void* aContext);
    void        HandleUnsubscribed(otMqttsnReturnCode aCode);
    static otMqttsnReturnCode HandlePublishReceived(const uint8_t* aPayload, int32_t aPayloadLength, otMqttsnTopicIdType aTopicIdType, otMqttsnTopicId aTopicId, const char* aShortTopicName, void* aContext);
    otMqttsnReturnCode        HandlePublishReceived(const uint8_t* aPayload, int32_t aPayloadLength, otMqttsnTopicIdType aTopicIdType, otMqttsnTopicId aTopicId, const char* aShortTopicName);
    static void HandleDisconnected(otMqttsnDisconnectType aType, void* aContext);
    void        HandleDisconnected(otMqttsnDisconnectType aType);
    static void HandleSearchgwResponse(otIp6Address aAddress, uint8_t aGatewayId, void* aContext);
    void        HandleSearchgwResponse(otIp6Address aAddress, uint8_t aGatewayId);

    void PrintFailedWithCode(const char *aCommandName, otMqttsnReturnCode aCode);

    static const Command sCommands[];
    Interpreter &        mInterpreter;
};

} // namespace Cli
} // namespace ot

#endif

#endif /* CLI_MQTT_HPP_ */
