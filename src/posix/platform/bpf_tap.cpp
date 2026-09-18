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

#ifndef __linux__

#include <errno.h>
#include <fcntl.h>
#include <net/bpf.h>
#include <net/if.h>
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

    // The BPF device does not clone on every platform: take the first free unit.
    for (uint16_t unit = 0; unit < kMaxUnits && mFd < 0; unit++)
    {
        char path[sizeof("/dev/bpf65535")];

        snprintf(path, sizeof(path), "/dev/bpf%u", unit);
        mFd = open(path, O_RDWR | O_CLOEXEC);

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

    // Frames written to the tap carry their own link-layer header.
    value = 1;
    VerifyOrExit(ioctl(mFd, BIOCSHDRCMPLT, &value) == 0, error = OT_ERROR_FAILED);

    value = aSeeSent ? 1 : 0;
    VerifyOrExit(ioctl(mFd, BIOCSSEESENT, &value) == 0, error = OT_ERROR_FAILED);

    VerifyOrExit(ioctl(mFd, BIOCGDLT, &mDataLinkType) == 0, error = OT_ERROR_FAILED);
    VerifyOrExit(GetLinkHeaderLength() > 0 || mDataLinkType == DLT_NULL, error = OT_ERROR_NOT_CAPABLE);

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
    size_t  offset = 0;

    VerifyOrExit(IsOpen(), error = OT_ERROR_INVALID_STATE);

    count = read(mFd, mBuffer, mBufferLength);

    if (count < 0)
    {
        VerifyOrExit(errno == EAGAIN || errno == EINTR, error = OT_ERROR_FAILED);
        ExitNow();
    }

    while (offset + sizeof(struct bpf_hdr) <= static_cast<size_t>(count))
    {
        struct bpf_hdr header;
        size_t         frameOffset;

        memcpy(&header, mBuffer + offset, sizeof(header));
        frameOffset = offset + header.bh_hdrlen;
        VerifyOrExit(frameOffset + header.bh_caplen <= static_cast<size_t>(count), error = OT_ERROR_PARSE);

        aHandler(aContext, mBuffer + frameOffset, static_cast<uint16_t>(header.bh_caplen));
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

#endif // __linux__
