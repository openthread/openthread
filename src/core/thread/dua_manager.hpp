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

private:
    Ip6::NetifUnicastAddress mDomainUnicastAddress;
    uint8_t                  mDadCounter;
};

} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif // DUA_MANAGER_HPP_
