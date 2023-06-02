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
#include "../lib/lbuf.h"
#include "tcp_fsm.h"
#include "tcp_timer.h"
#include "tcp_var.h"

#include "tcp_const.h"

/*
 * samkumar: These options only matter if we do blackhole detection, which we
 * are not doing in TCPlp.
 */
#if 0
int V_tcp_pmtud_blackhole_detect = 0;
int V_tcp_pmtud_blackhole_failed = 0;
int V_tcp_pmtud_blackhole_activated = 0;
int V_tcp_pmtud_blackhole_activated_min_mss = 0;
#endif

/*
 * TCP timer processing.
 */

/*
 * samkumar: I changed these functions to accept "struct tcpcb* tp" their
 * argument instead of "void *xtp". This is possible since we're no longer
 * relying on FreeBSD's callout subsystem in TCPlp. I also changed them to
 * return 1 if the connection is dropped, or 0 otherwise.
 */

int
tcp_timer_delack(struct tcpcb* tp)
{
	/* samkumar: I added this, to replace the code I removed below. */
	KASSERT(tpistimeractive(tp, TT_DELACK), ("Delack timer running, but unmarked"));
	tpcleartimeractive(tp, TT_DELACK);

	/*
	 * samkumar: There used to be code here to properly handle the callout,
	 * including edge cases (return early if the callout was reset after this
	 * function was scheduled for execution, deactivate the callout, return
	 * early if the INP_DROPPED flag is set on the inpcb, and assert that the
	 * tp->t_timers state is correct).
	 *
	 * I also removed stats collection, locking, and vnet, throughout the code.
	 */
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	return 0;
}

int
tcp_timer_keep(struct tcpcb* tp)
{
	uint32_t ticks = tcplp_sys_get_ticks();
	struct tcptemp t_template;

	/* samkumar: I added this, to replace the code I removed below. */
	KASSERT(tpistimeractive(tp, TT_KEEP), ("Keep timer running, but unmarked"));
	tpcleartimeractive(tp, TT_KEEP);

	/*
	 * samkumar: There used to be code here to properly handle the callout,
	 * including edge cases (return early if the callout was reset after this
	 * function was scheduled for execution, deactivate the callout, return
	 * early if the INP_DROPPED flag is set on the inpcb, and assert that the
	 * tp->t_timers state is correct).
	 *
	 * I also removed stats collection, locking, and vnet, throughout the code.
	 * I commented out checks on socket options (since we don't support those).
	 *
	 * I also allocate t_template on the stack instead of allocating it
	 * dynamically, on the heap.
	 */

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
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
		tcpip_maketemplate(tp, &t_template);
		/*
		 * samkumar: I removed checks to make sure t_template was successfully
		 * allocated and then free it as appropriate. This is not necessary
		 * because I rewrote this part of the code to allocate t_template on
		 * the stack.
		 */
		tcp_respond(tp, tp->instance, (struct ip6_hdr*) t_template.tt_ipgen,
			    &t_template.tt_t,
			    tp->rcv_nxt, tp->snd_una - 1, 0);
		/*
		 * samkumar: I replaced a call to callout_reset with the following
		 * code, which resets the timer the TCPlp way.
		 */
		tpmarktimeractive(tp, TT_KEEP);
		tcplp_sys_set_timer(tp, TT_KEEP, TP_KEEPINTVL(tp));
	} else {
		/*
		 * samkumar: I replaced a call to callout_reset with the following
		 * code, which resets the timer the TCPlp way.
		 */
		tpmarktimeractive(tp, TT_KEEP);
		tcplp_sys_set_timer(tp, TT_KEEP, TP_KEEPIDLE(tp));
	}

	/*
	 * samkumar: There used to be some code here and below the "dropit" label
	 * that handled debug tracing/probes, vnet, and locking. I removed that
	 * code.
	 */
	return 0;

dropit:
	tp = tcp_drop(tp, ETIMEDOUT);
	(void) tp; /* samkumar: prevent a compiler warning */
	return 1;
}

