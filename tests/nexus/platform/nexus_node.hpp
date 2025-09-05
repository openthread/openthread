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

#ifndef OT_NEXUS_NODE_HPP_
#define OT_NEXUS_NODE_HPP_

#include "instance/instance.hpp"

#include "nexus_alarm.hpp"
#include "nexus_core.hpp"
#include "nexus_mdns.hpp"
#include "nexus_radio.hpp"
#include "nexus_settings.hpp"
#include "nexus_trel.hpp"
#include "nexus_utils.hpp"

namespace ot {
namespace Nexus {

class Platform
{
public:
    Radio    mRadio;
    Alarm    mAlarm;
    Mdns     mMdns;
    Settings mSettings;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Trel mTrel;
#endif
    bool mPendingTasklet;

protected:
    Platform(void)
        : mPendingTasklet(false)
    {
    }
};

class Node : public Platform, public Heap::Allocatable<Node>, public LinkedListEntry<Node>, private Instance
{
    friend class Heap::Allocatable<Node>;

public:
    enum JoinMode : uint8_t
    {
        kAsFtd,
        kAsFed,
        kAsMed,
        kAsSed,
    };

    void Reset(void);
    void Form(void);
    void Join(Node &aNode, JoinMode aJoinMode = kAsFtd);
    void AllowList(Node &aNode);
    void UnallowList(Node &aNode);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void GetTrelSockAddr(Ip6::SockAddr &aSockAddr) const;
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    template <typename Type> Type &Get(void)
    {
        Core::Get().SetActiveNode(this);
        return Instance::Get<Type>();
    }

    Instance &GetInstance(void)
    {
        Core::Get().SetActiveNode(this);
        return *this;
    }

    uint32_t GetId(void) { return GetInstance().GetId(); }

    static Node &From(otInstance *aInstance) { return static_cast<Node &>(*aInstance); }

    using Platform::mAlarm;
    using Platform::mMdns;
    using Platform::mPendingTasklet;
    using Platform::mRadio;
    using Platform::mSettings;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    using Platform::mTrel;
#endif

    Node *mNext;

private:
    Node(void) = default;
};

inline Node &AsNode(otInstance *aInstance) { return Node::From(aInstance); }

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_NODE_HPP_
