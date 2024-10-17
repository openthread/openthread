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

#if OPENTHREAD_CONFIG_DIAG_ENABLE

#include <stdio.h>
#include <stdlib.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "instance/instance.hpp"
#include "utils/parse_cmdline.hpp"

OT_TOOL_WEAK
otError otPlatDiagProcess(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    OT_UNUSED_VARIABLE(aInstance);

    return ot::kErrorInvalidCommand;
}

namespace ot {
namespace FactoryDiags {

#if OPENTHREAD_RADIO && !OPENTHREAD_RADIO_CLI

const struct Diags::Command Diags::sCommands[] = {
    {"channel", &Diags::ProcessChannel},
    {"cw", &Diags::ProcessContinuousWave},
    {"echo", &Diags::ProcessEcho},
    {"gpio", &Diags::ProcessGpio},
    {"power", &Diags::ProcessPower},
    {"powersettings", &Diags::ProcessPowerSettings},
    {"rawpowersetting", &Diags::ProcessRawPowerSetting},
    {"start", &Diags::ProcessStart},
    {"stop", &Diags::ProcessStop},
    {"stream", &Diags::ProcessStream},
};

Diags::Diags(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mOutputCallback(nullptr)
    , mOutputContext(nullptr)
{
}

Error Diags::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;
    long  value;

    VerifyOrExit(aArgsLength == 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    VerifyOrExit(value >= Radio::kChannelMin && value <= Radio::kChannelMax, error = kErrorInvalidArgs);

    otPlatDiagChannelSet(static_cast<uint8_t>(value));

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;
    long  value;

    VerifyOrExit(aArgsLength == 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = ParseLong(aArgs[0], value));

    otPlatDiagTxPowerSet(static_cast<int8_t>(value));

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessEcho(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    if (aArgsLength == 1)
    {
        Output("%s\r\n", aArgs[0]);
    }
    else if ((aArgsLength == 2) && StringMatch(aArgs[0], "-n"))
    {
        static constexpr uint8_t  kReservedLen  = 1; // 1 byte '\0'
        static constexpr uint16_t kOutputLen    = OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE;
        static constexpr uint16_t kOutputMaxLen = kOutputLen - kReservedLen;
        char                      output[kOutputLen];
        long                      value;
        uint32_t                  i;
        uint32_t                  number;

        SuccessOrExit(error = ParseLong(aArgs[1], value));
        number = Min(static_cast<uint32_t>(value), static_cast<uint32_t>(kOutputMaxLen));

        for (i = 0; i < number; i++)
        {
            output[i] = '0' + i % 10;
        }

        output[number] = '\0';

        Output("%s\r\n", output);
    }
    else
    {
        error = kErrorInvalidArgs;
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otPlatDiagModeSet(true);

    return kErrorNone;
}

Error Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otPlatDiagModeSet(false);

    return kErrorNone;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance) { otPlatDiagAlarmCallback(aInstance); }

#else // OPENTHREAD_RADIO && !OPENTHREAD_RADIO_CLI
// For OPENTHREAD_FTD, OPENTHREAD_MTD, OPENTHREAD_RADIO_CLI
const struct Diags::Command Diags::sCommands[] = {
    {"channel", &Diags::ProcessChannel},
    {"cw", &Diags::ProcessContinuousWave},
    {"frame", &Diags::ProcessFrame},
    {"gpio", &Diags::ProcessGpio},
    {"power", &Diags::ProcessPower},
    {"powersettings", &Diags::ProcessPowerSettings},
    {"rawpowersetting", &Diags::ProcessRawPowerSetting},
    {"radio", &Diags::ProcessRadio},
    {"repeat", &Diags::ProcessRepeat},
    {"send", &Diags::ProcessSend},
    {"start", &Diags::ProcessStart},
    {"stats", &Diags::ProcessStats},
    {"stop", &Diags::ProcessStop},
    {"stream", &Diags::ProcessStream},
};

Diags::Diags(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxPacket(&Get<Radio>().GetTransmitBuffer())
    , mTxPeriod(0)
    , mTxPackets(0)
    , mChannel(20)
    , mTxPower(0)
    , mTxLen(0)
    , mIsTxPacketSet(false)
    , mRepeatActive(false)
    , mDiagSendOn(false)
    , mOutputCallback(nullptr)
    , mOutputContext(nullptr)
{
    mStats.Clear();
}

void Diags::ResetTxPacket(void)
{
    mTxPacket->mInfo.mTxInfo.mTxDelayBaseTime      = 0;
    mTxPacket->mInfo.mTxInfo.mTxDelay              = 0;
    mTxPacket->mInfo.mTxInfo.mMaxCsmaBackoffs      = 0;
    mTxPacket->mInfo.mTxInfo.mMaxFrameRetries      = 0;
    mTxPacket->mInfo.mTxInfo.mRxChannelAfterTxDone = mChannel;
    mTxPacket->mInfo.mTxInfo.mTxPower              = OT_RADIO_POWER_INVALID;
    mTxPacket->mInfo.mTxInfo.mIsHeaderUpdated      = false;
    mTxPacket->mInfo.mTxInfo.mIsARetx              = false;
    mTxPacket->mInfo.mTxInfo.mCsmaCaEnabled        = false;
    mTxPacket->mInfo.mTxInfo.mCslPresent           = false;
    mTxPacket->mInfo.mTxInfo.mIsSecurityProcessed  = false;
}

Error Diags::ProcessFrame(uint8_t aArgsLength, char *aArgs[])
{
    Error    error             = kErrorNone;
    uint16_t size              = OT_RADIO_FRAME_MAX_SIZE;
    bool     securityProcessed = false;
    bool     csmaCaEnabled     = false;
    int8_t   txPower           = OT_RADIO_POWER_INVALID;

    while (aArgsLength > 1)
    {
        if (StringMatch(aArgs[0], "-s"))
        {
            securityProcessed = true;
        }
        else if (StringMatch(aArgs[0], "-p"))
        {
            long value;

            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = ParseLong(aArgs[0], value));
            txPower = static_cast<int8_t>(value);
        }
        else if (StringMatch(aArgs[0], "-c"))
        {
            csmaCaEnabled = true;
        }
        else
        {
            ExitNow(error = kErrorInvalidArgs);
        }

        aArgs++;
        aArgsLength--;
    }

    VerifyOrExit(aArgsLength == 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsHexString(aArgs[0], size, mTxPacket->mPsdu));
    VerifyOrExit(size <= OT_RADIO_FRAME_MAX_SIZE, error = kErrorInvalidArgs);
    VerifyOrExit(size >= OT_RADIO_FRAME_MIN_SIZE, error = kErrorInvalidArgs);

    ResetTxPacket();
    mTxPacket->mInfo.mTxInfo.mCsmaCaEnabled       = csmaCaEnabled;
    mTxPacket->mInfo.mTxInfo.mIsSecurityProcessed = securityProcessed;
    mTxPacket->mInfo.mTxInfo.mTxPower             = txPower;
    mTxPacket->mLength                            = size;
    mIsTxPacketSet                                = true;

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    if (aArgsLength == 0)
    {
        Output("channel: %d\r\n", mChannel);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        VerifyOrExit(value >= Radio::kChannelMin && value <= Radio::kChannelMax, error = kErrorInvalidArgs);

        mChannel = static_cast<uint8_t>(value);
        IgnoreError(Get<Radio>().Receive(mChannel));
        otPlatDiagChannelSet(mChannel);

        Output("set channel to %d\r\nstatus 0x%02x\r\n", mChannel, error);
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    if (aArgsLength == 0)
    {
        Output("tx power: %d dBm\r\n", mTxPower);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));

        mTxPower = static_cast<int8_t>(value);
        SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
        otPlatDiagTxPowerSet(mTxPower);

        Output("set tx power to %d dBm\r\nstatus 0x%02x\r\n", mTxPower, error);
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessRepeat(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);
    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "stop"))
    {
        otPlatAlarmMilliStop(&GetInstance());
        mRepeatActive = false;
        Output("repeated packet transmission is stopped\r\nstatus 0x%02x\r\n", error);
    }
    else
    {
        long value;

        VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        mTxPeriod = static_cast<uint32_t>(value);

        if (aArgsLength >= 2)
        {
            SuccessOrExit(error = ParseLong(aArgs[1], value));
            mIsTxPacketSet = false;
        }
        else if (mIsTxPacketSet)
        {
            value = mTxPacket->mLength;
        }
        else
        {
            ExitNow(error = kErrorInvalidArgs);
        }

        VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = kErrorInvalidArgs);
        VerifyOrExit(value >= OT_RADIO_FRAME_MIN_SIZE, error = kErrorInvalidArgs);
        mTxLen = static_cast<uint8_t>(value);

        mRepeatActive = true;
        uint32_t now  = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(&GetInstance(), now, mTxPeriod);
        Output("sending packets of length %#x at the delay of %#x ms\r\nstatus 0x%02x\r\n", static_cast<int>(mTxLen),
               static_cast<int>(mTxPeriod), error);
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessSend(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;
    long  value;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);
    VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    mTxPackets = static_cast<uint32_t>(value);

    if (aArgsLength >= 2)
    {
        SuccessOrExit(ParseLong(aArgs[1], value));
        mIsTxPacketSet = false;
    }
    else if (mIsTxPacketSet)
    {
        value = mTxPacket->mLength;
    }
    else
    {
        ExitNow(error = kErrorInvalidArgs);
    }

    VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = kErrorInvalidArgs);
    VerifyOrExit(value >= OT_RADIO_FRAME_MIN_SIZE, error = kErrorInvalidArgs);
    mTxLen = static_cast<uint8_t>(value);

