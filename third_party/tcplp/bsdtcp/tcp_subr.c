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
#include "../lib/bitmap.h"
#include "../lib/cbuf.h"
#include "cc.h"

#include "tcp_const.h"

/* EXTERN DECLARATIONS FROM TCP_TIMER.H */
#if 0 // I put these in the enum below
int tcp_keepinit;		/* time to establish connection */
int tcp_keepidle;		/* time before keepalive probes begin */
int tcp_keepintvl;		/* time between keepalive probes */
//int tcp_keepcnt;			/* number of keepalives */
int tcp_delacktime;		/* time before sending a delayed ACK */
int tcp_maxpersistidle;
int tcp_rexmit_slop;
int tcp_msl;
//int tcp_ttl;			/* time to live for TCP segs */
int tcp_finwait2_timeout;
#endif
int tcp_rexmit_min;

// A simple linear congruential number generator
tcp_seq seed = (tcp_seq) 0xbeaddeed;
tcp_seq tcp_new_isn(struct tcpcb* tp) {
    seed = (((tcp_seq) 0xfaded011) * seed) + (tcp_seq) 0x1ead1eaf;
    return seed;
}

/* This is based on tcp_init in tcp_subr.c. */
void tcp_init(void) {
#if 0 // I'M NOT USING A HASH TABLE TO STORE TCBS.
	const char *tcbhash_tuneable;
	int hashsize;

	tcbhash_tuneable = "net.inet.tcp.tcbhashsize";

	if (hhook_head_register(HHOOK_TYPE_TCP, HHOOK_TCP_EST_IN,
	    &V_tcp_hhh[HHOOK_TCP_EST_IN], HHOOK_NOWAIT|HHOOK_HEADISINVNET) != 0)
		printf("%s: WARNING: unable to register helper hook\n", __func__);
	if (hhook_head_register(HHOOK_TYPE_TCP, HHOOK_TCP_EST_OUT,
	    &V_tcp_hhh[HHOOK_TCP_EST_OUT], HHOOK_NOWAIT|HHOOK_HEADISINVNET) != 0)
		printf("%s: WARNING: unable to register helper hook\n", __func__);

	hashsize = TCBHASHSIZE;
	TUNABLE_INT_FETCH(tcbhash_tuneable, &hashsize);
	if (hashsize == 0) {
		/*
		 * Auto tune the hash size based on maxsockets.
		 * A perfect hash would have a 1:1 mapping
		 * (hashsize = maxsockets) however it's been
		 * suggested that O(2) average is better.
		 */
		hashsize = maketcp_hashsize(maxsockets / 4);
		/*
		 * Our historical default is 512,
		 * do not autotune lower than this.
		 */
		if (hashsize < 512)
			hashsize = 512;
		if (bootverbose)
			printf("%s: %s auto tuned to %d\n", __func__,
			    tcbhash_tuneable, hashsize);
	}
	/*
	 * We require a hashsize to be a power of two.
	 * Previously if it was not a power of two we would just reset it
	 * back to 512, which could be a nasty surprise if you did not notice
	 * the error message.
	 * Instead what we do is clip it to the closest power of two lower
	 * than the specified hash value.
	 */
	if (!powerof2(hashsize)) {
		int oldhashsize = hashsize;

		hashsize = maketcp_hashsize(hashsize);
		/* prevent absurdly low value */
		if (hashsize < 16)
			hashsize = 16;
		printf("%s: WARNING: TCB hash size not a power of 2, "
		    "clipped from %d to %d.\n", __func__, oldhashsize,
		    hashsize);
	}
	in_pcbinfo_init(&V_tcbinfo, "tcp", &V_tcb, hashsize, hashsize,
	    "tcp_inpcb", tcp_inpcb_init, NULL, UMA_ZONE_NOFREE,
	    IPI_HASHFIELDS_4TUPLE);

	/*
	 * These have to be type stable for the benefit of the timers.
	 */
	V_tcpcb_zone = uma_zcreate("tcpcb", sizeof(struct tcpcb_mem),
	    NULL, NULL, NULL, NULL, UMA_ALIGN_PTR, UMA_ZONE_NOFREE);
	uma_zone_set_max(V_tcpcb_zone, maxsockets);
	uma_zone_set_warning(V_tcpcb_zone, "kern.ipc.maxsockets limit reached");

	tcp_tw_init();
	syncache_init();
	tcp_hc_init();

	TUNABLE_INT_FETCH("net.inet.tcp.sack.enable", &V_tcp_do_sack);
	V_sack_hole_zone = uma_zcreate("sackhole", sizeof(struct sackhole),
	    NULL, NULL, NULL, NULL, UMA_ALIGN_PTR, UMA_ZONE_NOFREE);

	/* Skip initialization of globals for non-default instances. */
	if (!IS_DEFAULT_VNET(curvnet))
		return;

	tcp_reass_global_init();
#endif
	/* XXX virtualize those bellow? */

#if 0 // To save memory, I put these in an enum, defined above
	tcp_delacktime = TCPTV_DELACK;
	tcp_keepinit = TCPTV_KEEP_INIT;
	tcp_keepidle = TCPTV_KEEP_IDLE;
	tcp_keepintvl = TCPTV_KEEPINTVL;
	tcp_maxpersistidle = TCPTV_KEEP_IDLE;
	tcp_msl = TCPTV_MSL;
#endif
	tcp_rexmit_min = TCPTV_MIN;
	if (tcp_rexmit_min < 1)
		tcp_rexmit_min = 1;
#if 0
	tcp_rexmit_slop = TCPTV_CPU_VAR;
	tcp_finwait2_timeout = TCPTV_FINWAIT2_TIMEOUT;
#endif
	//tcp_tcbhashsize = hashsize;

#if 0 // Ignoring this for now (may bring it back later if necessary)
	if (tcp_soreceive_stream) {
#ifdef INET
		tcp_usrreqs.pru_soreceive = soreceive_stream;
#endif
#ifdef INET6
		tcp6_usrreqs.pru_soreceive = soreceive_stream;
#endif /* INET6 */
	}

#ifdef INET6
#define TCP_MINPROTOHDR (sizeof(struct ip6_hdr) + sizeof(struct tcphdr))
#else /* INET6 */
#define TCP_MINPROTOHDR (sizeof(struct tcpiphdr))
#endif /* INET6 */
	if (max_protohdr < TCP_MINPROTOHDR)
		max_protohdr = TCP_MINPROTOHDR;
	if (max_linkhdr + TCP_MINPROTOHDR > MHLEN)
		panic("tcp_init");
#undef TCP_MINPROTOHDR

	ISN_LOCK_INIT();
	EVENTHANDLER_REGISTER(shutdown_pre_sync, tcp_fini, NULL,
		SHUTDOWN_PRI_DEFAULT);
	EVENTHANDLER_REGISTER(maxsockets_change, tcp_zone_change, NULL,
		EVENTHANDLER_PRI_ANY);
#ifdef TCPPCAP
	tcp_pcap_init();
#endif
#endif
}

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
    tcplp_sys_log("Socket %p: %s --> %s\n", tp, tcpstates[tp->t_state], tcpstates[newstate]);
	tp->t_state = newstate;

    // samkumar: may need to do other actions too...
    tcplp_sys_on_state_change(tp, newstate);
