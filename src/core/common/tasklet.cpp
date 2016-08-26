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
 *   This file implements the tasklet scheduler.
 */

#include <openthread.h>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/tasklet.hpp>
#include <openthreadinstance.h>

namespace Thread {

Tasklet::Tasklet(otInstance *aInstance, Handler aHandler, void *aContext)
{
    mInstance = aInstance;
    mHandler = aHandler;
    mContext = aContext;
    mNext = NULL;
}

ThreadError Tasklet::Post(void)
{
    return TaskletScheduler::Post(*this);
}

ThreadError TaskletScheduler::Post(Tasklet &aTasklet)
{
    ThreadError error = kThreadError_None;
    otInstance *aInstance = aTasklet.mInstance;

    VerifyOrExit(aInstance->mTaskletTail != &aTasklet && aTasklet.mNext == NULL, error = kThreadError_Busy);

    if (aInstance->mTaskletTail == NULL)
    {
        aInstance->mTaskletHead = &aTasklet;
        aInstance->mTaskletTail = &aTasklet;
        otSignalTaskletPending(aInstance);
    }
    else
    {
        aInstance->mTaskletTail->mNext = &aTasklet;
        aInstance->mTaskletTail = &aTasklet;
    }

exit:
    return error;
}

Tasklet *TaskletScheduler::PopTasklet(otInstance *aInstance)
{
    Tasklet *task = aInstance->mTaskletHead;

    if (task != NULL)
    {
        aInstance->mTaskletHead = aInstance->mTaskletHead->mNext;

        if (aInstance->mTaskletHead == NULL)
        {
            aInstance->mTaskletTail = NULL;
        }

        task->mNext = NULL;
    }

    return task;
}

bool TaskletScheduler::AreTaskletsPending(otInstance *aInstance)
{
    return aInstance->mTaskletHead != NULL;
}

void TaskletScheduler::RunNextTasklet(otInstance *aInstance)
{
    Tasklet  *task;

    task = PopTasklet(aInstance);

    if (task != NULL)
    {
        task->RunTask();
    }
}

}  // namespace Thread
