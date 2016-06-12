/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the Thread network interface.
 */

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

static const uint8_t kThreadMasterKey[] =
{
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

static const char name[] = "thread";

ThreadNetif::ThreadNetif(Ip6::Ip6 &aIp6):
    Netif(aIp6),
    mCoapServer(aIp6.mUdp, kCoapUdpPort),
    mAddressResolver(*this),
    mActiveDataset(*this),
    mPendingDataset(*this),
    mKeyManager(*this),
    mLowpan(*this),
    mMac(*this),
    mMeshForwarder(*this),
    mMleRouter(*this),
    mNetworkDataLocal(*this),
    mNetworkDataLeader(*this),
#if OPENTHREAD_ENABLE_COMMISSIONER
    mCommissioner(*this),
#endif  // OPENTHREAD_ENABLE_COMMISSIONER
#if OPENTHREAD_ENABLE_DTLS
    mDtls(*this),
#endif
#if OPENTHREAD_ENABLE_JOINER
    mJoiner(*this),
#endif  // OPENTHREAD_ENABLE_JOINER
    mJoinerRouter(*this),
    mLeader(*this)
{
    mKeyManager.SetMasterKey(kThreadMasterKey, sizeof(kThreadMasterKey));
}

const char *ThreadNetif::GetName(void) const
{
    return name;
}

ThreadError ThreadNetif::Up(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!mIsUp, error = kThreadError_Already);

    mIp6.AddNetif(*this);
    mMeshForwarder.Start();
    mCoapServer.Start();
    mMleRouter.Enable();
    mIsUp = true;

exit:
    return error;
}

ThreadError ThreadNetif::Down(void)
{
    mCoapServer.Stop();
    mMleRouter.Disable();
    mMeshForwarder.Stop();
    mIp6.RemoveNetif(*this);
    mIsUp = false;
    return kThreadError_None;
}

bool ThreadNetif::IsUp(void) const
{
    return mIsUp;
}

ThreadError ThreadNetif::GetLinkAddress(Ip6::LinkAddress &address) const
{
    address.mType = Ip6::LinkAddress::kEui64;
    address.mLength = sizeof(address.mExtAddress);
    memcpy(&address.mExtAddress, mMac.GetExtAddress(), address.mLength);
    return kThreadError_None;
}

ThreadError ThreadNetif::RouteLookup(const Ip6::Address &source, const Ip6::Address &destination, uint8_t *prefixMatch)
{
    return mNetworkDataLeader.RouteLookup(source, destination, prefixMatch, NULL);
}

ThreadError ThreadNetif::SendMessage(Message &message)
{
    return mMeshForwarder.SendMessage(message);
}

}  // namespace Thread
