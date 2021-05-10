/*-
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)tcp_timer.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD$
 */

#include <errno.h>
#include <stdio.h>

#include "../tcplp.h"
#include "../lib/cbuf.h"
#include "tcp_fsm.h"
#include "tcp_timer.h"
#include "tcp_var.h"

#include "tcp_const.h"

#if 0
int V_tcp_pmtud_blackhole_detect = 0;
int V_tcp_pmtud_blackhole_failed = 0;
int V_tcp_pmtud_blackhole_activated = 0;
int V_tcp_pmtud_blackhole_activated_min_mss = 0;
#endif

/*
 * TCP timer processing.
 */

void
tcp_timer_delack(/*void *xtp*/struct tcpcb* tp)
{
    KASSERT(tpistimeractive(tp, TT_DELACK), ("Delack timer running, but unmarked\n"));
    tpcleartimeractive(tp, TT_DELACK);
#if 0
	struct tcpcb *tp = xtp;
	struct inpcb *inp;
	CURVNET_SET(tp->t_vnet);

	inp = tp->t_inpcb;
	KASSERT(inp != NULL, ("%s: tp %p tp->t_inpcb == NULL", __func__, tp));
	INP_WLOCK(inp);
	if (callout_pending(&tp->t_timers->tt_delack) ||
	    !callout_active(&tp->t_timers->tt_delack)) {
		INP_WUNLOCK(inp);
		CURVNET_RESTORE();
		return;
	}
	callout_deactivate(&tp->t_timers->tt_delack);
	if ((inp->inp_flags & INP_DROPPED) != 0) {
		INP_WUNLOCK(inp);
		CURVNET_RESTORE();
		return;
	}
	KASSERT((tp->t_timers->tt_flags & TT_STOPPED) == 0,
		("%s: tp %p tcpcb can't be stopped here", __func__, tp));
	KASSERT((tp->t_timers->tt_flags & TT_DELACK) != 0,
		("%s: tp %p delack callout should be running", __func__, tp));
#endif
	tp->t_flags |= TF_ACKNOW;
//	TCPSTAT_INC(tcps_delack);
	(void) tcp_output(tp);
//	INP_WUNLOCK(inp);
//	CURVNET_RESTORE();
}

