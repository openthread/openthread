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
 *   This file includes definitions for tasklets and the tasklet scheduler.
 */

#ifndef TASKLET_HPP_
#define TASKLET_HPP_

#include "openthread-core-config.h"

#include <stdio.h>

#include <openthread/tasklet.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#if OPENTHREAD_CONFIG_GENERIC_TASKLET_ENABLE
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#endif

namespace ot {

/**
 * @addtogroup core-tasklet
 *
 * @brief
 *   This module includes definitions for tasklets and the tasklet scheduler.
 *
 * @{
 *
 */

/**
 * Is used to represent a tasklet.
 *
 */
class Tasklet : public InstanceLocator
{
public:
    /**
     * Implements the tasklet scheduler.
     *
     */
    class Scheduler : private NonCopyable
    {
        friend class Tasklet;

    public:
        /**
         * Initializes the object.
         *
         */
        Scheduler(void)
            : mTail(nullptr)
        {
        }

        /**
         * Indicates whether or not there are tasklets pending.
         *
         * @retval TRUE   If there are tasklets pending.
         * @retval FALSE  If there are no tasklets pending.
         *
         */
        bool AreTaskletsPending(void) const { return mTail != nullptr; }

        /**
         * Processes all tasklets queued when this is called.
         *
         */
        void ProcessQueuedTasklets(void);

    private:
        void PostTasklet(Tasklet &aTasklet);

        Tasklet *mTail; // A circular singly linked-list
    };

    /**
     * Reference is called when the tasklet is run.
     *
     * @param[in]  aTasklet  A reference to the tasklet being run.
     *
     */
    typedef void (&Handler)(Tasklet &aTasklet);

    /**
     * Creates a tasklet instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance object.
     * @param[in]  aHandler    A pointer to a function that is called when the tasklet is run.
     *
     */
    Tasklet(Instance &aInstance, Handler aHandler)
        : InstanceLocator(aInstance)
        , mHandler(aHandler)
        , mNext(nullptr)
    {
    }

    /**
     * Puts the tasklet on the tasklet scheduler run queue.
     *
     * If the tasklet is already posted, no change is made and run queue stays as before.
     *
     */
    void Post(void);

    /**
     * Indicates whether the tasklet is posted or not.
     *
     * @retval TRUE  The tasklet is posted.
     * @retval FALSE The tasklet is not posted.
     *
     */
    bool IsPosted(void) const { return (mNext != nullptr); }

private:
    void RunTask(void) { mHandler(*this); }

    Handler  mHandler;
    Tasklet *mNext;
};

/**
 * Defines a tasklet owned by specific type and using a method on owner type as the callback.
 *
 * @tparam Owner              The type of owner of this tasklet.
 * @tparam HandleTaskletPtr   A pointer to a non-static member method of `Owner` to use as tasklet handler.
 *
 * The `Owner` MUST be a type that is accessible using `InstanceLocator::Get<Owner>()`.
 *
 */
template <typename Owner, void (Owner::*HandleTaskletPtr)(void)> class TaskletIn : public Tasklet
{
public:
    /**
     * Initializes the tasklet.
     *
     * @param[in]  aInstance   The OpenThread instance.
     *
     */
    explicit TaskletIn(Instance &aInstance)
        : Tasklet(aInstance, HandleTasklet)
    {
    }

private:
    static void HandleTasklet(Tasklet &aTasklet); // Implemented in `locator_getters.hpp`
};

/**
 * Defines a tasklet that also maintains a user context pointer.
 *
 * In typical `Tasklet` use, in the handler callback, the owner of the tasklet is determined using `GetOwner<Type>`
 * method. This method works if there is a single instance of `Type` within OpenThread instance hierarchy. The
 * `TaskletContext` is intended for cases where there may be multiple instances of the same class/type using a `Tasklet`
 * object. `TaskletContext` will store a context `void *` information.
 *
 */
class TaskletContext : public Tasklet
{
public:
    /**
     * Creates a tasklet instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A pointer to a function that is called when the tasklet is run.
     * @param[in]  aContext    A pointer to an arbitrary context information.
     *
     */
    TaskletContext(Instance &aInstance, Handler aHandler, void *aContext)
        : Tasklet(aInstance, aHandler)
        , mContext(aContext)
    {
    }

    /**
     * Returns the pointer to the arbitrary context information.
     *
     * @returns Pointer to the arbitrary context information.
     *
     */
    void *GetContext(void) { return mContext; }

private:
    void *mContext;
};

#if OPENTHREAD_CONFIG_GENERIC_TASKLET_ENABLE
/**
 * Defines a generic tasklet that can be used to execute a callback function from the context of the OpenThread task
 *
 * The GenericTasklet is used by the OpenThread instance to execute an API-provided function from the context
 * of the OpenThread task. It maintains an internal list of entries that contain the callback function and
 * a context parameter for the callback function. It introduces a new method, `PostWithCb` that takes the
 * callback function and the context as parameters and pushes them to list and then calls `Post` internally.
 * Then, it uses `HandleGenericTasklet` to call the callback function when the tasklet executes. Since the
 * Tasklet can be posted only once until it is processed. The internal queue handles this by storing all received
 * messages and executing all of them from the queue when the tasklet posts.
 *
 */
class GenericTasklet : public Tasklet
{
public:
    /**
     * Creates a generic tasklet instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     *
     */
    GenericTasklet(Instance &aInstance)
        : Tasklet(aInstance, GenericTasklet::HandleGenericTasklet)
    {
    }

    typedef void (*TaskletCallback)(void *aContext);

    struct InternalContext : public LinkedListEntry<InternalContext>, public Heap::Allocatable<InternalContext>
    {
        friend class LinkedListEntry<InternalContext>;
        friend class Heap::Allocatable<InternalContext>;

    public:
        TaskletCallback mCallback;
        void           *mContext;

    private:
        Error            Init(TaskletCallback aCallback, void *aContext);
        InternalContext *mNext;
    };
    /**
     * Puts the tasklet on the tasklet scheduler run queue and the callback/context on the internal
     * list.
     * If the tasklet is already posted, no change is made.
     *
     * @param[in] aCallback The callback function that executes from the context of the OpenThread task.
     * @param[in] aContext  A pointer to a context that will be used by the callback function.
     *
     * @retval kErrorNone             Successfully allocated InternalContext.
     * @retval kErrorNoBufs           Insufficient space to store the InternalContext.
     *
     */
    Error PostWithCb(TaskletCallback aCallback, void *aContext);
    /**
     *
     * Static function used as the handler for the tasklet.
     *
     * @param[in] aTasklet The Tasklet object.
     *
     */
    static void HandleGenericTasklet(Tasklet &aTasklet);

    LinkedList<InternalContext> mEventList;
};
#endif /*OPENTHREAD_CONFIG_GENERIC_TASKLET_ENABLE*/

/**
 * @}
 *
 */

} // namespace ot

#endif // TASKLET_HPP_
