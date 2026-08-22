/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/num_utils.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/router_table.hpp"

namespace ot {

void TestDeviceMode(void)
{
    Mle::DeviceMode             mode;
    Mle::DeviceMode::ModeConfig config;
    Mle::DeviceMode::ModeConfig readConfig;

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // SED (stable subset netdata)
    config.mRxOnWhenIdle = false;
    config.mDeviceType   = false;
    config.mNetworkData  = false;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(!readConfig.mRxOnWhenIdle);
    VerifyOrQuit(!readConfig.mDeviceType);
    VerifyOrQuit(!readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(!mode.IsRxOnWhenIdle());
    VerifyOrQuit(!mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kStableSubset);
    VerifyOrQuit(!mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // SED (full set netdata)

    config.mRxOnWhenIdle = false;
    config.mDeviceType   = false;
    config.mNetworkData  = true;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(!readConfig.mRxOnWhenIdle);
    VerifyOrQuit(!readConfig.mDeviceType);
    VerifyOrQuit(readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(!mode.IsRxOnWhenIdle());
    VerifyOrQuit(!mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kFullSet);
    VerifyOrQuit(!mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // MED (stable subset netdata)

    config.mRxOnWhenIdle = true;
    config.mDeviceType   = false;
    config.mNetworkData  = false;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(readConfig.mRxOnWhenIdle);
    VerifyOrQuit(!readConfig.mDeviceType);
    VerifyOrQuit(!readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(mode.IsRxOnWhenIdle());
    VerifyOrQuit(!mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kStableSubset);
    VerifyOrQuit(mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // MED (full set netdata)

    config.mRxOnWhenIdle = true;
    config.mDeviceType   = false;
    config.mNetworkData  = true;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(readConfig.mRxOnWhenIdle);
    VerifyOrQuit(!readConfig.mDeviceType);
    VerifyOrQuit(readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(mode.IsRxOnWhenIdle());
    VerifyOrQuit(!mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kFullSet);
    VerifyOrQuit(mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // FTD (stable subset netdata)

    config.mRxOnWhenIdle = true;
    config.mDeviceType   = true;
    config.mNetworkData  = false;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(readConfig.mRxOnWhenIdle);
    VerifyOrQuit(readConfig.mDeviceType);
    VerifyOrQuit(!readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(mode.IsRxOnWhenIdle());
    VerifyOrQuit(mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kStableSubset);
    VerifyOrQuit(!mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // FTD (full set netdata)

    config.mRxOnWhenIdle = true;
    config.mDeviceType   = true;
    config.mNetworkData  = true;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(readConfig.mRxOnWhenIdle);
    VerifyOrQuit(readConfig.mDeviceType);
    VerifyOrQuit(readConfig.mNetworkData);

    VerifyOrQuit(mode.IsValid());
    VerifyOrQuit(mode.IsRxOnWhenIdle());
    VerifyOrQuit(mode.IsFullThreadDevice());
    VerifyOrQuit(mode.GetNetworkDataType() == NetworkData::kFullSet);
    VerifyOrQuit(!mode.IsMinimalEndDevice());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // Invalid

    config.mRxOnWhenIdle = false;
    config.mDeviceType   = true;
    config.mNetworkData  = true;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(!readConfig.mRxOnWhenIdle);
    VerifyOrQuit(readConfig.mDeviceType);
    VerifyOrQuit(readConfig.mNetworkData);

    VerifyOrQuit(!mode.IsValid());

    //- - - - - - - - - - - - - - - - - - - - - - - -
    // Invalid

    config.mRxOnWhenIdle = false;
    config.mDeviceType   = true;
    config.mNetworkData  = false;
    mode.Set(config);

    mode.Get(readConfig);
    VerifyOrQuit(!readConfig.mRxOnWhenIdle);
    VerifyOrQuit(readConfig.mDeviceType);
    VerifyOrQuit(!readConfig.mNetworkData);

    VerifyOrQuit(!mode.IsValid());

    printf("TestDeviceMode passed\n");
}

namespace {

constexpr uint16_t kOldParentRloc16 = 0x5400;
constexpr uint16_t kOldChildRloc16  = 0x5401;
constexpr uint16_t kNewParentRloc16 = 0x5c00;
constexpr uint16_t kNewChildRloc16  = 0x5c01;

constexpr uint8_t kOldDataVersion   = 4;
constexpr uint8_t kOldStableVersion = 3;
constexpr uint8_t kNewDataVersion   = 8;
constexpr uint8_t kNewStableVersion = 7;
constexpr uint8_t kMalformedDataLen = 31;

const uint8_t kOldNetworkData[] = {
    0x03, 0x0e, 0x00, 0x40, 0x20, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x07, 0x02, 0x11, 0x40,
    0x03, 0x0e, 0x00, 0x40, 0x20, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x07, 0x02, 0x02, 0x40,
};

const uint8_t kNewNetworkData[] = {
    0x03, 0x0e, 0x00, 0x40, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x03, 0x07, 0x02, 0x13, 0x40,
    0x03, 0x0e, 0x00, 0x40, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x04, 0x07, 0x02, 0x04, 0x40,
};

Ip6::Prefix PrefixFromString(const char *aString, uint8_t aLength)
{
    Ip6::Prefix prefix;

    SuccessOrQuit(AsCoreType(&prefix.mPrefix).FromString(aString));
    prefix.mLength = aLength;

    return prefix;
}

Mac::ExtAddress ExtAddressFromSeed(uint8_t aSeed)
{
    Mac::ExtAddress extAddress;

    for (size_t index = 0; index < sizeof(extAddress.m8); index++)
    {
        extAddress.m8[index] = static_cast<uint8_t>(aSeed + index);
    }

    return extAddress;
}

void VerifyContext(Instance &aInstance, uint8_t aContextId, const Ip6::Prefix &aExpectedPrefix, bool aShouldBeValid)
{
    Lowpan::Context context;

    aInstance.Get<NetworkData::Leader>().FindContextForId(aContextId, context);
    VerifyOrQuit(context.IsValid() == aShouldBeValid);

    if (aShouldBeValid)
    {
        VerifyOrQuit(context.GetContextId() == aContextId);
        VerifyOrQuit(context.GetPrefix() == aExpectedPrefix);
    }
}

} // namespace

class UnitTester
{
public:
    static void TestChildIdResponseNetworkDataHandling(void)
    {
        TestValidNetworkDataControl();
        TestMissingNetworkDataControl();
        TestMalformedNetworkDataControl();

        printf("TestChildIdResponseNetworkDataHandling passed\n");
    }

#if OPENTHREAD_FTD
    class TxChallenge : public Mle::TxChallenge
    {
    public:
        Mle::RxChallenge AsRx(void) const
        {
            Mle::RxChallenge rxChallenge;

            rxChallenge.InitFrom(*this);
            return rxChallenge;
        }
    };

    static void TestTxChallengeTable(void)
    {
        Instance                   *instance = static_cast<Instance *>(testInitInstance());
        Mle::Mle::TxChallengeTable *table;

        printf("TestTxChallengeTable\n");

        VerifyOrQuit(instance != nullptr);

        table = &instance->Get<Mle::Mle>().mTxChallengeTable;

        // Generate one challenge for router ID 1 and check matching & aging
        {
            static constexpr uint8_t kRouterId      = 1;
            static constexpr uint8_t kWrongRouterId = 2;

            TxChallenge challenge;
            TxChallenge badChallenge;

            table->Clear();

            SuccessOrQuit(table->GenerateFor(kRouterId, challenge));

            badChallenge.GenerateRandom();

            // Positive match
            VerifyOrQuit(table->ContainsMatching(challenge.AsRx(), kRouterId));

            // Negative matches
            VerifyOrQuit(!table->ContainsMatching(challenge.AsRx(), kWrongRouterId)); // Wrong router ID
            VerifyOrQuit(!table->ContainsMatching(badChallenge.AsRx(), kRouterId));   // Wrong challenge
            VerifyOrQuit(!table->ContainsMatching(badChallenge.AsRx(), kWrongRouterId));

            // Aging

            for (uint8_t i = 0; i < Mle::Mle::TxChallengeTable::kTimeout - 1; i++)
            {
                table->HandleTimeTick();
                VerifyOrQuit(table->ContainsMatching(challenge.AsRx(), kRouterId));
            }

            table->HandleTimeTick();
            VerifyOrQuit(!table->ContainsMatching(challenge.AsRx(), kRouterId));
        }

        // Multicast challenge
        {
            static constexpr uint8_t kRouterId = 5;

            TxChallenge challenge;
            TxChallenge multiChallenge;

            table->Clear();

            SuccessOrQuit(table->GenerateFor(kRouterId, challenge));

            SuccessOrQuit(table->GenerateForMulticast(multiChallenge));

            // Multicast challenge matches any router ID

            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                VerifyOrQuit(table->ContainsMatching(multiChallenge.AsRx(), routerId));

                if (routerId != kRouterId)
                {
                    VerifyOrQuit(!table->ContainsMatching(challenge.AsRx(), routerId));
                }
                else
                {
                    VerifyOrQuit(table->ContainsMatching(challenge.AsRx(), routerId));
                }
            }
        }

        // Overwriting & Timeout Reset
        {
            static constexpr uint8_t kRouterIdR1 = 3;
            static constexpr uint8_t kRouterIdR2 = 12;

            TxChallenge challengeR1;
            TxChallenge challengeR2;
            TxChallenge challengeMulti;
            TxChallenge newChallengeR2;
            TxChallenge newChallengeMulti;

            table->Clear();

            SuccessOrQuit(table->GenerateFor(kRouterIdR1, challengeR1));
            SuccessOrQuit(table->GenerateFor(kRouterIdR2, challengeR2));
            SuccessOrQuit(table->GenerateForMulticast(challengeMulti));

            // Tick 2 times
            table->HandleTimeTick();
            table->HandleTimeTick();

            VerifyOrQuit(table->ContainsMatching(challengeR1.AsRx(), kRouterIdR1));
            VerifyOrQuit(table->ContainsMatching(challengeR2.AsRx(), kRouterIdR2));

            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                VerifyOrQuit(table->ContainsMatching(challengeMulti.AsRx(), routerId));
            }

            // Regenerate challenge for Router ID 2 (overwrites old entry and resets timeout)

            SuccessOrQuit(table->GenerateFor(kRouterIdR2, newChallengeR2));

            // Check that old challenge for R2 no longer matches, but new challenge for R2 matches
            VerifyOrQuit(!table->ContainsMatching(challengeR2.AsRx(), kRouterIdR2));
            VerifyOrQuit(table->ContainsMatching(newChallengeR2.AsRx(), kRouterIdR2));

            // Tick 1 time
            table->HandleTimeTick();

            VerifyOrQuit(table->ContainsMatching(challengeR1.AsRx(), kRouterIdR1));
            VerifyOrQuit(table->ContainsMatching(newChallengeR2.AsRx(), kRouterIdR2));

            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                VerifyOrQuit(table->ContainsMatching(challengeMulti.AsRx(), routerId));
            }

            // Regenerate multicast challenge
            SuccessOrQuit(table->GenerateForMulticast(newChallengeMulti));

            VerifyOrQuit(table->ContainsMatching(challengeR1.AsRx(), kRouterIdR1));
            VerifyOrQuit(table->ContainsMatching(newChallengeR2.AsRx(), kRouterIdR2));

            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                VerifyOrQuit(!table->ContainsMatching(challengeMulti.AsRx(), routerId));
                VerifyOrQuit(table->ContainsMatching(newChallengeMulti.AsRx(), routerId));
            }

            table->HandleTimeTick();

            // R1 entry should have aged
            VerifyOrQuit(!table->ContainsMatching(challengeR1.AsRx(), kRouterIdR1));

            VerifyOrQuit(table->ContainsMatching(newChallengeR2.AsRx(), kRouterIdR2));
            VerifyOrQuit(table->ContainsMatching(newChallengeMulti.AsRx(), 0));

            // Wait two ticks - Now new R2 entry must have aged
            table->HandleTimeTick();
            table->HandleTimeTick();
            VerifyOrQuit(!table->ContainsMatching(newChallengeR2.AsRx(), kRouterIdR2));
            VerifyOrQuit(table->ContainsMatching(newChallengeMulti.AsRx(), 1));

            table->HandleTimeTick();
            VerifyOrQuit(!table->ContainsMatching(newChallengeMulti.AsRx(), 1));
        }

        // Fill Capacity & Clear
        {
            static constexpr uint8_t kChosenRouterId = 10;

            TxChallenge challenges[Mle::kMaxRouters];
            TxChallenge multicastChallenge;
            TxChallenge updatedChallenge;

            table->Clear();

            // Fill all router IDs (0 to kMaxRouters-1) and 1 multicast entry
            for (uint8_t routerId = 0; routerId < Mle::kMaxRouters; routerId++)
            {
                SuccessOrQuit(table->GenerateFor(routerId, challenges[routerId]));
                VerifyOrQuit(table->ContainsMatching(challenges[routerId].AsRx(), routerId));
            }

            SuccessOrQuit(table->GenerateForMulticast(multicastChallenge));

            table->HandleTimeTick();

            // Verify all entries match correctly
            for (uint8_t routerId = 0; routerId < Mle::kMaxRouters; routerId++)
            {
                VerifyOrQuit(table->ContainsMatching(challenges[routerId].AsRx(), routerId));
                VerifyOrQuit(table->ContainsMatching(multicastChallenge.AsRx(), routerId));
            }

            // Overwrite an existing router ID when full (should succeed)
            SuccessOrQuit(table->GenerateFor(kChosenRouterId, updatedChallenge));

            VerifyOrQuit(table->ContainsMatching(updatedChallenge.AsRx(), kChosenRouterId));
            VerifyOrQuit(!table->ContainsMatching(challenges[kChosenRouterId].AsRx(), kChosenRouterId));

            for (uint8_t i = 0; i < Mle::Mle::TxChallengeTable::kTimeout - 1; i++)
            {
                table->HandleTimeTick();
            }

            for (uint8_t routerId = 0; routerId < Mle::kMaxRouters; routerId++)
            {
                if (routerId == kChosenRouterId)
                {
                    VerifyOrQuit(table->ContainsMatching(updatedChallenge.AsRx(), routerId));
                }
                else
                {
                    VerifyOrQuit(!table->ContainsMatching(challenges[routerId].AsRx(), routerId));
                }

                VerifyOrQuit(!table->ContainsMatching(multicastChallenge.AsRx(), routerId));
            }

            // Fill table again
            for (uint8_t routerId = 0; routerId < Mle::kMaxRouters; routerId++)
            {
                SuccessOrQuit(table->GenerateFor(routerId, challenges[routerId]));
                VerifyOrQuit(table->ContainsMatching(challenges[routerId].AsRx(), routerId));
            }

            // Clear table and verify no matches
            table->Clear();

            for (uint8_t routerId = 0; routerId < Mle::kMaxRouters; routerId++)
            {
                VerifyOrQuit(!table->ContainsMatching(challenges[routerId].AsRx(), routerId));
            }
        }

        testFreeInstance(instance);
        printf("TestTxChallengeTable passed\n");
    }
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    static void HandleChildUpdateRequest(Mle::Mle &aMle, Message &aMessage, const Mac::ExtAddress &aChildExtAddress)
    {
        Ip6::Address     peerAddress;
        Ip6::MessageInfo messageInfo;
        Mle::Mle::RxInfo rxInfo(aMessage, messageInfo);

        peerAddress.InitAsLinkLocalAddress(aChildExtAddress);
        messageInfo.SetPeerAddr(peerAddress);
        messageInfo.SetSockAddr(aMle.GetLinkLocalAddress());

        aMessage.SetOffset(0);

        aMle.HandleChildUpdateRequestOnParent(rxInfo);
    }

    static void TestChildUpdateRequestCslChannel(void)
    {
        static constexpr uint16_t kCslPeriod = 3125;

        struct TestCase
        {
            uint16_t mChannel;
            bool     mShouldAccept;
        };

        static const TestCase kTestCases[] = {
            {0, true}, // Zero indicates CSL channel is not specified.
            {Radio::kChannelMin, true},
            {Radio::kChannelMax, true},
            {Radio::kChannelMax + 1, false},
            {200, false},
            {0x0100 + Radio::kChannelMin, false}, // Would be a valid channel if truncated to `uint8_t`.
            {0xffff, false},
        };

        Instance                   *instance        = static_cast<Instance *>(testInitInstance());
        Mac::ExtAddress             childExtAddress = ExtAddressFromSeed(0x30);
        Mle::Mle                   *mle;
        Mle::DeviceMode             mode;
        Mle::DeviceMode::ModeConfig config;
        Child                      *child;
        uint8_t                     expectedChannel = 0;

        printf("TestChildUpdateRequestCslChannel\n");

        VerifyOrQuit(instance != nullptr);

        mle = &instance->Get<Mle::Mle>();

        config.mRxOnWhenIdle = false;
        config.mDeviceType   = false;
        config.mNetworkData  = false;
        mode.Set(config);

        child = mle->mChildTable.GetNewChild();
        VerifyOrQuit(child != nullptr);

        child->SetExtAddress(childExtAddress);
        child->SetRloc16(kNewChildRloc16);
        child->SetDeviceMode(mode);
        child->SetState(Neighbor::kStateValid);
        child->SetCslPeriod(kCslPeriod);
        child->SetCslSynchronized(true);
        VerifyOrQuit(child->IsCslSynchronized());
        VerifyOrQuit(child->GetCslChannel() == expectedChannel);

        for (const TestCase &testCase : kTestCases)
        {
            Message *message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);

            VerifyOrQuit(message != nullptr);
            message->SetSubType(Message::kSubTypeMle);

            SuccessOrQuit(Tlv::Append<Mle::ModeTlv>(*message, mode.Get()));
            SuccessOrQuit(Tlv::Append<Mle::CslChannelTlv>(*message, Mle::ChannelTlvValue(testCase.mChannel)));

            HandleChildUpdateRequest(*mle, *message, childExtAddress);

            if (testCase.mShouldAccept)
            {
                expectedChannel = static_cast<uint8_t>(testCase.mChannel);
            }

            VerifyOrQuit(child->GetCslChannel() == expectedChannel);

            message->Free();
        }

        testFreeInstance(instance);
        printf("TestChildUpdateRequestCslChannel passed\n");
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
#endif // OPENTHREAD_FTD

private:
    static void SetNetworkData(Instance      &aInstance,
                               uint8_t        aDataVersion,
                               uint8_t        aStableVersion,
                               const uint8_t *aNetworkData,
                               uint8_t        aNetworkDataLength)
    {
        Message    *message = aInstance.Get<MessagePool>().Allocate(Message::kTypeIp6);
        OffsetRange offsetRange;

        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(message->AppendBytes(aNetworkData, aNetworkDataLength));
        offsetRange.Init(0, aNetworkDataLength);

        SuccessOrQuit(aInstance.Get<NetworkData::Leader>().SetNetworkData(
            aDataVersion, aStableVersion, NetworkData::kFullSet, *message, offsetRange));

        message->Free();
    }

    static Message *NewChildIdResponseMessage(Instance              &aInstance,
                                              uint16_t               aSourceAddress,
                                              uint16_t               aChildAddress,
                                              const Mle::LeaderData &aLeaderData,
                                              const uint8_t         *aNetworkData,
                                              uint8_t                aNetworkDataLength,
                                              bool                   aIncludeNetworkData)
    {
        Message                      *message = aInstance.Get<MessagePool>().Allocate(Message::kTypeIp6);
        const Mle::LeaderDataTlvValue leaderDataTlv(aLeaderData);

        VerifyOrQuit(message != nullptr);
        message->SetSubType(Message::kSubTypeMle);

        SuccessOrQuit(Tlv::Append<Mle::SourceAddressTlv>(*message, aSourceAddress));
        SuccessOrQuit(Tlv::Append<Mle::Address16Tlv>(*message, aChildAddress));
        SuccessOrQuit(Tlv::Append<Mle::LeaderDataTlv>(*message, leaderDataTlv));

        if (aIncludeNetworkData)
        {
            SuccessOrQuit(Tlv::Append<Mle::NetworkDataTlv>(*message, aNetworkData, aNetworkDataLength));
        }

        return message;
    }

    static void PrepareChildIdResponse(Mle::Mle &aMle, const Mac::ExtAddress &aParentExtAddress, uint16_t aParentRloc16)
    {
        Parent &parentCandidate = aMle.GetParentCandidate();

        aMle.SetStateDetached();
        aMle.mParent.SetState(Neighbor::kStateInvalid);
        aMle.SetRloc16(Mle::kInvalidRloc16);
        aMle.Get<ThreadNetif>().Up();
        aMle.Get<ThreadNetif>().AddUnicastAddress(aMle.mMeshLocalEid);

        parentCandidate.Clear();
        parentCandidate.GetExtAddress() = aParentExtAddress;
        parentCandidate.SetRloc16(aParentRloc16);
        parentCandidate.SetVersion(kThreadVersion);
        parentCandidate.SetDeviceMode(Mle::DeviceMode(Mle::DeviceMode::kModeFullThreadDevice |
                                                      Mle::DeviceMode::kModeRxOnWhenIdle |
                                                      Mle::DeviceMode::kModeFullNetworkData));
        parentCandidate.SetState(Neighbor::kStateValid);
        aMle.mAttacher.mState = Mle::Mle::Attacher::kStateChildIdRequest;
    }

    static void HandleChildIdResponse(Mle::Mle &aMle, Message &aMessage, const Mac::ExtAddress &aParentExtAddress)
    {
        Ip6::Address     peerAddress;
        Ip6::MessageInfo messageInfo;
        Mle::Mle::RxInfo rxInfo(aMessage, messageInfo);

        peerAddress.InitAsLinkLocalAddress(aParentExtAddress);
        messageInfo.SetPeerAddr(peerAddress);
        messageInfo.SetSockAddr(aMle.GetLinkLocalAddress());

        aMessage.SetOffset(0);
        rxInfo.mNeighbor = &aMle.mAttacher.mParentCandidate;

        aMle.mAttacher.HandleChildIdResponse(rxInfo);
    }

    static void VerifyDataVersions(Instance &aInstance, uint8_t aDataVersion, uint8_t aStableVersion)
    {
        VerifyOrQuit(aInstance.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet) == aDataVersion);
        VerifyOrQuit(aInstance.Get<NetworkData::Leader>().GetVersion(NetworkData::kStableSubset) == aStableVersion);
    }

    static Mle::LeaderData NewLeaderData(uint32_t aPartitionId,
                                         uint8_t  aWeighting,
                                         uint8_t  aLeaderRouterId,
                                         uint8_t  aDataVersion,
                                         uint8_t  aStableVersion)
    {
        Mle::LeaderData leaderData;

        leaderData.SetPartitionId(aPartitionId);
        leaderData.SetWeighting(aWeighting);
        leaderData.SetLeaderRouterId(aLeaderRouterId);
        leaderData.SetDataVersion(aDataVersion);
        leaderData.SetStableDataVersion(aStableVersion);

        return leaderData;
    }

    static void TestValidNetworkDataControl(void)
    {
        Instance             *instance         = static_cast<Instance *>(testInitInstance());
        Mle::Mle             &mle              = instance->Get<Mle::Mle>();
        const Mac::ExtAddress parentExtAddress = ExtAddressFromSeed(0x30);
        const Ip6::Prefix     oldPrefix1       = PrefixFromString("2001:2:0:1::", 64);
        const Ip6::Prefix     oldPrefix2       = PrefixFromString("2001:2:0:2::", 64);
        const Ip6::Prefix     newPrefix1       = PrefixFromString("2001:db8:0:3::", 64);
        const Ip6::Prefix     newPrefix2       = PrefixFromString("2001:db8:0:4::", 64);
        Message              *message;
        Mle::LeaderData       leaderData = NewLeaderData(0x11111111, 64, Mle::RouterIdFromRloc16(kNewParentRloc16),
                                                         kNewDataVersion, kNewStableVersion);

        printf("valid-network-data-control\n");

        SetNetworkData(*instance, kOldDataVersion, kOldStableVersion, kOldNetworkData, sizeof(kOldNetworkData));
        PrepareChildIdResponse(mle, parentExtAddress, kNewParentRloc16);

        message = NewChildIdResponseMessage(*instance, kNewParentRloc16, kNewChildRloc16, leaderData, kNewNetworkData,
                                            sizeof(kNewNetworkData), true);

        HandleChildIdResponse(mle, *message, parentExtAddress);

        VerifyOrQuit(mle.IsChild());
        VerifyOrQuit(mle.GetRloc16() == kNewChildRloc16);
        VerifyOrQuit(mle.GetParent().GetRloc16() == kNewParentRloc16);
        VerifyDataVersions(*instance, kNewDataVersion, kNewStableVersion);
        VerifyContext(*instance, 1, oldPrefix1, false);
        VerifyContext(*instance, 2, oldPrefix2, false);
        VerifyContext(*instance, 3, newPrefix1, true);
        VerifyContext(*instance, 4, newPrefix2, true);

        message->Free();
        testFreeInstance(instance);
    }

    static void TestMissingNetworkDataControl(void)
    {
        Instance             *instance         = static_cast<Instance *>(testInitInstance());
        Mle::Mle             &mle              = instance->Get<Mle::Mle>();
        const Mac::ExtAddress parentExtAddress = ExtAddressFromSeed(0x40);
        const Ip6::Prefix     oldPrefix1       = PrefixFromString("2001:2:0:1::", 64);
        const Ip6::Prefix     oldPrefix2       = PrefixFromString("2001:2:0:2::", 64);
        Message              *message;
        Mle::LeaderData leaderData = NewLeaderData(0x22222222, 64, Mle::RouterIdFromRloc16(kOldParentRloc16), 9, 9);

        printf("missing-network-data-control\n");

        SetNetworkData(*instance, kOldDataVersion, kOldStableVersion, kOldNetworkData, sizeof(kOldNetworkData));
        PrepareChildIdResponse(mle, parentExtAddress, kOldParentRloc16);

        message =
            NewChildIdResponseMessage(*instance, kOldParentRloc16, kOldChildRloc16, leaderData, nullptr, 0, false);

        HandleChildIdResponse(mle, *message, parentExtAddress);

        VerifyOrQuit(!mle.IsChild());
        VerifyDataVersions(*instance, kOldDataVersion, kOldStableVersion);
        VerifyContext(*instance, 1, oldPrefix1, true);
        VerifyContext(*instance, 2, oldPrefix2, true);

        message->Free();
        testFreeInstance(instance);
    }

    static void TestMalformedNetworkDataControl(void)
    {
        Instance             *instance         = static_cast<Instance *>(testInitInstance());
        Mle::Mle             &mle              = instance->Get<Mle::Mle>();
        const Mac::ExtAddress parentExtAddress = ExtAddressFromSeed(0x50);
        const Ip6::Prefix     oldPrefix1       = PrefixFromString("2001:2:0:1::", 64);
        const Ip6::Prefix     oldPrefix2       = PrefixFromString("2001:2:0:2::", 64);
        const Ip6::Prefix     newPrefix1       = PrefixFromString("2001:db8:0:3::", 64);
        Message              *message;
        Mle::LeaderData       leaderData = NewLeaderData(0x33333333, 64, Mle::RouterIdFromRloc16(kNewParentRloc16),
                                                         kNewDataVersion, kNewStableVersion);

        printf("malformed-network-data-control\n");

        SetNetworkData(*instance, kOldDataVersion, kOldStableVersion, kOldNetworkData, sizeof(kOldNetworkData));
        PrepareChildIdResponse(mle, parentExtAddress, kNewParentRloc16);

        message = NewChildIdResponseMessage(*instance, kNewParentRloc16, kNewChildRloc16, leaderData, kNewNetworkData,
                                            kMalformedDataLen, true);

        HandleChildIdResponse(mle, *message, parentExtAddress);

        VerifyOrQuit(mle.IsDetached());
        VerifyOrQuit(mle.GetParent().IsStateInvalid());
        VerifyOrQuit(mle.mAttacher.mState == Mle::Mle::Attacher::kStateStart);
        VerifyOrQuit(mle.mAttacher.mTimer.IsRunning());
        VerifyDataVersions(*instance, kOldDataVersion, kOldStableVersion);
        VerifyContext(*instance, 1, oldPrefix1, true);
        VerifyContext(*instance, 2, oldPrefix2, true);
        VerifyContext(*instance, 3, newPrefix1, false);

        message->Free();
        testFreeInstance(instance);
    }
};

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE

void TestDefaultDeviceProperties(void)
{
    Instance                 *instance;
    const otDeviceProperties *props;
    uint8_t                   weight;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    props = otThreadGetDeviceProperties(instance);

    VerifyOrQuit(props->mPowerSupply == OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY);
    VerifyOrQuit(!props->mSupportsCcm);
    VerifyOrQuit(!props->mIsUnstable);
    VerifyOrQuit(props->mLeaderWeightAdjustment == OPENTHREAD_CONFIG_MLE_DEFAULT_LEADER_WEIGHT_ADJUSTMENT);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    VerifyOrQuit(props->mIsBorderRouter);
#else
    VerifyOrQuit(!props->mIsBorderRouter);
#endif

    weight = 64;

    switch (props->mPowerSupply)
    {
    case OT_POWER_SUPPLY_BATTERY:
        weight -= 8;
        break;
    case OT_POWER_SUPPLY_EXTERNAL:
        break;
    case OT_POWER_SUPPLY_EXTERNAL_STABLE:
        weight += 4;
        break;
    case OT_POWER_SUPPLY_EXTERNAL_UNSTABLE:
        weight -= 4;
        break;
    }

    weight += props->mIsBorderRouter ? 1 : 0;

    VerifyOrQuit(otThreadGetLocalLeaderWeight(instance) == weight);

    printf("TestDefaultDeviceProperties passed\n");
}

void CompareDevicePropertiess(const otDeviceProperties &aFirst, const otDeviceProperties &aSecond)
{
    static constexpr int8_t kMinAdjustment = -16;
    static constexpr int8_t kMaxAdjustment = +16;

    VerifyOrQuit(aFirst.mPowerSupply == aSecond.mPowerSupply);
    VerifyOrQuit(aFirst.mIsBorderRouter == aSecond.mIsBorderRouter);
    VerifyOrQuit(aFirst.mSupportsCcm == aSecond.mSupportsCcm);
    VerifyOrQuit(aFirst.mIsUnstable == aSecond.mIsUnstable);
    VerifyOrQuit(Clamp(aFirst.mLeaderWeightAdjustment, kMinAdjustment, kMaxAdjustment) ==
                 Clamp(aSecond.mLeaderWeightAdjustment, kMinAdjustment, kMaxAdjustment));
}

void TestLeaderWeightCalculation(void)
{
    struct TestCase
    {
        otDeviceProperties mDeviceProperties;
        uint8_t            mExpectedLeaderWeight;
    };

    static const TestCase kTestCases[] = {
        {{OT_POWER_SUPPLY_BATTERY, false, false, false, 0}, 56},
        {{OT_POWER_SUPPLY_EXTERNAL, false, false, false, 0}, 64},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, false, false, false, 0}, 68},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, false, false, false, 0}, 60},

        {{OT_POWER_SUPPLY_BATTERY, true, false, false, 0}, 57},
        {{OT_POWER_SUPPLY_EXTERNAL, true, false, false, 0}, 65},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, true, false, false, 0}, 69},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, true, false, false, 0}, 61},

        {{OT_POWER_SUPPLY_BATTERY, true, true, false, 0}, 64},
        {{OT_POWER_SUPPLY_EXTERNAL, true, true, false, 0}, 72},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, true, true, false, 0}, 76},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, true, true, false, 0}, 68},

        // Check when `mIsUnstable` is set.
        {{OT_POWER_SUPPLY_BATTERY, false, false, true, 0}, 56},
        {{OT_POWER_SUPPLY_EXTERNAL, false, false, true, 0}, 60},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, false, false, true, 0}, 64},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, false, false, true, 0}, 60},

        {{OT_POWER_SUPPLY_BATTERY, true, false, true, 0}, 57},
        {{OT_POWER_SUPPLY_EXTERNAL, true, false, true, 0}, 61},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, true, false, true, 0}, 65},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, true, false, true, 0}, 61},

        // Include non-zero `mLeaderWeightAdjustment`.
        {{OT_POWER_SUPPLY_BATTERY, true, false, false, 10}, 67},
        {{OT_POWER_SUPPLY_EXTERNAL, true, false, false, 10}, 75},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, true, false, false, 10}, 79},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, true, false, false, 10}, 71},

        {{OT_POWER_SUPPLY_BATTERY, false, false, false, -10}, 46},
        {{OT_POWER_SUPPLY_EXTERNAL, false, false, false, -10}, 54},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, false, false, false, -10}, 58},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, false, false, false, -10}, 50},

        // Use `mLeaderWeightAdjustment` larger than valid range
        // Make sure it clamps to -16 and +16.
        {{OT_POWER_SUPPLY_BATTERY, false, false, false, 20}, 72},
        {{OT_POWER_SUPPLY_EXTERNAL, false, false, false, 20}, 80},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, false, false, false, 20}, 84},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, false, false, false, 20}, 76},

        {{OT_POWER_SUPPLY_BATTERY, true, false, false, -20}, 41},
        {{OT_POWER_SUPPLY_EXTERNAL, true, false, false, -20}, 49},
        {{OT_POWER_SUPPLY_EXTERNAL_STABLE, true, false, false, -20}, 53},
        {{OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, true, false, false, -20}, 45},
    };

    Instance *instance;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    for (const TestCase &testCase : kTestCases)
    {
        otThreadSetDeviceProperties(instance, &testCase.mDeviceProperties);
        CompareDevicePropertiess(testCase.mDeviceProperties, *otThreadGetDeviceProperties(instance));
        VerifyOrQuit(otThreadGetLocalLeaderWeight(instance) == testCase.mExpectedLeaderWeight);
    }

    printf("TestLeaderWeightCalculation passed\n");
}

