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

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "../tcplp.h"
#include "ip.h"
#include "ip6.h"
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_var.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "sys/queue.h"
#include "../lib/bitmap.h"
#include "../lib/cbuf.h"
#include "cc.h"

#include "tcp_const.h"

/*
 * samkumar: This is rewritten to have the host network stack to generate the
 * ISN with appropriate randomness.
 */
tcp_seq tcp_new_isn(struct tcpcb* tp) {
	return (uint32_t) tcplp_sys_generate_isn();
}

/*
 * samkumar: There used to be a function, void tcp_init(void), that would
 * initialize global state for TCP, including a hash table to store TCBs,
 * allocating memory zones for sockets, and setting global configurable state.
 * None of that is needed for TCPlp: TCB allocation and matching is done by
 * the host system and global configurable state is removed with hardcoded
 * values in order to save memory, for example. Thus, I've removed the function
 * entirely.
 */

/*
 * A subroutine which makes it easy to track TCP state changes with DTrace.
 * This function shouldn't be called for t_state initializations that don't
 * correspond to actual TCP state transitions.
 */
void
tcp_state_change(struct tcpcb *tp, int newstate)
{
#if 0
#if defined(KDTRACE_HOOKS)
	int pstate = tp->t_state;
#endif
#endif
	tcplp_sys_log("Socket %p: %s --> %s", tp, tcpstates[tp->t_state], tcpstates[newstate]);
	tp->t_state = newstate;

	// samkumar: may need to do other actions too, so call into the host
	tcplp_sys_on_state_change(tp, newstate);
#if 0
	TCP_PROBE6(state__change, NULL, tp, NULL, tp, NULL, pstate);
#endif
}

 /* samkumar: Based on tcp_newtcb in tcp_subr.c, and tcp_usr_attach in tcp_usrreq.c. */
__attribute__((used)) void initialize_tcb(struct tcpcb* tp) {
	uint32_t ticks = tcplp_sys_get_ticks();

	/* samkumar: Clear all fields starting laddr; rest are initialized by the host. */
	memset(((uint8_t*) tp) + offsetof(struct tcpcb, laddr), 0x00, sizeof(struct tcpcb) - offsetof(struct tcpcb, laddr));
	tp->reass_fin_index = -1;

	/*
	 * samkumar: Only New Reno congestion control is implemented at the moment,
	 * so there's no need to record the congestion control algorithm used for
	 * each TCB.
	 */
	// CC_ALGO(tp) = CC_DEFAULT();
	// tp->ccv->type = IPPROTO_TCP;
	tp->ccv->ccvc.tcp = tp;

	/*
	 * samkumar: The original code used to choose a different constant
	 * depending on whether it's an IPv4 or IPv6 connection. In TCPlp, we
	 * unconditionally choose the IPv6 branch.
	 */
	tp->t_maxseg = tp->t_maxopd =
//#ifdef INET6
		/*isipv6 ? */V_tcp_v6mssdflt /*:*/
//#endif /* INET6 */
		/*V_tcp_mssdflt*/;

	if (V_tcp_do_rfc1323)
		tp->t_flags = (TF_REQ_SCALE|TF_REQ_TSTMP);
	if (V_tcp_do_sack)
		tp->t_flags |= TF_SACK_PERMIT;
	TAILQ_INIT(&tp->snd_holes);

	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 4 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = ((TCPTV_RTOBASE - TCPTV_SRTTBASE) << TCP_RTTVAR_SHIFT) / 4;
	tp->t_rttmin = TCPTV_MIN < 1 ? 1 : TCPTV_MIN; /* samkumar: used to be tcp_rexmit_min, which was set in tcp_init */
	tp->t_rxtcur = TCPTV_RTOBASE;
	tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->t_rcvtime = ticks;

	/* samkumar: Taken from tcp_usr_attach in tcp_usrreq.c. */
	tp->t_state = TCP6S_CLOSED;

	/* samkumar: Added to initialize the per-TCP sackhole pool. */
	tcp_sack_init(tp);
}


/*
 * samkumar: Most of this function was no longer needed. It did things like
 * reference-counting for the TCB, updating host cache stats for better
 * starting values of, e.g., ssthresh, for new connections, freeing resources
 * for TCP offloading, etc. There's no host cache in TCPlp and the host system
 * is responsible for managing TCB memory, so much of this isn't needed. I just
 * kept (and adpated) the few parts of the function that appeared to be needed
 * for TCPlp.
 */
