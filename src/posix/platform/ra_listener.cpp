/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the router advertisement listener.
 */

#include "ra_listener.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <openthread/border_router.h>
#include <openthread/ip6.h>

#include "platform-posix.h"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
// ff02::01
static const otIp6Address kRaMulticastAddress = {
    {{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}};

namespace ot {
namespace Posix {

enum
{
    kICMPv6RaType     = 134,
    kOptionPrefixType = 3,
};

OT_TOOL_PACKED_BEGIN
struct RaPrefixInformation
{
    uint8_t      mPrefixLength;
    uint8_t      _rsv0 : 6;
    uint8_t      mAutoConfiguration : 1;
    uint8_t      mOnLink : 1;
    uint32_t     mValidLifetime;
    uint32_t     mPreferredLifetime;
    uint32_t     _rsv1;
    otIp6Address mPrefix;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct RaOptionHeader
{
    uint8_t mType;
    uint8_t mLength;
};

OT_TOOL_PACKED_BEGIN
struct RaOption
{
    RaOptionHeader      mHeader;
    RaPrefixInformation mPrefixInformation;

} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct RaHeader
{
    uint8_t  mType;
    uint8_t  mCode;
    uint16_t mChecksum;
    uint8_t  mHopLimit;
    uint8_t  _rsv : 6;
    uint8_t  mOtherConfiguration : 1;
    uint8_t  mMananged : 1;
    uint16_t mRouterLifetime;
    uint32_t mReachableTime;
    uint32_t mRetransTimer;
} OT_TOOL_PACKED_END;

RaListener::RaListener(void)
    : mRaFd(0)
{
}

otError RaListener::Init(unsigned int aInterfaceIndex)
{
    otError          error = OT_ERROR_NONE;
    struct ipv6_mreq mreq6;

    mRaFd                  = SocketWithCloseExec(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, kSocketNonBlock);
    mreq6.ipv6mr_interface = aInterfaceIndex;
    memcpy(&mreq6.ipv6mr_multiaddr, kRaMulticastAddress.mFields.m8, sizeof(kRaMulticastAddress.mFields.m8));
    VerifyOrExit(setsockopt(mRaFd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) == 0, error = OT_ERROR_FAILED);

exit:
    otLogInfoPlat("RaListener::Init error=%s", otThreadErrorToString(error));
    return error;
}

void RaListener::DeInit(void)
{
    if (mRaFd != 0)
    {
        close(mRaFd);
        mRaFd = 0;
    }
}

void RaListener::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, fd_set &aErrorFdSet, int &aMaxFd)
{
    (void)aWriteFdSet;

    FD_SET(mRaFd, &aReadFdSet);
    FD_SET(mRaFd, &aErrorFdSet);

    aMaxFd = (mRaFd > aMaxFd) ? mRaFd : aMaxFd;
}

otError RaListener::ProcessEvent(otInstance *  aInstance,
                                 const fd_set &aReadFdSet,
                                 const fd_set &aWriteFdSet,
                                 const fd_set &aErrorFdSet)
{
    (void)aWriteFdSet;

    const size_t kMaxRaEvent = 8192;
    uint8_t      buffer[kMaxRaEvent];
    ssize_t      bufferLen;
    otError      error = OT_ERROR_NONE;
    RaHeader *   header;
    uint8_t *    current;

    VerifyOrExit(FD_ISSET(mRaFd, &aReadFdSet), OT_NOOP);
    VerifyOrExit(!FD_ISSET(mRaFd, &aErrorFdSet), error = OT_ERROR_FAILED);

    bufferLen = recv(mRaFd, buffer, sizeof(buffer), 0);
    VerifyOrExit(bufferLen > 0, OT_NOOP);
    header  = reinterpret_cast<RaHeader *>(&buffer);
    current = &buffer[0];
    VerifyOrExit(static_cast<size_t>(bufferLen) >= sizeof(RaHeader), OT_NOOP);
    VerifyOrExit(header->mType == kICMPv6RaType, OT_NOOP);
    bufferLen -= sizeof(RaHeader);
    current += sizeof(RaHeader);
    while (bufferLen > 0)
    {
        RaOption *option = reinterpret_cast<RaOption *>(current);
        size_t    optionSize;

        VerifyOrExit(static_cast<size_t>(bufferLen) >= sizeof(RaOptionHeader), OT_NOOP);
        optionSize = option->mHeader.mLength * 8; // Length in octaves
        VerifyOrExit(static_cast<size_t>(bufferLen) >= optionSize, OT_NOOP);

        if (option->mHeader.mType == kOptionPrefixType)
        {
            otBorderRouterConfig config;

            VerifyOrExit(optionSize == sizeof(RaOptionHeader) + sizeof(RaPrefixInformation), OT_NOOP);
            memset(&config, 0, sizeof(otBorderRouterConfig));
            config.mSlaac          = option->mPrefixInformation.mAutoConfiguration;
            config.mDhcp           = header->mMananged;
            config.mPrefix.mLength = option->mPrefixInformation.mPrefixLength;
            config.mConfigure      = header->mOtherConfiguration;
            config.mOnMesh         = true; // Propogate this prefix to the network
            memcpy(&config.mPrefix.mPrefix, &option->mPrefixInformation.mPrefix, sizeof(config.mPrefix.mPrefix));

            VerifyOrExit(otBorderRouterAddOnMeshPrefix(aInstance, &config) == OT_ERROR_NONE, OT_NOOP);
            VerifyOrExit(otBorderRouterRegister(aInstance) == OT_ERROR_NONE, OT_NOOP);
            {
                char addressString[INET6_ADDRSTRLEN + 1];

                inet_ntop(AF_INET6, &config.mPrefix.mPrefix, addressString, sizeof(addressString));
                otLogInfoPlat("Added Prefix %s(%d) slaac: %d, dhcp: %d, configure: %d", addressString,
                              config.mPrefix.mLength, config.mSlaac, config.mDhcp, config.mConfigure);
            }
        }

        bufferLen -= optionSize;
        current += optionSize;
    }

exit:
    return error;
}

RaListener::~RaListener()
{
    DeInit();
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
