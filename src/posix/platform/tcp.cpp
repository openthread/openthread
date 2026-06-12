/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 * @brief
 *   This file implements the platform TCP support for POSIX.
 */

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include "openthread-posix-config.h"
#include "platform-posix.h"

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "posix/platform/infra_if.hpp"
#include "posix/platform/ip6_utils.hpp"
#include "posix/platform/mainloop.hpp"
#include "posix/platform/tcp.hpp"
#include "posix/platform/utils.hpp"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace ot::Posix::Ip6Utils;

namespace {

constexpr int kMaxListenBacklog     = 5;
constexpr int kMaxReceiveBufferSize = 1500;

int GetFd(otPlatTcpListener *aListener) { return aListener->mData.mDescriptor; }

void SetFd(otPlatTcpListener *aListener, int aFd) { aListener->mData.mDescriptor = aFd; }

int GetFd(otPlatTcpConnection *aConn) { return aConn->mData.mDescriptor; }

void SetFd(otPlatTcpConnection *aConn, int aFd) { aConn->mData.mDescriptor = aFd; }

void ConvertSockAddr(const otPlatTcpSockAddr &aIn, struct sockaddr_in6 &aOut)
{
    memset(&aOut, 0, sizeof(aOut));
    aOut.sin6_family   = AF_INET6;
    aOut.sin6_port     = htons(aIn.mSockAddr.mPort);
    aOut.sin6_scope_id = aIn.mIfIndex;

    if (IsIp6AddressUnspecified(aIn.mSockAddr.mAddress))
    {
        aOut.sin6_addr = in6addr_any;
    }
    else
    {
        CopyIp6AddressTo(aIn.mSockAddr.mAddress, &aOut.sin6_addr);
    }
}

void SetNoSigPipe(int aFd)
{
#ifdef SO_NOSIGPIPE
    int opt = 1;
    setsockopt(aFd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#else
    OT_UNUSED_VARIABLE(aFd);
#endif
}

void SetReuseAddr(int aFd)
{
    int opt = 1;

    setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

otError BindToInterface(int aFd, uint32_t aIfIndex)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIfIndex > 0);

#ifdef __linux__
    {
        char ifName[IFNAMSIZ];

        VerifyOrExit(if_indextoname(aIfIndex, ifName) != nullptr, error = OT_ERROR_FAILED);
        VerifyOrExit(setsockopt(aFd, SOL_SOCKET, SO_BINDTODEVICE, ifName, strlen(ifName)) == 0,
                     error = OT_ERROR_FAILED);
    }
#else  // __NetBSD__ || __FreeBSD__ || __APPLE__
    VerifyOrExit(setsockopt(aFd, IPPROTO_IPV6, IPV6_BOUND_IF, &aIfIndex, sizeof(aIfIndex)) == 0,
                 error = OT_ERROR_FAILED);
#endif // __linux__

exit:
    return error;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
// otPlatTcp APIs

extern "C" otError otPlatTcpEnableListener(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aLocalSockAddr)
{
    otError             error = OT_ERROR_NONE;
    int                 fd    = -1;
    struct sockaddr_in6 sin6;

    ConvertSockAddr(*aLocalSockAddr, sin6);

    fd = ot::Posix::SocketWithCloseExec(AF_INET6, SOCK_STREAM, IPPROTO_TCP, ot::Posix::kSocketNonBlock);
    VerifyOrExit(fd >= 0, error = OT_ERROR_FAILED);

    SetReuseAddr(fd);
    SetNoSigPipe(fd);

    SuccessOrExit(error = BindToInterface(fd, aLocalSockAddr->mIfIndex));

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&sin6), sizeof(sin6)) == -1)
    {
        error = (errno == EADDRINUSE) ? OT_ERROR_ALREADY : OT_ERROR_FAILED;
        ExitNow();
    }

    if (listen(fd, kMaxListenBacklog) == -1)
    {
        ExitNow(error = OT_ERROR_FAILED);
    }

    SetFd(aListener, fd);
    fd = -1;

exit:
    if (fd >= 0)
    {
        close(fd);
    }

    return error;
}

extern "C" void otPlatTcpDisableListener(otPlatTcpListener *aListener)
{
    int fd = GetFd(aListener);

    VerifyOrExit(fd >= 0);

    close(fd);
    SetFd(aListener, -1);

exit:
    return;
}

