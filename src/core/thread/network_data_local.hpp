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

#include "openthread-core-config.h"

#include "thread/network_data.hpp"

namespace ot {

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
class Local : public NetworkData
{
public:
    /**
     * This constructor initializes the local Network Data.
     *
     * @param[in]  aNetif  A reference to the Thread network interface.
     *
     */
    explicit Local(Instance &aInstance);

    /**
     * This method adds a Border Router entry to the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     * @param[in]  aPrf           The preference value.
     * @param[in]  aFlags         The Border Router Flags value.
     * @param[in]  aStable        The Stable value.
     *
     * @retval OT_ERROR_NONE         Successfully added the Border Router entry.
     * @retval OT_ERROR_NO_BUFS      Insufficient space to add the Border Router entry.
     * @retval OT_ERROR_INVALID_ARGS The prefix is mesh local prefix.
     *
     */
    otError AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, uint8_t aFlags, bool aStable);

    /**
     * This method removes a Border Router entry from the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed the Border Router entry.
     * @retval OT_ERROR_NOT_FOUND  Could not find the Border Router entry.
     *
     */
    otError RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method adds a Has Route entry to the Thread Network data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     * @param[in]  aPrf           The preference value.
     * @param[in]  aStable        The Stable value.
     *
     * @retval OT_ERROR_NONE     Successfully added the Has Route entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the Has Route entry.
     *
     */
    otError AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable);

    /**
     * This method removes a Border Router entry from the Thread Network Data.
     *
     * @param[in]  aPrefix        A pointer to the prefix.
     * @param[in]  aPrefixLength  The length of @p aPrefix in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed the Border Router entry.
     * @retval OT_ERROR_NOT_FOUND  Could not find the Border Router entry.
     *
     */
    otError RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

#if OPENTHREAD_ENABLE_SERVICE
    /**
     * This method adds a Service entry to the Thread Network local data.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number (IANA-assigned) for Service TLV
     * @param[in]  aServiceData       A pointer to the Service Data
     * @param[in]  aServiceDataLength The length of @p aServiceData in bytes.
     * @param[in]  aServerStable      The Stable flag value for Server TLV
     * @param[in]  aServerData        A pointer to the Server Data
     * @param[in]  aServerDataLength  The length of @p aServerData in bytes.
     *
     * @retval OT_ERROR_NONE     Successfully added the Service entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the Service entry.
     *
     */
    otError AddService(uint32_t       aEnterpriseNumber,
                       const uint8_t *aServiceData,
                       uint8_t        aServiceDataLength,
                       bool           aServerStable,
                       const uint8_t *aServerData,
                       uint8_t        aServerDataLength);

    /**
     * This method removes a Service entry from the Thread Network local data.
     *
     * @param[in]  aEnterpriseNumber   Enterprise Number of the service to be deleted.
     * @param[in]  aServiceData        A pointer to the service data.
     * @param[in]  aServiceDataLength  The length of @p aServiceData in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed the Border Router entry.
     * @retval OT_ERROR_NOT_FOUND  Could not find the Border Router entry.
     *
     */
    otError RemoveService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);
#endif

    /**
     * This method sends a Server Data Notification message to the Leader.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the notification message.
     * @retval OT_ERROR_NO_BUFS  Insufficient message buffers to generate the notification message.
     *
     */
    otError SendServerDataNotification(void);

private:
    otError UpdateRloc(void);
    otError UpdateRloc(PrefixTlv &aPrefix);
    otError UpdateRloc(HasRouteTlv &aHasRoute);
    otError UpdateRloc(BorderRouterTlv &aBorderRouter);
#if OPENTHREAD_ENABLE_SERVICE
    otError UpdateRloc(ServiceTlv &aService);
    otError UpdateRloc(ServerTlv &aService);
#endif

    bool IsOnMeshPrefixConsistent(void);
    bool IsExternalRouteConsistent(void);
#if OPENTHREAD_ENABLE_SERVICE
    bool IsServiceConsistent(void);
#endif

    uint16_t mOldRloc;
};

} // namespace NetworkData

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_LOCAL_HPP_