void
tcp_timer_keep(struct tcpcb* tp)
{
    uint32_t ticks = tcplp_sys_get_ticks();
	/*struct tcptemp *t_template;*/
    struct tcptemp t_template;
	KASSERT(tpistimeractive(tp, TT_KEEP), ("Keep timer running, but unmarked\n"));
	tpcleartimeractive(tp, TT_KEEP); // for our own internal bookkeeping
#if 0 // I already cancel this invocation if it was rescheduled meanwhile
	struct inpcb *inp;
	CURVNET_SET(tp->t_vnet);
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_RLOCK(&V_tcbinfo);
	inp = tp->t_inpcb;
	KASSERT(inp != NULL, ("%s: tp %p tp->t_inpcb == NULL", __func__, tp));
	INP_WLOCK(inp);
	if (callout_pending(&tp->t_timers->tt_keep) ||
	    !callout_active(&tp->t_timers->tt_keep)) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	callout_deactivate(&tp->t_timers->tt_keep);
	if ((inp->inp_flags & INP_DROPPED) != 0) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	KASSERT((tp->t_timers->tt_flags & TT_STOPPED) == 0,
		("%s: tp %p tcpcb can't be stopped here", __func__, tp));
	KASSERT((tp->t_timers->tt_flags & TT_KEEP) != 0,
		("%s: tp %p keep callout should be running", __func__, tp));
#endif
	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
//	TCPSTAT_INC(tcps_keeptimeo);
	if (tp->t_state < TCPS_ESTABLISHED)
		goto dropit;
	if ((always_keepalive/* || inp->inp_socket->so_options & SO_KEEPALIVE*/) &&
	    tp->t_state <= TCPS_CLOSING) {
		if (ticks - tp->t_rcvtime >= TP_KEEPIDLE(tp) + TP_MAXIDLE(tp))
			goto dropit;
		/*
		 * Send a packet designed to force a response
		 * if the peer is up and reachable:
		 * either an ACK if the connection is still alive,
		 * or an RST if the peer has closed the connection
		 * due to timeout or reboot.
		 * Using sequence number tp->snd_una-1
		 * causes the transmitted zero-length segment
		 * to lie outside the receive window;
		 * by the protocol spec, this requires the
		 * correspondent TCP to respond.
		 */
//		TCPSTAT_INC(tcps_keepprobe);
		tcpip_maketemplate(/*inp*/tp, &t_template);
		//if (t_template) {
			tcp_respond(tp, tp->instance, (struct ip6_hdr*) t_template.tt_ipgen,
				    &t_template.tt_t,/* (struct mbuf *)NULL,*/
				    tp->rcv_nxt, tp->snd_una - 1, 0);
			//free(t_template, M_TEMP);
			//ip_free(t_template);
		//}
#if 0
		if (!callout_reset(&tp->t_timers->tt_keep, TP_KEEPINTVL(tp),
		    tcp_timer_keep, tp)) {
			tp->t_timers->tt_flags &= ~TT_KEEP_RST;
		}
#endif
		tcplp_sys_set_timer(tp, TOS_KEEP, TP_KEEPINTVL(tp));
	} else /*if (!callout_reset(&tp->t_timers->tt_keep, TP_KEEPIDLE(tp),
		    tcp_timer_keep, tp)) {
			tp->t_timers->tt_flags &= ~TT_KEEP_RST;
		}*/
		{
			tcplp_sys_set_timer(tp, TOS_KEEP, TP_KEEPIDLE(tp));
		}
#if 0
#ifdef TCPDEBUG
	if (inp->inp_socket->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	TCP_PROBE2(debug__user, tp, PRU_SLOWTIMO);
	INP_WUNLOCK(inp);
	INP_INFO_RUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();
#endif
	return;

dropit:
//	TCPSTAT_INC(tcps_keepdrops);
	tp = tcp_drop(tp, ETIMEDOUT);
#if 0
#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	TCP_PROBE2(debug__user, tp, PRU_SLOWTIMO);
	if (tp != NULL)
		INP_WUNLOCK(tp->t_inpcb);
	INP_INFO_RUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();
#endif
}

void
tcp_timer_persist(struct tcpcb* tp)
{
    uint32_t ticks = tcplp_sys_get_ticks();
    KASSERT(tpistimeractive(tp, TT_PERSIST), ("Persist timer running, but unmarked\n"));
    tpcleartimeractive(tp, TT_PERSIST); // mark that this timer is no longer active
#if 0 // I already cancel if a timer was scheduled meanwhile
	struct inpcb *inp;
	CURVNET_SET(tp->t_vnet);
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_RLOCK(&V_tcbinfo);
	inp = tp->t_inpcb;
	KASSERT(inp != NULL, ("%s: tp %p tp->t_inpcb == NULL", __func__, tp));
	INP_WLOCK(inp);
	if (callout_pending(&tp->t_timers->tt_persist) ||
	    !callout_active(&tp->t_timers->tt_persist)) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	callout_deactivate(&tp->t_timers->tt_persist);
	if ((inp->inp_flags & INP_DROPPED) != 0) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	KASSERT((tp->t_timers->tt_flags & TT_STOPPED) == 0,
		("%s: tp %p tcpcb can't be stopped here", __func__, tp));
	KASSERT((tp->t_timers->tt_flags & TT_PERSIST) != 0,
		("%s: tp %p persist callout should be running", __func__, tp));
#endif
	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
//	TCPSTAT_INC(tcps_persisttimeo);
	/*
	 * Hack: if the peer is dead/unreachable, we do not
	 * time out if the window is closed.  After a full
	 * backoff, drop the connection if the idle time
	 * (no responses to probes) reaches the maximum
	 * backoff that we would use if retransmitting.
	 */

	if (tp->t_rxtshift == TCP_MAXRXTSHIFT &&
	    (ticks - tp->t_rcvtime >= tcp_maxpersistidle ||
	     ticks - tp->t_rcvtime >= TCP_REXMTVAL(tp) * tcp_totbackoff)) {
//		TCPSTAT_INC(tcps_persistdrop);
		tp = tcp_drop(tp, ETIMEDOUT);
		goto out;
	}

	/*
	 * If the user has closed the socket then drop a persisting
	 * connection after a much reduced timeout.
	 */
	if (tp->t_state > TCPS_CLOSE_WAIT &&
	    (ticks - tp->t_rcvtime) >= TCPTV_PERSMAX) {
//		TCPSTAT_INC(tcps_persistdrop);
		tp = tcp_drop(tp, ETIMEDOUT);
		goto out;
	}

	tcp_setpersist(tp);
	tp->t_flags |= TF_FORCEDATA;
	printf("Persist output: %zu bytes in sendbuf\n", cbuf_used_space(&tp->sendbuf));
	(void) tcp_output(tp);
	tp->t_flags &= ~TF_FORCEDATA;

out:
#if 0
#ifdef TCPDEBUG
	if (tp != NULL && tp->t_inpcb->inp_socket->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, NULL, NULL, PRU_SLOWTIMO);
#endif
	TCP_PROBE2(debug__user, tp, PRU_SLOWTIMO);
	if (tp != NULL)
		INP_WUNLOCK(inp);
	INP_INFO_RUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();
#endif
    return;
}

void
tcp_timer_2msl(struct tcpcb* tp)
{
	uint32_t ticks = tcplp_sys_get_ticks();
	KASSERT(tpistimeractive(tp, TT_2MSL), ("2MSL timer running, but unmarked\n"));
	tpcleartimeractive(tp, TT_2MSL); // for our own bookkeeping
#if 0
	struct inpcb *inp;
	CURVNET_SET(tp->t_vnet);
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_RLOCK(&V_tcbinfo);
	inp = tp->t_inpcb;
	KASSERT(inp != NULL, ("%s: tp %p tp->t_inpcb == NULL", __func__, tp));
	INP_WLOCK(inp);
	tcp_free_sackholes(tp);
	if (callout_pending(&tp->t_timers->tt_2msl) ||
	    !callout_active(&tp->t_timers->tt_2msl)) {
		INP_WUNLOCK(tp->t_inpcb);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	callout_deactivate(&tp->t_timers->tt_2msl);
	if ((inp->inp_flags & INP_DROPPED) != 0) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	KASSERT((tp->t_timers->tt_flags & TT_STOPPED) == 0,
		("%s: tp %p tcpcb can't be stopped here", __func__, tp));
	KASSERT((tp->t_timers->tt_flags & TT_2MSL) != 0,
		("%s: tp %p 2msl callout should be running", __func__, tp));
#endif
	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long delete connection control block.  Otherwise, check
	 * again in a bit.
	 *
	 * If in TIME_WAIT state just ignore as this timeout is handled in
	 * tcp_tw_2msl_scan(). (Sam: not anymore)
	 *
	 * If fastrecycle of FIN_WAIT_2, in FIN_WAIT_2 and receiver has closed,
	 * there's no point in hanging onto FIN_WAIT_2 socket. Just close it.
	 * Ignore fact that there were recent incoming segments.
	 */
#if 0
	if ((inp->inp_flags & INP_TIMEWAIT) != 0) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
#endif
	if (tcp_fast_finwait2_recycle && tp->t_state == TCPS_FIN_WAIT_2/* &&
	    tp->t_inpcb && tp->t_inpcb->inp_socket &&
	    (tp->t_inpcb->inp_socket->so_rcv.sb_state & SBS_CANTRCVMORE)*/) {
//		TCPSTAT_INC(tcps_finwait2_drops);
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
	} else if (tp->t_state == TCP6S_TIME_WAIT) { // Added by Sam
		/* Normally, this timer isn't used for sockets in the Time-wait state; instead the
		   tcp_tw_2msl_scan method is called periodically on the slow timer, and expired
		   tcbtw structs are closed and freed.

		   Instead, I keep the socket around, so I just use this timer to do it. */
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
	} else {
		if (ticks - tp->t_rcvtime <= TP_MAXIDLE(tp)) {
		/*
			if (!callout_reset(&tp->t_timers->tt_2msl,
			   TP_KEEPINTVL(tp), tcp_timer_2msl, tp)) {
				tp->t_timers->tt_flags &= ~TT_2MSL_RST;
			}
		*/
			tcplp_sys_set_timer(tp, TOS_2MSL, TP_KEEPINTVL(tp));
		} else {
			tp = tcp_close(tp);
			tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		}
    }
#if 0
#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	TCP_PROBE2(debug__user, tp, PRU_SLOWTIMO);

	if (tp != NULL)
		INP_WUNLOCK(inp);
	INP_INFO_RUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();
#endif
}

void
tcp_timer_rexmt(struct tcpcb *tp)
{
//	CURVNET_SET(tp->t_vnet);
	int rexmt;
	//int headlocked;
	uint32_t ticks = tcplp_sys_get_ticks();
	KASSERT(tpistimeractive(tp, TT_REXMT), ("Rexmt timer running, but unmarked\n"));
	tpcleartimeractive(tp, TT_REXMT); // for our own bookkeeping of active timers
//	struct inpcb *inp;
#if 0
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
#endif
//	INP_INFO_RLOCK(&V_tcbinfo);
//	inp = tp->t_inpcb;
//	KASSERT(inp != NULL, ("%s: tp %p tp->t_inpcb == NULL", __func__, tp));
//	INP_WLOCK(inp);
#if 0 // I already handle this edge case in the Timer.fired function in BsdTcpP.nc
	if (callout_pending(&tp->t_timers->tt_rexmt) ||
	    !callout_active(&tp->t_timers->tt_rexmt)) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
	callout_deactivate(&tp->t_timers->tt_rexmt);
	if ((inp->inp_flags & INP_DROPPED) != 0) {
		INP_WUNLOCK(inp);
		INP_INFO_RUNLOCK(&V_tcbinfo);
		CURVNET_RESTORE();
		return;
	}
#endif
//	KASSERT((tp->t_timers->tt_flags & TT_STOPPED) == 0,
//		("%s: tp %p tcpcb can't be stopped here", __func__, tp));
//	KASSERT((tp->t_timers->tt_flags & TT_REXMT) != 0,
//		("%s: tp %p rexmt callout should be running", __func__, tp));
//	tcp_free_sackholes(tp);
	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
        printf("rxtshift is %d\n", (int) tp->t_rxtshift);
	if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
		tp->t_rxtshift = TCP_MAXRXTSHIFT;
//		TCPSTAT_INC(tcps_timeoutdrop);

		tp = tcp_drop(tp, tp->t_softerror ?
			      tp->t_softerror : ETIMEDOUT);
		//headlocked = 1;
		goto out;
	}
//	INP_INFO_RUNLOCK(&V_tcbinfo);
	//headlocked = 0;
	if (tp->t_state == TCPS_SYN_SENT) {
		/*
		 * If the SYN was retransmitted, indicate CWND to be
		 * limited to 1 segment in cc_conn_init().
		 */
		tp->snd_cwnd = 1;
	} else if (tp->t_rxtshift == 1) {
		/*
		 * first retransmit; record ssthresh and cwnd so they can
		 * be recovered if this turns out to be a "bad" retransmit.
		 * A retransmit is considered "bad" if an ACK for this
		 * segment is received within RTT/2 interval; the assumption
		 * here is that the ACK was already in flight.  See
		 * "On Estimating End-to-End Network Path Properties" by
		 * Allman and Paxson for more details.
		 */
		tp->snd_cwnd_prev = tp->snd_cwnd;
		tp->snd_ssthresh_prev = tp->snd_ssthresh;
		tp->snd_recover_prev = tp->snd_recover;
		if (IN_FASTRECOVERY(tp->t_flags))
			tp->t_flags |= TF_WASFRECOVERY;
		else
			tp->t_flags &= ~TF_WASFRECOVERY;
		if (IN_CONGRECOVERY(tp->t_flags))
			tp->t_flags |= TF_WASCRECOVERY;
		else
			tp->t_flags &= ~TF_WASCRECOVERY;
		tp->t_badrxtwin = ticks + (tp->t_srtt >> (TCP_RTT_SHIFT + 1));
		tp->t_flags |= TF_PREVVALID;
	} else
		tp->t_flags &= ~TF_PREVVALID;
//	TCPSTAT_INC(tcps_rexmttimeo);
	if (tp->t_state == TCPS_SYN_SENT)
		rexmt = TCPTV_RTOBASE * tcp_syn_backoff[tp->t_rxtshift];
	else
		rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];
	TCPT_RANGESET(tp->t_rxtcur, rexmt,
		      tp->t_rttmin, TCPTV_REXMTMAX);

# if 0 // DON'T ATTEMPT BLACKHOLE DETECTION. OUR MTU SHOULD BE SMALL ENOUGH THAT ANY ROUTER CAN ROUTE IT
	/*
	 * We enter the path for PLMTUD if connection is established or, if
	 * connection is FIN_WAIT_1 status, reason for the last is that if
	 * amount of data we send is very small, we could send it in couple of
	 * packets and process straight to FIN. In that case we won't catch
	 * ESTABLISHED state.
	 */
	if (V_tcp_pmtud_blackhole_detect && (((tp->t_state == TCPS_ESTABLISHED))
	    || (tp->t_state == TCPS_FIN_WAIT_1))) {
		int optlen;
//#ifdef INET6
		int isipv6;
//#endif

		/*
		 * Idea here is that at each stage of mtu probe (usually, 1448
		 * -> 1188 -> 524) should be given 2 chances to recover before
		 *  further clamping down. 'tp->t_rxtshift % 2 == 0' should
		 *  take care of that.
		 */
		if (((tp->t_flags2 & (TF2_PLPMTU_PMTUD|TF2_PLPMTU_MAXSEGSNT)) ==
		    (TF2_PLPMTU_PMTUD|TF2_PLPMTU_MAXSEGSNT)) &&
		    (tp->t_rxtshift >= 2 && tp->t_rxtshift % 2 == 0)) {
			/*
			 * Enter Path MTU Black-hole Detection mechanism:
			 * - Disable Path MTU Discovery (IP "DF" bit).
			 * - Reduce MTU to lower value than what we
			 *   negotiated with peer.
			 */
			/* Record that we may have found a black hole. */
			tp->t_flags2 |= TF2_PLPMTU_BLACKHOLE;

			/* Keep track of previous MSS. */
			optlen = tp->t_maxopd - tp->t_maxseg;
			tp->t_pmtud_saved_maxopd = tp->t_maxopd;

			/*
			 * Reduce the MSS to blackhole value or to the default
			 * in an attempt to retransmit.
			 */
//#ifdef INET6
			//isipv6 = (tp->t_inpcb->inp_vflag & INP_IPV6) ? 1 : 0;
			isipv6 = 1;
			if (isipv6 &&
			    tp->t_maxopd > V_tcp_v6pmtud_blackhole_mss) {
				/* Use the sysctl tuneable blackhole MSS. */
				tp->t_maxopd = V_tcp_v6pmtud_blackhole_mss;
				V_tcp_pmtud_blackhole_activated++;
			} else if (isipv6) {
				/* Use the default MSS. */
				tp->t_maxopd = V_tcp_v6mssdflt;
				/*
				 * Disable Path MTU Discovery when we switch to
				 * minmss.
				 */
				tp->t_flags2 &= ~TF2_PLPMTU_PMTUD;
				V_tcp_pmtud_blackhole_activated_min_mss++;
			}
//#endif
#if 0
#if defined(INET6) && defined(INET)
			else
#endif
#ifdef INET
			if (tp->t_maxopd > V_tcp_pmtud_blackhole_mss) {
				/* Use the sysctl tuneable blackhole MSS. */
				tp->t_maxopd = V_tcp_pmtud_blackhole_mss;
				V_tcp_pmtud_blackhole_activated++;
			} else {
				/* Use the default MSS. */
				tp->t_maxopd = V_tcp_mssdflt;
				/*
				 * Disable Path MTU Discovery when we switch to
				 * minmss.
				 */
				tp->t_flags2 &= ~TF2_PLPMTU_PMTUD;
				V_tcp_pmtud_blackhole_activated_min_mss++;
			}
#endif
#endif
			tp->t_maxseg = tp->t_maxopd - optlen;
			/*
			 * Reset the slow-start flight size
			 * as it may depend on the new MSS.
			 */
			if (CC_ALGO(tp)->conn_init != NULL)
				CC_ALGO(tp)->conn_init(tp->ccv);
		} else {
			/*
			 * If further retransmissions are still unsuccessful
			 * with a lowered MTU, maybe this isn't a blackhole and
			 * we restore the previous MSS and blackhole detection
			 * flags.
			 * The limit '6' is determined by giving each probe
			 * stage (1448, 1188, 524) 2 chances to recover.
			 */
			if ((tp->t_flags2 & TF2_PLPMTU_BLACKHOLE) &&
			    (tp->t_rxtshift > 6)) {
				tp->t_flags2 |= TF2_PLPMTU_PMTUD;
				tp->t_flags2 &= ~TF2_PLPMTU_BLACKHOLE;
				optlen = tp->t_maxopd - tp->t_maxseg;
				tp->t_maxopd = tp->t_pmtud_saved_maxopd;
				tp->t_maxseg = tp->t_maxopd - optlen;
				V_tcp_pmtud_blackhole_failed++;
				/*
				 * Reset the slow-start flight size as it
				 * may depend on the new MSS.
				 */
				if (CC_ALGO(tp)->conn_init != NULL)
					CC_ALGO(tp)->conn_init(tp->ccv);
			}
		}
	}
#endif

	/*
	 * Disable RFC1323 and SACK if we haven't got any response to
	 * our third SYN to work-around some broken terminal servers
	 * (most of which have hopefully been retired) that have bad VJ
	 * header compression code which trashes TCP segments containing
	 * unknown-to-them TCP options.
	 */
	if (tcp_rexmit_drop_options && (tp->t_state == TCPS_SYN_SENT) &&
	    (tp->t_rxtshift == 3))
		tp->t_flags &= ~(TF_REQ_SCALE|TF_REQ_TSTMP|TF_SACK_PERMIT);
	/*
	 * If we backed off this far, our srtt estimate is probably bogus.
	 * Clobber it so we'll take the next rtt measurement as our srtt;
	 * move the current srtt into rttvar to keep the current
	 * retransmit times until then.
	 */
	if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
//#ifdef INET6
//		if ((tp->t_inpcb->inp_vflag & INP_IPV6) != 0)
//			in6_losing(tp->t_inpcb);
//#endif
		tp->t_rttvar += (tp->t_srtt >> TCP_RTT_SHIFT);
		tp->t_srtt = 0;
	}
	tp->snd_nxt = tp->snd_una;
	tp->snd_recover = tp->snd_max;
	/*
	 * Force a segment to be sent.
	 */
	tp->t_flags |= TF_ACKNOW;
	/*
	 * If timing a segment in this window, stop the timer.
	 */
	tp->t_rtttime = 0;

	cc_cong_signal(tp, NULL, CC_RTO);

	(void) tcp_output(tp);

