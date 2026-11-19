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

#include "nexus_pcap.hpp"

#include "nexus_utils.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Nexus {

#ifndef __EMSCRIPTEN__

Pcap::Pcap(void)
    : mFile(nullptr)
{
}

Pcap::~Pcap(void) { Close(); }

void Pcap::Open(const char *aFilename)
{
    Pcapng::Encoder encoder(mBlock, sizeof(mBlock));

    Close();

    mFile = fopen(aFilename, "wb");
    VerifyOrExit(mFile != nullptr);

    SuccessOrQuit(encoder.AppendSectionHeader());
    SuccessOrQuit(encoder.AppendInterfaceDescription(Pcapng::kLinkTypeIeee802154Tap));
    SuccessOrQuit(encoder.AppendInterfaceDescription(Pcapng::kLinkTypeEthernet));
    Write(encoder);

exit:
    return;
}

void Pcap::Close(void)
{
    VerifyOrExit(mFile != nullptr);

    fclose(mFile);
    mFile = nullptr;

exit:
    return;
}

void Pcap::WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs, int8_t aRssi, uint8_t aLqi)
{
    Pcapng::Encoder encoder(mBlock, sizeof(mBlock));

    VerifyOrExit(mFile != nullptr);

    // Nexus updates the FCS of every frame it transmits, so it is written as is.
    SuccessOrQuit(
        encoder.AppendIeee802154Frame(kInterfaceIdIeee802154, aTimeUs, aFrame, aRssi, aLqi, Pcapng::kFcsAsIs));
    Write(encoder);

exit:
    return;
}

void Pcap::WritePacket(const InfraIf::LinkLayerAddress &aSrcAddr,
                       const InfraIf::LinkLayerAddress &aDstAddr,
                       const uint8_t                   *aBuffer,
                       uint16_t                         aLength,
                       uint64_t                         aTimeUs)
{
    Pcapng::Encoder        encoder(mBlock, sizeof(mBlock));
    Pcapng::EthernetHeader ethHeader;

    VerifyOrExit(mFile != nullptr);

    VerifyOrQuit(aDstAddr.GetLength() >= sizeof(ethHeader.mDestination),
                 "Destination MAC address length is less than expected");
    memcpy(ethHeader.mDestination, aDstAddr.GetBytes(), sizeof(ethHeader.mDestination));
    VerifyOrQuit(aSrcAddr.GetLength() >= sizeof(ethHeader.mSource), "Source MAC address length is less than expected");
    memcpy(ethHeader.mSource, aSrcAddr.GetBytes(), sizeof(ethHeader.mSource));
    ethHeader.mEtherType = kEtherTypeIp6;

    SuccessOrQuit(encoder.AppendEthernetFrame(kInterfaceIdEthernet, aTimeUs, ethHeader, aBuffer, aLength));
    Write(encoder);

exit:
    return;
}

void Pcap::Write(const Pcapng::Encoder &aEncoder)
{
    VerifyOrExit(fwrite(mBlock, 1, aEncoder.GetLength(), mFile) == aEncoder.GetLength() && fflush(mFile) == 0, Close());

exit:
    return;
}

#else // __EMSCRIPTEN__

Pcap::Pcap(void) {}

Pcap::~Pcap(void) {}

void Pcap::Open(const char *) {}

void Pcap::Close(void) {}

void Pcap::WriteFrame(const otRadioFrame &, uint64_t, int8_t, uint8_t) {}

void Pcap::WritePacket(const InfraIf::LinkLayerAddress &,
                       const InfraIf::LinkLayerAddress &,
                       const uint8_t *,
                       uint16_t,
                       uint64_t)
{
}

#endif // __EMSCRIPTEN__

} // namespace Nexus
} // namespace ot
