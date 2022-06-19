/*-
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2007-2008,2010
 *	Swinburne University of Technology, Melbourne, Australia.
 * Copyright (c) 2009-2010 Lawrence Stewart <lstewart@freebsd.org>
 * Copyright (c) 2010 The FreeBSD Foundation
 * Copyright (c) 2010-2011 Juniper Networks, Inc.
 * All rights reserved.
 *
 * Portions of this software were developed at the Centre for Advanced Internet
 * Architectures, Swinburne University of Technology, by Lawrence Stewart,
 * James Healy and David Hayes, made possible in part by a grant from the Cisco
 * University Research Program Fund at Community Foundation Silicon Valley.
 *
 * Portions of this software were developed at the Centre for Advanced
 * Internet Architectures, Swinburne University of Technology, Melbourne,
 * Australia by David Hayes under sponsorship from the FreeBSD Foundation.
 *
 * Portions of this software were developed by Robert N. M. Watson under
 * contract to Juniper Networks, Inc.
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
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 */


/*
 * Determine a reasonable value for maxseg size.
 * If the route is known, check route for mtu.
 * If none, use an mss that can be handled on the outgoing interface
 * without forcing IP to fragment.  If no route is found, route has no mtu,
 * or the destination isn't local, use a default, hopefully conservative
 * size (usually 512 or the default IP max size, but no more than the mtu
 * of the interface), as we can't discover anything about intervening
 * gateways or networks.  We also initialize the congestion/slow start
 * window to be a single segment if the destination isn't local.
 * While looking at the routing entry, we also initialize other path-dependent
 * parameters from pre-set or cached values in the routing entry.
 *
 * Also take into account the space needed for options that we
 * send regularly.  Make maxseg shorter by that amount to assure
 * that we can send maxseg amount of data even when the options
 * are present.  Store the upper limit of the length of options plus
 * data in maxopd.
 *
 * NOTE that this routine is only called when we process an incoming
 * segment, or an ICMP need fragmentation datagram. Outgoing SYN/ACK MSS
 * settings are handled in tcp_mssopt().
 */

#include <errno.h>
#include <string.h>
#include <strings.h>

#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "tcp_var.h"
#include "../lib/bitmap.h"
#include "../lib/cbuf.h"
#include "icmp_var.h"
#include "ip.h"
#include "ip6.h"
#include "sys/queue.h"

#include "tcp_const.h"

/* samkumar: Copied from in.h */
#define IPPROTO_DONE 267

/* samkumar: Copied from sys/libkern.h */
static int imax(int a, int b) { return (a > b ? a : b); }
static int imin(int a, int b) { return (a < b ? a : b); }

static int min(int a, int b) { return imin(a, b); }

static void	 tcp_dooptions(struct tcpopt *, uint8_t *, int, int);
static void
tcp_do_segment(struct ip6_hdr* ip6, struct tcphdr *th, otMessage* msg,
    struct tcpcb *tp, int drop_hdrlen, int tlen, uint8_t iptos,
    struct tcplp_signals* sig);
static void	 tcp_xmit_timer(struct tcpcb *, int);
void tcp_hc_get(/*struct in_conninfo *inc*/ struct tcpcb* tp, struct hc_metrics_lite *hc_metrics_lite);
static void	 tcp_newreno_partial_ack(struct tcpcb *, struct tcphdr *);

/*
 * CC wrapper hook functions
 */
static inline void
cc_ack_received(struct tcpcb *tp, struct tcphdr *th, uint16_t type)
{
	tp->ccv->bytes_this_ack = BYTES_THIS_ACK(tp, th);
	if (tp->snd_cwnd <= tp->snd_wnd)
		tp->ccv->flags |= CCF_CWND_LIMITED;
	else
		tp->ccv->flags &= ~CCF_CWND_LIMITED;

	if (type == CC_ACK) {
		if (tp->snd_cwnd > tp->snd_ssthresh) {
			tp->t_bytes_acked += min(tp->ccv->bytes_this_ack,
			     V_tcp_abc_l_var * tp->t_maxseg);
			if (tp->t_bytes_acked >= tp->snd_cwnd) {
				tp->t_bytes_acked -= tp->snd_cwnd;
				tp->ccv->flags |= CCF_ABC_SENTAWND;
			}
		} else {
				tp->ccv->flags &= ~CCF_ABC_SENTAWND;
				tp->t_bytes_acked = 0;
		}
	}

	if (CC_ALGO(tp)->ack_received != NULL) {
		/* XXXLAS: Find a way to live without this */
		tp->ccv->curack = th->th_ack;
		CC_ALGO(tp)->ack_received(tp->ccv, type);
	}
}

static inline void
cc_conn_init(struct tcpcb *tp)
{
	struct hc_metrics_lite metrics;
	int rtt;

	/*
	 * samkumar: remove locks, inpcb, and stats.
	 */

	/* samkumar: Used to take &inp->inp_inc as an argument. */
	tcp_hc_get(tp, &metrics);

	if (tp->t_srtt == 0 && (rtt = metrics.rmx_rtt)) {
		tp->t_srtt = rtt;
		tp->t_rttbest = tp->t_srtt + TCP_RTT_SCALE;
		if (metrics.rmx_rttvar) {
			tp->t_rttvar = metrics.rmx_rttvar;
		} else {
			/* default variation is +- 1 rtt */
			tp->t_rttvar =
			    tp->t_srtt * TCP_RTTVAR_SCALE / TCP_RTT_SCALE;
		}
		TCPT_RANGESET(tp->t_rxtcur,
		    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
		    tp->t_rttmin, TCPTV_REXMTMAX);
	}
	if (metrics.rmx_ssthresh) {
		/*
		 * There's some sort of gateway or interface
		 * buffer limit on the path.  Use this to set
		 * the slow start threshhold, but set the
		 * threshold to no less than 2*mss.
		 */
		tp->snd_ssthresh = max(2 * tp->t_maxseg, metrics.rmx_ssthresh);
	}

	/*
	 * Set the initial slow-start flight size.
	 *
	 * RFC5681 Section 3.1 specifies the default conservative values.
	 * RFC3390 specifies slightly more aggressive values.
	 * RFC6928 increases it to ten segments.
	 * Support for user specified value for initial flight size.
	 *
	 * If a SYN or SYN/ACK was lost and retransmitted, we have to
	 * reduce the initial CWND to one segment as congestion is likely
	 * requiring us to be cautious.
	 */
	if (tp->snd_cwnd == 1)
		tp->snd_cwnd = tp->t_maxseg;		/* SYN(-ACK) lost */
	else if (V_tcp_initcwnd_segments)
		tp->snd_cwnd = min(V_tcp_initcwnd_segments * tp->t_maxseg,
		    max(2 * tp->t_maxseg, V_tcp_initcwnd_segments * 1460));
	else if (V_tcp_do_rfc3390)
		tp->snd_cwnd = min(4 * tp->t_maxseg,
		    max(2 * tp->t_maxseg, 4380));
	else {
		/* Per RFC5681 Section 3.1 */
		if (tp->t_maxseg > 2190)
			tp->snd_cwnd = 2 * tp->t_maxseg;
		else if (tp->t_maxseg > 1095)
			tp->snd_cwnd = 3 * tp->t_maxseg;
		else
			tp->snd_cwnd = 4 * tp->t_maxseg;
	}

	if (CC_ALGO(tp)->conn_init != NULL)
		CC_ALGO(tp)->conn_init(tp->ccv);

	/* samkumar: print statement for debugging. Resurrect with DEBUG macro? */
#ifdef INSTRUMENT_TCP
	tcplp_sys_log("TCP CC_INIT %u %d %d", (unsigned int) tcplp_sys_get_millis(), (int) tp->snd_cwnd, (int) tp->snd_ssthresh);
#endif
}

inline void
cc_cong_signal(struct tcpcb *tp, struct tcphdr *th, uint32_t type)
{
	/* samkumar: Remove locks and stats from this function. */

	switch(type) {
	case CC_NDUPACK:
		if (!IN_FASTRECOVERY(tp->t_flags)) {
			tp->snd_recover = tp->snd_max;
			if (tp->t_flags & TF_ECN_PERMIT)
				tp->t_flags |= TF_ECN_SND_CWR;
		}
		break;
	case CC_ECN:
		if (!IN_CONGRECOVERY(tp->t_flags)) {
			tp->snd_recover = tp->snd_max;
			if (tp->t_flags & TF_ECN_PERMIT)
				tp->t_flags |= TF_ECN_SND_CWR;
		}
		break;
	case CC_RTO:
		tp->t_dupacks = 0;
		tp->t_bytes_acked = 0;
		EXIT_RECOVERY(tp->t_flags);
		tp->snd_ssthresh = max(2, min(tp->snd_wnd, tp->snd_cwnd) / 2 /
		    tp->t_maxseg) * tp->t_maxseg;
		tp->snd_cwnd = tp->t_maxseg;

		/*
		 * samkumar: Stats for TCPlp: count the number of timeouts (RTOs).
		 * I've commented this out (with #if 0) because it isn't part of TCP
		 * functionality. At some point, we may want to bring it back to
		 * measure performance.
		 */
#if 0
		tcplp_timeoutRexmitCnt++;
#endif
#ifdef INSTRUMENT_TCP
		tcplp_sys_log("TCP CC_RTO %u %d %d", (unsigned int) tcplp_sys_get_millis(), (int) tp->snd_cwnd, (int) tp->snd_ssthresh);
#endif
		break;
	case CC_RTO_ERR:
		/* RTO was unnecessary, so reset everything. */
		tp->snd_cwnd = tp->snd_cwnd_prev;
		tp->snd_ssthresh = tp->snd_ssthresh_prev;
		tp->snd_recover = tp->snd_recover_prev;
		if (tp->t_flags & TF_WASFRECOVERY)
			ENTER_FASTRECOVERY(tp->t_flags);
		if (tp->t_flags & TF_WASCRECOVERY)
			ENTER_CONGRECOVERY(tp->t_flags);
		tp->snd_nxt = tp->snd_max;
		tp->t_flags &= ~TF_PREVVALID;
		tp->t_badrxtwin = 0;
#ifdef INSTRUMENT_TCP
		tcplp_sys_log("TCP CC_RTO_ERR %u %d %d", (unsigned int) tcplp_sys_get_millis(), (int) tp->snd_cwnd, (int) tp->snd_ssthresh);
#endif
		break;
	}

	if (CC_ALGO(tp)->cong_signal != NULL) {
		if (th != NULL)
			tp->ccv->curack = th->th_ack;
		CC_ALGO(tp)->cong_signal(tp->ccv, type);
	}
}

static inline void
cc_post_recovery(struct tcpcb *tp, struct tcphdr *th)
{
	/* samkumar: remove lock */

	/* XXXLAS: KASSERT that we're in recovery? */
	if (CC_ALGO(tp)->post_recovery != NULL) {
		tp->ccv->curack = th->th_ack;
		CC_ALGO(tp)->post_recovery(tp->ccv);
	}
	/* XXXLAS: EXIT_RECOVERY ? */
	tp->t_bytes_acked = 0;
}


/*
 * Indicate whether this ack should be delayed.  We can delay the ack if
 * following conditions are met:
 *	- There is no delayed ack timer in progress.
 *	- Our last ack wasn't a 0-sized window. We never want to delay
 *	  the ack that opens up a 0-sized window.
 *	- LRO wasn't used for this segment. We make sure by checking that the
 *	  segment size is not larger than the MSS.
 *	- Delayed acks are enabled or this is a half-synchronized T/TCP
 *	  connection.
 */
#define DELAY_ACK(tp, tlen)						\
	((!tcp_timer_active(tp, TT_DELACK) &&				\
	    (tp->t_flags & TF_RXWIN0SENT) == 0) &&			\
	    (tlen <= tp->t_maxopd) &&					\
	    (V_tcp_delack_enabled || (tp->t_flags & TF_NEEDSYN)))

static inline void
cc_ecnpkt_handler(struct tcpcb *tp, struct tcphdr *th, uint8_t iptos)
{
	/* samkumar: remove lock */

	if (CC_ALGO(tp)->ecnpkt_handler != NULL) {
		switch (iptos & IPTOS_ECN_MASK) {
		case IPTOS_ECN_CE:
			tp->ccv->flags |= CCF_IPHDR_CE;
			break;
		case IPTOS_ECN_ECT0:
			tp->ccv->flags &= ~CCF_IPHDR_CE;
			break;
		case IPTOS_ECN_ECT1:
			tp->ccv->flags &= ~CCF_IPHDR_CE;
			break;
		}

		if (th->th_flags & TH_CWR)
			tp->ccv->flags |= CCF_TCPHDR_CWR;
		else
			tp->ccv->flags &= ~CCF_TCPHDR_CWR;

		if (tp->t_flags & TF_DELACK)
			tp->ccv->flags |= CCF_DELACK;
		else
			tp->ccv->flags &= ~CCF_DELACK;

		CC_ALGO(tp)->ecnpkt_handler(tp->ccv);

		if (tp->ccv->flags & CCF_ACKNOW)
			tcp_timer_activate(tp, TT_DELACK, tcp_delacktime);
	}
}

/*
 * External function: look up an entry in the hostcache and fill out the
 * supplied TCP metrics structure.  Fills in NULL when no entry was found or
 * a value is not set.
 */
/*
 * samkumar: This function is taken from tcp_hostcache.c. We have no host cache
 * in TCPlp, so I changed this to always act as if there is a miss. I removed
 * the first argument, formerly "struct in_coninfo *inc".
 */
void
tcp_hc_get(struct tcpcb* tp, struct hc_metrics_lite *hc_metrics_lite)
{
	bzero(hc_metrics_lite, sizeof(*hc_metrics_lite));
}

/*
 * External function: look up an entry in the hostcache and return the
 * discovered path MTU.  Returns NULL if no entry is found or value is not
 * set.
 */
 /*
  * samkumar: This function is taken from tcp_hostcache.c. We have no host cache
  * in TCPlp, so I changed this to always act as if there is a miss.
  */
