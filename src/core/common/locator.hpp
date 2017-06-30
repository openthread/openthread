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
 *   This file includes definitions for locator class for OpenThread objects.
 */

#ifndef LOCATOR_HPP_
#define LOCATOR_HPP_

#include <openthread/config.h>

#include <openthread/types.h>

#include "openthread-core-config.h"
#include "openthread-single-instance.h"

namespace ot {

class ThreadNetif;
class MeshForwarder;
class TaskletScheduler;
class TimerScheduler;
namespace Ip6 { class Ip6; }

/**
 * @addtogroup core-locator
 *
 * @brief
 *   This module includes definitions for locator base class for OpenThread objects.
 *
 * @{
 *
 */

/**
 * This template class implements the base locator for OpenThread objects.
 *
 */
template <typename Type>
class Locator
{
protected:
    /**
     * This constructor initializes the locator.
     *
     * @param[in]  aObject  A reference to the object.
     *
     */
    Locator(Type &aObject)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
        : mLocatorObject(aObject)
#endif
    {
        OT_UNUSED_VARIABLE(aObject);
    }

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Type &mLocatorObject;
#endif
};

/**
 * This class implements a locator for ThreadNetif object.
 *
 */
class ThreadNetifLocator: private Locator<ThreadNetif>
{
public:
    /**
     * This method returns a reference to the thread network interface.
     *
     * @returns   A reference to the thread network interface.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    ThreadNetif &GetNetif(void) const { return mLocatorObject; }
#else
    ThreadNetif &GetNetif(void) const { return otGetThreadNetif(); }
#endif

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const;
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the thread network interface.
     *
     */
    ThreadNetifLocator(ThreadNetif &aThreadNetif): Locator(aThreadNetif) { }
};

/**
 * This class implements a locator for MeshForwarder object.
 *
 */
class MeshForwarderLocator: private Locator<MeshForwarder>
{
public:
    /**
     * This method returns a reference to the MeshForwarder.
     *
     * @returns   A reference to the MeshForwarder.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    MeshForwarder &GetMeshForwarder(void) const { return mLocatorObject; }
#else
    MeshForwarder &GetMeshForwarder(void) const { return otGetMeshForwarder(); }
#endif

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const;
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aMeshForwarder  A reference to the MeshForwarder.
     *
     */
    MeshForwarderLocator(MeshForwarder &aMeshForwarder): Locator(aMeshForwarder) { }
};

/**
 * This class implements a locator for TimerScheduler object.
 *
 */
class TimerSchedulerLocator: private Locator<TimerScheduler>
{
public:
    /**
     * This method returns a reference to the TimerScheduler.
     *
     * @returns   A reference to the TimerScheduler.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    TimerScheduler &GetTimerScheduler(void) const { return mLocatorObject; }
#else
    TimerScheduler &GetTimerScheduler(void) const { return otGetTimerScheduler(); }
#endif

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const;
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aTimerScheduler  A reference to the TimerScheduler.
     *
     */
    TimerSchedulerLocator(TimerScheduler &aTimerScheduler): Locator(aTimerScheduler) { }
};

/**
 * This class implements a locator for TaskletScheduler object.
 *
 */
class TaskletSchedulerLocator: private Locator<TaskletScheduler>
{
public:
    /**
     * This method returns a reference to the TaskletScheduler.
     *
     * @returns   A reference to the TaskletScheduler.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    TaskletScheduler &GetTaskletScheduler(void) const { return mLocatorObject; }
#else
    TaskletScheduler &GetTaskletScheduler(void) const { return otGetTaskletScheduler(); }
#endif

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const;
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aTaskletScheduler  A reference to the TaskletScheduler.
     *
     */
    TaskletSchedulerLocator(TaskletScheduler &aTaskletScheduler): Locator(aTaskletScheduler) { }
};

/**
 * This class implements a locator for Ip6 object.
 *
 */
class Ip6Locator: private Locator<Ip6::Ip6>
{
public:
    /**
     * This method returns a reference to the Ip6.
     *
     * @returns   A reference to the Ip6.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Ip6::Ip6 &GetIp6(void) const { return mLocatorObject; }
#else
    Ip6::Ip6 &GetIp6(void) const { return otGetIp6(); }
#endif

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const;
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aIp6  A reference to the Ip6.
     *
     */
    Ip6Locator(Ip6::Ip6 &aIp6): Locator(aIp6) { }
};

/**
 * This class implements locator for  otInstance object
 *
 */
class InstanceLocator
{
public:
    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure, or NULL if the instance has been finalized.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *GetInstance(void) const { return mInstance; }
#else
    otInstance *GetInstance(void) const { return otGetInstance(); }
#endif

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A pointer to the otInstance.
     *
     */
    InstanceLocator(otInstance *aInstance)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
        : mInstance(aInstance)
#endif
    {
        OT_UNUSED_VARIABLE(aInstance);
    }

private:
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance *mInstance;
#endif
};

/**
 * @}
 *
 */

}  // namespace ot

#endif  // LOCATOR_HPP_
