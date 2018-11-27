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
 *   This file includes definitions for unique service manager.
 */

#ifndef UNIQUE_SERVICE_MANAGER_HPP_
#define UNIQUE_SERVICE_MANAGER_HPP_

#include "openthread-core-config.h"

#include <openthread/error.h>
#include <openthread/server.h>

#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "thread/network_data_local.hpp"

#if OPENTHREAD_ENABLE_SERVICE && OPENTHREAD_ENABLE_UNIQUE_SERVICE

namespace ot {
namespace Utils {

/**
 * @addtogroup utils-unique-service-manager
 *
 * @brief
 *   This module includes definitions for unique service which ensures only one server provide the service.
 *
 * @{
 */

/**
 * This class implements ServiceEntry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ServiceEntry
{
public:
    /**
     * This method initializes the entry.
     *
     * @param[in]  aConfig           A pointer to the service configuration.
     * @param[in]  aUpdateCallback   A pointer to a function that is called when the server changes.
     * @param[in]  aCompareCallback  A pointer to a function for comparing servers.
     * @param[in]  aContext          A pointer to application-specific context.
     *
     * @retval OT_ERROR_NONE     Successfully initialized the entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to initialize the service configuration.
     */
    otError Init(const otServiceConfig & aConfig,
                 otServiceUpdateCallback aUpdateCallback,
                 otServerCompareCallback aCompareCallback,
                 void *                  aContext);

    /**
     * This method returns the size of the entry.
     *
     * @returns The size of the entry.
     *
     */
    uint8_t GetSize(void) const { return sizeof(*this) - sizeof(mData) + mServiceDataLength + mServerDataLength; }

    /**
     * This method indicates whether or not the entry matched the given service.
     *
     * @retval TRUE   If the entry matches the given service.
     * @retval FALSE  If the entry doesn't match the given service.
     *
     */
    bool MatchService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
    {
        return (aEnterpriseNumber == mEnterpriseNumber) && (aServiceDataLength == GetServiceDataLength()) &&
               (memcmp(aServiceData, GetServiceData(), aServiceDataLength) == 0);
    }

    /**
     * This method returns the service configuration.
     *
     * @returns The service configuration structure.
     *
     */
    otServiceConfig GetServiceConfig(void);

    /**
     * This method returns the enterprise number of the service.
     *
     * @returns The enterprise number value.
     *
     */
    uint32_t GetEnterpriseNumber(void) const { return mEnterpriseNumber; }

    /**
     * This method returns the service data length.
     *
     * @returns The service data length value.
     *
     */
    uint8_t GetServiceDataLength(void) const { return mServiceDataLength; }

    /**
     * This method returns a pointer to the service data.
     *
     * @returns A pointer to the service data.
     *
     */
    const uint8_t *GetServiceData(void) const { return mData; }

    /**
     * This method returns the server data length.
     *
     * @returns The server data length value.
     *
     */
    uint8_t GetServerDataLength(void) const { return mServerDataLength; }

    /**
     * This method returns a pointer to the server data.
     *
     * @returns A pointer to the server data.
     *
     */
    const uint8_t *GetServerData(void) const { return mData + mServiceDataLength; }

    /**
     * This method returns a pointer to a function that is called when the server changes.
     *
     * @returns A pointer to a function that is called when the server changes.
     *
     */
    otServiceUpdateCallback GetUpdateCallback(void) const { return mUpdateCallback; }

    /**
     * This method returns a pointer to a function for comparing servers.
     *
     * @returns A pointer to a function for comparing servers.
     *
     */
    otServerCompareCallback GetCompareCallback(void) const { return mCompareCallback; }

    /**
     * This method returns a pointer to application-specific context.
     *
     * @returns A pointer to application-specific context.
     *
     */
    void *GetContext(void) const { return mContext; }

private:
    enum
    {
        kDataSize = 255, ///< Maximum total size including service and server data.
    };

    uint32_t                mEnterpriseNumber;
    uint8_t                 mServiceDataLength;
    uint8_t                 mServerDataLength;
    bool                    mServerStable : 1;
    otServiceUpdateCallback mUpdateCallback;
    otServerCompareCallback mCompareCallback;
    void *                  mContext;
    uint8_t                 mData[kDataSize];
} OT_TOOL_PACKED_END;

/**
 * This class implements Service Table processing.
 *
 */
class ServiceTable
{
public:
    enum
    {
        kIteratorInit = 0, ///< Initializer for Iterator.
    };

    typedef uint8_t Iterator; ///< Used to iterate through Service Table.

    /**
     * This constructor initializes the object.
     *
     */
    ServiceTable(void)
        : mLength(0)
    {
    }

    /**
     * This method adds a service entry to the service table.
     *
     * @param[in]  aEntry  A reference the service entry.
     *
     * @retval OT_ERROR_NONE     Successfully added the service entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the service entry.
     *
     */
    otError AddService(const ServiceEntry &aEntry);

    /**
     * This method removes a service entry from the service table.
     *
     * @param[in]  aEnterpriseNumber   A enterprise number (IANA-assigned).
     * @param[in]  aServiceData        A pointer to the service data.
     * @param[in]  aServiceDataLength  The length of @p aServiceData in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed the service entry.
     * @retval OT_ERROR_PARSE      Failed to parse the service entry.
     * @retval OT_ERROR_NOT_FOUND  Could not find the service.
     *
     */
    otError RemoveService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);