uint64_t
tcp_hc_getmtu(struct tcpcb* tp)
{
	return 0;
}


/*
 * Issue RST and make ACK acceptable to originator of segment.
 * The mbuf must still include the original packet header.
 * tp may be NULL.
 */
/*
 * samkumar: Original signature was:
 * static void tcp_dropwithreset(struct mbuf *m, struct tcphdr *th, struct tcpcb *tp,
 *    int tlen, int rstreason)
 */
void
tcp_dropwithreset(struct ip6_hdr* ip6, struct tcphdr *th, struct tcpcb *tp, otInstance* instance,
    int tlen, int rstreason)
{
	/*
	 * samkumar: I removed logic to skip this for broadcast or multicast
	 * packets. In the FreeBSD version of this function, it would just
	 * call m_freem(m), if m->m_flags has M_BCAST or M_MCAST set, and not
	 * send a response packet.
	 * I also removed bandwidth limiting.
	 */
	if (th->th_flags & TH_RST)
		return;

	/* tcp_respond consumes the mbuf chain. */
	if (th->th_flags & TH_ACK) {
		tcp_respond(tp, instance, ip6, th, (tcp_seq) 0, th->th_ack, TH_RST);
	} else {
		if (th->th_flags & TH_SYN)
			tlen++;
		tcp_respond(tp, instance, ip6, th, th->th_seq + tlen, (tcp_seq) 0, TH_RST | TH_ACK);
	}
	return;
}

/*
 * TCP input handling is split into multiple parts:
 *   tcp6_input is a thin wrapper around tcp_input for the extended
 *	ip6_protox[] call format in ip6_input
 *   tcp_input handles primary segment validation, inpcb lookup and
 *	SYN processing on listen sockets
 *   tcp_do_segment processes the ACK and text of the segment for
 *	establishing, established and closing connections
 */
/* samkumar: The signature of this function was originally:
   tcp_input(struct mbuf **mp, int *offp, int proto) */
/* NOTE: tcp_fields_to_host(th) must be called before this function is called. */
int
tcp_input(struct ip6_hdr* ip6, struct tcphdr* th, otMessage* msg, struct tcpcb* tp, struct tcpcb_listen* tpl,
          struct tcplp_signals* sig)
{
	/*
	 * samkumar: I significantly modified this function, compared to the
	 * FreeBSD version. This function used to be reponsible for matching an
	 * incoming TCP segment to its TCB. That functionality is now done by
	 * TCPlp, and this function is only called once a match has been
	 * identified.
	 *
	 * The tp and tpl arguments are used to indicate the match. Exactly one of
	 * them must be NULL, and the other must be set. If tp is non-NULL, then
	 * this function assumes that the packet was matched to an active socket
	 * (connection endpoint). If tpl is non-NULL, then this function assumes
	 * that this packet is a candidate match for a passive socket (listener)
	 * and attempts to set up a new connection if the flags, sequence numbers,
	 * etc. look OK.
	 *
	 * TCPlp assumes that the packets are IPv6, so I removed any logic specific
	 * to IPv4.
	 *
	 * And of course, all code pertaining to locks and stats has been removed.
	 */
	int tlen = 0, off;
	int thflags;
	uint8_t iptos = 0;
	int drop_hdrlen;
	int rstreason = 0;
	struct tcpopt to;		/* options in this segment */
	uint8_t* optp = NULL;
	int optlen = 0;
	to.to_flags = 0;
	KASSERT(tp || tpl, ("One of tp and tpl must be positive"));

	/*
	 * samkumar: Here, there used to be code that handled preprocessing:
	 * calling m_pullup(m, sizeof(*ip6) + sizeof(*th)) to get the headers
	 * contiguous in memory, setting the ip6 and th pointers, validating the
	 * checksum, and dropping packets with unspecified source address. In
	 * TCPlp, all of this is done for a packet before this function is called.
	 */

	tlen = ntohs(ip6->ip6_plen); // assume *off == sizeof(*ip6)

	/*
	 * samkumar: Logic that handled IPv4 was deleted below. I won't add a
	 * comment every time this is done, but I'm putting it here (one of the
	 * first instances of this) for clarity.
	 */
	iptos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.		XXX
	 */
	off = (th->th_off_x2 >> TH_OFF_SHIFT) << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		goto drop;
	}
	tlen -= off;	/* tlen is used instead of ti->ti_len */
	/* samkumar: now, tlen is the length of the data */

	if (off > sizeof (struct tcphdr)) {
		/*
		 * samkumar: I removed a call to IP6_EXTHDR_CHECK, which I believe
		 * checks for IPv6 extension headers. In TCPlp, we assume that these
		 * are handled elsewhere in the networking stack, before the incoming
		 * packet is processed at the TCP layer. I also removed the followup
		 * calls to reassign the ip6 and th pointers.
		 */
		optlen = off - sizeof (struct tcphdr);
		optp = (uint8_t *)(th + 1);
	}

	thflags = th->th_flags;

	/*
	 * samkumar: There used to be a call here to tcp_fields_to_host(th), which
	 * changes the byte order of various fields to host format. I removed this
	 * call from there and handle it in TCPlp, before calling this. The reason
	 * is that it's possible for this function to be called twice by TCPlp's
	 * logic (e.g., if the packet matches a TIME-WAIT socket this function
	 * returns early, and the packet may then match a listening socket, at
 	 * which ppoint this function will be called again). Thus, any operations
	 * like this, which mutate the packet itself, need to happen before calling
	 * this function.
	 */

	/*
	 * Delay dropping TCP, IP headers, IPv6 ext headers, and TCP options.
	 *
	 * samkumar: My TCP header is in a different buffer from the IP header.
	 * drop_hdrlen is only meaningful as an offset into the TCP buffer,
	 * because it is used to determine how much of the packet to discard
	 * before copying it into the receive buffer. Therefore, my offset does
	 * not include the length of IP header and options, only the length of
	 * the TCP header and options.
	 */
	drop_hdrlen = /*off0 +*/ off;

	/*
	 * Locate pcb for segment; if we're likely to add or remove a
	 * connection then first acquire pcbinfo lock.  There are three cases
	 * where we might discover later we need a write lock despite the
	 * flags: ACKs moving a connection out of the syncache, ACKs for a
	 * connection in TIMEWAIT and SYNs not targeting a listening socket.
	 */

	/*
	 * samkumar: Locking code is removed, invalidating most of the above
	 * comment.
	 */

	/*
	 * samkumar: The FreeBSD code at logic here to check m->m_flags for the
	 * M_IP6_NEXTHOP flag, and search for the PACKET_TAG_IPFORWARD tag and
	 * store it in fwd_tag if so. In TCPlp, we assume that the IPv6 layer of
	 * the host network stack handles this kind of IPv6-related functionality,
	 * so this logic has been removed.
	 */

	/*
	 * samkumar: Here, there was code to match the packet to an inpcb and reply
	 * with an RST segment if no match is found. This included taking the
	 * fwd_tag into account, if set above (see the previous comment). I removed
	 * this code because, in TCPlp, this is done before calling this function.
	 */

	/*
	 * A previous connection in TIMEWAIT state is supposed to catch stray
	 * or duplicate segments arriving late.  If this segment was a
	 * legitimate new connection attempt, the old INPCB gets removed and
	 * we can try again to find a listening socket.
	 *
	 * At this point, due to earlier optimism, we may hold only an inpcb
	 * lock, and not the inpcbinfo write lock.  If so, we need to try to
	 * acquire it, or if that fails, acquire a reference on the inpcb,
	 * drop all locks, acquire a global write lock, and then re-acquire
	 * the inpcb lock.  We may at that point discover that another thread
	 * has tried to free the inpcb, in which case we need to loop back
	 * and try to find a new inpcb to deliver to.
	 *
	 * XXXRW: It may be time to rethink timewait locking.
	 */
	/*
	 * samkumar: The original code checked inp->inp_flags & INP_TIMEWAIT. I
	 * changed it to instead check tp->t_state, since we don't use inpcbs in
	 * TCPlp.
	 */
	if (tp && tp->t_state == TCP6S_TIME_WAIT) {
		/*
		 * samkumar: There's nothing wrong with the call to tcp_dooptions call
		 * that I've commented out below; it's just that the modified
		 * "tcp_twcheck" function no longer needs the options structure, so
		 * I figured that there's no longer a good reason to parse the options.
		 * In fact, this call was probably unnecessary even in the original
		 * FreeBSD TCP code, since tcp_twcheck, even without my modifications,
		 * did not use the pointer to the options structure!
		 */
		//if (thflags & TH_SYN)
			//tcp_dooptions(&to, optp, optlen, TO_SYN);
		/*
		 * samkumar: The original code would "goto findpcb;" if this branch is
		 * taken. Matching with a TCB is done outside of this function in
		 * TCPlp, so we instead return a special value so that the caller knows
		 * to try re-matching this packet to a socket.
		 */
		if (tcp_twcheck(tp,/*inp, &to,*/ th, /*m,*/ tlen))
			return (RELOOKUP_REQUIRED);
		return (IPPROTO_DONE);
	}
	/*
	 * The TCPCB may no longer exist if the connection is winding
	 * down or it is in the CLOSED state.  Either way we drop the
	 * segment and send an appropriate response.
	 */
	/*
	 * samkumar: There used to be code here that grabs the tp from the inpcb
	 * and drops with reset if the connection is in the closed state or if
	 * the tp is NULL. In TCPlp, the equivalent logic is done before entering
	 * this function. There was also code here to handle TCP offload, which
	 * TCPlp does not handle.
	 */

	/*
	 * We've identified a valid inpcb, but it could be that we need an
	 * inpcbinfo write lock but don't hold it.  In this case, attempt to
	 * acquire using the same strategy as the TIMEWAIT case above.  If we
	 * relock, we have to jump back to 'relocked' as the connection might
	 * now be in TIMEWAIT.
	 */
	/*
	 * samkumar: There used to be some code here for synchronization, MAC
	 * management, and debugging.
	 */

	/*
	 * When the socket is accepting connections (the INPCB is in LISTEN
	 * state) we look into the SYN cache if this is a new connection
	 * attempt or the completion of a previous one. Instead of checking
	 * so->so_options to check if the socket is listening, we rely on the
	 * arguments passed to this function (if tp == NULL, then tpl is not NULL
	 * and is the matching listen socket).
	 */

	if (/*so->so_options & SO_ACCEPTCONN*/tp == NULL) {
		/* samkumar: NULL check isn't needed but prevents a compiler warning */
		KASSERT(tpl != NULL && tpl->t_state == TCP6S_LISTEN, ("listen socket must be in listening state!"));

		/*
		 * samkumar: There used to be some code here that checks if the
		 * received segment is an ACK, and if so, searches the SYN cache to
		 * find an entry whose connection establishment handshake this segment
		 * completes. If such an entry is found, then a socket is created and
		 * then tcp_do_segment is called to actually run the code to mark the
		 * connection as established. If the received segment is an RST, then
		 * that is processed in the syncache as well. In TCPlp we do not use a
		 * SYN cache, so I've removed that code. The actual connection
		 * establishment/processing logic happens in tcp_do_segment anyway,
		 * which is called at the bottom of this function, so there's no need
		 * to rewrite this code with special-case logic for that.
		 */

		/*
		 * We can't do anything without SYN.
		 */
		if ((thflags & TH_SYN) == 0) {
			/*
			 * samkumar: Here, and in several other instances, the FreeBSD
			 * code would call tcp_log_addrs. Improving logging in these
			 * edge cases in TCPlp is left for the future --- for now, I just
			 * put "<addrs go here>" where the address string would go.
			 */
			tcplp_sys_log("%s; %s: Listen socket: "
			    "SYN is missing, segment ignored",
			    "<addrs go here>", __func__);
			goto dropunlock;
		}
		/*
		 * (SYN|ACK) is bogus on a listen socket.
		 */
		if (thflags & TH_ACK) {
			/* samkumar: See above comment regarding tcp_log_addrs. */
			tcplp_sys_log("%s; %s: Listen socket: "
			    "SYN|ACK invalid, segment rejected",
			    "<addrs go here>", __func__);
			/* samkumar: Removed call to syncache_badack(&inc); */
			rstreason = BANDLIM_RST_OPENPORT;
			goto dropwithreset;
		}
		/*
		 * If the drop_synfin option is enabled, drop all
		 * segments with both the SYN and FIN bits set.
		 * This prevents e.g. nmap from identifying the
		 * TCP/IP stack.
		 * XXX: Poor reasoning.  nmap has other methods
		 * and is constantly refining its stack detection
		 * strategies.
		 * XXX: This is a violation of the TCP specification
		 * and was used by RFC1644.
		 */
		if ((thflags & TH_FIN) && V_drop_synfin) {
			/* samkumar: See above comment regarding tcp_log_addrs. */
			tcplp_sys_log("%s; %s: Listen socket: "
			    "SYN|FIN segment ignored (based on "
			    "sysctl setting)", "<addrs go here>", __func__);
			goto dropunlock;
		}
		/*
		 * Segment's flags are (SYN) or (SYN|FIN).
		 *
		 * TH_PUSH, TH_URG, TH_ECE, TH_CWR are ignored
		 * as they do not affect the state of the TCP FSM.
		 * The data pointed to by TH_URG and th_urp is ignored.
		 */
		KASSERT((thflags & (TH_RST|TH_ACK)) == 0,
		    ("%s: Listen socket: TH_RST or TH_ACK set", __func__));
		KASSERT(thflags & (TH_SYN),
		    ("%s: Listen socket: TH_SYN not set", __func__));

		/*
		 * samkumar: There used to be some code here to reject incoming
		 * SYN packets for deprecated interface addresses unless
		 * V_ip6_use_deprecated is true. Rejecting the packet, in this case,
		 * means to "goto dropwithreset". I removed this functionality.
		 */

		/*
		 * Basic sanity checks on incoming SYN requests:
		 *   Don't respond if the destination is a link layer
		 *	broadcast according to RFC1122 4.2.3.10, p. 104.
		 *   If it is from this socket it must be forged.
		 *   Don't respond if the source or destination is a
		 *	global or subnet broad- or multicast address.
		 *   Note that it is quite possible to receive unicast
		 *	link-layer packets with a broadcast IP address. Use
		 *	in_broadcast() to find them.
		 */

		/*
		 * samkumar: There used to be a sanity check that drops (via
		 * "goto dropunlock") any broadcast or multicast packets. This check is
		 * done by checking m->m_flags for (M_BAST|M_MCAST). The original
		 * FreeBSD code for this has been removed (since checking m->m_flags
		 * isn't really useful to us anyway). Note that other FreeBSD code that
		 * checks for multicast source/destination addresses is retained below
		 * (but only for the IPv6 case; the original FreeBSD code also handled
	 	 * it for IPv4 addresses).
		 */

		if (th->th_dport == th->th_sport &&
		    IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &ip6->ip6_src)) {
			/* samkumar: See above comment regarding tcp_log_addrs. */
			tcplp_sys_log("%s; %s: Listen socket: "
			"Connection attempt to/from self "
			"ignored", "<addrs go here>", __func__);
			goto dropunlock;
		}
		if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
		    IN6_IS_ADDR_MULTICAST(&ip6->ip6_src)) {
			/* samkumar: See above comment regarding tcp_log_addrs. */
			tcplp_sys_log("%s; %s: Listen socket: "
			"Connection attempt from/to multicast "
			"address ignored", "<addrs go here>", __func__);
			goto dropunlock;
		}

		/*
		 * samkumar: The FreeBSD code would call
		 * syncache_add(&inc, &to, th, inp, &so, m, NULL, NULL);
		 * to add an entry to the SYN cache at this point. TCPlp doesn't use a
		 * syncache, so we initialize the new socket right away. The code to
		 * initialize the socket is taken from the syncache_socket function.
		 */

		tcp_dooptions(&to, optp, optlen, TO_SYN);
		tp = tcplp_sys_accept_ready(tpl, &ip6->ip6_dst, th->th_sport); // Try to allocate an active socket to accept into
		if (tp == NULL) {
			/* If we couldn't allocate, just ignore the SYN. */
			return IPPROTO_DONE;
		}
		if (tp == (struct tcpcb *) -1) {
			rstreason = ECONNREFUSED;
			goto dropwithreset;
		}
		tcp_state_change(tp, TCPS_SYN_RECEIVED);
		tpmarkpassiveopen(tp);
		tp->t_flags |= TF_ACKNOW; // samkumar: my addition
		tp->iss = tcp_new_isn(tp);
		tp->irs = th->th_seq;
		tcp_rcvseqinit(tp);
		tcp_sendseqinit(tp);
		tp->snd_wl1 = th->th_seq;
		tp->snd_max = tp->iss/* + 1*/;
		tp->snd_nxt = tp->iss/* + 1*/;
		tp->rcv_up = th->th_seq + 1;
		tp->rcv_wnd = imin(imax(cbuf_free_space(&tp->recvbuf), 0), TCP_MAXWIN);
		tp->rcv_adv += tp->rcv_wnd;
		tp->last_ack_sent = tp->rcv_nxt;
		memcpy(&tp->laddr, &ip6->ip6_dst, sizeof(tp->laddr));
		memcpy(&tp->faddr, &ip6->ip6_src, sizeof(tp->faddr));
		tp->fport = th->th_sport;
		tp->lport = tpl->lport;

		/*
		 * samkumar: Several of the checks below (taken from syncache_socket!)
		 * check for flags in sc->sc_flags. They have been written to directly
		 * check for the conditions on the TCP options structure or in the TCP
		 * header that would ordinarily be used to set flags in sc->sc_flags
		 * when adding an entry to the SYN cache.
		 *
		 * In effect, we combine the logic in syncache_add to set elements of
		 * sc with the logic in syncache_socket to transfer state from sc
		 * to the socket, but short-circuit the process to avoid ever storing
		 * data in sc. Since this isn't just adding or deleting code, I decided
		 * that it's better to keep comments indicating exactly how I composed
		 * these two functions.
		 */
		tp->t_flags = tp->t_flags & (TF_NOPUSH | TF_NODELAY | TF_NOOPT);
