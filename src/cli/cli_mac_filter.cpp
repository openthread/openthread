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

template <> otError MacFilter::Process<Cmd("addr")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otExtAddress extAddr;

    /**
     * @cli macfilter addr
     * @code
     * macfilter addr
     * Allowlist
     * 0f6127e33af6b403 : rss -95 (lqi 1)
     * 0f6127e33af6b402
     * Done
     * @endcode
     * @par
     * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
     * @par
     * Provides the following information:
     * - Current mode of the MAC filter list: Either `AllowList`, `DenyList,` or `Disabled`
     * - A list of all the extended addresses in the filter. The received signal strength (rss) and
     *   link quality indicator (lqi) are listed next to the address if these values have been set to be
     *   different from the default values.
     * @sa otLinkFilterGetAddressMode
     */
    if (aArgs[0].IsEmpty())
    {
        OutputFilter(kAddressFilter);
    }
    /**
     * @cli macfilter addr add
     * @code
     * macfilter addr add 0f6127e33af6b403 -95
     * Done
     * @endcode
     * @code
     * macfilter addr add 0f6127e33af6b402
     * Done
     * @endcode
     * @cparam macfilter addr add @ca{extaddr} [@ca{rss}]
     * @par
     * Is available only when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
     * @par
     * Adds an IEEE 802.15.4 Extended Address to the MAC filter list.
     * If you specify the optional `rss` argument, this fixes the received signal strength for messages from the
     * address. If you do not use the `rss` option, the address will use whatever default value you have set.
     * If you have not set a default, the signal strength will be the over-air signal.
     * @sa otLinkFilterAddAddress
     */
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
    /**
     * @cli macfilter addr remove
     * @code
     * macfilter addr remove 0f6127e33af6b402
     * Done
     * @endcode
     * @cparam macfilter addr remove @ca{extaddr}
     * @par
     * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
     * @par
     * This command removes the specified extended address from the MAC filter list.
     * @note No action is performed if the specified extended address does not match an entry in the MAC filter list.
     * @sa otLinkFilterRemoveAddress
     */
    else if (aArgs[0] == "remove")
    {
        SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
        otLinkFilterRemoveAddress(GetInstancePtr(), &extAddr);
    }
    /**
     * @cli macfilter addr clear
     * @code
     * macfilter addr clear
     * Done
     * @endcode
     * @par
     * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
     * @par
     * This command clears all the extended addresses from the MAC filter list.
     * @note This command does not affect entries in the `RssIn` list. That list contains extended addresses where the
     * `rss` has been set to a fixed value that differs from the default.
     * @sa otLinkFilterClearAddresses
     */
    else if (aArgs[0] == "clear")
    {
        otLinkFilterClearAddresses(GetInstancePtr());
    }
    else
    {
        static const char *const kModeCommands[] = {
            /**
             * @cli macfilter addr disable
             * @code
             * macfilter addr disable
             * Done
             * @endcode
             * @par
             * Disables MAC filter modes.
             */
            "disable", // (0) OT_MAC_FILTER_ADDRESS_MODE_DISABLED
            /**
             * @cli macfilter addr allowlist
             * @code
             * macfilter addr allowlist
             * Done
             * @endcode
             * @par
             * Enables the `allowlist` MAC filter mode, which means that only the MAC addresses in the MAC filter list
             * will be allowed access.
             * @sa otLinkFilterSetAddressMode
             */
            "allowlist", // (1) OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST
            /**
             * @cli macfilter addr denylist
             * @code
             * macfilter addr denylist
             * Done
             * @endcode
             * @par
             * Enables the `denylist` MAC filter mode, which means that all MAC addresses in the MAC filter list
             * will be denied access.
             * @sa otLinkFilterSetAddressMode
             */
            "denylist", // (2) OT_MAC_FILTER_ADDRESS_MODE_DENYLIST
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

/**
 * @cli macfilter rss
 * @code
 * macfilter rss
 * 0f6127e33af6b403 : rss -95 (lqi 1)
 * Default rss: -50 (lqi 3)
 * Done
 * @endcode
 * @par
 * Provides the following information:
 * - Listing of all the extended addresses
 * where the received signal strength (`rss`) has been set to be different from
 * the default value. The link quality indicator (`lqi`) is also shown. The `rss`
 * and `lqi` settings map to each other. If you set one, the value of the other
 * gets set automatically. This list of addresses is called the `RssIn List`.
 * Setting either the `rsi` or the `lqi` adds the corresponding extended address
 * to the `RssIn` list.
 * - `Default rss`: Shows the default values, if applicable, for the `rss` and `lqi` settings.
 * @sa otLinkFilterGetNextRssIn
 */
template <> otError MacFilter::Process<Cmd("rss")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otExtAddress extAddr;
    int8_t       rss;

    if (aArgs[0].IsEmpty())
    {
        OutputFilter(kRssFilter);
    }
    /**
     * @cli macfilter rss add-lqi
     * @code
     * macfilter rss add-lqi * 3
     * Done
     * @endcode
     * @code
     * macfilter rss add-lqi 0f6127e33af6b404 2
     * Done
     * @endcode
     * @cparam macfilter rss add-lqi @ca{extaddr} @ca{lqi}
     * To set a default value for the link quality indicator for all received messages,
     * use the `*` for the `extaddr` argument. The allowed range is 0 to 3.
     * @par
     * Adds the specified Extended Address to the `RssIn` list (or modifies an existing address in the `RssIn` list)
     * and sets the fixed link quality indicator for messages from that address.
     * The Extended Address
     * does not necessarily have to be in the `address allowlist/denylist` filter to set the `lqi`.
     * @note The `RssIn` list contains Extended Addresses whose `lqi` or
     * received signal strength (`rss`) values have been set to be different from the defaults.
     * The `lqi` will automatically get converted to a corresponding `rss` value.
     * @par
     * This is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
     * @sa otLinkConvertLinkQualityToRss
     * @sa otLinkFilterSetDefaultRssIn
     */
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
    /**
     * @cli macfilter rss add
     * @code
     * macfilter rss add * -50
     * Done
     * @endcode
     * @code
     * macfilter rss add 0f6127e33af6b404 -85
     * Done
     * @endcode
     * @cparam macfilter rss add @ca{extaddr} @ca{rss}
     * To set a default value for the received signal strength for all received messages,
     * use the `*` for the `extaddr` argument.
     * @par api_copy
     * #otLinkFilterAddRssIn
     */
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
    /**
     * @cli macfilter rss remove
     * @code
     * macfilter rss remove *
     * Done
     * @endcode
     * @code
     * macfilter rss remove 0f6127e33af6b404
     * Done
     * @endcode
     * @cparam macfilter rss remove @ca{extaddr}
     * If you wish to remove the default received signal strength and link quality indicator settings,
     * use the `*` as the `extaddr`. This unsets the defaults but does not remove
     * entries from the `RssIn` list.
     * @par api_copy
     * #otLinkFilterRemoveRssIn
     */
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
    /**
     * @cli macfilter rss clear
     * @code
     * macfilter rss clear
     * Done
     * @endcode
     * @par api_copy
     * #otLinkFilterClearAllRssIn
     */
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

    /**
     * @cli macfilter
     * @code
     * macfilter
     * Address Mode: Allowlist
     * 0f6127e33af6b403 : rss -95 (lqi 1)
     * 0f6127e33af6b402
     * RssIn List:
     * 0f6127e33af6b403 : rss -95 (lqi 1)
     * Default rss: -50 (lqi 3)
     * Done
     * @endcode
     * @par
     * Provides the following information:
     * - `Address Mode`: Current mode of the MAC filter: Either `AllowList`, `DenyList,` or `Disabled`
     * - A list of all the extended addresses in the MAC filter list. The received signal strength (rss) and
     *   link quality indicator (lqi) are listed next to the address if these values have been set to be
     *   different from the default values.
     * - A separate list (`RssIn List`) that shows all the extended addresses where the `rss` has been set to
     *   be different from the default value.
     * - `Default rss`: Shows the default values, if applicable, for the `rss` and `lqi` settings.
     * @note An extended address can be in the `RssIn` list without being in the MAC filter list.
     * @sa otLinkFilterSetAddressMode
     * @sa otLinkFilterGetNextAddress
     * @sa otLinkFilterGetNextRssIn
     */
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