extern "C" otError otPlatTcpConnect(otPlatTcpConnection     *aConn,
                                    const otPlatTcpSockAddr *aPeerSockAddr,
                                    const otPlatTcpSockAddr *aLocalSockAddr)
{
    otError             error = OT_ERROR_NONE;
    int                 fd;
    struct sockaddr_in6 peerSin6;
    uint32_t            ifIndex;

    fd = ot::Posix::SocketWithCloseExec(AF_INET6, SOCK_STREAM, IPPROTO_TCP, ot::Posix::kSocketNonBlock);
    VerifyOrExit(fd >= 0, error = OT_ERROR_FAILED);

    SetNoSigPipe(fd);

    ifIndex = (aLocalSockAddr != nullptr && aLocalSockAddr->mIfIndex > 0) ? aLocalSockAddr->mIfIndex
                                                                          : aPeerSockAddr->mIfIndex;
    SuccessOrExit(error = BindToInterface(fd, ifIndex));

    if (aLocalSockAddr != nullptr)
    {
        struct sockaddr_in6 localSin6;

        ConvertSockAddr(*aLocalSockAddr, localSin6);

        SetReuseAddr(fd);

        if (bind(fd, reinterpret_cast<struct sockaddr *>(&localSin6), sizeof(localSin6)) == -1)
        {
            ExitNow(error = OT_ERROR_FAILED);
        }
    }

    ConvertSockAddr(*aPeerSockAddr, peerSin6);

    if (connect(fd, reinterpret_cast<struct sockaddr *>(&peerSin6), sizeof(peerSin6)) == -1)
    {
        if (errno != EINPROGRESS)
        {
            ExitNow(error = OT_ERROR_FAILED);
        }
    }

    SetFd(aConn, fd);
    fd = -1;

exit:
    if (fd >= 0)
    {
        close(fd);
    }

    return error;
}

extern "C" void otPlatTcpNotifyTxPending(otPlatTcpConnection *aConn)
{
    // No action requires since we use `otPlatTcpIsTxPending()`
    // in `Update(Mainloop::Context &aContext)`.

    OT_UNUSED_VARIABLE(aConn);
}

extern "C" uint16_t otPlatTcpSend(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength)
{
    uint16_t written = 0;
    int      fd      = GetFd(aConn);
    ssize_t  ret;

    VerifyOrExit(fd >= 0);

    ret = send(fd, aBuffer, aLength, MSG_NOSIGNAL);

    if (ret > 0)
    {
        written = static_cast<uint16_t>(ret);
    }

exit:
    return written;
}

extern "C" void otPlatTcpClose(otPlatTcpConnection *aConn)
{
    int fd = GetFd(aConn);

    if (fd >= 0)
    {
        close(fd);
        SetFd(aConn, -1);
    }

    otPlatTcpHandleDisconnected(aConn, OT_PLAT_TCP_DISCONNECT_REASON_CLOSED);
}

