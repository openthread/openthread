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

#ifndef OT_NEXUS_PLATFORM_NEXUS_RADIO_MODEL_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_RADIO_MODEL_HPP_

#include <stdint.h>

namespace ot {
namespace Nexus {

class Node;

/**
 * This class implements a radio model for RSSI calculation.
 *
 */
class RadioModel
{
public:
    static constexpr double kPathLossConstant = 40.0;
    static constexpr double kPathLossExponent = 20.0;

    RadioModel(void) = delete;

    /**
     * This static method calculates the RSSI between two nodes based on their distance.
     *
     * @param[in] aTxNode  The transmitter node.
     * @param[in] aRxNode  The receiver node.
     *
     * @returns The calculated RSSI in dBm.
     */
    static int16_t CalculateRssi(const Node &aTxNode, const Node &aRxNode);

    /**
     * This static method determines if a packet should be dropped based on its RSSI.
     *
     * @param[in] aRssi  The RSSI of the packet.
     *
     * @retval true if the packet should be dropped.
     * @retval false if the packet should not be dropped.
     */
    static bool ShouldDropPacket(int16_t aRssi);
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_RADIO_MODEL_HPP_
