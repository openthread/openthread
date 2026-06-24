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

#if OPENTHREAD_FTD

#include "instance/instance.hpp"

namespace ot {

//---------------------------------------------------------------------------------------------------------------------
// `ChildTable::Iterator`

ChildTable::Iterator::Iterator(Instance &aInstance, Child::StateFilter aFilter)
    : InstanceLocator(aInstance)
    , ItemPtrIterator(nullptr)
    , mFilter(aFilter)
{
    Reset();
}

void ChildTable::Iterator::Reset(void)
{
    mItem = &Get<ChildTable>().mChildren[0];

    if (!mItem->Matches(mFilter))
    {
        Advance();
    }
}

void ChildTable::Iterator::Advance(void)
{
    VerifyOrExit(mItem != nullptr);

    do
    {
        mItem++;
        VerifyOrExit(mItem < Get<ChildTable>().mChildren.end(), mItem = nullptr);
    } while (!mItem->Matches(mFilter));

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// `ChildTable`

ChildTable::ChildTable(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mMaxChildIpAddresses(0)
#endif
    , mNextChildId(Mle::kMaxChildId)
{
    mChildren.SetLength(kMaxChildren);

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

Child *ChildTable::GetChildAtIndex(uint16_t aChildIndex) { return mChildren.At(aChildIndex); }

Child *ChildTable::GetNewChild(void)
{
    Child *child = FindChild(Child::AddressMatcher(Child::kInStateInvalid));

    VerifyOrExit(child != nullptr);
    child->Clear();

exit:
    return child;
}

uint16_t ChildTable::AllocateNewChildRloc16(void)
{
    uint16_t rloc16;

    do
    {
        mNextChildId++;

        if (mNextChildId > Mle::kMaxChildId)
        {
            mNextChildId = Mle::kMinChildId;
        }

        rloc16 = Get<Mle::Mle>().GetRloc16() | mNextChildId;

    } while (FindChild(rloc16, Child::kInStateAnyExceptInvalid) != nullptr);

    return rloc16;
}

const Child *ChildTable::FindChild(const Child::AddressMatcher &aMatcher) const
{
    return mChildren.FindMatching(aMatcher);
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
    return mChildren.ContainsMatching(Child::AddressMatcher(aFilter));
}

uint16_t ChildTable::GetNumChildren(Child::StateFilter aFilter) const { return mChildren.CountMatching(aFilter); }

Error ChildTable::SetMaxChildrenAllowed(uint16_t aMaxChildren)
{
    Error error = kErrorNone;

    VerifyOrExit(aMaxChildren > 0 && aMaxChildren <= kMaxChildren, error = kErrorInvalidArgs);
    VerifyOrExit(!HasChildren(Child::kInStateAnyExceptInvalid), error = kErrorInvalidState);

    mChildren.SetLength(aMaxChildren);

exit:
    return error;
}

Error ChildTable::GetChildInfoById(uint16_t aChildId, Child::Info &aChildInfo)
{
    Error    error = kErrorNone;
    Child   *child;
    uint16_t rloc16;

    if ((aChildId & ~Mle::kMaxChildId) != 0)
    {
        aChildId = Mle::ChildIdFromRloc16(aChildId);
    }

    rloc16 = Get<Mle::Mle>().GetRloc16() | aChildId;
    child  = FindChild(rloc16, Child::kInStateValidOrRestoring);
    VerifyOrExit(child != nullptr, error = kErrorNotFound);

    aChildInfo.SetFrom(*child);

exit:
    return error;
}

Error ChildTable::GetChildInfoByIndex(uint16_t aChildIndex, Child::Info &aChildInfo)
{
    Error  error = kErrorNone;
    Child *child = nullptr;

    child = GetChildAtIndex(aChildIndex);
    VerifyOrExit((child != nullptr) && child->IsStateValidOrRestoring(), error = kErrorNotFound);

    aChildInfo.SetFrom(*child);

exit:
    return error;
}

void ChildTable::Restore(void)
{
    Error    error          = kErrorNone;
    bool     foundDuplicate = false;
    uint16_t numChildren    = 0;

    for (const Settings::ChildInfo &childInfo : Get<Settings>().IterateChildInfo())
    {
        Child *child;

        child = FindChild(childInfo.GetExtAddress(), Child::kInStateAnyExceptInvalid);

        if (child == nullptr)
        {
            VerifyOrExit((child = GetNewChild()) != nullptr, error = kErrorNoBufs);
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
        child->GenerateChallenge();
        child->SetLastHeard(TimerMilli::GetNow());
        child->SetVersion(childInfo.GetVersion());
        Get<IndirectSender>().SetChildUseShortAddress(*child, true);
        Get<NeighborTable>().Signal(NeighborTable::kChildAdded, *child);
        numChildren++;
    }

exit:

    if (foundDuplicate || (numChildren > GetMaxChildren()) || (error != kErrorNone))
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

Error ChildTable::StoreChild(const Child &aChild)
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
    Get<Settings>().DeleteAllChildInfo();

    for (const Child &child : mChildren)
    {
        if (child.IsStateInvalid())
        {
            continue;
        }

        SuccessOrExit(StoreChild(child));
    }

exit:
    return;
}

bool ChildTable::HasMinimalChild(uint16_t aRloc16) const
{
    bool         hasMinimalChild = false;
    const Child *child;

    VerifyOrExit(Get<Mle::Mle>().HasMatchingRouterIdWith(aRloc16));

    child = FindChild(Child::AddressMatcher(aRloc16, Child::kInStateValidOrRestoring));
    VerifyOrExit(child != nullptr);

    hasMinimalChild = !child->IsFullThreadDevice();

exit:
    return hasMinimalChild;
}

bool ChildTable::HasSleepyChildWithAddress(const Ip6::Address &aIp6Address) const
{
    bool hasChild = false;

    for (const Child &child : mChildren)
    {
        if (child.IsStateValidOrRestoring() && !child.IsRxOnWhenIdle() && child.HasIp6Address(aIp6Address))
        {
            hasChild = true;
            break;
        }
    }

    return hasChild;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

Error ChildTable::OverrideMaxChildIpAddresses(uint8_t aMaxIpAddresses)
{
    Error error = kErrorNone;

    VerifyOrExit(aMaxIpAddresses <= kMaxChildIpAddresses, error = kErrorInvalidArgs);
    mMaxChildIpAddresses = aMaxIpAddresses;

exit:
    return error;
}

#endif

} // namespace ot

#endif // OPENTHREAD_FTD
