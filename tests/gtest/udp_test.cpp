/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <openthread/border_agent.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include <openthread/platform/time.h>
#include "net/socket.hpp"
#include "openthread/error.h"
#include "openthread/message.h"
#include "openthread/udp.h"

#include "fake_platform.hpp"
#include "mock_callback.hpp"

#include "core/net/ip6_address.hpp"

using namespace ot;

using MockReceiveCallback = MockCallback<void, otMessage *, const otMessageInfo *>;

class UdpTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        otOperationalDataset     dataset;
        otOperationalDatasetTlvs datasetTlvs;

        ASSERT_EQ(OT_ERROR_NONE, otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset));

        otDatasetConvertToTlvs(&dataset, &datasetTlvs);
        ASSERT_EQ(OT_ERROR_NONE, otDatasetSetActiveTlvs(FakePlatform::CurrentInstance(), &datasetTlvs));

        ASSERT_EQ(OT_ERROR_NONE, otIp6SetEnabled(FakePlatform::CurrentInstance(), true));
        ASSERT_EQ(OT_ERROR_NONE, otThreadSetEnabled(FakePlatform::CurrentInstance(), true));

        mFakePlatform.GoInMs(10000);
    }

    FakePlatform mFakePlatform;
};

TEST_F(UdpTest, shouldSuccessWhenBindingMulticastAddressAndReceiveFromIt)
{
    otUdpSocket         receiver;
    MockReceiveCallback receiverCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &receiver,
                                       &MockReceiveCallback::CallWithContextAhead, &receiverCallback));

    Ip6::SockAddr listenAddr;

    ASSERT_EQ(OT_ERROR_NONE, listenAddr.GetAddress().FromString("ff02::21"));
    listenAddr.SetPort(2121);

    ASSERT_EQ(OT_ERROR_NONE, otUdpBind(FakePlatform::CurrentInstance(), &receiver, &listenAddr, OT_NETIF_UNSPECIFIED));

    ASSERT_EQ(OT_ERROR_NONE, otIp6SubscribeMulticastAddress(FakePlatform::CurrentInstance(), &listenAddr.mAddress));
    EXPECT_CALL(receiverCallback, Call).Times(1);

    otUdpSocket         sender{};
    MockReceiveCallback senderCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &sender,
                                       &MockReceiveCallback::CallWithContextAhead, &senderCallback));
    otMessageInfo messageInfo{};

    messageInfo.mPeerAddr      = listenAddr.mAddress;
    messageInfo.mPeerPort      = listenAddr.mPort;
    messageInfo.mMulticastLoop = true;

    otMessage *message = otUdpNewMessage(FakePlatform::CurrentInstance(), nullptr);
    ASSERT_NE(message, nullptr);
    ASSERT_EQ(otMessageAppend(message, "multicast", sizeof("multicast") - 1), OT_ERROR_NONE);

    ASSERT_EQ(OT_ERROR_NONE, otUdpSend(FakePlatform::CurrentInstance(), &sender, message, &messageInfo));

    mFakePlatform.GoInMs(1000);

    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &sender));
    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &receiver));
}

TEST_F(UdpTest, shouldSuccessWhenBindingMulticastAddressAndNoReceiveFromDifferentMulticast)
{
    otUdpSocket         receiver;
    MockReceiveCallback receiverCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &receiver,
                                       &MockReceiveCallback::CallWithContextAhead, &receiverCallback));

    Ip6::Address  group1;
    Ip6::Address  group2;
    Ip6::SockAddr listenAddr;

    ASSERT_EQ(OT_ERROR_NONE, group1.FromString("ff02::21"));
    ASSERT_EQ(OT_ERROR_NONE, group2.FromString("ff02::22"));
    listenAddr.SetAddress(group1);
    listenAddr.SetPort(2121);

    ASSERT_EQ(OT_ERROR_NONE, otUdpBind(FakePlatform::CurrentInstance(), &receiver, &listenAddr, OT_NETIF_UNSPECIFIED));

    ASSERT_EQ(OT_ERROR_NONE, otIp6SubscribeMulticastAddress(FakePlatform::CurrentInstance(), &group1));
    ASSERT_EQ(OT_ERROR_NONE, otIp6SubscribeMulticastAddress(FakePlatform::CurrentInstance(), &group2));
    EXPECT_CALL(receiverCallback, Call).Times(0);

    otUdpSocket         sender{};
    MockReceiveCallback senderCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &sender,
                                       &MockReceiveCallback::CallWithContextAhead, &senderCallback));
    otMessageInfo messageInfo{};

    messageInfo.mPeerAddr      = group2;
    messageInfo.mPeerPort      = listenAddr.mPort;
    messageInfo.mMulticastLoop = true;

    otMessage *message = otUdpNewMessage(FakePlatform::CurrentInstance(), nullptr);
    ASSERT_NE(message, nullptr);
    ASSERT_EQ(otMessageAppend(message, "multicast", sizeof("multicast") - 1), OT_ERROR_NONE);

    ASSERT_EQ(OT_ERROR_NONE, otUdpSend(FakePlatform::CurrentInstance(), &sender, message, &messageInfo));

    mFakePlatform.GoInMs(1000);

    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &sender));
    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &receiver));
}

TEST_F(UdpTest, shouldSuccessWhenBindingMulticastAddressAndNoReceiveIfNotSubscribed)
{
    otUdpSocket         receiver;
    MockReceiveCallback receiverCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &receiver,
                                       &MockReceiveCallback::CallWithContextAhead, &receiverCallback));

    Ip6::SockAddr listenAddr;

    ASSERT_EQ(OT_ERROR_NONE, listenAddr.GetAddress().FromString("ff02::21"));
    listenAddr.SetPort(2121);

    ASSERT_EQ(OT_ERROR_NONE, otUdpBind(FakePlatform::CurrentInstance(), &receiver, &listenAddr, OT_NETIF_UNSPECIFIED));

    EXPECT_CALL(receiverCallback, Call).Times(0);

    otUdpSocket         sender{};
    MockReceiveCallback senderCallback;
    ASSERT_EQ(OT_ERROR_NONE, otUdpOpen(FakePlatform::CurrentInstance(), &sender,
                                       &MockReceiveCallback::CallWithContextAhead, &senderCallback));
    otMessageInfo messageInfo{};

    messageInfo.mPeerAddr      = listenAddr.GetAddress();
    messageInfo.mPeerPort      = listenAddr.mPort;
    messageInfo.mMulticastLoop = true;

    otMessage *message = otUdpNewMessage(FakePlatform::CurrentInstance(), nullptr);
    ASSERT_NE(message, nullptr);
    ASSERT_EQ(otMessageAppend(message, "multicast", sizeof("multicast") - 1), OT_ERROR_NONE);

    ASSERT_EQ(OT_ERROR_NONE, otUdpSend(FakePlatform::CurrentInstance(), &sender, message, &messageInfo));

    mFakePlatform.GoInMs(1000);

    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &sender));
    ASSERT_EQ(OT_ERROR_NONE, otUdpClose(FakePlatform::CurrentInstance(), &receiver));
}
