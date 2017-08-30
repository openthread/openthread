/*
 *
 *    Copyright 2016-2017 The nlfaultinjection Authors.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

/**
 *    @file
 *      This file implements unit tests for FaultInjection.{h,cpp}
 *
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <nlunit-test.h>
#include <nlfaultinjection.hpp>

using namespace nl::FaultInjection;

/**
 * The list of fault IDs
 */
typedef enum
{
    kTestFaultInjectionID_A,
    kTestFaultInjectionID_B,
    kTestFaultInjectionID_NumItems,

} TestFaultInjectionID;

static const char *sFaultNames[kTestFaultInjectionID_NumItems] = {
    "A",
    "B",
};

static const char *sManagerName = "TestFaultMgr";

/**
 * Storage for the FaultInjectionManager
 */
static Record sFaultRecordArray[kTestFaultInjectionID_NumItems];

static int32_t sFaultAArguments[4];


/**
 * The manager object
 */
static class Manager sTestFaultInMgr;

static int sNumTimesRebooted = 0;
void RebootCB(void)
{
    sNumTimesRebooted++;
}

static int sNumTimesPrinted = 0;
void PostInjectionCB(Manager *aManager, Identifier aId, Record *aFaultRecord)
{
    sNumTimesPrinted++;
    printf("PostInjectionCB: %s, fault %d - %s, numTimesChecked: %u\n",
            aManager->GetName(), aId, aManager->GetFaultNames()[aId], aFaultRecord->mNumTimesChecked);
}

static GlobalContext sGlobalContext = {
    {
        RebootCB,
        NULL
    }
};

/**
 * Getter for the manager object
 */
static Manager &GetTestFIMgr(void)
{
    if (0 == sTestFaultInMgr.GetNumFaults())
    {
        sTestFaultInMgr.Init(kTestFaultInjectionID_NumItems, sFaultRecordArray, sManagerName, sFaultNames);
        memset(&sFaultAArguments, 0, sizeof(sFaultAArguments));
        sFaultRecordArray[kTestFaultInjectionID_A].mArguments = sFaultAArguments;
        sFaultRecordArray[kTestFaultInjectionID_A].mLengthOfArguments =
            static_cast<uint16_t>(sizeof(sFaultAArguments)/sizeof(sFaultAArguments[0]));
    }
    return sTestFaultInMgr;
}

static int sLockCounter = 0;
static nlTestSuite *sSuite;
void TestLock(void *inLockContext)
{
    int *counter = static_cast<int *>(inLockContext);

    NL_TEST_ASSERT(sSuite, *counter == 0);

    (*counter)++;
}

void TestUnlock(void *inLockContext)
{
    int *counter = static_cast<int *>(inLockContext);

    NL_TEST_ASSERT(sSuite, *counter == 1);

    (*counter)--;
}

#define TEST_FAULT_INJECT( aId, aStatements ) \
    nlFAULT_INJECT(GetTestFIMgr(), aId, aStatements)

#define TEST_FAULT_INJECT_WITH_ARGS( aId, aProtectedStatements, aUnprotectedStatements ) \
    nlFAULT_INJECT_WITH_ARGS(GetTestFIMgr(), aId, aProtectedStatements, aUnprotectedStatements )

static bool DoA()
{
    bool retval = false;

    // Show that we can compile in a whole block; the simplest form is:
    // TEST_FAULT_INJECT(kTestFaultInjectionID_A, retval = true);

    TEST_FAULT_INJECT(kTestFaultInjectionID_A,
                      {
                          int tmp = 0;
                          tmp++;
                          retval = true;
                          --tmp;
                      });

    return retval;
}

static int DoAWithArgs()
{
    int retval = 0;

    // Show that we can access the arguments saved in the Record

    TEST_FAULT_INJECT_WITH_ARGS(kTestFaultInjectionID_A,
                      {
                          int i;
                          for (i = 0; i < numFaultArgs; i++)
                          {
                              printf("arg %d: %d\n", i, faultArgs[i]);
                              retval += faultArgs[i];
                          }
                      },
                      {
                          printf("printing without the lock: counter: %d\n", sLockCounter);
                          NL_TEST_ASSERT(sSuite, sLockCounter == 0);
                      });

    return retval;
}

