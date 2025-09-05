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
    Error   error = kErrorNone;
    uint8_t channel;

    VerifyOrExit(aArgsLength == 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], channel));
    VerifyOrExit(IsChannelValid(channel), error = kErrorInvalidArgs);

    otPlatDiagChannelSet(channel);

exit:
    return error;
}

Error Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[])
{
    Error  error = kErrorNone;
    int8_t power;

    VerifyOrExit(aArgsLength == 1, error = kErrorInvalidArgs);

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt8(aArgs[0], power));

    otPlatDiagTxPowerSet(power);

exit:
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
        uint32_t                  i;
        uint32_t                  number;

        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[1], number));
        number = Min(number, static_cast<uint32_t>(kOutputMaxLen));

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
    return error;
}

Error Diags::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    Get<Radio>().SetDiagMode(true);

    return kErrorNone;
}

Error Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    Get<Radio>().SetDiagMode(false);

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
    {"sweep", &Diags::ProcessSweep},
};

Diags::Diags(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxPacket(&Get<Radio>().GetTransmitBuffer())
    , mTxPeriod(0)
    , mTxPackets(0)
    , mChannel(20)
    , mTxPower(0)
    , mTxLen(0)
    , mCurTxCmd(kTxCmdNone)
    , mIsTxPacketSet(false)
    , mIsAsyncSend(false)
    , mDiagSendOn(false)
    , mIsSleepOn(false)
    , mIsAsyncSweep(false)
    , mOutputCallback(nullptr)
    , mOutputContext(nullptr)
{
    mStats.Clear();
}

void Diags::ResetTxPacket(void)
{
    mIsHeaderUpdated                               = false;
    mIsSecurityProcessed                           = false;
    mTxPacket->mInfo.mTxInfo.mTxDelayBaseTime      = 0;
    mTxPacket->mInfo.mTxInfo.mTxDelay              = 0;
    mTxPacket->mInfo.mTxInfo.mMaxCsmaBackoffs      = 0;
    mTxPacket->mInfo.mTxInfo.mMaxFrameRetries      = 0;
    mTxPacket->mInfo.mTxInfo.mRxChannelAfterTxDone = mChannel;
    mTxPacket->mInfo.mTxInfo.mTxPower              = Radio::kInvalidPower;
    mTxPacket->mInfo.mTxInfo.mIsHeaderUpdated      = false;
    mTxPacket->mInfo.mTxInfo.mIsARetx              = false;
    mTxPacket->mInfo.mTxInfo.mCsmaCaEnabled        = false;
    mTxPacket->mInfo.mTxInfo.mCslPresent           = false;
}

Error Diags::ProcessFrame(uint8_t aArgsLength, char *aArgs[])
{
    Error    error                = kErrorNone;
    uint16_t size                 = Radio::kFrameMaxSize;
    bool     securityProcessed    = false;
    bool     csmaCaEnabled        = false;
    bool     isHeaderUpdated      = false;
    int8_t   txPower              = Radio::kInvalidPower;
    uint8_t  maxFrameRetries      = 0;
    uint8_t  maxCsmaBackoffs      = 0;
    uint8_t  rxChannelAfterTxDone = mChannel;
    uint32_t txDelayBaseTime      = 0;
    uint32_t txDelay              = 0;

    while (aArgsLength > 1)
    {
        if (StringMatch(aArgs[0], "-b"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], maxCsmaBackoffs));
        }
        else if (StringMatch(aArgs[0], "-c"))
        {
            csmaCaEnabled = true;
        }
        else if (StringMatch(aArgs[0], "-C"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], rxChannelAfterTxDone));
            VerifyOrExit(IsChannelValid(rxChannelAfterTxDone), error = kErrorInvalidArgs);
        }
        else if (StringMatch(aArgs[0], "-d"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[0], txDelay));
            txDelayBaseTime = static_cast<uint32_t>(otPlatRadioGetNow(&GetInstance()));
        }
        else if (StringMatch(aArgs[0], "-p"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt8(aArgs[0], txPower));
        }
        else if (StringMatch(aArgs[0], "-r"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 1, error = kErrorInvalidArgs);
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], maxFrameRetries));
        }
        else if (StringMatch(aArgs[0], "-s"))
        {
            securityProcessed = true;
            isHeaderUpdated   = true;
        }
        else if (StringMatch(aArgs[0], "-u"))
        {
            isHeaderUpdated = true;
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
    VerifyOrExit(IsFrameLengthValid(size), error = kErrorInvalidArgs);

    ResetTxPacket();
    mTxPacket->mInfo.mTxInfo.mCsmaCaEnabled        = csmaCaEnabled;
    mTxPacket->mInfo.mTxInfo.mTxPower              = txPower;
    mTxPacket->mInfo.mTxInfo.mTxDelayBaseTime      = txDelayBaseTime;
    mTxPacket->mInfo.mTxInfo.mTxDelay              = txDelay;
    mTxPacket->mInfo.mTxInfo.mMaxFrameRetries      = maxFrameRetries;
    mTxPacket->mInfo.mTxInfo.mMaxCsmaBackoffs      = maxCsmaBackoffs;
    mTxPacket->mInfo.mTxInfo.mRxChannelAfterTxDone = rxChannelAfterTxDone;
    mTxPacket->mLength                             = size;
    mIsHeaderUpdated                               = isHeaderUpdated;
    mIsSecurityProcessed                           = securityProcessed;
    mIsTxPacketSet                                 = true;

exit:
    return error;
}