    Output("sending %#x packet(s), length %#x\r\nstatus 0x%02x\r\n", static_cast<int>(mTxPackets),
           static_cast<int>(mTxLen), error);
    TransmitPacket();

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    Error error = kErrorNone;

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    VerifyOrExit(!Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);
#endif

    otPlatDiagChannelSet(mChannel);
    otPlatDiagTxPowerSet(mTxPower);

    IgnoreError(Get<Radio>().Enable());
    Get<Radio>().SetPromiscuous(true);
    otPlatAlarmMilliStop(&GetInstance());
    SuccessOrExit(error = Get<Radio>().Receive(mChannel));
    SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
    otPlatDiagModeSet(true);
    mStats.Clear();
    Output("start diagnostics mode\r\nstatus 0x%02x\r\n", error);

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessStats(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    if ((aArgsLength == 1) && StringMatch(aArgs[0], "clear"))
    {
        mStats.Clear();
        Output("stats cleared\r\n");
    }
    else
    {
        VerifyOrExit(aArgsLength == 0, error = kErrorInvalidArgs);
        Output("received packets: %d\r\nsent packets: %d\r\n"
               "first received packet: rssi=%d, lqi=%d\r\n"
               "last received packet: rssi=%d, lqi=%d\r\n",
               static_cast<int>(mStats.mReceivedPackets), static_cast<int>(mStats.mSentPackets),
               static_cast<int>(mStats.mFirstRssi), static_cast<int>(mStats.mFirstLqi),
               static_cast<int>(mStats.mLastRssi), static_cast<int>(mStats.mLastLqi));
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    Error error = kErrorNone;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    otPlatAlarmMilliStop(&GetInstance());
    otPlatDiagModeSet(false);
    Get<Radio>().SetPromiscuous(false);

    Output("received packets: %d\r\nsent packets: %d\r\n"
           "first received packet: rssi=%d, lqi=%d\r\n"
           "last received packet: rssi=%d, lqi=%d\r\n"
           "\nstop diagnostics mode\r\nstatus 0x%02x\r\n",
           static_cast<int>(mStats.mReceivedPackets), static_cast<int>(mStats.mSentPackets),
           static_cast<int>(mStats.mFirstRssi), static_cast<int>(mStats.mFirstLqi), static_cast<int>(mStats.mLastRssi),
           static_cast<int>(mStats.mLastLqi), error);

exit:
    AppendErrorResult(error);
    return error;
}

void Diags::TransmitPacket(void)
{
    mTxPacket->mChannel = mChannel;

    if (!mIsTxPacketSet)
    {
        ResetTxPacket();
        mTxPacket->mLength = mTxLen;

        for (uint8_t i = 0; i < mTxLen; i++)
        {
            mTxPacket->mPsdu[i] = i;
        }
    }

    mDiagSendOn = true;
    IgnoreError(Get<Radio>().Transmit(*static_cast<Mac::TxFrame *>(mTxPacket)));
}

Error Diags::ProcessRadio(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);
    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "sleep"))
    {
        SuccessOrExit(error = Get<Radio>().Sleep());
        Output("set radio from receive to sleep \r\nstatus 0x%02x\r\n", error);
    }
    else if (StringMatch(aArgs[0], "receive"))
    {
        SuccessOrExit(error = Get<Radio>().Receive(mChannel));
        SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
        otPlatDiagChannelSet(mChannel);
        otPlatDiagTxPowerSet(mTxPower);

        Output("set radio from sleep to receive on channel %d\r\nstatus 0x%02x\r\n", mChannel, error);
    }
    else if (StringMatch(aArgs[0], "state"))
    {
        otRadioState state = Get<Radio>().GetState();

        error = kErrorNone;

        switch (state)
        {
        case OT_RADIO_STATE_DISABLED:
            Output("disabled\r\n");
            break;

        case OT_RADIO_STATE_SLEEP:
            Output("sleep\r\n");
            break;

        case OT_RADIO_STATE_RECEIVE:
            Output("receive\r\n");
            break;

        case OT_RADIO_STATE_TRANSMIT:
            Output("transmit\r\n");
            break;

        default:
            Output("invalid\r\n");
            break;
        }
    }

exit:
    AppendErrorResult(error);
    return error;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance) { AsCoreType(aInstance).Get<Diags>().AlarmFired(); }

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

