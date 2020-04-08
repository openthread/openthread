/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file includes definitions for Thread child table.
 */

#include "child_table.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

namespace ot {

#if OPENTHREAD_FTD

ChildTable::Iterator::Iterator(Instance &aInstance, Child::StateFilter aFilter)
    : InstanceLocator(aInstance)
    , mFilter(aFilter)
    , mStart(NULL)
    , mChild(NULL)
{
    Reset();
}

ChildTable::Iterator::Iterator(Instance &aInstance, Child::StateFilter aFilter, Child *aStartingChild)
    : InstanceLocator(aInstance)
    , mFilter(aFilter)
    , mStart(aStartingChild)
    , mChild(NULL)
{
    Reset();
}

void ChildTable::Iterator::Reset(void)
{
    if (mStart == NULL)
    {
        mStart = &Get<ChildTable>().mChildren[0];
    }

    mChild = mStart;

    if (!mChild->MatchesFilter(mFilter))
    {
        Advance();
    }
}

void ChildTable::Iterator::Advance(void)
{
    ChildTable &childTable = Get<ChildTable>();
    Child *     listStart  = &childTable.mChildren[0];
    Child *     listEnd    = &childTable.mChildren[childTable.mMaxChildrenAllowed];

    VerifyOrExit(mChild != NULL, OT_NOOP);

    do
    {
        mChild++;

        if (mChild >= listEnd)
        {
            mChild = listStart;
        }

        VerifyOrExit(mChild != mStart, mChild = NULL);
    } while (!mChild->MatchesFilter(mFilter));

exit:
    return;
}

ChildTable::ChildTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMaxChildrenAllowed(kMaxChildren)
{
    for (Child *child = &mChildren[0]; child < OT_ARRAY_END(mChildren); child++)
    {
        child->Init(aInstance);
        child->Clear();
    }
}

void ChildTable::Clear(void)
{
    for (Child *child = &mChildren[0]; child < OT_ARRAY_END(mChildren); child++)
    {
        child->Clear();
    }
}

Child *ChildTable::GetChildAtIndex(uint16_t aChildIndex)
{
    Child *child = NULL;

    VerifyOrExit(aChildIndex < mMaxChildrenAllowed, OT_NOOP);
    child = &mChildren[aChildIndex];

exit:
    return child;
}

Child *ChildTable::GetNewChild(void)
{
    Child *child = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->IsStateInvalid())
        {
            child->Clear();
            ExitNow();
        }
    }

    child = NULL;

exit:
    return child;
}

Child *ChildTable::FindChild(uint16_t aRloc16, Child::StateFilter aFilter)
{
    Child *child = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->MatchesFilter(aFilter) && (child->GetRloc16() == aRloc16))
        {
            ExitNow();
        }
    }

    child = NULL;

exit:
    return child;
}

Child *ChildTable::FindChild(const Mac::ExtAddress &aAddress, Child::StateFilter aFilter)
{
    Child *child = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->MatchesFilter(aFilter) && (child->GetExtAddress() == aAddress))
        {
            ExitNow();
        }
    }

    child = NULL;

exit:
    return child;
}

Child *ChildTable::FindChild(const Mac::Address &aAddress, Child::StateFilter aFilter)
{
    Child *child = NULL;

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        child = FindChild(aAddress.GetShort(), aFilter);
        break;

    case Mac::Address::kTypeExtended:
        child = FindChild(aAddress.GetExtended(), aFilter);
        break;

    default:
        break;
    }

    return child;
}

bool ChildTable::HasChildren(Child::StateFilter aFilter) const
{
    bool         rval  = false;
    const Child *child = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->MatchesFilter(aFilter))
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

uint16_t ChildTable::GetNumChildren(Child::StateFilter aFilter) const
{
    uint16_t     numChildren = 0;
    const Child *child       = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->MatchesFilter(aFilter))
        {
            numChildren++;
        }
    }

    return numChildren;
}

otError ChildTable::SetMaxChildrenAllowed(uint16_t aMaxChildren)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aMaxChildren > 0 && aMaxChildren <= kMaxChildren, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!HasChildren(Child::kInStateAnyExceptInvalid), error = OT_ERROR_INVALID_STATE);

    mMaxChildrenAllowed = aMaxChildren;

exit:
    return error;
}

#endif // OPENTHREAD_FTD

} // namespace ot
