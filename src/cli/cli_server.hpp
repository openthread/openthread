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
 *   This file contains definitions for adding a CLI command to the CLI server.
 */

#ifndef CLI_SERVER_HPP_
#define CLI_SERVER_HPP_

#include <openthread-types.h>

namespace Thread {
namespace Cli {

/**
 * This class implements the CLI server.
 *
 */
class Server
{
public:
    /**
     * This method delivers raw characters to the client.
     *
     * @param[in]  aBuf        A pointer to a buffer.
     * @param[in]  aBufLength  Number of bytes in the buffer.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    virtual int Output(const char *aBuf, uint16_t aBufLength) = 0;

    /**
     * This method delivers formatted output to the client.
     *
     * @param[in]  aFmt  A pointer to the format string.
     * @param[in]  ...   A variable list of arguments to format.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    virtual int OutputFormat(const char *fmt, ...) = 0;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_SERVER_HPP_
