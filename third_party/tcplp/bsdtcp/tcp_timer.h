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

/**
 * samkumar: The FreeBSD Operating System uses its callout subsystem to
 * implement TCP timers. I've removed the relevant definitions/declarations
 * below, since TCPlp relies on the host system to implement TCP timers. To
 * save memory, I've removed some configurability (e.g., per-TCB keepalive
 * parameters) and global statistics (e.g. tcp_keepcnt).
 */

#ifndef TCPLP_NETINET_TCP_TIMER_H_
#define TCPLP_NETINET_TCP_TIMER_H_

#include "tcp_var.h"

/*
 * The TCPT_REXMT timer is used to force retransmissions.
 * The TCP has the TCPT_REXMT timer set whenever segments
 * have been sent for which ACKs are expected but not yet
 * received.  If an ACK is received which advances tp->snd_una,
 * then the retransmit timer is cleared (if there are no more
 * outstanding segments) or reset to the base value (if there
 * are more ACKs expected).  Whenever the retransmit timer goes off,
 * we retransmit one unacknowledged segment, and do a backoff
 * on the retransmit timer.
 *
 * The TCPT_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous transmissions
 * have been acknowledged (so that there are no retransmissions in progress),
 * and the window is too small to bother sending anything, then we start
 * the TCPT_PERSIST timer.  When it expires, if the window is nonzero,
 * we go to transmit state.  Otherwise, at intervals send a single byte
 * into the peer's window to force him to update our window information.
 * We do this at most as often as TCPT_PERSMIN time intervals,
 * but no more frequently than the current estimate of round-trip
 * packet time.  The TCPT_PERSIST timer is cleared whenever we receive
 * a window update from the peer.
 *
 * The TCPT_KEEP timer is used to keep connections alive.  If an
 * connection is idle (no segments received) for TCPTV_KEEP_INIT amount of time,
 * but not yet established, then we drop the connection.  Once the connection
 * is established, if the connection is idle for TCPTV_KEEP_IDLE time
 * (and keepalives have been enabled on the socket), we begin to probe
 * the connection.  We force the peer to send us a segment by sending:
 *	<SEQ=SND.UNA-1><ACK=RCV.NXT><CTL=ACK>
 * This segment is (deliberately) outside the window, and should elicit
 * an ack segment in response from the peer.  If, despite the TCPT_KEEP
 * initiated segments we cannot elicit a response from a peer in TCPT_MAXIDLE
 * amount of time probing, then we drop the connection.
 */

#define TT_DELACK	0x0001
#define TT_REXMT	0x0002
#define TT_PERSIST	0x0004
#define TT_KEEP		0x0008
#define TT_2MSL		0x0010

/*
 * Time constants.
 */
#define	TCPTV_MSL	( 30*hz)		/* max seg lifetime (hah!) */
#define	TCPTV_SRTTBASE	0			/* base roundtrip time;
						   if 0, no idea yet */
#define	TCPTV_RTOBASE	(  3*hz)		/* assumed RTO if no info */

#define	TCPTV_PERSMIN	(  5*hz)		/* retransmit persistence */
#define	TCPTV_PERSMAX	( 60*hz)		/* maximum persist interval */

#define	TCPTV_KEEP_INIT	( 75*hz)		/* initial connect keepalive */
#define	TCPTV_KEEP_IDLE	(120*60*hz)		/* dflt time before probing */
#define	TCPTV_KEEPINTVL	( 75*hz)		/* default probe interval */
#define	TCPTV_KEEPCNT	8			/* max probes before drop */

#define TCPTV_FINWAIT2_TIMEOUT (60*hz)         /* FIN_WAIT_2 timeout if no receiver */

/*
 * Minimum retransmit timer is 3 ticks, for algorithmic stability.
 * TCPT_RANGESET() will add another TCPTV_CPU_VAR to deal with
 * the expected worst-case processing variances by the kernels
 * representing the end points.  Such variances do not always show
 * up in the srtt because the timestamp is often calculated at
 * the interface rather then at the TCP layer.  This value is
 * typically 50ms.  However, it is also possible that delayed
 * acks (typically 100ms) could create issues so we set the slop
 * to 200ms to try to cover it.  Note that, properly speaking,
 * delayed-acks should not create a major issue for interactive
 * environments which 'P'ush the last segment, at least as
 * long as implementations do the required 'at least one ack
 * for every two packets' for the non-interactive streaming case.
 * (maybe the RTO calculation should use 2*RTT instead of RTT
 * to handle the ack-every-other-packet case).
 *
 * The prior minimum of 1*hz (1 second) badly breaks throughput on any
 * networks faster then a modem that has minor (e.g. 1%) packet loss.
 */
#define	TCPTV_MIN	( hz/33 )		/* minimum allowable value */
#define TCPTV_CPU_VAR	( hz/5 )		/* cpu variance allowed (200ms) */
#define	TCPTV_REXMTMAX	( 64*hz)		/* max allowable REXMT value */

#define TCPTV_TWTRUNC	8			/* RTO factor to truncate TW */

#define	TCP_LINGERTIME	120			/* linger at most 2 minutes */

#define	TCP_MAXRXTSHIFT	12			/* maximum retransmits */

#define	TCPTV_DELACK	( hz/10 )		/* 100ms timeout */

#ifdef	TCPTIMERS
static const char *tcptimers[] =
    { "REXMT", "PERSIST", "KEEP", "2MSL", "DELACK" };
#endif

int tcp_timer_active(struct tcpcb *tp, uint32_t timer_type);
void tcp_timer_activate(struct tcpcb *tp, uint32_t timer_type, uint32_t delta);
void tcp_cancel_timers(struct tcpcb* tp);

/* I moved the definition of TCPT_RANGESET to tcp_const.h. */

static const int	tcp_syn_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 1, 1, 1, 1, 2, 4, 8, 16, 32, 64, 64, 64 };

static const int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 512, 512, 512 };

static const int tcp_totbackoff = 2559;	/* sum of tcp_backoff[] */

/*
 * samkumar: Changed return value to int to indicate whether connection was
 * dropped or not.
 */
int tcp_timer_delack(struct tcpcb* tp);
int tcp_timer_keep(struct tcpcb* tp);
int tcp_timer_persist(struct tcpcb* tp);
int tcp_timer_2msl(struct tcpcb* tp);
int tcp_timer_rexmt(struct tcpcb *tp);
int tcp_timer_active(struct tcpcb *tp, uint32_t timer_type);

/*
 * samkumar: Modified to use default keepalive parameters, since we removed
 * the fields from tcpcb that allow them to be set on a per-connection basis.
 */
#define	TP_KEEPINIT(tp)	(/*(tp)->t_keepinit ? (tp)->t_keepinit :*/ tcp_keepinit)
#define	TP_KEEPIDLE(tp)	(/*(tp)->t_keepidle ? (tp)->t_keepidle :*/ tcp_keepidle)
#define	TP_KEEPINTVL(tp) (/*(tp)->t_keepintvl ? (tp)->t_keepintvl :*/ tcp_keepintvl)
#define	TP_KEEPCNT(tp)	(/*(tp)->t_keepcnt ? (tp)->t_keepcnt :*/ tcp_keepcnt)
#define	TP_MAXIDLE(tp)	(TP_KEEPCNT(tp) * TP_KEEPINTVL(tp))

/*
 * samkumar: MOVED NECESSARY EXTERN DECLARATIONS TO TCP_SUBR.C
 * Removed timer declarations that aren't needed since timers are handled
 * by the host system (in this case, OpenThread), in TCPlp.
 */

#endif /* !_NETINET_TCP_TIMER_H_ */
