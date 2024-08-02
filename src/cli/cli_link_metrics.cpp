/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the CLI interpreter for Link Metrics function.
 */

#include "cli_link_metrics.hpp"

#include <openthread/link_metrics.h>

#include "cli/cli.hpp"
#include "cli/cli_utils.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

namespace ot {
namespace Cli {

LinkMetrics::LinkMetrics(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
    , mQuerySync(false)
    , mConfigForwardTrackingSeriesSync(false)
    , mConfigEnhAckProbingSync(false)
{
}

template <> otError LinkMetrics::Process<Cmd("query")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    OutputLine("The command \"linkmetrics query\" has been replaced by the command \"linkmetrics request\".");
    return OT_ERROR_INVALID_COMMAND;
}

template <> otError LinkMetrics::Process<Cmd("request")>(Arg aArgs[])
{
    otError       error = OT_ERROR_NONE;
    otIp6Address  address;
    bool          sync = true;
    bool          isSingle;
    uint8_t       seriesId;
    otLinkMetrics linkMetrics;

    if (aArgs[0] == "async")
    {
        sync = false;
        aArgs++;
    }

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(address));

    /**
     * @cli linkmetrics request single
     * @code
     * linkmetrics request fe80:0:0:0:3092:f334:1455:1ad2 single qmr
     * Received Link Metrics Report from: fe80:0:0:0:3092:f334:1455:1ad2
     * - LQI: 76 (Exponential Moving Average)
     * - Margin: 82 (dB) (Exponential Moving Average)
     * - RSSI: -18 (dBm) (Exponential Moving Average)
     * Done
     * @endcode
     * @cparam linkmetrics request [@ca{async}] @ca{peer-ipaddr} single [@ca{pqmr}]
     * - `async`: Use the non-blocking mode.
     * - `peer-ipaddr`: Peer address.
     * - [`p`, `q`, `m`, and `r`] map to #otLinkMetrics.
     *   - `p`: Layer 2 Number of PDUs received.
     *   - `q`: Layer 2 LQI.
     *   - `m`: Link Margin.
     *   - `r`: RSSI.
     * @par
     * Perform a Link Metrics query (Single Probe).
     * @sa otLinkMetricsQuery
     */
    if (aArgs[1] == "single")
    {
        isSingle = true;
        SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[2]));
    }
    /**
     * @cli linkmetrics request forward
     * @code
     * linkmetrics request fe80:0:0:0:3092:f334:1455:1ad2 forward 1
     * Received Link Metrics Report from: fe80:0:0:0:3092:f334:1455:1ad2
     * - PDU Counter: 2 (Count/Summation)
     * - LQI: 76 (Exponential Moving Average)
     * - Margin: 82 (dB) (Exponential Moving Average)
     * - RSSI: -18 (dBm) (Exponential Moving Average)
     * Done
     * @endcode
     * @cparam linkmetrics query [@ca{async}] @ca{peer-ipaddr} forward @ca{series-id}
     * - `async`: Use the non-blocking mode.
     * - `peer-ipaddr`: Peer address.
     * - `series-id`: The Series ID.
     * @par
     * Perform a Link Metrics query (Forward Tracking Series).
     * @sa otLinkMetricsQuery
     */
    else if (aArgs[1] == "forward")
    {
        isSingle = false;
        SuccessOrExit(error = aArgs[2].ParseAsUint8(seriesId));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(error = otLinkMetricsQuery(GetInstancePtr(), &address, isSingle ? 0 : seriesId,
                                             isSingle ? &linkMetrics : nullptr, HandleLinkMetricsReport, this));

    if (sync)
    {
        mQuerySync = true;
        error      = OT_ERROR_PENDING;
    }

exit:
    return error;
}

template <> otError LinkMetrics::Process<Cmd("mgmt")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    OutputLine("The command \"linkmetrics mgmt\" has been replaced by the command \"linkmetrics config\".");
    return OT_ERROR_INVALID_COMMAND;
}