void
tcp_discardcb(struct tcpcb *tp)
{
	tcp_cancel_timers(tp);

	/* Allow the CC algorithm to clean up after itself. */
	if (CC_ALGO(tp)->cb_destroy != NULL)
		CC_ALGO(tp)->cb_destroy(tp->ccv);

	tcp_free_sackholes(tp);
}


 /*
 * Attempt to close a TCP control block, marking it as dropped, and freeing
 * the socket if we hold the only reference.
 */
/*
 * samkumar: Most of this function has to do with dropping the reference to
 * the inpcb, freeing resources at the socket layer and marking it as
 * disconnected, and miscellaneous cleanup. I've rewritten this to do what is
 * needed for TCP.
 */
struct tcpcb *
tcp_close(struct tcpcb *tp)
{
	tcp_state_change(tp, TCP6S_CLOSED); // for the print statement
	tcp_discardcb(tp);
	// Don't reset the TCB by calling initialize_tcb, since that overwrites the buffer contents.
	return tp;
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Allocates an mbuf and fills in a skeletal tcp/ip header.  The only
 * use for this function is in keepalives, which use tcp_respond.
 */
/* samkumar: I changed the signature of this function. Instead of allocating
 * the struct tcptemp using malloc, populating it, and then returning it, I
 * have the caller allocate it. This function merely populates it now.
 */
void
tcpip_maketemplate(struct tcpcb* tp, struct tcptemp* t)
{
	tcpip_fillheaders(tp, (void *)&t->tt_ipgen, (void *)&t->tt_t);
}

/*
 * Fill in the IP and TCP headers for an outgoing packet, given the tcpcb.
 * tcp_template used to store this data in mbufs, but we now recopy it out
 * of the tcpcb each time to conserve mbufs.
 */
/*
 * samkumar: This has a different signature from the original function in
 * tcp_subr.c. In particular, IP header information is filled into an
 * otMessageInfo rather than into a struct representing the on-wire header
 * format. Additionally, I have changed it to always assume IPv6; I removed the
 * code for IPv4.
 */
void
tcpip_fillheaders(struct tcpcb* tp, otMessageInfo* ip_ptr, void *tcp_ptr)
{
	struct tcphdr *th = (struct tcphdr *)tcp_ptr;

	/* Fill in the IP header */

    /* samkumar: The old IPv6 code, for reference. */
	// ip6 = (struct ip6_hdr *)ip_ptr;
	// ip6->ip6_flow = (ip6->ip6_flow & ~IPV6_FLOWINFO_MASK) |
	//     (inp->inp_flow & IPV6_FLOWINFO_MASK);
	// ip6->ip6_vfc = (ip6->ip6_vfc & ~IPV6_VERSION_MASK) |
	//     (IPV6_VERSION & IPV6_VERSION_MASK);
	// ip6->ip6_nxt = IPPROTO_TCP;
	// ip6->ip6_plen = htons(sizeof(struct tcphdr));
	// ip6->ip6_src = inp->in6p_laddr;
	// ip6->ip6_dst = inp->in6p_faddr;

	memset(ip_ptr, 0x00, sizeof(otMessageInfo));
	memcpy(&ip_ptr->mSockAddr, &tp->laddr, sizeof(ip_ptr->mSockAddr));
	memcpy(&ip_ptr->mPeerAddr, &tp->faddr, sizeof(ip_ptr->mPeerAddr));

	/* Fill in the TCP header */
	/* samkumar: I kept the old code commented out, for reference. */
	//th->th_sport = inp->inp_lport;
	//th->th_dport = inp->inp_fport;
	th->th_sport = tp->lport;
	th->th_dport = tp->fport;
	th->th_seq = 0;
	th->th_ack = 0;
	// th->th_x2 = 0;
	// th->th_off = 5;
	th->th_off_x2 = (5 << TH_OFF_SHIFT);
	th->th_flags = 0;
	th->th_win = 0;
	th->th_urp = 0;
	th->th_sum = 0;		/* in_pseudo() is called later for ipv4 */
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If m == NULL, then we make a copy
 * of the tcpiphdr at th and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection.  If flags are given then we send
 * a message back to the TCP which originated the segment th,
 * and discard the mbuf containing it and any other attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 *
 * NOTE: If m != NULL, then th must point to *inside* the mbuf.
 */
/* samkumar: Original signature was
void
tcp_respond(struct tcpcb *tp, void *ipgen, struct tcphdr *th, struct mbuf *m,
	tcp_seq ack, tcp_seq seq, int flags)
*/
/*
 * samkumar: Essentially all of the code had to be discarded/rewritten since I
 * have to send out packets by allocating buffers from the host system,
 * populating them, and passing them back to the host system to send out. I
 * simplified the code by only using the logic that was fully necessary,
 * eliminating the code for IPv4 packets and keeping only the code for IPv6
 * packets. I also removed all of the mbuf logic, instead using the logic for
 * using the host system's buffering (in particular, the code to reuse the
 * provided mbuf is no longer there).
 */
void
tcp_respond(struct tcpcb *tp, otInstance* instance, struct ip6_hdr* ip6gen, struct tcphdr *thgen,
	tcp_seq ack, tcp_seq seq, int flags)
{
	otMessage* message = tcplp_sys_new_message(instance);
	if (message == NULL) {
		return;
	}
	if (otMessageSetLength(message, sizeof(struct tcphdr)) != OT_ERROR_NONE) {
		tcplp_sys_free_message(instance, message);
		return;
	}

	struct tcphdr th;
	struct tcphdr* nth = &th;
	otMessageInfo ip6info;
	int win = 0;
	if (tp != NULL) {
		if (!(flags & TH_RST)) {
			win = cbuf_free_space(&tp->recvbuf);
			if (win > (long)TCP_MAXWIN << tp->rcv_scale)
				win = (long)TCP_MAXWIN << tp->rcv_scale;
		}
	}
	memset(&ip6info, 0x00, sizeof(otMessageInfo));
	memcpy(&ip6info.mSockAddr, &ip6gen->ip6_dst, sizeof(ip6info.mSockAddr));
	memcpy(&ip6info.mPeerAddr, &ip6gen->ip6_src, sizeof(ip6info.mPeerAddr));
	nth->th_sport = thgen->th_dport;
	nth->th_dport = thgen->th_sport;
	nth->th_seq = htonl(seq);
	nth->th_ack = htonl(ack);
	/* samkumar: original code for setting th_x2 and th_off, for reference. */
	// nth->th_x2 = 0;
	// nth->th_off = (sizeof (struct tcphdr) + optlen) >> 2;
	nth->th_off_x2 = (sizeof(struct tcphdr) >> 2) << TH_OFF_SHIFT;
	nth->th_flags = flags;
	if (tp != NULL)
		nth->th_win = htons((uint16_t) (win >> tp->rcv_scale));
	else
		nth->th_win = htons((uint16_t)win);
	nth->th_urp = 0;
	nth->th_sum = 0;

	otMessageWrite(message, 0, &th, sizeof(struct tcphdr));

	tcplp_sys_send_message(instance, message, &ip6info);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
/*
 * samkumar: I changed the parameter "errno" to "errnum" since it caused
 * problems during compilation. I also the code for asserting locks,
 * incermenting stats, and managing the sockets layer.
 */
struct tcpcb *
tcp_drop(struct tcpcb *tp, int errnum)
{
	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tcp_state_change(tp, TCPS_CLOSED);
		(void) tcp_output(tp);
	}
	if (errnum == ETIMEDOUT && tp->t_softerror)
		errnum = tp->t_softerror;
	tp = tcp_close(tp);
	tcplp_sys_connection_lost(tp, errnum);
	return tp;
}

/*
 * Look-up the routing entry to the peer of this inpcb.  If no route
 * is found and it cannot be allocated, then return 0.  This routine
 * is called by TCP routines that access the rmx structure and by
 * tcp_mss_update to get the peer/interface MTU.
 */
/*
 * samkumar: In TCPlp, we don't bother with keeping track of the MTU for each
 * route. The MSS we choose for the 6LoWPAN/802.15.4 network is probably the
 * bottleneck, so we just use that. (I also removed the struct in_conninfo *
 * that was formerly the first argument).
 */
uint64_t
tcp_maxmtu6(struct tcpcb* tp, struct tcp_ifcap *cap)
{
	uint64_t maxmtu = 0;

	KASSERT (tp != NULL, ("tcp_maxmtu6 with NULL tcpcb pointer"));
	if (!IN6_IS_ADDR_UNSPECIFIED(&tp->faddr)) {
		maxmtu = FRAMES_PER_SEG * FRAMECAP_6LOWPAN;
	}

	return (maxmtu);
}
