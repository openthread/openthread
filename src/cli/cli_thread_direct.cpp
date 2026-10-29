/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements CLI commands for Thread Direct.
 */

#include "cli_thread_direct.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

ThreadDirect::ThreadDirect(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
{
}

const otThreadDirectKey *ThreadDirect::FindKeyAtIndex(uint16_t aIndex)
{
    const otThreadDirectKey  *key;
    otThreadDirectKeyIterator iterator;

    otThreadDirectInitKeyIterator(GetInstancePtr(), &iterator);

    for (uint16_t i = 0; i <= aIndex; i++)
    {
        key = otThreadDirectGetNextKey(&iterator);
        VerifyOrExit(key != nullptr);
    }

exit:
    return key;
}

template <> otError ThreadDirect::Process<Cmd("key")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli td key
     * @code
     * td key
     * | Index | Key                              | Key ID     | FrameCntr  |
     * +-------+----------------------------------+------------+------------+
     * |     0 | 76ea9298480c5d4348ba966a6fbfc80a | 1200340056 | 0x00000000 |
     * |     1 | 4754ad204bc15dfbaf2bf98f4f8e78f1 | 0011223344 | 0x00000000 |
     * |     2 | 4754ad204bc15dfbaf2bf98f4f8e78f1 | 8cf67543be | 0x00000000 |
     * |     3 | 7566e59e18575b5e103daceeed7cc3c9 | 5fcfcdfe00 | 0x00000000 |
     * Done
     * @endcode
     * @cparam td key [table]
     * @par
     * Prints the registered Thread Direct keys in a table format.
     * When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, the `KeyRef` column is printed instead of
     * `Key`.
     * @sa otThreadDirectGetNextKey
     */
    if (aArgs[0].IsEmpty() || aArgs[0] == "table")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        OutputKeyTable();
    }
    /**
     * @cli td key register
     * @code
     * td key register 7566e59e18575b5e103daceeed7cc3c9 5fcfcdfe00
     * Done
     * @endcode
     * @cparam td key register @ca{key|keyref} @ca{key-id}
     * * `key|keyref`: When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is disabled, specify the 16-byte key as
     *   a hex string. When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, specify the `otCryptoKeyRef`
     *   as a `uint32_t` value.
     * * `key-id`: The 5-byte Thread Direct Key ID as a hex string.
     * @par
     * Registers a Thread Direct key and its associated Key ID.
     * @sa otThreadDirectRegisterKey
     */
    else if (aArgs[0] == "register")
    {
        otThreadDirectKeyMaterial keyMaterial;
        otThreadDirectKeyId       keyId;

#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        SuccessOrExit(error = aArgs[1].ParseAsHexString(keyMaterial.mKeyMaterial.mKey.m8));
#else
        SuccessOrExit(error = aArgs[1].ParseAsUint32(keyMaterial.mKeyMaterial.mKeyRef));
#endif

        SuccessOrExit(error = aArgs[2].ParseAsHexString(keyId.m8));
        VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        if (otThreadDirectRegisterKey(GetInstancePtr(), &keyMaterial, &keyId) == nullptr)
        {
            error = OT_ERROR_FAILED;
        }
    }
    /**
     * @cli td key unregister
     * @code
     * td key unregister 2
     * Done
     * @endcode
     * @cparam td key unregister @ca{index}
     * * `index`: The index of the key in the `td key` table to unregister.
     * @par
     * Unregisters the Thread Direct key at the given table index.
     * @sa otThreadDirectUnregisterKey
     */
    else if (aArgs[0] == "unregister")
    {
        uint16_t                 index;
        const otThreadDirectKey *key;

        SuccessOrExit(error = aArgs[1].ParseAsUint16(index));
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        key = FindKeyAtIndex(index);
        VerifyOrExit(key != nullptr, error = OT_ERROR_NOT_FOUND);

        otThreadDirectUnregisterKey(GetInstancePtr(), key);
    }
    /**
     * @cli td key unregisterall
     * @code
     * td key unregisterall
     * Done
     * @endcode
     * @par
     * Unregisters all registered Thread Direct keys.
     * @sa otThreadDirectUnregisterAllKeys
     */
    else if (aArgs[0] == "unregisterall")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        otThreadDirectUnregisterAllKeys(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void ThreadDirect::OutputKeyTable(void)
{
#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    static const char *const kTableTitles[]       = {"Index", "Key", "Key ID", "FrameCntr"};
    static const uint8_t     kTableColumnWidths[] = {7, 34, 12, 12};
#else
    static const char *const kTableTitles[]       = {"Index", "KeyRef", "Key ID", "FrameCntr"};
    static const uint8_t     kTableColumnWidths[] = {7, 12, 12, 12};
#endif

    const otThreadDirectKey  *key;
    otThreadDirectKeyIterator iterator;

    OutputTableHeader(kTableTitles, kTableColumnWidths);

    otThreadDirectInitKeyIterator(GetInstancePtr(), &iterator);

    for (uint16_t index = 0; (key = otThreadDirectGetNextKey(&iterator)) != nullptr; index++)
    {
        OutputFormat("| %5u | ", index);
#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        OutputBytes(otThreadDirectKeyGetMaterial(key)->mKeyMaterial.mKey.m8);
#else
        OutputFormat("0x%08lx", ToUlong(otThreadDirectKeyGetMaterial(key)->mKeyMaterial.mKeyRef));
#endif
        OutputFormat(" | ");
        OutputBytes(otThreadDirectKeyGetId(key)->m8);
        OutputLine(" | 0x%08lx |", ToUlong(otThreadDirectKeyGetFrameCounter(key)));
    }
}

otError ThreadDirect::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString) {aCommandString, &ThreadDirect::Process<Cmd(aCommandString)>}

    static constexpr Command kCommands[] = {
        CmdEntry("key"),
    };

#undef CmdEntry

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

} // namespace Cli
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
