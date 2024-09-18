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

#include "tcp6.hpp"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include "instance/instance.hpp"

#include "../../third_party/tcplp/tcplp.h"

namespace ot {
namespace Ip6 {

RegisterLogModule("Tcp");

static_assert(sizeof(struct tcpcb) == sizeof(Tcp::Endpoint::mTcb), "mTcb field in otTcpEndpoint is sized incorrectly");
static_assert(alignof(struct tcpcb) == alignof(decltype(Tcp::Endpoint::mTcb)),
              "mTcb field in otTcpEndpoint is aligned incorrectly");
static_assert(offsetof(Tcp::Endpoint, mTcb) == 0, "mTcb field in otTcpEndpoint has nonzero offset");

static_assert(sizeof(struct tcpcb_listen) == sizeof(Tcp::Listener::mTcbListen),
              "mTcbListen field in otTcpListener is sized incorrectly");
static_assert(alignof(struct tcpcb_listen) == alignof(decltype(Tcp::Listener::mTcbListen)),
              "mTcbListen field in otTcpListener is aligned incorrectly");
static_assert(offsetof(Tcp::Listener, mTcbListen) == 0, "mTcbListen field in otTcpEndpoint has nonzero offset");

Tcp::Tcp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
    , mTasklet(aInstance)
    , mEphemeralPort(kDynamicPortMin)
{
    OT_UNUSED_VARIABLE(mEphemeralPort);
}

Error Tcp::Endpoint::Initialize(Instance &aInstance, const otTcpEndpointInitializeArgs &aArgs)
{
    Error         error;
    struct tcpcb &tp = GetTcb();

    ClearAllBytes(tp);

    SuccessOrExit(error = aInstance.Get<Tcp>().mEndpoints.Add(*this));

    mContext                  = aArgs.mContext;
    mEstablishedCallback      = aArgs.mEstablishedCallback;
    mSendDoneCallback         = aArgs.mSendDoneCallback;
    mForwardProgressCallback  = aArgs.mForwardProgressCallback;
    mReceiveAvailableCallback = aArgs.mReceiveAvailableCallback;
    mDisconnectedCallback     = aArgs.mDisconnectedCallback;

    ClearAllBytes(mTimers);
    ClearAllBytes(mSockAddr);
    mPendingCallbacks = 0;

    /*
     * Initialize buffers --- formerly in initialize_tcb.
     */
    {
        uint8_t *recvbuf    = static_cast<uint8_t *>(aArgs.mReceiveBuffer);
        size_t   recvbuflen = aArgs.mReceiveBufferSize - ((aArgs.mReceiveBufferSize + 8) / 9);
        uint8_t *reassbmp   = recvbuf + recvbuflen;

        lbuf_init(&tp.sendbuf);
        cbuf_init(&tp.recvbuf, recvbuf, recvbuflen);
        tp.reassbmp = reassbmp;
        bmp_init(tp.reassbmp, BITS_TO_BYTES(recvbuflen));
    }

    tp.accepted_from = nullptr;
    initialize_tcb(&tp);

    /* Note that we do not need to zero-initialize mReceiveLinks. */

    tp.instance = &aInstance;

exit:
    return error;
}

Instance &Tcp::Endpoint::GetInstance(void) const { return AsNonConst(AsCoreType(GetTcb().instance)); }

const SockAddr &Tcp::Endpoint::GetLocalAddress(void) const
{
    const struct tcpcb &tp = GetTcb();

    static otSockAddr temp;

    memcpy(&temp.mAddress, &tp.laddr, sizeof(temp.mAddress));
    temp.mPort = BigEndian::HostSwap16(tp.lport);

    return AsCoreType(&temp);
}

const SockAddr &Tcp::Endpoint::GetPeerAddress(void) const
{
    const struct tcpcb &tp = GetTcb();

    static otSockAddr temp;

    memcpy(&temp.mAddress, &tp.faddr, sizeof(temp.mAddress));
    temp.mPort = BigEndian::HostSwap16(tp.fport);

    return AsCoreType(&temp);
}

Error Tcp::Endpoint::Bind(const SockAddr &aSockName)
{
    Error         error;
    struct tcpcb &tp = GetTcb();

    VerifyOrExit(!AsCoreType(&aSockName.mAddress).IsUnspecified(), error = kErrorInvalidArgs);
    VerifyOrExit(Get<Tcp>().CanBind(aSockName), error = kErrorInvalidState);

    memcpy(&tp.laddr, &aSockName.mAddress, sizeof(tp.laddr));
    tp.lport = BigEndian::HostSwap16(aSockName.mPort);
    error    = kErrorNone;

exit:
    return error;
}

Error Tcp::Endpoint::Connect(const SockAddr &aSockName, uint32_t aFlags)
{
    Error         error = kErrorNone;
    struct tcpcb &tp    = GetTcb();

    VerifyOrExit(tp.t_state == TCP6S_CLOSED, error = kErrorInvalidState);

    if (aFlags & OT_TCP_CONNECT_NO_FAST_OPEN)
    {
        struct sockaddr_in6 sin6p;

        tp.t_flags &= ~TF_FASTOPEN;
        memcpy(&sin6p.sin6_addr, &aSockName.mAddress, sizeof(sin6p.sin6_addr));
        sin6p.sin6_port = BigEndian::HostSwap16(aSockName.mPort);
        error           = BsdErrorToOtError(tcp6_usr_connect(&tp, &sin6p));
    }
    else
    {
        tp.t_flags |= TF_FASTOPEN;

        /* Stash the destination address in tp. */
        memcpy(&tp.faddr, &aSockName.mAddress, sizeof(tp.faddr));
        tp.fport = BigEndian::HostSwap16(aSockName.mPort);
    }

exit:
    return error;
}

Error Tcp::Endpoint::SendByReference(otLinkedBuffer &aBuffer, uint32_t aFlags)
{
    Error         error;
    struct tcpcb &tp = GetTcb();

    size_t backlogBefore = GetBacklogBytes();
    size_t sent          = aBuffer.mLength;

    struct sockaddr_in6  sin6p;
    struct sockaddr_in6 *name = nullptr;

    if (IS_FASTOPEN(tp.t_flags))
    {
        memcpy(&sin6p.sin6_addr, &tp.faddr, sizeof(sin6p.sin6_addr));
        sin6p.sin6_port = tp.fport;
        name            = &sin6p;
    }
    SuccessOrExit(
        error = BsdErrorToOtError(tcp_usr_send(&tp, (aFlags & OT_TCP_SEND_MORE_TO_COME) != 0, &aBuffer, 0, name)));

    PostCallbacksAfterSend(sent, backlogBefore);

exit:
    return error;
}

Error Tcp::Endpoint::SendByExtension(size_t aNumBytes, uint32_t aFlags)
{
    Error         error;
    bool          moreToCome    = (aFlags & OT_TCP_SEND_MORE_TO_COME) != 0;
    struct tcpcb &tp            = GetTcb();
    size_t        backlogBefore = GetBacklogBytes();
    int           bsdError;

    struct sockaddr_in6  sin6p;
    struct sockaddr_in6 *name = nullptr;

    VerifyOrExit(lbuf_head(&tp.sendbuf) != nullptr, error = kErrorInvalidState);

    if (IS_FASTOPEN(tp.t_flags))
    {
        memcpy(&sin6p.sin6_addr, &tp.faddr, sizeof(sin6p.sin6_addr));
        sin6p.sin6_port = tp.fport;
        name            = &sin6p;
    }

    bsdError = tcp_usr_send(&tp, moreToCome ? 1 : 0, nullptr, aNumBytes, name);
    SuccessOrExit(error = BsdErrorToOtError(bsdError));

    PostCallbacksAfterSend(aNumBytes, backlogBefore);

exit:
    return error;
}

Error Tcp::Endpoint::ReceiveByReference(const otLinkedBuffer *&aBuffer)
{
    struct tcpcb &tp = GetTcb();

    cbuf_reference(&tp.recvbuf, &mReceiveLinks[0], &mReceiveLinks[1]);
    aBuffer = &mReceiveLinks[0];

    return kErrorNone;
}

Error Tcp::Endpoint::ReceiveContiguify(void)
{
    struct tcpcb &tp = GetTcb();

    cbuf_contiguify(&tp.recvbuf, tp.reassbmp);

    return kErrorNone;
}

Error Tcp::Endpoint::CommitReceive(size_t aNumBytes, uint32_t aFlags)
{
    Error         error = kErrorNone;
    struct tcpcb &tp    = GetTcb();

    OT_UNUSED_VARIABLE(aFlags);

    VerifyOrExit(cbuf_used_space(&tp.recvbuf) >= aNumBytes, error = kErrorFailed);
    VerifyOrExit(aNumBytes > 0, error = kErrorNone);

    cbuf_pop(&tp.recvbuf, aNumBytes);
    error = BsdErrorToOtError(tcp_usr_rcvd(&tp));

exit:
    return error;
}

Error Tcp::Endpoint::SendEndOfStream(void)
{
    struct tcpcb &tp = GetTcb();

    return BsdErrorToOtError(tcp_usr_shutdown(&tp));
}

Error Tcp::Endpoint::Abort(void)
{
    struct tcpcb &tp = GetTcb();

    tcp_usr_abort(&tp);
    /* connection_lost will do any reinitialization work for this socket. */
    return kErrorNone;
}

Error Tcp::Endpoint::Deinitialize(void)
{
    Error error;

    SuccessOrExit(error = Get<Tcp>().mEndpoints.Remove(*this));
    SetNext(nullptr);

    SuccessOrExit(error = Abort());

exit:
    return error;
}

bool Tcp::Endpoint::IsClosed(void) const { return GetTcb().t_state == TCP6S_CLOSED; }

uint8_t Tcp::Endpoint::TimerFlagToIndex(uint8_t aTimerFlag)
{
    uint8_t timerIndex = 0;

    switch (aTimerFlag)
    {
    case TT_DELACK:
        timerIndex = kTimerDelack;
        break;
    case TT_REXMT:
    case TT_PERSIST:
        timerIndex = kTimerRexmtPersist;
        break;
    case TT_KEEP:
        timerIndex = kTimerKeep;
        break;
    case TT_2MSL:
        timerIndex = kTimer2Msl;
        break;
    }

    return timerIndex;
}

bool Tcp::Endpoint::IsTimerActive(uint8_t aTimerIndex)
{
    bool          active = false;
    struct tcpcb *tp     = &GetTcb();

    OT_ASSERT(aTimerIndex < kNumTimers);
    switch (aTimerIndex)
    {
    case kTimerDelack:
        active = tcp_timer_active(tp, TT_DELACK);
        break;
    case kTimerRexmtPersist:
        active = tcp_timer_active(tp, TT_REXMT) || tcp_timer_active(tp, TT_PERSIST);
        break;
    case kTimerKeep:
        active = tcp_timer_active(tp, TT_KEEP);
        break;
    case kTimer2Msl:
        active = tcp_timer_active(tp, TT_2MSL);
        break;
    }

    return active;
}

void Tcp::Endpoint::SetTimer(uint8_t aTimerFlag, uint32_t aDelay)
{
    /*
     * TCPlp has already set the flag for this timer to record that it's
     * running. So, all that's left to do is record the expiry time and
     * (re)set the main timer as appropriate.
     */

    TimeMilli now         = TimerMilli::GetNow();
    TimeMilli newFireTime = now + aDelay;
    uint8_t   timerIndex  = TimerFlagToIndex(aTimerFlag);

    mTimers[timerIndex] = newFireTime.GetValue();
    LogDebg("Endpoint %p set timer %u to %u ms", static_cast<void *>(this), static_cast<unsigned int>(timerIndex),
            static_cast<unsigned int>(aDelay));

    Get<Tcp>().mTimer.FireAtIfEarlier(newFireTime);
}

void Tcp::Endpoint::CancelTimer(uint8_t aTimerFlag)
{
    /*
     * TCPlp has already cleared the timer flag before calling this. Since the
     * main timer's callback properly handles the case where no timers are
     * actually due, there's actually no work to be done here.
     */

    OT_UNUSED_VARIABLE(aTimerFlag);

    LogDebg("Endpoint %p cancelled timer %u", static_cast<void *>(this),
            static_cast<unsigned int>(TimerFlagToIndex(aTimerFlag)));
}

bool Tcp::Endpoint::FirePendingTimers(TimeMilli aNow, bool &aHasFutureTimer, TimeMilli &aEarliestFutureExpiry)
{
    bool          calledUserCallback = false;
    struct tcpcb *tp                 = &GetTcb();

    /*
     * NOTE: Firing a timer might potentially activate/deactivate other timers.
     * If timers x and y expire at the same time, but the callback for timer x
     * (for x < y) cancels or postpones timer y, should timer y's callback be
     * called? Our answer is no, since timer x's callback has updated the
     * TCP stack's state in such a way that it no longer expects timer y's
     * callback to to be called. Because the TCP stack thinks that timer y
     * has been cancelled, calling timer y's callback could potentially cause
     * problems.
     *
     * If the timer callbacks set other timers, then they may not be taken
     * into account when setting aEarliestFutureExpiry. But mTimer's expiry
     * time will be updated by those, so we can just compare against mTimer's
     * expiry time when resetting mTimer.
     */
    for (uint8_t timerIndex = 0; timerIndex != kNumTimers; timerIndex++)
    {
        if (IsTimerActive(timerIndex))
        {
            TimeMilli expiry(mTimers[timerIndex]);

            if (expiry <= aNow)
            {
                /*
                 * If a user callback is called, then return true. For TCPlp,
                 * this only happens if the connection is dropped (e.g., it
                 * times out).
                 */
                int dropped = 0;

                switch (timerIndex)
                {
                case kTimerDelack:
                    dropped = tcp_timer_delack(tp);
                    break;
                case kTimerRexmtPersist:
                    if (tcp_timer_active(tp, TT_REXMT))
                    {
                        dropped = tcp_timer_rexmt(tp);
                    }
                    else
                    {
                        dropped = tcp_timer_persist(tp);
                    }
                    break;
                case kTimerKeep:
                    dropped = tcp_timer_keep(tp);
                    break;
                case kTimer2Msl:
                    dropped = tcp_timer_2msl(tp);
                    break;
                }
                VerifyOrExit(dropped == 0, calledUserCallback = true);
            }
            else
            {
                aHasFutureTimer       = true;
                aEarliestFutureExpiry = Min(aEarliestFutureExpiry, expiry);
            }
        }
    }

exit:
    return calledUserCallback;
}

void Tcp::Endpoint::PostCallbacksAfterSend(size_t aSent, size_t aBacklogBefore)
{
    size_t backlogAfter = GetBacklogBytes();

    if (backlogAfter < aBacklogBefore + aSent && mForwardProgressCallback != nullptr)
    {
        mPendingCallbacks |= kForwardProgressCallbackFlag;
        Get<Tcp>().mTasklet.Post();
    }
}

bool Tcp::Endpoint::FirePendingCallbacks(void)
{
    bool calledUserCallback = false;

    if ((mPendingCallbacks & kForwardProgressCallbackFlag) != 0 && mForwardProgressCallback != nullptr)
    {
        mForwardProgressCallback(this, GetSendBufferBytes(), GetBacklogBytes());
        calledUserCallback = true;
    }

    mPendingCallbacks = 0;

    return calledUserCallback;
}

size_t Tcp::Endpoint::GetSendBufferBytes(void) const
{
    const struct tcpcb &tp = GetTcb();
    return lbuf_used_space(&tp.sendbuf);
}

size_t Tcp::Endpoint::GetInFlightBytes(void) const
{
    const struct tcpcb &tp = GetTcb();
    return tp.snd_max - tp.snd_una;
}

size_t Tcp::Endpoint::GetBacklogBytes(void) const { return GetSendBufferBytes() - GetInFlightBytes(); }

Address &Tcp::Endpoint::GetLocalIp6Address(void) { return *reinterpret_cast<Address *>(&GetTcb().laddr); }

const Address &Tcp::Endpoint::GetLocalIp6Address(void) const
{
    return *reinterpret_cast<const Address *>(&GetTcb().laddr);
}

Address &Tcp::Endpoint::GetForeignIp6Address(void) { return *reinterpret_cast<Address *>(&GetTcb().faddr); }

const Address &Tcp::Endpoint::GetForeignIp6Address(void) const
{
    return *reinterpret_cast<const Address *>(&GetTcb().faddr);
}

bool Tcp::Endpoint::Matches(const MessageInfo &aMessageInfo) const
{
    bool                matches = false;
    const struct tcpcb *tp      = &GetTcb();

    VerifyOrExit(tp->t_state != TCP6S_CLOSED);
    VerifyOrExit(tp->lport == BigEndian::HostSwap16(aMessageInfo.GetSockPort()));
    VerifyOrExit(tp->fport == BigEndian::HostSwap16(aMessageInfo.GetPeerPort()));
    VerifyOrExit(GetLocalIp6Address().IsUnspecified() || GetLocalIp6Address() == aMessageInfo.GetSockAddr());
    VerifyOrExit(GetForeignIp6Address() == aMessageInfo.GetPeerAddr());

    matches = true;

exit:
    return matches;
}

Error Tcp::Listener::Initialize(Instance &aInstance, const otTcpListenerInitializeArgs &aArgs)
{
    Error                error;
    struct tcpcb_listen *tpl = &GetTcbListen();

    SuccessOrExit(error = aInstance.Get<Tcp>().mListeners.Add(*this));

    mContext             = aArgs.mContext;
    mAcceptReadyCallback = aArgs.mAcceptReadyCallback;
    mAcceptDoneCallback  = aArgs.mAcceptDoneCallback;

    ClearAllBytes(*tpl);
    tpl->instance = &aInstance;

exit:
    return error;
}

Instance &Tcp::Listener::GetInstance(void) const { return AsNonConst(AsCoreType(GetTcbListen().instance)); }

Error Tcp::Listener::Listen(const SockAddr &aSockName)
{
    Error                error;
    uint16_t             port = BigEndian::HostSwap16(aSockName.mPort);
    struct tcpcb_listen *tpl  = &GetTcbListen();

    VerifyOrExit(Get<Tcp>().CanBind(aSockName), error = kErrorInvalidState);

    memcpy(&tpl->laddr, &aSockName.mAddress, sizeof(tpl->laddr));
    tpl->lport   = port;
    tpl->t_state = TCP6S_LISTEN;
    error        = kErrorNone;

exit:
    return error;
}

Error Tcp::Listener::StopListening(void)
{
    struct tcpcb_listen *tpl = &GetTcbListen();

    ClearAllBytes(tpl->laddr);
    tpl->lport   = 0;
    tpl->t_state = TCP6S_CLOSED;
    return kErrorNone;
}

Error Tcp::Listener::Deinitialize(void)
{
    Error error;

    SuccessOrExit(error = Get<Tcp>().mListeners.Remove(*this));
    SetNext(nullptr);

exit:
    return error;
}

bool Tcp::Listener::IsClosed(void) const { return GetTcbListen().t_state == TCP6S_CLOSED; }

Address &Tcp::Listener::GetLocalIp6Address(void) { return *reinterpret_cast<Address *>(&GetTcbListen().laddr); }

const Address &Tcp::Listener::GetLocalIp6Address(void) const
{
    return *reinterpret_cast<const Address *>(&GetTcbListen().laddr);
}

bool Tcp::Listener::Matches(const MessageInfo &aMessageInfo) const
{
    bool                       matches = false;
    const struct tcpcb_listen *tpl     = &GetTcbListen();

    VerifyOrExit(tpl->t_state == TCP6S_LISTEN);
    VerifyOrExit(tpl->lport == BigEndian::HostSwap16(aMessageInfo.GetSockPort()));
    VerifyOrExit(GetLocalIp6Address().IsUnspecified() || GetLocalIp6Address() == aMessageInfo.GetSockAddr());

    matches = true;

exit:
    return matches;
}

Error Tcp::HandleMessage(ot::Ip6::Header &aIp6Header, Message &aMessage, MessageInfo &aMessageInfo)
{
    Error error = kErrorNotImplemented;

    /*
     * The type uint32_t was chosen for alignment purposes. The size is the
     * maximum TCP header size, including options.
     */
    uint32_t header[15];

    uint16_t length = aIp6Header.GetPayloadLength();
    uint8_t  headerSize;

    struct ip6_hdr *ip6Header;
    struct tcphdr  *tcpHeader;

    Endpoint *endpoint;
    Endpoint *endpointPrev;

    Listener *listener;
    Listener *listenerPrev;

    struct tcplp_signals sig;
    int                  nextAction;

    VerifyOrExit(length == aMessage.GetLength() - aMessage.GetOffset(), error = kErrorParse);
    VerifyOrExit(length >= sizeof(Tcp::Header), error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset() + offsetof(struct tcphdr, th_off_x2), headerSize));
    headerSize = static_cast<uint8_t>((headerSize >> TH_OFF_SHIFT) << 2);
    VerifyOrExit(headerSize >= sizeof(struct tcphdr) && headerSize <= sizeof(header) &&
                     static_cast<uint16_t>(headerSize) <= length,
                 error = kErrorParse);
    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoTcp));
    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), &header[0], headerSize));

    ip6Header = reinterpret_cast<struct ip6_hdr *>(&aIp6Header);
    tcpHeader = reinterpret_cast<struct tcphdr *>(&header[0]);
    tcp_fields_to_host(tcpHeader);

    aMessageInfo.mPeerPort = BigEndian::HostSwap16(tcpHeader->th_sport);
    aMessageInfo.mSockPort = BigEndian::HostSwap16(tcpHeader->th_dport);

    endpoint = mEndpoints.FindMatching(aMessageInfo, endpointPrev);
    if (endpoint != nullptr)
    {
        struct tcpcb *tp = &endpoint->GetTcb();

        otLinkedBuffer *priorHead    = lbuf_head(&tp->sendbuf);
        size_t          priorBacklog = endpoint->GetSendBufferBytes() - endpoint->GetInFlightBytes();

        ClearAllBytes(sig);
        nextAction = tcp_input(ip6Header, tcpHeader, &aMessage, tp, nullptr, &sig);
        if (nextAction != RELOOKUP_REQUIRED)
        {
            ProcessSignals(*endpoint, priorHead, priorBacklog, sig);
            ExitNow();
        }
        /* If the matching socket was in the TIME-WAIT state, then we try passive sockets. */
    }

    listener = mListeners.FindMatching(aMessageInfo, listenerPrev);
    if (listener != nullptr)
    {
        struct tcpcb_listen *tpl = &listener->GetTcbListen();

        ClearAllBytes(sig);
        nextAction = tcp_input(ip6Header, tcpHeader, &aMessage, nullptr, tpl, &sig);
        OT_ASSERT(nextAction != RELOOKUP_REQUIRED);
        if (sig.accepted_connection != nullptr)
        {
            ProcessSignals(Tcp::Endpoint::FromTcb(*sig.accepted_connection), nullptr, 0, sig);
        }
        ExitNow();
    }

    tcp_dropwithreset(ip6Header, tcpHeader, nullptr, &InstanceLocator::GetInstance(), length - headerSize,
                      ECONNREFUSED);

