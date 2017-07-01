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
 *   This file contains definitions for the CLI MacFilter interpreter.
 */

#ifndef CLI_MACFILTER_HPP_
#define CLI_MACFILTER_HPP_

#include <openthread/types.h>

#include "cli/cli_server.hpp"

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI MacFilter interpreter.
 *
 */
class MacFilter
{
public:
    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     * @param[in]  argc       The number of elements in argv.
     * @param[in]  argv       A pointer to an array of command line arguments.
     * @param[in]  aServer    A reference to the CLI server.
     *
     */
    static otError Process(otInstance *aInstance, int argc, char *argv[], Server &aServer);

private:
    static void OutputExtAddress(const otExtAddress *aAddress);
    static void PrintFilter();
    static void PrintLinkQualityInFilter();
    static void PrintAddressFilter();

    static otError ProcessAddressFilter(int argc, char *argv[]);
    static otError ProcessLinkQualityInFilter(int argc, char *argv[]);

    static Server *sServer;
    static otInstance *sInstance;
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_MACFILTER_HPP_
