/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements TCP/IPv6 sockets.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include "tcp6.hpp"

#include <new>

#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

#include "../third_party/tcplp/tcplp.h"

namespace ot {
namespace Ip6 {

Tcp::Tcp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEphemeralPort(kDynamicPortMin)
{
}

Error Tcp::Endpoint::Initialize(Instance &aInstance, otTcpEndpointInitializeArgs &aArgs)
{
    Error error = kErrorNone;

    IgnoreReturnValue(new (&GetDelackTimer()) TimerMilli(aInstance, HandleDelackTimer));
    IgnoreReturnValue(new (&GetRexmtPersistTimer()) TimerMilli(aInstance, HandleRexmtPersistTimer));
    IgnoreReturnValue(new (&GetKeepTimer()) TimerMilli(aInstance, HandleKeepTimer));
    IgnoreReturnValue(new (&Get2MslTimer()) TimerMilli(aInstance, Handle2MslTimer));

    mContext                  = aArgs.mContext;
    mEstablishedCallback      = aArgs.mEstablishedCallback;
    mSendDoneCallback         = aArgs.mSendDoneCallback;
    mSendReadyCallback        = aArgs.mSendReadyCallback;
    mReceiveAvailableCallback = aArgs.mReceiveAvailableCallback;
    mDisconnectedCallback     = aArgs.mDisconnectedCallback;

    memset(&mTcb, 0x0, sizeof(mTcb));
    mTcb.instance = &aInstance;

    /*
     * Initialize buffers --- formerly in initialize_tcb.
     */
    uint8_t *recvbuf    = static_cast<uint8_t *>(aArgs.mReceiveBuffer);
    size_t   recvbuflen = aArgs.mReceiveBufferSize - ((aArgs.mReceiveBufferSize + 8) / 9);
    uint8_t *reassbmp   = recvbuf + recvbuflen;
    lbuf_init(&mTcb.sendbuf);
    if (recvbuf)
    {
        cbuf_init(&mTcb.recvbuf, recvbuf, recvbuflen);
        mTcb.reassbmp = reassbmp;
        bmp_init(mTcb.reassbmp, BITS_TO_BYTES(recvbuflen));
    }

    mTcb.accepted_from = nullptr;
    initialize_tcb(&mTcb);

    SuccessOrExit(error = aInstance.Get<Tcp>().mEndpoints.Add(*this));

exit:
    return error;
}

Instance &Tcp::Endpoint::GetInstance(void)
{
    return *reinterpret_cast<Instance *>(mTcb.instance);
}

void *Tcp::Endpoint::GetContext(void)
{
    return mContext;
}

const SockAddr &Tcp::Endpoint::GetLocalAddress(void) const
{
    static otSockAddr temp;
    memcpy(&temp.mAddress, &mTcb.laddr, sizeof(otIp6Address));
    temp.mPort = HostSwap16(mTcb.lport);
    return *static_cast<SockAddr *>(&temp);
}

const SockAddr &Tcp::Endpoint::GetPeerAddress(void) const
{
    static otSockAddr temp;
    memcpy(&temp.mAddress, &mTcb.faddr, sizeof(otIp6Address));
    temp.mPort = HostSwap16(mTcb.fport);
    return *static_cast<SockAddr *>(&temp);
}

Error Tcp::Endpoint::Bind(const SockAddr &aSockName)
{
    Error error;
    VerifyOrExit(!static_cast<const Address *>(&aSockName.mAddress)->IsUnspecified(), error = kErrorInvalidArgs);
    VerifyOrExit(GetInstance().Get<Tcp>().CanBind(aSockName), error = kErrorInvalidState);

    memcpy(&mTcb.laddr, &aSockName.mAddress, sizeof(mTcb.laddr));
    mTcb.lport = HostSwap16(aSockName.mPort);
    error      = kErrorNone;

exit:
    return error;
}

Error Tcp::Endpoint::Connect(const SockAddr &aSockName, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aFlags);

    Error error = kErrorNone;

    VerifyOrExit(mTcb.t_state == TCP6S_CLOSED, error = kErrorInvalidState);

    struct sockaddr_in6 sin6p;
    memcpy(&sin6p.sin6_addr, &aSockName.mAddress, sizeof(sin6p.sin6_addr));
    sin6p.sin6_port = HostSwap16(aSockName.mPort);
    error           = BsdErrorToOtError(tcp6_usr_connect(&mTcb, &sin6p));

exit:
    return error;
}

Error Tcp::Endpoint::SendByReference(otLinkedBuffer &aBuffer, uint32_t aFlags)
{
    Error error;

    bool moreToCome = (aFlags & OT_TCP_SEND_MORE_TO_COME) != 0;
    int  bsdError   = tcp_usr_send(&mTcb, moreToCome ? 1 : 0, &aBuffer, 0);
    SuccessOrExit(error = BsdErrorToOtError(bsdError));

exit:
    return error;
}

Error Tcp::Endpoint::SendByExtension(size_t aNumBytes, uint32_t aFlags)
{
    Error error;

    bool moreToCome = (aFlags & OT_TCP_SEND_MORE_TO_COME) != 0;
    int  bsdError;

    VerifyOrExit(lbuf_head(&mTcb.sendbuf) != nullptr, error = kErrorInvalidState);

    bsdError = tcp_usr_send(&mTcb, moreToCome ? 1 : 0, nullptr, aNumBytes);
    SuccessOrExit(error = BsdErrorToOtError(bsdError));

exit:
    return error;
}

Error Tcp::Endpoint::ReceiveByReference(const otLinkedBuffer *&aBuffer)
{
    cbuf_reference(&mTcb.recvbuf, &mReceiveLinks[0], &mReceiveLinks[1]);
    aBuffer = &mReceiveLinks[0];

    return kErrorNone;
}

Error Tcp::Endpoint::ReceiveContiguify(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Endpoint::CommitReceive(size_t aNumBytes, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aFlags);

    Error error = kErrorNone;

    VerifyOrExit(cbuf_used_space(&mTcb.recvbuf) >= aNumBytes, error = kErrorFailed);
    VerifyOrExit(aNumBytes > 0, error = kErrorNone);

    cbuf_pop(&mTcb.recvbuf, aNumBytes);
    error = BsdErrorToOtError(tcp_usr_rcvd(&mTcb));

exit:
    return error;
}

Error Tcp::Endpoint::SendEndOfStream(void)
{
    return BsdErrorToOtError(tcp_usr_shutdown(&mTcb));
}

Error Tcp::Endpoint::Abort(void)
{
    tcp_usr_abort(&mTcb);
    /* connection_lost will do any reinitialization work for this socket. */
    return kErrorNone;
}

Error Tcp::Endpoint::Deinitialize(void)
{
    Error     error;
    Endpoint *prev;
    Tcp &     tcp = GetInstance().Get<Tcp>();

    SuccessOrExit(error = Abort());

    SuccessOrExit(error = tcp.mEndpoints.Find(*this, prev));
    tcp.mEndpoints.PopAfter(prev);
    SetNext(nullptr);

    GetDelackTimer().~TimerMilli();
    GetRexmtPersistTimer().~TimerMilli();
    GetKeepTimer().~TimerMilli();
    Get2MslTimer().~TimerMilli();

exit:
    return error;
}

bool Tcp::Endpoint::Matches(const MessageInfo &aMessageInfo) const
{
    bool matches = false;

    VerifyOrExit(mTcb.t_state != TCP6S_CLOSED);
    VerifyOrExit(mTcb.lport == HostSwap16(aMessageInfo.GetSockPort()));
    VerifyOrExit(mTcb.fport == HostSwap16(aMessageInfo.GetPeerPort()));
    VerifyOrExit(GetLocalIp6Address().IsUnspecified() || GetLocalIp6Address() == aMessageInfo.GetSockAddr());
    VerifyOrExit(GetForeignIp6Address() == aMessageInfo.GetPeerAddr());

    matches = true;

exit:
    return matches;
}

Error Tcp::Listener::Initialize(Instance &aInstance, otTcpListenerInitializeArgs &aArgs)
{
    Error error = kErrorNone;

    mContext             = aArgs.mContext;
    mAcceptReadyCallback = aArgs.mAcceptReadyCallback;
    mAcceptDoneCallback  = aArgs.mAcceptDoneCallback;

    memset(&mTcbListen, 0x00, sizeof(mTcbListen));
    mTcbListen.instance = &aInstance;

    error = aInstance.Get<Tcp>().mListeners.Add(*this);

    return error;
}

Instance &Tcp::Listener::GetInstance(void)
{
    return *reinterpret_cast<Instance *>(mTcbListen.instance);
}

void *Tcp::Listener::GetContext(void)
{
    return mContext;
}

Error Tcp::Listener::Listen(const SockAddr &aSockName)
{
    Error    error;
    uint16_t port = HostSwap16(aSockName.mPort);

    VerifyOrExit(GetInstance().Get<Tcp>().CanBind(aSockName), error = kErrorInvalidState);

    memcpy(&mTcbListen.laddr, &aSockName.mAddress, sizeof(mTcbListen.laddr));
    mTcbListen.lport   = port;
    mTcbListen.t_state = TCP6S_LISTEN;
    error              = kErrorNone;

exit:
    return error;
}

Error Tcp::Listener::StopListening(void)
{
    memset(&mTcbListen.laddr, 0x00, sizeof(mTcbListen.laddr));
    mTcbListen.lport   = 0;
    mTcbListen.t_state = TCP6S_CLOSED;
    return kErrorNone;
}

Error Tcp::Listener::Deinitialize(void)
{
    Error     error;
    Listener *prev;

    Tcp &tcp = GetInstance().Get<Tcp>();
    SuccessOrExit(error = tcp.mListeners.Find(*this, prev));
    tcp.mListeners.PopAfter(prev);
    SetNext(nullptr);

exit:
    return error;
}

bool Tcp::Listener::Matches(const MessageInfo &aMessageInfo) const
{
    bool matches = false;

    VerifyOrExit(mTcbListen.t_state == TCP6S_LISTEN);
    VerifyOrExit(mTcbListen.lport == HostSwap16(aMessageInfo.GetSockPort()));
    VerifyOrExit(GetLocalIp6Address().IsUnspecified() || GetLocalIp6Address() == aMessageInfo.GetSockAddr());

    matches = true;

exit:
    return matches;
}

Error Tcp::ProcessReceivedSegment(ot::Ip6::Header &aIp6Header, Message &aMessage, MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error error = kErrorNotImplemented;

    union
    {
        Tcp::Header align;
        char        header[60];
    };

    uint16_t length = aIp6Header.GetPayloadLength();
    uint8_t  headerSize;

    struct ip6_hdr *ip6Header;
    struct tcphdr * tcpHeader;

    Endpoint *endpoint;
    Endpoint *endpointPrev;

    Listener *listener;
    Listener *listenerPrev;

    VerifyOrExit(length == aMessage.GetLength() - aMessage.GetOffset(), error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset() + 12, &headerSize, 1));
    headerSize = (headerSize >> 4) << 2;
    VerifyOrExit(headerSize >= 20 && headerSize <= 60 && static_cast<uint16_t>(headerSize) <= length,
                 error = kErrorParse);
    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoTcp));
    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), &header[0], headerSize));

    ip6Header = reinterpret_cast<struct ip6_hdr *>(&aIp6Header);
    tcpHeader = reinterpret_cast<struct tcphdr *>(&header[0]);
    tcp_fields_to_host(tcpHeader);

    aMessageInfo.mPeerPort = HostSwap16(tcpHeader->th_sport);
    aMessageInfo.mSockPort = HostSwap16(tcpHeader->th_dport);

    endpoint = mEndpoints.FindMatching(aMessageInfo, endpointPrev);
    if (endpoint != nullptr)
    {
        struct signals  sig;
        otLinkedBuffer *priorHead = lbuf_head(&endpoint->mTcb.sendbuf);
        memset(&sig, 0x00, sizeof(sig));
        int nextAction = tcp_input(ip6Header, tcpHeader, &aMessage, &endpoint->mTcb, nullptr, &sig);
        if (nextAction != RELOOKUP_REQUIRED)
        {
            if (endpoint->mTcb.t_state != TCP6S_CLOSED)
            {
                ProcessSignals(*endpoint, priorHead, sig);
            }
            ExitNow();
        }
        /* If the matching socket was in the TIME-WAIT state, then we try passive sockets. */
    }

    listener = mListeners.FindMatching(aMessageInfo, listenerPrev);
    if (listener != nullptr)
    {
        tcp_input(ip6Header, tcpHeader, &aMessage, nullptr, &listener->mTcbListen, nullptr);
        ExitNow();
    }

    tcp_dropwithreset(ip6Header, tcpHeader, nullptr, &InstanceLocator::GetInstance(), length - headerSize,
                      ECONNREFUSED);

