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

#include "nexus_radio_model.hpp"

#include "nexus_node.hpp"
#include "common/num_utils.hpp"

#include <cmath>

namespace ot {
namespace Nexus {

int16_t RadioModel::CalculateRssi(const Node &aTxNode, const Node &aRxNode)
{
    // Simple path loss model
    // RSSI = TxPower - PathLoss
    // PathLoss = kPathLossExponent * log10(distance) + kPathLossConstant

    double dx       = aTxNode.GetPositionX() - aRxNode.GetPositionX();
    double dy       = aTxNode.GetPositionY() - aRxNode.GetPositionY();
    double distance = std::hypot(dx, dy);

    distance = Max(distance, 1.0);

    // Assume TxPower = 0 dBm
    // RSSI = - (kPathLossConstant + kPathLossExponent * std::log10(distance))

    double rssi = -(kPathLossConstant + kPathLossExponent * std::log10(distance));

    return static_cast<int16_t>(std::round(rssi));
}

bool RadioModel::ShouldDropPacket(int16_t aRssi) { return aRssi < Radio::kRadioSensitivity; }

} // namespace Nexus
} // namespace ot