exit:
    return error;
}

void Tcp::ProcessSignals(Endpoint             &aEndpoint,
                         otLinkedBuffer       *aPriorHead,
                         size_t                aPriorBacklog,
                         struct tcplp_signals &aSignals) const
{
    VerifyOrExit(IsInitialized(aEndpoint) && !aEndpoint.IsClosed());
    if (aSignals.conn_established && aEndpoint.mEstablishedCallback != nullptr)
    {
        aEndpoint.mEstablishedCallback(&aEndpoint);
    }

    VerifyOrExit(IsInitialized(aEndpoint) && !aEndpoint.IsClosed());
    if (aEndpoint.mSendDoneCallback != nullptr)
    {
        otLinkedBuffer *curr = aPriorHead;

        OT_ASSERT(curr != nullptr || aSignals.links_popped == 0);

        for (uint32_t i = 0; i != aSignals.links_popped; i++)
        {
            otLinkedBuffer *next = curr->mNext;

            VerifyOrExit(i == 0 || (IsInitialized(aEndpoint) && !aEndpoint.IsClosed()));

            curr->mNext = nullptr;
            aEndpoint.mSendDoneCallback(&aEndpoint, curr);
            curr = next;
        }
    }

    VerifyOrExit(IsInitialized(aEndpoint) && !aEndpoint.IsClosed());
    if (aEndpoint.mForwardProgressCallback != nullptr)
    {
        size_t backlogBytes = aEndpoint.GetBacklogBytes();

        if (aSignals.bytes_acked > 0 || backlogBytes < aPriorBacklog)
        {
            aEndpoint.mForwardProgressCallback(&aEndpoint, aEndpoint.GetSendBufferBytes(), backlogBytes);
            aEndpoint.mPendingCallbacks &= ~kForwardProgressCallbackFlag;
        }
    }