Error Diags::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    if (aArgsLength == 0)
    {
        Output("%u\r\n", mChannel);
    }
    else
    {
        uint8_t channel;

        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], channel));
        VerifyOrExit(IsChannelValid(channel), error = kErrorInvalidArgs);

        mChannel = channel;
        otPlatDiagChannelSet(mChannel);

        if (!mIsSleepOn)
        {
            IgnoreError(Get<Radio>().Receive(mChannel));
        }
    }

exit:
    return error;
}

Error Diags::ProcessPower(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    if (aArgsLength == 0)
    {
        Output("%d\r\n", mTxPower);
    }
    else
    {
        int8_t txPower;

        SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt8(aArgs[0], txPower));

        mTxPower = txPower;
        SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
        otPlatDiagTxPowerSet(mTxPower);
    }

exit:
    return error;
}

Error Diags::ProcessRepeat(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "stop"))
    {
        otPlatAlarmMilliStop(&GetInstance());
        mCurTxCmd = kTxCmdNone;
    }
    else
    {
        uint32_t txPeriod;
        uint16_t txLength;

        VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);
        VerifyOrExit(mCurTxCmd == kTxCmdNone, error = kErrorInvalidState);

        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[0], txPeriod));
        mTxPeriod = txPeriod;

        if (aArgsLength >= 2)
        {
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint16(aArgs[1], txLength));
            mIsTxPacketSet = false;
        }
        else if (mIsTxPacketSet)
        {
            txLength = mTxPacket->mLength;
        }
        else
        {
            ExitNow(error = kErrorInvalidArgs);
        }

        VerifyOrExit(IsFrameLengthValid(txLength), error = kErrorInvalidArgs);

        mTxLen    = txLength;
        mCurTxCmd = kTxCmdRepeat;
        otPlatAlarmMilliStartAt(&GetInstance(), otPlatAlarmMilliGetNow(), mTxPeriod);
    }

exit:
    return error;
}

