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
 *   This file implements the CLI MacFilter interpreter.
 */

#include <openthread/config.h>

#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

#include <openthread/link.h>

#include "cli/cli_macfilter.hpp"
#include "cli/cli.hpp"

namespace ot {
namespace Cli {

Server *MacFilter::sServer;
otInstance *MacFilter::sInstance;

otError MacFilter::Process(otInstance *aInstance, int argc, char *argv[], Server &aServer)
{
    otError error = OT_ERROR_NONE;

    sInstance = aInstance;
    sServer = &aServer;

    if (argc == 0)
    {
        PrintFilter();
    }
    else
    {
        if (strcmp(argv[0], "addressfilter") == 0)
        {
            ProcessAddressFilter(argc - 1, argv + 1);
        }
        else if (strcmp(argv[0], "lqinfilter") == 0)
        {
            ProcessLinkQualityInFilter(argc - 1, argv + 1);
        }
        else if (strcmp(argv[0], "reset") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkAddressFilterReset(sInstance);
            otLinkLinkQualityInFilterReset(sInstance);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otThreadErrorToString(error);
    }

    return error;
}

otError MacFilter::ProcessLinkQualityInFilter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        PrintLinkQualityInFilter();
    }
    else
    {
        if (strcmp(argv[0], "unset") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkLinkQualityInFilterUnset(sInstance);
        }
        else if (strcmp(argv[0], "set") == 0)
        {
            uint8_t linkquality;

            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
            linkquality = static_cast<uint8_t>(value);
            SuccessOrExit(error = otLinkLinkQualityInFilterSet(sInstance, linkquality));
        }
        else if (strcmp(argv[0], "add") == 0)
        {
            otExtAddress addr;
            uint8_t linkquality;

            VerifyOrExit(argc == 3, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[1], addr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
            linkquality = static_cast<uint8_t>(value);

            SuccessOrExit(error = otLinkLinkQualityInFilterAddEntry(sInstance, &addr, linkquality));

        }
        else if (strcmp(argv[0], "remove") == 0)
        {
            otExtAddress addr;

            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[1], addr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);

            SuccessOrExit(error = otLinkLinkQualityInFilterRemoveEntry(sInstance, &addr));
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkLinkQualityInFilterClearEntries(sInstance);
        }
        else if (strcmp(argv[0], "reset") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkLinkQualityInFilterReset(sInstance);
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otThreadErrorToString(error);
    }

    return error;
}

otError MacFilter::ProcessAddressFilter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t state;

    if (argc == 0)
    {
        PrintAddressFilter();
    }
    else
    {
        if (strcmp(argv[0], "off") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            state = OT_MAC_ADDRESSFILTER_DISABLED;
            SuccessOrExit(error = otLinkAddressFilterSetState(sInstance, state));
        }
        else if (strcmp(argv[0], "on-whitelist") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            state = OT_MAC_ADDRESSFILTER_WHITELIST;
            SuccessOrExit(error = otLinkAddressFilterSetState(sInstance, state));
        }
        else if (strcmp(argv[0], "on-blacklist") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            state = OT_MAC_ADDRESSFILTER_BLACKLIST;
            SuccessOrExit(error = otLinkAddressFilterSetState(sInstance, state));
        }
        else if (strcmp(argv[0], "add") == 0)
        {
            otExtAddress addr;
            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[1], addr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);

            SuccessOrExit(error = otLinkAddressFilterAddEntry(sInstance, &addr));
        }
        else if (strcmp(argv[0], "remove") == 0)
        {
            otExtAddress addr;
            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[1], addr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);

            SuccessOrExit(error = otLinkAddressFilterRemoveEntry(sInstance, &addr));
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otLinkAddressFilterClearEntries(sInstance));
        }
        else if (strcmp(argv[0], "reset") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkAddressFilterReset(sInstance);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otThreadErrorToString(error);
    }

    return error;
}


void MacFilter::OutputExtAddress(const otExtAddress *aAddress)
{
    for (uint8_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        sServer->OutputFormat("%02x", aAddress->m8[i]);
    }
}

