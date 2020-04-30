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
 *   This file implements managing DUA.
 */

#include "dua_manager.hpp"

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_DUA_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/ip6_address.hpp"
#include "thread/thread_netif.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

DuaManager::DuaManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mDadCounter(0)
{
    mDomainUnicastAddress.Clear();
    mDomainUnicastAddress.mPreferred          = true;
    mDomainUnicastAddress.mScopeOverride      = Ip6::Address::kGlobalScope;
    mDomainUnicastAddress.mScopeOverrideValid = true;
}

void DuaManager::UpdateDomainUnicastAddress(BackboneRouter::Leader::DomainPrefixState aState)
{
    const otIp6Prefix *prefix;

    if ((aState == BackboneRouter::Leader::kDomainPrefixRemoved) ||
        (aState == BackboneRouter::Leader::kDomainPrefixRefreshed))
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mDomainUnicastAddress);
    }

    VerifyOrExit((aState == BackboneRouter::Leader::kDomainPrefixAdded) ||
                     (aState == BackboneRouter::Leader::kDomainPrefixRefreshed),
                 OT_NOOP);

    prefix = Get<BackboneRouter::Leader>().GetDomainPrefix();

    OT_ASSERT(prefix != NULL);

    mDomainUnicastAddress.GetAddress().Clear();
    mDomainUnicastAddress.GetAddress().SetPrefix(prefix->mPrefix.mFields.m8, prefix->mLength);
    mDomainUnicastAddress.mPrefixLength = prefix->mLength;

    if (Get<Utils::Slaac>().GenerateIid(mDomainUnicastAddress, NULL, 0, &mDadCounter) == OT_ERROR_NONE)
    {
        mDomainUnicastAddress.mValid = true;
        Get<ThreadNetif>().AddUnicastAddress(mDomainUnicastAddress);
    }
    else
    {
        otLogWarnCore("Failed to generate valid DUA");
    }

exit:
    return;
}

} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_DUA_ENABLE