static int DoAExportingArgs()
{
    int retval = 0;

    // Show that we can save arguments in the Record from the fault injection
    // location as a way to export them to the test harness

    {
        // Usually this block would be implemented with a macro

        nl::FaultInjection::Manager &mgr = GetTestFIMgr();
        const nl::FaultInjection::Record *records = mgr.GetFaultRecords();

        // Check for existing arguments to tell if the harness has
        // installed arguments already, not to override them.
        if (records[kTestFaultInjectionID_A].mNumArguments == 0)
        {
            // Assuming this fault id takes two arguments, lets save 4 values,
            // as to tell the harness that there are two interesting test cases here
            // The application developer needs to reserve enough space in
            // records[kTestFaultInjectionID_A].mArguments
            int32_t args[] = { 1, 2, 10, 20 };
            uint16_t numArgs = 4;
            mgr.StoreArgsAtFault(kTestFaultInjectionID_A, numArgs, args);
        }
    }

    TEST_FAULT_INJECT_WITH_ARGS(kTestFaultInjectionID_A,
                      {
                          int i;
                          for (i = 0; i < numFaultArgs; i++)
                          {
                              printf("arg %d: %d\n", i, faultArgs[i]);
                              retval += faultArgs[i];
                          }
                      },
                      {
                          printf("printing without the lock: counter: %d\n", sLockCounter);
                          NL_TEST_ASSERT(sSuite, sLockCounter == 0);
                      });
    return retval;
}

/**
 * This test uses three callbacks; they are stored in an array so
 * the test can iterate over them
 */
enum {
    kNumCallbacks = 3
};

/**
 * Array of counters, one per callback
 */
static int sCbFnCalled[kNumCallbacks];

/**
 * Array or values to be returned by the callbacks
 */
static bool sCbFnRetval[kNumCallbacks];

/**
 * Whether callbacks should remove themself
 */
static bool sCbRemoveItself[kNumCallbacks];

/**
 * Boolean to tell callback 0 to enable the second fault
 */
static bool sTriggerFault2;

static bool cbFn0(Identifier aFaultID, Record *aFaultRecord, void *aContext);
static bool cbFn1(Identifier aFaultID, Record *aFaultRecord, void *aContext);
static bool cbFn2(Identifier aFaultID, Record *aFaultRecord, void *aContext);

static Callback sCb[kNumCallbacks] = { { cbFn0, NULL, NULL }, { cbFn1, NULL, NULL }, { cbFn2, NULL, NULL } };

static bool cbFn0(Identifier aFaultID, Record *aFaultRecord, void *aContext)
{
    (void)aFaultID;
    (void)aFaultRecord;
    (void)aContext;

    Manager fMgr = GetTestFIMgr();

    sCbFnCalled[0]++;

    if (sTriggerFault2)
    {
        (void)fMgr.FailAtFault(kTestFaultInjectionID_B, 0, 1, Manager::kMutexDoNotTake);
    }

    if (sCbRemoveItself[0])
    {
        (void)fMgr.RemoveCallbackAtFault(aFaultID, &sCb[0], Manager::kMutexDoNotTake);

    }

    return sCbFnRetval[0];
}

static bool cbFn1(Identifier aFaultID, Record *aFaultRecord, void *aContext)
{
    (void)aFaultID;
    (void)aFaultRecord;
    (void)aContext;

    sCbFnCalled[1]++;
    return sCbFnRetval[1];
}
static bool cbFn2(Identifier aFaultID, Record *aFaultRecord, void *aContext)
{
    (void)aFaultID;
    (void)aFaultRecord;
    (void)aContext;

    sCbFnCalled[2]++;
    return sCbFnRetval[2];
}

