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

#if OPENTHREAD_CONFIG_MQTTSN_ENABLE

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "common/code_utils.hpp"

#include <openthread/mqttsn.h>

namespace ot {
namespace Cli {

const struct Mqtt::Command Mqtt::sCommands[] = {
    {"help", &Mqtt::ProcessHelp},               {"start", &Mqtt::ProcessStart},
    {"stop", &Mqtt::ProcessStop},               {"connect", &Mqtt::ProcessConnect},
    {"subscribe", &Mqtt::ProcessSubscribe},     {"state", &Mqtt::ProcessState},
    {"register", &Mqtt::ProcessRegister},       {"publish", &Mqtt::ProcessPublish},
    {"unsubscribe", &Mqtt::ProcessUnsubscribe}, {"disconnect", &Mqtt::ProcessDisconnect}
};

Mqtt::Mqtt(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    ;
}

otError Mqtt::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc, argv);
                break;
            }
        }
    }

    return error;
}

otError Mqtt::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Mqtt::ProcessStart(int argc, char *argv[])
{
    otError error;
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    SuccessOrExit(error = otMqttsnSetPublishReceivedHandler(mInterpreter.mInstance, &Mqtt::HandlePublishReceived, this));
    SuccessOrExit(error = otMqttsnStart(mInterpreter.mInstance, OT_DEFAULT_MQTTSN_PORT));
exit:
    return error;
}

otError Mqtt::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otMqttsnStop(mInterpreter.mInstance);
}

otError Mqtt::ProcessConnect(int argc, char *argv[])
{
	otError error;
	otIp6Address destinationIp;
	long destinationPort;

    if (argc != 3)
    {
    	ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = otIp6AddressFromString(argv[1], &destinationIp));
    SuccessOrExit(error = mInterpreter.ParseLong(argv[2], destinationPort));
    if (destinationPort < 1 || destinationPort > 65535)
    {
    	ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = otMqttsnSetConnectedHandler(mInterpreter.mInstance, &Mqtt::HandleConnected, this));
    SuccessOrExit(error = otMqttsnSetDisconnectedHandler(mInterpreter.mInstance, &Mqtt::HandleDisconnected, this));
    SuccessOrExit(error = otMqttsnConnectDefault(mInterpreter.mInstance, destinationIp, (uint16_t)destinationPort));

exit:
	return error;
}

otError Mqtt::ProcessSubscribe(int argc, char *argv[])
{
    otError error;
    char *topicName;
    otMqttsnQos qos = kQos1;
    if (argc < 2 || argc > 3)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    topicName = argv[1];
    if (argc > 2)
    {
        SuccessOrExit(error = otMqttsnStringToQos(argv[2], &qos));
    }
    SuccessOrExit(error = otMqttsnSubscribe(mInterpreter.mInstance, topicName, qos, &HandleSubscribed, this));
exit:
    return error;
}

otError Mqtt::ProcessState(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error;
    otMqttsnClientState clientState;
    const char *clientStateString;
    clientState = otMqttsnGetState(mInterpreter.mInstance);
    SuccessOrExit(error = otMqttsnClientStateToString(clientState, &clientStateString));
    mInterpreter.mServer->OutputFormat("%s\r\n", clientStateString);
exit:
    return error;
}

otError Mqtt::ProcessRegister(int argc, char *argv[])
{
    otError error;
    char *topicName;
    if (argc != 2)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    topicName = argv[1];
    SuccessOrExit(error = otMqttsnRegister(mInterpreter.mInstance, topicName, &HandleRegistered, this));
exit:
    return error;
}

otError Mqtt::ProcessPublish(int argc, char *argv[])
{
    otError error;
    long topicId;
    otMqttsnQos qos = kQos1;
    uint8_t* data = "";
    int32_t length = 0;

    if (argc < 2 || argc > 4)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = mInterpreter.ParseLong(argv[1], topicId));
    if (argc > 2)
    {
        data = static_cast<uint8_t *>(argv[2]);
        length = strlen(argv[2]);
    }
    if (argc > 3)
    {
        SuccessOrExit(error = otMqttsnStringToQos(argv[3], &qos));
    }
    SuccessOrExit(error = otMqttsnPublish(mInterpreter.mInstance, data,
        length, qos, (otMqttsnTopicId)topicId, &Mqtt::HandlePublished, this));
exit:
    return error;
}

otError Mqtt::ProcessUnsubscribe(int argc, char *argv[])
{
    otError error;
    long topicId;

    if (argc != 2)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = mInterpreter.ParseLong(argv[1], topicId));
    SuccessOrExit(error = otMqttsnUnsubscribe(mInterpreter.mInstance, (otMqttsnTopicId)topicId, &Mqtt::HandleUnsubscribed, this));
exit:
    return error;
}

