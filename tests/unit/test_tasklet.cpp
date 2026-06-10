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
static Tasklet  *sTask1                   = nullptr;
static bool      sTask1Handled            = false;
static bool      sTask2Handled            = false;
static bool      sTask3Handled            = false;
static bool      sTask4Handled            = false;
static bool      sTask5Handled            = false;
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
    sTask4Handled        = false;
    sTask5Handled        = false;
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

void HandleTask4(Tasklet &aTasklet)
{
    Log("   HandleTask4() - will post task1");
    CheckTaskeltFromHandler(aTasklet);
    VerifyOrQuit(!sTask4Handled);
    sTask4Handled = true;

    sTask1->Post();
}

void HandleTask5(Tasklet &aTasklet)
{
    Log("   HandleTask5() - will un-post task1");
    CheckTaskeltFromHandler(aTasklet);
    VerifyOrQuit(!sTask5Handled);
    sTask5Handled = true;

    sTask1->Unpost();
}

void TestTasklet(void)
{
    Log("TestTasklet");

    sInstance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(sInstance != nullptr);

    {
        Tasklet::Scheduler &scheduler = sInstance->Get<Tasklet::Scheduler>();
        Tasklet             task1(*sInstance, HandleTask1);
        Tasklet             task2(*sInstance, HandleTask2);
        Tasklet             task3(*sInstance, HandleTask3);
        Tasklet             task4(*sInstance, HandleTask4);
        Tasklet             task5(*sInstance, HandleTask5);

        sTask1 = &task1;

        Log("Process all initially posted tasks after `Instance` initialization");

        while (scheduler.AreTaskletsPending())
        {
            scheduler.ProcessQueuedTasklets();
        }

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

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task4 (which posts task1 from its handler) and then task1");
        Log("We expect the posting of task1 (from task4 handler) to be ignored since it is already posted");

        ResetTestFlags();

        task4.Post();
        task1.Post();

        VerifyOrQuit(task4.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask4Handled);
        VerifyOrQuit(sTask1Handled);

        VerifyOrQuit(!task4.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task1 first and then task4 (which posts task1 from its handler) now");
        Log("Since task1 is first, it should be handled before task4's handler and task4's handler should post task1");

        ResetTestFlags();

        task1.Post();
        task4.Post();

        VerifyOrQuit(task4.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask4Handled);
        VerifyOrQuit(sTask1Handled);

        VerifyOrQuit(!task4.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        VerifyOrQuit(scheduler.AreTaskletsPending());

        ResetTestFlags();

        scheduler.ProcessQueuedTasklets();
        VerifyOrQuit(!sTask4Handled);
        VerifyOrQuit(sTask1Handled);

        VerifyOrQuit(!task4.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task5 (which un-posts task1 from its handler) and then task1");
        Log("Since task5 is first, we expect task1 to be removed and its handler never called");

        ResetTestFlags();

        task5.Post();
        task1.Post();

        VerifyOrQuit(task5.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask5Handled);
        VerifyOrQuit(!sTask1Handled);

        VerifyOrQuit(!task5.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task1 first, then task5 (which un-posts task1 from its handler)");
        Log("Both tasks should be handled");

        ResetTestFlags();

        task1.Post();
        task5.Post();

        VerifyOrQuit(task1.IsPosted());
        VerifyOrQuit(task5.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask1Handled);
        VerifyOrQuit(sTask5Handled);

        VerifyOrQuit(!task5.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task5 on its own, which un-posts task1 from its handler - it should do nothing to task1");

        ResetTestFlags();

        task5.Post();

        VerifyOrQuit(task5.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask5Handled);

        VerifyOrQuit(!task5.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(!sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task4 (which posts task1 from its handler), then task5 (which un-posts task1 from its handler)");
        Log("The two should cancel each other and at the end task1 should not be posted");

        ResetTestFlags();

        task4.Post();
        task5.Post();

        VerifyOrQuit(task4.IsPosted());
        VerifyOrQuit(task5.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask4Handled);
        VerifyOrQuit(sTask5Handled);
        VerifyOrQuit(!sTask1Handled);

        VerifyOrQuit(!task5.IsPosted());
        VerifyOrQuit(!task4.IsPosted());
        VerifyOrQuit(!task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        VerifyOrQuit(!scheduler.AreTaskletsPending());

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Post task5 (which un-posts task1 from its handler), then task1, and then task4 (which posts task1)");
        Log("The task1 should be removed while processing task5, but then posted again from task4");

        ResetTestFlags();

        task5.Post();
        task1.Post();
        task4.Post();

        VerifyOrQuit(task4.IsPosted());
        VerifyOrQuit(task5.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        sSignalPendingCalled = false;

        scheduler.ProcessQueuedTasklets();

        VerifyOrQuit(sTask5Handled);
        VerifyOrQuit(sTask4Handled);
        VerifyOrQuit(!sTask1Handled);

        VerifyOrQuit(!task5.IsPosted());
        VerifyOrQuit(!task4.IsPosted());
        VerifyOrQuit(task1.IsPosted());

        VerifyOrQuit(sSignalPendingCalled);
        VerifyOrQuit(scheduler.AreTaskletsPending());

        // Handle the posted task1
        ResetTestFlags();

        scheduler.ProcessQueuedTasklets();
        VerifyOrQuit(sTask1Handled);

        VerifyOrQuit(!task1.IsPosted());

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