//		tp->t_flags = sototcpcb(lso)->t_flags & (TF_NOPUSH|TF_NODELAY);
//		if (sc->sc_flags & SCF_NOOPT)
//			tp->t_flags |= TF_NOOPT;
//		else {
		if (!(tp->t_flags & TF_NOOPT) && V_tcp_do_rfc1323) {
			if (/*sc->sc_flags & SCF_WINSCALE*/to.to_flags & TOF_SCALE) {
				int wscale = 0;

				/*
				 * Pick the smallest possible scaling factor that
				 * will still allow us to scale up to sb_max, aka
				 * kern.ipc.maxsockbuf.
				 *
				 * We do this because there are broken firewalls that
				 * will corrupt the window scale option, leading to
				 * the other endpoint believing that our advertised
				 * window is unscaled.  At scale factors larger than
				 * 5 the unscaled window will drop below 1500 bytes,
				 * leading to serious problems when traversing these
				 * broken firewalls.
				 *
				 * With the default maxsockbuf of 256K, a scale factor
				 * of 3 will be chosen by this algorithm.  Those who
				 * choose a larger maxsockbuf should watch out
				 * for the compatiblity problems mentioned above.
				 *
				 * RFC1323: The Window field in a SYN (i.e., a <SYN>
				 * or <SYN,ACK>) segment itself is never scaled.
				 */

				/*
				 * samkumar: The original logic, taken from syncache_add, is
				 * listed below, commented out. In practice, we just use
				 * wscale = 0 because in TCPlp we assume that the buffers
				 * aren't big enough for window scaling to be all that useful.
				 */
#if 0
				while (wscale < TCP_MAX_WINSHIFT &&
					(TCP_MAXWIN << wscale) < sb_max)
					wscale++;
#endif

				tp->t_flags |= TF_REQ_SCALE|TF_RCVD_SCALE;
				tp->snd_scale = /*sc->sc_requested_s_scale*/to.to_wscale;
				tp->request_r_scale = wscale;
			}
			if (/*sc->sc_flags & SCF_TIMESTAMP*/to.to_flags & TOF_TS) {
				tp->t_flags |= TF_REQ_TSTMP|TF_RCVD_TSTMP;
				tp->ts_recent = /*sc->sc_tsreflect*/to.to_tsval;
				tp->ts_recent_age = tcp_ts_getticks();
				tp->ts_offset = /*sc->sc_tsoff*/0; // No syncookies, so this should always be 0
			}

			/*
			 * samkumar: there used to be code here that would set the
			 * TF_SIGNATURE flag on tp->t_flags if SCF_SIGNATURE is set on
			 * sc->sc_flags. I've left it in below, commented out.
			 */
#if 0
	#ifdef TCP_SIGNATURE
			if (sc->sc_flags & SCF_SIGNATURE)
				tp->t_flags |= TF_SIGNATURE;
	#endif
#endif
			if (/*sc->sc_flags & SCF_SACK*/ to.to_flags & TOF_SACKPERM)
				tp->t_flags |= TF_SACK_PERMIT;
		}
		if (/*sc->sc_flags & SCF_ECN*/(th->th_flags & (TH_ECE|TH_CWR)) && V_tcp_do_ecn)
			tp->t_flags |= TF_ECN_PERMIT;

		/*
		 * Set up MSS and get cached values from tcp_hostcache.
		 * This might overwrite some of the defaults we just set.
		 */
		tcp_mss(tp, /*sc->sc_peer_mss*/(to.to_flags & TOF_MSS) ? to.to_mss : 0);

		tcp_output(tp); // to send the SYN-ACK

		tp->accepted_from = tpl;
		return (IPPROTO_DONE);
	} else if (tp->t_state == TCPS_LISTEN) {
		/*
		 * When a listen socket is torn down the SO_ACCEPTCONN
		 * flag is removed first while connections are drained
		 * from the accept queue in a unlock/lock cycle of the
		 * ACCEPT_LOCK, opening a race condition allowing a SYN
		 * attempt go through unhandled.
		 */
		goto dropunlock;
	}

	KASSERT(tp, ("tp is still NULL!"));

	/*
	 * samkumar: There used to be code here to verify TCP signatures. We don't
	 * support TCP signatures in TCPlp.
	 */

	/*
	 * Segment belongs to a connection in SYN_SENT, ESTABLISHED or later
	 * state.  tcp_do_segment() always consumes the mbuf chain, unlocks
	 * the inpcb, and unlocks pcbinfo.
	 */
	tcp_do_segment(ip6, th, msg, tp, drop_hdrlen, tlen, iptos, sig);
	return (IPPROTO_DONE);

	/*
	 * samkumar: Removed some locking and debugging code under all three of
	 * these labels: dropwithreset, dropunlock, and drop. I also removed some
	 * memory management code (e.g., calling m_freem(m) if m != NULL) since
	 * the caller of this function will take care of that kind of memory
	 * management in TCPlp.
	 */
dropwithreset:

	/*
	 * samkumar: The check against inp != NULL is now a check on tp != NULL.
	 */
	if (tp != NULL) {
		tcp_dropwithreset(ip6, th, tp, tp->instance, tlen, rstreason);
	} else
		tcp_dropwithreset(ip6, th, NULL, tpl->instance, tlen, rstreason);
	goto drop;

dropunlock:
drop:
	return (IPPROTO_DONE);
}

/*
 * samkumar: Original signature
 * static void
 * tcp_do_segment(struct mbuf *m, struct tcphdr *th, struct socket *so,
 *     struct tcpcb *tp, int drop_hdrlen, int tlen, uint8_t iptos,
 *     int ti_locked)
 */
static void
tcp_do_segment(struct ip6_hdr* ip6, struct tcphdr *th, otMessage* msg,
    struct tcpcb *tp, int drop_hdrlen, int tlen, uint8_t iptos,
    struct tcplp_signals* sig)
{
	/*
	 * samkumar: All code pertaining to locks, stats, and debug has been
	 * removed from this function.
	 */

	int thflags, acked, ourfinisacked, needoutput = 0;
	int rstreason, todrop, win;
	uint64_t tiwin;
	struct tcpopt to;
	uint32_t ticks = tcplp_sys_get_ticks();
	otInstance* instance = tp->instance;
	thflags = th->th_flags;
	tp->sackhint.last_sack_ack = 0;

	/*
	 * If this is either a state-changing packet or current state isn't
	 * established, we require a write lock on tcbinfo.  Otherwise, we
	 * allow the tcbinfo to be in either alocked or unlocked, as the
	 * caller may have unnecessarily acquired a write lock due to a race.
	 */

	/* samkumar: There used to be synchronization code here. */
	KASSERT(tp->t_state > TCPS_LISTEN, ("%s: TCPS_LISTEN",
	    __func__));
	KASSERT(tp->t_state != TCPS_TIME_WAIT, ("%s: TCPS_TIME_WAIT",
	    __func__));

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 * XXX: This should be done after segment
	 * validation to ignore broken/spoofed segs.
	 */
	tp->t_rcvtime = ticks;
	if (TCPS_HAVEESTABLISHED(tp->t_state))
		tcp_timer_activate(tp, TT_KEEP, TP_KEEPIDLE(tp));

	/*
	 * Scale up the window into a 32-bit value.
	 * For the SYN_SENT state the scale is zero.
	 */
	tiwin = th->th_win << tp->snd_scale;

	/*
	 * TCP ECN processing.
	 */
	/*
	 * samkumar: I intentionally left the TCPSTAT_INC lines below commented
	 * out, to avoid altering the structure of the code too much by
	 * reorganizing the switch statement.
	 */
	if (tp->t_flags & TF_ECN_PERMIT) {
		if (thflags & TH_CWR)
			tp->t_flags &= ~TF_ECN_SND_ECE;
		switch (iptos & IPTOS_ECN_MASK) {
		case IPTOS_ECN_CE:
			tp->t_flags |= TF_ECN_SND_ECE;
			//TCPSTAT_INC(tcps_ecn_ce);
			break;
		case IPTOS_ECN_ECT0:
			//TCPSTAT_INC(tcps_ecn_ect0);
			break;
		case IPTOS_ECN_ECT1:
			//TCPSTAT_INC(tcps_ecn_ect1);
			break;
		}

		/* Process a packet differently from RFC3168. */
		cc_ecnpkt_handler(tp, th, iptos);

		/* Congestion experienced. */
		if (thflags & TH_ECE) {
			cc_cong_signal(tp, th, CC_ECN);
		}
	}

	/*
	 * Parse options on any incoming segment.
	 */
	tcp_dooptions(&to, (uint8_t *)(th + 1),
	    ((th->th_off_x2 >> TH_OFF_SHIFT) << 2) - sizeof(struct tcphdr),
	    (thflags & TH_SYN) ? TO_SYN : 0);

	/*
	 * If echoed timestamp is later than the current time,
	 * fall back to non RFC1323 RTT calculation.  Normalize
	 * timestamp if syncookies were used when this connection
	 * was established.
	 */

	if ((to.to_flags & TOF_TS) && (to.to_tsecr != 0)) {
		to.to_tsecr -= tp->ts_offset;
		if (TSTMP_GT(to.to_tsecr, tcp_ts_getticks()))
			to.to_tsecr = 0;
	}
	/*
	 * If timestamps were negotiated during SYN/ACK they should
	 * appear on every segment during this session and vice versa.
	 */
	if ((tp->t_flags & TF_RCVD_TSTMP) && !(to.to_flags & TOF_TS)) {
		/* samkumar: See above comment regarding tcp_log_addrs. */
		tcplp_sys_log("%s; %s: Timestamp missing, "
			"no action", "<addrs go here>", __func__);
	}
	if (!(tp->t_flags & TF_RCVD_TSTMP) && (to.to_flags & TOF_TS)) {
		/* samkumar: See above comment regarding tcp_log_addrs. */
		tcplp_sys_log("%s; %s: Timestamp not expected, "
			"no action", "<addrs go here>", __func__);
	}