void Diags::ReceiveDone(otRadioFrame *aFrame, Error aError)
{
    if (aError == kErrorNone)
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

void Diags::TransmitDone(Error aError)
{
    VerifyOrExit(mDiagSendOn);
    mDiagSendOn = false;

    if (aError == kErrorNone)
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

    VerifyOrExit(!mRepeatActive);
    TransmitPacket();

exit:
    return;
}

#endif // OPENTHREAD_RADIO

Error Diags::ProcessContinuousWave(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);
    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "start"))
    {
        SuccessOrExit(error = otPlatDiagRadioTransmitCarrier(&GetInstance(), true));
    }
    else if (StringMatch(aArgs[0], "stop"))
    {
        SuccessOrExit(error = otPlatDiagRadioTransmitCarrier(&GetInstance(), false));
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessStream(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);
    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "start"))
    {
        error = otPlatDiagRadioTransmitStream(&GetInstance(), true);
    }
    else if (StringMatch(aArgs[0], "stop"))
    {
        error = otPlatDiagRadioTransmitStream(&GetInstance(), false);
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::GetPowerSettings(uint8_t aChannel, PowerSettings &aPowerSettings)
{
    aPowerSettings.mRawPowerSetting.mLength = RawPowerSetting::kMaxDataSize;
    return otPlatDiagRadioGetPowerSettings(&GetInstance(), aChannel, &aPowerSettings.mTargetPower,
                                           &aPowerSettings.mActualPower, aPowerSettings.mRawPowerSetting.mData,
                                           &aPowerSettings.mRawPowerSetting.mLength);
}

Error Diags::ProcessPowerSettings(uint8_t aArgsLength, char *aArgs[])
{
    Error         error = kErrorInvalidArgs;
    uint8_t       channel;
    PowerSettings powerSettings;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    if (aArgsLength == 0)
    {
        bool          isPrePowerSettingsValid = false;
        uint8_t       preChannel              = 0;
        PowerSettings prePowerSettings;

        Output("| StartCh | EndCh | TargetPower | ActualPower | RawPowerSetting |\r\n"
               "+---------+-------+-------------+-------------+-----------------+\r\n");

        for (channel = Radio::kChannelMin; channel <= Radio::kChannelMax + 1; channel++)
        {
            error = (channel == Radio::kChannelMax + 1) ? kErrorNotFound : GetPowerSettings(channel, powerSettings);

            if (isPrePowerSettingsValid && ((powerSettings != prePowerSettings) || (error != kErrorNone)))
            {
                Output("| %7u | %5u | %11d | %11d | %15s |\r\n", preChannel, channel - 1, prePowerSettings.mTargetPower,
                       prePowerSettings.mActualPower, prePowerSettings.mRawPowerSetting.ToString().AsCString());
                isPrePowerSettingsValid = false;
            }

            if ((error == kErrorNone) && (!isPrePowerSettingsValid))
            {
                preChannel              = channel;
                prePowerSettings        = powerSettings;
                isPrePowerSettingsValid = true;
            }
        }

        error = kErrorNone;
    }
    else if (aArgsLength == 1)
    {
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], channel));
        VerifyOrExit(channel >= Radio::kChannelMin && channel <= Radio::kChannelMax, error = kErrorInvalidArgs);

        SuccessOrExit(error = GetPowerSettings(channel, powerSettings));
        Output("TargetPower(0.01dBm): %d\r\nActualPower(0.01dBm): %d\r\nRawPowerSetting: %s\r\n",
               powerSettings.mTargetPower, powerSettings.mActualPower,
               powerSettings.mRawPowerSetting.ToString().AsCString());
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::GetRawPowerSetting(RawPowerSetting &aRawPowerSetting)
{
    aRawPowerSetting.mLength = RawPowerSetting::kMaxDataSize;
    return otPlatDiagRadioGetRawPowerSetting(&GetInstance(), aRawPowerSetting.mData, &aRawPowerSetting.mLength);
}