    /**
     * This method provides the next service entry in the service table.
     *
     * @param[inout]  aIterator  A reference to the service table iterator context. To get the first
     *                           service entry it should be set to kIteratorInit.
     * @param[out]    aEntry     A reference to where the service entry will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service entry exists in the service table.
     *
     */
    otError GetNextService(Iterator &aIterator, ServiceEntry &aEntry);

    /**
     * This method returns a reference to the service entry for the given service.
     *
     * @param[in]   aEnterpriseNumber   A enterprise number (IANA-assigned).
     * @param[in]   aServiceData        A pointer to the service data
     * @param[in]   aServiceDataLength  The length of @p aServiceData in bytes.
     * @param[out]  aEntry              A reference to where the service entry will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the service entry.
     * @retval OT_ERROR_NOT_FOUND  No matching service exists.
     *
     */
    otError FindService(uint32_t       aEnterpriseNumber,
                        const uint8_t *aServiceData,
                        uint8_t        aServiceDataLength,
                        ServiceEntry & aEntry);

    /**
     * This method indicates whether or not the service table contains the given service.
     *
     * @param[in]  aEnterpriseNumber   A enterprise number (IANA-assigned).
     * @param[in]  aServiceData        A pointer to the service data
     * @param[in]  aServiceDataLength  The length of @p aServiceData in bytes.
     *
     * @returns TRUE if service table contains the given service. FALSE otherwise.
     *
     */
    bool ContainsService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
    {
        return FindService(aEnterpriseNumber, aServiceData, aServiceDataLength) != NULL;
    }

private:
    enum
    {
        kDataSize = OPENTHREAD_CONFIG_UNIQUE_SERVICE_BUFFER_SIZE,
    };

    ServiceEntry *FindService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);

    uint8_t mLength;
    uint8_t mData[kDataSize];
};

/**
 * This class implements the Unique Service Manager.
 *
 */
class UniqueServiceManager : public InstanceLocator
{
public:
    /**
     * This constructor initializes the Unique Service Manager object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit UniqueServiceManager(Instance &aInstance);

    /**
     * This method registers a unique service to Leader.
     *
     * If the Leader Network Data doesn't contains this service, the server will send SVR_DATA.ntf to the Leader to
     * register the service. Otherwise, the server uses the existing service.
     *
     * @param[in]  aConfig                 A pointer to the service configuration.
     * @param[in]  aServiceUpdateCallback  A pointer to a function that is called when the server changes.
     * @param[in]  aServerCompareCallback  A pointer to a function for comparing servers.
     * @param[in]  aContext                A pointer to a user context.
     *
     * @retval OT_ERROR_NONE     Successfully added and registered the service configuration.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the service configuration.
     *
     */
    otError RegisterService(const otServiceConfig * aConfig,
                            otServiceUpdateCallback aServiceUpdateCallback,
                            otServerCompareCallback aServerCompareCallback,
                            void *                  aContext);

    /**
     * This method unregisters a unique service to Leader.
     *
     * If the server provides the service in the Leader Network Data, it sends a SVR_DATA.ntf to the Leader to
     * unregister the service.
     *
     * @param[in]  aEnterpriseNumber   Enterprise Number of the service to be deleted.
     * @param[in]  aServiceData        A pointer to the service data.
     * @param[in]  aServiceDataLength  The length of @p aServiceData in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed and unregistered the service configuration.
     * @retval OT_ERROR_NOT_FOUND  Could not find the service configuration.
     *
     */
    otError UnregisterService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);

    /**
     * This method gets server configuration for the given unique service.
     *
     * @param[in]   aEnterpriseNumber   Enterprise Number of the service to be deleted.
     * @param[in]   aServiceData        A pointer to the service data.
     * @param[in]   aServiceDataLength  The length of @p aServiceData in bytes.
     * @param[out]  aServerConfig       A pointer to where the server configuration will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the server configuration.
     * @retval OT_ERROR_NOT_FOUND  Could not find the given unique service.
     *
     */
    otError GetServerConfig(uint32_t        aEnterpriseNumber,
                            const uint8_t * aServiceData,
                            uint8_t         aServiceDataLength,
                            otServerConfig *aServerConfig);

private:
    enum
    {
        kMaxRegisterDelay = 30, ///< Maximum delay before registering the service (seconds).
    };

    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    otError NetworkDataLeaderServiceLookup(ServiceEntry &aEntry, otServiceConfig &aConfig, bool &aRlocIn);
    otError AddNetworkDataLocalService(const otServiceConfig &aConfig);
    otError RemoveNetworkDataLocalService(const otServiceConfig &aConfig);

    static bool DefaultServerCompare(const otServerConfig *aServerA, const otServerConfig *aServerB, void *aContext);

    ServiceTable       mServiceTable;
    TimerMilli         mTimer;
    Notifier::Callback mNotifierCallback;
};

/**
 * @}
 *
 */

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_SERVICE && OPENTHREAD_ENABLE_UNIQUE_SERVICE

#endif // UNIQUE_SERVICE_MANAGER_HPP_
