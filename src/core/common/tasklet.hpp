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

#include "openthread/tasklet.h"

namespace Thread {

namespace Ip6 { class Ip6; }

class TaskletScheduler;

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
 * This class is used to represent a tasklet.
 *
 */
class Tasklet
{
    friend class TaskletScheduler;

public:
    /**
     * This function pointer is called when the tasklet is run.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    typedef void (*Handler)(void *aContext);

    /**
     * This constructor creates a tasklet instance.
     *
     * @param[in]  aScheduler  A reference to the tasklet scheduler.
     * @param[in]  aHandler    A pointer to a function that is called when the tasklet is run.
     * @param[in]  aContext    A pointer to arbitrary context information.
     *
     */
    Tasklet(TaskletScheduler &aScheduler, Handler aHandler, void *aContext);

    /**
     * This method puts the tasklet on the run queue.
     *
     */
    ThreadError Post(void);

private:
    void RunTask(void) { mHandler(mContext); }

    TaskletScheduler &mScheduler;
    Handler           mHandler;
    void             *mContext;
    Tasklet          *mNext;
};

/**
 * This class implements the tasklet scheduler.
 *
 */
class TaskletScheduler
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    TaskletScheduler(void);

    /**
     * This method enqueues a tasklet into the run queue.
     *
     * @param[in]  aTasklet  A reference to the tasklet to enqueue.
     *
     * @retval kThreadError_None  Successfully enqueued the tasklet.
     * @retval kThreadError_Already  The tasklet was already enqueued.
     */
    ThreadError Post(Tasklet &aTasklet);

    /**
     * This method indicates whether or not there are tasklets pending.
     *
     * @retval TRUE   If there are tasklets pending.
     * @retval FALSE  If there are no tasklets pending.
     *
     */
    bool AreTaskletsPending(void);

    /**
     * This method processes all tasklets queued when this is called.
     *
     */
    void ProcessQueuedTasklets(void);

    /**
     * This method returns the pointer to the parent Ip6 structure.
     *
     * @returns The pointer to the parent Ip6 structure.
     *
     */
    Ip6::Ip6 *GetIp6();

private:
    Tasklet *PopTasklet(void);
    Tasklet *mHead;
    Tasklet *mTail;
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // TASKLET_HPP_