static nl::FaultInjection::Callback sHarvestArgsID_ACb;
static bool cbToHarvestArgs(Identifier aFaultID, Record *aFaultRecord, void *aContext)
{
    nl::FaultInjection::Manager &mgr = GetTestFIMgr();
    const char *faultName = mgr.GetFaultNames()[aFaultID];
    (void)aContext;

    if (aFaultID == kTestFaultInjectionID_A)
    {
        uint16_t numArgs = aFaultRecord->mNumArguments;
        if (numArgs)
        {
            int i = 0;
            while (i < numArgs)
            {
                // The harness can grep for strings like this and find the test cases to run
                // in subsequent executions
                printf("Found test case: %s_%s_s%u_a%u_a%u;\n", mgr.GetName(), faultName, aFaultRecord->mNumTimesChecked,
                        aFaultRecord->mArguments[i], aFaultRecord->mArguments[i+1]);
                i += 2;
            }

            // also just copy the array out for the sake of this test
            int32_t *output = static_cast<int32_t *>(aContext);
            memcpy(output, aFaultRecord->mArguments, aFaultRecord->mNumArguments * sizeof(aFaultRecord->mArguments[0]));
        }
    }

    // This callback never triggers the fault
    return false;
}


/**
 * Test FailAtFault
 */
void TestFailAtFault(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager &fMgr = GetTestFIMgr();
    bool shouldFail;
    nl::FaultInjection::Identifier id;
    uint32_t i;
    uint32_t maxTimesToFail = 10;
    uint32_t maxTimesToSkip = 10;
    uint32_t timesToFail = 0;
    uint32_t timesToSkip = 0;

    (void)inContext;

    sSuite = inSuite;

    // fMgr is a singleton, get it twice
    fMgr = GetTestFIMgr();

    SetGlobalContext(&sGlobalContext);

    for (id = 0; id < kTestFaultInjectionID_NumItems; id++)
    {
        shouldFail = fMgr.CheckFault(id);
        NL_TEST_ASSERT(inSuite, shouldFail == false);
    }

    // out of boundary
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_NumItems);
    NL_TEST_ASSERT(inSuite, shouldFail == false);

    // Test a few combinations of timesToSkip and timesToFail
    for (timesToFail = 0; timesToFail <= maxTimesToFail; timesToFail++)
    {
        for (timesToSkip = 0; timesToSkip <= maxTimesToSkip; timesToSkip++)
        {
            err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
            NL_TEST_ASSERT(inSuite, err == 0);

            shouldFail = fMgr.CheckFault(kTestFaultInjectionID_B);
            NL_TEST_ASSERT(inSuite, shouldFail == false);

            for (i = 0; i < timesToSkip; i++)
            {
                shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
                NL_TEST_ASSERT(inSuite, shouldFail == false);
            }

            for (i = 0; i < timesToFail; i++)
            {
                shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
                NL_TEST_ASSERT(inSuite, shouldFail);
            }

            shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
            NL_TEST_ASSERT(inSuite, shouldFail == false);
        }
    }
}

/**
 * TestRebootAndPrintAtFault
 */
void TestRebootAndPrintAtFault(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager &fMgr = GetTestFIMgr();
    bool shouldFail;
    uint32_t timesToFail = 1;
    uint32_t timesToSkip = 0;

    (void)inContext;

    sSuite = inSuite;

    // fMgr is a singleton, get it twice
    fMgr = GetTestFIMgr();

    SetGlobalContext(&sGlobalContext);

    // Enable logging, to see that it works
    sNumTimesPrinted = 0;
    sGlobalContext.mCbTable.mPostInjectionCb = PostInjectionCB;

    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, sNumTimesPrinted == 0);

    sNumTimesRebooted = 0;

    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);

    err = fMgr.RebootAtFault(kTestFaultInjectionID_NumItems);
    NL_TEST_ASSERT(inSuite, err == -EINVAL);

    err = fMgr.RebootAtFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, err == 0);

    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == true);
    NL_TEST_ASSERT(inSuite, sNumTimesRebooted == 1);
    NL_TEST_ASSERT(inSuite, sNumTimesPrinted == 1);

    sGlobalContext.mCbTable.mPostInjectionCb = NULL;
}