	/*
	 * Process options only when we get SYN/ACK back. The SYN case
	 * for incoming connections is handled in tcp_syncache.
	 * According to RFC1323 the window field in a SYN (i.e., a <SYN>
	 * or <SYN,ACK>) segment itself is never scaled.
	 * XXX this is traditional behavior, may need to be cleaned up.
	 */
	if (tp->t_state == TCPS_SYN_SENT && (thflags & TH_SYN)) {
		if ((to.to_flags & TOF_SCALE) &&
		    (tp->t_flags & TF_REQ_SCALE)) {
			tp->t_flags |= TF_RCVD_SCALE;
			tp->snd_scale = to.to_wscale;
		}
		/*
		 * Initial send window.  It will be updated with
		 * the next incoming segment to the scaled value.
		 */
		tp->snd_wnd = th->th_win;
		if (to.to_flags & TOF_TS) {
			tp->t_flags |= TF_RCVD_TSTMP;
			tp->ts_recent = to.to_tsval;
			tp->ts_recent_age = tcp_ts_getticks();
		}
		if (to.to_flags & TOF_MSS)
			tcp_mss(tp, to.to_mss);
		if ((tp->t_flags & TF_SACK_PERMIT) &&
		    (to.to_flags & TOF_SACKPERM) == 0)
			tp->t_flags &= ~TF_SACK_PERMIT;
	}
	/*
	 * Header prediction: check for the two common cases
	 * of a uni-directional data xfer.  If the packet has
	 * no control flags, is in-sequence, the window didn't
	 * change and we're not retransmitting, it's a
	 * candidate.  If the length is zero and the ack moved
	 * forward, we're the sender side of the xfer.  Just
	 * free the data acked & wake any higher level process
	 * that was blocked waiting for space.  If the length
	 * is non-zero and the ack didn't move, we're the
	 * receiver side.  If we're getting packets in-order
	 * (the reassembly queue is empty), add the data to
	 * the socket buffer and note that we need a delayed ack.
	 * Make sure that the hidden state-flags are also off.
	 * Since we check for TCPS_ESTABLISHED first, it can only
	 * be TH_NEEDSYN.
	 */
	/*
	 * samkumar: Replaced LIST_EMPTY(&tp->tsegq with the call to bmp_isempty).
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	    th->th_seq == tp->rcv_nxt &&
	    (thflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ACK)) == TH_ACK &&
	    tp->snd_nxt == tp->snd_max &&
	    tiwin && tiwin == tp->snd_wnd &&
	    ((tp->t_flags & (TF_NEEDSYN|TF_NEEDFIN)) == 0) &&
	    bmp_isempty(tp->reassbmp, REASSBMP_SIZE(tp)) &&
	    ((to.to_flags & TOF_TS) == 0 ||
	     TSTMP_GEQ(to.to_tsval, tp->ts_recent)) ) {

		/*
		 * If last ACK falls within this segment's sequence numbers,
		 * record the timestamp.
		 * NOTE that the test is modified according to the latest
		 * proposal of the tcplw@cray.com list (Braden 1993/04/26).
		 */
		if ((to.to_flags & TOF_TS) != 0 &&
		    SEQ_LEQ(th->th_seq, tp->last_ack_sent)) {
			tp->ts_recent_age = tcp_ts_getticks();
			tp->ts_recent = to.to_tsval;
		}

