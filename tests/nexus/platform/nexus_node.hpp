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
    Radio    mRadio;
    Alarm    mAlarmMilli;
    Alarm    mAlarmMicro;
    Logging  mLogging;
    Mdns     mMdns;
    InfraIf  mInfraIf;
    Udp      mUdp;
    Settings mSettings;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Trel mTrel;
#endif
    bool mPendingTasklet;

protected:
    explicit Platform(Instance &aInstance)
        : mInfraIf(aInstance)
        , mUdp(aInstance)
        , mPendingTasklet(false)
    {
    }
};

class Node : public Platform, public Heap::Allocatable<Node>, public LinkedListEntry<Node>, public Instance
{
    friend class Heap::Allocatable<Node>;

public:
    // Defines the device role and Network Data request behavior
    // during joining:
    //
    // - `kAsFtd`, `kAsFed`, and `kAsMed` all request full netdata.
    // - `kAsSed` requests only stable netdata (default for SED).
    // - `kAsSedWithFullNetData` explicitly request full netdata.
    enum JoinMode : uint8_t
    {
        kAsFtd,
        kAsFed,
        kAsMed,
        kAsSed,
        kAsSedWithFullNetData,
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

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void GetTrelSockAddr(Ip6::SockAddr &aSockAddr) const;
#endif

    // Finds and returns the address on device matching the given `aPrefix`.
    // It requires a matching prefix to be found, otherwise it is treated as
    // a test failure (emits error message and exits the program.)
    const Ip6::Address &FindMatchingAddress(const char *aPrefix);

    /**
     * Finds and returns a global scope address on the device.
     *
     * It requires a global scope address to be found, otherwise it is treated as a test failure (emits error message
     * and exits the program).
     *
     * @returns A reference to the global scope address.
     */
    const Ip6::Address &FindGlobalAddress(void);

    void        SetName(const char *aName) { mName.Clear().Append("%s", aName); }
    void        SetName(const char *aPrefix, uint16_t aIndex);
    const char *GetName(void) const { return mName.AsCString(); }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    template <typename Type> Type &Get(void) { return Instance::Get<Type>(); }

    Instance &GetInstance(void) { return *this; }

    uint32_t GetId(void) { return GetInstance().GetId(); }

    static Node &From(otInstance *aInstance) { return static_cast<Node &>(*aInstance); }

    static void HandleIp6Receive(otMessage *aMessage, void *aContext);

    using Platform::mAlarmMicro;
    using Platform::mAlarmMilli;
    using Platform::mInfraIf;
    using Platform::mLogging;
    using Platform::mMdns;
    using Platform::mPendingTasklet;
    using Platform::mRadio;
    using Platform::mSettings;
    using Platform::mUdp;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    using Platform::mTrel;
#endif

    Node *mNext;

    Ip6::Address mSrpHostAddresses[OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_MAX_HOST_ADDRESSES];

private:
    Node(void)
        : Platform(static_cast<Instance &>(*this))
    {
    }

    void HandleIp6Receive(OwnedPtr<Message> aMessagePtr);

    String<32> mName;
};

inline Node &AsNode(otInstance *aInstance) { return Node::From(aInstance); }

} // namespace Nexus

template <> inline Nexus::InfraIf &Instance::Get(void) { return static_cast<Nexus::Node *>(this)->mInfraIf; }

} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_NODE_HPP_