#endif // #if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE

void TestRouterIdMask(void)
{
    Mle::RouterIdMask mask;

    mask.Clear();
    VerifyOrQuit(mask.IsValid());
    VerifyOrQuit(mask.DetermineAllocatedCount() == 0);

    for (uint16_t routerId = 0; routerId <= 255; routerId++)
    {
        VerifyOrQuit(!mask.IsAllocated(static_cast<uint8_t>(routerId)));
    }

    mask.Add(0);
    mask.Add(10);
    mask.Add(Mle::kMaxRouterId);

    VerifyOrQuit(mask.IsAllocated(0));
    VerifyOrQuit(mask.IsAllocated(10));
    VerifyOrQuit(mask.IsAllocated(Mle::kMaxRouterId));
    VerifyOrQuit(!mask.IsAllocated(1));
    VerifyOrQuit(!mask.IsAllocated(61));

    for (uint16_t routerId = Mle::kMaxRouterId + 1; routerId <= 255; routerId++)
    {
        VerifyOrQuit(!mask.IsAllocated(static_cast<uint8_t>(routerId)));
    }

    mask.Remove(10);
    VerifyOrQuit(!mask.IsAllocated(10));

    printf("TestRouterIdMask passed\n");
}

#if OPENTHREAD_FTD
void TestRouterTableRouterIdBounds(void)
{
    Instance    *instance    = static_cast<Instance *>(testInitInstance());
    RouterTable &routerTable = instance->Get<RouterTable>();

    for (uint16_t routerId = 0; routerId <= 255; routerId++)
    {
        VerifyOrQuit(!routerTable.IsAllocated(static_cast<uint8_t>(routerId)));
    }

    testFreeInstance(instance);
    printf("TestRouterTableRouterIdBounds passed\n");
}
#endif

} // namespace ot

int main(void)
{
    ot::TestDeviceMode();
    ot::TestRouterIdMask();
    ot::UnitTester::TestChildIdResponseNetworkDataHandling();

#if OPENTHREAD_FTD
    ot::UnitTester::TestTxChallengeTable();
    ot::TestRouterTableRouterIdBounds();
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ot::UnitTester::TestChildUpdateRequestCslChannel();
#endif
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    ot::TestDefaultDeviceProperties();
    ot::TestLeaderWeightCalculation();
#endif

    printf("All tests passed\n");
    return 0;
}
