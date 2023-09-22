/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the OpenThread SRP Replication (SRPL) APIs.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#include <openthread/srp_replication.h>

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

otError otSrpReplicationSetEnabled(otInstance *aInstance, bool aEnable)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().SetEnabled(aEnable);
}

otSrpReplicationState otSrpReplicationGetState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<Srp::Srpl>().GetState());
}

otError otSrpReplicationSetDomain(otInstance *aInstance, const char *aName)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().SetDomain(aName);
}

const char *otSrpReplicationGetDomain(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().GetDomain();
}

otError otSrpReplicationSetDefaultDomain(otInstance *aInstance, const char *aName)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().SetDefaultDomain(aName);
}

const char *otSrpReplicationGetDefaultDomain(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().GetDefaultDomain();
}

uint64_t otSrpReplicationGetId(otInstance *aInstance) { return AsCoreType(aInstance).Get<Srp::Srpl>().GetId(); }

otError otSrpReplicationGetDatasetId(otInstance *aInstance, uint64_t *aDatasetId)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().GetDatasetId(*aDatasetId);
}

void otSrpReplicationInitPartnerIterator(otSrpReplicationPartnerIterator *aIterator) { AsCoreType(aIterator).Init(); }

otError otSrpReplicationGetNextPartner(otInstance                      *aInstance,
                                       otSrpReplicationPartnerIterator *aIterator,
                                       otSrpReplicationPartner         *aPartner)
{
    return AsCoreType(aInstance).Get<Srp::Srpl>().GetNextPartner(AsCoreType(aIterator), AsCoreType(aPartner));
}

#if OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
const otSrpReplicationTestConfig *otSrpReplicationGetTestConfig(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Srp::Srpl>().GetTestConfig();
}

void otSrpReplicationSetTestConfig(otInstance *aInstance, const otSrpReplicationTestConfig *aTestConfig)
{
    AsCoreType(aInstance).Get<Srp::Srpl>().SetTestConfig(AsCoreType(aTestConfig));
}
#endif

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