int
tcp_timer_persist(struct tcpcb* tp)
{
	uint32_t ticks = tcplp_sys_get_ticks();
	int dropped = 0;

	/* samkumar: I added this, to replace the code I removed below. */
	KASSERT(tpistimeractive(tp, TT_PERSIST), ("Persist timer running, but unmarked"));
	tpcleartimeractive(tp, TT_PERSIST); // mark that this timer is no longer active

	/*
	 * samkumar: There used to be code here to properly handle the callout,
	 * including edge cases (return early if the callout was reset after this
	 * function was scheduled for execution, deactivate the callout, return
	 * early if the INP_DROPPED flag is set on the inpcb, and assert that the
	 * tp->t_timers state is correct).
	 *
	 * I also removed stats collection, locking, and vnet, throughout the code.
	 * I commented out checks on socket options (since we don't support those).
	 */

	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
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
		tp = tcp_drop(tp, ETIMEDOUT);
		dropped = 1;
		goto out;
	}

	/*
	 * If the user has closed the socket then drop a persisting
	 * connection after a much reduced timeout.
	 */
	if (tp->t_state > TCPS_CLOSE_WAIT &&
	    (ticks - tp->t_rcvtime) >= TCPTV_PERSMAX) {
		tp = tcp_drop(tp, ETIMEDOUT);
		dropped = 1;
		goto out;
	}

	tcp_setpersist(tp);
	tp->t_flags |= TF_FORCEDATA;
	tcplp_sys_log("Persist output: %zu bytes in sendbuf", lbuf_used_space(&tp->sendbuf));
	(void) tcp_output(tp);
	tp->t_flags &= ~TF_FORCEDATA;

out:
	/*
	 * samkumar: There used to be some code here that handled debug
	 * tracing/probes, vnet, and locking. I removed that code.
	 */
	(void) tp; /* samkumar: prevent a compiler warning */
	return dropped;
}

int
tcp_timer_2msl(struct tcpcb* tp)
{
	uint32_t ticks = tcplp_sys_get_ticks();
	int dropped = 0;

	/* samkumar: I added this, to replace the code I removed below. */
	KASSERT(tpistimeractive(tp, TT_2MSL), ("2MSL timer running, but unmarked"));
	tpcleartimeractive(tp, TT_2MSL);

	/*
	 * samkumar: There used to be code here to properly handle the callout,
	 * including edge cases (return early if the callout was reset after this
	 * function was scheduled for execution, deactivate the callout, return
	 * early if the INP_DROPPED flag is set on the inpcb, and assert that the
	 * tp->t_timers state is correct).
	 *
	 * I also removed stats collection, locking, and vnet, throughout the code.
	 */

	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long delete connection control block.  Otherwise, check
	 * again in a bit.
	 *
	 * If in TIME_WAIT state just ignore as this timeout is handled in
	 * tcp_tw_2msl_scan().
	 *
	 * If fastrecycle of FIN_WAIT_2, in FIN_WAIT_2 and receiver has closed,
	 * there's no point in hanging onto FIN_WAIT_2 socket. Just close it.
	 * Ignore fact that there were recent incoming segments.
	 */

	/*
	 * samkumar: The above comment about ignoring this timeout if we're in the
	 * TIME_WAIT state no longer is true, since in TCPlp we changed how
	 * TIME_WAIT is handled. In FreeBSD, this timer isn't used for sockets in
	 * the TIME_WAIT state; instead the tcp_tw_2msl_scan method is called
	 * periodically on the slow timer, and expired tcptw structs are closed and
	 * freed. I changed it so that TIME-WAIT connections are still represented
	 * as tcpcb's, not tcptw's, and to rely on this timer to close the
	 * connection.
	 *
	 * Below, there used to be an if statement that checks the inpcb to tell
	 * if we're in TIME-WAIT state, and return early if so. I've replaced this
	 * with an if statement that checks the tcpcb if we're in the TIME-WAIT
	 * state, and acts appropriately if so.
	 */
	if (tp->t_state == TCP6S_TIME_WAIT) {
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		dropped = 1;
		return dropped;
	}
	/*
	 * samkumar: This if statement also used to check that an inpcb is still
	 * attached and also
	 * (tp->t_inpcb->inp_socket->so_rcv.sb_state & SBS_CANTRCVMORE).
	 * We replaced the check on that flag with a call to tpiscantrcv. We
	 * haven't received a FIN, since we're in FIN-WAIT-2, so the only way it
	 * would pass the check is if the user called shutdown(SHUT_RD)
	 * or shutdown(SHUT_RDWR), which is impossible unless the host system
	 * provides an API for that.
	 */
	if (tcp_fast_finwait2_recycle && tp->t_state == TCPS_FIN_WAIT_2 &&
	    tpiscantrcv(tp)) {
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		dropped = 1;
	} else {
		if (ticks - tp->t_rcvtime <= TP_MAXIDLE(tp)) {
			/*
			 * samkumar: I replaced a call to callout_reset with the following
			 * code, which resets the timer the TCPlp way.
			 */
			tpmarktimeractive(tp, TT_2MSL);
			tcplp_sys_set_timer(tp, TT_2MSL, TP_KEEPINTVL(tp));
		} else {
			tp = tcp_close(tp);
			tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
			dropped = 1;
		}
	}
	/*
	 * samkumar: There used to be some code here that handled debug
	 * tracing/probes, vnet, and locking. I removed that code.
	 */
	return dropped;
}