Error Diags::ProcessSend(uint8_t aArgsLength, char *aArgs[])
{
    Error    error = kErrorNone;
    uint32_t txPackets;
    uint16_t txLength;

    VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);
    VerifyOrExit(mCurTxCmd == kTxCmdNone, error = kErrorInvalidState);

    if (StringMatch(aArgs[0], "async"))
    {
        aArgs++;
        aArgsLength--;
        VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);
        mIsAsyncSend = true;
    }
    else
    {
        mIsAsyncSend = false;
    }

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[0], txPackets));
    mTxPackets = txPackets;

    if (aArgsLength >= 2)
    {
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint16(aArgs[1], txLength));
        mIsTxPacketSet = false;
    }
    else if (mIsTxPacketSet)
    {
        txLength = mTxPacket->mLength;
    }
    else
    {
        ExitNow(error = kErrorInvalidArgs);
    }

    VerifyOrExit(IsFrameLengthValid(txLength), error = kErrorInvalidArgs);
    mTxLen = txLength;

    SuccessOrExit(error = TransmitPacket());
    mCurTxCmd = kTxCmdSend;

    if (!mIsAsyncSend)
    {
        error = kErrorPending;
    }

exit:
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
    Get<Mac::SubMac>().SetRxOnWhenIdle(true);
    otPlatAlarmMilliStop(&GetInstance());
    SuccessOrExit(error = Get<Radio>().Receive(mChannel));
    SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
    Get<Radio>().SetDiagMode(true);
    mStats.Clear();

exit:
    return error;
}

void Diags::OutputStats(void)
{
    Output("received packets: %lu\r\n"
           "sent success packets: %lu\r\n"
           "sent error cca packets: %lu\r\n"
           "sent error abort packets: %lu\r\n"
           "sent error invalid state packets: %lu\r\n"
           "sent error others packets: %lu\r\n"
           "first received packet: rssi=%d, lqi=%u\r\n"
           "last received packet: rssi=%d, lqi=%u\r\n",
           ToUlong(mStats.mReceivedPackets), ToUlong(mStats.mSentSuccessPackets), ToUlong(mStats.mSentErrorCcaPackets),
           ToUlong(mStats.mSentErrorAbortPackets), ToUlong(mStats.mSentErrorInvalidStatePackets),
           ToUlong(mStats.mSentErrorOthersPackets), mStats.mFirstRssi, mStats.mFirstLqi, mStats.mLastRssi,
           mStats.mLastLqi);
}

Error Diags::ProcessStats(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorNone;

    if ((aArgsLength == 1) && StringMatch(aArgs[0], "clear"))
    {
        mStats.Clear();
    }
    else
    {
        VerifyOrExit(aArgsLength == 0, error = kErrorInvalidArgs);
        OutputStats();
    }

exit:
    return error;
}

Error Diags::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otPlatAlarmMilliStop(&GetInstance());
    Get<Radio>().SetDiagMode(false);
    Get<Radio>().SetPromiscuous(false);
    Get<Mac::SubMac>().SetRxOnWhenIdle(false);

    return kErrorNone;
}

Error Diags::ProcessSweep(uint8_t aArgsLength, char *aArgs[])
{
    Error   error = kErrorNone;
    uint8_t txLength;

    VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);
    VerifyOrExit(mCurTxCmd == kTxCmdNone, error = kErrorInvalidState);

    if (StringMatch(aArgs[0], "async"))
    {
        aArgs++;
        aArgsLength--;
        VerifyOrExit(aArgsLength >= 1, error = kErrorInvalidArgs);
        mIsAsyncSweep = true;
    }
    else
    {
        mIsAsyncSweep = false;
    }

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(aArgs[0], txLength));
    VerifyOrExit(IsFrameLengthValid(txLength), error = kErrorInvalidArgs);

    mTxLen         = txLength;
    mIsTxPacketSet = false;

    mChannel = Radio::kChannelMin;
    otPlatDiagChannelSet(mChannel);

    SuccessOrExit(error = TransmitPacket());
    mCurTxCmd = kTxCmdSweep;

    if (!mIsAsyncSweep)
    {
        error = kErrorPending;
    }

exit:
    return error;
}