    VerifyOrExit(IsInitialized(aEndpoint) && !aEndpoint.IsClosed());
    if ((aSignals.recvbuf_added || aSignals.rcvd_fin) && aEndpoint.mReceiveAvailableCallback != nullptr)
    {
        aEndpoint.mReceiveAvailableCallback(&aEndpoint, cbuf_used_space(&aEndpoint.GetTcb().recvbuf),
                                            aEndpoint.GetTcb().reass_fin_index != -1,
                                            cbuf_free_space(&aEndpoint.GetTcb().recvbuf));
    }

    VerifyOrExit(IsInitialized(aEndpoint) && !aEndpoint.IsClosed());
    if (aEndpoint.GetTcb().t_state == TCP6S_TIME_WAIT && aEndpoint.mDisconnectedCallback != nullptr)
    {
        aEndpoint.mDisconnectedCallback(&aEndpoint, OT_TCP_DISCONNECTED_REASON_TIME_WAIT);
    }

exit:
    return;
}

Error Tcp::BsdErrorToOtError(int aBsdError)
{
    Error error = kErrorFailed;

    switch (aBsdError)
    {
    case 0:
        error = kErrorNone;
        break;
    }

    return error;
}

bool Tcp::CanBind(const SockAddr &aSockName)
{
    uint16_t port    = BigEndian::HostSwap16(aSockName.mPort);
    bool     allowed = false;

    for (Endpoint &endpoint : mEndpoints)
    {
        struct tcpcb *tp = &endpoint.GetTcb();

        if (tp->lport == port)
        {
            VerifyOrExit(!aSockName.GetAddress().IsUnspecified());
            VerifyOrExit(!reinterpret_cast<Address *>(&tp->laddr)->IsUnspecified());
            VerifyOrExit(memcmp(&endpoint.GetTcb().laddr, &aSockName.mAddress, sizeof(tp->laddr)) != 0);
        }
    }

    for (Listener &listener : mListeners)
    {
        struct tcpcb_listen *tpl = &listener.GetTcbListen();

        if (tpl->lport == port)
        {
            VerifyOrExit(!aSockName.GetAddress().IsUnspecified());
            VerifyOrExit(!reinterpret_cast<Address *>(&tpl->laddr)->IsUnspecified());
            VerifyOrExit(memcmp(&tpl->laddr, &aSockName.mAddress, sizeof(tpl->laddr)) != 0);
        }
    }

    allowed = true;

exit:
    return allowed;
}