exit:
    return error;
}

void Tcp::ProcessSignals(Endpoint &aEndpoint, otLinkedBuffer *aPriorHead, struct signals &aSignals)
{
    if (aEndpoint.mSendDoneCallback != nullptr)
    {
        otLinkedBuffer *curr = aPriorHead;
        for (int i = 0; i != aSignals.links_popped; i++)
        {
            otLinkedBuffer *next = curr->mNext;
            curr->mNext          = nullptr;
            aEndpoint.mSendDoneCallback(&aEndpoint, curr);
            curr = next;
        }
    }
    if (aSignals.conn_established && aEndpoint.mEstablishedCallback != nullptr)
    {
        aEndpoint.mEstablishedCallback(&aEndpoint);
    }
    if ((aSignals.recvbuf_notempty || aSignals.rcvd_fin) && aEndpoint.mReceiveAvailableCallback != nullptr)
    {
        aEndpoint.mReceiveAvailableCallback(&aEndpoint, cbuf_used_space(&aEndpoint.mTcb.recvbuf),
                                            aEndpoint.mTcb.reass_fin_index != -1,
                                            cbuf_free_space(&aEndpoint.mTcb.recvbuf));
    }
    if (aEndpoint.mTcb.t_state == TCP6S_TIME_WAIT && aEndpoint.mDisconnectedCallback != nullptr)
    {
        aEndpoint.mDisconnectedCallback(&aEndpoint, OT_TCP_DISCONNECTED_REASON_TIME_WAIT);
    }
}