/**
 * Test the macro
 */
void TestTheMacro(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager &fMgr = GetTestFIMgr();
    bool failed;
    uint32_t timesToSkip = 0;
    uint32_t timesToFail = 1;

    (void)inContext;

    sSuite = inSuite;

    failed = DoA();
    NL_TEST_ASSERT(inSuite, failed == false);

    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);

    failed = DoA();
    NL_TEST_ASSERT(inSuite, failed);
}

/**
 * Test callbacks
 */
void TestInsertRemoveCallback(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager fMgr = GetTestFIMgr();
    bool shouldFail;
    int i, j;

    (void)inContext;

    sSuite = inSuite;

    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_NumItems, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == -EINVAL);

    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, NULL);
    NL_TEST_ASSERT(inSuite, err == -EINVAL);

    // Try removing a callback that's not installed
    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    // Now add it
    err = fMgr.InsertCallbackAtFault(kTestFaultInjectionID_A, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    // Add it again, should be a no-op (the callback should be called only once)
    err = fMgr.InsertCallbackAtFault(kTestFaultInjectionID_A, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    // Try removing one that's not installed with a non-empty list
    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sCb[1]);
    NL_TEST_ASSERT(inSuite, err == 0);

    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == false);
    NL_TEST_ASSERT(inSuite, sCbFnCalled[0] == 1);
    sCbFnCalled[0] = 0;
    NL_TEST_ASSERT(inSuite, sCbFnCalled[1] == 0);

    // Say the fault is on from the callback
    sCbFnRetval[0] = true;
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail);
    sCbFnRetval[0] = false;

    // Turn on the second fault from the first callback; the first should return false,
    // and then the second should return true
    sTriggerFault2 = true;
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == false);
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_B);
    NL_TEST_ASSERT(inSuite, shouldFail);

    // Remove it
    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    sCbFnCalled[0] = 0;
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == false);
    NL_TEST_ASSERT(inSuite, sCbFnCalled[0] == 0);

    sTriggerFault2 = false;

    // Given three callback on the same fault, test removal of the first one, of the
    // one in the middle and of the last one.
    // Keep in mind though that the last one is not really the last one of the list:
    // all lists end in the two default callbacks.
    for (i = 0; i < kNumCallbacks; i++)
    {
        // add all three
        for (j = 0; j < kNumCallbacks; j++)
        {
            sCbFnCalled[j] = 0;
            err = fMgr.InsertCallbackAtFault(kTestFaultInjectionID_A, &sCb[j]);
            NL_TEST_ASSERT(inSuite, err == 0);
        }

        // remove one
        err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sCb[i]);
        NL_TEST_ASSERT(inSuite, err == 0);

        // trigger
        shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
        NL_TEST_ASSERT(inSuite, shouldFail == false);

        for (j = 0; j < kNumCallbacks; j++)
        {
            if (j == i)
            {
                NL_TEST_ASSERT(inSuite, sCbFnCalled[j] == 0);
            }
            else
            {
                NL_TEST_ASSERT(inSuite, sCbFnCalled[j] == 1);
            }
        }

        // remove all of them
        for (j = 0; j < kNumCallbacks; j++)
        {
            err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sCb[j]);
            NL_TEST_ASSERT(inSuite, err == 0);
            sCbFnCalled[j] = 0;
        }

        // check that they're all gone
        shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
        NL_TEST_ASSERT(inSuite, shouldFail == false);
        for (j = 0; j < kNumCallbacks; j++)
        {
            NL_TEST_ASSERT(inSuite, sCbFnCalled[j] == 0);
        }
    }
}

/**
 * Test a callback that removes itself
 */