bool Tcp::AutoBind(const SockAddr &aPeer, SockAddr &aToBind, bool aBindAddress, bool aBindPort)
{
    bool success;

    if (aBindAddress)
    {
        const Address *source;

        source = Get<Ip6>().SelectSourceAddress(aPeer.GetAddress());
        VerifyOrExit(source != nullptr, success = false);
        aToBind.SetAddress(*source);
    }

    if (aBindPort)
    {
        /*
         * TODO: Use a less naive algorithm to allocate ephemeral ports. For
         * example, see RFC 6056.
         */

        for (uint16_t i = 0; i != kDynamicPortMax - kDynamicPortMin + 1; i++)
        {
            aToBind.SetPort(mEphemeralPort);

            if (mEphemeralPort == kDynamicPortMax)
            {
                mEphemeralPort = kDynamicPortMin;
            }
            else
            {
                mEphemeralPort++;
            }

            if (CanBind(aToBind))
            {
                ExitNow(success = true);
            }
        }

        ExitNow(success = false);
    }

    success = CanBind(aToBind);

exit:
    return success;
}

void Tcp::HandleTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();
    bool      pendingTimer;
    TimeMilli earliestPendingTimerExpiry;

    LogDebg("Main TCP timer expired");

    /*
     * The timer callbacks could potentially set/reset/cancel timers.
     * Importantly, Endpoint::SetTimer and Endpoint::CancelTimer do not call
     * this function to recompute the timer. If they did, we'd have a
     * re-entrancy problem, where the callbacks called in this function could
     * wind up re-entering this function in a nested call frame.
     *
     * In general, calling this function from Endpoint::SetTimer and
     * Endpoint::CancelTimer could be inefficient, since those functions are
     * called multiple times on each received TCP segment. If we want to
     * prevent the main timer from firing except when an actual TCP timer
     * expires, a better alternative is to reset the main timer in
     * HandleMessage, right before processing signals. That would achieve that
     * objective while avoiding re-entrancy issues altogether.
     */
