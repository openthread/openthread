/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread child supervision API.
 */

#include "openthread/child_supervision.h"

#include "openthread-instance.h"

using namespace ot;

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION

uint16_t otChildSupervisionGetInterval(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetChildSupervisor().GetSupervisionInterval();
}

void otChildSupervisionSetInterval(otInstance *aInstance, uint16_t aInterval)
{
    aInstance->mThreadNetif.GetChildSupervisor().SetSupervisionInterval(aInterval);
}

uint16_t otChildSupervisionGetCheckTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetSupervisionListener().GetTimeout();
}

void otChildSupervisionSetCheckTimeout(otInstance *aInstance, uint16_t aTimeout)
{
    aInstance->mThreadNetif.GetSupervisionListener().SetTimeout(aTimeout);
}

#endif  // OPENTHREAD_ENABLE_CHILD_SUPERVISION
