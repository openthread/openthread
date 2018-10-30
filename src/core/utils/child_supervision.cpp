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

#include "openthread-core-config.h"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace Utils {

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION

#if OPENTHREAD_FTD

ChildSupervisor::ChildSupervisor(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSupervisionInterval(kDefaultSupervisionInterval)
    , mTimer(aInstance, &ChildSupervisor::HandleTimer, this)
    , mNotifierCallback(&ChildSupervisor::HandleStateChanged, this)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

void ChildSupervisor::SetSupervisionInterval(uint16_t aInterval)
{
    mSupervisionInterval = aInterval;
    CheckState();
}

Child *ChildSupervisor::GetDestination(const Message &aMessage) const
{
    Child * child = NULL;
    uint8_t childIndex;

    VerifyOrExit(aMessage.GetType() == Message::kTypeSupervision);

    aMessage.Read(0, sizeof(childIndex), &childIndex);
    child = GetNetif().GetMle().GetChildTable().GetChildAtIndex(childIndex);

exit:
    return child;
}

void ChildSupervisor::SendMessage(Child &aChild)
{
    ThreadNetif &netif   = GetNetif();
    Message *    message = NULL;
    uint8_t      childIndex;

    VerifyOrExit(aChild.GetIndirectMessageCount() == 0);

    message = netif.GetInstance().GetMessagePool().New(Message::kTypeSupervision, sizeof(uint8_t));
    VerifyOrExit(message != NULL);

    // Supervision message is an empty payload 15.4 data frame.
    // The child index is stored here in the message content to allow
    // the destination of the message to be later retrieved using
    // `ChildSupervisor::GetDestination(message)`.

    childIndex = netif.GetMle().GetChildTable().GetChildIndex(aChild);
    SuccessOrExit(message->Append(&childIndex, sizeof(childIndex)));

    SuccessOrExit(netif.SendMessage(*message));
    message = NULL;

    otLogInfoUtil("Sending supervision message to child 0x%04x", aChild.GetRloc16());

exit:

    if (message != NULL)
    {
        message->Free();
    }
}

void ChildSupervisor::UpdateOnSend(Child &aChild)
{
    aChild.ResetSecondsSinceLastSupervision();
}

void ChildSupervisor::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ChildSupervisor>().HandleTimer();
}

void ChildSupervisor::HandleTimer(void)
{
    VerifyOrExit(mSupervisionInterval != 0);

    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValid); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        child.IncrementSecondsSinceLastSupervision();

        if ((child.GetSecondsSinceLastSupervision() >= mSupervisionInterval) && (child.IsRxOnWhenIdle() == false))
        {
            SendMessage(child);
        }
    }

    mTimer.Start(kOneSecond);

exit:
    return;
}

void ChildSupervisor::CheckState(void)
{
    bool            shouldRun = false;
    Mle::MleRouter &mle       = GetInstance().Get<Mle::MleRouter>();

    // Child Supervision should run if `mSupervisionInterval` is not
    // zero, Thread MLE operation is enabled, and there is at least one
    // "valid" child in the child table.

    shouldRun = ((mSupervisionInterval != 0) && (mle.GetRole() != OT_DEVICE_ROLE_DISABLED) &&
                 mle.GetChildTable().HasChildren(ChildTable::kInStateValid));

    if (shouldRun && !mTimer.IsRunning())
    {
        mTimer.Start(kOneSecond);
        otLogInfoUtil("Starting Child Supervision");
    }

    if (!shouldRun && mTimer.IsRunning())
    {
        mTimer.Stop();
        otLogInfoUtil("Stopping Child Supervision");
    }
}

void ChildSupervisor::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<ChildSupervisor>().HandleStateChanged(aFlags);
}

void ChildSupervisor::HandleStateChanged(otChangedFlags aFlags)
{
    if ((aFlags & (OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_CHILD_ADDED | OT_CHANGED_THREAD_CHILD_REMOVED)) != 0)
    {
        CheckState();
    }
}

#endif // #if OPENTHREAD_FTD

SupervisionListener::SupervisionListener(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimeout(0)
    , mTimer(aInstance, &SupervisionListener::HandleTimer, this)
{
    SetTimeout(kDefaultTimeout);
}

void SupervisionListener::Start(void)
{
    RestartTimer();
}

void SupervisionListener::Stop(void)
{
    mTimer.Stop();
}

void SupervisionListener::SetTimeout(uint16_t aTimeout)
{
    if (mTimeout != aTimeout)
    {
        mTimeout = aTimeout;
        RestartTimer();
    }
}

void SupervisionListener::UpdateOnReceive(const Mac::Address &aSourceAddress, bool aIsSecure)
{
    ThreadNetif &netif = GetNetif();

    // If listener is enabled and device is a child and it received a secure frame from its parent, restart the timer.

    VerifyOrExit(mTimer.IsRunning() && aIsSecure && (netif.GetMle().GetRole() == OT_DEVICE_ROLE_CHILD) &&
                 (netif.GetMle().GetNeighbor(aSourceAddress) == netif.GetMle().GetParent()));

    RestartTimer();

exit:
    return;
}

void SupervisionListener::RestartTimer(void)
{
    ThreadNetif &netif = GetNetif();

    if ((mTimeout != 0) && (netif.GetMle().GetRole() != OT_DEVICE_ROLE_DISABLED) &&
        (netif.GetMeshForwarder().GetRxOnWhenIdle() == false))
    {
        mTimer.Start(TimerMilli::SecToMsec(mTimeout));
    }
    else
    {
        mTimer.Stop();
    }
}

void SupervisionListener::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<SupervisionListener>().HandleTimer();
}

void SupervisionListener::HandleTimer(void)
{
    ThreadNetif &netif = GetNetif();

    VerifyOrExit((netif.GetMle().GetRole() == OT_DEVICE_ROLE_CHILD) &&
                 (netif.GetMeshForwarder().GetRxOnWhenIdle() == false));

    otLogWarnUtil("Supervision timeout. No frame from parent in %d sec", mTimeout);

    netif.GetMle().SendChildUpdateRequest();

exit:
    RestartTimer();
}

#endif // #if OPENTHREAD_ENABLE_CHILD_SUPERVISION

} // namespace Utils
} // namespace ot
