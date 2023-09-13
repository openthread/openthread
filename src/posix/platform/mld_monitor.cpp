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

#include "mld_monitor.hpp"

#include "platform-posix.h"
#include "utils.hpp"

#include <common/code_utils.hpp>

#include <openthread/logging.h>
#include <openthread/message.h>
#include <openthread/udp.h>
#include <openthread/platform/dns.h>
#include <openthread/platform/time.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#if OPENTHREAD_POSIX_USE_MLD_MONITOR

namespace ot {
namespace Posix {

// ff02::16
static const otIp6Address kMLDv2MulticastAddress = {
    {{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16}}};

static constexpr int kICMPv6MLDv2Type                      = 143;
static constexpr int kICMPv6MLDv2RecordChangeToExcludeType = 3;
static constexpr int kICMPv6MLDv2RecordChangeToIncludeType = 4;

OT_TOOL_PACKED_BEGIN
struct MLDv2Header
{
    uint8_t  mType;
    uint8_t  _rsv0;
    uint16_t mChecksum;
    uint16_t _rsv1;
    uint16_t mNumRecords;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct MLDv2Record
{
    uint8_t         mRecordType;
    uint8_t         mAuxDataLen;
    uint16_t        mNumSources;
    struct in6_addr mMulticastAddress;
} OT_TOOL_PACKED_END;

MldMonitor &MldMonitor::Get(void)
{
    static MldMonitor sMldMonitor;

    return sMldMonitor;
}

void MldMonitor::SetUp(otInstance *aInstance)
{
    struct ipv6_mreq mreq6;

    mInstance = aInstance;
    VerifyOrExit(mFd < 0);

    mFd                    = SocketWithCloseExec(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, kSocketNonBlock);
    mreq6.ipv6mr_interface = otSysGetThreadNetifIndex();
    memcpy(&mreq6.ipv6mr_multiaddr, kMLDv2MulticastAddress.mFields.m8, sizeof(kMLDv2MulticastAddress.mFields.m8));

    VerifyOrDie(setsockopt(mFd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) == 0, OT_EXIT_FAILURE);
#if defined(__linux__)
    VerifyOrDie(setsockopt(mFd, SOL_SOCKET, SO_BINDTODEVICE, otSysGetThreadNetifName(),
                           static_cast<socklen_t>(strnlen(otSysGetThreadNetifName(), IFNAMSIZ))) == 0,
                OT_EXIT_FAILURE);
#endif

    Mainloop::Manager::Get().Add(*this);

exit:
    return;
}

void MldMonitor::TearDown(void)
{
    Mainloop::Manager::Get().Remove(*this);

    close(mFd);
    mFd = -1;

    mInstance = nullptr;
}

void MldMonitor::Update(otSysMainloopContext &aContext)
{
    if (mFd < 0)
    {
        otLogWarnPlat("[mld] MLD monitor FD is broken");
        ExitNow();
    }

    FD_SET(mFd, &aContext.mReadFdSet);
    FD_SET(mFd, &aContext.mErrorFdSet);
    if (mFd > aContext.mMaxFd)
    {
        aContext.mMaxFd = mFd;
    }

exit:
    return;
}

static void ProcessMLDEvent(otInstance *aInstance, int fd)
{
    const size_t        kMaxMLDEvent = 8192;
    uint8_t             buffer[kMaxMLDEvent];
    ssize_t             bufferLen = -1;
    struct sockaddr_in6 srcAddr;
    socklen_t           addrLen  = sizeof(srcAddr);
    bool                fromSelf = false;
    MLDv2Header        *hdr      = reinterpret_cast<MLDv2Header *>(buffer);
    size_t              offset;
    uint8_t             type;
    struct ifaddrs     *ifAddrs = nullptr;
    char                addressString[INET6_ADDRSTRLEN + 1];

    bufferLen = recvfrom(fd, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&srcAddr), &addrLen);
    VerifyOrExit(bufferLen > 0);

    type = buffer[0];
    VerifyOrExit(type == kICMPv6MLDv2Type && bufferLen >= static_cast<ssize_t>(sizeof(MLDv2Header)));

    // Check whether it is sent by self
    VerifyOrExit(getifaddrs(&ifAddrs) == 0);
    for (struct ifaddrs *ifAddr = ifAddrs; ifAddr != nullptr; ifAddr = ifAddr->ifa_next)
    {
        if (ifAddr->ifa_addr != nullptr && ifAddr->ifa_addr->sa_family == AF_INET6 &&
            strncmp(otSysGetThreadNetifName(), ifAddr->ifa_name, IFNAMSIZ) == 0)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
            struct sockaddr_in6 *addr6 = reinterpret_cast<struct sockaddr_in6 *>(ifAddr->ifa_addr);
#pragma GCC diagnostic pop

            if (memcmp(&addr6->sin6_addr, &srcAddr.sin6_addr, sizeof(in6_addr)) == 0)
            {
                fromSelf = true;
                break;
            }
        }
    }
    VerifyOrExit(fromSelf);

    hdr    = reinterpret_cast<MLDv2Header *>(buffer);
    offset = sizeof(MLDv2Header);

    for (size_t i = 0; i < ntohs(hdr->mNumRecords) && offset < static_cast<size_t>(bufferLen); i++)
    {
        if (static_cast<size_t>(bufferLen) >= (sizeof(MLDv2Record) + offset))
        {
            MLDv2Record *record = reinterpret_cast<MLDv2Record *>(&buffer[offset]);

            otError          err;
            ot::Ip6::Address address;

            memcpy(&address.mFields.m8, &record->mMulticastAddress, sizeof(address.mFields.m8));
            inet_ntop(AF_INET6, &record->mMulticastAddress, addressString, sizeof(addressString));
            if (record->mRecordType == kICMPv6MLDv2RecordChangeToIncludeType)
            {
                err = otIp6SubscribeMulticastAddress(aInstance, &address);
                logAddrEvent(/* isAdd */ true, address, err);
            }
            else if (record->mRecordType == kICMPv6MLDv2RecordChangeToExcludeType)
            {
                err = otIp6UnsubscribeMulticastAddress(aInstance, &address);
                logAddrEvent(/* isAdd */ false, address, err);
            }

            offset += sizeof(MLDv2Record) + sizeof(in6_addr) * ntohs(record->mNumSources);
        }
    }

exit:
    if (ifAddrs)
    {
        freeifaddrs(ifAddrs);
    }
}

void MldMonitor::Process(const otSysMainloopContext &aContext)
{
    if (FD_ISSET(mFd, &aContext.mErrorFdSet))
    {
        close(mFd);
        mFd = -1;
        otLogWarnPlat("[mld] MLD monitor FD is broken");
        ExitNow();
    }

    ProcessMLDEvent(mInstance, mFd);

exit:
    return;
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_USE_MLD_MONITOR