void TestCallbackRemovesItself(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager fMgr = GetTestFIMgr();
    bool shouldFail;

    (void)inContext;

    sSuite = inSuite;

    err = fMgr.InsertCallbackAtFault(kTestFaultInjectionID_A, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    sCbRemoveItself[0] = true;
    sCbFnRetval[0] = true;

    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == true);

    // Now it returns false, because the callback is gone
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == false);

    sCbRemoveItself[0] = false;
    sCbFnRetval[0] = false;
}

/**
 * Test random failures
 */
void TestFailRandomly(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager fMgr = GetTestFIMgr();
    bool shouldFail;
    uint8_t percentage = 80;
    int numFailures = 0;
    int numIterations = 100;
    int i;

    (void)inContext;

    sSuite = inSuite;

    err = fMgr.FailRandomlyAtFault(kTestFaultInjectionID_A, percentage);
    NL_TEST_ASSERT(inSuite, err == 0);
    for (i = 0; i < numIterations; i++)
    {
        shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
        if (shouldFail)
            numFailures++;
    }

    printf("numFailures: %d\n", numFailures);

    // At the time of this writing, I'm getting 75-82 failures out of 100
    NL_TEST_ASSERT(inSuite, numFailures > (numIterations/2));

    percentage = 20;
    numFailures = 0;
    err = fMgr.FailRandomlyAtFault(kTestFaultInjectionID_A, percentage);
    NL_TEST_ASSERT(inSuite, err == 0);
    for (i = 0; i < numIterations; i++)
    {
        shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
        if (shouldFail)
            numFailures++;
    }

    printf("numFailures: %d\n", numFailures);

    // At the time of this writing, I'm getting 18-22 failures out of 100
    NL_TEST_ASSERT(inSuite, numFailures < (numIterations/2));

    err = fMgr.FailRandomlyAtFault(kTestFaultInjectionID_A, 0);
    NL_TEST_ASSERT(inSuite, err == 0);
}

/**
 * Test StoreArgsAtFault
 */
void TestArguments(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager fMgr = GetTestFIMgr();
    bool shouldFail;
    uint32_t timesToFail = 1;
    uint32_t timesToSkip = 0;
    int32_t args[] = { 42, 24 };
    uint16_t numArgs = static_cast<uint16_t>(sizeof(args)/sizeof(args[0]));
    int32_t *outArgs = NULL;
    uint16_t outNumArgs = 0;
    int retval;

    (void)inContext;

    sSuite = inSuite;

    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);
    err = fMgr.StoreArgsAtFault(kTestFaultInjectionID_A, numArgs, args);
    NL_TEST_ASSERT(inSuite, err == 0);
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A, outNumArgs, outArgs);
    NL_TEST_ASSERT(inSuite, shouldFail);
    NL_TEST_ASSERT(inSuite, outNumArgs == numArgs);
    NL_TEST_ASSERT(inSuite, args[0] == outArgs[0]);
    NL_TEST_ASSERT(inSuite, args[1] == outArgs[1]);

    // Now test the handling of arguments in the macro
    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);
    retval = DoAWithArgs();
    NL_TEST_ASSERT(inSuite, retval == (args[0] + args[1]));
}


/**
 * Test ParseFaultInjectionStr
 */
