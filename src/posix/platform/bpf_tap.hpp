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
 *   This file includes definitions for a packet tap on a network interface
 *   through the BSD Packet Filter.
 */

#ifndef OT_POSIX_PLATFORM_BPF_TAP_HPP_
#define OT_POSIX_PLATFORM_BPF_TAP_HPP_

#include "openthread-posix-config.h"

#ifndef __linux__

#include <stddef.h>
#include <stdint.h>

#include <openthread/error.h>

#include "core/common/non_copyable.hpp"

struct bpf_insn;

namespace ot {
namespace Posix {

/**
 * Represents a packet tap on one network interface through the BSD Packet
 * Filter.
 *
 * The tap delivers the frames the kernel receives on the interface, and
 * optionally the ones it transmits, and can inject frames into the
 * interface's transmit path. For a tunnel interface that transmit path is
 * the userspace client that owns the tunnel, so writing to the tap hands
 * the frame to that client.
 */
class BpfTap : private NonCopyable
{
public:
    /**
     * Receives one captured frame, link-layer header included.
     *
     * @param[in] aContext  The handler's context.
     * @param[in] aFrame    The frame.
     * @param[in] aLength   The captured length of the frame.
     */
    typedef void (*FrameHandler)(void *aContext, const uint8_t *aFrame, uint16_t aLength);

    BpfTap(void);
    ~BpfTap(void);

    /**
     * Attaches the tap to an interface.
     *
     * @param[in] aIfName   The interface name.
     * @param[in] aSeeSent  Whether frames transmitted by this host are delivered too.
     *
     * @retval OT_ERROR_NONE           The tap is attached.
     * @retval OT_ERROR_ALREADY        The tap is already attached.
     * @retval OT_ERROR_NOT_CAPABLE    The interface's link type is not supported.
     * @retval OT_ERROR_FAILED         A BPF device could not be opened or configured; `errno` is set.
     */
    otError Open(const char *aIfName, bool aSeeSent);

    /**
     * Detaches the tap.
     */
    void Close(void);

    bool     IsOpen(void) const { return mFd >= 0; }
    int      GetFd(void) const { return mFd; }
    uint32_t GetDataLinkType(void) const { return mDataLinkType; }

    /**
     * Returns the length of the link-layer header the interface's frames carry.
     */
    uint16_t GetLinkHeaderLength(void) const;

    /**
     * Installs a packet filter program.
     *
     * @param[in] aInstructions  The program.
     * @param[in] aCount         The number of instructions.
     */
    otError SetFilter(const struct bpf_insn *aInstructions, uint16_t aCount);

    /**
     * Reads every frame currently buffered and hands each to @p aHandler.
     */
    otError Read(FrameHandler aHandler, void *aContext);

    /**
     * Injects a frame into the interface's transmit path.
     *
     * @param[in] aFrame   The frame, link-layer header included.
     * @param[in] aLength  The frame length.
     */
    otError Write(const uint8_t *aFrame, uint16_t aLength);

private:
    static constexpr uint16_t kMaxUnits = 256;

    int      mFd;
    uint32_t mDataLinkType;
    uint32_t mBufferLength;
    uint8_t *mBuffer;
};

} // namespace Posix
} // namespace ot

#endif // __linux__

#endif // OT_POSIX_PLATFORM_BPF_TAP_HPP_
