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
 *   This file includes definitions for Thread global IPv6 address configuration with SLAAC.
 */

#ifndef SLAAC_ADDRESS_HPP_
#define SLAAC_ADDRESS_HPP_

#include "openthread-core-config.h"

#include <openthread/ip6.h>

namespace ot {
namespace Utils {

/**
 * @addtogroup core-slaac-address
 *
 * @brief
 *   This module includes definitions for Thread global IPv6 address configuration with SLAAC.
 *
 * @{
 */

/**
 * This class implements the SLAAC utility for Thread protocol.
 *
 */
class Slaac
{
public:
    /**
     * This function pointer is called when SLAAC creates global address.
     *
     * @param[in]     aInstance  A pointer to an OpenThread instance.
     * @param[inout]  aAddress   A pointer to structure containing IPv6 address for which IID is being created.
     * @param[in]     aContext   A pointer to creator-specific context.
     *
     * @retval  OT_ERROR_NONE    if generated valid IID.
     * @retval  OT_ERROR_FAILED  if creating IID failed.
     *
     */
    typedef otError (*IidCreator)(otInstance *aInstance, otNetifAddress *aAddress, void *aContext);

    /**
     * This function update addresses that shall be automatically created using SLAAC.
     *
     * @param[in]     aInstance     A pointer to OpenThread instance.
     * @param[inout]  aAddresses    A pointer to an array containing addresses created by this module.
     * @param[in]     aNumAddresses The number of elements in aAddresses array.
     * @param[in]     aIidCreator   A pointer to function that will be used to create IID for IPv6 addresses.
     * @param[in]     aContext      A pointer to IID creator-specific context data.
     *
     */
    static void UpdateAddresses(otInstance *    aInstance,
                                otNetifAddress *aAddresses,
                                uint32_t        aNumAddresses,
                                IidCreator      aIidCreator,
                                void *          aContext);

    /**
     * This function creates randomly generated IPv6 IID for given IPv6 address.
     *
     * @param[in]     aInstance  A pointer to an OpenThread instance.
     * @param[inout]  aAddress   A pointer to structure containing IPv6 address for which IID is being created.
     * @param[in]     aContext   Unused pointer.
     *
     * @retval  OT_ERROR_NONE  Generated valid IID.
     */
    static otError CreateRandomIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext);
};

/**
 * This class implements the Method for Generating Semantically Opaque IIDs with IPv6 SLAAC (RFC 7217).
 *
 */
class SemanticallyOpaqueIidGenerator : public otSemanticallyOpaqueIidGeneratorData
{
public:
    /**
     * This method creates semantically opaque IID for given IPv6 address and context.
     *
     * The generator starts with DAD counter provided as a class member field. The DAD counter is automatically
     * incremented at most kMaxRetries times if creation of valid IPv6 address fails.
     *
     * @param[in]     aInstance  A pointer to an OpenThread instance.
     * @param[inout]  aAddress   A pointer to structure containing IPv6 address for which IID is being created.
     *
     * @retval  OT_ERROR_NONE                          Generated valid IID.
     * @retval  OT_ERROR_INVALID_ARGS                  Any given parameter is invalid.
     * @retval  OT_ERROR_IP6_ADDRESS_CREATION_FAILURE  Could not generate IID due to RFC 7217 restrictions.
     *
     */
    otError CreateIid(otInstance *aInstance, otNetifAddress *aAddress);

private:
    enum
    {
        kMaxRetries = 255,
    };

    /**
     * This method creates semantically opaque IID for given arguments.
     *
     * This function creates IID only for given DAD counter value.
     *
     * @param[in]     aInstance  A pointer to an OpenThread instance.
     * @param[inout]  aAddress   A pointer to structure containing IPv6 address for which IID is being created.
     *
     * @retval  OT_ERROR_NONE                          Generated valid IID.
     * @retval  OT_ERROR_INVALID_ARGS                  Any given parameter is invalid.
     * @retval  OT_ERROR_IP6_ADDRESS_CREATION_FAILURE  Could not generate IID due to RFC 7217 restrictions.
     *
     */
    otError CreateIidOnce(otInstance *aInstance, otNetifAddress *aAddress);

    /**
     * This method checks if created IPv6 address is already registered in the Thread interface.
     *
     * @param[in]  aInstance        A pointer to an OpenThread instance.
     * @param[in]  aCreatedAddress  A pointer to created IPv6 address.
     *
     * @retval  true   Given address is present in the address list.
     * @retval  false  Given address is missing in the address list.
     *
     */
    bool IsAddressRegistered(otInstance *aInstance, otNetifAddress *aCreatedAddress);
};

/**
 * @}
 */

} // namespace Utils
} // namespace ot

#endif // SLAAC_ADDRESS_HPP_