		if (tlen == 0) {
			if (SEQ_GT(th->th_ack, tp->snd_una) &&
			    SEQ_LEQ(th->th_ack, tp->snd_max) &&
			    !IN_RECOVERY(tp->t_flags) &&
			    (to.to_flags & TOF_SACK) == 0 &&
			    TAILQ_EMPTY(&tp->snd_holes)) {
				/*
				 * This is a pure ack for outstanding data.
				 */

				/*
				 * "bad retransmit" recovery.
				 */
				if (tp->t_rxtshift == 1 &&
				    tp->t_flags & TF_PREVVALID &&
				    (int)(ticks - tp->t_badrxtwin) < 0) {
					cc_cong_signal(tp, th, CC_RTO_ERR);
				}

				/*
				 * Recalculate the transmit timer / rtt.
				 *
				 * Some boxes send broken timestamp replies
				 * during the SYN+ACK phase, ignore
				 * timestamps of 0 or we could calculate a
				 * huge RTT and blow up the retransmit timer.
				 */

				if ((to.to_flags & TOF_TS) != 0 &&
				    to.to_tsecr) {
					uint32_t t;

					t = tcp_ts_getticks() - to.to_tsecr;
					if (!tp->t_rttlow || tp->t_rttlow > t)
						tp->t_rttlow = t;
					tcp_xmit_timer(tp,
					    TCP_TS_TO_TICKS(t) + 1);
				} else if (tp->t_rtttime &&
				    SEQ_GT(th->th_ack, tp->t_rtseq)) {
					if (!tp->t_rttlow ||
					    tp->t_rttlow > ticks - tp->t_rtttime)
						tp->t_rttlow = ticks - tp->t_rtttime;
					tcp_xmit_timer(tp,
							ticks - tp->t_rtttime);
				}

				acked = BYTES_THIS_ACK(tp, th);

				/*
				 * samkumar: Replaced sbdrop(&so->so_snd, acked) with this call
				 * to lbuf_pop.
				 */
				{
					uint32_t poppedbytes = lbuf_pop(&tp->sendbuf, acked, &sig->links_popped);
					KASSERT(poppedbytes == acked, ("More bytes were acked than are in the send buffer"));
					sig->bytes_acked += poppedbytes;
				}
				if (SEQ_GT(tp->snd_una, tp->snd_recover) &&
				    SEQ_LEQ(th->th_ack, tp->snd_recover))
					tp->snd_recover = th->th_ack - 1;

				/*
				 * Let the congestion control algorithm update
				 * congestion control related information. This
				 * typically means increasing the congestion
				 * window.
				 */
				cc_ack_received(tp, th, CC_ACK);

				tp->snd_una = th->th_ack;
				/*
				 * Pull snd_wl2 up to prevent seq wrap relative
				 * to th_ack.
				 */
				tp->snd_wl2 = th->th_ack;
				tp->t_dupacks = 0;

				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selwakeup/signal.  If data
				 * are ready to send, let tcp_output
				 * decide between more output or persist.
				 */

				if (tp->snd_una == tp->snd_max)
					tcp_timer_activate(tp, TT_REXMT, 0);
				else if (!tcp_timer_active(tp, TT_PERSIST))
					tcp_timer_activate(tp, TT_REXMT,
						      tp->t_rxtcur);

				/*
				 * samkumar: There used to be a call to sowwakeup(so); here,
				 * which wakes up any threads waiting for the socket to
				 * become ready for writing. TCPlp handles its send buffer
				 * differently so we do not need to replace this call with
				 * specialized code to handle this.
				 */

				/*
				 * samkumar: Replaced sbavail(&so->so_snd) with this call to
				 * lbuf_used_space.
				 */
				if (lbuf_used_space(&tp->sendbuf))
					(void) tcp_output(tp);
				goto check_delack;
			}
		} else if (th->th_ack == tp->snd_una &&
			/*
			 * samkumar: Replaced sbspace(&so->so_rcv) with this call to
			 * cbuf_free_space.
			 */
		    tlen <= cbuf_free_space(&tp->recvbuf)) {

			/*
			 * This is a pure, in-sequence data packet with
			 * nothing on the reassembly queue and we have enough
			 * buffer space to take it.
			 */
			/* Clean receiver SACK report if present */
			if ((tp->t_flags & TF_SACK_PERMIT) && tp->rcv_numsacks)
				tcp_clean_sackreport(tp);

			tp->rcv_nxt += tlen;
			/*
			 * Pull snd_wl1 up to prevent seq wrap relative to
			 * th_seq.
			 */
			tp->snd_wl1 = th->th_seq;
			/*
			 * Pull rcv_up up to prevent seq wrap relative to
			 * rcv_nxt.
			 */
			tp->rcv_up = tp->rcv_nxt;

		/*
		 * Automatic sizing of receive socket buffer.  Often the send
		 * buffer size is not optimally adjusted to the actual network
		 * conditions at hand (delay bandwidth product).  Setting the
		 * buffer size too small limits throughput on links with high
		 * bandwidth and high delay (eg. trans-continental/oceanic links).
		 *
		 * On the receive side the socket buffer memory is only rarely
		 * used to any significant extent.  This allows us to be much
		 * more aggressive in scaling the receive socket buffer.  For
		 * the case that the buffer space is actually used to a large
		 * extent and we run out of kernel memory we can simply drop
		 * the new segments; TCP on the sender will just retransmit it
		 * later.  Setting the buffer size too big may only consume too
		 * much kernel memory if the application doesn't read() from
		 * the socket or packet loss or reordering makes use of the
		 * reassembly queue.
		 *
		 * The criteria to step up the receive buffer one notch are:
		 *  1. Application has not set receive buffer size with
		 *     SO_RCVBUF. Setting SO_RCVBUF clears SB_AUTOSIZE.
		 *  2. the number of bytes received during the time it takes
		 *     one timestamp to be reflected back to us (the RTT);
		 *  3. received bytes per RTT is within seven eighth of the
		 *     current socket buffer size;
		 *  4. receive buffer size has not hit maximal automatic size;
		 *
		 * This algorithm does one step per RTT at most and only if
		 * we receive a bulk stream w/o packet losses or reorderings.
		 * Shrinking the buffer during idle times is not necessary as
		 * it doesn't consume any memory when idle.
		 *
		 * TODO: Only step up if the application is actually serving
		 * the buffer to better manage the socket buffer resources.
		 */

			/*
			 * samkumar: There used to be code here to dynamically size the
			 * receive buffer (tp->rfbuf_ts, rp->rfbuf_cnt, and the local
			 * newsize variable). In TCPlp, we don't support this, as the user
			 * allocates the receive buffer and its size can't be changed here.
			 * Therefore, I removed the code that does this. Note that the
			 * actual resizing of the buffer is done using sbreserve_locked,
			 * whose call comes later (not exactly where this comment is).
			 */

			/* Add data to socket buffer. */

			/*
			 * samkumar: The code that was here would just free the mbuf
			 * (with m_freem(m)) if SBS_CANTRCVMORE is set in
			 * so->so_rcv.sb_state. Otherwise, it would cut drop_hdrlen bytes
			 * from the mbuf (using m_adj(m, drop_hdrlen)) to discard the
			 * headers and then append the mbuf to the receive buffer using
			 * sbappendstream_locked(&so->so_rcv, m, 0). I've rewritten this
			 * to work the TCPlp way. The check to so->so_rcv.sb_state is
			 * replaced by a tcpiscantrcv call, and we copy bytes into
			 * TCPlp's circular buffer (since we designed it to avoid
			 * having dynamically-allocated memory for the receive buffer).
			 */

			if (!tpiscantrcv(tp)) {
				cbuf_write(&tp->recvbuf, msg, otMessageGetOffset(msg) + drop_hdrlen, tlen, cbuf_copy_from_message);
				if (tlen > 0) {
					sig->recvbuf_added = true;
				}
			} else {
				/*
				 * samkumar: We already know tlen != 0, so if we got here, then
				 * it means that we got data after we called SHUT_RD, or after
				 * receiving a FIN. I'm going to drop the connection in this
				 * case. I think FreeBSD might have just dropped the packet
				 * silently, but Linux handles it this way; this seems to be
				 * the right approach to me.
				 */
				tcp_drop(tp, ECONNABORTED);
				goto drop;
			}
			/* NB: sorwakeup_locked() does an implicit unlock. */
			/*
			 * samkumar: There used to be a call to sorwakeup_locked(so); here,
			 * which wakes up any threads waiting for the socket to become
			 * become ready for reading. TCPlp handles its buffering
			 * differently so we do not need to replace this call with
			 * specialized code to handle this.
			 */
			if (DELAY_ACK(tp, tlen)) {
				tp->t_flags |= TF_DELACK;
			} else {
				tp->t_flags |= TF_ACKNOW;
				tcp_output(tp);
			}
			goto check_delack;
		}
	}

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	/* samkumar: Replaced sbspace(&so->so_rcv) with call to cbuf_free_space. */
	win = cbuf_free_space(&tp->recvbuf);
	if (win < 0)
		win = 0;
	tp->rcv_wnd = imax(win, (int)(tp->rcv_adv - tp->rcv_nxt));

	/* Reset receive buffer auto scaling when not in bulk receive mode. */
	/* samkumar: Removed this receive buffer autoscaling code. */

	switch (tp->t_state) {

	/*
	 * If the state is SYN_RECEIVED:
	 *	if seg contains an ACK, but not for our SYN/ACK, send a RST.
	 *  (Added by Sam) if seg is resending the original SYN, resend the SYN/ACK
	 */
	/*
	 * samkumar: If we receive a retransmission of the original SYN, then
	 * resend the SYN/ACK segment. This case was probably handled by the
	 * SYN cache. Because TCPlp does not use a SYN cache, we need to write
	 * custom logic for it. It is handled in the "else if" clause below.
	 */
	case TCPS_SYN_RECEIVED:
		if ((thflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->snd_una) ||
		     SEQ_GT(th->th_ack, tp->snd_max))) {
				rstreason = BANDLIM_RST_OPENPORT;
				goto dropwithreset;
		} else if ((thflags & TH_SYN) && !(thflags & TH_ACK) && (th->th_seq == tp->irs)) {
			tp->t_flags |= TF_ACKNOW;
		}
		break;

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if seg contains an ECE and ECN support is enabled, the stream
	 *	    is ECN capable.
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((thflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->iss) ||
		     SEQ_GT(th->th_ack, tp->snd_max))) {
			rstreason = BANDLIM_UNLIMITED;
			goto dropwithreset;
		}
		if ((thflags & (TH_ACK|TH_RST)) == (TH_ACK|TH_RST)) {
			tp = tcp_drop(tp, ECONNREFUSED);
		}
		if (thflags & TH_RST)
			goto drop;
		if (!(thflags & TH_SYN))
			goto drop;

		tp->irs = th->th_seq;
		tcp_rcvseqinit(tp);
		if (thflags & TH_ACK) {
			/*
			 * samkumar: Removed call to soisconnected(so), since TCPlp has its
			 * own buffering.
			 */

			/* Do window scaling on this connection? */
			if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
				(TF_RCVD_SCALE|TF_REQ_SCALE)) {
				tp->rcv_scale = tp->request_r_scale;
			}
			tp->rcv_adv += imin(tp->rcv_wnd,
			    TCP_MAXWIN << tp->rcv_scale);
			tp->snd_una++;		/* SYN is acked */
			/*
			 * If there's data, delay ACK; if there's also a FIN
			 * ACKNOW will be turned on later.
			 */
			if (DELAY_ACK(tp, tlen) && tlen != 0)
				tcp_timer_activate(tp, TT_DELACK,
				    tcp_delacktime);
			else
				tp->t_flags |= TF_ACKNOW;

			if ((thflags & TH_ECE) && V_tcp_do_ecn) {
				tp->t_flags |= TF_ECN_PERMIT;
			}

			/*
			 * Received <SYN,ACK> in SYN_SENT[*] state.
			 * Transitions:
			 *	SYN_SENT  --> ESTABLISHED
			 *	SYN_SENT* --> FIN_WAIT_1
			 */
			tp->t_starttime = ticks;
			if (tp->t_flags & TF_NEEDFIN) {
				tcp_state_change(tp, TCPS_FIN_WAIT_1);
				tp->t_flags &= ~TF_NEEDFIN;
				thflags &= ~TH_SYN;
			} else {
				tcp_state_change(tp, TCPS_ESTABLISHED);
				/* samkumar: Set conn_established signal for TCPlp. */
				sig->conn_established = true;
				cc_conn_init(tp);
				tcp_timer_activate(tp, TT_KEEP,
				    TP_KEEPIDLE(tp));
			}
		} else {
			/*
			 * Received initial SYN in SYN-SENT[*] state =>
			 * simultaneous open.
			 * If it succeeds, connection is * half-synchronized.
			 * Otherwise, do 3-way handshake:
			 *        SYN-SENT -> SYN-RECEIVED
			 *        SYN-SENT* -> SYN-RECEIVED*
			 */
			tp->t_flags |= (TF_ACKNOW | TF_NEEDSYN);
			tcp_timer_activate(tp, TT_REXMT, 0);
			tcp_state_change(tp, TCPS_SYN_RECEIVED);
			/*
			 * samkumar: We would have incremented snd_next in tcp_output when
			 * we sent the original SYN, so decrement it here. (Another
			 * consequence of removing the SYN cache.)
			 */
			tp->snd_nxt--;
		}

		/*
		 * Advance th->th_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		th->th_seq++;
		if (tlen > tp->rcv_wnd) {
			todrop = tlen - tp->rcv_wnd;
			/*
			 * samkumar: I removed a call to m_adj(m, -todrop), which intends
			 * to trim the data so it fits in the window. We can just read less
			 * when copying into the receive buffer in TCPlp, so we don't need
			 * to do this.
			 */
			(void) todrop; /* samkumar: Prevent a compiler warning */
			tlen = tp->rcv_wnd;
			thflags &= ~TH_FIN;
		}
		tp->snd_wl1 = th->th_seq - 1;
		tp->rcv_up = th->th_seq;
		/*
		 * Client side of transaction: already sent SYN and data.
		 * If the remote host used T/TCP to validate the SYN,
		 * our data will be ACK'd; if so, enter normal data segment
		 * processing in the middle of step 5, ack processing.
		 * Otherwise, goto step 6.
		 */
		if (thflags & TH_ACK)
			goto process_ACK;

		goto step6;

	/*
	 * If the state is LAST_ACK or CLOSING or TIME_WAIT:
	 *      do normal processing.
	 *
	 * NB: Leftover from RFC1644 T/TCP.  Cases to be reused later.
	 */
	case TCPS_LAST_ACK:
	case TCPS_CLOSING:
		break;  /* continue normal processing */
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check the RST flag and sequence number since reset segments
	 * are exempt from the timestamp and connection count tests.  This
	 * fixes a bug introduced by the Stevens, vol. 2, p. 960 bugfix
	 * below which allowed reset segments in half the sequence space
	 * to fall though and be processed (which gives forged reset
	 * segments with a random sequence number a 50 percent chance of
	 * killing a connection).
	 * Then check timestamp, if present.
	 * Then check the connection count, if present.
	 * Then check that at least some bytes of segment are within
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
	if (thflags & TH_RST) {
		/*
		 * RFC5961 Section 3.2
		 *
		 * - RST drops connection only if SEG.SEQ == RCV.NXT.
		 * - If RST is in window, we send challenge ACK.
		 *
		 * Note: to take into account delayed ACKs, we should
		 *   test against last_ack_sent instead of rcv_nxt.
		 * Note 2: we handle special case of closed window, not
		 *   covered by the RFC.
		 */
		if ((SEQ_GEQ(th->th_seq, tp->last_ack_sent) &&
		    SEQ_LT(th->th_seq, tp->last_ack_sent + tp->rcv_wnd)) ||
		    (tp->rcv_wnd == 0 && tp->last_ack_sent == th->th_seq)) {

			/*
			 * samkumar: This if statement used to also be prefaced with
			 * "V_tcp_insecure_rst ||". But I removed it, since there's no
			 * reason to support an insecure option in TCPlp (my guess is that
			 * FreeBSD supported it for legacy reasons).
			 */
			if (tp->last_ack_sent == th->th_seq) {
				/*
				 * samkumar: Normally, the error number would be stored in
				 * so->so_error. Instead, we put it in this "droperror" local
				 * variable and then pass it to tcplp_sys_connection_lost.
				 */
				int droperror = 0;
				/* Drop the connection. */
				switch (tp->t_state) {
				case TCPS_SYN_RECEIVED:
					droperror = ECONNREFUSED;
					goto close;
				case TCPS_ESTABLISHED:
				case TCPS_FIN_WAIT_1:
				case TCPS_FIN_WAIT_2:
				case TCPS_CLOSE_WAIT:
					droperror = ECONNRESET;
				close:
					tcp_state_change(tp, TCPS_CLOSED);
					/* FALLTHROUGH */
				default:
					tp = tcp_close(tp);
					tcplp_sys_connection_lost(tp, droperror);
				}
			} else {
				/* Send challenge ACK. */
				tcp_respond(tp, tp->instance, ip6, th, tp->rcv_nxt, tp->snd_nxt, TH_ACK);
				tp->last_ack_sent = tp->rcv_nxt;
			}
		}
		goto drop;
	}

	/*
	 * RFC5961 Section 4.2
	 * Send challenge ACK for any SYN in synchronized state.
	 */
	/*
	 * samkumar: I added the check for the SYN-RECEIVED state in this if
	 * statement (another consequence of removing the SYN cache).
	 */
	if ((thflags & TH_SYN) && tp->t_state != TCPS_SYN_SENT && tp->t_state != TCP6S_SYN_RECEIVED) {
		/*
		 * samkumar: The modern way to handle this is to send a Challenge ACK.
		 * FreeBSD supports this, but it also has this V_tcp_insecure_syn
		 * options that will cause it to drop the connection if the SYN falls
		 * in the receive window. In TCPlp we *only* support Challenge ACKs
		 * (the secure way of doing it), so I've removed code for the insecure
		 * way. (Presumably the reason why FreeBSD supports the insecure way is
		 * for legacy code, which we don't really care about in TCPlp).
		 */
		/* Send challenge ACK. */
		tcplp_sys_log("Sending challenge ACK");
		tcp_respond(tp, tp->instance, ip6, th, tp->rcv_nxt, tp->snd_nxt, TH_ACK);
		tp->last_ack_sent = tp->rcv_nxt;
		goto drop;
	}

	/*
	 * RFC 1323 PAWS: If we have a timestamp reply on this segment
	 * and it's less than ts_recent, drop it.
	 */
	if ((to.to_flags & TOF_TS) != 0 && tp->ts_recent &&
	    TSTMP_LT(to.to_tsval, tp->ts_recent)) {

		/* Check to see if ts_recent is over 24 days old.  */
		if (tcp_ts_getticks() - tp->ts_recent_age > TCP_PAWS_IDLE) {
			/*
			 * Invalidate ts_recent.  If this segment updates
			 * ts_recent, the age will be reset later and ts_recent
			 * will get a valid value.  If it does not, setting
			 * ts_recent to zero will at least satisfy the
			 * requirement that zero be placed in the timestamp
			 * echo reply when ts_recent isn't valid.  The
			 * age isn't reset until we get a valid ts_recent
			 * because we don't want out-of-order segments to be
			 * dropped when ts_recent is old.
			 */
			tp->ts_recent = 0;
		} else {
			if (tlen)
				goto dropafterack;
			goto drop;
		}
	}

	/*
	 * In the SYN-RECEIVED state, validate that the packet belongs to
	 * this connection before trimming the data to fit the receive
	 * window.  Check the sequence number versus IRS since we know
	 * the sequence numbers haven't wrapped.  This is a partial fix
	 * for the "LAND" DoS attack.
	 */
	if (tp->t_state == TCPS_SYN_RECEIVED && SEQ_LT(th->th_seq, tp->irs)) {
		rstreason = BANDLIM_RST_OPENPORT;
		goto dropwithreset;
	}

	todrop = tp->rcv_nxt - th->th_seq;
	if (todrop > 0) {
		if (thflags & TH_SYN) {
			thflags &= ~TH_SYN;
			th->th_seq++;
			if (th->th_urp > 1)
				th->th_urp--;
			else
				thflags &= ~TH_URG;
			todrop--;
		}
		/*
		 * Following if statement from Stevens, vol. 2, p. 960.
		 */
		if (todrop > tlen
		    || (todrop == tlen && (thflags & TH_FIN) == 0)) {
			/*
			 * Any valid FIN must be to the left of the window.
			 * At this point the FIN must be a duplicate or out
			 * of sequence; drop it.
			 */
			thflags &= ~TH_FIN;

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */
			tp->t_flags |= TF_ACKNOW;
			todrop = tlen;
		}
		/* samkumar: There was an else case that only collected stats. */
		drop_hdrlen += todrop;	/* drop from the top afterwards */
		th->th_seq += todrop;
		tlen -= todrop;
		if (th->th_urp > todrop)
			th->th_urp -= todrop;
		else {
			thflags &= ~TH_URG;
			th->th_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	/*
	 * samkumar: TCPlp is designed for embedded systems where there is no
	 * concept of a "process" that has allocated a TCP socket. Therefore, we
	 * do not implement the functionality in the above comment (the code for
	 * it used to be here, and I removed it).
	 */
	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (th->th_seq + tlen) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		if (todrop >= tlen) {
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && th->th_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
			} else
				goto dropafterack;
		}
		/*
		 * samkumar: I removed a call to m_adj(m, -todrop), which intends
		 * to trim the data so it fits in the window. We can just read less
		 * when copying into the receive buffer in TCPlp, so we don't need
		 * to do this. Subtracting it from tlen gives us enough information to
		 * do this later. In FreeBSD, this isn't possible because the mbuf
		 * itself becomes part of the receive buffer, so the mbuf has to be
		 * trimmed in order for this to work out.
		 */
		tlen -= todrop;
		thflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If last ACK falls within this segment's sequence numbers,
	 * record its timestamp.
	 * NOTE:
	 * 1) That the test incorporates suggestions from the latest
	 *    proposal of the tcplw@cray.com list (Braden 1993/04/26).
	 * 2) That updating only on newer timestamps interferes with
	 *    our earlier PAWS tests, so this check should be solely
	 *    predicated on the sequence space of this segment.
	 * 3) That we modify the segment boundary check to be
	 *        Last.ACK.Sent <= SEG.SEQ + SEG.Len
	 *    instead of RFC1323's
	 *        Last.ACK.Sent < SEG.SEQ + SEG.Len,
	 *    This modified check allows us to overcome RFC1323's
	 *    limitations as described in Stevens TCP/IP Illustrated
	 *    Vol. 2 p.869. In such cases, we can still calculate the
	 *    RTT correctly when RCV.NXT == Last.ACK.Sent.
	 */

	if ((to.to_flags & TOF_TS) != 0 &&
	    SEQ_LEQ(th->th_seq, tp->last_ack_sent) &&
	    SEQ_LEQ(tp->last_ack_sent, th->th_seq + tlen +
		((thflags & (TH_SYN|TH_FIN)) != 0))) {
		tp->ts_recent_age = tcp_ts_getticks();
		tp->ts_recent = to.to_tsval;
	}

	/*
	 * If the ACK bit is off:  if in SYN-RECEIVED state or SENDSYN
	 * flag is on (half-synchronized state), then queue data for
	 * later processing; else drop segment and return.
	 */
	if ((thflags & TH_ACK) == 0) {
		if (tp->t_state == TCPS_SYN_RECEIVED ||
		    (tp->t_flags & TF_NEEDSYN))
			goto step6;
		else if (tp->t_flags & TF_ACKNOW)
			goto dropafterack;
		else
			goto drop;
	}

	tcplp_sys_log("Processing ACK");

	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state, the ack ACKs our SYN, so enter
	 * ESTABLISHED state and continue processing.
	 * The ACK was checked above.
	 */
	case TCPS_SYN_RECEIVED:
		/*
		 * samkumar: Removed call to soisconnected(so), since TCPlp has its
		 * own buffering.
		 */
		/* Do window scaling? */
		if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
			(TF_RCVD_SCALE|TF_REQ_SCALE)) {
			tp->rcv_scale = tp->request_r_scale;
			tp->snd_wnd = tiwin;
		}
		/*
		 * Make transitions:
		 *      SYN-RECEIVED  -> ESTABLISHED
		 *      SYN-RECEIVED* -> FIN-WAIT-1
		 */
		tp->t_starttime = ticks;
		if (tp->t_flags & TF_NEEDFIN) {
			tcp_state_change(tp, TCPS_FIN_WAIT_1);
			tp->t_flags &= ~TF_NEEDFIN;
		} else {
			tcp_state_change(tp, TCPS_ESTABLISHED);
			/* samkumar: Set conn_established signal for TCPlp. */
			sig->conn_established = true;
			cc_conn_init(tp);
			tcp_timer_activate(tp, TT_KEEP, TP_KEEPIDLE(tp));
			/*
			 * samkumar: I added this check to account for simultaneous open.
			 * If this socket was opened actively, then the fact that we are
			 * in SYN-RECEIVED indicates that we are in simultaneous open.
			 * Therefore, don't ACK the SYN-ACK (unless it contains data or
			 * something, which will be processed later).
			 */
			if (!tpispassiveopen(tp)) {
				tp->t_flags &= ~TF_ACKNOW;
			} else {
				/*
				 * samkumar: Otherwise, we entered the ESTABLISHED state by
				 * accepting a connection, so call the appropriate callback in
				 * TCPlp. TODO: consider using signals to handle this?
				 */
				 bool accepted = tcplp_sys_accepted_connection(tp->accepted_from, tp, &ip6->ip6_src, th->th_sport);
				 if (!accepted) {
					 rstreason = ECONNREFUSED;
					 goto dropwithreset;
				 }
			 }
		}
		/*
		 * If segment contains data or ACK, will call tcp_reass()
		 * later; if not, do so now to pass queued data to user.
		 */
		if (tlen == 0 && (thflags & TH_FIN) == 0)
			(void) tcp_reass(tp, (struct tcphdr *)0, 0,
			    (otMessage*)0, 0, sig);

		tp->snd_wl1 = th->th_seq - 1;
		/* FALLTHROUGH */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < th->th_ack <= tp->snd_max
	 * then advance tp->snd_una to th->th_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
		if (SEQ_GT(th->th_ack, tp->snd_max)) {
			goto dropafterack;
		}

		if ((tp->t_flags & TF_SACK_PERMIT) &&
		    ((to.to_flags & TOF_SACK) ||
		     !TAILQ_EMPTY(&tp->snd_holes)))
			tcp_sack_doack(tp, &to, th->th_ack);

		if (SEQ_LEQ(th->th_ack, tp->snd_una)) {
			if (tlen == 0 && tiwin == tp->snd_wnd) {
				/*
				 * If this is the first time we've seen a
				 * FIN from the remote, this is not a
				 * duplicate and it needs to be processed
				 * normally.  This happens during a
				 * simultaneous close.
				 */
				if ((thflags & TH_FIN) &&
				    (TCPS_HAVERCVDFIN(tp->t_state) == 0)) {
					tp->t_dupacks = 0;
					break;
				}
				/*
				 * If we have outstanding data (other than
				 * a window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change and FIN isn't set),
				 * the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.
				 *
				 * We know we're losing at the current
				 * window size so do congestion avoidance
				 * (set ssthresh to half the current window
				 * and pull our congestion window back to
				 * the new ssthresh).
				 *
				 * Dup acks mean that packets have left the
				 * network (they're now cached at the receiver)
				 * so bump cwnd by the amount in the receiver
				 * to keep a constant cwnd packets in the
				 * network.
				 *
				 * When using TCP ECN, notify the peer that
				 * we reduced the cwnd.
				 */
				if (!tcp_timer_active(tp, TT_REXMT) ||
				    th->th_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks > tcprexmtthresh ||
				     IN_FASTRECOVERY(tp->t_flags)) {
					cc_ack_received(tp, th, CC_DUPACK);
					if ((tp->t_flags & TF_SACK_PERMIT) &&
					    IN_FASTRECOVERY(tp->t_flags)) {
						int awnd;

						/*
						 * Compute the amount of data in flight first.
						 * We can inject new data into the pipe iff
						 * we have less than 1/2 the original window's
						 * worth of data in flight.
						 */
						awnd = (tp->snd_nxt - tp->snd_fack) +
							tp->sackhint.sack_bytes_rexmit;
						if (awnd < tp->snd_ssthresh) {
							tp->snd_cwnd += tp->t_maxseg;
							if (tp->snd_cwnd > tp->snd_ssthresh)
								tp->snd_cwnd = tp->snd_ssthresh;
						}
					} else
						tp->snd_cwnd += tp->t_maxseg;
#ifdef INSTRUMENT_TCP
					tcplp_sys_log("TCP DUPACK");
#endif
					(void) tcp_output(tp);
					goto drop;
				} else if (tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;

					/*
					 * If we're doing sack, check to
					 * see if we're already in sack
					 * recovery. If we're not doing sack,
					 * check to see if we're in newreno
					 * recovery.
					 */
					if (tp->t_flags & TF_SACK_PERMIT) {
						if (IN_FASTRECOVERY(tp->t_flags)) {
							tp->t_dupacks = 0;
							break;
						}
					} else {
						if (SEQ_LEQ(th->th_ack,
						    tp->snd_recover)) {
							tp->t_dupacks = 0;
							break;
						}
					}
					/* Congestion signal before ack. */
					cc_cong_signal(tp, th, CC_NDUPACK);
					cc_ack_received(tp, th, CC_DUPACK);
					tcp_timer_activate(tp, TT_REXMT, 0);
					tp->t_rtttime = 0;

#ifdef INSTRUMENT_TCP
					tcplp_sys_log("TCP DUPACK_THRESH");
#endif
					if (tp->t_flags & TF_SACK_PERMIT) {
						tp->sack_newdata = tp->snd_nxt;
						tp->snd_cwnd = tp->t_maxseg;
						(void) tcp_output(tp);
						goto drop;
					}

					tp->snd_nxt = th->th_ack;
					tp->snd_cwnd = tp->t_maxseg;
					(void) tcp_output(tp);
					tp->snd_cwnd = tp->snd_ssthresh +
					     tp->t_maxseg *
					     (tp->t_dupacks - tp->snd_limited);
#ifdef INSTRUMENT_TCP
					tcplp_sys_log("TCP SET_cwnd %d", (int) tp->snd_cwnd);
#endif
					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				} else if (V_tcp_do_rfc3042) {
					/*
					 * Process first and second duplicate
					 * ACKs. Each indicates a segment
					 * leaving the network, creating room
					 * for more. Make sure we can send a
					 * packet on reception of each duplicate
					 * ACK by increasing snd_cwnd by one
					 * segment. Restore the original
					 * snd_cwnd after packet transmission.
					 */
					uint64_t oldcwnd;
					tcp_seq oldsndmax;
					uint32_t sent;
					int avail;
					cc_ack_received(tp, th, CC_DUPACK);
					oldcwnd = tp->snd_cwnd;
					oldsndmax = tp->snd_max;

#ifdef INSTRUMENT_TCP
					tcplp_sys_log("TCP LIM_TRANS");
#endif

					KASSERT(tp->t_dupacks == 1 ||
					    tp->t_dupacks == 2,
					    ("%s: dupacks not 1 or 2",
					    __func__));
					if (tp->t_dupacks == 1)
						tp->snd_limited = 0;
					tp->snd_cwnd =
					    (tp->snd_nxt - tp->snd_una) +
					    (tp->t_dupacks - tp->snd_limited) *
					    tp->t_maxseg;
					/*
					 * Only call tcp_output when there
					 * is new data available to be sent.
					 * Otherwise we would send pure ACKs.
					 */
					/*
					 * samkumar: Replace sbavail(&so->so_snd) with the call to
					 * lbuf_used_space.
					 */
					avail = lbuf_used_space(&tp->sendbuf) -
					    (tp->snd_nxt - tp->snd_una);
					if (avail > 0)
						(void) tcp_output(tp);
					sent = tp->snd_max - oldsndmax;
					if (sent > tp->t_maxseg) {
						KASSERT((tp->t_dupacks == 2 &&
						    tp->snd_limited == 0) ||
						   (sent == tp->t_maxseg + 1 &&
						    tp->t_flags & TF_SENTFIN),
						    ("%s: sent too much",
						    __func__));
						tp->snd_limited = 2;
					} else if (sent > 0)
						++tp->snd_limited;
					tp->snd_cwnd = oldcwnd;
#ifdef INSTRUMENT_TCP
					tcplp_sys_log("TCP RESET_cwnd %d", (int) tp->snd_cwnd);
#endif
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}

		KASSERT(SEQ_GT(th->th_ack, tp->snd_una),
		    ("%s: th_ack <= snd_una", __func__));

		/*
		 * If the congestion window was inflated to account
		 * for the other side's cached packets, retract it.
		 */
		if (IN_FASTRECOVERY(tp->t_flags)) {
			if (SEQ_LT(th->th_ack, tp->snd_recover)) {
				if (tp->t_flags & TF_SACK_PERMIT)
					tcp_sack_partialack(tp, th);
				else
					tcp_newreno_partial_ack(tp, th);
			} else
				cc_post_recovery(tp, th);
		}

		tp->t_dupacks = 0;
		/*
		 * If we reach this point, ACK is not a duplicate,
		 *     i.e., it ACKs something we sent.
		 */
		if (tp->t_flags & TF_NEEDSYN) {
			/*
			 * T/TCP: Connection was half-synchronized, and our
			 * SYN has been ACK'd (so connection is now fully
			 * synchronized).  Go to non-starred state,
			 * increment snd_una for ACK of SYN, and check if
			 * we can do window scaling.
			 */
			tp->t_flags &= ~TF_NEEDSYN;
			tp->snd_una++;
			/* Do window scaling? */
			if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
				(TF_RCVD_SCALE|TF_REQ_SCALE)) {
				tp->rcv_scale = tp->request_r_scale;
				/* Send window already scaled. */
			}
		}

process_ACK:
		acked = BYTES_THIS_ACK(tp, th);

		tcplp_sys_log("Bytes acked: %d", acked);
		/*
		 * If we just performed our first retransmit, and the ACK
		 * arrives within our recovery window, then it was a mistake
		 * to do the retransmit in the first place.  Recover our
		 * original cwnd and ssthresh, and proceed to transmit where
		 * we left off.
		 */
		if (tp->t_rxtshift == 1 && tp->t_flags & TF_PREVVALID &&
		    (int)(ticks - tp->t_badrxtwin) < 0)
			cc_cong_signal(tp, th, CC_RTO_ERR);

		/*
		 * If we have a timestamp reply, update smoothed
		 * round trip time.  If no timestamp is present but
		 * transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 *
		 * Some boxes send broken timestamp replies
		 * during the SYN+ACK phase, ignore
		 * timestamps of 0 or we could calculate a
		 * huge RTT and blow up the retransmit timer.
		 */

		if ((to.to_flags & TOF_TS) != 0 && to.to_tsecr) {
			uint32_t t;

			t = tcp_ts_getticks() - to.to_tsecr;
			if (!tp->t_rttlow || tp->t_rttlow > t)
				tp->t_rttlow = t;
			tcp_xmit_timer(tp, TCP_TS_TO_TICKS(t) + 1);
		} else if (tp->t_rtttime && SEQ_GT(th->th_ack, tp->t_rtseq)) {
			if (!tp->t_rttlow || tp->t_rttlow > ticks - tp->t_rtttime)
				tp->t_rttlow = ticks - tp->t_rtttime;
			tcp_xmit_timer(tp, ticks - tp->t_rtttime);
		}

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (th->th_ack == tp->snd_max) {
			tcp_timer_activate(tp, TT_REXMT, 0);
			needoutput = 1;
		} else if (!tcp_timer_active(tp, TT_PERSIST)) {
			tcp_timer_activate(tp, TT_REXMT, tp->t_rxtcur);
		}

		/*
		 * If no data (only SYN) was ACK'd,
		 *    skip rest of ACK processing.
		 */
		if (acked == 0)
			goto step6;

		/*
		 * Let the congestion control algorithm update congestion
		 * control related information. This typically means increasing
		 * the congestion window.
		 */
		cc_ack_received(tp, th, CC_ACK);

		/*
		 * samkumar: I replaced the calls to sbavail(&so->so_snd) with new
		 * calls to lbuf_used_space, and then I modified the code to actually
		 * remove code from the send buffer, formerly done via
		 * sbcut_locked(&so->so_send, (int)sbavail(&so->so_snd)) in the if case
		 * and sbcut_locked(&so->so_snd, acked) in the else case, to use the
		 * data structures for TCPlp's data buffering.
		 */
		if (acked > lbuf_used_space(&tp->sendbuf)) {
			uint32_t poppedbytes;
			uint32_t usedspace = lbuf_used_space(&tp->sendbuf);
			tp->snd_wnd -= usedspace;
			poppedbytes = lbuf_pop(&tp->sendbuf, usedspace, &sig->links_popped);
			KASSERT(poppedbytes == usedspace, ("Could not fully empty send buffer"));
			sig->bytes_acked += poppedbytes;
			ourfinisacked = 1;
		} else {
			uint32_t poppedbytes = lbuf_pop(&tp->sendbuf, acked, &sig->links_popped);
			KASSERT(poppedbytes == acked, ("Could not remove acked bytes from send buffer"));
			sig->bytes_acked += poppedbytes;
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		/* NB: sowwakeup_locked() does an implicit unlock. */
		/*
		 * samkumar: There used to be a call to sowwakeup(so); here,
		 * which wakes up any threads waiting for the socket to
		 * become ready for writing. TCPlp handles its send buffer
		 * differently so we do not need to replace this call with
		 * specialized code to handle this.
		 */
		/* Detect una wraparound. */
		if (!IN_RECOVERY(tp->t_flags) &&
		    SEQ_GT(tp->snd_una, tp->snd_recover) &&
		    SEQ_LEQ(th->th_ack, tp->snd_recover))
			tp->snd_recover = th->th_ack - 1;
		/* XXXLAS: Can this be moved up into cc_post_recovery? */
		if (IN_RECOVERY(tp->t_flags) &&
		    SEQ_GEQ(th->th_ack, tp->snd_recover)) {
			EXIT_RECOVERY(tp->t_flags);
		}
		tp->snd_una = th->th_ack;
		if (tp->t_flags & TF_SACK_PERMIT) {
			if (SEQ_GT(tp->snd_una, tp->snd_recover))
				tp->snd_recover = tp->snd_una;
		}
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 *
				 * XXXjl:
				 * we should release the tp also, and use a
				 * compressed state.
				 */
				/*
				 * samkumar: I replaced a check for the SBS_CANTRCVMORE flag
				 * in so->so_rcv.sb_state with a call to tcpiscantrcv.
				 */
				if (tpiscantrcv(tp)) {
					/* samkumar: Removed a call to soisdisconnected(so). */
					tcp_timer_activate(tp, TT_2MSL,
					    (tcp_fast_finwait2_recycle ?
					    tcp_finwait2_timeout :
					    TP_MAXIDLE(tp)));
				}
				tcp_state_change(tp, TCPS_FIN_WAIT_2);
			}
			break;

		/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				/*
				 * samkumar: I added the line below. We need to avoid sending
				 * an ACK in the TIME-WAIT state, since we don't want to
				 * ACK ACKs. This edge case appears because TCPlp, unlike the
				 * original FreeBSD code, uses tcpcbs for connections in the
				 * TIME-WAIT state (FreeBSD uses a different, smaller
				 * structure).
				 */
				tp->t_flags &= ~TF_ACKNOW;
				tcp_twstart(tp);
				return;
			}
			break;

		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_close(tp);
				tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
				goto drop;
			}
			break;
		}
	}

