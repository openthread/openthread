/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "test_platform.h"
#include "test_util.h"

#include <openthread/config.h>

#include "instance/instance.hpp"

namespace ot {

#define Log(aMessage) fprintf(stderr, aMessage "\n\r")

static Instance *sInstance                = nullptr;
static bool      sTask1Handled            = false;
static bool      sTask2Handled            = false;
static bool      sTask3Handled            = false;
static bool      sSignalPendingCalled     = false;
static bool      sShouldTask3RepostItself = false;

extern "C" void otTaskletsSignalPending(otInstance *aInstance)
{
    Log("   otTaskletsSignalPending()");

    if (sInstance != nullptr)
    {
        VerifyOrQuit(aInstance == sInstance);
    }

    sSignalPendingCalled = true;
}

void ResetTestFlags(void)
{
    sTask1Handled        = false;
    sTask2Handled        = false;
    sTask3Handled        = false;
    sSignalPendingCalled = false;
}

void CheckTaskeltFromHandler(Tasklet &aTasklet)
{
    VerifyOrQuit(&aTasklet.GetInstance() == sInstance);
    VerifyOrQuit(!aTasklet.IsPosted());
}

void HandleTask1(Tasklet &aTasklet)
{
    Log("   HandleTask1()");
    CheckTaskeltFromHandler(aTasklet);
    VerifyOrQuit(!sTask1Handled);
    sTask1Handled = true;
}

void HandleTask2(Tasklet &aTasklet)
{
    Log("   HandleTask2()");
    CheckTaskeltFromHandler(aTasklet);
    VerifyOrQuit(!sTask2Handled);
    sTask2Handled = true;
}

void HandleTask3(Tasklet &aTasklet)
{
    Log("   HandleTask3()");
    CheckTaskeltFromHandler(aTasklet);
    VerifyOrQuit(!sTask3Handled);
    sTask3Handled = true;

    if (sShouldTask3RepostItself)
    {
        aTasklet.Post();
    }
}

void TestTasklet(void)
{
    Log("TestTasklet");

    sInstance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(sInstance != nullptr);

    sInstance->Get<Tasklet::Scheduler>().ProcessQueuedTasklets();

    {
        Tasklet::Scheduler &scheduler = sInstance->Get<Tasklet::Scheduler>();
        Tasklet             task1(*sInstance, HandleTask1);
        Tasklet             task2(*sInstance, HandleTask2);
        Tasklet             task3(*sInstance, HandleTask3);

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Posting a single task");

        ResetTestFlags();

        task1.Post();
        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        VerifyOrQuit(scheduler.AreTaskletsPending());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Posting multiple tasks");

        ResetTestFlags();

        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        VerifyOrQuit(scheduler.AreTaskletsPending());

        task2.Post();
        task1.Post();

        VerifyOrQuit(!sSignalPendingCalled);

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(!scheduler.AreTaskletsPending());
        VerifyOrQuit(!sSignalPendingCalled);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Posting the same task multiple times");

        ResetTestFlags();

        task2.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        VerifyOrQuit(scheduler.AreTaskletsPending());

        task2.Post();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        task1.Post();
        task2.Post();
        task1.Post();

        VerifyOrQuit(!sSignalPendingCalled);

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!scheduler.AreTaskletsPending());
        VerifyOrQuit(!sSignalPendingCalled);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Task posting itself from its handler");

        ResetTestFlags();
        sShouldTask3RepostItself = true;

        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        task2.Post();
        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(scheduler.AreTaskletsPending());
        VerifyOrQuit(sSignalPendingCalled);

        ResetTestFlags();

        sShouldTask3RepostItself = false;

        scheduler.ProcessQueuedTasklets();
        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Un-posting a single posted task");

        ResetTestFlags();

        task1.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());

        task1.Unpost();

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        VerifyOrQuit(!task1.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post the first one");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task1.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post the middle one");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task2.Unpost();

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post the last one");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task3.Unpost();

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post the first and last ones");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task1.Unpost();
        task3.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post all in the same order added");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task1.Unpost();
        task2.Unpost();
        task3.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        VerifyOrQuit(!scheduler.AreTaskletsPending());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post all in the reverse order added");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task3.Unpost();
        task2.Unpost();
        task1.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        VerifyOrQuit(!scheduler.AreTaskletsPending());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post multiple tasks and then un-post all in different order (middle first)");

        ResetTestFlags();

        task1.Post();
        task2.Post();
        task3.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        task2.Unpost();
        task3.Unpost();
        task1.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(!task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        VerifyOrQuit(!scheduler.AreTaskletsPending());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(!sTask2Handled);
        VerifyOrQuit(!sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Un-posting tasks not yet posted");

        ResetTestFlags();

        task1.Unpost();

        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        task2.Post();

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;
        VerifyOrQuit(scheduler.AreTaskletsPending());

        task1.Unpost();
        task3.Unpost();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(!task3.IsPosted());

        VerifyOrQuit(scheduler.AreTaskletsPending());

        task3.Post();

        VerifyOrQuit(!task1.IsPosted());
        VerifyOrQuit(task2.IsPosted());
        VerifyOrQuit(task3.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(scheduler.AreTaskletsPending());

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(!sTask1Handled);
        VerifyOrQuit(sTask2Handled);
        VerifyOrQuit(sTask3Handled);

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());
    }
}

} // namespace ot

int main(void)
{
    ot::TestTasklet();
    printf("All tests passed\n");
    return 0;
}
