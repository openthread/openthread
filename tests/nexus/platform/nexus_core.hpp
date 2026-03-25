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

#ifndef OT_NEXUS_PLATFORM_NEXUS_CORE_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_CORE_HPP_

#include "common/array.hpp"
#include "common/owning_list.hpp"
#include "instance/instance.hpp"
#include "thread/key_manager.hpp"

#include "nexus_alarm.hpp"
#include "nexus_pcap.hpp"
#include "nexus_radio.hpp"
#include "nexus_utils.hpp"

namespace ot {
namespace Nexus {

class Node;

class Core
{
public:
    Core(void);
    ~Core(void);

    static Core &Get(void) { return *sCore; }

    Node             &CreateNode(void);
    LinkedList<Node> &GetNodes(void) { return mNodes; }

    TimeMilli GetNow(void) { return TimeMilli(static_cast<uint32_t>(mNow / 1000u)); }
    TimeMicro GetNowMicro(void) { return TimeMicro(static_cast<uint32_t>(mNow)); }
    uint64_t  GetNowMicro64(void) const { return mNow; }
    void      AdvanceTime(uint32_t aDuration);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Test specific helper methods

    void SaveTestInfo(const char *aFilename, Node *aLeaderNode = nullptr);
    void AddNetworkKey(const NetworkKey &aKey);
    void AddTestVar(const char *aName, const char *aValue);
    void AddOmrPrefixTestVar(const char *aName, Node &aNode);
    void SendAndVerifyEchoRequest(Node               &aSender,
                                  const Ip6::Address &aDestination,
                                  uint16_t            aPayloadSize     = 0,
                                  uint8_t             aHopLimit        = Ip6::kDefaultHopLimit,
                                  uint32_t            aResponseTimeout = 1000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Used by platform implementation

    void UpdateNextAlarmMilli(const Alarm &aAlarm);
    void UpdateNextAlarmMicro(const Alarm &aAlarm);
    void MarkPendingAction(void) { mPendingAction = true; }

private:
    static constexpr int8_t  kDefaultRxRssi = -20;
    static constexpr uint8_t kDefaultRxLqi  = 255;

    enum AckMode : uint8_t
    {
        kNoAck,
        kSendAckNoFramePending,
        kSendAckFramePending,
    };

    struct IcmpEchoResponseContext
    {
        IcmpEchoResponseContext(Node &aNode, uint16_t aIdentifier);

        Node    &mNode;
        uint16_t mIdentifier;
        bool     mResponseReceived;
    };

    struct TestVar
    {
        String<32> mName;
        String<64> mValue;
    };

    void Process(Node &aNode);
    void ProcessRadio(Node &aNode);
    void ProcessInfraIf(Node &aNode);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void ProcessTrel(Node &aNode);
#endif

    Node *FindNodeByInfraIfAddress(const Ip6::Address &aAddress);

    static void HandleIcmpResponse(void                *aContext,
                                   otMessage           *aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   const otIcmp6Header *aIcmpHeader);

    static Core *sCore;
    static bool  sInUse;

    OwningList<Node>      mNodes;
    Pcap                  mPcap;
    Array<NetworkKey, 16> mNetworkKeys;
    Array<TestVar, 128>   mTestVars;
    uint16_t              mCurNodeId;
    bool                  mPendingAction;
    bool                  mSaveNodeLogs;
    uint64_t              mNow;
    uint64_t              mNextAlarmTime;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_CORE_HPP_
