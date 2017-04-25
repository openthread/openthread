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
 *   This file includes definitions for manipulating local Thread Network Data.
 */

#ifndef NETWORK_DATA_LOCAL_HPP_
#define NETWORK_DATA_LOCAL_HPP_

#include <thread/network_data.hpp>

namespace ot {

class ThreadNetif;

namespace NetworkData {

class Local
{
public:
    explicit Local(ThreadNetif &) { }

    void Clear(void) { }

    ThreadError AddOnMeshPrefix(const uint8_t *, uint8_t, int8_t, uint8_t, bool) { return kThreadError_NotImplemented; }
    ThreadError RemoveOnMeshPrefix(const uint8_t *, uint8_t) { return kThreadError_NotImplemented; }

    ThreadError AddHasRoutePrefix(const uint8_t *, uint8_t, int8_t, bool) { return kThreadError_NotImplemented; }
    ThreadError RemoveHasRoutePrefix(const uint8_t *, uint8_t) { return kThreadError_NotImplemented; }

    ThreadError SendServerDataNotification(void) { return kThreadError_NotImplemented; }

    void GetNetworkData(bool, uint8_t *, uint8_t &aDataLength) { aDataLength = 0; }
    ThreadError GetNextOnMeshPrefix(otNetworkDataIterator *, otBorderRouterConfig *) { return kThreadError_NotFound; }
    void ClearResubmitDelayTimer(void) { }

};

}  // namespace NetworkData
}  // namespace ot

#endif  // NETWORK_DATA_LOCAL_HPP_