Error Diags::ProcessRawPowerSetting(uint8_t aArgsLength, char *aArgs[])
{
    Error           error = kErrorInvalidArgs;
    RawPowerSetting setting;

    VerifyOrExit(otPlatDiagModeGet(), error = kErrorInvalidState);

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = GetRawPowerSetting(setting));
        Output("%s\r\n", setting.ToString().AsCString());
    }
    else if (StringMatch(aArgs[0], "enable"))
    {
        SuccessOrExit(error = otPlatDiagRadioRawPowerSettingEnable(&GetInstance(), true));
    }
    else if (StringMatch(aArgs[0], "disable"))
    {
        SuccessOrExit(error = otPlatDiagRadioRawPowerSettingEnable(&GetInstance(), false));
    }
    else
    {
        setting.mLength = RawPowerSetting::kMaxDataSize;
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsHexString(aArgs[0], setting.mLength, setting.mData));
        SuccessOrExit(error = otPlatDiagRadioSetRawPowerSetting(&GetInstance(), setting.mData, setting.mLength));
    }

exit:
    AppendErrorResult(error);
    return error;
}

Error Diags::ProcessGpio(uint8_t aArgsLength, char *aArgs[])
{
    Error      error = kErrorInvalidArgs;
    long       value;
    uint32_t   gpio;
    bool       level;
    otGpioMode mode;

    if ((aArgsLength == 2) && StringMatch(aArgs[0], "get"))
    {
        SuccessOrExit(error = ParseLong(aArgs[1], value));
        gpio = static_cast<uint32_t>(value);
        SuccessOrExit(error = otPlatDiagGpioGet(gpio, &level));
        Output("%d\r\n", level);
    }
    else if ((aArgsLength == 3) && StringMatch(aArgs[0], "set"))
    {
        SuccessOrExit(error = ParseLong(aArgs[1], value));
        gpio = static_cast<uint32_t>(value);
        SuccessOrExit(error = ParseBool(aArgs[2], level));
        SuccessOrExit(error = otPlatDiagGpioSet(gpio, level));
    }
    else if ((aArgsLength >= 2) && StringMatch(aArgs[0], "mode"))
    {
        SuccessOrExit(error = ParseLong(aArgs[1], value));
        gpio = static_cast<uint32_t>(value);

        if (aArgsLength == 2)
        {
            SuccessOrExit(error = otPlatDiagGpioGetMode(gpio, &mode));
            if (mode == OT_GPIO_MODE_INPUT)
            {
                Output("in\r\n");
            }
            else if (mode == OT_GPIO_MODE_OUTPUT)
            {
                Output("out\r\n");
            }
        }
        else if ((aArgsLength == 3) && StringMatch(aArgs[2], "in"))
        {
            SuccessOrExit(error = otPlatDiagGpioSetMode(gpio, OT_GPIO_MODE_INPUT));
        }
        else if ((aArgsLength == 3) && StringMatch(aArgs[2], "out"))
        {
            SuccessOrExit(error = otPlatDiagGpioSetMode(gpio, OT_GPIO_MODE_OUTPUT));
        }
    }

exit:
    AppendErrorResult(error);
    return error;
}