extern "C" void otPlatTcpAbort(otPlatTcpConnection *aConn)
{
    struct linger sl;
    int           fd = GetFd(aConn);

    VerifyOrExit(fd >= 0);

    sl.l_onoff  = 1;
    sl.l_linger = 0;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

    close(fd);
    SetFd(aConn, -1);

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Posix::Tcp

namespace ot {
namespace Posix {

const char Tcp::kLogModuleName[] = "Tcp";

Tcp &Tcp::Get(void)
{
    static Tcp sInstance;

    return sInstance;
}

void Tcp::Init(void) { LogDebg("Init"); }

void Tcp::SetUp(void) { Mainloop::Manager::Get().Add(*this); }

void Tcp::TearDown(void) { Mainloop::Manager::Get().Remove(*this); }

void Tcp::Deinit(void) { LogDebg("Deinit"); }

void Tcp::Update(Mainloop::Context &aContext)
{
    otPlatTcpListener   *listener = nullptr;
    otPlatTcpConnection *conn     = nullptr;

    while ((listener = otPlatTcpIterateListeners(gInstance, listener)) != nullptr)
    {
        Mainloop::AddToReadFdSet(GetFd(listener), aContext);
    }

    while ((conn = otPlatTcpIterateConnections(gInstance, conn)) != nullptr)
    {
        int fd = GetFd(conn);

        if (otPlatTcpIsConnecting(conn))
        {
            Mainloop::AddToWriteFdSet(fd, aContext);
            Mainloop::AddToErrorFdSet(fd, aContext);
        }
        else
        {
            Mainloop::AddToReadFdSet(fd, aContext);

            if (otPlatTcpIsTxPending(conn))
            {
                Mainloop::AddToWriteFdSet(fd, aContext);
            }
        }
    }
}

void Tcp::Process(const Mainloop::Context &aContext)
{
    otPlatTcpListener   *listener = nullptr;
    otPlatTcpConnection *conn     = nullptr;

    while ((listener = otPlatTcpIterateListeners(gInstance, listener)) != nullptr)
    {
        ProcessListener(listener, aContext);
    }

    while ((conn = otPlatTcpIterateConnections(gInstance, conn)) != nullptr)
    {
        ProcessConnection(conn, aContext);
    }
}

void Tcp::ProcessListener(otPlatTcpListener *aListener, const Mainloop::Context &aContext)
{
    int                  fd = GetFd(aListener);
    struct sockaddr_in6  peerAddr;
    socklen_t            addrLen = sizeof(peerAddr);
    int                  newFd;
    otPlatTcpSockAddr    peerSockAddr;
    otPlatTcpConnection *newConn;

    VerifyOrExit(fd >= 0);

    VerifyOrExit(Mainloop::IsFdReadable(fd, aContext));

    newFd = accept(fd, reinterpret_cast<struct sockaddr *>(&peerAddr), &addrLen);
    VerifyOrExit(newFd >= 0);

    memset(&peerSockAddr, 0, sizeof(peerSockAddr));
    peerSockAddr.mSockAddr.mPort = ntohs(peerAddr.sin6_port);
    peerSockAddr.mIfIndex        = peerAddr.sin6_scope_id;
    ReadIp6AddressFrom(&peerAddr.sin6_addr, peerSockAddr.mSockAddr.mAddress);

    newConn = otPlatTcpAccept(aListener, &peerSockAddr);
    VerifyOrExit(newConn != nullptr, close(newFd));

    fcntl(newFd, F_SETFD, FD_CLOEXEC);
    fcntl(newFd, F_SETFL, fcntl(newFd, F_GETFL, 0) | O_NONBLOCK);
    SetNoSigPipe(newFd);
    SetFd(newConn, newFd);

    otPlatTcpHandleConnected(newConn);

exit:
    return;
}

void Tcp::ProcessConnection(otPlatTcpConnection *aConn, const Mainloop::Context &aContext)
{
    int fd = GetFd(aConn);

    VerifyOrExit(fd >= 0);

    if (otPlatTcpIsConnecting(aConn))
    {
        int       err = 0;
        socklen_t len = sizeof(err);

        VerifyOrExit(Mainloop::IsFdWritable(fd, aContext) || Mainloop::HasFdErrored(fd, aContext));

        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0)
        {
            otPlatTcpHandleConnected(aConn);
        }
        else
        {
            otPlatTcpDisconnectReason reason = OT_PLAT_TCP_DISCONNECT_REASON_ERROR;

            if (err == ECONNREFUSED)
            {
                reason = OT_PLAT_TCP_DISCONNECT_REASON_REFUSED;
            }
            else if (err == ETIMEDOUT)
            {
                reason = OT_PLAT_TCP_DISCONNECT_REASON_TIMEOUT;
            }

            close(fd);
            SetFd(aConn, -1);

            otPlatTcpHandleDisconnected(aConn, reason);
        }

        ExitNow();
    }

    // Connection is already connected

    if (Mainloop::IsFdReadable(fd, aContext))
    {
        uint8_t buffer[kMaxReceiveBufferSize];
        ssize_t ret = recv(fd, buffer, sizeof(buffer), 0);

        if (ret > 0)
        {
            otPlatTcpHandleReceive(aConn, buffer, static_cast<uint16_t>(ret));

            // The connection may be closed or aborted from within the
            // `otPlatTcpHandleReceive()` callback. We verify that the
            // file descriptor is still valid before proceeding, to
            // avoid checking `IsFdWritable()` and possibly invoking
            // `otPlatTcpHandleTxReady()` on an aborted connection.
            // Note that `otPlatTcpIterateConnections()` guarantees
            // that `aConn` remains valid during iteration (it is not
            // removed from the list).

            VerifyOrExit(GetFd(aConn) >= 0);
        }
        else if (ret == 0)
        {
            close(fd);
            SetFd(aConn, -1);

            otPlatTcpHandleDisconnected(aConn, OT_PLAT_TCP_DISCONNECT_REASON_CLOSED);
            ExitNow();
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
        {
            otPlatTcpDisconnectReason reason = OT_PLAT_TCP_DISCONNECT_REASON_ERROR;

            if (errno == ECONNRESET)
            {
                reason = OT_PLAT_TCP_DISCONNECT_REASON_RESET;
            }
            else if (errno == ETIMEDOUT)
            {
                reason = OT_PLAT_TCP_DISCONNECT_REASON_TIMEOUT;
            }

            close(fd);
            SetFd(aConn, -1);

            otPlatTcpHandleDisconnected(aConn, reason);
            ExitNow();
        }
    }

    if (Mainloop::IsFdWritable(fd, aContext))
    {
        otPlatTcpHandleTxReady(aConn);
    }

exit:
    return;
}

} // namespace Posix
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE
