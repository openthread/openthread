/*-
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tcp_subr.c	8.2 (Berkeley) 5/24/95
 */

#include <string.h>

#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "tcp_var.h"

#include "tcp_const.h"
#include <openthread/ip6.h>
#include <openthread/message.h>

enum tcp_timewait_consts {
    V_nolocaltimewait = 0 // For now, to keep things simple
};

static void
tcp_tw_2msl_reset(struct tcpcb* tp, int rearm)
{

//	INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
//	INP_WLOCK_ASSERT(tw->tw_inpcb);

//	TW_WLOCK(V_tw_lock);
//	if (rearm)
//		TAILQ_REMOVE(&V_twq_2msl, tw, tw_2msl);
//	/*tw*/tp->tw_time = get_ticks()/*ticks*/ + 2 * tcp_msl;
	tcp_timer_activate(tp, TT_2MSL, 2 * tcp_msl);
//	TAILQ_INSERT_TAIL(&V_twq_2msl, tw, tw_2msl);
//	TW_WUNLOCK(V_tw_lock);
}

static int
tcp_twrespond(struct tcpcb* tp, int flags)
{
	/* Essentially all the code needs to be discarded because I need to send packets the TinyOS way.
	   There are some parts that I copied; I didn't want to comment out everything except the few
	   lines I needed since I felt that this would be cleaner.

       Update: I just made inline updates for the RIOT OS version. */
	//struct ip6_packet* msg;
	//struct ip6_hdr* ip6;
	struct tcphdr* nth;
	//struct ip_iovec* iov;
	struct tcpopt to;
	uint32_t optlen = 0;
	uint8_t opt[TCP_MAXOLEN];
	//int alen;
	//char* bufreal;
	int win = 0;
	//char* buf;

	to.to_flags = 0;

	/*
	 * Send a timestamp and echo-reply if both our side and our peer
	 * have sent timestamps in our SYN's and this is not a RST.
	 */
	if (/*tw->t_recent*/(tp->t_flags & TF_RCVD_TSTMP) && flags == TH_ACK) {
		to.to_flags |= TOF_TS;
		to.to_tsval = tcp_ts_getticks() + /*tw->ts_offset*/tp->ts_offset;
		to.to_tsecr = /*tw->t_recent*/tp->ts_recent;
	}
	optlen = tcp_addoptions(&to, opt);

    otMessage* message = tcplp_sys_new_message(tp->instance);
    if (message == NULL) {
        return 0; // drop the message
    }
    if (otMessageSetLength(message, sizeof(struct tcphdr) + optlen) != OT_ERROR_NONE) {
        tcplp_sys_free_message(tp->instance, message);
        return 0; // drop the message
    }

    /*gnrc_pktsnip_t* tcpsnip = gnrc_pktbuf_add(NULL, NULL, sizeof(struct tcphdr) + optlen, GNRC_NETTYPE_TCP);
    if (tcpsnip == NULL) {
        return 0; // drop the message;
    }
    gnrc_pktsnip_t* ip6snip = gnrc_pktbuf_add(tcpsnip, NULL, sizeof(struct ip6_hdr), GNRC_NETTYPE_IPV6);
    if (ip6snip == NULL) {
        gnrc_pktbuf_release(tcpsnip);
        return 0; // drop the message;
    }*/

    char outbuf[sizeof(struct tcphdr) + optlen];
    nth = (struct tcphdr*) &outbuf[0];
    otMessageInfo ip6info;
    memset(&ip6info, 0x00, sizeof(ip6info));
    //ip6 = ip6snip->data;

    #if 0
	alen = sizeof(struct ip6_packet) + sizeof(struct tcphdr) + optlen + sizeof(struct ip_iovec);
	bufreal = ip_malloc(alen + 3);
	if (bufreal == NULL) {
		return 0; // drop the message
	}
    #endif
	if (tp != NULL) {
		if (!(flags & TH_RST)) {
			win = cbuf_free_space(&tp->recvbuf);
			if (win > (long)TCP_MAXWIN << tp->rcv_scale)
				win = (long)TCP_MAXWIN << tp->rcv_scale;
		}
	}
    #if 0
	buf = (char*) (((uint32_t) (bufreal + 3)) & 0xFFFFFFFCu);
	memset(buf, 0, alen); // for safe measure
	msg = (struct ip6_packet*) buf;
  	iov = (struct ip_iovec*) (buf + alen - sizeof(struct ip_iovec));
  	iov->iov_next = NULL;
	iov->iov_len = sizeof(struct tcphdr) + optlen;
	iov->iov_base = (void*) (msg + 1);
	msg->ip6_data = iov;
	ip6 = &msg->ip6_hdr;
    #endif
    //ip6->ip6_vfc = 0x60;
	//ip6->ip6_nxt = IANA_TCP;
	//ip6->ip6_plen = htons(sizeof(struct tcphdr) + optlen);
    //memset(&ip6->ip6_src, 0x00, sizeof(ip6->ip6_src));
	//ip6->ip6_dst = tp->faddr;

    memcpy(&ip6info.mSockAddr, &tp->laddr, sizeof(ip6info.mSockAddr));
    memcpy(&ip6info.mPeerAddr, &tp->faddr, sizeof(ip6info.mPeerAddr));
    ip6info.mVersionClassFlow = 0x60000000;
	nth->th_sport = tp->lport;
	nth->th_dport = tp->fport;
	nth->th_seq = htonl(tp->snd_nxt);
	nth->th_ack = htonl(tp->rcv_nxt);
	nth->th_x2 = 0;
	nth->th_off = (sizeof(struct tcphdr) + optlen) >> 2;
	nth->th_flags = flags;
	nth->th_win = htons(tp->tw_last_win);
	nth->th_urp = 0;
    nth->th_sum = 0;

	memcpy(nth + 1, opt, optlen);
    otMessageWrite(message, 0, outbuf, sizeof(struct tcphdr) + optlen);
	tcplp_sys_send_message(tp->instance, message, &ip6info);

	return 0;
#if 0
//	struct inpcb *inp = tw->tw_inpcb;
#if defined(INET6) || defined(INET)
	struct tcphdr *th = NULL;
#endif
	struct mbuf *m;
#ifdef INET
	struct ip *ip = NULL;
#endif
	uint32_t hdrlen, optlen;
	int error = 0;			/* Keep compiler happy */
	struct tcpopt to;
#ifdef INET6
	struct ip6_hdr *ip6 = NULL;
	int isipv6 = inp->inp_inc.inc_flags & INC_ISIPV6;
#endif
	hdrlen = 0;                     /* Keep compiler happy */

	INP_WLOCK_ASSERT(inp);

	m = m_gethdr(M_NOWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);
	m->m_data += max_linkhdr;

#ifdef MAC
	mac_inpcb_create_mbuf(inp, m);
#endif

#ifdef INET6
	if (isipv6) {
		hdrlen = sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
		ip6 = mtod(m, struct ip6_hdr *);
		th = (struct tcphdr *)(ip6 + 1);
		tcpip_fillheaders(inp, ip6, th);
	}
#endif
#if defined(INET6) && defined(INET)
	else
#endif
#ifdef INET
	{
		hdrlen = sizeof(struct tcpiphdr);
		ip = mtod(m, struct ip *);
		th = (struct tcphdr *)(ip + 1);
		tcpip_fillheaders(inp, ip, th);
	}
#endif
	to.to_flags = 0;

	/*
	 * Send a timestamp and echo-reply if both our side and our peer
	 * have sent timestamps in our SYN's and this is not a RST.
	 */
	if (tw->t_recent && flags == TH_ACK) {
		to.to_flags |= TOF_TS;
		to.to_tsval = tcp_ts_getticks() + tw->ts_offset;
		to.to_tsecr = tw->t_recent;
	}
	optlen = tcp_addoptions(&to, (uint8_t *)(th + 1));

	m->m_len = hdrlen + optlen;
	m->m_pkthdr.len = m->m_len;

	KASSERT(max_linkhdr + m->m_len <= MHLEN, ("tcptw: mbuf too small"));

	th->th_seq = htonl(tw->snd_nxt);
	th->th_ack = htonl(tw->rcv_nxt);
	th->th_off = (sizeof(struct tcphdr) + optlen) >> 2;
	th->th_flags = flags;
	th->th_win = htons(tw->last_win);

	m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
#ifdef INET6
	if (isipv6) {
		m->m_pkthdr.csum_flags = CSUM_TCP_IPV6;
		th->th_sum = in6_cksum_pseudo(ip6,
		    sizeof(struct tcphdr) + optlen, IPPROTO_TCP, 0);
		ip6->ip6_hlim = in6_selecthlim(inp, NULL);
		error = ip6_output(m, inp->in6p_outputopts, NULL,
		    (tw->tw_so_options & SO_DONTROUTE), NULL, NULL, inp);
	}
#endif
#if defined(INET6) && defined(INET)
	else
#endif
#ifdef INET
	{
		m->m_pkthdr.csum_flags = CSUM_TCP;
		th->th_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr,
		    htons(sizeof(struct tcphdr) + optlen + IPPROTO_TCP));
		ip->ip_len = htons(m->m_pkthdr.len);
		if (V_path_mtu_discovery)
			ip->ip_off |= htons(IP_DF);
		error = ip_output(m, inp->inp_options, NULL,
		    ((tw->tw_so_options & SO_DONTROUTE) ? IP_ROUTETOIF : 0),
		    NULL, inp);
	}
#endif
	if (flags & TH_ACK)
		TCPSTAT_INC(tcps_sndacks);
	else
		TCPSTAT_INC(tcps_sndctrl);
	TCPSTAT_INC(tcps_sndtotal);
	return (error);
#endif
}

/*
 * Move a TCP connection into TIME_WAIT state.
 *    tcbinfo is locked.
 *    inp is locked, and is unlocked before returning.
 */
void
tcp_twstart(struct tcpcb *tp)
{
#if 0
	struct tcptw *tw;
	struct inpcb *inp = tp->t_inpcb;
#endif
	int acknow;
#if 0
	struct socket *so;
#ifdef INET6
	int isipv6 = inp->inp_inc.inc_flags & INC_ISIPV6;
#endif

	INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
	INP_WLOCK_ASSERT(inp);
#endif
//	if (V_nolocaltimewait) {
//		int error = 0;
//#ifdef INET6
//		if (isipv6)
//			error = in6_localaddr(&inp->in6p_faddr);
//#endif
//#if defined(INET6) && defined(INET)
//		else
//#endif
//#ifdef INET
//			error = in_localip(inp->inp_faddr);
//#endif
//		if (error) {
//			tp = tcp_close(tp);
//			if (tp != NULL)
//				INP_WUNLOCK(inp);
//			return;
//		}
//	}

	/*
	 * For use only by DTrace.  We do not reference the state
	 * after this point so modifying it in place is not a problem.
	 * Sam: Not true anymore. I use this state, since I don't associate every struct tcpcb with a struct inpcb.
	 */
	tcp_state_change(tp, TCPS_TIME_WAIT);

#if 0 //RATHER THAN CLOSING THE SOCKET AND KEEPING TRACK OF TIMEWAIT USING THE struct tcptw, I'M JUST GOING TO KEEP AROUND THE struct tcpcb
	tw = uma_zalloc(V_tcptw_zone, M_NOWAIT);
	if (tw == NULL) {
		/*
		 * Reached limit on total number of TIMEWAIT connections
		 * allowed. Remove a connection from TIMEWAIT queue in LRU
		 * fashion to make room for this connection.
		 *
		 * XXX:  Check if it possible to always have enough room
		 * in advance based on guarantees provided by uma_zalloc().
		 */
		tw = tcp_tw_2msl_scan(1);
		if (tw == NULL) {
			tp = tcp_close(tp);
			if (tp != NULL)
				INP_WUNLOCK(inp);
			return;
		}
	}
	/*
	 * The tcptw will hold a reference on its inpcb until tcp_twclose
	 * is called
	 */
	tw->tw_inpcb = inp;
	in_pcbref(inp);	/* Reference from tw */
#endif
	/*
	 * Recover last window size sent.
	 */
	if (SEQ_GT(tp->rcv_adv, tp->rcv_nxt))
		tp->tw_last_win = (tp->rcv_adv - tp->rcv_nxt) >> tp->rcv_scale;
	else
		tp->tw_last_win = 0;

	/*
	 * Set t_recent if timestamps are used on the connection.
	 */
	if ((tp->t_flags & (TF_REQ_TSTMP|TF_RCVD_TSTMP|TF_NOOPT)) ==
	    (TF_REQ_TSTMP|TF_RCVD_TSTMP)) {
//		tw->t_recent = tp->ts_recent;
//		tw->ts_offset = tp->ts_offset;
	} else {
		tp->/*t_recent*/ts_recent = 0;
		tp->ts_offset = 0;
	}

//	tw->snd_nxt = tp->snd_nxt;
//	tw->rcv_nxt = tp->rcv_nxt;
//	tw->iss     = tp->iss;
//	tw->irs     = tp->irs;
//	tw->t_starttime = tp->t_starttime;
	/*tw*/tp->tw_time = 0;

/* XXX
 * If this code will
 * be used for fin-wait-2 state also, then we may need
 * a ts_recent from the last segment.
 */
	acknow = tp->t_flags & TF_ACKNOW;

	/*
	 * First, discard tcpcb state, which includes stopping its timers and
	 * freeing it.  tcp_discardcb() used to also release the inpcb, but
	 * that work is now done in the caller.
	 *
	 * Note: soisdisconnected() call used to be made in tcp_discardcb(),
	 * and might not be needed here any longer.
	 */
	tcp_cancel_timers(tp); /*tcp_discardcb(tp);*/ // The discardcb() call needs to be moved to tcp_close()
//	so = inp->inp_socket;
//	soisdisconnected(so);
//	tw->tw_cred = crhold(so->so_cred);
//	SOCK_LOCK(so);
//	tw->tw_so_options = so->so_options;
//	SOCK_UNLOCK(so);
	if (acknow)
		tcp_twrespond(/*tw*/tp, TH_ACK);
//	inp->inp_ppcb = tw;
//	inp->inp_flags |= INP_TIMEWAIT;
	tcp_tw_2msl_reset(/*tw*/tp, 0);
#if 0
	/*
	 * If the inpcb owns the sole reference to the socket, then we can
	 * detach and free the socket as it is not needed in time wait.
	 */
	if (inp->inp_flags & INP_SOCKREF) {
		KASSERT(so->so_state & SS_PROTOREF,
		    ("tcp_twstart: !SS_PROTOREF"));
		inp->inp_flags &= ~INP_SOCKREF;
		INP_WUNLOCK(inp);
		ACCEPT_LOCK();
		SOCK_LOCK(so);
		so->so_state &= ~SS_PROTOREF;
		sofree(so);
	} else
		INP_WUNLOCK(inp);
#endif
}

