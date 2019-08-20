/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements of MLE device mode.
 */

#include "device_mode.hpp"

#include "common/code_utils.hpp"

namespace ot {
namespace Mle {

void DeviceMode::Get(ModeConfig &aModeConfig) const
{
    aModeConfig.mRxOnWhenIdle       = IsRxOnWhenIdle();
    aModeConfig.mSecureDataRequests = IsSecureDataRequest();
    aModeConfig.mDeviceType         = IsFullThreadDevice();
    aModeConfig.mNetworkData        = IsFullNetworkData();
}

void DeviceMode::Set(const ModeConfig &aModeConfig)
{
    mMode = 0;
    mMode |= aModeConfig.mRxOnWhenIdle ? kModeRxOnWhenIdle : 0;
    mMode |= aModeConfig.mSecureDataRequests ? kModeSecureDataRequest : 0;
    mMode |= aModeConfig.mDeviceType ? kModeFullThreadDevice : 0;
    mMode |= aModeConfig.mNetworkData ? kModeFullNetworkData : 0;
}

DeviceMode::InfoString DeviceMode::ToString(void) const
{
    return InfoString("rx-on:%s sec-poll:%s ftd:%s full-net:%s", IsRxOnWhenIdle() ? "yes" : "no",
                      IsSecureDataRequest() ? "yes" : "no", IsFullThreadDevice() ? "yes" : "no",
                      IsFullNetworkData() ? "yes" : "no");
}

} // namespace Mle
} // namespace ot