otError Mqtt::ProcessDisconnect(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otMqttsnDisconnect(mInterpreter.mInstance);
}

void Mqtt::HandleConnected(otMqttsnReturnCode aCode, void *aContext)
{
	static_cast<Mqtt *>(aContext)->HandleConnected(aCode);
}

void Mqtt::HandleConnected(otMqttsnReturnCode aCode)
{
	if (aCode == kCodeAccepted)
	{
		mInterpreter.mServer->OutputFormat("connected\r\n");
	}
	else
	{
	    PrintFailedWithCode("connect", aCode);
	}
}

void Mqtt::HandleSubscribed(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, otMqttsnQos aQos, void* aContext)
{
    static_cast<Mqtt *>(aContext)->HandleSubscribed(aCode, aTopicId, aQos);
}

void Mqtt::HandleSubscribed(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, otMqttsnQos aQos)
{
    if (aCode == kCodeAccepted)
    {
        mInterpreter.mServer->OutputFormat("subscribed topic id:%u\r\n", (unsigned int)aTopicId);
    }
    else
    {
        PrintFailedWithCode("subscribe", aCode);
    }
}

void Mqtt::HandleRegistered(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, void* aContext)
{
    static_cast<Mqtt *>(aContext)->HandleRegistered(aCode, aTopicId);
}

void Mqtt::HandleRegistered(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId)
{
    if (aCode == kCodeAccepted)
    {
        mInterpreter.mServer->OutputFormat("registered topic id:%u\r\n", (unsigned int)aTopicId);
    }
    else
    {
        PrintFailedWithCode("register", aCode);
    }
}

void Mqtt::HandlePublished(otMqttsnReturnCode aCode, void* aContext)
{
    static_cast<Mqtt *>(aContext)->HandlePublished(aCode);
}

void Mqtt::HandlePublished(otMqttsnReturnCode aCode)
{
    if (aCode == kCodeAccepted)
    {
        mInterpreter.mServer->OutputFormat("published\r\n");
    }
    else
    {
        PrintFailedWithCode("publish", aCode);
    }
}

void Mqtt::HandleUnsubscribed(otMqttsnReturnCode aCode, void* aContext)
{
    static_cast<Mqtt *>(aContext)->HandleUnsubscribed(aCode);
}

void Mqtt::HandleUnsubscribed(otMqttsnReturnCode aCode)
{
    if (aCode == kCodeAccepted)
    {
        mInterpreter.mServer->OutputFormat("unsubscribed\r\n");
    }
    else
    {
        PrintFailedWithCode("unsubscribe", aCode);
    }
}

otMqttsnReturnCode Mqtt::HandlePublishReceived(const uint8_t* aPayload, int32_t aPayloadLength, otMqttsnTopicIdType aTopicIdType, otMqttsnTopicId aTopicId, const char* aShortTopicName, void* aContext)
{
    return static_cast<Mqtt *>(aContext)->HandlePublishReceived(aPayload, aPayloadLength, aTopicIdType, aTopicId, aShortTopicName);
}

otMqttsnReturnCode Mqtt::HandlePublishReceived(const uint8_t* aPayload, int32_t aPayloadLength, otMqttsnTopicIdType aTopicIdType, otMqttsnTopicId aTopicId, const char* aShortTopicName)
{
    if (aTopicIdType == kTopicId)
    {
        mInterpreter.mServer->OutputFormat("received publish from topic id %u:\r\n", (unsigned int)aTopicId);
    }
    else if (aTopicIdType == kShortTopicName)
    {
        mInterpreter.mServer->OutputFormat("received publish from topic %s:\r\n", aShortTopicName);
    }
    mInterpreter.mServer->OutputFormat("%.*s\r\n", aPayloadLength, aPayload);
    return kCodeAccepted;
}

void Mqtt::HandleDisconnected(otMqttsnDisconnectType aType, void* aContext)
{
    static_cast<Mqtt *>(aContext)->HandleDisconnected(aType);
}

void Mqtt::HandleDisconnected(otMqttsnDisconnectType aType)
{
    const char* disconnectTypeText;
    if (otMqttsnDisconnectTypeToString(aType, &disconnectTypeText) == OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("disconnected reason: %s\r\n", disconnectTypeText);
    }
    else
    {
        mInterpreter.mServer->OutputFormat("disconnected with unknown reason: %d\r\n", aType);
    }
}

void Mqtt::PrintFailedWithCode(const char *aCommandName, otMqttsnReturnCode aCode)
{
    const char* codeText;
    if (otMqttsnReturnCodeToString(aCode, &codeText) == OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("%s failed: %s\r\n", aCommandName, codeText);
    }
    else
    {
        mInterpreter.mServer->OutputFormat("%s failed with unknown code: %d\r\n", aCommandName, aCode);
    }
}

} // namespace Cli
} // namespace ot

#endif