step6:

	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if ((thflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, th->th_seq) ||
	    (tp->snd_wl1 == th->th_seq && (SEQ_LT(tp->snd_wl2, th->th_ack) ||
	     (tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd))))) {
		/* keep track of pure window updates */
		/*
		 * samkumar: There used to be an if statement here that would check if
		 * this is a "pure" window update (tlen == 0 &&
		 * tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd) and keep
		 * statistics for how often that happens.
		 */
		tp->snd_wnd = tiwin;
		tp->snd_wl1 = th->th_seq;
		tp->snd_wl2 = th->th_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */
	/*
	 * samkumar: TCPlp does not support the urgent pointer, so we omit all
	 * urgent-pointer-related processing and buffering. The code below is the
	 * code that was in the "else" case that handles no valid urgent data in
	 * the received packet.
	 */
	{
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
	}

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((tlen || (thflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		tcp_seq save_start = th->th_seq;
		/*
		 * samkumar: I removed a call to m_adj(m, drop_hdrlen), which intends
		 * to drop data from the mbuf so it can be chained into the receive
		 * header. This is not necessary for TCPlp because we copy the data
		 * anyway; we just add the offset when copying data into the receive
		 * buffer.
		 */
		/*
		 * Insert segment which includes th into TCP reassembly queue
		 * with control block tp.  Set thflags to whether reassembly now
		 * includes a segment with FIN.  This handles the common case
		 * inline (segment is the next to be received on an established
		 * connection, and the queue is empty), avoiding linkage into
		 * and removal from the queue and repetition of various
		 * conversions.
		 * Set DELACK for segments received in order, but ack
		 * immediately when segments are out of order (so
		 * fast retransmit can work).
		 */
		/*
		 * samkumar: I replaced LIST_EMPTY(&tp->t_segq) with the calls to
		 * tpiscantrcv and bmp_isempty on the second line below.
		 */
		if (th->th_seq == tp->rcv_nxt &&
		    (tpiscantrcv(tp) || bmp_isempty(tp->reassbmp, REASSBMP_SIZE(tp))) &&
		    TCPS_HAVEESTABLISHED(tp->t_state)) {
			if (DELAY_ACK(tp, tlen))
				tp->t_flags |= TF_DELACK;
			else
				tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt += tlen;
			thflags = th->th_flags & TH_FIN;

			/*
			 * samkumar: I replaced the code that used to be here (which would
			 * free the mbuf with m_freem(m) if the SBS_CANTRCVMORE flag is set
			 * on so->so_rcv.sb_state, and otherwise call
			 * sbappendstream_locked(&so->so_rcv, m, 0);).
			 */
			if (!tpiscantrcv(tp)) {
				cbuf_write(&tp->recvbuf, msg, otMessageGetOffset(msg) + drop_hdrlen, tlen, cbuf_copy_from_message);
				if (tlen > 0) {
					sig->recvbuf_added = true;
				}
			} else if (tlen > 0) {
				/*
				 * samkumar: We already know tlen != 0, so if we got here, then
				 * it means that we got data after we called SHUT_RD, or after
				 * receiving a FIN. I'm going to drop the connection in this
				 * case. I think FreeBSD might have just dropped the packet
				 * silently, but Linux handles it this way; this seems to be
				 * the right approach to me.
				 */
				tcp_drop(tp, ECONNABORTED);
				goto drop;
			}
			/* NB: sorwakeup_locked() does an implicit unlock. */
			/*
			 * samkumar: There used to be a call to sorwakeup_locked(so); here,
			 * which wakes up any threads waiting for the socket to become
			 * become ready for reading. TCPlp handles its buffering
			 * differently so we do not need to replace this call with
			 * specialized code to handle this.
			 */
		} else if (tpiscantrcv(tp)) {
			/*
			 * samkumar: We will reach this point if we get out-of-order data
			 * on a socket which was shut down with SHUT_RD, or where we
			 * already received a FIN. My response here is to drop the segment
			 * and send an RST.
			 */
			tcp_drop(tp, ECONNABORTED);
			goto drop;
		} else {
			/*
			 * XXX: Due to the header drop above "th" is
			 * theoretically invalid by now.  Fortunately
			 * m_adj() doesn't actually frees any mbufs
			 * when trimming from the head.
			 */
			thflags = tcp_reass(tp, th, &tlen, msg, otMessageGetOffset(msg) + drop_hdrlen, sig);
			tp->t_flags |= TF_ACKNOW;
		}
		// Only place tlen is used after the call to tcp_reass is below
		if (tlen > 0 && (tp->t_flags & TF_SACK_PERMIT))
			tcp_update_sack_list(tp, save_start, save_start + tlen);
		/*
		 * samkumar: This is not me commenting things out; this was already
		 * commented out in the FreeBSD code.
		 */
#if 0
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 * XXX: Unused.
		 */
		if (SEQ_GT(tp->rcv_adv, tp->rcv_nxt))
			len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		else
			len = so->so_rcv.sb_hiwat;
#endif
	} else {
		thflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.
	 */
	if (thflags & TH_FIN) {
		tcplp_sys_log("FIN Processing start");
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			/* samkumar: replace socantrcvmore with tpcantrcvmore */
			tpcantrcvmore(tp);
			/*
			 * If connection is half-synchronized
			 * (ie NEEDSYN flag on) then delay ACK,
			 * so it may be piggybacked when SYN is sent.
			 * Otherwise, since we received a FIN then no
			 * more input can be expected, send ACK now.
			 */
			if (tp->t_flags & TF_NEEDSYN)
				tp->t_flags |= TF_DELACK;
			else
				tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		/*
		 * samkumar: This -2 state is added by me, so that we do not consider
		 * any more FINs in reassembly.
		 */
		if (tp->reass_fin_index != -2) {
			sig->rcvd_fin = true;
			tp->reass_fin_index = -2;
		}
		switch (tp->t_state) {

		/*
		 * In SYN_RECEIVED and ESTABLISHED STATES
		 * enter the CLOSE_WAIT state.
		 */
		case TCPS_SYN_RECEIVED:
			tp->t_starttime = ticks;
			/* FALLTHROUGH */
		case TCPS_ESTABLISHED:
			tcp_state_change(tp, TCPS_CLOSE_WAIT);
			break;

		/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */
		case TCPS_FIN_WAIT_1:
			tcp_state_change(tp, TCPS_CLOSING);
			break;

		/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other
		 * standard timers.
		 */
		case TCPS_FIN_WAIT_2:
			tcp_twstart(tp);
			return;
		}
	}

	/*
	 * samkumar: Remove code for synchronization and debugging, here and in
	 * the labels below. I also removed the line to free the mbuf if it hasn't
	 * been freed already (the line was "m_freem(m)").
	 */
	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) tcp_output(tp);

check_delack:
	if (tp->t_flags & TF_DELACK) {
		tp->t_flags &= ~TF_DELACK;
		tcp_timer_activate(tp, TT_DELACK, tcp_delacktime);
	}
	return;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 *
	 * We can now skip the test for the RST flag since all
	 * paths to this code happen after packets containing
	 * RST have been dropped.
	 *
	 * In the SYN-RECEIVED state, don't send an ACK unless the
	 * segment we received passes the SYN-RECEIVED ACK test.
	 * If it fails send a RST.  This breaks the loop in the
	 * "LAND" DoS attack, and also prevents an ACK storm
	 * between two listening ports that have been sent forged
	 * SYN segments, each with the source address of the other.
	 */
	if (tp->t_state == TCPS_SYN_RECEIVED && (thflags & TH_ACK) &&
	    (SEQ_GT(tp->snd_una, th->th_ack) ||
	     SEQ_GT(th->th_ack, tp->snd_max)) ) {
		rstreason = BANDLIM_RST_OPENPORT;
		goto dropwithreset;
	}

	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	return;

dropwithreset:
	if (tp != NULL) {
		tcp_dropwithreset(ip6, th, tp, instance, tlen, rstreason);
	} else
		tcp_dropwithreset(ip6, th, NULL, instance, tlen, rstreason);
	return;

drop:
	return;
}

/*
 * Parse TCP options and place in tcpopt.
 */
static void
tcp_dooptions(struct tcpopt *to, uint8_t *cp, int cnt, int flags)
{
	int opt, optlen;

	to->to_flags = 0;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			if (cnt < 2)
				break;
			optlen = cp[1];
			if (optlen < 2 || optlen > cnt)
				break;
		}
		switch (opt) {
		case TCPOPT_MAXSEG:
			if (optlen != TCPOLEN_MAXSEG)
				continue;
			if (!(flags & TO_SYN))
				continue;
			to->to_flags |= TOF_MSS;
			bcopy((char *)cp + 2,
			    (char *)&to->to_mss, sizeof(to->to_mss));
			to->to_mss = ntohs(to->to_mss);
			break;
		case TCPOPT_WINDOW:
			if (optlen != TCPOLEN_WINDOW)
				continue;
			if (!(flags & TO_SYN))
				continue;
			to->to_flags |= TOF_SCALE;
			to->to_wscale = min(cp[2], TCP_MAX_WINSHIFT);
			break;
		case TCPOPT_TIMESTAMP:
			if (optlen != TCPOLEN_TIMESTAMP)
				continue;
			to->to_flags |= TOF_TS;
			bcopy((char *)cp + 2,
			    (char *)&to->to_tsval, sizeof(to->to_tsval));
			to->to_tsval = ntohl(to->to_tsval);
			bcopy((char *)cp + 6,
			    (char *)&to->to_tsecr, sizeof(to->to_tsecr));
			to->to_tsecr = ntohl(to->to_tsecr);
			break;
#ifdef TCP_SIGNATURE
		/*
		 * XXX In order to reply to a host which has set the
		 * TCP_SIGNATURE option in its initial SYN, we have to
		 * record the fact that the option was observed here
		 * for the syncache code to perform the correct response.
		 */
		case TCPOPT_SIGNATURE:
			if (optlen != TCPOLEN_SIGNATURE)
				continue;
			to->to_flags |= TOF_SIGNATURE;
			to->to_signature = cp + 2;
			break;
#endif
		case TCPOPT_SACK_PERMITTED:
			if (optlen != TCPOLEN_SACK_PERMITTED)
				continue;
			if (!(flags & TO_SYN))
				continue;
			if (!V_tcp_do_sack)
				continue;
			to->to_flags |= TOF_SACKPERM;
			break;
		case TCPOPT_SACK:
			if (optlen <= 2 || (optlen - 2) % TCPOLEN_SACK != 0)
				continue;
			if (flags & TO_SYN)
				continue;
			to->to_flags |= TOF_SACK;
			to->to_nsacks = (optlen - 2) / TCPOLEN_SACK;
			to->to_sacks = cp + 2;
			break;
		default:
			continue;
		}
	}
}