template <> otError LinkMetrics::Process<Cmd("config")>(Arg aArgs[])
{
    otError                  error;
    otIp6Address             address;
    otLinkMetricsSeriesFlags seriesFlags;
    bool                     sync  = true;
    bool                     clear = false;

    if (aArgs[0] == "async")
    {
        sync = false;
        aArgs++;
    }

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(address));

    ClearAllBytes(seriesFlags);

    /**
     * @cli linkmetrics config forward
     * @code
     * linkmetrics config fe80:0:0:0:3092:f334:1455:1ad2 forward 1 dra pqmr
     * Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
     * Status: SUCCESS
     * Done
     * @endcode
     * @cparam linkmetrics config [@ca{async}] @ca{peer-ipaddr} forward @ca{series-id} [@ca{ldraX}][@ca{pqmr}]
     * - `async`: Use the non-blocking mode.
     * - `peer-ipaddr`: Peer address.
     * - `series-id`: The Series ID.
     * - [`l`, `d`, `r`, and `a`] map to #otLinkMetricsSeriesFlags. `X` represents none of the
     *   `otLinkMetricsSeriesFlags`, and stops the accounting and removes the series.
     *   - `l`: MLE Link Probe.
     *   - `d`: MAC Data.
     *   - `r`: MAC Data Request.
     *   - `a`: MAC Ack.
     *   - `X`: Can only be used without any other flags.
     * - [`p`, `q`, `m`, and `r`] map to #otLinkMetricsValues.
     *   - `p`: Layer 2 Number of PDUs received.
     *   - `q`: Layer 2 LQI.
     *   - `m`: Link Margin.
     *   - `r`: RSSI.
     * @par api_copy
     * #otLinkMetricsConfigForwardTrackingSeries
     */
    if (aArgs[1] == "forward")
    {
        uint8_t       seriesId;
        otLinkMetrics linkMetrics;

        ClearAllBytes(linkMetrics);
        SuccessOrExit(error = aArgs[2].ParseAsUint8(seriesId));
        VerifyOrExit(!aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        for (const char *arg = aArgs[3].GetCString(); *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'l':
                seriesFlags.mLinkProbe = true;
                break;

            case 'd':
                seriesFlags.mMacData = true;
                break;

            case 'r':
                seriesFlags.mMacDataRequest = true;
                break;

            case 'a':
                seriesFlags.mMacAck = true;
                break;

            case 'X':
                VerifyOrExit(arg == aArgs[3].GetCString() && *(arg + 1) == '\0' && aArgs[4].IsEmpty(),
                             error = OT_ERROR_INVALID_ARGS); // Ensure the flags only contain 'X'
                clear = true;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        if (!clear)
        {
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[4]));
            VerifyOrExit(aArgs[5].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }

        SuccessOrExit(error = otLinkMetricsConfigForwardTrackingSeries(
                          GetInstancePtr(), &address, seriesId, seriesFlags, clear ? nullptr : &linkMetrics,
                          &LinkMetrics::HandleLinkMetricsConfigForwardTrackingSeriesMgmtResponse, this));

        if (sync)
        {
            mConfigForwardTrackingSeriesSync = true;
            error                            = OT_ERROR_PENDING;
        }
    }
    else if (aArgs[1] == "enhanced-ack")
    {
        otLinkMetricsEnhAckFlags enhAckFlags;
        otLinkMetrics            linkMetrics;
        otLinkMetrics           *pLinkMetrics = &linkMetrics;

        /**
         * @cli linkmetrics config enhanced-ack clear
         * @code
         * linkmetrics config fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack clear
         * Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
         * Status: Success
         * Done
         * @endcode
         * @cparam linkmetrics config [@ca{async}] @ca{peer-ipaddr} enhanced-ack clear
         * - `async`: Use the non-blocking mode.
         * - `peer-ipaddr` should be the Link Local address of the neighboring device.
         * @par
         * Sends a Link Metrics Management Request to clear an Enhanced-ACK Based Probing.
         * @sa otLinkMetricsConfigEnhAckProbing
         */
        if (aArgs[2] == "clear")
        {
            enhAckFlags  = OT_LINK_METRICS_ENH_ACK_CLEAR;
            pLinkMetrics = nullptr;
        }
        /**
         * @cli linkmetrics config enhanced-ack register
         * @code
         * linkmetrics config fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack register qm
         * Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
         * Status: Success
         * Done
         * @endcode
         * @code
         * > linkmetrics config fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack register qm r
         * Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
         * Status: Cannot support new series
         * Done
         * @endcode
         * @cparam linkmetrics config [@ca{async}] @ca{peer-ipaddr} enhanced-ack register [@ca{qmr}][@ca{r}]
         * - `async`: Use the non-blocking mode.
         * - [`q`, `m`, and `r`] map to #otLinkMetricsValues. Per spec 4.11.3.4.4.6, you can
         *   only use a maximum of two options at once, for example `q`, or `qm`.
         *   - `q`: Layer 2 LQI.
         *   - `m`: Link Margin.
         *   - `r`: RSSI.
         * @par
         * The additional `r` is optional and only used for reference devices. When this option
         * is specified, Type/Average Enum of each Type Id Flags is set to reserved. This is
         * used to verify that the Probing Subject correctly handles invalid Type Id Flags, and
         * only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
         * @par
         * Sends a Link Metrics Management Request to register an Enhanced-ACK Based Probing.
         * @sa otLinkMetricsConfigEnhAckProbing
         */
        else if (aArgs[2] == "register")
        {
            enhAckFlags = OT_LINK_METRICS_ENH_ACK_REGISTER;
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[3]));
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
            if (aArgs[4] == "r")
            {
                linkMetrics.mReserved = true;
            }
#endif
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        SuccessOrExit(
            error = otLinkMetricsConfigEnhAckProbing(GetInstancePtr(), &address, enhAckFlags, pLinkMetrics,
                                                     &LinkMetrics::HandleLinkMetricsConfigEnhAckProbingMgmtResponse,
                                                     this, &LinkMetrics::HandleLinkMetricsEnhAckProbingIe, this));

        if (sync)
        {
            mConfigEnhAckProbingSync = true;
            error                    = OT_ERROR_PENDING;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

template <> otError LinkMetrics::Process<Cmd("probe")>(Arg aArgs[])
{
    /**
     * @cli linkmetrics probe
     * @code
     * linkmetrics probe fe80:0:0:0:3092:f334:1455:1ad2 1 10
     * Done
     * @endcode
     * @cparam linkmetrics probe @ca{peer-ipaddr} @ca{series-id} @ca{length}
     * - `peer-ipaddr`: Peer address.
     * - `series-id`: The Series ID for which this Probe message targets.
     * - `length`: The length of the Probe message. A valid range is [0, 64].
     * @par api_copy
     * #otLinkMetricsSendLinkProbe
     */
    otError error = OT_ERROR_NONE;

    otIp6Address address;
    uint8_t      seriesId;
    uint8_t      length;

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(address));
    SuccessOrExit(error = aArgs[1].ParseAsUint8(seriesId));
    SuccessOrExit(error = aArgs[2].ParseAsUint8(length));

    error = otLinkMetricsSendLinkProbe(GetInstancePtr(), &address, seriesId, length);
exit:
    return error;
}

otError LinkMetrics::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                   \
    {                                                              \
        aCommandString, &LinkMetrics::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("config"), CmdEntry("mgmt"), CmdEntry("probe"), CmdEntry("query"), CmdEntry("request"),
    };

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

otError LinkMetrics::ParseLinkMetricsFlags(otLinkMetrics &aLinkMetrics, const Arg &aFlags)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aFlags.IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    ClearAllBytes(aLinkMetrics);

    for (const char *arg = aFlags.GetCString(); *arg != '\0'; arg++)
    {
        switch (*arg)
        {
        case 'p':
            aLinkMetrics.mPduCount = true;
            break;

        case 'q':
            aLinkMetrics.mLqi = true;
            break;

        case 'm':
            aLinkMetrics.mLinkMargin = true;
            break;

        case 'r':
            aLinkMetrics.mRssi = true;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

void LinkMetrics::HandleLinkMetricsReport(const otIp6Address        *aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          otLinkMetricsStatus        aStatus,
                                          void                      *aContext)
{
    static_cast<LinkMetrics *>(aContext)->HandleLinkMetricsReport(aAddress, aMetricsValues, aStatus);
}

void LinkMetrics::PrintLinkMetricsValue(const otLinkMetricsValues *aMetricsValues)
{
    static const char kLinkMetricsTypeAverage[] = "(Exponential Moving Average)";

    if (aMetricsValues->mMetrics.mPduCount)
    {
        OutputLine(" - PDU Counter: %lu (Count/Summation)", ToUlong(aMetricsValues->mPduCountValue));
    }

    if (aMetricsValues->mMetrics.mLqi)
    {
        OutputLine(" - LQI: %u %s", aMetricsValues->mLqiValue, kLinkMetricsTypeAverage);
    }

    if (aMetricsValues->mMetrics.mLinkMargin)
    {
        OutputLine(" - Margin: %u (dB) %s", aMetricsValues->mLinkMarginValue, kLinkMetricsTypeAverage);
    }

    if (aMetricsValues->mMetrics.mRssi)
    {
        OutputLine(" - RSSI: %d (dBm) %s", aMetricsValues->mRssiValue, kLinkMetricsTypeAverage);
    }
}

void LinkMetrics::HandleLinkMetricsReport(const otIp6Address        *aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          otLinkMetricsStatus        aStatus)
{
    OutputFormat("Received Link Metrics Report from: ");
    OutputIp6AddressLine(*aAddress);

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
    else
    {
        OutputLine("Link Metrics Report, status: %s", LinkMetricsStatusToStr(aStatus));
    }

    if (mQuerySync)
    {
        mQuerySync = false;
        OutputResult(OT_ERROR_NONE);
    }
}

void LinkMetrics::HandleLinkMetricsConfigForwardTrackingSeriesMgmtResponse(const otIp6Address *aAddress,
                                                                           otLinkMetricsStatus aStatus,
                                                                           void               *aContext)
{
    static_cast<LinkMetrics *>(aContext)->HandleLinkMetricsConfigForwardTrackingSeriesMgmtResponse(aAddress, aStatus);
}

void LinkMetrics::HandleLinkMetricsConfigForwardTrackingSeriesMgmtResponse(const otIp6Address *aAddress,
                                                                           otLinkMetricsStatus aStatus)
{
    HandleLinkMetricsMgmtResponse(aAddress, aStatus);

    if (mConfigForwardTrackingSeriesSync)
    {
        mConfigForwardTrackingSeriesSync = false;
        OutputResult(OT_ERROR_NONE);
    }
}

void LinkMetrics::HandleLinkMetricsConfigEnhAckProbingMgmtResponse(const otIp6Address *aAddress,
                                                                   otLinkMetricsStatus aStatus,
                                                                   void               *aContext)
{
    static_cast<LinkMetrics *>(aContext)->HandleLinkMetricsConfigEnhAckProbingMgmtResponse(aAddress, aStatus);
}

void LinkMetrics::HandleLinkMetricsConfigEnhAckProbingMgmtResponse(const otIp6Address *aAddress,
                                                                   otLinkMetricsStatus aStatus)
{
    HandleLinkMetricsMgmtResponse(aAddress, aStatus);

    if (mConfigEnhAckProbingSync)
    {
        mConfigEnhAckProbingSync = false;
        OutputResult(OT_ERROR_NONE);
    }
}

void LinkMetrics::HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus)
{
    OutputFormat("Received Link Metrics Management Response from: ");
    OutputIp6AddressLine(*aAddress);

    OutputLine("Status: %s", LinkMetricsStatusToStr(aStatus));
}

void LinkMetrics::HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                   const otExtAddress        *aExtAddress,
                                                   const otLinkMetricsValues *aMetricsValues,
                                                   void                      *aContext)
{
    static_cast<LinkMetrics *>(aContext)->HandleLinkMetricsEnhAckProbingIe(aShortAddress, aExtAddress, aMetricsValues);
}

void LinkMetrics::HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                   const otExtAddress        *aExtAddress,
                                                   const otLinkMetricsValues *aMetricsValues)
{
    OutputFormat("Received Link Metrics data in Enh Ack from neighbor, short address:0x%02x , extended address:",
                 aShortAddress);
    OutputExtAddressLine(*aExtAddress);

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
}

const char *LinkMetrics::LinkMetricsStatusToStr(otLinkMetricsStatus aStatus)
{
    static const char *const kStatusStrings[] = {
        "Success",                      // (0) OT_LINK_METRICS_STATUS_SUCCESS
        "Cannot support new series",    // (1) OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES
        "Series ID already registered", // (2) OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED
        "Series ID not recognized",     // (3) OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED
        "No matching series ID",        // (4) OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED
    };

    const char *str = "Unknown error";

    static_assert(0 == OT_LINK_METRICS_STATUS_SUCCESS, "STATUS_SUCCESS is incorrect");
    static_assert(1 == OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES, "CANNOT_SUPPORT_NEW_SERIES is incorrect");
    static_assert(2 == OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED, "SERIESID_ALREADY_REGISTERED is incorrect");
    static_assert(3 == OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED, "SERIESID_NOT_RECOGNIZED is incorrect");
    static_assert(4 == OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED, "NO_MATCHING_FRAMES_RECEIVED is incorrect");

    if (aStatus < OT_ARRAY_LENGTH(kStatusStrings))
    {
        str = kStatusStrings[aStatus];
    }
    else if (aStatus == OT_LINK_METRICS_STATUS_OTHER_ERROR)
    {
        str = "Other error";
    }

    return str;
}

void LinkMetrics::OutputResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