restart:
    pendingTimer               = false;
    earliestPendingTimerExpiry = now.GetDistantFuture();

    for (Endpoint &endpoint : mEndpoints)
    {
        if (endpoint.FirePendingTimers(now, pendingTimer, earliestPendingTimerExpiry))
        {
            /*
             * If a non-OpenThread callback is called --- which, in practice,
             * happens if the connection times out and the user-defined
             * connection lost callback is called --- then we might have to
             * start over. The reason is that the user might deinitialize
             * endpoints, changing the structure of the linked list. For
             * example, if the user deinitializes both this endpoint and the
             * next one in the linked list, then we can't continue traversing
             * the linked list.
             */
            goto restart;
        }
    }

    if (pendingTimer)
    {
        /*
         * We need to use Timer::FireAtIfEarlier instead of timer::FireAt
         * because one of the earlier callbacks might have set TCP timers,
         * in which case `mTimer` would have been set to the earliest of those
         * timers.
         */
        mTimer.FireAtIfEarlier(earliestPendingTimerExpiry);
        LogDebg("Reset main TCP timer to %u ms", static_cast<unsigned int>(earliestPendingTimerExpiry - now));
    }
    else
    {
        LogDebg("Did not reset main TCP timer");
    }
}

void Tcp::ProcessCallbacks(void)
{
    for (Endpoint &endpoint : mEndpoints)
    {
        if (endpoint.FirePendingCallbacks())
        {
            mTasklet.Post();
            break;
        }
    }
}

} // namespace Ip6
} // namespace ot