/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 */
static void
tcp_xmit_timer(struct tcpcb *tp, int rtt)
{
	int delta;

	tp->t_rttupdated++;
	if (tp->t_srtt != 0) {
		/*
		 * srtt is stored as fixed point with 5 bits after the
		 * binary point (i.e., scaled by 8).  The following magic
		 * is equivalent to the smoothing algorithm in rfc793 with
		 * an alpha of .875 (srtt = rtt/8 + srtt*7/8 in fixed
		 * point).  Adjust rtt to origin 0.
		 */
		delta = ((rtt - 1) << TCP_DELTA_SHIFT)
			- (tp->t_srtt >> (TCP_RTT_SHIFT - TCP_DELTA_SHIFT));

		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1;

		/*
		 * We accumulate a smoothed rtt variance (actually, a
		 * smoothed mean difference), then set the retransmit
		 * timer to smoothed rtt + 4 times the smoothed variance.
		 * rttvar is stored as fixed point with 4 bits after the
		 * binary point (scaled by 16).  The following is
		 * equivalent to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).  This replaces
		 * rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= tp->t_rttvar >> (TCP_RTTVAR_SHIFT - TCP_DELTA_SHIFT);
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1;
		if (tp->t_rttbest > tp->t_srtt + tp->t_rttvar)
		    tp->t_rttbest = tp->t_srtt + tp->t_rttvar;
	} else {
		/*
		 * No rtt measurement yet - use the unsmoothed rtt.
		 * Set the variance to half the rtt (so our first
		 * retransmit happens at 3*rtt).
		 */
		tp->t_srtt = rtt << TCP_RTT_SHIFT;
		tp->t_rttvar = rtt << (TCP_RTTVAR_SHIFT - 1);
		tp->t_rttbest = tp->t_srtt + tp->t_rttvar;
	}
	tp->t_rtttime = 0;
	tp->t_rxtshift = 0;

	/*
	 * the retransmit should happen at rtt + 4 * rttvar.
	 * Because of the way we do the smoothing, srtt and rttvar
	 * will each average +1/2 tick of bias.  When we compute
	 * the retransmit timer, we want 1/2 tick of rounding and
	 * 1 extra tick because of +-1/2 tick uncertainty in the
	 * firing of the timer.  The bias will give us exactly the
	 * 1.5 tick we need.  But, because the bias is
	 * statistical, we have to test that we don't drop below
	 * the minimum feasible timer (which is 2 ticks).
	 */
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
		      max(tp->t_rttmin, rtt + 2), TCPTV_REXMTMAX);

