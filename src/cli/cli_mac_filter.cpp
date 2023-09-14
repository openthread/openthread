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
 *   This file implements CLI for MAC Filter.
 */

#include "cli_mac_filter.hpp"

#include "cli/cli.hpp"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

namespace ot {
namespace Cli {

void MacFilter::OutputFilter(uint8_t aFilters)
{
    otMacFilterEntry    entry;
    otMacFilterIterator iterator;

    if (aFilters & kAddressFilter)
    {
        if ((aFilters & ~kAddressFilter) != 0)
        {
            OutputFormat("Address Mode: ");
        }

        OutputLine("%s", AddressModeToString(otLinkFilterGetAddressMode(GetInstancePtr())));

        iterator = OT_MAC_FILTER_ITERATOR_INIT;

        while (otLinkFilterGetNextAddress(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
        {
            OutputEntry(entry);
        }
    }

    if (aFilters & kRssFilter)
    {
        if ((aFilters & ~kRssFilter) != 0)
        {
            OutputLine("RssIn List:");
        }

        iterator = OT_MAC_FILTER_ITERATOR_INIT;

        while (otLinkFilterGetNextRssIn(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
        {
            if (IsDefaultRss(entry.mExtAddress))
            {
                OutputLine("Default rss: %d (lqi %u)", entry.mRssIn,
                           otLinkConvertRssToLinkQuality(GetInstancePtr(), entry.mRssIn));
            }
            else
            {
                OutputEntry(entry);
            }
        }
    }
}

bool MacFilter::IsDefaultRss(const otExtAddress &aExtAddress)
{
    // In default RSS entry, the extended address will be all `0xff`.

    bool isDefault = true;

    for (uint8_t byte : aExtAddress.m8)
    {
        if (byte != 0xff)
        {
            isDefault = false;
            break;
        }
    }

    return isDefault;
}

const char *MacFilter::AddressModeToString(otMacFilterAddressMode aMode)
{
    static const char *const kModeStrings[] = {
        "Disabled",  // (0) OT_MAC_FILTER_ADDRESS_MODE_DISABLED
        "Allowlist", // (1) OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST
        "Denylist",  // (2) OT_MAC_FILTER_ADDRESS_MODE_DENYLIST
    };

    static_assert(0 == OT_MAC_FILTER_ADDRESS_MODE_DISABLED, "OT_MAC_FILTER_ADDRESS_MODE_DISABLED value is incorrect");
    static_assert(1 == OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST, "OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST value is incorrect");
    static_assert(2 == OT_MAC_FILTER_ADDRESS_MODE_DENYLIST, "OT_MAC_FILTER_ADDRESS_MODE_DENYLIST value is incorrect");

    return Stringify(aMode, kModeStrings);
}

void MacFilter::OutputEntry(const otMacFilterEntry &aEntry)
{
    OutputExtAddress(aEntry.mExtAddress);

    if (aEntry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        OutputFormat(" : rss %d (lqi %d)", aEntry.mRssIn,
                     otLinkConvertRssToLinkQuality(GetInstancePtr(), aEntry.mRssIn));
    }

    OutputNewLine();
}

/**
 * @cli macfilter addr
 * @code
 * macfilter addr
 * Allowlist
 * 0f6127e33af6b403 : rss -95 (lqi 1)
 * 0f6127e33af6b402
 * Done
 * @endcode
 * @par api_copy
 * #otLinkFilterGetAddressMode
 */
template <> otError MacFilter::Process<Cmd("addr")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otExtAddress extAddr;

    if (aArgs[0].IsEmpty())
    {
        OutputFilter(kAddressFilter);
    }
    else if (aArgs[0] == "add")
    {
        SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
        error = otLinkFilterAddAddress(GetInstancePtr(), &extAddr);

        VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

        if (!aArgs[2].IsEmpty())
        {
            int8_t rss;

            SuccessOrExit(error = aArgs[2].ParseAsInt8(rss));
            SuccessOrExit(error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss));
        }
    }
    else if (aArgs[0] == "remove")
    {
        SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
        otLinkFilterRemoveAddress(GetInstancePtr(), &extAddr);
    }
    else if (aArgs[0] == "clear")
    {
        otLinkFilterClearAddresses(GetInstancePtr());
    }
    else
    {
        static const char *const kModeCommands[] = {
            "disable",   // (0) OT_MAC_FILTER_ADDRESS_MODE_DISABLED
            "allowlist", // (1) OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST
            "denylist",  // (2) OT_MAC_FILTER_ADDRESS_MODE_DENYLIST
        };

        for (size_t index = 0; index < OT_ARRAY_LENGTH(kModeCommands); index++)
        {
            if (aArgs[0] == kModeCommands[index])
            {
                VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
                otLinkFilterSetAddressMode(GetInstancePtr(), static_cast<otMacFilterAddressMode>(index));
                ExitNow();
            }
        }

        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

template <> otError MacFilter::Process<Cmd("rss")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otExtAddress extAddr;
    int8_t       rss;

    if (aArgs[0].IsEmpty())
    {
        OutputFilter(kRssFilter);
    }
    else if (aArgs[0] == "add-lqi")
    {
        uint8_t linkQuality;

        SuccessOrExit(error = aArgs[2].ParseAsUint8(linkQuality));
        VerifyOrExit(linkQuality <= 3, error = OT_ERROR_INVALID_ARGS);
        rss = otLinkConvertLinkQualityToRss(GetInstancePtr(), linkQuality);

        if (aArgs[1] == "*")
        {
            otLinkFilterSetDefaultRssIn(GetInstancePtr(), rss);
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss);
        }
    }
    else if (aArgs[0] == "add")
    {
        SuccessOrExit(error = aArgs[2].ParseAsInt8(rss));

        if (aArgs[1] == "*")
        {
            otLinkFilterSetDefaultRssIn(GetInstancePtr(), rss);
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss);
        }
    }
    else if (aArgs[0] == "remove")
    {
        if (aArgs[1] == "*")
        {
            otLinkFilterClearDefaultRssIn(GetInstancePtr());
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            otLinkFilterRemoveRssIn(GetInstancePtr(), &extAddr);
        }
    }
    else if (aArgs[0] == "clear")
    {
        otLinkFilterClearAllRssIn(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError MacFilter::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                 \
    {                                                            \
        aCommandString, &MacFilter::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("addr"),
        CmdEntry("rss"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        OutputFilter(kAddressFilter | kRssFilter);
        ExitNow(error = OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
