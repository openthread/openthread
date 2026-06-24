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

} // namespace ot

int main(void)
{
    ot::TestDeviceMode();
    ot::UnitTester::TestChildIdResponseNetworkDataHandling();

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    ot::TestDefaultDeviceProperties();
    ot::TestLeaderWeightCalculation();
#endif

    printf("All tests passed\n");
    return 0;
}