Error Diags::TransmitPacket(void)
{
    Error error         = kErrorNone;
    mTxPacket->mChannel = mChannel;

    if (mIsTxPacketSet)
    {
        // The `mInfo.mTxInfo.mIsHeaderUpdated` and `mInfo.mTxInfo.mIsSecurityProcessed` fields may be updated by
        // the radio driver after the frame is sent. Here sets these fields field before transmitting the frame.
        mTxPacket->mInfo.mTxInfo.mIsHeaderUpdated     = mIsHeaderUpdated;
        mTxPacket->mInfo.mTxInfo.mIsSecurityProcessed = mIsSecurityProcessed;
    }
    else
    {
        ResetTxPacket();
        mTxPacket->mLength = mTxLen;

        for (uint16_t i = 0; i < mTxLen; i++)
        {
            mTxPacket->mPsdu[i] = static_cast<uint8_t>(i & 0xff);
        }
    }

    error = Get<Radio>().Transmit(*static_cast<Mac::TxFrame *>(mTxPacket));
    if (error == kErrorNone)
    {
        mDiagSendOn = true;
    }
    else
    {
        UpdateTxStats(error);
    }

    return error;
}

Error Diags::ParseReceiveConfigFormat(const char *aFormat, ReceiveConfig &aConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(aFormat != nullptr, error = kErrorInvalidArgs);

    for (const char *arg = aFormat; *arg != '\0'; arg++)
    {
        switch (*arg)
        {
        case 'r':
            aConfig.mShowRssi = true;
            break;

        case 'l':
            aConfig.mShowLqi = true;
            break;

        case 'p':
            aConfig.mShowPsdu = true;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

Error Diags::RadioReceive(void)
{
    Error error;

    SuccessOrExit(error = Get<Radio>().Receive(mChannel));
    SuccessOrExit(error = Get<Radio>().SetTransmitPower(mTxPower));
    otPlatDiagChannelSet(mChannel);
    otPlatDiagTxPowerSet(mTxPower);
    mIsSleepOn = false;

exit:
    return error;
}

Error Diags::ProcessRadio(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

    VerifyOrExit(aArgsLength > 0, error = kErrorInvalidArgs);

    if (StringMatch(aArgs[0], "sleep"))
    {
        SuccessOrExit(error = Get<Radio>().Sleep());
        mIsSleepOn = true;
    }
    else if (StringMatch(aArgs[0], "receive"))
    {
        ReceiveConfig receiveConfig;

        aArgs++;
        aArgsLength--;

        if (aArgsLength == 0)
        {
            SuccessOrExit(error = RadioReceive());
            ExitNow();
        }

        if (StringMatch(aArgs[0], "filter"))
        {
            aArgs++;
            aArgsLength--;

            VerifyOrExit(aArgsLength > 0);

            if (StringMatch(aArgs[0], "enable"))
            {
                mReceiveConfig.mIsFilterEnabled = true;
                error                           = kErrorNone;
            }
            else if (StringMatch(aArgs[0], "disable"))
            {
                mReceiveConfig.mIsFilterEnabled = false;
                error                           = kErrorNone;
            }
            else
            {
                Mac::Address dstAddress;

                if (StringMatch(aArgs[0], "-"))
                {
                    dstAddress.SetNone();
                    error = kErrorNone;
                }
                else if (strlen(aArgs[0]) == 2 * sizeof(Mac::ExtAddress))
                {
                    Mac::ExtAddress extAddress;

                    SuccessOrExit(error = Utils::CmdLineParser::ParseAsHexString(aArgs[0], extAddress.m8));
                    mReceiveConfig.mFilterAddress.SetExtended(extAddress);
                }
                else
                {
                    Mac::ShortAddress shortAddress;

                    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint16(aArgs[0], shortAddress));
                    mReceiveConfig.mFilterAddress.SetShort(shortAddress);
                }
            }

            ExitNow();
        }

        if (StringMatch(aArgs[0], "async"))
        {
            aArgs++;
            aArgsLength--;
            receiveConfig.mIsAsyncCommand = true;
        }

        VerifyOrExit(aArgsLength > 0);
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint16(aArgs[0], receiveConfig.mNumFrames));
        aArgs++;
        aArgsLength--;

        if (aArgsLength > 0)
        {
            SuccessOrExit(error = ParseReceiveConfigFormat(aArgs[0], receiveConfig));
        }

        SuccessOrExit(error = RadioReceive());

        mReceiveConfig.mIsEnabled      = true;
        mReceiveConfig.mIsAsyncCommand = receiveConfig.mIsAsyncCommand;
        mReceiveConfig.mShowRssi       = receiveConfig.mShowRssi;
        mReceiveConfig.mShowLqi        = receiveConfig.mShowLqi;
        mReceiveConfig.mShowPsdu       = receiveConfig.mShowPsdu;
        mReceiveConfig.mReceiveCount   = receiveConfig.mReceiveCount;
        mReceiveConfig.mNumFrames      = receiveConfig.mNumFrames;

        if (!mReceiveConfig.mIsAsyncCommand)
        {
            error = kErrorPending;
        }
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
    else if (StringMatch(aArgs[0], "enable"))
    {
        SuccessOrExit(error = Get<Radio>().Enable());
    }
    else if (StringMatch(aArgs[0], "disable"))
    {
        SuccessOrExit(error = Get<Radio>().Disable());
    }

exit:
    return error;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance) { AsCoreType(aInstance).Get<Diags>().AlarmFired(); }

void Diags::AlarmFired(void)
{
    if (mCurTxCmd == kTxCmdRepeat)
    {
        uint32_t now = otPlatAlarmMilliGetNow();

        IgnoreError(TransmitPacket());
        otPlatAlarmMilliStartAt(&GetInstance(), now, mTxPeriod);
    }
    else
    {
        otPlatDiagAlarmCallback(&GetInstance());
    }
}

void Diags::OutputReceivedFrame(const otRadioFrame *aFrame)
{
    VerifyOrExit(mReceiveConfig.mIsEnabled && (aFrame != nullptr));

    Output("%u", mReceiveConfig.mReceiveCount++);

    if (mReceiveConfig.mShowRssi)
    {
        Output(", rssi:%d", aFrame->mInfo.mRxInfo.mRssi);
    }

    if (mReceiveConfig.mShowLqi)
    {
        Output(", lqi:%u", aFrame->mInfo.mRxInfo.mLqi);
    }

    if (mReceiveConfig.mShowPsdu)
    {
        static constexpr uint16_t kBufSize = 255;
        char                      buf[kBufSize];
        StringWriter              writer(buf, sizeof(buf));

        writer.AppendHexBytes(aFrame->mPsdu, aFrame->mLength);
        Output(", len:%u, psdu:%s", aFrame->mLength, buf);
    }

    Output("\r\n");

    if (mReceiveConfig.mReceiveCount >= mReceiveConfig.mNumFrames)
    {
        mReceiveConfig.mIsEnabled = false;

        if (!mReceiveConfig.mIsAsyncCommand)
        {
            Output("OT_ERROR_NONE");
        }
    }

exit:
    return;
}

void Diags::ReceiveDone(otRadioFrame *aFrame, Error aError)
{
    if (aError == kErrorNone)
    {
        if (mReceiveConfig.mIsFilterEnabled)
        {
            VerifyOrExit(ShouldHandleReceivedFrame(*aFrame));
        }

        OutputReceivedFrame(aFrame);

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

exit:
    return;
}

void Diags::TransmitDone(Error aError)
{
    VerifyOrExit(mDiagSendOn);
    mDiagSendOn = false;

    if (mIsSleepOn)
    {
        IgnoreError(Get<Radio>().Sleep());
    }

    UpdateTxStats(aError);
    VerifyOrExit(((mCurTxCmd == kTxCmdSend) && (mTxPackets > 0)) || (mCurTxCmd == kTxCmdSweep));

    if (mCurTxCmd == kTxCmdSweep)
    {
        if (IsChannelValid(mChannel + 1))
        {
            mChannel += 1;
            otPlatDiagChannelSet(mChannel);

            IgnoreError(TransmitPacket());
        }
        else
        {
            mCurTxCmd = kTxCmdNone;

            if (!mIsAsyncSweep)
            {
                Output("OT_ERROR_NONE");
            }
        }
    }
    else if (mTxPackets > 1)
    {
        mTxPackets--;
        IgnoreError(TransmitPacket());
    }
    else
    {
        mTxPackets = 0;
        mCurTxCmd  = kTxCmdNone;

        if (!mIsAsyncSend)
        {
            Output("OT_ERROR_NONE");
        }
    }

exit:
    return;
}

bool Diags::ShouldHandleReceivedFrame(const otRadioFrame &aFrame) const
{
    bool                ret   = false;
    const Mac::RxFrame &frame = static_cast<const Mac::RxFrame &>(aFrame);
    Mac::Address        dstAddress;

    VerifyOrExit(frame.GetDstAddr(dstAddress) == kErrorNone);
    VerifyOrExit(dstAddress == mReceiveConfig.mFilterAddress);
    ret = true;

exit:
    return ret;
}

void Diags::UpdateTxStats(Error aError)
{
    switch (aError)
    {
    case kErrorNone:
        mStats.mSentSuccessPackets++;
        break;

    case kErrorChannelAccessFailure:
        mStats.mSentErrorCcaPackets++;
        break;

    case kErrorAbort:
        mStats.mSentErrorAbortPackets++;
        break;

    case kErrorInvalidState:
        mStats.mSentErrorInvalidStatePackets++;
        break;

    default:
        mStats.mSentErrorOthersPackets++;
        break;
    }
}

#endif // OPENTHREAD_RADIO

Error Diags::ProcessContinuousWave(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

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
    return error;
}

Error Diags::ProcessStream(uint8_t aArgsLength, char *aArgs[])
{
    Error error = kErrorInvalidArgs;

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
        VerifyOrExit(IsChannelValid(channel), error = kErrorInvalidArgs);

        SuccessOrExit(error = GetPowerSettings(channel, powerSettings));
        Output("TargetPower(0.01dBm): %d\r\nActualPower(0.01dBm): %d\r\nRawPowerSetting: %s\r\n",
               powerSettings.mTargetPower, powerSettings.mActualPower,
               powerSettings.mRawPowerSetting.ToString().AsCString());
    }

exit:
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
    return error;
}

Error Diags::ProcessGpio(uint8_t aArgsLength, char *aArgs[])
{
    Error      error = kErrorInvalidArgs;
    uint32_t   gpio;
    bool       level;
    otGpioMode mode;

    if ((aArgsLength == 2) && StringMatch(aArgs[0], "get"))
    {
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[1], gpio));
        SuccessOrExit(error = otPlatDiagGpioGet(gpio, &level));
        Output("%d\r\n", level);
    }
    else if ((aArgsLength == 3) && StringMatch(aArgs[0], "set"))
    {
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[1], gpio));
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsBool(aArgs[2], level));
        SuccessOrExit(error = otPlatDiagGpioSet(gpio, level));
    }
    else if ((aArgsLength >= 2) && StringMatch(aArgs[0], "mode"))
    {
        SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(aArgs[1], gpio));

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
    return error;
}

bool Diags::IsChannelValid(uint8_t aChannel)
{
    return (aChannel >= Radio::kChannelMin && aChannel <= Radio::kChannelMax);
}

bool Diags::IsFrameLengthValid(uint16_t aLength)
{
    return (aLength >= Radio::kFrameMinSize) && (aLength <= Radio::kFrameMaxSize);
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
        Output("diagnostics mode is %s\r\n", IsEnabled() ? "enabled" : "disabled");
        ExitNow();
    }

    if (!IsEnabled() && !StringMatch(aArgs[0], "start"))
    {
        Output("diagnostics mode is disabled\r\n");
        ExitNow(error = kErrorInvalidState);
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

bool Diags::IsEnabled(void) { return Get<Radio>().GetDiagMode(); }

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