/*
 * Implement TCPlp system stubs declared in tcplp.h.
 *
 * Because these functions have C linkage, it is important that only one
 * definition is given for each function name, regardless of the namespace it
 * in. For example, if we give two definitions of tcplp_sys_new_message, we
 * will get errors, even if they are in different namespaces. To avoid
 * confusion, I've put these functions outside of any namespace.
 */

using namespace ot;
using namespace ot::Ip6;

extern "C" {

otMessage *tcplp_sys_new_message(otInstance *aInstance)
{
    Instance &instance = AsCoreType(aInstance);
    Message  *message  = instance.Get<ot::Ip6::Ip6>().NewMessage(0);

    if (message)
    {
        message->SetLinkSecurityEnabled(true);
    }

    return message;
}

void tcplp_sys_free_message(otInstance *aInstance, otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aInstance);
    Message &message = AsCoreType(aMessage);
    message.Free();
}

void tcplp_sys_send_message(otInstance *aInstance, otMessage *aMessage, otMessageInfo *aMessageInfo)
{
    Instance    &instance = AsCoreType(aInstance);
    Message     &message  = AsCoreType(aMessage);
    MessageInfo &info     = AsCoreType(aMessageInfo);

    LogDebg("Sending TCP segment: payload_size = %d", static_cast<int>(message.GetLength()));

    IgnoreError(instance.Get<ot::Ip6::Ip6>().SendDatagram(message, info, kProtoTcp));
}