#if 0
	TCP_PROBE6(state__change, NULL, tp, NULL, tp, NULL, pstate);
#endif
}

 /* This is based on tcp_newtcb in tcp_subr.c, and tcp_usr_attach in tcp_usrreq.c.
    The length of the reassembly bitmap is fixed at ceil(0.125 * recvbuflen). */
__attribute__((used)) void initialize_tcb(struct tcpcb* tp) {
	uint32_t ticks = tcplp_sys_get_ticks();

    memset(((uint8_t*) tp) + offsetof(struct tcpcb, laddr), 0x00, sizeof(struct tcpcb) - offsetof(struct tcpcb, laddr));
    tp->reass_fin_index = -1;
    // if (laddr != NULL) {
    //     memcpy(&tp->laddr, laddr, sizeof(tp->laddr));
    // }
    // tp->lport = lport;
    // Congestion control algorithm.

    // I only implement New Reno, so I'm not going to waste memory in each socket describing what the congestion algorithm is; it's always New Reno
//    CC_ALGO(tp) = CC_DEFAULT();
//    tp->ccv->type = IPPROTO_TCP;
	tp->ccv->ccvc.tcp = tp;

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
	tp->t_rttmin = tcp_rexmit_min;
	tp->t_rxtcur = TCPTV_RTOBASE;
	tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->t_rcvtime = ticks;

	/* From tcp_usr_attach in tcp_usrreq.c. */
	tp->t_state = TCP6S_CLOSED;

    // Added by Sam: Need to initialize the sackhole pool.
	tcp_sack_init(tp);
}

