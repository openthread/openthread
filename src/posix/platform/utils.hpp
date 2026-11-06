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

#ifndef OT_POSIX_PLATFORM_UTILS_HPP_
#define OT_POSIX_PLATFORM_UTILS_HPP_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef __linux__
#include <linux/netlink.h>
#endif

#include <openthread/error.h>

namespace ot {
namespace Posix {

/**
 * Represents socket block/non-block options.
 */
enum SocketBlockOption : uint8_t
{
    kSocketBlock,
    kSocketNonBlock,
};

/**
 * Creates a socket with SOCK_CLOEXEC flag set.
 *
 * @param[in]   aDomain       The communication domain.
 * @param[in]   aType         The semantics of communication.
 * @param[in]   aProtocol     The protocol to use.
 * @param[in]   aBlockOption  Whether to add nonblock flags.
 *
 * @returns The file descriptor of the created socket, or -1 if fails to create the socket.
 * @retval  -1  Failed to create socket.
 */
int SocketWithCloseExec(int aDomain, int aType, int aProtocol, SocketBlockOption aBlockOption);

/**
 * Formats a system command to execute.
 *
 * @param[in] aFormat  A pointer to the format string.
 * @param[in] ...      Arguments for the format specification.
 *
 * @retval OT_ERROR_NONE    The command was executed successfully.
 * @retval OT_ERROR_FAILED  It failed to execute the command.
 */
otError ExecuteCommand(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

#ifdef __linux__
/**
 * Checks whether a Netlink message header fits within the remaining buffer length.
 *
 * Replaces the kernel `NLMSG_OK()` macro, which compares `__u32 nlmsg_len` directly against the remaining buffer
 * length and triggers `-Wsign-compare` when passing the signed remaining length required by `NLMSG_NEXT()`.
 *
 * @param[in] aHeader  A pointer to the Netlink message header.
 * @param[in] aLen     The remaining number of bytes in the Netlink message buffer (may be negative after `NLMSG_NEXT`).
 *
 * @retval true   The remaining buffer contains a complete, valid Netlink message.
 * @retval false  The remaining buffer is exhausted or the message length is invalid or truncated.
 */
inline bool IsNetlinkMessageOk(const struct nlmsghdr *aHeader, ssize_t aLen)
{
    return (aLen >= static_cast<ssize_t>(sizeof(struct nlmsghdr))) && (aHeader->nlmsg_len >= sizeof(struct nlmsghdr)) &&
           (static_cast<size_t>(aHeader->nlmsg_len) <= static_cast<size_t>(aLen));
}
#endif

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_UTILS_HPP_
