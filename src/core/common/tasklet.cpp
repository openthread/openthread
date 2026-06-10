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
 *   This file implements the tasklet scheduler.
 */

#include "tasklet.hpp"

#include "common/code_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

void Tasklet::Post(void)
{
    if (!IsPosted())
    {
        Get<Scheduler>().mPostedQueue.PostTasklet(*this);
    }
}

void Tasklet::Unpost(void)
{
    Get<Scheduler>().mPostedQueue.RemoveTasklet(*this);
    Get<Scheduler>().mRunningQueue.RemoveTasklet(*this);
}

void Tasklet::Scheduler::Queue::PostTasklet(Tasklet &aTasklet)
{
    // Tasklets are saved in a circular singly linked list.

    if (mTail == nullptr)
    {
        mTail        = &aTasklet;
        mTail->mNext = mTail;
        otTaskletsSignalPending(&aTasklet.GetInstance());
    }
    else
    {
        aTasklet.mNext = mTail->mNext;
        mTail->mNext   = &aTasklet;
        mTail          = &aTasklet;
    }
}

void Tasklet::Scheduler::Queue::RemoveTasklet(Tasklet &aTasklet)
{
    Tasklet *prev;

    VerifyOrExit(aTasklet.IsPosted());

    VerifyOrExit(!IsEmpty());

    prev = mTail;

    while (prev->mNext != &aTasklet)
    {
        prev = prev->mNext;

        if (prev == mTail)
        {
            ExitNow();
        }
    }

    prev->mNext    = aTasklet.mNext;
    aTasklet.mNext = nullptr;

    if (mTail == &aTasklet)
    {
        mTail = (prev != &aTasklet) ? prev : nullptr;
    }

exit:
    return;
}

Tasklet *Tasklet::Scheduler::Queue::PopTasklet(void)
{
    Tasklet *tasklet;

    if (IsEmpty())
    {
        tasklet = nullptr;
    }
    else
    {
        tasklet = mTail->mNext;
        RemoveTasklet(*tasklet);
    }

    return tasklet;
}

void Tasklet::Scheduler::ProcessQueuedTasklets(void)
{
    Tasklet *tasklet;

    // We transfer all currently posted tasklets to the `mRunningQueue` and
    // clear the `mPostedQueue`. This ensures that any new tasklet posted
    // while we are processing `mRunningQueue` will be added to `mPostedQueue`
    // and will trigger a call to `otTaskletsSignalPending()`.

    mRunningQueue = mPostedQueue;
    mPostedQueue.Clear();

    while ((tasklet = mRunningQueue.PopTasklet()) != nullptr)
    {
        tasklet->RunTask();
    }
}

} // namespace ot