void
tcp_discardcb(struct tcpcb *tp)
{
	tcp_cancel_timers(tp);

	/* Allow the CC algorithm to clean up after itself. */
	if (CC_ALGO(tp)->cb_destroy != NULL)
		CC_ALGO(tp)->cb_destroy(tp->ccv);

//	khelp_destroy_osd(tp->osd);

//	CC_ALGO(tp) = NULL;

	tcp_free_sackholes(tp);
#if 0 // Most of this is not applicable anymore. Above, I've copied the relevant parts.
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
#ifdef INET6
	int isipv6 = (inp->inp_vflag & INP_IPV6) != 0;
#endif /* INET6 */
	int released;

	INP_WLOCK_ASSERT(inp);

	/*
	 * Make sure that all of our timers are stopped before we delete the
	 * PCB.
	 *
	 * If stopping a timer fails, we schedule a discard function in same
	 * callout, and the last discard function called will take care of
	 * deleting the tcpcb.
	 */
	tcp_timer_stop(tp, TT_REXMT);
	tcp_timer_stop(tp, TT_PERSIST);
	tcp_timer_stop(tp, TT_KEEP);
	tcp_timer_stop(tp, TT_2MSL);
	tcp_timer_stop(tp, TT_DELACK);

	/*
	 * If we got enough samples through the srtt filter,
	 * save the rtt and rttvar in the routing entry.
	 * 'Enough' is arbitrarily defined as 4 rtt samples.
	 * 4 samples is enough for the srtt filter to converge
	 * to within enough % of the correct value; fewer samples
	 * and we could save a bogus rtt. The danger is not high
	 * as tcp quickly recovers from everything.
	 * XXX: Works very well but needs some more statistics!
	 */
	if (tp->t_rttupdated >= 4) {
		struct hc_metrics_lite metrics;
		uint64_t ssthresh;

		bzero(&metrics, sizeof(metrics));
		/*
		 * Update the ssthresh always when the conditions below
		 * are satisfied. This gives us better new start value
		 * for the congestion avoidance for new connections.
		 * ssthresh is only set if packet loss occured on a session.
		 *
		 * XXXRW: 'so' may be NULL here, and/or socket buffer may be
		 * being torn down.  Ideally this code would not use 'so'.
		 */
		ssthresh = tp->snd_ssthresh;
		if (ssthresh != 0 && ssthresh < so->so_snd.sb_hiwat / 2) {
			/*
			 * convert the limit from user data bytes to
			 * packets then to packet data bytes.
			 */
			ssthresh = (ssthresh + tp->t_maxseg / 2) / tp->t_maxseg;
			if (ssthresh < 2)
				ssthresh = 2;
			ssthresh *= (uint64_t)(tp->t_maxseg +
#ifdef INET6
			    (isipv6 ? sizeof (struct ip6_hdr) +
				sizeof (struct tcphdr) :
#endif
				sizeof (struct tcpiphdr)
#ifdef INET6
			    )
#endif
			    );
		} else
			ssthresh = 0;
		metrics.rmx_ssthresh = ssthresh;

		metrics.rmx_rtt = tp->t_srtt;
		metrics.rmx_rttvar = tp->t_rttvar;
		metrics.rmx_cwnd = tp->snd_cwnd;
		metrics.rmx_sendpipe = 0;
		metrics.rmx_recvpipe = 0;

		tcp_hc_update(&inp->inp_inc, &metrics);
	}

	/* free the reassembly queue, if any */
	tcp_reass_flush(tp);

#ifdef TCP_OFFLOAD
	/* Disconnect offload device, if any. */
	if (tp->t_flags & TF_TOE)
		tcp_offload_detach(tp);
#endif

	tcp_free_sackholes(tp);

#ifdef TCPPCAP
	/* Free the TCP PCAP queues. */
	tcp_pcap_drain(&(tp->t_inpkts));
	tcp_pcap_drain(&(tp->t_outpkts));
#endif

	/* Allow the CC algorithm to clean up after itself. */
	if (CC_ALGO(tp)->cb_destroy != NULL)
		CC_ALGO(tp)->cb_destroy(tp->ccv);

	khelp_destroy_osd(tp->osd);

	CC_ALGO(tp) = NULL;
	inp->inp_ppcb = NULL;
	if ((tp->t_timers->tt_flags & TT_MASK) == 0) {
		/* We own the last reference on tcpcb, let's free it. */
		tp->t_inpcb = NULL;
		uma_zfree(V_tcpcb_zone, tp);
		released = in_pcbrele_wlocked(inp);
		KASSERT(!released, ("%s: inp %p should not have been released "
			"here", __func__, inp));
	}
#endif
}


 /*
 * Attempt to close a TCP control block, marking it as dropped, and freeing
 * the socket if we hold the only reference.
 */
