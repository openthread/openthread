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
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "net/ip6.hpp"

namespace ot {

Tasklet::Tasklet(Instance &aInstance, Handler aHandler, void *aOwner)
    : InstanceLocator(aInstance)
    , OwnerLocator(aOwner)
    , mHandler(aHandler)
    , mNext(nullptr)
{
}

void Tasklet::Post(void)
{
    if (!IsPosted())
    {
        Get<TaskletScheduler>().PostTasklet(*this);
    }
}

TaskletScheduler::TaskletScheduler(void)
    : mTail(nullptr)
{
}

void TaskletScheduler::PostTasklet(Tasklet &aTasklet)
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

void TaskletScheduler::ProcessQueuedTasklets(void)
{
    Tasklet *tail = mTail;

    // This method processes all tasklets queued when this is called. We
    // keep a copy the current list and then clear the main list by
    // setting `mTail` to nullptr. A newly posted tasklet while processing
    // the currently queued tasklets will then trigger a call to
    // `otTaskletsSignalPending()`.

    mTail = nullptr;

    while (tail != nullptr)
    {
        Tasklet *tasklet = tail->mNext;

        if (tasklet == tail)
        {
            tail = nullptr;
        }
        else
        {
            tail->mNext = tasklet->mNext;
        }

        tasklet->mNext = nullptr;
        tasklet->RunTask();
    }
}

} // namespace ot
