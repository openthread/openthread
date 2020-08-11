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
    , mChild(nullptr)
{
    Reset();
}

void ChildTable::Iterator::Reset(void)
{
    mChild = &Get<ChildTable>().mChildren[0];

    if (!mChild->MatchesFilter(mFilter))
    {
        Advance();
    }
}

void ChildTable::Iterator::Advance(void)
{
    VerifyOrExit(mChild != nullptr, OT_NOOP);

    do
    {
        mChild++;
        VerifyOrExit(mChild < &Get<ChildTable>().mChildren[Get<ChildTable>().mMaxChildrenAllowed], mChild = nullptr);
    } while (!mChild->MatchesFilter(mFilter));

exit:
    return;
}

ChildTable::ChildTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMaxChildrenAllowed(kMaxChildren)
{
    for (Child &child : mChildren)
    {
        child.Init(aInstance);
        child.Clear();
    }
}

void ChildTable::Clear(void)
{
    for (Child &child : mChildren)
    {
        child.Clear();
    }
}

Child *ChildTable::GetChildAtIndex(uint16_t aChildIndex)
{
    Child *child = nullptr;

    VerifyOrExit(aChildIndex < mMaxChildrenAllowed, OT_NOOP);
    child = &mChildren[aChildIndex];

exit:
    return child;
}

Child *ChildTable::GetNewChild(void)
{
    Child *child = FindChild(Child::AddressMatcher(Child::kInStateInvalid));

    VerifyOrExit(child != nullptr, OT_NOOP);
    child->Clear();

exit:
    return child;
}

const Child *ChildTable::FindChild(const Child::AddressMatcher &aMatcher) const
{
    const Child *child = mChildren;

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->Matches(aMatcher))
        {
            ExitNow();
        }
    }

    child = nullptr;

exit:
    return child;
}

Child *ChildTable::FindChild(uint16_t aRloc16, Child::StateFilter aFilter)
{
    return FindChild(Child::AddressMatcher(aRloc16, aFilter));
}

Child *ChildTable::FindChild(const Mac::ExtAddress &aExtAddress, Child::StateFilter aFilter)
{
    return FindChild(Child::AddressMatcher(aExtAddress, aFilter));
}

Child *ChildTable::FindChild(const Mac::Address &aMacAddress, Child::StateFilter aFilter)
{
    return FindChild(Child::AddressMatcher(aMacAddress, aFilter));
}

bool ChildTable::HasChildren(Child::StateFilter aFilter) const
{
    return (FindChild(Child::AddressMatcher(aFilter)) != nullptr);
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

otError ChildTable::GetChildInfoById(uint16_t aChildId, Child::Info &aChildInfo)
{
    otError  error = OT_ERROR_NONE;
    Child *  child;
    uint16_t rloc16;

    if ((aChildId & ~Mle::kMaxChildId) != 0)
    {
        aChildId = Mle::Mle::ChildIdFromRloc16(aChildId);
    }

    rloc16 = Get<Mac::Mac>().GetShortAddress() | aChildId;
    child  = FindChild(rloc16, Child::kInStateValidOrRestoring);
    VerifyOrExit(child != nullptr, error = OT_ERROR_NOT_FOUND);

    aChildInfo.SetFrom(*child);

exit:
    return error;
}

otError ChildTable::GetChildInfoByIndex(uint16_t aChildIndex, Child::Info &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    Child * child = nullptr;

    child = GetChildAtIndex(aChildIndex);
    VerifyOrExit((child != nullptr) && child->IsStateValidOrRestoring(), error = OT_ERROR_NOT_FOUND);

    aChildInfo.SetFrom(*child);

exit:
    return error;
}

void ChildTable::Restore(void)
{
    otError  error          = OT_ERROR_NONE;
    bool     foundDuplicate = false;
    uint16_t numChildren    = 0;

    for (const Settings::ChildInfo &childInfo : Get<Settings>().IterateChildInfo())
    {
        Child *child;

        child = FindChild(childInfo.GetExtAddress(), Child::kInStateAnyExceptInvalid);

        if (child == nullptr)
        {
            VerifyOrExit((child = GetNewChild()) != nullptr, error = OT_ERROR_NO_BUFS);
        }
        else
        {
            foundDuplicate = true;
        }

        child->Clear();

        child->SetExtAddress(childInfo.GetExtAddress());
        child->GetLinkInfo().Clear();
        child->SetRloc16(childInfo.GetRloc16());
        child->SetTimeout(childInfo.GetTimeout());
        child->SetDeviceMode(Mle::DeviceMode(childInfo.GetMode()));
        child->SetState(Neighbor::kStateRestored);
        child->SetLastHeard(TimerMilli::GetNow());
        child->SetVersion(static_cast<uint8_t>(childInfo.GetVersion()));
        Get<IndirectSender>().SetChildUseShortAddress(*child, true);
        numChildren++;
    }

exit:

    if (foundDuplicate || (numChildren > GetMaxChildren()) || (error != OT_ERROR_NONE))
    {
        // If there is any error, e.g., there are more saved children
        // in non-volatile settings than could be restored or there are
        // duplicate entries with same extended address, refresh the stored
        // children info to ensure that the non-volatile settings remain
        // consistent with the child table.

        RefreshStoredChildren();
    }
}

void ChildTable::RemoveStoredChild(const Child &aChild)
{
    for (Settings::ChildInfoIterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        if (iter.GetChildInfo().GetRloc16() == aChild.GetRloc16())
        {
            IgnoreError(iter.Delete());
            break;
        }
    }
}

otError ChildTable::StoreChild(const Child &aChild)
{
    Settings::ChildInfo childInfo;

    RemoveStoredChild(aChild);

    childInfo.Init();
    childInfo.SetExtAddress(aChild.GetExtAddress());
    childInfo.SetTimeout(aChild.GetTimeout());
    childInfo.SetRloc16(aChild.GetRloc16());
    childInfo.SetMode(aChild.GetDeviceMode().Get());
    childInfo.SetVersion(aChild.GetVersion());

    return Get<Settings>().AddChildInfo(childInfo);
}

void ChildTable::RefreshStoredChildren(void)
{
    const Child *child = &mChildren[0];

    SuccessOrExit(Get<Settings>().DeleteAllChildInfo());

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->IsStateInvalid())
        {
            continue;
        }

        SuccessOrExit(StoreChild(*child));
    }

exit:
    return;
}

bool ChildTable::HasSleepyChildWithAddress(const Ip6::Address &aIp6Address) const
{
    bool         hasChild = false;
    const Child *child    = &mChildren[0];

    for (uint16_t num = mMaxChildrenAllowed; num != 0; num--, child++)
    {
        if (child->IsStateValidOrRestoring() && !child->IsRxOnWhenIdle() && child->HasIp6Address(aIp6Address))
        {
            hasChild = true;
            break;
        }
    }

    return hasChild;
}

#endif // OPENTHREAD_FTD

} // namespace ot
