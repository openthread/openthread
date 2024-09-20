/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the child supervision feature.
 */

#include "child_supervision.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("ChildSupervsn");

#if OPENTHREAD_FTD

ChildSupervisor::ChildSupervisor(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Child *ChildSupervisor::GetDestination(const Message &aMessage) const
{
    Child   *child = nullptr;
    uint16_t childIndex;

    VerifyOrExit(aMessage.GetType() == Message::kTypeSupervision);

    IgnoreError(aMessage.Read(0, childIndex));
    child = Get<ChildTable>().GetChildAtIndex(childIndex);

exit:
    return child;
}

void ChildSupervisor::SendMessage(Child &aChild)
{
    OwnedPtr<Message> messagePtr;
    uint16_t          childIndex;

    VerifyOrExit(aChild.GetIndirectMessageCount() == 0);

    messagePtr.Reset(Get<MessagePool>().Allocate(Message::kTypeSupervision, sizeof(uint8_t)));
    VerifyOrExit(messagePtr != nullptr);

    // Supervision message is an empty payload 15.4 data frame.
    // The child index is stored here in the message content to allow
    // the destination of the message to be later retrieved using
    // `ChildSupervisor::GetDestination(message)`.

    childIndex = Get<ChildTable>().GetChildIndex(aChild);
    SuccessOrExit(messagePtr->Append(childIndex));

    Get<MeshForwarder>().SendMessage(messagePtr.PassOwnership());

    LogInfo("Sending supervision message to child 0x%04x", aChild.GetRloc16());

exit:
    return;
}

void ChildSupervisor::UpdateOnSend(Child &aChild) { aChild.ResetSecondsSinceLastSupervision(); }

void ChildSupervisor::HandleTimeTick(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsRxOnWhenIdle() || (child.GetSupervisionInterval() == 0))
        {
            continue;
        }

        child.IncrementSecondsSinceLastSupervision();

        if (child.GetSecondsSinceLastSupervision() >= child.GetSupervisionInterval())
        {
            SendMessage(child);
        }
    }
}

void ChildSupervisor::CheckState(void)
{
    // Child Supervision should run if Thread MLE operation is
    // enabled, and there is at least one "valid" child in the
    // child table.

    bool shouldRun = (!Get<Mle::Mle>().IsDisabled() && Get<ChildTable>().HasChildren(Child::kInStateValid));

    if (shouldRun && !Get<TimeTicker>().IsReceiverRegistered(TimeTicker::kChildSupervisor))
    {
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kChildSupervisor);
        LogInfo("Starting Child Supervision");
    }

    if (!shouldRun && Get<TimeTicker>().IsReceiverRegistered(TimeTicker::kChildSupervisor))
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kChildSupervisor);
        LogInfo("Stopping Child Supervision");
    }
}

void ChildSupervisor::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadChildAdded | kEventThreadChildRemoved))
    {
        CheckState();
    }
}

#endif // #if OPENTHREAD_FTD

SupervisionListener::SupervisionListener(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimeout(0)
    , mInterval(kDefaultInterval)
    , mTimer(aInstance)
{
    SetTimeout(kDefaultTimeout);
}

void SupervisionListener::Start(void) { RestartTimer(); }

void SupervisionListener::Stop(void) { mTimer.Stop(); }

void SupervisionListener::SetInterval(uint16_t aInterval)
{
    VerifyOrExit(mInterval != aInterval);

    LogInfo("Interval: %u -> %u", mInterval, aInterval);

    mInterval = aInterval;

    if (Get<Mle::Mle>().IsChild())
    {
        IgnoreError(Get<Mle::Mle>().SendChildUpdateRequest());
    }

exit:
    return;
}

void SupervisionListener::SetTimeout(uint16_t aTimeout)
{
    if (mTimeout != aTimeout)
    {
        LogInfo("Timeout: %u -> %u", mTimeout, aTimeout);

        mTimeout = aTimeout;
        RestartTimer();
    }
}

void SupervisionListener::UpdateOnReceive(const Mac::Address &aSourceAddress, bool aIsSecure)
{
    // If listener is enabled and device is a child and it received a secure frame from its parent, restart the timer.

    VerifyOrExit(mTimer.IsRunning() && aIsSecure && Get<Mle::MleRouter>().IsChild() &&
                 (Get<NeighborTable>().FindNeighbor(aSourceAddress) == &Get<Mle::MleRouter>().GetParent()));

    RestartTimer();

exit:
    return;
}

void SupervisionListener::RestartTimer(void)
{
    if ((mTimeout != 0) && !Get<Mle::MleRouter>().IsDisabled() && !Get<MeshForwarder>().GetRxOnWhenIdle())
    {
        mTimer.Start(Time::SecToMsec(mTimeout));
    }
    else
    {
        mTimer.Stop();
    }
}

void SupervisionListener::HandleTimer(void)
{
    VerifyOrExit(Get<Mle::MleRouter>().IsChild() && !Get<MeshForwarder>().GetRxOnWhenIdle());

    LogWarn("Supervision timeout. No frame from parent in %u sec", mTimeout);
    mCounter++;

    IgnoreError(Get<Mle::MleRouter>().SendChildUpdateRequest());

exit:
    RestartTimer();
}

} // namespace ot
