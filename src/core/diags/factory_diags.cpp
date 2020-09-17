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
 *   This file implements the diagnostics module.
 */

#include "factory_diags.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "radio/radio.hpp"
#include "utils/parse_cmdline.hpp"

OT_TOOL_WEAK
otError otPlatDiagProcess(otInstance *aInstance,
                          uint8_t     aArgsLength,
                          char *      aArgs[],
                          char *      aOutput,
                          size_t      aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aOutput);
    OT_UNUSED_VARIABLE(aOutputMaxLen);

    return OT_ERROR_INVALID_COMMAND;
}

namespace ot {
namespace FactoryDiags {

#if OPENTHREAD_CONFIG_DIAG_ENABLE
#if OPENTHREAD_RADIO

const struct Diags::Command Diags::sCommands[] = {
    {"channel", &Diags::ProcessChannel},
    {"power", &Diags::ProcessPower},
    {"start", &Diags::ProcessStart},
    {"stop", &Diags::ProcessStop},
};

Diags::Diags(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

otError Diags::ProcessChannel(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    VerifyOrExit(value >= Radio::kChannelMin && value <= Radio::kChannelMax, error = OT_ERROR_INVALID_ARGS);

    otPlatDiagChannelSet(static_cast<uint8_t>(value));

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgs[0], value));

    otPlatDiagTxPowerSet(static_cast<int8_t>(value));

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessStart(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    OT_UNUSED_VARIABLE(aOutput);
    OT_UNUSED_VARIABLE(aOutputMaxLen);

    otPlatDiagModeSet(true);

    return OT_ERROR_NONE;
}

otError Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    OT_UNUSED_VARIABLE(aOutput);
    OT_UNUSED_VARIABLE(aOutputMaxLen);

    otPlatDiagModeSet(false);

