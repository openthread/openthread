/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the CLI interpreter for Ping Sender function.
 */

#include "cli_ping.hpp"

#include <openthread/ping_sender.h>

#include "cli/cli.hpp"
#include "cli/cli_utils.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE

namespace ot {
namespace Cli {

PingSender::PingSender(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
    , mPingIsAsync(false)
{
}

otError PingSender::Process(Arg aArgs[])
{
    otError            error = OT_ERROR_NONE;
    otPingSenderConfig config;
    bool               async = false;
    bool               nat64Synth;

    /**
     * @cli ping stop
     * @code
     * ping stop
     * Done
     * @endcode
     * @par
     * Stop sending ICMPv6 Echo Requests.
     * @sa otPingSenderStop
     */
    if (aArgs[0] == "stop")
    {
        otPingSenderStop(GetInstancePtr());
        ExitNow();
    }
    else if (aArgs[0] == "async")
    {
        async = true;
        aArgs++;
    }

    ClearAllBytes(config);

    if (aArgs[0] == "-I")
    {
        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(config.mSource));

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        VerifyOrExit(otIp6HasUnicastAddress(GetInstancePtr(), &config.mSource), error = OT_ERROR_INVALID_ARGS);
#endif
        aArgs += 2;
    }

    if (aArgs[0] == "-m")
    {
        config.mMulticastLoop = true;
        aArgs++;
    }

    SuccessOrExit(error = ParseToIp6Address(GetInstancePtr(), aArgs[0], config.mDestination, nat64Synth));

    if (nat64Synth)
    {
        OutputFormat("Pinging synthesized IPv6 address: ");
        OutputIp6AddressLine(config.mDestination);
    }

    if (!aArgs[1].IsEmpty())
    {
        SuccessOrExit(error = aArgs[1].ParseAsUint16(config.mSize));
    }

    if (!aArgs[2].IsEmpty())
    {
        SuccessOrExit(error = aArgs[2].ParseAsUint16(config.mCount));
    }

    if (!aArgs[3].IsEmpty())
    {
        SuccessOrExit(error = ParsePingInterval(aArgs[3], config.mInterval));
    }

    if (!aArgs[4].IsEmpty())
    {
        SuccessOrExit(error = aArgs[4].ParseAsUint8(config.mHopLimit));
        config.mAllowZeroHopLimit = (config.mHopLimit == 0);
    }

    if (!aArgs[5].IsEmpty())
    {
        uint32_t timeout;

        SuccessOrExit(error = ParsePingInterval(aArgs[5], timeout));
        VerifyOrExit(timeout <= NumericLimits<uint16_t>::kMax, error = OT_ERROR_INVALID_ARGS);
        config.mTimeout = static_cast<uint16_t>(timeout);
    }

    VerifyOrExit(aArgs[6].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    config.mReplyCallback      = PingSender::HandlePingReply;
    config.mStatisticsCallback = PingSender::HandlePingStatistics;
    config.mCallbackContext    = this;

    SuccessOrExit(error = otPingSenderPing(GetInstancePtr(), &config));

    mPingIsAsync = async;

    if (!async)
    {
        error = OT_ERROR_PENDING;
    }

exit:
    return error;
}

otError PingSender::ParsePingInterval(const Arg &aArg, uint32_t &aInterval)
{
    otError        error    = OT_ERROR_NONE;
    const char    *string   = aArg.GetCString();
    const uint32_t msFactor = 1000;
    uint32_t       factor   = msFactor;

    aInterval = 0;

    while (*string)
    {
        if ('0' <= *string && *string <= '9')
        {
            // In the case of seconds, change the base of already calculated value.
            if (factor == msFactor)
            {
                aInterval *= 10;
            }

            aInterval += static_cast<uint32_t>(*string - '0') * factor;

            // In the case of milliseconds, change the multiplier factor.
            if (factor != msFactor)
            {
                factor /= 10;
            }
        }
        else if (*string == '.')
        {
            // Accept only one dot character.
            VerifyOrExit(factor == msFactor, error = OT_ERROR_INVALID_ARGS);

            // Start analyzing hundreds of milliseconds.
            factor /= 10;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        string++;
    }

exit:
    return error;
}

void PingSender::HandlePingReply(const otPingSenderReply *aReply, void *aContext)
{
    static_cast<PingSender *>(aContext)->HandlePingReply(aReply);
}

void PingSender::HandlePingReply(const otPingSenderReply *aReply)
{
    OutputFormat("%u bytes from ", static_cast<uint16_t>(aReply->mSize + sizeof(otIcmp6Header)));
    OutputIp6Address(aReply->mSenderAddress);
    OutputLine(": icmp_seq=%u hlim=%u time=%ums", aReply->mSequenceNumber, aReply->mHopLimit, aReply->mRoundTripTime);
}

void PingSender::HandlePingStatistics(const otPingSenderStatistics *aStatistics, void *aContext)
{
    static_cast<PingSender *>(aContext)->HandlePingStatistics(aStatistics);
}

void PingSender::HandlePingStatistics(const otPingSenderStatistics *aStatistics)
{
    OutputFormat("%u packets transmitted, %u packets received.", aStatistics->mSentCount, aStatistics->mReceivedCount);

    if ((aStatistics->mSentCount != 0) && !aStatistics->mIsMulticast &&
        aStatistics->mReceivedCount <= aStatistics->mSentCount)
    {
        uint32_t packetLossRate =
            1000 * (aStatistics->mSentCount - aStatistics->mReceivedCount) / aStatistics->mSentCount;

        OutputFormat(" Packet loss = %lu.%u%%.", ToUlong(packetLossRate / 10),
                     static_cast<uint16_t>(packetLossRate % 10));
    }

    if (aStatistics->mReceivedCount != 0)
    {
        uint32_t avgRoundTripTime = 1000 * aStatistics->mTotalRoundTripTime / aStatistics->mReceivedCount;

        OutputFormat(" Round-trip min/avg/max = %u/%u.%u/%u ms.", aStatistics->mMinRoundTripTime,
                     static_cast<uint16_t>(avgRoundTripTime / 1000), static_cast<uint16_t>(avgRoundTripTime % 1000),
                     aStatistics->mMaxRoundTripTime);
    }

    OutputNewLine();

    if (!mPingIsAsync)
    {
        OutputResult(OT_ERROR_NONE);
    }
}

void PingSender::OutputResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_PING_SENDER_ENABLE