#ifdef INSTRUMENT_TCP
	tcplp_sys_log("TCP timer %u %d %d %d", (unsigned int) tcplp_sys_get_millis(), rtt, (int) tp->t_srtt, (int) tp->t_rttvar);
#endif


	/*
	 * We received an ack for a packet that wasn't retransmitted;
	 * it is probably safe to discard any error indications we've
	 * received recently.  This isn't quite right, but close enough
	 * for now (a route might have failed after we sent a segment,
	 * and the return path might not be symmetrical).
	 */
	tp->t_softerror = 0;
}

/*
 * samkumar: Taken from netinet6/in6.c.
 *
 * This function is supposed to check whether the provided address is an
 * IPv6 address of this host. This function, however, is used only as a hint,
 * as the MSS is clamped at V_tcp_v6mssdflt for connections to non-local
 * addresses. It is difficult for us to actually determine if the address
 * belongs to us, so we are conservative and only return 1 (true) if it is
 * obviously so---we keep the part of the function that checks for loopback or
 * link local and remove the rest of the code that checks for the addresses
 * assigned to interfaces. In cases where we return 0 but should have returned
 * 1, we may conservatively clamp the MTU, but that should be OK for TCPlp.
 * In fact, the constants are set such that we'll get the right answer whether
 * we clamp or not, so this shouldn't really matter at all.
 */
int
in6_localaddr(struct in6_addr *in6)
{
	if (IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_ADDR_LINKLOCAL(in6))
		return 1;
	return (0);
}

/*
 * Determine a reasonable value for maxseg size.
 * If the route is known, check route for mtu.
 * If none, use an mss that can be handled on the outgoing interface
 * without forcing IP to fragment.  If no route is found, route has no mtu,
 * or the destination isn't local, use a default, hopefully conservative
 * size (usually 512 or the default IP max size, but no more than the mtu
 * of the interface), as we can't discover anything about intervening
 * gateways or networks.  We also initialize the congestion/slow start
 * window to be a single segment if the destination isn't local.
 * While looking at the routing entry, we also initialize other path-dependent
 * parameters from pre-set or cached values in the routing entry.
 *
 * Also take into account the space needed for options that we
 * send regularly.  Make maxseg shorter by that amount to assure
 * that we can send maxseg amount of data even when the options
 * are present.  Store the upper limit of the length of options plus
 * data in maxopd.
 *
 * NOTE that this routine is only called when we process an incoming
 * segment, or an ICMP need fragmentation datagram. Outgoing SYN/ACK MSS
 * settings are handled in tcp_mssopt().
 */
/*
 * samkumar: Using struct tcpcb instead of the inpcb.
 */
void
tcp_mss_update(struct tcpcb *tp, int offer, int mtuoffer,
    struct hc_metrics_lite *metricptr, struct tcp_ifcap *cap)
{
	/*
	 * samkumar: I removed all IPv4-specific logic and cases, including logic
	 * to check for IPv4 vs. IPv6, as well as all locking and debugging code.
	 */
	int mss = 0;
	uint64_t maxmtu = 0;
	struct hc_metrics_lite metrics;
	int origoffer;
	size_t min_protoh = IP6HDR_SIZE + sizeof (struct tcphdr);

	if (mtuoffer != -1) {
		KASSERT(offer == -1, ("%s: conflict", __func__));
		offer = mtuoffer - min_protoh;
	}
	origoffer = offer;

	maxmtu = tcp_maxmtu6(tp, cap);
	tp->t_maxopd = tp->t_maxseg = V_tcp_v6mssdflt;

	/*
	 * No route to sender, stay with default mss and return.
	 */
	if (maxmtu == 0) {
		/*
		 * In case we return early we need to initialize metrics
		 * to a defined state as tcp_hc_get() would do for us
		 * if there was no cache hit.
		 */
		if (metricptr != NULL)
			bzero(metricptr, sizeof(struct hc_metrics_lite));
		return;
	}

	/* What have we got? */
	switch (offer) {
		case 0:
			/*
			 * Offer == 0 means that there was no MSS on the SYN
			 * segment, in this case we use tcp_mssdflt as
			 * already assigned to t_maxopd above.
			 */
			offer = tp->t_maxopd;
			break;

		case -1:
			/*
			 * Offer == -1 means that we didn't receive SYN yet.
			 */
			/* FALLTHROUGH */

		default:
			/*
			 * Prevent DoS attack with too small MSS. Round up
			 * to at least minmss.
			 */
			offer = max(offer, V_tcp_minmss);
	}

	/*
	 * rmx information is now retrieved from tcp_hostcache.
	 */
	tcp_hc_get(tp, &metrics);
	if (metricptr != NULL)
		bcopy(&metrics, metricptr, sizeof(struct hc_metrics_lite));

	/*
	 * If there's a discovered mtu in tcp hostcache, use it.
	 * Else, use the link mtu.
	 */
	if (metrics.rmx_mtu)
		mss = min(metrics.rmx_mtu, maxmtu) - min_protoh;
	else {
		mss = maxmtu - min_protoh;
		if (!V_path_mtu_discovery &&
		    !in6_localaddr(&tp->faddr))
			mss = min(mss, V_tcp_v6mssdflt);
		/*
		 * XXX - The above conditional (mss = maxmtu - min_protoh)
		 * probably violates the TCP spec.
		 * The problem is that, since we don't know the
		 * other end's MSS, we are supposed to use a conservative
		 * default.  But, if we do that, then MTU discovery will
		 * never actually take place, because the conservative
		 * default is much less than the MTUs typically seen
		 * on the Internet today.  For the moment, we'll sweep
		 * this under the carpet.
		 *
		 * The conservative default might not actually be a problem
		 * if the only case this occurs is when sending an initial
		 * SYN with options and data to a host we've never talked
		 * to before.  Then, they will reply with an MSS value which
		 * will get recorded and the new parameters should get
		 * recomputed.  For Further Study.
		 */
	}
	mss = min(mss, offer);

	/*
	 * Sanity check: make sure that maxopd will be large
	 * enough to allow some data on segments even if the
	 * all the option space is used (40bytes).  Otherwise
	 * funny things may happen in tcp_output.
	 */
	/*
	 * samkumar: When I was experimenting with different MSS values, I had
	 * changed this to "mss = max(mss, TCP_MAXOLEN + 1);" but I am changing it
	 * back for the version that will be merged into OpenThread.
	 */
	mss = max(mss, 64);

	/*
	 * maxopd stores the maximum length of data AND options
	 * in a segment; maxseg is the amount of data in a normal
	 * segment.  We need to store this value (maxopd) apart
	 * from maxseg, because now every segment carries options
	 * and thus we normally have somewhat less data in segments.
	 */
	tp->t_maxopd = mss;

	/*
	 * origoffer==-1 indicates that no segments were received yet.
	 * In this case we just guess.
	 */
	if ((tp->t_flags & (TF_REQ_TSTMP|TF_NOOPT)) == TF_REQ_TSTMP &&
	    (origoffer == -1 ||
	     (tp->t_flags & TF_RCVD_TSTMP) == TF_RCVD_TSTMP))
		mss -= TCPOLEN_TSTAMP_APPA;

	tp->t_maxseg = mss;
}

void
tcp_mss(struct tcpcb *tp, int offer)
{
	struct hc_metrics_lite metrics;
	struct tcp_ifcap cap;

	KASSERT(tp != NULL, ("%s: tp == NULL", __func__));

	bzero(&cap, sizeof(cap));
	tcp_mss_update(tp, offer, -1, &metrics, &cap);

	/*
	 * samkumar: There used to be code below that might modify the MSS, but I
	 * removed all of it (see the comments below for the reason). It used to
	 * read tp->t_maxseg into the local variable mss, modify mss, and then
	 * reassign tp->t_maxseg to mss. I've kept the assignments, commented out,
	 * for clarity.
	 */
	//mss = tp->t_maxseg;

	/*
	 * If there's a pipesize, change the socket buffer to that size,
	 * don't change if sb_hiwat is different than default (then it
	 * has been changed on purpose with setsockopt).
	 * Make the socket buffers an integral number of mss units;
	 * if the mss is larger than the socket buffer, decrease the mss.
	 */

	/*
	 * samkumar: There used to be code here would would limit the MSS to at
	 * most the size of the send buffer, and then round up the send buffer to
	 * a multiple of the MSS using
	 * "sbreserve_locked(&so->so_snd, bufsize, so, NULL);". With TCPlp, we do
	 * not do this, because the linked buffer used at the send buffer doesn't
	 * have a real limit. Had we used a circular buffer, then limiting the MSS
	 * to the buffer size would have made sense, but we still would not be able
	 * to resize the send buffer because it is not allocated by TCPlp.
	 */

	/*
	 * samkumar: See the comment above about me removing code that modifies
	 * the MSS, making this assignment and the one above both unnecessary.
	 */
	//tp->t_maxseg = mss;

	/*
	 * samkumar: There used to be code here that would round up the receive
	 * buffer size to a multiple of the MSS, assuming that the receive buffer
	 * size is bigger than the MSS. The new buffer size is set using
	 * "sbreserve_locked(&so->so_rcv, bufsize, so, NULL);". In TCPlp, the
	 * buffer is not allocated by TCPlp so I removed the code for this.
	 */
	/*
	 * samkumar: There used to be code here to handle TCP Segmentation
	 * Offloading (TSO); I removed it becuase we don't support that in TCPlp.
	 */
}

/*
 * Determine the MSS option to send on an outgoing SYN.
 */
/*
 * samkumar: In the signature, changed "struct in_conninfo *inc" to
 * "struct tcpcb* tp".
 */
int
tcp_mssopt(struct tcpcb* tp)
{
	/*
	 * samkumar: I removed all processing code specific to IPv4, or to decide
	 * between IPv4 and IPv6. This is OK because TCPlp assumes IPv6.
	 */
	int mss = 0;
	uint64_t maxmtu = 0;
	uint64_t thcmtu = 0;
	size_t min_protoh;

	KASSERT(tp != NULL, ("tcp_mssopt with NULL tcpcb pointer"));

	mss = V_tcp_v6mssdflt;
	maxmtu = tcp_maxmtu6(tp, NULL);
	min_protoh = IP6HDR_SIZE + sizeof(struct tcphdr);

	thcmtu = tcp_hc_getmtu(tp); /* IPv4 and IPv6 */

	if (maxmtu && thcmtu)
		mss = min(maxmtu, thcmtu) - min_protoh;
	else if (maxmtu || thcmtu)
		mss = max(maxmtu, thcmtu) - min_protoh;

	return (mss);
}

/*
 * On a partial ack arrives, force the retransmission of the
 * next unacknowledged segment.  Do not clear tp->t_dupacks.
 * By setting snd_nxt to ti_ack, this forces retransmission timer to
 * be started again.
 */
static void
tcp_newreno_partial_ack(struct tcpcb *tp, struct tcphdr *th)
{
	tcp_seq onxt = tp->snd_nxt;
	uint64_t  ocwnd = tp->snd_cwnd;

	tcp_timer_activate(tp, TT_REXMT, 0);
	tp->t_rtttime = 0;
	tp->snd_nxt = th->th_ack;
	/*
	 * Set snd_cwnd to one segment beyond acknowledged offset.
	 * (tp->snd_una has not yet been updated when this function is called.)
	 */
	tp->snd_cwnd = tp->t_maxseg + BYTES_THIS_ACK(tp, th);
	tp->t_flags |= TF_ACKNOW;
#ifdef INSTRUMENT_TCP
	tcplp_sys_log("TCP Partial_ACK");
#endif
	(void) tcp_output(tp);
	tp->snd_cwnd = ocwnd;
	if (SEQ_GT(onxt, tp->snd_nxt))
		tp->snd_nxt = onxt;
	/*
	 * Partial window deflation.  Relies on fact that tp->snd_una
	 * not updated yet.
	 */
	if (tp->snd_cwnd > BYTES_THIS_ACK(tp, th))
		tp->snd_cwnd -= BYTES_THIS_ACK(tp, th);
	else
		tp->snd_cwnd = 0;
	tp->snd_cwnd += tp->t_maxseg;
#ifdef INSTRUMENT_TCP
	tcplp_sys_log("TCP Partial_ACK_final %d", (int) tp->snd_cwnd);
#endif
}