Error Tcp::BsdErrorToOtError(int aBsdError)
{
    switch (aBsdError)
    {
    case 0:
        return kErrorNone;
    default:
        return kErrorFailed;
    }
}

bool Tcp::CanBind(const SockAddr &aSockName)
{
    uint16_t port = HostSwap16(aSockName.mPort);
    for (Endpoint *endpoint = mEndpoints.GetHead(); endpoint != nullptr; endpoint = endpoint->GetNext())
    {
        if (endpoint->mTcb.lport == port)
        {
            if (aSockName.GetAddress().IsUnspecified())
            {
                return false;
            }
            if (reinterpret_cast<Address *>(&endpoint->mTcb.laddr)->IsUnspecified())
            {
                return false;
            }
            if (memcmp(&endpoint->mTcb.laddr, &aSockName.mAddress, sizeof(endpoint->mTcb.laddr)) == 0)
            {
                return false;
            }
        }
    }

    for (Listener *listener = mListeners.GetHead(); listener != nullptr; listener = listener->GetNext())
    {
        if (listener->mTcbListen.lport == port)
        {
            if (aSockName.GetAddress().IsUnspecified())
            {
                return false;
            }
            if (reinterpret_cast<Address *>(&listener->mTcbListen.laddr)->IsUnspecified())
            {
                return false;
            }
            if (memcmp(&listener->mTcbListen.laddr, &aSockName.mAddress, sizeof(listener->mTcbListen.laddr)) == 0)
            {
                return false;
            }
        }
    }
    return true;
}

