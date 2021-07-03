/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements CLI for the History Tracker.
 */

#include "cli_history.hpp"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#include <string.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

constexpr History::Command History::sCommands[];

otError History::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError History::ParseArgs(Arg aArgs[], bool &aIsList, uint16_t &aNumEntries) const
{
    if (*aArgs == "list")
    {
        aArgs++;
        aIsList = true;
    }
    else
    {
        aIsList = false;
    }

    if (aArgs->ParseAsUint16(aNumEntries) == OT_ERROR_NONE)
    {
        aArgs++;
    }
    else
    {
        aNumEntries = 0;
    }

    return aArgs[0].IsEmpty() ? OT_ERROR_NONE : OT_ERROR_INVALID_ARGS;
}

otError History::ProcessNetInfo(Arg aArgs[])
{
    otError                            error;
    bool                               isList;
    uint16_t                           numEntries;
    otHistoryTrackerIterator           iterator;
    const otHistoryTrackerNetworkInfo *info;
    uint32_t                           entryAge;
    char                               ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                               linkModeString[Interpreter::kLinkModeStringSize];

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Time                     | Role     | Mode | RLOC16 | Partition ID |
        // +--------------------------+----------+------+--------+--------------+

        static const char *const kNetInfoTableTitles[]       = {"Time", "Role", "Mode", "RLOC16", "Partition ID"};
        static const uint8_t     kNetInfoTableColumnWidths[] = {26, 10, 6, 8, 14};

        mInterpreter.OutputTableHeader(kNetInfoTableTitles, kNetInfoTableColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateNetInfoHistory(mInterpreter.mInstance, &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        mInterpreter.OutputLine(
            isList ? "%s -> role:%s mode:%s rloc16:0x%04x partition-id:%u" : "| %24s | %-8s | %-4s | 0x%04x | %12u |",
            ageString, otThreadDeviceRoleToString(info->mRole),
            Interpreter::LinkModeToString(info->mMode, linkModeString), info->mRloc16, info->mPartitionId);
    }

exit:
    return error;
}

otError History::ProcessRx(Arg aArgs[])
{
    return ProcessRxTxHistory(/* aIsRx */ true, aArgs);
}

otError History::ProcessTx(Arg aArgs[])
{
    return ProcessRxTxHistory(/* aIsRx */ false, aArgs);
}

const char *History::MessagePriorityToString(uint8_t aPriority)
{
    const char *str = "unkn";

    switch (aPriority)
    {
    case OT_HISTORY_TRACKER_MSG_PRIORITY_LOW:
        str = "low";
        break;

    case OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL:
        str = "norm";
        break;

    case OT_HISTORY_TRACKER_MSG_PRIORITY_HIGH:
        str = "high";
        break;

    case OT_HISTORY_TRACKER_MSG_PRIORITY_NET:
        str = "net";
        break;

    default:
        break;
    }

    return str;
}

const char *History::RadioTypeToString(const otHistoryTrackerMessageInfo &aInfo)
{
    const char *str = "none";

    if (aInfo.mRadioTrelUdp6 && aInfo.mRadioIeee802154)
    {
        str = "all";
    }
    else if (aInfo.mRadioIeee802154)
    {
        str = "15.4";
    }
    else if (aInfo.mRadioTrelUdp6)
    {
        str = "trel";
    }

    return str;
}

const char *History::MessageTypeToString(const otHistoryTrackerMessageInfo &aInfo)
{
    const char *str = otIp6ProtoToString(aInfo.mIpProto);

    if (aInfo.mIpProto == OT_IP6_PROTO_ICMP6)
    {
        switch (aInfo.mIcmp6Type)
        {
        case OT_ICMP6_TYPE_DST_UNREACH:
            str = "ICMP6(Unreach)";
            break;
        case OT_ICMP6_TYPE_PACKET_TO_BIG:
            str = "ICMP6(TooBig)";
            break;
        case OT_ICMP6_TYPE_ECHO_REQUEST:
            str = "ICMP6(EchoReqst)";
            break;
        case OT_ICMP6_TYPE_ECHO_REPLY:
            str = "ICMP6(EchoReply)";
            break;
        case OT_ICMP6_TYPE_ROUTER_SOLICIT:
            str = "ICMP6(RouterSol)";
            break;
        case OT_ICMP6_TYPE_ROUTER_ADVERT:
            str = "ICMP6(RouterAdv)";
            break;
        default:
            str = "ICMP6(Other)";
            break;
        }
    }

    return str;
}

otError History::ProcessRxTxHistory(bool aIsRx, Arg aArgs[])
{
    static constexpr uint8_t kIndentSize = 4;

    otError                            error;
    bool                               isList;
    uint16_t                           numEntries;
    otHistoryTrackerIterator           iterator;
    const otHistoryTrackerMessageInfo *info;
    uint32_t                           entryAge;
    char                               ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                               srcAddrString[OT_IP6_SOCK_ADDR_STRING_SIZE];
    char                               dstAddrString[OT_IP6_SOCK_ADDR_STRING_SIZE];

    // | Time                     | Type             | Len   | Sec | Prio | Succ | Form   | Radio |
    // +--------------------------+------------------+-------+-----+------+------+--------+-------+

    static const char *const kTxTableTitles[]     = {"Time", "Type", "Len", "Sec", "Prio", "Succ", "To", "Radio"};
    static const char *const kRxTableTitles[]     = {"Time", "Type", "Len", "Sec", "Prio", "RSS", "From", "Radio"};
    static const uint8_t     kTableColumnWidths[] = {26, 18, 7, 5, 6, 6, 8, 7};

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        mInterpreter.OutputTableHeader(aIsRx ? kRxTableTitles : kTxTableTitles, kTableColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = aIsRx ? otHistoryTrackerIterateRxHistory(mInterpreter.mInstance, &iterator, &entryAge)
                     : otHistoryTrackerIterateTxHistory(mInterpreter.mInstance, &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));
        otIp6SockAddrToString(&info->mSource, srcAddrString, sizeof(srcAddrString));
        otIp6SockAddrToString(&info->mDestination, dstAddrString, sizeof(dstAddrString));

        if (isList)
        {
            mInterpreter.OutputLine("%s", ageString);
            mInterpreter.OutputFormat(kIndentSize, "type:%s len:%u sec:%s prio:%s ", MessageTypeToString(*info),
                                      info->mPayloadLength, info->mLinkSecurity ? "yes" : "no",
                                      MessagePriorityToString(info->mPriority));

            if (aIsRx)
            {
                mInterpreter.OutputFormat("rss:%d", info->mAveRxRss);
            }
            else
            {
                mInterpreter.OutputFormat("tx-success:%s", info->mTxSuccess ? "yes" : "no");
            }

            mInterpreter.OutputLine(" %s:0x%04x radio:%s", aIsRx ? "from" : "to", info->mNeighborRloc16,
                                    RadioTypeToString(*info));
            mInterpreter.OutputLine(kIndentSize, "src:%s", srcAddrString);
            mInterpreter.OutputLine(kIndentSize, "dst:%s", dstAddrString);
        }
        else
        {
            if (index != 0)
            {
                mInterpreter.OutputTableSeperator(kTableColumnWidths);
            }

            mInterpreter.OutputFormat("| %24s | %-16.16s | %5u | %3s | %4s | ", "", MessageTypeToString(*info),
                                      info->mPayloadLength, info->mLinkSecurity ? "yes" : "no",
                                      MessagePriorityToString(info->mPriority));

            if (aIsRx)
            {
                mInterpreter.OutputFormat("%4d", info->mAveRxRss);
            }
            else
            {
                mInterpreter.OutputFormat("%4s", info->mTxSuccess ? "yes" : "no");
            }

            mInterpreter.OutputLine(" | 0x%04x | %5.5s |", info->mNeighborRloc16, RadioTypeToString(*info));
            mInterpreter.OutputLine("| %24s | src: %-56s |", ageString, srcAddrString);
            mInterpreter.OutputLine("| %24s | dst: %-56s |", "", dstAddrString);
        }
    }

exit:
    return error;
}

otError History::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow();
    }

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
