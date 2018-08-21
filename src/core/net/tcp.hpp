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
 *   This file includes definitions for parsing TCP header.
 */

#ifndef TCP_HPP_
#define TCP_HPP_

#include "openthread-core-config.h"

#include "net/ip6_headers.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-tcp
 *
 * @brief
 *   This module includes definitions for parsing TCP header
 *
 * @{
 *
 */

OT_TOOL_PACKED_BEGIN
struct TcpHeaderPoD
{
    uint16_t mSource;
    uint16_t mDestination;
    uint32_t mSequenceNumber;
    uint32_t mAckNumber;
    uint16_t mFlags;
    uint16_t mWindow;
    uint16_t mChecksum;
    uint16_t mUrgentPointer;
} OT_TOOL_PACKED_END;

/**
 * This class implements TCP header parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TcpHeader : private TcpHeaderPoD
{
public:
    /**
     * This method returns the TCP Source Port.
     *
     * @returns The TCP Source Port.
     *
     */
    uint16_t GetSourcePort(void) const { return HostSwap16(mSource); }

    /**
     * This method returns the TCP Destination Port.
     *
     * @returns The TCP Destination Port.
     *
     */
    uint16_t GetDestinationPort(void) const { return HostSwap16(mDestination); }

    /**
     * This method returns the TCP Sequence Number.
     *
     * @returns The TCP Sequence Number.
     *
     */
    uint32_t GetSequenceNumber(void) const { return HostSwap32(mSequenceNumber); }

    /**
     * This method returns the TCP Acknowledgment Sequence Number.
     *
     * @returns The TCP Acknowledgment Sequence Number.
     *
     */
    uint32_t GetAcknowledgmentNumber(void) const { return HostSwap32(mAckNumber); }

    /**
     * This method returns the TCP Flags.
     *
     * @returns The TCP Flags.
     *
     */
    uint16_t GetFlags(void) const { return HostSwap16(mFlags); }

    /**
     * This method returns the TCP Window.
     *
     * @returns The TCP Window.
     *
     */
    uint16_t GetWindow(void) const { return HostSwap16(mWindow); }

    /**
     * This method returns the TCP Checksum.
     *
     * @returns The TCP Checksum.
     *
     */
    uint16_t GetChecksum(void) const { return HostSwap16(mChecksum); }

    /**
     * This method returns the TCP Urgent Pointer.
     *
     * @returns The TCP Urgent Pointer.
     *
     */
    uint16_t GetUrgentPointer(void) const { return HostSwap16(mUrgentPointer); }

} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // TCP_HPP_