void TestParser(nlTestSuite *inSuite, void *inContext)
{
    Manager &fMgr = GetTestFIMgr();
    bool shouldFail;
    bool parserVal;
    nl::FaultInjection::Identifier id;
    nl::FaultInjection::GetManagerFn faultMgrTable[] = {
        GetTestFIMgr,
    };
    char buffer[80];
    const char *singleConfig = "TestFaultMgr_A_s0_f1";
    const char *doubleConfig = "TestFaultMgr_A_s0_f1:TestFaultMgr_B_p50";
    const char *singleConfigWithArgs = "TestFaultMgr_A_s0_f1_a12_a-7";
    int retval = 0;
    int expectedRetVal = (12 -7);

    (void)inContext;

    sSuite = inSuite;

    for (id = 0; id < kTestFaultInjectionID_NumItems; id++)
    {
        shouldFail = fMgr.CheckFault(id);
        NL_TEST_ASSERT(inSuite, shouldFail == false);
    }

    strncpy(buffer, singleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == true);

    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == true);
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_B);
    NL_TEST_ASSERT(inSuite, shouldFail == false);

    strncpy(buffer, doubleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == true);
    shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, shouldFail == true);
    NL_TEST_ASSERT(inSuite, sFaultRecordArray[1].mPercentage == 50);

    /* reboot */
    sNumTimesRebooted = 0;
    singleConfig = "TestFaultMgr_A_s0_f1_r";
    strncpy(buffer, singleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == true);
    NL_TEST_ASSERT(inSuite, sFaultRecordArray[0].mReboot == true);

    /* passing parameters */
    strncpy(buffer, singleConfigWithArgs, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == true);
    retval = DoAWithArgs();
    NL_TEST_ASSERT(inSuite, retval == expectedRetVal);

    /* Bad strings*/
    singleConfig = "TestFaultMgr_C_s0_f1";
    doubleConfig = "TestFaultMgr_A_g0_f1:TestFaultMgr_B_r50";

    strncpy(buffer, singleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == false);

    strncpy(buffer, doubleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == false);

    /* Bad percentage values */
    singleConfig = "TestFaultMgr_A_p101";
    strncpy(buffer, singleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == false);

    singleConfig = "TestFaultMgr_A_p-1";
    strncpy(buffer, singleConfig, sizeof(buffer));
    parserVal = nl::FaultInjection::ParseFaultInjectionStr(buffer,
                faultMgrTable, sizeof(faultMgrTable)/sizeof(faultMgrTable[0]));
    NL_TEST_ASSERT(inSuite, parserVal == false);
}

/**
 * Test Exporting argument values to be used in future test runs
 */
void TestExportArguments(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager fMgr = GetTestFIMgr();
    bool failed = false;
    int32_t outputContext[4];
    (void)inContext;

    sSuite = inSuite;

    // cleanup
    fMgr.StoreArgsAtFault(kTestFaultInjectionID_A, 0, NULL);

    /*
     * Install a callback to harvest the arguments during a run without
     * faults
     */
    memset(&sHarvestArgsID_ACb, 0, sizeof(sHarvestArgsID_ACb));
    sHarvestArgsID_ACb.mCallBackFn = cbToHarvestArgs;
    sHarvestArgsID_ACb.mContext = outputContext;
    fMgr.InsertCallbackAtFault(kTestFaultInjectionID_A, &sHarvestArgsID_ACb);

    /*
     * During a sequence without faults, save useful arguments in the
     * mArguments array
     */

    failed = DoAExportingArgs();
    NL_TEST_ASSERT(inSuite, failed == false);

    /* Check the right values got exported */
    NL_TEST_ASSERT(inSuite, outputContext[0] == 1);
    NL_TEST_ASSERT(inSuite, outputContext[1] == 2);
    NL_TEST_ASSERT(inSuite, outputContext[2] == 10);
    NL_TEST_ASSERT(inSuite, outputContext[3] == 20);

    /* Now a real application would use the values collected above for two more tests */

    /* cleanup */
    err = fMgr.RemoveCallbackAtFault(kTestFaultInjectionID_A, &sHarvestArgsID_ACb);
    NL_TEST_ASSERT(inSuite, err == 0);
}

/**
 * Test ResetFaultCounters
 */