/*
 * Returns 1 if the TIME_WAIT state was killed and we should start over,
 * looking for a pcb in the listen state.  Returns 0 otherwise.
 */
int
tcp_twcheck(struct tcpcb* tp,/*struct inpcb *inp, struct tcpopt *to __unused, */struct tcphdr *th,
    /*struct mbuf *m, */int tlen)
{
//	struct tcptw *tw;
	int thflags;
	tcp_seq seq;

//	INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
//	INP_WLOCK_ASSERT(inp);

	/*
	 * XXXRW: Time wait state for inpcb has been recycled, but inpcb is
	 * still present.  This is undesirable, but temporarily necessary
	 * until we work out how to handle inpcb's who's timewait state has
	 * been removed.
	 */
//	tw = intotw(inp);
//	if (tw == NULL)
//		goto drop;

	thflags = th->th_flags;

	/*
	 * NOTE: for FIN_WAIT_2 (to be added later),
	 * must validate sequence number before accepting RST
	 */

	/*
	 * If the segment contains RST:
	 *	Drop the segment - see Stevens, vol. 2, p. 964 and
	 *      RFC 1337.
	 */
	if (thflags & TH_RST)
		goto drop;

#if 0
/* PAWS not needed at the moment */
	/*
	 * RFC 1323 PAWS: If we have a timestamp reply on this segment
	 * and it's less than ts_recent, drop it.
	 */
	if ((to.to_flags & TOF_TS) != 0 && tp->ts_recent &&
	    TSTMP_LT(to.to_tsval, tp->ts_recent)) {
		if ((thflags & TH_ACK) == 0)
			goto drop;
		goto ack;
	}
	/*
	 * ts_recent is never updated because we never accept new segments.
	 */
#endif

	/*
	 * If a new connection request is received
	 * while in TIME_WAIT, drop the old connection
	 * and start over if the sequence numbers
	 * are above the previous ones.
	 */
	if ((thflags & TH_SYN) && SEQ_GT(th->th_seq, /*tw*/tp->rcv_nxt)) {
		//tcp_twclose(tw, 0);
		tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		return (1);
	}

	/*
	 * Drop the segment if it does not contain an ACK.
	 */
	if ((thflags & TH_ACK) == 0)
		goto drop;

	/*
	 * Reset the 2MSL timer if this is a duplicate FIN.
	 */
	if (thflags & TH_FIN) {
		seq = th->th_seq + tlen + (thflags & TH_SYN ? 1 : 0);
		if (seq + 1 == /*tw*/tp->rcv_nxt)
			tcp_tw_2msl_reset(/*tw*/tp, 1);
	}

	/*
	 * Acknowledge the segment if it has data or is not a duplicate ACK.
	 */
	if (thflags != TH_ACK || tlen != 0 ||
	    th->th_seq != /*tw*/tp->rcv_nxt || th->th_ack != /*tw*/tp->snd_nxt)
		tcp_twrespond(/*tw*/tp, TH_ACK);
drop:
//	INP_WUNLOCK(inp);
//	m_freem(m);
	return (0);
}
