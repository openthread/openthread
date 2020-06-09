/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for managing Domain Unicast Address feature defined in Thread 1.2.
 */

#ifndef DUA_MANAGER_HPP_
#define DUA_MANAGER_HPP_

#include "openthread-core-config.h"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#include "backbone_router/leader.hpp"
#include "common/locator.hpp"
#include "net/netif.hpp"

namespace ot {

/**
 * @addtogroup core-dua
 *
 * @brief
 *   This module includes definitions for generating, managing, registering Domain Unicast Address.
 *
 * @{
 *
 * @defgroup core-dua Dua
 *
 * @}
 *
 */

/**
 * This class implements managing DUA.
 *
 */
class DuaManager : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit DuaManager(Instance &aInstance);

    /**
     * This method updates Domain Unicast Address.
     *
     * @param[in]  aState  The Domain Prefix state or state change.
     *
     */
    void UpdateDomainUnicastAddress(BackboneRouter::Leader::DomainPrefixState aState);

    /**
     * This method returns a reference to the Domain Unicast Address.
     *
     * @returns A reference to the Domain Unicast Address.
     *
     */
    const Ip6::Address &GetDomainUnicastAddress(void) const { return mDomainUnicastAddress.GetAddress(); }

    /**
     * This method sets the Interface Identifier manually specified for the Thread Domain Unicast Address.
     *
     * @param[in]  aIid        A reference to the Interface Identifier to set.
     *
     * @retval OT_ERROR_NONE           Successfully set the Interface Identifier.
     * @retval OT_ERROR_INVALID_ARGS   The specified Interface Identifier is reserved.
     *
     */
    otError SetFixedDuaInterfaceIdentifier(const Ip6::InterfaceIdentifier &aIid);

    /**
     * This method clears the Interface Identifier manually specified for the Thread Domain Unicast Address.
     *
     */
    void ClearFixedDuaInterfaceIdentifier(void);

    /**
     * This method indicates whether or not there is Interface Identifier manually specified for the Thread
     * Domain Unicast Address.
     *
     * @retval true  If there is Interface Identifier manually specified.
     * @retval false If there is no Interface Identifier manually specified.
     *
     */
    bool IsFixedDuaInterfaceIdentifierSet(void) { return !mFixedDuaInterfaceIdentifier.IsUnspecified(); }

    /**
     * This method gets the Interface Identifier for the Thread Domain Unicast Address if manually specified.
     *
     * @returns A reference to the Interface Identifier.
     *
     */
    const Ip6::InterfaceIdentifier &GetFixedDuaInterfaceIdentifier(void) const { return mFixedDuaInterfaceIdentifier; }

    /*
     * This method restores duplicate address detection information from non-volatile memory.
     *
     */
    void Restore(void);

private:
    otError GenerateDomainUnicastAddressIid(void);
    otError Store(void);

    Ip6::InterfaceIdentifier mFixedDuaInterfaceIdentifier;
    Ip6::NetifUnicastAddress mDomainUnicastAddress;
    uint8_t                  mDadCounter;
};

} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif // DUA_MANAGER_HPP_
