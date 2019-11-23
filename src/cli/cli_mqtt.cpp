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
    {"help", &Mqtt::ProcessHelp},           {"start", &Mqtt::ProcessStart},
    {"stop", &Mqtt::ProcessStop},           {"connect", &Mqtt::ProcessConnect},
    {"subscribe", &Mqtt::ProcessSubscribe}, {"state", &Mqtt::ProcessState}
};

Mqtt::Mqtt(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    ;
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
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otMqttsnStart(mInterpreter.mInstance, OT_DEFAULT_MQTTSN_PORT);
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

    if (argc != 2)
    {
    	ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = otIp6AddressFromString(argv[0], &destinationIp));
    SuccessOrExit(error = mInterpreter.ParseLong(argv[0], destinationPort));
    if (destinationPort < 1 || destinationPort > 65535)
    {
    	ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    SuccessOrExit(error = otMqttsnSetConnectedHandler(mInterpreter.mInstance, &Mqtt::HandleConnected, this));
    SuccessOrExit(error = otMqttsnConnectDefault(mInterpreter.mInstance, destinationIp, (uint16_t)destinationPort));

exit:
	return error;
}

otError Mqtt::ProcessSubscribe(int argc, char *argv[])
{
    otError error;
    char *topicName;
    otMqttsnQos qos = kQos1;
    if (argc < 1 || argc > 2)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    topicName = argv[0];
    if (argc > 1)
    {
        SuccessOrExit(error = otMqttsnStringToQos(argv[1], &qos));
    }
    SuccessOrExit(error = otMqttsnSubscribe(mInterpreter.mInstance, topicName, qos, &HandleSubscribed, this));
exit:
    return error;
}

otError Mqtt::ProcessState(int argc, char *argv[])
{
    otError error;
    otMqttsnClientState clientState;
    const char *clientStateString;
    if (argc != 0)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
    clientState = otMqttsnGetState(mInterpreter.mInstance);
    SuccessOrExit(error = otMqttsnClientStateToString(clientState, &clientStateString));
    mInterpreter.mServer->OutputFormat("%s\r\n", clientStateString);
exit:
    return error;
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