uint32_t tcplp_sys_get_ticks(void) { return TimerMilli::GetNow().GetValue(); }

uint32_t tcplp_sys_get_millis(void) { return TimerMilli::GetNow().GetValue(); }

void tcplp_sys_set_timer(struct tcpcb *aTcb, uint8_t aTimerFlag, uint32_t aDelay)
{
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aTcb);
    endpoint.SetTimer(aTimerFlag, aDelay);
}

void tcplp_sys_stop_timer(struct tcpcb *aTcb, uint8_t aTimerFlag)
{
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aTcb);
    endpoint.CancelTimer(aTimerFlag);
}

struct tcpcb *tcplp_sys_accept_ready(struct tcpcb_listen *aTcbListen, struct in6_addr *aAddr, uint16_t aPort)
{
    Tcp::Listener                &listener = Tcp::Listener::FromTcbListen(*aTcbListen);
    Tcp                          &tcp      = listener.Get<Tcp>();
    struct tcpcb                 *rv       = (struct tcpcb *)-1;
    otSockAddr                    addr;
    otTcpEndpoint                *endpointPtr;
    otTcpIncomingConnectionAction action;

    VerifyOrExit(listener.mAcceptReadyCallback != nullptr);

    memcpy(&addr.mAddress, aAddr, sizeof(addr.mAddress));
    addr.mPort = BigEndian::HostSwap16(aPort);
    action     = listener.mAcceptReadyCallback(&listener, &addr, &endpointPtr);