void MacFilter::PrintLinkQualityInFilter(void)
{
    uint8_t linkquality;
    otMacFilterIterator iterator;
    otMacFilterEntry entry;

    sServer->OutputFormat("LinkQualityInFilter entries:\r\n");

    iterator = OT_MAC_FILTER_ITERATOR_INIT;

    while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        if (entry.mLinkQualityInFixed)
        {
            OutputExtAddress(&entry.mExtAddress);
            sServer->OutputFormat(" : %d\r\n", entry.mLinkQualityIn);
        }
    }

    if (otLinkLinkQualityInFilterGet(sInstance, &linkquality) == OT_ERROR_NONE)
    {
        sServer->OutputFormat("lqin: fixed as %d\r\n", linkquality);
    }
    else
    {
        sServer->OutputFormat("lqin: no\r\n");
    }
}

void MacFilter::PrintAddressFilter(void)
{
    uint8_t addressfilter = otLinkAddressFilterGetState(sInstance);
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterEntry entry;

    if (addressfilter == OT_MAC_ADDRESSFILTER_WHITELIST)
    {
        sServer->OutputFormat("Whitelist\r\n");
    }
    else if (addressfilter == OT_MAC_ADDRESSFILTER_BLACKLIST)
    {
        sServer->OutputFormat("Blacklist\r\n");
    }
    else
    {
        sServer->OutputFormat("Disabled\r\n");
        ExitNow();
    }

    while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        // only output whitelisted/blacklisted entry
        if (!entry.mFiltered)
        {
            continue;
        }

        OutputExtAddress(&entry.mExtAddress);
        sServer->OutputFormat("\r\n");
    }

exit:
    return;
}

void MacFilter::PrintFilter(void)
{
    uint8_t linkquality;
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    uint8_t addressfilter = otLinkAddressFilterGetState(sInstance);
    bool hasLqInFixedEntry = false;

    if (addressfilter == OT_MAC_ADDRESSFILTER_WHITELIST)
    {
        sServer->OutputFormat("AddressFilter whitelist enabled\r\n");
        sServer->OutputFormat("|   Extended MAC   | LqIn fixed | LqIn Value |\r\n");
        sServer->OutputFormat("+------------------+------------+------------|\r\n");

        while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            // only output whitelisted entry
            if (!entry.mFiltered)
            {
                continue;
            }

            sServer->OutputFormat("|");
            OutputExtAddress(&entry.mExtAddress);

            if (entry.mLinkQualityInFixed)
            {
                sServer->OutputFormat("  |     Y      |     %d     |\r\n", entry.mLinkQualityIn);
            }
            else
            {
                sServer->OutputFormat("  |     N      |            |\r\n");
            }
        }
    }
    else if (addressfilter == OT_MAC_ADDRESSFILTER_BLACKLIST)
    {
        sServer->OutputFormat("AddressFilter blacklist enabled\r\n");

        while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            // only output blocklisted entry
            if (!entry.mFiltered)
            {
                hasLqInFixedEntry = true;
                continue;
            }

            OutputExtAddress(&entry.mExtAddress);
            sServer->OutputFormat("\r\n");
        }
    }
    else
    {
        sServer->OutputFormat("AddressFilter is disabled\r\n");

        // check whether there are LinkQualityIn filter entry
        while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            hasLqInFixedEntry = true;
        }
    }

    if (hasLqInFixedEntry)
    {
        sServer->OutputFormat("LinkQualityInFilter entries:\r\n");

        iterator = OT_MAC_FILTER_ITERATOR_INIT;

        while (otLinkFilterGetNextEntry(sInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            if (entry.mLinkQualityInFixed)
            {
                OutputExtAddress(&entry.mExtAddress);
                sServer->OutputFormat(" : %d\r\n", entry.mLinkQualityIn);
            }
        }
    }

    if (otLinkLinkQualityInFilterGet(sInstance, &linkquality) == OT_ERROR_NONE)
    {
        sServer->OutputFormat("LinkQualityInFilter lqin: fixed as %d\r\n", linkquality);
    }
    else
    {
        sServer->OutputFormat("LinkQualityInFilter lqin: no\r\n");
    }
}

}  // namespace Cli
}  // namespace ot