out:
#if 0
#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	TCP_PROBE2(debug__user, tp, PRU_SLOWTIMO);
	if (tp != NULL)
		INP_WUNLOCK(inp);
	if (headlocked)
		INP_INFO_RUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();
#endif
    return;
}

int
tcp_timer_active(struct tcpcb *tp, uint32_t timer_type)
{
	return tpistimeractive(tp, timer_type);
}

void
tcp_timer_activate(struct tcpcb *tp, uint32_t timer_type, u_int delta) {
	uint8_t tos_timer;
	switch (timer_type) {
	case TT_DELACK:
		tos_timer = TOS_DELACK;
		break;
	case TT_REXMT:
		tos_timer = TOS_REXMT;
		break;
	case TT_PERSIST:
		tos_timer = TOS_PERSIST;
		break;
	case TT_KEEP:
		tos_timer = TOS_KEEP;
		break;
	case TT_2MSL:
		tos_timer = TOS_2MSL;
		break;
	default:
		tcplp_sys_log("Invalid timer 0x%u: skipping\n", (unsigned int) timer_type);
		return;
	}
	if (delta) {
	    tpmarktimeractive(tp, timer_type);
		if (tpistimeractive(tp, TT_REXMT) && tpistimeractive(tp, TT_PERSIST)) {
		    char* msg = "TCP CRITICAL FAILURE: Retransmit and Persist timers are simultaneously running!\n";
		    tcplp_sys_log("%s\n", msg);
		}
		tcplp_sys_set_timer(tp, tos_timer, (uint32_t) delta);
	} else {
		tpcleartimeractive(tp, timer_type);
		tcplp_sys_stop_timer(tp, tos_timer);
	}
}

void
tcp_cancel_timers(struct tcpcb* tp) {
	tpcleartimeractive(tp, TOS_DELACK);
	tcplp_sys_stop_timer(tp, TOS_DELACK);
	tpcleartimeractive(tp, TOS_REXMT);
	tcplp_sys_stop_timer(tp, TOS_REXMT);
	tpcleartimeractive(tp, TOS_PERSIST);
	tcplp_sys_stop_timer(tp, TOS_PERSIST);
	tpcleartimeractive(tp, TOS_KEEP);
	tcplp_sys_stop_timer(tp, TOS_KEEP);
	tpcleartimeractive(tp, TOS_2MSL);
	tcplp_sys_stop_timer(tp, TOS_2MSL);
}