    return OT_ERROR_NONE;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance)
{
    otPlatDiagAlarmCallback(aInstance);
}

#else // OPENTHREAD_RADIO

const struct Diags::Command Diags::sCommands[] = {
    {"channel", &Diags::ProcessChannel}, {"power", &Diags::ProcessPower}, {"radio", &Diags::ProcessRadio},
    {"repeat", &Diags::ProcessRepeat},   {"send", &Diags::ProcessSend},   {"start", &Diags::ProcessStart},
    {"stats", &Diags::ProcessStats},     {"stop", &Diags::ProcessStop},
};

Diags::Diags(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxPacket(&Get<Radio>().GetTransmitBuffer())
    , mTxPeriod(0)
    , mTxPackets(0)
    , mChannel(20)
    , mTxPower(0)
    , mTxLen(0)
    , mRepeatActive(false)
{
    mStats.Clear();
}

otError Diags::ProcessChannel(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgsLength == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "channel: %d\r\n", mChannel);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        VerifyOrExit(value >= Radio::kChannelMin && value <= Radio::kChannelMax, error = OT_ERROR_INVALID_ARGS);

        mChannel = static_cast<uint8_t>(value);
        IgnoreError(Get<Radio>().Receive(mChannel));
        otPlatDiagChannelSet(mChannel);

        snprintf(aOutput, aOutputMaxLen, "set channel to %d\r\nstatus 0x%02x\r\n", mChannel, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgsLength == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "tx power: %d dBm\r\n", mTxPower);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));

        mTxPower = static_cast<int8_t>(value);
        SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
        otPlatDiagTxPowerSet(mTxPower);

        snprintf(aOutput, aOutputMaxLen, "set tx power to %d dBm\r\nstatus 0x%02x\r\n", mTxPower, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessRepeat(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "stop") == 0)
    {
        otPlatAlarmMilliStop(&GetInstance());
        mRepeatActive = false;
        snprintf(aOutput, aOutputMaxLen, "repeated packet transmission is stopped\r\nstatus 0x%02x\r\n", error);
    }
    else
    {
        long value;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        mTxPeriod = static_cast<uint32_t>(value);

        SuccessOrExit(error = ParseLong(aArgs[1], value));
        VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
        mTxLen = static_cast<uint8_t>(value);

        mRepeatActive = true;
        uint32_t now  = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(&GetInstance(), now, mTxPeriod);
        snprintf(aOutput, aOutputMaxLen, "sending packets of length %#x at the delay of %#x ms\r\nstatus 0x%02x\r\n",
                 static_cast<int>(mTxLen), static_cast<int>(mTxPeriod), error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessSend(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    mTxPackets = static_cast<uint32_t>(value);

    SuccessOrExit(error = ParseLong(aArgs[1], value));
    VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
    mTxLen = static_cast<uint8_t>(value);

    snprintf(aOutput, aOutputMaxLen, "sending %#x packet(s), length %#x\r\nstatus 0x%02x\r\n",
             static_cast<int>(mTxPackets), static_cast<int>(mTxLen), error);
    TransmitPacket();

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessStart(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    otPlatDiagChannelSet(mChannel);
    otPlatDiagTxPowerSet(mTxPower);

    IgnoreError(Get<Radio>().Enable());
    Get<Radio>().SetPromiscuous(true);
    otPlatAlarmMilliStop(&GetInstance());
    SuccessOrExit(error = Get<Radio>().Receive(mChannel));
    SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
    otPlatDiagModeSet(true);
    mStats.Clear();
    snprintf(aOutput, aOutputMaxLen, "start diagnostics mode\r\nstatus 0x%02x\r\n", error);

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessStats(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if ((aArgsLength == 1) && (strcmp(aArgs[0], "clear") == 0))
    {
        mStats.Clear();
        snprintf(aOutput, aOutputMaxLen, "stats cleared\r\n");
    }
    else
    {
        VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);
        snprintf(aOutput, aOutputMaxLen,
                 "received packets: %d\r\nsent packets: %d\r\n"
                 "first received packet: rssi=%d, lqi=%d\r\n"
                 "last received packet: rssi=%d, lqi=%d\r\n",
                 static_cast<int>(mStats.mReceivedPackets), static_cast<int>(mStats.mSentPackets),
                 static_cast<int>(mStats.mFirstRssi), static_cast<int>(mStats.mFirstLqi),
                 static_cast<int>(mStats.mLastRssi), static_cast<int>(mStats.mLastLqi));
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

otError Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    otPlatAlarmMilliStop(&GetInstance());
    otPlatDiagModeSet(false);
    Get<Radio>().SetPromiscuous(false);

    snprintf(aOutput, aOutputMaxLen,
             "received packets: %d\r\nsent packets: %d\r\n"
             "first received packet: rssi=%d, lqi=%d\r\n"
             "last received packet: rssi=%d, lqi=%d\r\n"
             "\nstop diagnostics mode\r\nstatus 0x%02x\r\n",
             static_cast<int>(mStats.mReceivedPackets), static_cast<int>(mStats.mSentPackets),
             static_cast<int>(mStats.mFirstRssi), static_cast<int>(mStats.mFirstLqi),
             static_cast<int>(mStats.mLastRssi), static_cast<int>(mStats.mLastLqi), error);

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

void Diags::TransmitPacket(void)
{
    mTxPacket->mLength  = mTxLen;
    mTxPacket->mChannel = mChannel;

    for (uint8_t i = 0; i < mTxLen; i++)
    {
        mTxPacket->mPsdu[i] = i;
    }

    IgnoreError(Get<Radio>().Transmit(*static_cast<Mac::TxFrame *>(mTxPacket)));
}

otError Diags::ProcessRadio(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_INVALID_ARGS;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "sleep") == 0)
    {
        SuccessOrExit(error = Get<Radio>().Sleep());
        snprintf(aOutput, aOutputMaxLen, "set radio from receive to sleep \r\nstatus 0x%02x\r\n", error);
    }
    else if (strcmp(aArgs[0], "receive") == 0)
    {
        SuccessOrExit(error = Get<Radio>().Receive(mChannel));
        SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
        otPlatDiagChannelSet(mChannel);
        otPlatDiagTxPowerSet(mTxPower);

        snprintf(aOutput, aOutputMaxLen, "set radio from sleep to receive on channel %d\r\nstatus 0x%02x\r\n", mChannel,
                 error);
    }
    else if (strcmp(aArgs[0], "state") == 0)
    {
        otRadioState state = Get<Radio>().GetState();

        error = OT_ERROR_NONE;

        switch (state)
        {
        case OT_RADIO_STATE_DISABLED:
            snprintf(aOutput, aOutputMaxLen, "disabled\r\n");
            break;

        case OT_RADIO_STATE_SLEEP:
            snprintf(aOutput, aOutputMaxLen, "sleep\r\n");
            break;

        case OT_RADIO_STATE_RECEIVE:
            snprintf(aOutput, aOutputMaxLen, "receive\r\n");
            break;

        case OT_RADIO_STATE_TRANSMIT:
            snprintf(aOutput, aOutputMaxLen, "transmit\r\n");
            break;

        default:
            snprintf(aOutput, aOutputMaxLen, "invalid\r\n");
            break;
        }
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    instance->Get<Diags>().AlarmFired();
}

void Diags::AlarmFired(void)
{
    if (mRepeatActive)
    {
        uint32_t now = otPlatAlarmMilliGetNow();

        TransmitPacket();
        otPlatAlarmMilliStartAt(&GetInstance(), now, mTxPeriod);
    }
    else
    {
        otPlatDiagAlarmCallback(&GetInstance());
    }
}

void Diags::ReceiveDone(otRadioFrame *aFrame, otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        // for sensitivity test, only record the rssi and lqi for the first and last packet
        if (mStats.mReceivedPackets == 0)
        {
            mStats.mFirstRssi = aFrame->mInfo.mRxInfo.mRssi;
            mStats.mFirstLqi  = aFrame->mInfo.mRxInfo.mLqi;
        }

        mStats.mLastRssi = aFrame->mInfo.mRxInfo.mRssi;
        mStats.mLastLqi  = aFrame->mInfo.mRxInfo.mLqi;

        mStats.mReceivedPackets++;
    }

    otPlatDiagRadioReceived(&GetInstance(), aFrame, aError);
}

void Diags::TransmitDone(otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        mStats.mSentPackets++;

        if (mTxPackets > 1)
        {
            mTxPackets--;
        }
        else
        {
            ExitNow();
        }
    }

    VerifyOrExit(!mRepeatActive, OT_NOOP);
    TransmitPacket();

exit:
    return;
}

#endif // OPENTHREAD_RADIO

void Diags::AppendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen)
{
    if (aError != OT_ERROR_NONE)
    {
        snprintf(aOutput, aOutputMaxLen, "failed\r\nstatus %#x\r\n", aError);
    }
}

otError Diags::ParseLong(char *aString, long &aLong)
{
    char *endptr;
    aLong = strtol(aString, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

void Diags::ProcessLine(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    enum
    {
        kMaxArgs          = OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX,
        kMaxCommandBuffer = OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE,
    };

    otError error = OT_ERROR_NONE;
    char    buffer[kMaxCommandBuffer];
    char *  aArgsector[kMaxArgs];
    uint8_t argCount = 0;

    VerifyOrExit(StringLength(aString, kMaxCommandBuffer) < kMaxCommandBuffer, error = OT_ERROR_NO_BUFS);

    strcpy(buffer, aString);
    error = ot::Utils::CmdLineParser::ParseCmd(buffer, argCount, aArgsector, kMaxArgs);

exit:

    switch (error)
    {
    case OT_ERROR_NONE:
        aOutput[0] = '\0'; // In case there is no output.
        IgnoreError(ProcessCmd(argCount, &aArgsector[0], aOutput, aOutputMaxLen));
        break;

    case OT_ERROR_NO_BUFS:
        snprintf(aOutput, aOutputMaxLen, "failed: command string too long\r\n");
        break;

    case OT_ERROR_INVALID_ARGS:
        snprintf(aOutput, aOutputMaxLen, "failed: command string contains too many arguments\r\n");
        break;

    default:
        snprintf(aOutput, aOutputMaxLen, "failed to parse command string\r\n");
        break;
    }
}

otError Diags::ProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    // This `rcp` command is for debugging and testing only, building only when NDEBUG is not defined
    // so that it will be excluded from release build.
#if !defined(NDEBUG) && defined(OPENTHREAD_RADIO)
    if (aArgsLength > 0 && !strcmp(aArgs[0], "rcp"))
    {
        aArgs++;
        aArgsLength--;
    }
#endif

    if (aArgsLength == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "diagnostics mode is %s\r\n", otPlatDiagModeGet() ? "enabled" : "disabled");
        ExitNow();
    }
    else
    {
        aOutput[0] = '\0';
    }

    for (const Command &command : sCommands)
    {
        if (strcmp(aArgs[0], command.mName) == 0)
        {
            error = (this->*command.mCommand)(aArgsLength - 1, (aArgsLength > 1) ? &aArgs[1] : nullptr, aOutput,
                                              aOutputMaxLen);
            ExitNow();
        }
    }

    // more platform specific features will be processed under platform layer
    error = otPlatDiagProcess(&GetInstance(), aArgsLength, aArgs, aOutput, aOutputMaxLen);

exit:
    // Add more platform specific diagnostics features here.
    if (error == OT_ERROR_INVALID_COMMAND && aArgsLength > 1)
    {
        snprintf(aOutput, aOutputMaxLen, "diag feature '%s' is not supported\r\n", aArgs[0]);
    }

    return error;
}

bool Diags::IsEnabled(void)
{
    return otPlatDiagModeGet();
}

#endif // OPENTHREAD_CONFIG_DIAG_ENABLE

} // namespace FactoryDiags
} // namespace ot
