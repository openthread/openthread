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

/**
 * @file
 *   This file includes definitions for Network Identity tracker.
 */

#ifndef OT_CORE_MESHCOP_NETWORK_IDENTITY_HPP_
#define OT_CORE_MESHCOP_NETWORK_IDENTITY_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "meshcop/extended_panid.hpp"
#include "meshcop/network_name.hpp"

namespace ot {
namespace MeshCoP {

/**
 * Tracks the Network Identify related parameters such as Extended PAN ID and Network Name.
 */
class NetworkIdentity : public InstanceLocator, private NonCopyable
{
public:
    static const char kDefaultNetworkName[]; ///< Default Network Name ("OpenThread")
    static const char kDefaultDomainName[];  ///< Default Domain Name ("DefaultDomain")

    /**
     * Initializes the `NetworkIdentity`.
     *
     * @param[in]  aInstance  The OpenThread instance.
     */
    explicit NetworkIdentity(Instance &aInstance);

    /**
     * Returns the Extended PAN Identifier.
     *
     * @returns The Extended PAN Identifier.
     */
    const ExtendedPanId &GetExtPanId(void) const { return mExtendedPanId; }

    /**
     * Sets the Extended PAN Identifier.
     *
     * @param[in]  aExtendedPanId  The Extended PAN Identifier.
     */
    void SetExtPanId(const ExtendedPanId &aExtendedPanId);

    /**
     * Returns the Network Name.
     *
     * @returns The Network Name.
     */
    const NetworkName &GetNetworkName(void) const { return mNetworkName; }

    /**
     * Sets the Network Name.
     *
     * @param[in]  aNameString   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     */
    Error SetNetworkName(const char *aNameString);

    /**
     * Sets the Network Name.
     *
     * @param[in]  aNameData     A name data (pointer to char buffer and length).
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     */
    Error SetNetworkName(const NameData &aNameData);

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    /**
     * Returns the Thread Domain Name.
     *
     * @returns The Thread Domain Name.
     */
    const DomainName &GetDomainName(void) const { return mDomainName; }

    /**
     * Sets the Thread Domain Name.
     *
     * @param[in]  aNameString   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone          Successfully set the Thread Domain Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     */
    Error SetDomainName(const char *aNameString);

    /**
     * Sets the Thread Domain Name.
     *
     * @param[in]  aNameData     A name data (pointer to char buffer and length).
     *
     * @retval kErrorNone          Successfully set the Thread Domain Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     */
    Error SetDomainName(const NameData &aNameData);

    /**
     * Checks whether the Thread Domain Name is currently set to the default name.
     *
     * @returns true if Thread Domain Name equals "DefaultDomain", false otherwise.
     */
    bool IsDefaultDomainNameSet(void) const;

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

private:
    static const otExtendedPanId sExtendedPanidInit;

    Error SignalNetworkNameChange(Error aError);

    ExtendedPanId mExtendedPanId;
    NetworkName   mNetworkName;
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    DomainName mDomainName;
#endif
};

} // namespace MeshCoP
} // namespace ot

#endif // OT_CORE_MESHCOP_NETWORK_IDENTITY_HPP_