void Tcp::HandleDelackTimer(Timer &aTimer)
{
    TimerMilli &timer    = *static_cast<TimerMilli *>(&aTimer);
    Endpoint &  endpoint = Endpoint::FromDelackTimer(timer);
    tcp_timer_delack(&endpoint.mTcb);
}

void Tcp::HandleRexmtPersistTimer(Timer &aTimer)
{
    TimerMilli &timer    = *static_cast<TimerMilli *>(&aTimer);
    Endpoint &  endpoint = Endpoint::FromRexmtPersistTimer(timer);
    if (tcp_timer_active(&endpoint.mTcb, TT_REXMT))
    {
        tcp_timer_rexmt(&endpoint.mTcb);
    }
    else
    {
        tcp_timer_persist(&endpoint.mTcb);
    }
}

void Tcp::HandleKeepTimer(Timer &aTimer)
{
    TimerMilli &timer    = *static_cast<TimerMilli *>(&aTimer);
    Endpoint &  endpoint = Endpoint::FromKeepTimer(timer);
    tcp_timer_keep(&endpoint.mTcb);
}

void Tcp::Handle2MslTimer(Timer &aTimer)
{
    TimerMilli &timer    = *static_cast<TimerMilli *>(&aTimer);
    Endpoint &  endpoint = Endpoint::From2MslTimer(timer);
    tcp_timer_2msl(&endpoint.mTcb);
}

} // namespace Ip6
} // namespace ot

using namespace ot;
using namespace ot::Ip6;

/* Implement TCPlp system stubs declared in tcplp.h. */
extern "C" {

otMessage *tcplp_sys_new_message(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Message * message  = instance.Get<Ip6::Ip6>().NewMessage(0);

    if (message)
    {
        message->SetLinkSecurityEnabled(true);
    }

    return message;
}

void tcplp_sys_free_message(otInstance *aInstance, otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aInstance);
    Message &message = *static_cast<Message *>(aMessage);
    message.Free();
}

void tcplp_sys_send_message(otInstance *aInstance, otMessage *aMessage, otMessageInfo *aMessageInfo)
{
    Instance &        instance = *static_cast<Instance *>(aInstance);
    Message &         message  = *static_cast<Message *>(aMessage);
    Ip6::MessageInfo &info     = *static_cast<Ip6::MessageInfo *>(aMessageInfo);

    otLogDebgTcp("Sending TCP segment: payload_size = %d\n", static_cast<int>(message.GetLength()));

    IgnoreError(instance.Get<Ip6::Ip6>().SendDatagram(message, info, kProtoTcp));
}

uint32_t tcplp_sys_get_ticks()
{
    return TimerMilli::GetNow().GetValue();
}

uint32_t tcplp_sys_get_millis()
{
    return TimerMilli::GetNow().GetValue();
}

void tcplp_sys_set_timer(struct tcpcb *aTcb, uint8_t aTimerId, uint32_t aDelay)
{
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aTcb);
    TimerMilli &   timer    = endpoint.GetTimer(aTimerId);
    timer.Start(aDelay);
}

void tcplp_sys_stop_timer(struct tcpcb *aTcb, uint8_t aTimerId)
{
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aTcb);
    TimerMilli &   timer    = endpoint.GetTimer(aTimerId);
    timer.Stop();
}