void Diags::AppendErrorResult(Error aError)
{
    if (aError != kErrorNone)
    {
        Output("failed\r\nstatus %#x\r\n", aError);
    }
}

Error Diags::ParseLong(char *aString, long &aLong)
{
    char *endptr;
    aLong = strtol(aString, &endptr, 0);
    return (*endptr == '\0') ? kErrorNone : kErrorParse;
}

Error Diags::ParseBool(char *aString, bool &aBool)
{
    Error error;
    long  value;

    SuccessOrExit(error = ParseLong(aString, value));
    VerifyOrExit((value == 0) || (value == 1), error = kErrorParse);
    aBool = static_cast<bool>(value);

exit:
    return error;
}

Error Diags::ParseCmd(char *aString, uint8_t &aArgsLength, char *aArgs[])
{
    Error                     error;
    Utils::CmdLineParser::Arg args[kMaxArgs + 1];

    SuccessOrExit(error = Utils::CmdLineParser::ParseCmd(aString, args));
    aArgsLength = Utils::CmdLineParser::Arg::GetArgsLength(args);
    Utils::CmdLineParser::Arg::CopyArgsToStringArray(args, aArgs);

exit:
    return error;
}

Error Diags::ProcessLine(const char *aString)
{
    constexpr uint16_t kMaxCommandBuffer = OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE;

    Error   error = kErrorNone;
    char    buffer[kMaxCommandBuffer];
    char   *args[kMaxArgs];
    uint8_t argCount = 0;

    VerifyOrExit(StringLength(aString, kMaxCommandBuffer) < kMaxCommandBuffer, error = kErrorNoBufs);

    strcpy(buffer, aString);
    error = ParseCmd(buffer, argCount, args);

exit:

    switch (error)
    {
    case kErrorNone:
        error = ProcessCmd(argCount, &args[0]);
        break;

    case kErrorNoBufs:
        Output("failed: command string too long\r\n");
        break;

    case kErrorInvalidArgs:
        Output("failed: command string contains too many arguments\r\n");
        break;

    default:
        Output("failed to parse command string\r\n");
        break;
    }

    return error;
}

