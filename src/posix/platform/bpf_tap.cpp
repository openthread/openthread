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

#include "posix/platform/bpf_tap.hpp"

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && !defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "common/code_utils.hpp"

namespace ot {
namespace Posix {

BpfTap::BpfTap(void)
    : mFd(-1)
    , mDataLinkType(0)
    , mBufferLength(0)
    , mBuffer(nullptr)
{
}

BpfTap::~BpfTap(void) { Close(); }

otError BpfTap::Open(const char *aIfName, bool aSeeSent)
{
    otError      error = OT_ERROR_NONE;
    int          savedErrno;
    struct ifreq ifr;
    unsigned int value;

    VerifyOrExit(!IsOpen(), error = OT_ERROR_ALREADY);

    // The cloning device where the platform has one (macOS, FreeBSD), else the first free unit. Non-blocking: the
    // mainloop must not stall on a wakeup without data.
    mFd = open("/dev/bpf", O_RDWR | O_CLOEXEC | O_NONBLOCK);

    for (uint16_t unit = 0; unit < kMaxUnits && mFd < 0; unit++)
    {
        char path[sizeof("/dev/bpf65535")];

        snprintf(path, sizeof(path), "/dev/bpf%u", unit);
        mFd = open(path, O_RDWR | O_CLOEXEC | O_NONBLOCK);

        if (mFd < 0 && errno != EBUSY)
        {
            break;
        }
    }
    VerifyOrExit(mFd >= 0, error = OT_ERROR_FAILED);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, aIfName, sizeof(ifr.ifr_name) - 1);
    VerifyOrExit(ioctl(mFd, BIOCSETIF, &ifr) == 0, error = OT_ERROR_FAILED);

    // Deliver frames as they arrive rather than when the buffer fills.
    value = 1;
    VerifyOrExit(ioctl(mFd, BIOCIMMEDIATE, &value) == 0, error = OT_ERROR_FAILED);

    value = aSeeSent ? 1 : 0;
    VerifyOrExit(ioctl(mFd, BIOCSSEESENT, &value) == 0, error = OT_ERROR_FAILED);

    VerifyOrExit(ioctl(mFd, BIOCGDLT, &mDataLinkType) == 0, error = OT_ERROR_FAILED);
    VerifyOrExit(GetLinkHeaderLength() > 0, error = OT_ERROR_NOT_CAPABLE);

    // Frames written to an Ethernet tap carry their own header. For a tunnel (DLT_NULL) the platforms differ: on
    // macOS a "complete" write is the one that bypasses the framer, which would otherwise prepend a second address
    // family word (bpfwrite() in xnu bsd/net/bpf.c); on FreeBSD some tun drivers reject a "complete" write
    // (EAFNOSUPPORT), while a plain write takes the family from the word the frame starts with.
#if defined(__APPLE__)
    value = 1;
#else
    value = (mDataLinkType == DLT_EN10MB) ? 1 : 0;
#endif
    VerifyOrExit(ioctl(mFd, BIOCSHDRCMPLT, &value) == 0, error = OT_ERROR_FAILED);

    VerifyOrExit(ioctl(mFd, BIOCGBLEN, &mBufferLength) == 0, error = OT_ERROR_FAILED);
    mBuffer = static_cast<uint8_t *>(malloc(mBufferLength));
    VerifyOrExit(mBuffer != nullptr, error = OT_ERROR_NO_BUFS);

exit:
    if (error != OT_ERROR_NONE && error != OT_ERROR_ALREADY)
    {
        savedErrno = errno;
        Close();
        errno = savedErrno;
    }

    return error;
}

void BpfTap::Close(void)
{
    if (mFd >= 0)
    {
        close(mFd);
        mFd = -1;
    }

    free(mBuffer);
    mBuffer       = nullptr;
    mDataLinkType = 0;
    mBufferLength = 0;
}

uint16_t BpfTap::GetLinkHeaderLength(void) const
{
    uint16_t length;

    switch (mDataLinkType)
    {
    case DLT_NULL:
        length = 4; // address family, in host byte order
        break;
    case DLT_EN10MB:
        length = 14;
        break;
    default:
        length = 0;
        break;
    }

    return length;
}

otError BpfTap::SetFilter(const struct bpf_insn *aInstructions, uint16_t aCount)
{
    otError            error = OT_ERROR_NONE;
    struct bpf_program program;

    VerifyOrExit(IsOpen(), error = OT_ERROR_INVALID_STATE);

    program.bf_len   = aCount;
    program.bf_insns = const_cast<struct bpf_insn *>(aInstructions);
    VerifyOrExit(ioctl(mFd, BIOCSETF, &program) == 0, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError BpfTap::Read(FrameHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;
    ssize_t count;

    VerifyOrExit(IsOpen(), error = OT_ERROR_INVALID_STATE);

    count = read(mFd, mBuffer, mBufferLength);

    if (count < 0)
    {
        VerifyOrExit(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR, error = OT_ERROR_FAILED);
        ExitNow();
    }

    error = ParseBuffer(mBuffer, static_cast<size_t>(count), aHandler, aContext);

exit:
    return error;
}

otError BpfTap::ParseBuffer(const uint8_t *aBuffer, size_t aLength, FrameHandler aHandler, void *aContext)
{
    // The kernel pads the record header so that the frame behind it is word-aligned: `bh_hdrlen` is the fields
    // (18 bytes) plus that padding, and can be smaller than `sizeof(struct bpf_hdr)`, which has its own tail padding
    // (macOS: 18 on Ethernet, 20 on a tunnel).
    constexpr size_t kMinHeaderLength = offsetof(struct bpf_hdr, bh_hdrlen) + sizeof(u_short);

    otError error  = OT_ERROR_NONE;
    size_t  offset = 0;

    while (offset + kMinHeaderLength <= aLength)
    {
        struct bpf_hdr header;
        size_t         frameOffset;

        memset(&header, 0, sizeof(header));
        memcpy(&header, aBuffer + offset, kMinHeaderLength);
        VerifyOrExit(header.bh_hdrlen >= kMinHeaderLength && header.bh_hdrlen <= aLength - offset,
                     error = OT_ERROR_PARSE);
        frameOffset = offset + header.bh_hdrlen;
        VerifyOrExit(header.bh_caplen <= aLength - frameOffset, error = OT_ERROR_PARSE);

        // A capture shorter than the frame is not a frame to forward.
        if (header.bh_caplen == header.bh_datalen && header.bh_caplen <= UINT16_MAX)
        {
            aHandler(aContext, aBuffer + frameOffset, static_cast<uint16_t>(header.bh_caplen));
        }

        offset += BPF_WORDALIGN(header.bh_hdrlen + header.bh_caplen);
    }

exit:
    return error;
}

otError BpfTap::Write(const uint8_t *aFrame, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsOpen(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(write(mFd, aFrame, aLength) == static_cast<ssize_t>(aLength), error = OT_ERROR_FAILED);

exit:
    return error;
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && !defined(__linux__)
