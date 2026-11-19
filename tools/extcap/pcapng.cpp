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

#include "pcapng.hpp"

#include <errno.h>

#include "common/code_utils.hpp"
#include "lib/pcapng/pcapng.hpp"

namespace ot {
namespace Extcap {

PcapngWriter::PcapngWriter(void)
    : mFile(nullptr)
{
}

PcapngWriter::~PcapngWriter(void) { Close(); }

otError PcapngWriter::Open(const char *aFilename)
{
    uint8_t         block[Pcapng::Encoder::kSectionHeaderBlockSize + Pcapng::Encoder::kInterfaceDescriptionBlockSize];
    Pcapng::Encoder encoder(block, sizeof(block));
    otError         error = OT_ERROR_NONE;

    Close();

    VerifyOrExit(aFilename != nullptr, error = OT_ERROR_INVALID_ARGS);

    mFile = fopen(aFilename, "wb");
    VerifyOrExit(mFile != nullptr, error = OT_ERROR_FAILED);

    SuccessOrExit(error = encoder.AppendSectionHeader());
    SuccessOrExit(error = encoder.AppendInterfaceDescription(Pcapng::kLinkTypeIeee802154Tap));
    error = Write(block, encoder.GetLength());

exit:
    if (error != OT_ERROR_NONE)
    {
        int savedErrno = errno;

        Close();
        errno = savedErrno;
    }
    return error;
}

void PcapngWriter::Close(void)
{
    if (mFile != nullptr)
    {
        fclose(mFile);
        mFile = nullptr;
    }
}

otError PcapngWriter::WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs, int8_t aRssi, uint8_t aLqi)
{
    uint8_t         block[Pcapng::Encoder::kMaxIeee802154FrameBlockSize];
    Pcapng::Encoder encoder(block, sizeof(block));
    otError         error = OT_ERROR_NONE;

    VerifyOrExit(mFile != nullptr, error = OT_ERROR_INVALID_STATE);

    // Hardware RCPs only report frames which passed the FCS check, but some radio drivers overwrite the FCS bytes with
    // driver metadata or leave them uninitialized, so recompute the FCS for Wireshark.
    SuccessOrExit(error =
                      encoder.AppendIeee802154Frame(kInterfaceId, aTimeUs, aFrame, aRssi, aLqi, Pcapng::kFcsRecompute));
    error = Write(block, encoder.GetLength());

exit:
    return error;
}

otError PcapngWriter::Write(const uint8_t *aData, size_t aLength)
{
    otError error = OT_ERROR_NONE;

    // Write each block with a single `fwrite()` so that a FIFO reader never sees a partial block.
    VerifyOrExit(fwrite(aData, 1, aLength, mFile) == aLength && fflush(mFile) == 0, error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        // Preserve errno (e.g. EPIPE when Wireshark closes the FIFO) for the caller.
        int savedErrno = errno;

        Close();
        errno = savedErrno;
    }
    return error;
}

} // namespace Extcap
} // namespace ot
