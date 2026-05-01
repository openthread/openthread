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

#ifndef OT_NEXUS_PLATFORM_NEXUS_NODE_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_NODE_HPP_

#include "instance/instance.hpp"

#include "nexus_alarm.hpp"
#include "nexus_core.hpp"
#include "nexus_dns.hpp"
#include "nexus_infra_if.hpp"
#include "nexus_logging.hpp"
#include "nexus_mdns.hpp"
#include "nexus_radio.hpp"
#include "nexus_settings.hpp"
#include "nexus_trel.hpp"
#include "nexus_udp.hpp"
#include "nexus_utils.hpp"

namespace ot {
namespace Nexus {

class Platform
{
public:
    Radio       mRadio;
    Alarm       mAlarmMilli;
    Alarm       mAlarmMicro;
    Logging     mLogging;
    Mdns        mMdns;
    UpstreamDns mUpstreamDns;
    InfraIf     mInfraIf;
    Udp         mUdp;
    Settings    mSettings;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Trel mTrel;
#endif
    bool mPendingTasklet;

protected:
    explicit Platform(Instance &aInstance)
        : mUpstreamDns(aInstance)
        , mInfraIf(aInstance)
        , mUdp(aInstance)
        , mPendingTasklet(false)
    {
    }
};

class Node : public Platform, public Heap::Allocatable<Node>, public LinkedListEntry<Node>, public Instance
{
    friend class Heap::Allocatable<Node>;

public:
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Helper methods - tests

    enum JoinMode : uint8_t
    {
        kAsFtd,
        kAsFed,
        kAsMed,
        kAsSed,                // Request stable netdata (default for SED)
        kAsSedWithFullNetData, // Request full netdata as SED
    };

    enum AddressNetif : uint8_t // Used in `const Ip6::Address &aAddress, AddressNetif aNetif)`
    {
        kThreadNetifAddress,
        kInfraNetifAddress,
        kAnyNetifAddress,
    };

    void Reset(void);
    void Form(void);
    void Join(Node &aNode, JoinMode aJoinMode = kAsFtd);
    void AllowList(Node &aNode);
    void UnallowList(Node &aNode);
    void SendEchoRequest(const Ip6::Address &aDestination,
                         uint16_t            aIdentifier  = 0,
                         uint16_t            aPayloadSize = 0,
                         uint8_t             aHopLimit    = 64,
                         const Ip6::Address *aSrcAddress  = nullptr);

    const Ip6::Address &FindMatchingAddress(const char *aPrefix);
    const Ip6::Address &FindGlobalAddress(void);

    bool Matches(const Ip6::Address &aAddress, AddressNetif aNetif) const;
    bool Matches(uint32_t aId) const { return GetInstance().GetId() == aId; }
    bool Matches(const Mac::ExtAddress &aExtAddress) const { return Get<Mac::Mac>().GetExtAddress() == aExtAddress; }

    const char *GetExtendedRoleString(void) const;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void GetTrelSockAddr(Ip6::SockAddr &aSockAddr) const;
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Node Properties

    uint32_t    GetId(void) const { return GetInstance().GetId(); }
    void        SetName(const char *aName) { mName.Clear().Append("%s", aName); }
    void        SetName(const char *aPrefix, uint16_t aIndex);
    const char *GetName(void) const { return mName.AsCString(); }
    void        SetPosition(float aX, float aY) { mX = aX, mY = aY; }
    float       GetPositionX(void) const { return mX; }
    float       GetPositionY(void) const { return mY; }
    uint32_t    GetLastParentId(void) const { return mLastParentId; }
    void        SetLastParentId(uint32_t aId) { mLastParentId = aId; }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Helper methods - Access OpenThread Core components

    template <typename Type> Type       &Get(void) { return Instance::Get<Type>(); }
    template <typename Type> const Type &Get(void) const { return AsConst(AsNonConst(this)->Get<Type>()); }

    Instance       &GetInstance(void) { return *this; }
    const Instance &GetInstance(void) const { return *this; }

    static Node &From(otInstance *aInstance)
    {
        Instance *instance = static_cast<Instance *>(aInstance);
        return *static_cast<Node *>(instance);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Helper methods - Callbacks

    static void HandleIp6Receive(otMessage *aMessage, void *aContext);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Platform components

    using Platform::mAlarmMicro;
    using Platform::mAlarmMilli;
    using Platform::mInfraIf;
    using Platform::mLogging;
    using Platform::mMdns;
    using Platform::mPendingTasklet;
    using Platform::mRadio;
    using Platform::mSettings;
    using Platform::mUdp;
    using Platform::mUpstreamDns;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    using Platform::mTrel;
#endif

    Node *mNext;

private:
    Node(void);
    void HandleIp6Receive(OwnedPtr<Message> aMessagePtr);

    String<32> mName;
    float      mX;
    float      mY;
    uint32_t   mLastParentId;
};

inline Node &AsNode(otInstance *aInstance) { return Node::From(aInstance); }

void AllowLinkBetween(Node &aFirstNode, Node &aSecondNode);
void UnallowLinkBetween(Node &aFirstNode, Node &aSecondNode);

} // namespace Nexus

template <> inline Nexus::Node        &Instance::Get(void) { return Nexus::AsNode(this); }
template <> inline Nexus::InfraIf     &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mInfraIf; }
template <> inline Nexus::Udp         &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mUdp; }
template <> inline Nexus::Trel        &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mTrel; }
template <> inline Nexus::Mdns        &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mMdns; }
template <> inline Nexus::UpstreamDns &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mUpstreamDns; }

} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_NODE_HPP_
