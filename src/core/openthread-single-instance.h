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
 * @brief
 *  This file defines the getter functions for single instance OpenThread objects.
 */

#ifndef SINGLE_OPENTHREAD_INSTANCE_H_
#define SINGLE_OPENTHREAD_INSTANCE_H_

#include <openthread/config.h>

#include <openthread/types.h>

#include "openthread-core-config.h"


#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

namespace ot {

class ThreadNetif;
class MeshForwarder;
class TaskletScheduler;
namespace Ip6 { class Ip6; }

} // namespace ot

/**
 * This function returns a pointer to the single otInstance (if initialized and ready).
 *
 * @returns A pointer to single otInstance structure, or NULL if the instance is not ready, i.e. either it is not yet
 *          initialized (no call to `otInstanceInitSingle()`) or it is finalized (call to `otInstanceFinalize()`).
 *
 */
otInstance *otGetInstance(void);

/**
 * This function returns a reference to the single thread network interface instance.
 *
 * @returns A reference to the thread network interface instance.
 *
 */
ot::ThreadNetif &otGetThreadNetif(void);

/**
 * This function returns a reference to the single MeshForwarder instance.
 *
 * @returns A reference to the MeshForwarder instance.
 *
 */
ot::MeshForwarder &otGetMeshForwarder(void);

/**
 * This function returns a reference to the single TaskletShceduler instance.
 *
 * @returns A reference to the TaskletShceduler instance.
 *
 */
ot::TaskletScheduler &otGetTaskletScheduler(void);

/**
 * This function returns a reference to the single Ip6 instance.
 *
 * @returns A reference to the Ip6 instance.
 *
 */
ot::Ip6::Ip6 &otGetIp6(void);

#endif // #if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

#endif  // SINGLE_OPENTHREAD_INSTANCE_H_
