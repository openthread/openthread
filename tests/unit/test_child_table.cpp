/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include <openthread/config.h>

#include "test_util.h"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "thread/child_table.hpp"

namespace ot {

static ot::Instance *sInstance;

enum
{
    kMaxChildren = OPENTHREAD_CONFIG_MAX_CHILDREN,
};

struct TestChild
{
    Child::State mState;
    uint16_t     mRloc16;
    otExtAddress mExtAddress;
};

const ChildTable::StateFilter kAllFilters[] = {
    ChildTable::kInStateValid,
    ChildTable::kInStateValidOrRestoring,
    ChildTable::kInStateChildIdRequest,
    ChildTable::kInStateValidOrAttaching,
    ChildTable::kInStateAnyExceptInvalid,
};

// Checks whether a `Child` matches the `TestChild` struct.
static bool ChildMatches(const Child &aChild, const TestChild &aTestChild)
{
    return (aChild.GetState() == aTestChild.mState) && (aChild.GetRloc16() == aTestChild.mRloc16) &&
           (aChild.GetExtAddress() == static_cast<const Mac::ExtAddress &>(aTestChild.mExtAddress));
}

// Checks whether a `Child::State` matches a `ChildTable::StateFilter`.
static bool StateMatchesFilter(Child::State aState, ChildTable::StateFilter aFilter)
{
    bool  rval = false;
    Child child;

    child.SetState(aState);

    switch (aFilter)
    {
    case ChildTable::kInStateAnyExceptInvalid:
        rval = (aState != Child::kStateInvalid);
        break;

    case ChildTable::kInStateValid:
        rval = (aState == Child::kStateValid);
        break;

    case ChildTable::kInStateValidOrRestoring:
        rval = child.IsStateValidOrRestoring();
        break;

    case ChildTable::kInStateChildIdRequest:
        rval = (aState == Child::kStateChildIdRequest);
        break;

    case ChildTable::kInStateValidOrAttaching:
        rval = child.IsStateValidOrAttaching();
        break;

    case ChildTable::kInStateAnyExceptValidOrRestoring:
        rval = !child.IsStateValidOrRestoring();
        break;
    }

    return rval;
}

// Verifies that `ChildTable` contains a given list of `TestChild` entries.
void VerifyChildTableContent(ChildTable &aTable, uint8_t aChildListLength, const TestChild *aChildList)
{
    printf("Test ChildTable with %d entries", aChildListLength);

    for (uint8_t k = 0; k < OT_ARRAY_LENGTH(kAllFilters); k++)
    {
        ChildTable::StateFilter filter = kAllFilters[k];

        // Verify that we can find all children from given list by rloc or extended address.

        for (uint8_t listIndex = 0; listIndex < aChildListLength; listIndex++)
        {
            Child *      child;
            Mac::Address address;

            if (!StateMatchesFilter(aChildList[listIndex].mState, filter))
            {
                continue;
            }

            child = aTable.FindChild(aChildList[listIndex].mRloc16, filter);
            VerifyOrQuit(child != NULL, "FindChild(rloc) failed");
            VerifyOrQuit(ChildMatches(*child, aChildList[listIndex]), "FindChild(rloc) returned incorrect child");

            child = aTable.FindChild(static_cast<const Mac::ExtAddress &>(aChildList[listIndex].mExtAddress), filter);
            VerifyOrQuit(child != NULL, "FindChild(ExtAddress) failed");
            VerifyOrQuit(ChildMatches(*child, aChildList[listIndex]), "FindChild(ExtAddress) returned incorrect child");

            address.SetShort(aChildList[listIndex].mRloc16);
            child = aTable.FindChild(address, filter);
            VerifyOrQuit(child != NULL, "FindChild(address) failed");
            VerifyOrQuit(ChildMatches(*child, aChildList[listIndex]), "FindChild(address) returned incorrect child");

            address.SetExtended(static_cast<const Mac::ExtAddress &>(aChildList[listIndex].mExtAddress));
            child = aTable.FindChild(address, filter);
            VerifyOrQuit(child != NULL, "FindChild(address) failed");
            VerifyOrQuit(ChildMatches(*child, aChildList[listIndex]), "FindChild(address) returned incorrect child");
        }

        // Verify `ChildTable::Iterator` behavior when starting from different child entries.

        for (uint8_t listIndex = 0; listIndex <= aChildListLength; listIndex++)
        {
            Child *startingChild = NULL;

            if (listIndex < aChildListLength)
            {
                startingChild = aTable.FindChild(aChildList[listIndex].mRloc16, ChildTable::kInStateAnyExceptInvalid);
                VerifyOrQuit(startingChild != NULL, "FindChild() failed");
            }

            // Test an iterator starting from `startingChild`.

            {
                ChildTable::Iterator iter(*sInstance, filter, startingChild);
                bool                 childObserved[kMaxChildren];
                uint8_t              numChildren = 0;

                memset(childObserved, 0, sizeof(childObserved));

                // Check if the first entry matches the `startingChild`

                if ((startingChild != NULL) && StateMatchesFilter(startingChild->GetState(), filter))
                {
                    VerifyOrQuit(!iter.IsDone(), "iterator IsDone() failed");
                    VerifyOrQuit(iter.GetChild() != NULL, "iterator GetChild() failed");
                    VerifyOrQuit(iter.GetChild() == startingChild,
                                 "Iterator failed to start from the given child entry");

                    iter++;
                    iter.Reset();
                    VerifyOrQuit(iter.GetChild() == startingChild, "iterator Reset() failed");
                }

                // Use the iterator and verify that each returned `Child` entry is in the expected list.

                for (; !iter.IsDone(); iter++)
                {
                    Child * child   = iter.GetChild();
                    bool    didFind = false;
                    uint8_t childIndex;

                    VerifyOrQuit(child != NULL, "iter.GetChild() failed");

                    childIndex = aTable.GetChildIndex(*child);
                    VerifyOrQuit(childIndex < aTable.GetMaxChildrenAllowed(), "Child Index is out of bound");
                    VerifyOrQuit(aTable.GetChildAtIndex(childIndex) == child, "GetChildAtIndex() failed");

                    for (uint8_t index = 0; index < aChildListLength; index++)
                    {
                        if (ChildMatches(*iter.GetChild(), aChildList[index]))
                        {
                            childObserved[index] = true;
                            numChildren++;
                            didFind = true;
                            break;
                        }
                    }

                    VerifyOrQuit(didFind, "ChildTable::Iterator returned an entry not in the expected list");
                }

                // Verify that when iterator is done, it points to `NULL`.

                VerifyOrQuit(iter.GetChild() == NULL, "iterator GetChild() failed");

                iter++;
                VerifyOrQuit(iter.IsDone(), "iterator Advance() (after iterator is done) failed");
                VerifyOrQuit(iter.GetChild() == NULL, "iterator GetChild() failed");

                // Verify that the number of children matches the number of entries we get from iterator.

                VerifyOrQuit(aTable.GetNumChildren(filter) == numChildren, "GetNumChildren() failed");
                VerifyOrQuit(aTable.HasChildren(filter) == (numChildren != 0), "HasChildren() failed");

                // Verify that there is no missing or extra entry between the expected list
                // and what was observed/returned by the iterator.

                for (uint8_t index = 0; index < aChildListLength; index++)
                {
                    if (StateMatchesFilter(aChildList[index].mState, filter))
                    {
                        VerifyOrQuit(childObserved[index], "iterator failed to return an expected entry");
                    }
                    else
                    {
                        VerifyOrQuit(!childObserved[index], "iterator returned an extra unexpected entry");
                    }
                }
            }
        }
    }

    printf(" -- PASS\n");
}

void TestChildTable(void)
{
    const TestChild testChildList[] = {
        {
            Child::kStateValid,
            0x8001,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x16}},
        },
        {
            Child::kStateParentRequest,
            0x8002,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x17}},
        },
        {
            Child::kStateValid,
            0x8003,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x18}},
        },
        {
            Child::kStateValid,
            0x8004,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x19}},
        },
        {
            Child::kStateRestored,
            0x8005,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x20}},
        },
        {
            Child::kStateValid,
            0x8006,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x21}},
        },
        {
            Child::kStateChildIdRequest,
            0x8007,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x22}},
        },
        {
            Child::kStateChildUpdateRequest,
            0x8008,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x23}},
        },
        {
            Child::kStateParentResponse,
            0x8009,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x24}},
        },
        {
            Child::kStateRestored,
            0x800a,
            {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x25}},
        },
    };

    const uint8_t testListLength = OT_ARRAY_LENGTH(testChildList);

    uint8_t testNumAllowedChildren = 2;

    ChildTable *table;
    otError     error;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != NULL, "Null instance");

    table = &sInstance->Get<ChildTable>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Checking initial state after child table is constructed");

    VerifyOrQuit(table->GetMaxChildrenAllowed() == table->GetMaxChildren(),
                 "GetMaxChildrenAllowed() initial value is incorrect ");

    for (uint8_t i = 0; i < OT_ARRAY_LENGTH(kAllFilters); i++)
    {
        ChildTable::StateFilter filter = kAllFilters[i];

        VerifyOrQuit(table->HasChildren(filter) == false, "HasChildren() failed after init");
        VerifyOrQuit(table->GetNumChildren(filter) == 0, "GetNumChildren() failed after init");
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    VerifyChildTableContent(*table, 0, testChildList);

    VerifyOrQuit(table->GetMaxChildrenAllowed() >= testListLength,
                 "Default child table size is too small for the unit test");

    // Add the child entries from test list one by one and verify the table content
    for (uint8_t i = 0; i < testListLength; i++)
    {
        Child *child;

        child = table->GetNewChild();
        VerifyOrQuit(child != NULL, "GetNewChild() failed");

        child->SetState(testChildList[i].mState);
        child->SetRloc16(testChildList[i].mRloc16);
        child->SetExtAddress((static_cast<const Mac::ExtAddress &>(testChildList[i].mExtAddress)));

        VerifyChildTableContent(*table, i + 1, testChildList);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify Child Table clear

    table->Clear();

    VerifyChildTableContent(*table, 0, testChildList);

    // Add the child entries from test list in reverse order and verify the table content
    for (uint8_t i = testListLength; i > 0; i--)
    {
        Child *child;

        child = table->GetNewChild();
        VerifyOrQuit(child != NULL, "GetNewChild() failed");

        child->SetState(testChildList[i - 1].mState);
        child->SetRloc16(testChildList[i - 1].mRloc16);
        child->SetExtAddress((static_cast<const Mac::ExtAddress &>(testChildList[i - 1].mExtAddress)));

        VerifyChildTableContent(*table, testListLength - i + 1, &testChildList[i - 1]);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Test Get/SetMaxChildrenAllowed");

    error = table->SetMaxChildrenAllowed(kMaxChildren - 1);
    VerifyOrQuit(error == OT_ERROR_INVALID_STATE, "SetMaxChildrenAllowed() should fail when table is not empty");

    table->Clear();
    error = table->SetMaxChildrenAllowed(kMaxChildren + 1);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS, "SetMaxChildrenAllowed() did not fail with an invalid arg");

    error = table->SetMaxChildrenAllowed(0);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS, "SetMaxChildrenAllowed() did not fail with an invalid arg");

    error = table->SetMaxChildrenAllowed(testNumAllowedChildren);
    VerifyOrQuit(error == OT_ERROR_NONE, "SetMaxChildrenAllowed() failed");
    VerifyOrQuit(table->GetMaxChildrenAllowed() == testNumAllowedChildren, "GetMaxChildrenAllowed() failed");

    for (uint8_t num = 0; num < testNumAllowedChildren; num++)
    {
        Child *child = table->GetNewChild();

        VerifyOrQuit(child != NULL, "GetNewChild() failed");
        child->SetState(Child::kStateValid);
    }

    VerifyOrQuit(table->GetNewChild() == NULL, "GetNewChild() did not fail when table was full");

    printf(" -- PASS\n");

    testFreeInstance(sInstance);
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestChildTable();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