void TestResetFaultCounters(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager &fMgr = GetTestFIMgr();
    nl::FaultInjection::Identifier id = kTestFaultInjectionID_A;
    uint32_t timesToFail = 2;
    uint32_t timesToSkip = 2;
    bool shouldFail;
    uint32_t i;

    (void)inContext;

    sSuite = inSuite;

    (void)fMgr.CheckFault(kTestFaultInjectionID_A);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumTimesChecked != 0);

    fMgr.ResetFaultCounters();
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumTimesChecked == 0);

    // Now check that resetting the counters does not break the configuration
    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);

    for (i = 0; i < timesToFail + timesToSkip; i++)
    {
        fMgr.ResetFaultCounters();
        NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumTimesChecked == 0);

        shouldFail = fMgr.CheckFault(kTestFaultInjectionID_A);
        if (i < timesToSkip)
        {
            NL_TEST_ASSERT(inSuite, shouldFail == false);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, shouldFail == true);
        }
        NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumTimesChecked != 0);
    }
}

/**
 * Test ResetFaultConfigurations
 */
void TestResetFaultConfigurations(nlTestSuite *inSuite, void *inContext)
{
    int32_t err;
    Manager &fMgr = GetTestFIMgr();
    nl::FaultInjection::Identifier id = kTestFaultInjectionID_A;
    uint32_t timesToFail = 7;
    uint32_t timesToSkip = 8;
    uint8_t percentage = 80;

    (void)inContext;

    sSuite = inSuite;

    err = fMgr.FailAtFault(kTestFaultInjectionID_A, timesToSkip, timesToFail);
    NL_TEST_ASSERT(inSuite, err == 0);

    err = fMgr.ResetFaultConfigurations();
    NL_TEST_ASSERT(inSuite, err == 0);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumCallsToSkip == 0);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mNumCallsToFail == 0);

    err = fMgr.FailRandomlyAtFault(id, percentage);
    NL_TEST_ASSERT(inSuite, err == 0);

    err = fMgr.ResetFaultConfigurations();
    NL_TEST_ASSERT(inSuite, err == 0);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mPercentage == 0);

    err = fMgr.InsertCallbackAtFault(id, &sCb[0]);
    NL_TEST_ASSERT(inSuite, err == 0);

    err = fMgr.ResetFaultConfigurations();
    NL_TEST_ASSERT(inSuite, err == 0);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mCallbackList != &sCb[0]);

    err = fMgr.RebootAtFault(id);
    NL_TEST_ASSERT(inSuite, err == 0);

    err = fMgr.ResetFaultConfigurations();
    NL_TEST_ASSERT(inSuite, err == 0);
    NL_TEST_ASSERT(inSuite, fMgr.GetFaultRecords()[id].mReboot == false);
}

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Test FailAtFault",                TestFailAtFault),
    NL_TEST_DEF("Test RebootAndPrintAtFault",      TestRebootAndPrintAtFault),
    NL_TEST_DEF("Test the macro",                  TestTheMacro),
    NL_TEST_DEF("Test InsertRemoveCallback",       TestInsertRemoveCallback),
    NL_TEST_DEF("Test CallbackRemovesItself",      TestCallbackRemovesItself),
    NL_TEST_DEF("Test Random failures",            TestFailRandomly),
    NL_TEST_DEF("Test Parser",                     TestParser),
    NL_TEST_DEF("Test Arguments",                  TestArguments),
    NL_TEST_DEF("Test Exporting useful arguments", TestExportArguments),
    NL_TEST_DEF("Test ResetFaultCounters",         TestResetFaultCounters),
    NL_TEST_DEF("Test ResetFaultConfigurations",   TestResetFaultConfigurations),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    (void)inContext;

    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    (void)inContext;

    return (SUCCESS);
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
    nl::FaultInjection::Manager &mgr = GetTestFIMgr();
    (void)argc;
    (void)argv;

    srand(static_cast<unsigned int>(time(NULL)));

    nlTestSuite theSuite;
    memset(&theSuite, 0, sizeof(theSuite));
    theSuite.name = "fault-injection";
    theSuite.tests = &sTests[0];
    theSuite.setup = TestSetup;
    theSuite.tear_down = TestTeardown;

    // Set the critical section callbacks once here, instead of in every test
    mgr.SetLockCallbacks(TestLock, TestUnlock, &sLockCounter);

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
