/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for cli BLE.
 */

#ifndef CLI_BLE_HPP_
#define CLI_BLE_HPP_

#include <stdint.h>

#include "openthread-core-config.h"

#include <openthread/platform/ble.h>

#include "common/instance.hpp"

#if OPENTHREAD_ENABLE_CLI_BLE && !OPENTHREAD_ENABLE_TOBLE

namespace ot {
namespace Cli {

class Interpreter;

class Ble
{
    friend class Interpreter;

public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    Ble(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

    /**
     * This method prints the UUID to the client.
     *
     * @param[in]  aUuid A pointer to a UUID.
     *
     */
    static void PrintUuid(const otPlatBleUuid *aUuid);

    /**
     * This method reverses the bytes in the buffer.
     *
     * @param[in]  aBuffer   A pointer to a buffer.
     * @param[in]  aLength  The length of the buffer.
     *
     */
    static void ReverseBuf(uint8_t *aBuffer, uint8_t aLength);

    /**
     * This method prints the bytes in the buffer to the client.
     *
     * @param[in]  aBuffer  A pointer to a buffer.
     * @param[in]  aLength  The length of the buffer.
     *
     */
    static void PrintBytes(const uint8_t *aBuffer, uint8_t aLength);

private:
    enum
    {
        kAdvInterval    = 320,  ///< Advertising interval (unit: 0.625ms).
        kScanInterval   = 320,  ///< Scan interval (unit: 0.625ms).
        kScanWindow     = 80,   ///< Scan window (unit: 0.625ms)
        kConnInterval   = 160,  ///< Connection interval (unit: 0.625ms).
        kConnSupTimeout = 60,   ///< Connection establishment supervision timeout (unit: 10ms).
        kAttMtu         = 23,   ///< The ATT_MTU value.
        kL2capMaxMtu    = 1280, ///< The maxmim L2CAP MTU value.
        kL2capPsm       = 0x80, ///< The L2CAP PSM value.
    };

    struct Command
    {
        const char *mName;
        otError (Ble::*mCommand)(int argc, char *argv[]);
    };

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessBdAddr(int argc, char *argv[]);
    otError ProcessDisable(int argc, char *argv[]);
    otError ProcessEnable(int argc, char *argv[]);
    otError ProcessAdvertise(int argc, char *argv[]);
    otError ProcessScan(int argc, char *argv[]);
    otError ProcessConnect(int argc, char *argv[]);
    otError ProcessDisconnect(int argc, char *argv[]);
    otError ProcessL2cap(int argc, char *argv[]);
    otError ProcessGatt(int argc, char *argv[]);

    static Ble &GetOwner(OwnerLocator &aOwnerLocator);

    static void s_HandleScanTimer(Timer &aTimer);
    void        HandleScanTimer(void);

    static const Command sCommands[];

    TimerMilli   mScanTimer;
    Interpreter &mInterpreter;
};
} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#endif // CLI_BLE_HPP_
