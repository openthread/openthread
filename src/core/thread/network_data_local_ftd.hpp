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

/**
 * @addtogroup core-netdata-local
 *
 * @brief
 *   This module includes definitions for manipulating local Thread Network Data.
 *
 * @{
 */

namespace NetworkData {

/**
 * This class implements the Thread Network Data contributed by the local device.
 *
 */
class Local: public NetworkData
{
public:
    /**
     * This constructor initializes the local Network Data.
     *
     * @param[in]  aNetif  A reference to the Thread network interface.
     *
     */
    explicit Local(ThreadNetif &aNetif);

    /**
     * This method adds a Border Router entry to the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     * @param[in]  aPrf           The preference value.
     * @param[in]  aFlags         The Border Router Flags value.
     * @param[in]  aStable        The Stable value.
     *
     * @retval kThreadError_None        Successfully added the Border Router entry.
     * @retval kThreadError_NoBufs      Insufficient space to add the Border Router entry.
     * @retval kThreadError_InvalidArgs The prefix is mesh local prefix.
     *
     */
    ThreadError AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, uint8_t aFlags,
                                bool aStable);

    /**
     * This method removes a Border Router entry from the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     *
     * @retval kThreadError_None      Successfully removed the Border Router entry.
     * @retval kThreadError_NotFound  Could not find the Border Router entry.
     *
     */
    ThreadError RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method adds a Has Route entry to the Thread Network data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     * @param[in]  aPrf           The preference value.
     * @param[in]  aStable        The Stable value.
     *
     * @retval kThreadError_None    Successfully added the Has Route entry.
     * @retval kThreadError_NoBufs  Insufficient space to add the Has Route entry.
     *
     */
    ThreadError AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable);

    /**
     * This method removes a Border Router entry from the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     *
     * @retval kThreadError_None      Successfully removed the Border Router entry.
     * @retval kThreadError_NotFound  Could not find the Border Router entry.
     *
     */
    ThreadError RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method sends a Server Data Notification message to the Leader.
     *
     * @retval kThreadError_None    Successfully enqueued the notification message.
     * @retval kThreadError_NoBufs  Insufficient message buffers to generate the notification message.
     *
     */
    ThreadError SendServerDataNotification(void);

private:
    ThreadError UpdateRloc(void);
    ThreadError UpdateRloc(PrefixTlv &aPrefix);
    ThreadError UpdateRloc(HasRouteTlv &aHasRoute);
    ThreadError UpdateRloc(BorderRouterTlv &aBorderRouter);

    bool IsOnMeshPrefixConsistent(void);
    bool IsExternalRouteConsistent(void);

    uint16_t mOldRloc;
};

}  // namespace NetworkData

/**
 * @}
 */

}  // namespace ot

#endif  // NETWORK_DATA_LOCAL_HPP_