struct tcpcb *
tcp_close(struct tcpcb *tp)
{
	// Seriously, it looks like this is all this function does, that I'm concerned with
	tcp_state_change(tp, TCP6S_CLOSED); // for the print statement
	tcp_discardcb(tp);
	// Don't reset the TCB by calling initialize_tcb, since that overwrites the buffer contents.
	return tp;
#if 0
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so;

	INP_INFO_LOCK_ASSERT(&V_tcbinfo);
	INP_WLOCK_ASSERT(inp);

#ifdef TCP_OFFLOAD
	if (tp->t_state == TCPS_LISTEN)
		tcp_offload_listen_stop(tp);
#endif
	in_pcbdrop(inp);
	TCPSTAT_INC(tcps_closed);
	KASSERT(inp->inp_socket != NULL, ("tcp_close: inp_socket NULL"));
	so = inp->inp_socket;
	soisdisconnected(so);
	if (inp->inp_flags & INP_SOCKREF) {
		KASSERT(so->so_state & SS_PROTOREF,
		    ("tcp_close: !SS_PROTOREF"));
		inp->inp_flags &= ~INP_SOCKREF;
		INP_WUNLOCK(inp);
		ACCEPT_LOCK();
		SOCK_LOCK(so);
		so->so_state &= ~SS_PROTOREF;
		sofree(so);
		return (NULL);
	}
	return (tp);
#endif
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Allocates an mbuf and fills in a skeletal tcp/ip header.  The only
 * use for this function is in keepalives, which use tcp_respond.
 */
 // NOTE: I CHANGED THE SIGNATURE OF THIS FUNCTION
void
tcpip_maketemplate(struct tcpcb* tp, struct tcptemp* t)
{
	//struct tcptemp *t;
#if 0
	t = malloc(sizeof(*t), M_TEMP, M_NOWAIT);
#endif
	//t = ip_malloc(sizeof(struct tcptemp));
	//if (t == NULL)
	//	return (NULL);
	tcpip_fillheaders(tp, (void *)&t->tt_ipgen, (void *)&t->tt_t);
	//return (t);
}

/*
 * Fill in the IP and TCP headers for an outgoing packet, given the tcpcb.
 * tcp_template used to store this data in mbufs, but we now recopy it out
 * of the tcpcb each time to conserve mbufs.
 */
 // NOTE: HAS A DIFFERENT SIGNATURE FROM THE ORIGINAL FUNCTION IN tcp_subr.c
void
tcpip_fillheaders(struct tcpcb* tp, otMessageInfo* ip_ptr, void *tcp_ptr)
{
	struct tcphdr *th = (struct tcphdr *)tcp_ptr;

//	INP_WLOCK_ASSERT(inp);

/* I fill in the IP header elsewhere. In send_message in BsdTcpP.nc, to be exact. */
#if 0
#ifdef INET6
	if ((inp->inp_vflag & INP_IPV6) != 0) {
		struct ip6_hdr *ip6;

		ip6 = (struct ip6_hdr *)ip_ptr;
		ip6->ip6_flow = (ip6->ip6_flow & ~IPV6_FLOWINFO_MASK) |
			(inp->inp_flow & IPV6_FLOWINFO_MASK);
		ip6->ip6_vfc = (ip6->ip6_vfc & ~IPV6_VERSION_MASK) |
			(IPV6_VERSION & IPV6_VERSION_MASK);
		ip6->ip6_nxt = IPPROTO_TCP;
		ip6->ip6_plen = htons(sizeof(struct tcphdr));
		ip6->ip6_src = inp->in6p_laddr;
		ip6->ip6_dst = inp->in6p_faddr;
	}
#endif /* INET6 */
#if defined(INET6) && defined(INET)
	else
#endif
#ifdef INET
	{
		struct ip *ip;

		ip = (struct ip *)ip_ptr;
		ip->ip_v = IPVERSION;
		ip->ip_hl = 5;
		ip->ip_tos = inp->inp_ip_tos;
		ip->ip_len = 0;
		ip->ip_id = 0;
		ip->ip_off = 0;
		ip->ip_ttl = inp->inp_ip_ttl;
		ip->ip_sum = 0;
		ip->ip_p = IPPROTO_TCP;
		ip->ip_src = inp->inp_laddr;
		ip->ip_dst = inp->inp_faddr;
	}
#endif /* INET */
#endif
	/* Fill in the IP header */

    //ip6->ip6_vfc = 0x60;
    //memset(&ip6->ip6_src, 0x00, sizeof(ip6->ip6_src));
    memset(ip_ptr, 0x00, sizeof(otMessageInfo));
    memcpy(&ip_ptr->mSockAddr, &tp->laddr, sizeof(ip_ptr->mSockAddr));
    memcpy(&ip_ptr->mPeerAddr, &tp->faddr, sizeof(ip_ptr->mPeerAddr));
    ip_ptr->mVersionClassFlow = 0x60000000;
    //ip6->ip6_dst = tp->faddr;
	/* Fill in the TCP header */
	//th->th_sport = inp->inp_lport;
	//th->th_dport = inp->inp_fport;
	th->th_sport = tp->lport;
	th->th_dport = tp->fport;
	th->th_seq = 0;
	th->th_ack = 0;
	th->th_x2 = 0;
	th->th_off = 5;
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
/* Original signature was
void
tcp_respond(struct tcpcb *tp, void *ipgen, struct tcphdr *th, struct mbuf *m,
    tcp_seq ack, tcp_seq seq, int flags)
*/
void
tcp_respond(struct tcpcb *tp, otInstance* instance, struct ip6_hdr* ip6gen, struct tcphdr *thgen,
    tcp_seq ack, tcp_seq seq, int flags)
{
    /* Again, I rewrote this function for the RIOT port of the code. */
    /*gnrc_pktsnip_t* tcpsnip = gnrc_pktbuf_add(NULL, NULL, sizeof(struct tcphdr), GNRC_NETTYPE_TCP);
    if (tcpsnip == NULL) {
        return; // drop the message;
    }
    gnrc_pktsnip_t* ip6snip = gnrc_pktbuf_add(tcpsnip, NULL, sizeof(struct ip6_hdr), GNRC_NETTYPE_IPV6);
    if (ip6snip == NULL) {
        return; // drop the message;
    }*/

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
    //struct tcphdr* nth = tcpsnip->data;
    //struct ip6_hdr* ip6 = ip6snip->data;
    int win = 0;
    if (tp != NULL) {
		if (!(flags & TH_RST)) {
			win = cbuf_free_space(&tp->recvbuf);
			if (win > (long)TCP_MAXWIN << tp->rcv_scale)
				win = (long)TCP_MAXWIN << tp->rcv_scale;
		}
	}
    /*ip6->ip6_vfc = 0x60;
    ip6->ip6_nxt = IANA_TCP;
	ip6->ip6_plen = htons(sizeof(struct tcphdr));
	ip6->ip6_src = ip6gen->ip6_dst;
	ip6->ip6_dst = ip6gen->ip6_src;*/
    memset(&ip6info, 0x00, sizeof(otMessageInfo));
    memcpy(&ip6info.mSockAddr, &ip6gen->ip6_dst, sizeof(ip6info.mSockAddr));
    memcpy(&ip6info.mPeerAddr, &ip6gen->ip6_src, sizeof(ip6info.mPeerAddr));
    ip6info.mVersionClassFlow = 0x60000000;
	nth->th_sport = thgen->th_dport;
	nth->th_dport = thgen->th_sport;
	nth->th_seq = htonl(seq);
	nth->th_ack = htonl(ack);
	nth->th_x2 = 0;
	nth->th_off = sizeof(struct tcphdr) >> 2;
	nth->th_flags = flags;
	if (tp != NULL)
		nth->th_win = htons((uint16_t) (win >> tp->rcv_scale));
	else
		nth->th_win = htons((uint16_t)win);
	nth->th_urp = 0;
    nth->th_sum = 0;

    otMessageWrite(message, 0, &th, sizeof(struct tcphdr));

    tcplp_sys_send_message(instance, message, &ip6info);
#if 0
	/* Essentially all the code needs to be discarded because I need to send packets the TinyOS way.
	   There are some parts that I copied; I didn't want to comment out everything except the few
	   lines I needed since I felt that this would be cleaner. */
	struct ip6_packet* msg;
	struct ip6_hdr* ip6;
	struct tcphdr* nth;
	struct ip_iovec* iov;
	int alen = sizeof(struct ip6_packet) + sizeof(struct tcphdr) + sizeof(struct ip_iovec);
	char* bufreal = ip_malloc(alen + 3);
	int win = 0;
	char* buf;
	if (bufreal == NULL) {
		return; // drop the message
	}
	if (tp != NULL) {
		if (!(flags & TH_RST)) {
			win = cbuf_free_space(&tp->recvbuf);
			if (win > (long)TCP_MAXWIN << tp->rcv_scale)
				win = (long)TCP_MAXWIN << tp->rcv_scale;
		}
	}
	buf = (char*) (((uint32_t) (bufreal + 3)) & 0xFFFFFFFCu);
	memset(buf, 0, alen); // for safe measure
	msg = (struct ip6_packet*) buf;
  	iov = (struct ip_iovec*) (buf + alen - sizeof(struct ip_iovec));
  	iov->iov_next = NULL;
	iov->iov_len = sizeof(struct tcphdr);
	iov->iov_base = (void*) (msg + 1);
	msg->ip6_data = iov;
	ip6 = &msg->ip6_hdr;
	ip6->ip6_nxt = IANA_TCP;
	ip6->ip6_plen = htons(sizeof(struct tcphdr));
	ip6->ip6_src = ip6gen->ip6_dst;
	ip6->ip6_dst = ip6gen->ip6_src;
	nth = (struct tcphdr*) (ip6 + 1);
	nth->th_sport = thgen->th_dport;
	nth->th_dport = thgen->th_sport;
	nth->th_seq = htonl(seq);
	nth->th_ack = htonl(ack);
	nth->th_x2 = 0;
	nth->th_off = sizeof (struct tcphdr) >> 2;
	nth->th_flags = flags;
	if (tp != NULL)
		nth->th_win = htons((uint16_t) (win >> tp->rcv_scale));
	else
		nth->th_win = htons((uint16_t)win);
	nth->th_urp = 0;
	send_message(tp, msg, nth, sizeof(struct tcphdr));
	ip_free(bufreal);
#endif
#if 0
	int tlen;
	int win = 0;
	struct ip *ip;
	struct tcphdr *nth;
#ifdef INET6
	struct ip6_hdr *ip6;
	int isipv6;
#endif /* INET6 */
	int ipflags = 0;
	struct inpcb *inp;
#if 0
	KASSERT(tp != NULL || m != NULL, ("tcp_respond: tp and m both NULL"));

#ifdef INET6
	isipv6 = ((struct ip *)ipgen)->ip_v == (IPV6_VERSION >> 4);
	ip6 = ipgen;
#endif /* INET6 */
	ip = ipgen;
#endif

	if (tp != NULL) {
		inp = tp->t_inpcb;
		KASSERT(inp != NULL, ("tcp control block w/o inpcb"));
		INP_WLOCK_ASSERT(inp);
	} else
		inp = NULL;

	if (tp != NULL) {
		if (!(flags & TH_RST)) {
			win = sbspace(&inp->inp_socket->so_rcv);
			if (win > (long)TCP_MAXWIN << tp->rcv_scale)
				win = (long)TCP_MAXWIN << tp->rcv_scale;
		}
	}
	if (m == NULL) {
		m = m_gethdr(M_NOWAIT, MT_DATA);
		if (m == NULL)
			return;
		tlen = 0;
		m->m_data += max_linkhdr;
#ifdef INET6
		if (isipv6) {
			bcopy((caddr_t)ip6, mtod(m, caddr_t),
			      sizeof(struct ip6_hdr));
			ip6 = mtod(m, struct ip6_hdr *);
			nth = (struct tcphdr *)(ip6 + 1);
		} else
#endif /* INET6 */
		{
			bcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
			ip = mtod(m, struct ip *);
			nth = (struct tcphdr *)(ip + 1);
		}
		bcopy((caddr_t)th, (caddr_t)nth, sizeof(struct tcphdr));
		flags = TH_ACK;
	} else {
		/*
		 *  reuse the mbuf.
		 * XXX MRT We inherrit the FIB, which is lucky.
		 */
		m_freem(m->m_next);
		m->m_next = NULL;
		m->m_data = (caddr_t)ipgen;
		/* m_len is set later */
		tlen = 0;
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
#ifdef INET6
		if (isipv6) {
			xchg(ip6->ip6_dst, ip6->ip6_src, struct in6_addr);
			nth = (struct tcphdr *)(ip6 + 1);
		} else
#endif /* INET6 */
		{
			xchg(ip->ip_dst.s_addr, ip->ip_src.s_addr, uint32_t);
			nth = (struct tcphdr *)(ip + 1);
		}
		if (th != nth) {
			/*
			 * this is usually a case when an extension header
			 * exists between the IPv6 header and the
			 * TCP header.
			 */
			nth->th_sport = th->th_sport;
			nth->th_dport = th->th_dport;
		}
		xchg(nth->th_dport, nth->th_sport, uint16_t);
#undef xchg
	}
#ifdef INET6
	if (isipv6) {
		ip6->ip6_flow = 0;
		ip6->ip6_vfc = IPV6_VERSION;
		ip6->ip6_nxt = IPPROTO_TCP;
		tlen += sizeof (struct ip6_hdr) + sizeof (struct tcphdr);
		ip6->ip6_plen = htons(tlen - sizeof(*ip6));
	}
#endif
#if defined(INET) && defined(INET6)
	else
#endif
#ifdef INET
	{
		tlen += sizeof (struct tcpiphdr);
		ip->ip_len = htons(tlen);
		ip->ip_ttl = V_ip_defttl;
		if (V_path_mtu_discovery)
			ip->ip_off |= htons(IP_DF);
	}
#endif
	m->m_len = tlen;
	m->m_pkthdr.len = tlen;
	m->m_pkthdr.rcvif = NULL;
#ifdef MAC
	if (inp != NULL) {
		/*
		 * Packet is associated with a socket, so allow the
		 * label of the response to reflect the socket label.
		 */
		INP_WLOCK_ASSERT(inp);
		mac_inpcb_create_mbuf(inp, m);
	} else {
		/*
		 * Packet is not associated with a socket, so possibly
		 * update the label in place.
		 */
		mac_netinet_tcp_reply(m);
	}
#endif
	nth->th_seq = htonl(seq);
	nth->th_ack = htonl(ack);
	nth->th_x2 = 0;
	nth->th_off = sizeof (struct tcphdr) >> 2;
	nth->th_flags = flags;
	if (tp != NULL)
		nth->th_win = htons((uint16_t) (win >> tp->rcv_scale));
	else
		nth->th_win = htons((uint16_t)win);
	nth->th_urp = 0;

	m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
//#ifdef INET6
//	if (isipv6) {
		m->m_pkthdr.csum_flags = CSUM_TCP_IPV6;
		nth->th_sum = in6_cksum_pseudo(ip6,
		    tlen - sizeof(struct ip6_hdr), IPPROTO_TCP, 0);
		ip6->ip6_hlim = in6_selecthlim(tp != NULL ? tp->t_inpcb :
		    NULL, NULL);
//	}
//#endif /* INET6 */
#if 0
#if defined(INET6) && defined(INET)
	else
#endif
#ifdef INET
	{
		m->m_pkthdr.csum_flags = CSUM_TCP;
		nth->th_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr,
		    htons((uint16_t)(tlen - sizeof(struct ip) + ip->ip_p)));
	}
#endif /* INET */
#ifdef TCPDEBUG
	if (tp == NULL || (inp->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_OUTPUT, 0, tp, mtod(m, void *), th, 0);
#endif
	TCP_PROBE3(debug__input, tp, th, mtod(m, const char *));
	if (flags & TH_RST)
		TCP_PROBE5(accept__refused, NULL, NULL, mtod(m, const char *),
		    tp, nth);
#endif
//	TCP_PROBE5(send, NULL, tp, mtod(m, const char *), tp, nth);
//#ifdef INET6
	if (isipv6)
		(void) ip6_output(m, NULL, NULL, ipflags, NULL, NULL, inp);
//#endif /* INET6 */
#if 0
#if defined(INET) && defined(INET6)
	else
#endif
#ifdef INET
		(void) ip_output(m, NULL, NULL, ipflags, NULL, inp);
#endif
#endif
#endif
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
/* Sam: I changed the parameter "errno" to "errnum" since it caused
 * problems during compilation.
 */
struct tcpcb *
tcp_drop(struct tcpcb *tp, int errnum)
{
//	struct socket *so = tp->t_inpcb->inp_socket;

//	INP_INFO_LOCK_ASSERT(&V_tcbinfo);
//	INP_WLOCK_ASSERT(tp->t_inpcb);

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tcp_state_change(tp, TCPS_CLOSED);
		(void) tcp_output(tp);
//		TCPSTAT_INC(tcps_drops);
	}// else
//		TCPSTAT_INC(tcps_conndrops);
	if (errnum == ETIMEDOUT && tp->t_softerror)
		errnum = tp->t_softerror;
//	so->so_error = errnum;
//	return (tcp_close(tp));
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
uint64_t
tcp_maxmtu6(/*struct in_conninfo *inc,*/struct tcpcb* tp, struct tcp_ifcap *cap)
{
	uint64_t maxmtu = 0;

	KASSERT (tp != NULL, ("tcp_maxmtu6 with NULL tcpcb pointer"));
	if (!IN6_IS_ADDR_UNSPECIFIED(&tp->faddr)) {
		maxmtu = FRAMES_PER_SEG * FRAMECAP_6LOWPAN;
	}

	return (maxmtu);

#if 0 // I rewrote this function above
	struct route_in6 sro6;
	struct ifnet *ifp;
	uint64_t maxmtu = 0;

	KASSERT(inc != NULL, ("tcp_maxmtu6 with NULL in_conninfo pointer"));

	bzero(&sro6, sizeof(sro6));
	if (!IN6_IS_ADDR_UNSPECIFIED(&inc->inc6_faddr)) {
		sro6.ro_dst.sin6_family = AF_INET6;
		sro6.ro_dst.sin6_len = sizeof(struct sockaddr_in6);
		sro6.ro_dst.sin6_addr = inc->inc6_faddr;
		in6_rtalloc_ign(&sro6, 0, inc->inc_fibnum);
	}
	if (sro6.ro_rt != NULL) {
		ifp = sro6.ro_rt->rt_ifp;
		if (sro6.ro_rt->rt_mtu == 0)
			maxmtu = IN6_LINKMTU(sro6.ro_rt->rt_ifp);
		else
			maxmtu = min(sro6.ro_rt->rt_mtu,
				     IN6_LINKMTU(sro6.ro_rt->rt_ifp));

		/* Report additional interface capabilities. */
		if (cap != NULL) {
			if (ifp->if_capenable & IFCAP_TSO6 &&
			    ifp->if_hwassist & CSUM_TSO) {
				cap->ifcap |= CSUM_TSO;
				cap->tsomax = ifp->if_hw_tsomax;
				cap->tsomaxsegcount = ifp->if_hw_tsomaxsegcount;
				cap->tsomaxsegsize = ifp->if_hw_tsomaxsegsize;
			}
		}
		RTFREE(sro6.ro_rt);
	}

	return (maxmtu);
#endif
}
