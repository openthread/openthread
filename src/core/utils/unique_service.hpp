/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for unique service.
 */

#ifndef UNIQUE_SERVICE_HPP_
#define UNIQUE_SERVICE_HPP_

#include <openthread/error.h>
#include <openthread/server.h>

#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "thread/network_data_local.hpp"

namespace ot {
namespace Utils {

/**
 * This class represents metadata used to manage the service.
 *
 */
OT_TOOL_PACKED_BEGIN
class ServiceMetadata
{
public:
    enum State
    {
        kStateIdle,              ///< No action.
        kStateRegisterService,   ///< Waiting to send service data notification to the leader to register service.
        kStateUnregisterService, ///< Waiting to send service data notification to the leader to unregister service.
        kStateDeleteService,     ///< Waiting to delete the service from Unique Network Data and unregister service.
    };

    /**
     * Default constructor for the object.
     *
     */
    ServiceMetadata(void)
        : mState(kStateIdle)
        , mTimeout(0)
        , mServerCompareCallback(NULL){};

    /**
     * Set the current service state.
     *
     * @param[in]  aState  The state of the current service.
     *
     */
    void SetState(uint8_t aState) { mState = aState; }

    /**
     * Get the state of current service.
     *
     * @returns The service state.
     */
    uint8_t GetState(void) { return mState; }

    /**
     * Set the timeout value of the service.
     *
     * @param[in]  aTimeout  The timeout value.
     *
     */
    void SetTimeout(uint8_t aTimeout) { mTimeout = aTimeout; }

    /**
     * Get the timeout value of the service.
     *
     * @returns The timeout value.
     */
    uint8_t GetTimeout(void) { return mTimeout; }

    /**
     * Set the callback function to compare servers provided the same service.
     *
     * @param[in]  aCallback  A pointer to a callback function for comparing servers.
     *
     */
    void SetServerCompareCallback(otServerCompareCallback aCallback) { mServerCompareCallback = aCallback; }

    /**
     * Get the service server comparation callback function.
     *
     * @returns The pointer to a callback function for comparing servers.
     *
     */
    otServerCompareCallback GetServerCompareCallback(void) { return mServerCompareCallback; }

private:
    uint8_t                 mState;
    uint8_t                 mTimeout;
    otServerCompareCallback mServerCompareCallback;
} OT_TOOL_PACKED_END;

/**
 * This class controls the process of service registeration to ensure that only one server provides the service when
 * multiple servers register the same service.
 *
 */
class UniqueService : public InstanceLocator
{
public:
    /**
     * This constructor initializes the Unique Service object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit UniqueService(Instance &aInstance);

    /**
     * This method adds a service entry to the Unique Network Data. If the Leader Network Data doesn't contains this
     * service, the server will send a service data notification message to the Leader to register the service.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number (IANA-assigned) for Service TLV
     * @param[in]  aServiceData       A pointer to the Service Data
     * @param[in]  aServiceDataLength The length of @p aServiceData in bytes.
     * @param[in]  aServerStable      The Stable flag value for Server TLV
     * @param[in]  aServerData        A pointer to the Server Data
     * @param[in]  aServerDataLength  The length of @p aServerData in bytes.
     * @param[in]  aCallback          A pointer to a function for comparing servers provided the same service.
     *
     * @retval OT_ERROR_NONE     Successfully added the Service entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the Service entry.
     *
     */
    otError AddService(uint32_t                aEnterpriseNumber,
                       const uint8_t *         aServiceData,
                       uint8_t                 aServiceDataLength,
                       bool                    aServerStable,
                       const uint8_t *         aServerData,
                       uint8_t                 aServerDataLength,
                       otServerCompareCallback aCallback);

    /**
     * This method removes a service entry from the Unique Network Data. If the server is in the server list of the
     * service in the Leader Network Data, it sends a service data notification message to the Leader to unregister
     * the service.
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

    /**
     * This method provides the next service in the Unique Network Data.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[out]    aConfig    A pointer to where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig);

    /**
     * This method provides the next unique service exists in both Unique Network Data and Leader Network Data.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[out]    aConfig    A pointer to where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Leader Network Data.
     *
     */
    otError GetNextLeaderService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig);

private:
    enum
    {
        kMaxRegisterServiceDelay = 120, ///< Maximum delay before sending server data notification message to the
                                        ///< leader to register the service (seconds)
        kMaxUnregisterServiceDelay = 5, ///< Maximum delay before sending server data notification message to the
                                        ///< leader to unregister the service (seconds)
        kStateUpdatePeriod = 1000u,     ///< State update period in milliseconds.
    };

    otError GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig, bool aMetadata);

    otError AddNetworkDataLocalService(otServiceConfig &aConfig);
    otError RemoveNetworkDataLocalService(otServiceConfig &aConfig);
    otError AddNetworkDataUniqueService(otServiceConfig &aConfig);
    otError RemoveNetworkDataUniqueService(otServiceConfig &aConfig);
    otError UpdateNetworkDataUniqueService(otServiceConfig &aConfig);
    otError FindNetworkDataUniqueService(uint32_t         aEnterpriseNumber,
                                         const uint8_t *  aServiceData,
                                         uint8_t          aServiceDataLength,
                                         otServiceConfig &aConfig);

    bool             NetworkDataLeaderContainsService(const otServiceConfig &aConfig);
    bool             ServiceCompare(const otServiceConfig &aServiceA, const otServiceConfig &aServiceB) const;
    static int       DefaultServerCompare(const otServerConfig *aServerA, const otServerConfig *aServerB);
    ServiceMetadata *GetServiceMetadata(otServiceConfig &aConfig);

    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    TimerMilli mTimer;

    NetworkData::Local mNetworkData; ///< The Unique Network Data.
    Notifier::Callback mNotifierCallback;
};

} // namespace Utils
} // namespace ot

#endif // UNIQUE_SERVICE_HPP_