int
tcp_timer_rexmt(struct tcpcb *tp)
{
	int rexmt;
	uint32_t ticks = tcplp_sys_get_ticks();
	int dropped = 0;

	/* samkumar: I added this, to replace the code I removed below. */
	KASSERT(tpistimeractive(tp, TT_REXMT), ("Rexmt timer running, but unmarked"));
	tpcleartimeractive(tp, TT_REXMT);

	/*
	 * samkumar: There used to be code here to properly handle the callout,
	 * including edge cases (return early if the callout was reset after this
	 * function was scheduled for execution, deactivate the callout, return
	 * early if the INP_DROPPED flag is set on the inpcb, and assert that the
	 * tp->t_timers state is correct).
	 *
	 * I also removed stats collection, locking, and vnet, throughout the code.
	 */

	tcp_free_sackholes(tp);
	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	tcplp_sys_log("rxtshift is %d", (int) tp->t_rxtshift);
	if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
		tp->t_rxtshift = TCP_MAXRXTSHIFT;

		tp = tcp_drop(tp, tp->t_softerror ?
			      tp->t_softerror : ETIMEDOUT);
		dropped = 1;
		goto out;
	}
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
	if (tp->t_state == TCPS_SYN_SENT)
		rexmt = TCPTV_RTOBASE * tcp_syn_backoff[tp->t_rxtshift];
	else
		rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];
	TCPT_RANGESET(tp->t_rxtcur, rexmt,
		      tp->t_rttmin, TCPTV_REXMTMAX);

	/*
	 * samkumar: There used to be more than 100 lines of code here, which
	 * implemented a feature called blackhole detection. The idea here is that
	 * some routers may silently discard packets whose MTU is too large,
	 * instead of fragmenting the packet or sending an ICMP packet to give
	 * feedback to the host. Blackhole detection decreases the MTU in response
	 * to packet loss in case the packets are being dropped by such a router.
	 *
	 * In TCPlp, we do not do blackhole detection because we use a small MTU
	 * (hundreds of bytes) that is unlikely to be too large for intermediate
	 * routers in the Internet. The edge low-power wireless network is
	 * assumed to be engineered to handle 6LoWPAN correctly.
	 */

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
		/*
		 * samkumar: Here, there used to be a call to "in6_losing", used to
		 * inform the lower layers about bad connectivity so it can search for
		 * different routes. The call was wrapped in a check on the inpcb's
		 * flags to check for IPv6 (which isn't relevant to us, since TCPlp
		 * assumes IPv6).
		 */
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
	/*
	 * samkumar: There used to be some code here that handled debug
	 * tracing/probes, vnet, and locking. I removed that code.
	 */
	(void) tp; /* samkumar: Prevent a compiler warning */
	return dropped;
}

int
tcp_timer_active(struct tcpcb *tp, uint32_t timer_type)
{
	return tpistimeractive(tp, timer_type);
}

void
tcp_timer_activate(struct tcpcb *tp, uint32_t timer_type, uint32_t delta) {
	if (delta) {
		tpmarktimeractive(tp, timer_type);
		if (tpistimeractive(tp, TT_REXMT) && tpistimeractive(tp, TT_PERSIST)) {
			tcplp_sys_panic("TCP CRITICAL FAILURE: Retransmit and Persist timers are simultaneously running!");
		}
		tcplp_sys_set_timer(tp, timer_type, (uint32_t) delta);
	} else {
		tpcleartimeractive(tp, timer_type);
		tcplp_sys_stop_timer(tp, timer_type);
	}
}

void
tcp_cancel_timers(struct tcpcb* tp) {
	tpcleartimeractive(tp, TT_DELACK);
	tcplp_sys_stop_timer(tp, TT_DELACK);
	tpcleartimeractive(tp, TT_REXMT);
	tcplp_sys_stop_timer(tp, TT_REXMT);
	tpcleartimeractive(tp, TT_PERSIST);
	tcplp_sys_stop_timer(tp, TT_PERSIST);
	tpcleartimeractive(tp, TT_KEEP);
	tcplp_sys_stop_timer(tp, TT_KEEP);
	tpcleartimeractive(tp, TT_2MSL);
	tcplp_sys_stop_timer(tp, TT_2MSL);
}