Error Diags::ProcessCmd(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    // This `rcp` command is for debugging and testing only, building only when NDEBUG is not defined
    // so that it will be excluded from release build.
#if OPENTHREAD_RADIO && !defined(NDEBUG)
    if (aArgsLength > 0 && StringMatch(aArgs[0], "rcp"))
    {
        aArgs++;
        aArgsLength--;
    }
#endif

    if (aArgsLength == 0)
    {
        Output("diagnostics mode is %s\r\n", otPlatDiagModeGet() ? "enabled" : "disabled");
        ExitNow();
    }

    for (const Command &command : sCommands)
    {
        if (StringMatch(aArgs[0], command.mName))
        {
            error = (this->*command.mCommand)(aArgsLength - 1, (aArgsLength > 1) ? &aArgs[1] : nullptr);
            ExitNow();
        }
    }

    // more platform specific features will be processed under platform layer
    error = otPlatDiagProcess(&GetInstance(), aArgsLength, aArgs);

exit:
    // Add more platform specific diagnostics features here.
    if (error == kErrorInvalidCommand && aArgsLength > 1)
    {
        Output("diag feature '%s' is not supported\r\n", aArgs[0]);
    }

    return error;
}

void Diags::SetOutputCallback(otDiagOutputCallback aCallback, void *aContext)
{
    mOutputCallback = aCallback;
    mOutputContext  = aContext;

    otPlatDiagSetOutputCallback(&GetInstance(), aCallback, aContext);
}

void Diags::Output(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    if (mOutputCallback != nullptr)
    {
        mOutputCallback(aFormat, args, mOutputContext);
    }

    va_end(args);
}

bool Diags::IsEnabled(void) { return otPlatDiagModeGet(); }

} // namespace FactoryDiags
} // namespace ot

OT_TOOL_WEAK otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue)
{
    OT_UNUSED_VARIABLE(aGpio);
    OT_UNUSED_VARIABLE(aValue);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue)
{
    OT_UNUSED_VARIABLE(aGpio);
    OT_UNUSED_VARIABLE(aValue);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode)
{
    OT_UNUSED_VARIABLE(aGpio);
    OT_UNUSED_VARIABLE(aMode);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode)
{
    OT_UNUSED_VARIABLE(aGpio);
    OT_UNUSED_VARIABLE(aMode);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioSetRawPowerSetting(otInstance    *aInstance,
                                                       const uint8_t *aRawPowerSetting,
                                                       uint16_t       aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aRawPowerSetting);
    OT_UNUSED_VARIABLE(aRawPowerSettingLength);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioGetRawPowerSetting(otInstance *aInstance,
                                                       uint8_t    *aRawPowerSetting,
                                                       uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aRawPowerSetting);
    OT_UNUSED_VARIABLE(aRawPowerSettingLength);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioRawPowerSettingEnable(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioTransmitStream(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK otError otPlatDiagRadioGetPowerSettings(otInstance *aInstance,
                                                     uint8_t     aChannel,
                                                     int16_t    *aTargetPower,
                                                     int16_t    *aActualPower,
                                                     uint8_t    *aRawPowerSetting,
                                                     uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aChannel);
    OT_UNUSED_VARIABLE(aTargetPower);
    OT_UNUSED_VARIABLE(aActualPower);
    OT_UNUSED_VARIABLE(aRawPowerSetting);
    OT_UNUSED_VARIABLE(aRawPowerSettingLength);

    return OT_ERROR_NOT_IMPLEMENTED;
}

#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
