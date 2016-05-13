/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <openthread-types.h>

namespace Thread {

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
     * @param[in]  aHandler  A pointer to a function that is called when the tasklet is run.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    Tasklet(Handler aHandler, void *aContext);

    /**
     * This method puts the tasklet on the run queue.
     *
     */
    ThreadError Post(void);

private:
    /**
     * This method is called when the tasklet is run.
     *
     */
    void RunTask(void) { mHandler(mContext); }

    Handler  mHandler;  ///< A pointer to a function that is called when the tasklet is run.
    void    *mContext;  ///< A pointer to arbitrary context information.
    Tasklet *mNext;     ///< A pointer to the next tasklet in the run queue.
};

/**
 * This class implements the tasklet scheduler.
 *
 */
class TaskletScheduler
{
public:
    /**
     * This static method enqueues a tasklet into the run queue.
     *
     * @param[in]  aTasklet  A reference to the tasklet to enqueue.
     *
     * @retval kThreadError_None  Successfully enqueued the tasklet.
     * @retval kThreadError_Busy  The tasklet was already enqueued.
     */
    static ThreadError Post(Tasklet &aTasklet);

    /**
     * This static method indicates whether or not there are tasklets pending.
     *
     * @retval TRUE   If there are tasklets pending.
     * @retval FALSE  If there are no tasklets pending.
     *
     */
    static bool AreTaskletsPending(void);

    /**
     * This static method runs the next tasklet.
     *
     */
    static void RunNextTasklet(void);

private:
    static Tasklet *PopTasklet(void);
    static Tasklet *sHead;
    static Tasklet *sTail;
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // TASKLET_HPP_