    VerifyOrExit(tcp.IsInitialized(listener) && !listener.IsClosed());

    switch (action)
    {
    case OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT:
    {
        Tcp::Endpoint &endpoint = AsCoreType(endpointPtr);

        /*
         * The documentation says that the user must initialize the
         * endpoint before passing it here, so we do a sanity check to make
         * sure the endpoint is initialized and closed. That check may not
         * be necessary, but we do it anyway.
         */
        VerifyOrExit(tcp.IsInitialized(endpoint) && endpoint.IsClosed());

        rv = &endpoint.GetTcb();

        break;
    }
    case OT_TCP_INCOMING_CONNECTION_ACTION_DEFER:
        rv = nullptr;
        break;
    case OT_TCP_INCOMING_CONNECTION_ACTION_REFUSE:
        rv = (struct tcpcb *)-1;
        break;
    }

exit:
    return rv;
}

bool tcplp_sys_accepted_connection(struct tcpcb_listen *aTcbListen,
                                   struct tcpcb        *aAccepted,
                                   struct in6_addr     *aAddr,
                                   uint16_t             aPort)
{
    Tcp::Listener &listener = Tcp::Listener::FromTcbListen(*aTcbListen);
    Tcp::Endpoint &endpoint = Tcp::Endpoint::FromTcb(*aAccepted);
    Tcp           &tcp      = endpoint.Get<Tcp>();
    bool           accepted = true;

    if (listener.mAcceptDoneCallback != nullptr)
    {
        otSockAddr addr;

        memcpy(&addr.mAddress, aAddr, sizeof(addr.mAddress));
        addr.mPort = BigEndian::HostSwap16(aPort);
        listener.mAcceptDoneCallback(&listener, &endpoint, &addr);

        if (!tcp.IsInitialized(endpoint) || endpoint.IsClosed())
        {
            accepted = false;
        }
    }

    return accepted;
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

    LogDebg("%s", buffer);
}

void tcplp_sys_panic(const char *aFormat, ...)
{
    char    buffer[128];
    va_list args;
    va_start(args, aFormat);
    vsnprintf(buffer, sizeof(buffer), aFormat, args);
    va_end(args);

    LogCrit("%s", buffer);

    OT_ASSERT(false);
}

bool tcplp_sys_autobind(otInstance       *aInstance,
                        const otSockAddr *aPeer,
                        otSockAddr       *aToBind,
                        bool              aBindAddress,
                        bool              aBindPort)
{
    Instance &instance = AsCoreType(aInstance);

    return instance.Get<Tcp>().AutoBind(*static_cast<const SockAddr *>(aPeer), *static_cast<SockAddr *>(aToBind),
                                        aBindAddress, aBindPort);
}

uint32_t tcplp_sys_generate_isn()
{
    uint32_t isn;
    IgnoreError(Random::Crypto::Fill(isn));
    return isn;
}

uint16_t tcplp_sys_hostswap16(uint16_t aHostPort) { return BigEndian::HostSwap16(aHostPort); }

uint32_t tcplp_sys_hostswap32(uint32_t aHostPort) { return BigEndian::HostSwap32(aHostPort); }
}

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