struct tcpcb *tcplp_sys_accept_ready(struct tcpcb_listen *aTcbListen, struct in6_addr *aAddr, uint16_t aPort)
{
    Tcp::Listener &listener = Tcp::Listener::FromTcbListen(*aTcbListen);
    if (listener.mAcceptReadyCallback != nullptr)
    {
        otSockAddr addr;
        memcpy(&addr.mAddress, aAddr, sizeof(addr.mAddress));
        addr.mPort = HostSwap16(aPort);

        otTcpEndpoint *               endpoint;
        otTcpIncomingConnectionAction action = listener.mAcceptReadyCallback(&listener, &addr, &endpoint);
        switch (action)
        {
        case OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT:
            return &endpoint->mTcb;
        case OT_TCP_INCOMING_CONNECTION_ACTION_DEFER:
            return nullptr;
        case OT_TCP_INCOMING_CONNECTION_ACTION_REFUSE:
            return (struct tcpcb *)-1;
        }
    }
    return nullptr;
}

bool tcplp_sys_accepted_connection(struct tcpcb_listen *aTcbListen,
                                   struct tcpcb *       aAccepted,
                                   struct in6_addr *    aAddr,
                                   uint16_t             aPort)
{
    Tcp::Listener &listener = Tcp::Listener::FromTcbListen(*aTcbListen);
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aAccepted);
    if (listener.mAcceptDoneCallback != nullptr)
    {
        otSockAddr addr;
        memcpy(&addr.mAddress, aAddr, sizeof(addr.mAddress));
        addr.mPort = HostSwap16(aPort);
        listener.mAcceptDoneCallback(&listener, &endpoint, &addr);
    }
    return true;
}

void tcplp_sys_connection_lost(struct tcpcb *aTcb, uint8_t aErrNum)
{
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aTcb);
    if (endpoint.mDisconnectedCallback != nullptr)
    {
        otTcpDisconnectedReason reason;
        switch (aErrNum)
        {
        case CONN_LOST_NORMAL:
            reason = OT_TCP_DISCONNECTED_REASON_NORMAL;
            break;
        case ECONNREFUSED:
            reason = OT_TCP_DISCONNECTED_REASON_REFUSED;
            break;
        case ETIMEDOUT:
            reason = OT_TCP_DISCONNECTED_REASON_TIMED_OUT;
            break;
        case ECONNRESET:
        default:
            reason = OT_TCP_DISCONNECTED_REASON_RESET;
            break;
        }
        endpoint.mDisconnectedCallback(&endpoint, reason);
    }
}

void tcplp_sys_on_state_change(struct tcpcb *aTcb, int aNewState)
{
    if (aNewState == TCP6S_CLOSED)
    {
        /* Re-initialize the TCB. */
        cbuf_pop(&aTcb->recvbuf, cbuf_used_space(&aTcb->recvbuf));
        aTcb->accepted_from = nullptr;
        initialize_tcb(aTcb);
    }
    /* Any adaptive changes to the sleep interval would go here. */
}

void tcplp_sys_log(const char *aFormat, ...)
{
    char    buffer[128];
    va_list args;
    va_start(args, aFormat);
    vsnprintf(buffer, sizeof(buffer), aFormat, args);
    va_end(args);

    otLogDebgTcp(buffer);
}

const void *tcplp_sys_get_source_ipv6_address(otInstance *aInstance, const struct in6_addr *aPeer)
{
    Instance &  instance = *static_cast<Instance *>(aInstance);
    MessageInfo peerInfo;
    peerInfo.Clear();
    peerInfo.SetPeerAddr(*reinterpret_cast<const Address *>(aPeer));
    const NetifUnicastAddress *netifAddress = instance.Get<Ip6::Ip6>().SelectSourceAddress(peerInfo);
    if (netifAddress == nullptr)
    {
        return nullptr;
    }
    const Address &address = netifAddress->GetAddress();
    return &address;
}

uint16_t tcplp_sys_hostswap16(uint16_t aHostPort)
{
    return HostSwap16(aHostPort);
}

uint32_t tcplp_sys_hostswap32(uint32_t aHostPort)
{
    return HostSwap32(aHostPort);
}

uint32_t tcplp_totalRexmitCnt   = 0;
uint32_t tcplp_timeoutRexmitCnt = 0;

} // extern "C"

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
