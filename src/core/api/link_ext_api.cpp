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
 *   This file implements the OpenThread Link Extension API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#include <openthread/link_ext.h>

#include "instance/instance.hpp"

using namespace ot;

otError otLinkExtTransmitBurst(otInstance *aInstance, const otLinkExtBurstConfig *aConfig, otLinkExtBurstDone aCallback)
{
    return AsCoreType(aInstance).Get<Mac::LinkExt>().TransmitBurst(*aConfig, aCallback);
}

otError otLinkExtStopBurst(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mac::LinkExt>().StopBurst(); }

void otLinkExtGetBurstConfig(otInstance *aInstance, otLinkExtBurstConfig *aConfig)
{
    AsCoreType(aInstance).Get<Mac::LinkExt>().GetBurstConfig(*aConfig);
}

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
otError otLinkExtSetPeriodicRxSchedules(otInstance                        *aInstance,
                                        const otLinkExtPeriodicRxSchedule *aSchedules,
                                        uint8_t                            aNumSchedules)
{
    return AsCoreType(aInstance).Get<Mac::LinkExt>().SetPeriodicRxSchedules(aSchedules, aNumSchedules);
}

const otLinkExtPeriodicRxSchedule *otLinkExtGetPeriodicRxSchedules(otInstance *aInstance, uint8_t *aNumSchedules)
{
    return AsCoreType(aInstance).Get<Mac::LinkExt>().GetPeriodicRxSchedules(*aNumSchedules);
}
#endif

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE
