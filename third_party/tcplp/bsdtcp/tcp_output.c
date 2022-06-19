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
 *	@(#)tcp_output.c	8.4 (Berkeley) 5/24/95
 */

#include <errno.h>
#include <string.h>

#include "../tcplp.h"
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_var.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "ip.h"
#include "../lib/cbuf.h"

#include "tcp_const.h"

#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/tcp.h>

static inline void
cc_after_idle(struct tcpcb *tp)
{
	/* samkumar: Removed synchronization. */
	if (CC_ALGO(tp)->after_idle != NULL)
		CC_ALGO(tp)->after_idle(tp->ccv);
}

long min(long a, long b) {
	if (a < b) {
		return a;
	} else {
		return b;
	}
}

unsigned long ulmin(unsigned long a, unsigned long b) {
	if (a < b) {
		return a;
	} else {
		return b;
	}
}

#define lmin(a, b) min(a, b)

void
tcp_setpersist(struct tcpcb *tp)
{
	int t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
	int tt;

	tp->t_flags &= ~TF_PREVVALID;
	if (tcp_timer_active(tp, TT_REXMT))
		tcplp_sys_panic("PANIC: tcp_setpersist: retransmit pending");
	/*
	 * Start/restart persistance timer.
	 */
	TCPT_RANGESET(tt, t * tcp_backoff[tp->t_rxtshift],
		      TCPTV_PERSMIN, TCPTV_PERSMAX);
	tcp_timer_activate(tp, TT_PERSIST, tt);
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}

/*
 * Tcp output routine: figure out what should be sent and send it.
 */
int
tcp_output(struct tcpcb *tp)
{
	/*
	 * samkumar: The biggest change in this function is in how outgoing
	 * segments are built and sent out. That code has been updated to account
	 * for TCPlp's buffering, and using otMessages rather than mbufs to
	 * construct the outgoing segments.
	 *
	 * And, of course, all code corresponding to locks, stats, and debugging
	 * has been removed, and all code specific to IPv4 or to decide between
	 * IPv6 and IPv4 handling has been removed.
	 */

	struct tcphdr* th = NULL;
	int idle;
	long len, recwin, sendwin;
	int off, flags, error = 0;	/* Keep compiler happy */
	int sendalot, mtu;
	int sack_rxmit, sack_bytes_rxmt;
	struct sackhole* p;
	unsigned ipoptlen, optlen, hdrlen;
	struct tcpopt to;
	uint8_t opt[TCP_MAXOLEN];
	uint32_t ticks = tcplp_sys_get_ticks();

	/* samkumar: Code for TCP offload has been removed. */

	/*
	 * Determine length of data that should be transmitted,
	 * and flags that will be used.
	 * If there is some data or critical controls (SYN, RST)
	 * to send, then transmit; otherwise, investigate further.
	 */
	idle = (tp->t_flags & TF_LASTIDLE) || (tp->snd_max == tp->snd_una);
	if (idle && ticks - tp->t_rcvtime >= tp->t_rxtcur)
		cc_after_idle(tp);

	tp->t_flags &= ~TF_LASTIDLE;
	if (idle) {
		if (tp->t_flags & TF_MORETOCOME) {
			tp->t_flags |= TF_LASTIDLE;
			idle = 0;
		}
	}
	/* samkumar: This would be printed once per _window_ that is transmitted. */
#ifdef INSTRUMENT_TCP
	tcplp_sys_log("TCP output %u %d %d", (unsigned int) tcplp_sys_get_millis(), (int) tp->snd_wnd, (int) tp->snd_cwnd);
#endif

again:
	/*
	 * If we've recently taken a timeout, snd_max will be greater than
	 * snd_nxt.  There may be SACK information that allows us to avoid
	 * resending already delivered data.  Adjust snd_nxt accordingly.
	 */
	if ((tp->t_flags & TF_SACK_PERMIT) &&
	    SEQ_LT(tp->snd_nxt, tp->snd_max))
		tcp_sack_adjust(tp);
	sendalot = 0;
	/* samkumar: Removed code for supporting TSO. */
	mtu = 0;
	off = tp->snd_nxt - tp->snd_una;
	sendwin = min(tp->snd_wnd, tp->snd_cwnd);

	flags = tcp_outflags[tp->t_state];
	/*
	 * Send any SACK-generated retransmissions.  If we're explicitly trying
	 * to send out new data (when sendalot is 1), bypass this function.
	 * If we retransmit in fast recovery mode, decrement snd_cwnd, since
	 * we're replacing a (future) new transmission with a retransmission
	 * now, and we previously incremented snd_cwnd in tcp_input().
	 */
	/*
	 * Still in sack recovery , reset rxmit flag to zero.
	 */
	sack_rxmit = 0;
	sack_bytes_rxmt = 0;
	len = 0;
	p = NULL;
	if ((tp->t_flags & TF_SACK_PERMIT) && IN_FASTRECOVERY(tp->t_flags) &&
	    (p = tcp_sack_output(tp, &sack_bytes_rxmt))) {
		long cwin;

		cwin = min(tp->snd_wnd, tp->snd_cwnd) - sack_bytes_rxmt;
		if (cwin < 0)
			cwin = 0;
		/* Do not retransmit SACK segments beyond snd_recover */
		if (SEQ_GT(p->end, tp->snd_recover)) {
			/*
			 * (At least) part of sack hole extends beyond
			 * snd_recover. Check to see if we can rexmit data
			 * for this hole.
			 */
			if (SEQ_GEQ(p->rxmit, tp->snd_recover)) {
				/*
				 * Can't rexmit any more data for this hole.
				 * That data will be rexmitted in the next
				 * sack recovery episode, when snd_recover
				 * moves past p->rxmit.
				 */
				p = NULL;
				goto after_sack_rexmit;
			} else
				/* Can rexmit part of the current hole */
				len = ((long)ulmin(cwin,
						   tp->snd_recover - p->rxmit));
		} else
			len = ((long)ulmin(cwin, p->end - p->rxmit));
		off = p->rxmit - tp->snd_una;
		KASSERT(off >= 0,("%s: sack block to the left of una : %d",
		    __func__, off));
		if (len > 0) {
			sack_rxmit = 1;
			sendalot = 1;
		}
	}
after_sack_rexmit:
	/*
	 * Get standard flags, and add SYN or FIN if requested by 'hidden'
	 * state flags.
	 */
	if (tp->t_flags & TF_NEEDFIN)
		flags |= TH_FIN;
	if (tp->t_flags & TF_NEEDSYN)
		flags |= TH_SYN;

	/*
	 * If in persist timeout with window of 0, send 1 byte.
	 * Otherwise, if window is small but nonzero
	 * and timer expired, we will send what we can
	 * and go to transmit state.
	 */
	if (tp->t_flags & TF_FORCEDATA) {
		if (sendwin == 0) {
			/*
			 * If we still have some data to send, then
			 * clear the FIN bit.  Usually this would
			 * happen below when it realizes that we
			 * aren't sending all the data.  However,
			 * if we have exactly 1 byte of unsent data,
			 * then it won't clear the FIN bit below,
			 * and if we are in persist state, we wind
			 * up sending the packet without recording
			 * that we sent the FIN bit.
			 *
			 * We can't just blindly clear the FIN bit,
			 * because if we don't have any more data
			 * to send then the probe will be the FIN
			 * itself.
			 */
			/*
			 * samkumar: Replaced call to sbused(&so->so_snd) with the call to
			 * lbuf_used_space below.
			 */
			if (off < lbuf_used_space(&tp->sendbuf))
				flags &= ~TH_FIN;
			sendwin = 1;
		} else {
			tcp_timer_activate(tp, TT_PERSIST, 0);
			tp->t_rxtshift = 0;
		}
	}

	/*
	 * If snd_nxt == snd_max and we have transmitted a FIN, the
	 * offset will be > 0 even if so_snd.sb_cc is 0, resulting in
	 * a negative length.  This can also occur when TCP opens up
	 * its congestion window while receiving additional duplicate
	 * acks after fast-retransmit because TCP will reset snd_nxt
	 * to snd_max after the fast-retransmit.
	 *
	 * In the normal retransmit-FIN-only case, however, snd_nxt will
	 * be set to snd_una, the offset will be 0, and the length may
	 * wind up 0.
	 *
	 * If sack_rxmit is true we are retransmitting from the scoreboard
	 * in which case len is already set.
	 */
	if (sack_rxmit == 0) {
		if (sack_bytes_rxmt == 0)
			/*
			 * samkumar: Replaced sbavail(&so->so_snd) with this call to
			 * lbuf_used_space.
			 */
			len = ((long)ulmin(lbuf_used_space(&tp->sendbuf), sendwin) -
			    off);
		else {
			long cwin;

			/*
			 * We are inside of a SACK recovery episode and are
			 * sending new data, having retransmitted all the
			 * data possible in the scoreboard.
			 */
			/*
			 * samkumar: Replaced sbavail(&so->so_snd) with this call to
			 * lbuf_used_space.
			 */
			len = ((long)ulmin(lbuf_used_space(&tp->sendbuf), tp->snd_wnd) -
			    off);
			/*
			 * Don't remove this (len > 0) check !
			 * We explicitly check for len > 0 here (although it
			 * isn't really necessary), to work around a gcc
			 * optimization issue - to force gcc to compute
			 * len above. Without this check, the computation
			 * of len is bungled by the optimizer.
			 */
			if (len > 0) {
				cwin = tp->snd_cwnd -
					(tp->snd_nxt - tp->sack_newdata) -
					sack_bytes_rxmt;
				if (cwin < 0)
					cwin = 0;
				len = lmin(len, cwin);
			}
		}
	}

	/*
	 * Lop off SYN bit if it has already been sent.  However, if this
	 * is SYN-SENT state and if segment contains data and if we don't
	 * know that foreign host supports TAO, suppress sending segment.
	 */
	if ((flags & TH_SYN) && SEQ_GT(tp->snd_nxt, tp->snd_una)) {
		if (tp->t_state != TCPS_SYN_RECEIVED)
			flags &= ~TH_SYN;
		off--, len++;
	}

	/*
	 * Be careful not to send data and/or FIN on SYN segments.
	 * This measure is needed to prevent interoperability problems
	 * with not fully conformant TCP implementations.
	 */
	if ((flags & TH_SYN) && (tp->t_flags & TF_NOOPT)) {
		len = 0;
		flags &= ~TH_FIN;
	}

	if (len <= 0) {
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be < 0.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit, pull snd_nxt back
		 * to (closed) window, and set the persist timer
		 * if it isn't already going.  If the window didn't
		 * close completely, just wait for an ACK.
		 *
		 * We also do a general check here to ensure that
		 * we will set the persist timer when we have data
		 * to send, but a 0-byte window. This makes sure
		 * the persist timer is set even if the packet
		 * hits one of the "goto send" lines below.
		 */
		len = 0;
		/*
		 * samkumar: Replaced sbavail(&so->so_snd) with this call to
		 * lbuf_used_space.
		 */
		if ((sendwin == 0) && (TCPS_HAVEESTABLISHED(tp->t_state)) &&
			(off < (int) lbuf_used_space(&tp->sendbuf))) {
			tcp_timer_activate(tp, TT_REXMT, 0);
			tp->t_rxtshift = 0;
			tp->snd_nxt = tp->snd_una;
			if (!tcp_timer_active(tp, TT_PERSIST)) {
				tcp_setpersist(tp);
			}
		}
	}


	/* len will be >= 0 after this point. */
	KASSERT(len >= 0, ("[%s:%d]: len < 0", __func__, __LINE__));

	/*
	 * Automatic sizing of send socket buffer.  Often the send buffer
	 * size is not optimally adjusted to the actual network conditions
	 * at hand (delay bandwidth product).  Setting the buffer size too
	 * small limits throughput on links with high bandwidth and high
	 * delay (eg. trans-continental/oceanic links).  Setting the
	 * buffer size too big consumes too much real kernel memory,
	 * especially with many connections on busy servers.
	 *
	 * The criteria to step up the send buffer one notch are:
	 *  1. receive window of remote host is larger than send buffer
	 *     (with a fudge factor of 5/4th);
	 *  2. send buffer is filled to 7/8th with data (so we actually
	 *     have data to make use of it);
	 *  3. send buffer fill has not hit maximal automatic size;
	 *  4. our send window (slow start and cogestion controlled) is
	 *     larger than sent but unacknowledged data in send buffer.
	 *
	 * The remote host receive window scaling factor may limit the
	 * growing of the send buffer before it reaches its allowed
	 * maximum.
	 *
	 * It scales directly with slow start or congestion window
	 * and does at most one step per received ACK.  This fast
	 * scaling has the drawback of growing the send buffer beyond
	 * what is strictly necessary to make full use of a given
	 * delay*bandwith product.  However testing has shown this not
	 * to be much of an problem.  At worst we are trading wasting
	 * of available bandwith (the non-use of it) for wasting some
	 * socket buffer memory.
	 *
	 * TODO: Shrink send buffer during idle periods together
	 * with congestion window.  Requires another timer.  Has to
	 * wait for upcoming tcp timer rewrite.
	 *
	 * XXXGL: should there be used sbused() or sbavail()?
	 */
	 /*
	 * samkumar: There used to be code here to dynamically size the
	 * send buffer (by calling sbreserve_locked). In TCPlp, we don't support
	 * this, as the send buffer doesn't have a well-defined size (and even if
	 * we were to use a circular buffer, it would be a fixed-size buffer
	 * allocated by the application). Therefore, I removed the code that does
	 * this.
	 */

	 /*
 	 * samkumar: There used to be code here to handle TCP Segmentation
 	 * Offloading (TSO); I removed it becuase we don't support that in TCPlp.
 	 */

	if (sack_rxmit) {
		/*
		 * samkumar: Replaced sbused(&so->so_snd) with this call to
		 * lbuf_used_space.
		 */
		if (SEQ_LT(p->rxmit + len, tp->snd_una + lbuf_used_space(&tp->sendbuf)))
			flags &= ~TH_FIN;
	} else {
		if (SEQ_LT(tp->snd_nxt + len, tp->snd_una +
			/*
			 * samkumar: Replaced sbused(&so->so_snd) with this call to
			 * lbuf_used_space.
			 */
			lbuf_used_space(&tp->sendbuf)))
			flags &= ~TH_FIN;
	}

	/*
	 * samkumar: Replaced sbspace(&so->so_rcv) with this call to
	 * cbuf_free_space.
	 */
	recwin = cbuf_free_space(&tp->recvbuf);

	/*
	 * Sender silly window avoidance.   We transmit under the following
	 * conditions when len is non-zero:
	 *
	 *	- We have a full segment (or more with TSO)
	 *	- This is the last buffer in a write()/send() and we are
	 *	  either idle or running NODELAY
	 *	- we've timed out (e.g. persist timer)
	 *	- we have more then 1/2 the maximum send window's worth of
	 *	  data (receiver may be limited the window size)
	 *	- we need to retransmit
	 */
	if (len) {
		if (len >= tp->t_maxseg)
			goto send;
		/*
		 * NOTE! on localhost connections an 'ack' from the remote
		 * end may occur synchronously with the output and cause
		 * us to flush a buffer queued with moretocome.  XXX
		 *
		 * note: the len + off check is almost certainly unnecessary.
		 */
		/*
 		 * samkumar: Replaced sbavail(&so->so_snd) with this call to
 		 * lbuf_used_space.
 		 */
		if (!(tp->t_flags & TF_MORETOCOME) &&	/* normal case */
		    (idle || (tp->t_flags & TF_NODELAY)) &&
		    len + off >= lbuf_used_space(&tp->sendbuf) &&
		    (tp->t_flags & TF_NOPUSH) == 0) {
			goto send;
		}
		if (tp->t_flags & TF_FORCEDATA)		/* typ. timeout case */
			goto send;
		if (len >= tp->max_sndwnd / 2 && tp->max_sndwnd > 0)
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))	/* retransmit case */
			goto send;
		if (sack_rxmit)
			goto send;
	}

	/*
	 * Sending of standalone window updates.
	 *
	 * Window updates are important when we close our window due to a
	 * full socket buffer and are opening it again after the application
	 * reads data from it.  Once the window has opened again and the
	 * remote end starts to send again the ACK clock takes over and
	 * provides the most current window information.
	 *
	 * We must avoid the silly window syndrome whereas every read
	 * from the receive buffer, no matter how small, causes a window
	 * update to be sent.  We also should avoid sending a flurry of
	 * window updates when the socket buffer had queued a lot of data
	 * and the application is doing small reads.
	 *
	 * Prevent a flurry of pointless window updates by only sending
	 * an update when we can increase the advertized window by more
	 * than 1/4th of the socket buffer capacity.  When the buffer is
	 * getting full or is very small be more aggressive and send an
	 * update whenever we can increase by two mss sized segments.
	 * In all other situations the ACK's to new incoming data will
	 * carry further window increases.
	 *
	 * Don't send an independent window update if a delayed
	 * ACK is pending (it will get piggy-backed on it) or the
	 * remote side already has done a half-close and won't send
	 * more data.  Skip this if the connection is in T/TCP
	 * half-open state.
	 */
	if (recwin > 0 && !(tp->t_flags & TF_NEEDSYN) &&
	    !(tp->t_flags & TF_DELACK) &&
	    !TCPS_HAVERCVDFIN(tp->t_state)) {
		/*
		 * "adv" is the amount we could increase the window,
		 * taking into account that we are limited by
		 * TCP_MAXWIN << tp->rcv_scale.
		 */
		long adv;
		int oldwin;

		adv = min(recwin, (long)TCP_MAXWIN << tp->rcv_scale);
		if (SEQ_GT(tp->rcv_adv, tp->rcv_nxt)) {
			oldwin = (tp->rcv_adv - tp->rcv_nxt);
			adv -= oldwin;
		} else
			oldwin = 0;

		/*
		 * If the new window size ends up being the same as the old
		 * size when it is scaled, then don't force a window update.
		 */
		if (oldwin >> tp->rcv_scale == (adv + oldwin) >> tp->rcv_scale)
			goto dontupdate;

		/*
		 * samkumar: Here, FreeBSD has some heuristics to decide whether or
		 * not to send a window update. The code for the original heuristics
		 * is commented out, using #if 0. These heuristics compare "adv,"
		 * the size of the window update, with the size of the local receive
		 * buffer. The FreeBSD heuristics aren't applicable because they are
		 * orders of magnitude off from what we see in TCPlp. For example,
		 * FreeBSD only sends a window update if it is at least two segments
		 * big. Note that, in the experiments I did, the second case did not
		 * filter window updates further because, in the experiments, the
		 * receive buffer was smaller than 8 segments.
		 *
		 * I replaced these heuristics with a simpler version, which you can
		 * see below. For the experiments I did, the first condition
		 * (checking if adv >= (long)(2 * tp->t_maxseg)) wasn't included; this
		 * did not matter because the receive buffer was smaller than 8
		 * segments, so any condition that would have triggered the first
		 * condition would have triggered the second one anyway. I've included
		 * the first condition in this version in an effort to be more robust,
		 * in case someone does try to run TCPlp with a large receive buffer.
		 *
		 * It may be worth studying this more and revisiting the heuristic to
		 * use here. In case we try to resurrect the old FreeBSD heuristics,
		 * note that so->so_rcv.sb_hiwat in FreeBSD corresponds roughly to
		 * cbuf_size(&tp->recvbuf) in TCPlp.
		 */
#if 0
		if (adv >= (long)(2 * tp->t_maxseg) &&
		    (adv >= (long)(so->so_rcv.sb_hiwat / 4) ||
		     recwin <= (long)(so->so_rcv.sb_hiwat / 8) ||
		     so->so_rcv.sb_hiwat <= 8 * tp->t_maxseg))
			goto send;
#endif
		if (adv >= (long)(2 * tp->t_maxseg) ||
		    adv >= (long)cbuf_size(&tp->recvbuf) / 4)
			goto send;
	}
dontupdate:

	/*
	 * Send if we owe the peer an ACK, RST, SYN, or urgent data.  ACKNOW
	 * is also a catch-all for the retransmit timer timeout case.
	 */
	if (tp->t_flags & TF_ACKNOW) {
		goto send;
	}
	if ((flags & TH_RST) ||
	    ((flags & TH_SYN) && (tp->t_flags & TF_NEEDSYN) == 0))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;
	/*
	 * If our state indicates that FIN should be sent
	 * and we have not yet done so, then we need to send.
	 */
	if (flags & TH_FIN &&
	    ((tp->t_flags & TF_SENTFIN) == 0 || tp->snd_nxt == tp->snd_una))
		goto send;
	/*
	 * In SACK, it is possible for tcp_output to fail to send a segment
	 * after the retransmission timer has been turned off.  Make sure
	 * that the retransmission timer is set.
	 */
	if ((tp->t_flags & TF_SACK_PERMIT) &&
	    SEQ_GT(tp->snd_max, tp->snd_una) &&
	    !tcp_timer_active(tp, TT_REXMT) &&
	    !tcp_timer_active(tp, TT_PERSIST)) {
		tcp_timer_activate(tp, TT_REXMT, tp->t_rxtcur);
		goto just_return;
	}

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are:
	 *	idle			not doing retransmits or persists
	 *	persisting		to move a small or zero window
	 *	(re)transmitting	and thereby not persisting
	 *
	 * tcp_timer_active(tp, TT_PERSIST)
	 *	is true when we are in persist state.
	 * (tp->t_flags & TF_FORCEDATA)
	 *	is set when we are called to send a persist packet.
	 * tcp_timer_active(tp, TT_REXMT)
	 *	is set when we are retransmitting
	 * The output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state.
	 * If nothing happens soon, send when timer expires:
	 * if window is nonzero, transmit what we can,
	 * otherwise force out a byte.
	 */
	/*
	 * samkumar: Replaced sbavail(&so->so_snd) with this call to
	 * lbuf_used_space.
	 */
	if (lbuf_used_space(&tp->sendbuf) && !tcp_timer_active(tp, TT_REXMT) &&
	    !tcp_timer_active(tp, TT_PERSIST)) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}

	/*
	 * No reason to send a segment, just return.
	 */
just_return:
	return (0);

send:
	if (len > 0) {
		if (len >= tp->t_maxseg)
			tp->t_flags2 |= TF2_PLPMTU_MAXSEGSNT;
		else
			tp->t_flags2 &= ~TF2_PLPMTU_MAXSEGSNT;
	}
	/*
	 * Before ESTABLISHED, force sending of initial options
	 * unless TCP set not to do any options.
	 * NOTE: we assume that the IP/TCP header plus TCP options
	 * always fit in a single mbuf, leaving room for a maximum
	 * link header, i.e.
	 *	max_linkhdr + sizeof (struct tcpiphdr) + optlen <= MCLBYTES
	 */
	optlen = 0;
	hdrlen = sizeof (struct ip6_hdr) + sizeof (struct tcphdr);

	/*
	 * Compute options for segment.
	 * We only have to care about SYN and established connection
	 * segments.  Options for SYN-ACK segments are handled in TCP
	 * syncache.
	 * Sam: I've done away with the syncache. However, it seems that
	 * the existing logic works fine for SYN-ACK as well
	 */
	if ((tp->t_flags & TF_NOOPT) == 0) {
		to.to_flags = 0;
		/* Maximum segment size. */
		if (flags & TH_SYN) {
			tp->snd_nxt = tp->iss;
			to.to_mss = tcp_mssopt(tp);
			to.to_flags |= TOF_MSS;
		}
		/* Window scaling. */
		if ((flags & TH_SYN) && (tp->t_flags & TF_REQ_SCALE)) {
			to.to_wscale = tp->request_r_scale;
			to.to_flags |= TOF_SCALE;
		}
		/* Timestamps. */
		if ((tp->t_flags & TF_RCVD_TSTMP) ||
		    ((flags & TH_SYN) && (tp->t_flags & TF_REQ_TSTMP))) {
			to.to_tsval = tcp_ts_getticks() + tp->ts_offset;
			to.to_tsecr = tp->ts_recent;
			to.to_flags |= TOF_TS;
			/*
			 * samkumar: I removed the code to set the timestamp tp->rfbuf_ts
			 * for receive buffer autosizing, since we don't do autosizing on
			 * the receive buffer in TCPlp.
			 */
		}

		/* Selective ACK's. */
		if (tp->t_flags & TF_SACK_PERMIT) {
			if (flags & TH_SYN)
				to.to_flags |= TOF_SACKPERM;
			else if (TCPS_HAVEESTABLISHED(tp->t_state) &&
			    (tp->t_flags & TF_SACK_PERMIT) &&
			    tp->rcv_numsacks > 0) {
				to.to_flags |= TOF_SACK;
				to.to_nsacks = tp->rcv_numsacks;
				to.to_sacks = (uint8_t *)tp->sackblks;
			}
		}

		/*
		 * samkumar: Remove logic to set TOF_SIGNATURE flag in to.to_flags,
		 * since TCPlp does not support TCP signatures.
		 */

		/* Processing the options. */
		hdrlen += optlen = tcp_addoptions(&to, opt);
	}
	/*
	 * samkumar: This used to be set to ip6_optlen(tp->t_inpcb), instead of 0,
	 * along with some additional code to handle IPSEC. In TCPlp we don't set
	 * IPv6 options here; we expect those to be set by the host network stack.
	 * Of course, code that supports IPv4 has been removed as well.
	 */
	ipoptlen = 0;

	/*
	 * Adjust data length if insertion of options will
	 * bump the packet length beyond the t_maxopd length.
	 * Clear the FIN bit because we cut off the tail of
	 * the segment.
	 */
	if (len + optlen + ipoptlen > tp->t_maxopd) {
		flags &= ~TH_FIN;
		/*
		 * samkumar: Remove code for TCP segmentation offloading.
		 */
		len = tp->t_maxopd - optlen - ipoptlen;
		sendalot = 1;
	}
	/*
	 * samkumar: The else case of the above "if" statement would set tso to 0.
	 * Removing this since we no longer need a tso variable.
	 */
	KASSERT(len + hdrlen + ipoptlen <= IP_MAXPACKET,
	    ("%s: len > IP_MAXPACKET", __func__));

	/*
	 * This KASSERT is here to catch edge cases at a well defined place.
	 * Before, those had triggered (random) panic conditions further down.
	 */
	KASSERT(len >= 0, ("[%s:%d]: len < 0", __func__, __LINE__));

	/*
	 * Grab a header mbuf, attaching a copy of data to
	 * be transmitted, and initialize the header from
	 * the template for sends on this connection.
	 */

	/*
	 * samkumar: The code to allocate, build, and send outgoing segments has
	 * been rewritten. I've left the original code to build the output mbuf
	 * here in a comment, for reference. The new code is below.
	 */
#if 0
	if (len) {
		struct mbuf *mb;
		uint32_t moff;

		if ((tp->t_flags & TF_FORCEDATA) && len == 1)
			TCPSTAT_INC(tcps_sndprobe);
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max) || sack_rxmit) {
			tp->t_sndrexmitpack++;
			TCPSTAT_INC(tcps_sndrexmitpack);
			TCPSTAT_ADD(tcps_sndrexmitbyte, len);
		} else {
			TCPSTAT_INC(tcps_sndpack);
			TCPSTAT_ADD(tcps_sndbyte, len);
		}
#ifdef INET6
		if (MHLEN < hdrlen + max_linkhdr)
			m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
		else
#endif
			m = m_gethdr(M_NOWAIT, MT_DATA);

		if (m == NULL) {
			SOCKBUF_UNLOCK(&so->so_snd);
			error = ENOBUFS;
			sack_rxmit = 0;
			goto out;
		}

		m->m_data += max_linkhdr;
		m->m_len = hdrlen;

		/*
		 * Start the m_copy functions from the closest mbuf
		 * to the offset in the socket buffer chain.
		 */
		mb = sbsndptr(&so->so_snd, off, len, &moff);

		if (len <= MHLEN - hdrlen - max_linkhdr) {
			m_copydata(mb, moff, (int)len,
			    mtod(m, caddr_t) + hdrlen);
			m->m_len += len;
		} else {
			m->m_next = m_copy(mb, moff, (int)len);
			if (m->m_next == NULL) {
				SOCKBUF_UNLOCK(&so->so_snd);
				(void) m_free(m);
				error = ENOBUFS;
				sack_rxmit = 0;
				goto out;
			}
		}

		/*
		 * If we're sending everything we've got, set PUSH.
		 * (This will keep happy those implementations which only
		 * give data to the user when a buffer fills or
		 * a PUSH comes in.)
		 */
		if (off + len == sbused(&so->so_snd))
			flags |= TH_PUSH;
		SOCKBUF_UNLOCK(&so->so_snd);
	} else {
		SOCKBUF_UNLOCK(&so->so_snd);
		if (tp->t_flags & TF_ACKNOW)
			TCPSTAT_INC(tcps_sndacks);
		else if (flags & (TH_SYN|TH_FIN|TH_RST))
			TCPSTAT_INC(tcps_sndctrl);
		else if (SEQ_GT(tp->snd_up, tp->snd_una))
			TCPSTAT_INC(tcps_sndurg);
		else
			TCPSTAT_INC(tcps_sndwinup);

		m = m_gethdr(M_NOWAIT, MT_DATA);
		if (m == NULL) {
			error = ENOBUFS;
			sack_rxmit = 0;
			goto out;
		}
#ifdef INET6
		if (isipv6 && (MHLEN < hdrlen + max_linkhdr) &&
		    MHLEN >= hdrlen) {
			M_ALIGN(m, hdrlen);
		} else
#endif
		m->m_data += max_linkhdr;
		m->m_len = hdrlen;
	}
#endif

	KASSERT(ipoptlen == 0, ("No IP options supported")); // samkumar

	otMessage* message = tcplp_sys_new_message(tp->instance);
	if (message == NULL) {
		error = ENOBUFS;
		sack_rxmit = 0;
		goto out;
	}
	if (otMessageSetLength(message, sizeof(struct tcphdr) + optlen + len) != OT_ERROR_NONE) {
		tcplp_sys_free_message(tp->instance, message);
		error = ENOBUFS;
		sack_rxmit = 0;
		goto out;
	}
	if (len) {
	    uint32_t used_space = lbuf_used_space(&tp->sendbuf);

		/*
		 * The TinyOS version has a way to avoid the copying we have to do here.
		 * Because it is possible to send iovecs directly in the BLIP stack, and
		 * an lbuf is made of iovecs, we could just "save" the starting and ending
		 * iovecs, modify them to get exactly the slice we want, call "send" on
		 * the resulting chain, and then restore the starting and ending iovecs
		 * once "send" returns.
		 *
		 * In RIOT, pktsnips have additional behavior regarding memory management
		 * that precludes this optimization. But, now that we have moved to
		 * cbufs, this is not relevant anymore.
		 */
		{
			otLinkedBuffer* start;
			size_t start_offset;
			otLinkedBuffer* end;
			size_t end_offset;
			otLinkedBuffer* curr;
			int rv = lbuf_getrange(&tp->sendbuf, off, len, &start, &start_offset, &end, &end_offset);
			size_t message_offset = otMessageGetOffset(message) + sizeof(struct tcphdr) + optlen;
			KASSERT(rv == 0, ("Reading send buffer out of range!"));
			for (curr = start; curr != end->mNext; curr = curr->mNext) {
				const uint8_t* data_to_copy = curr->mData;
				size_t length_to_copy = curr->mLength;
				if (curr == start) {
					data_to_copy += start_offset;
					length_to_copy -= start_offset;
				}
				if (curr == end) {
					length_to_copy -= end_offset;
				}
				otMessageWrite(message, message_offset, data_to_copy, length_to_copy);
				message_offset += length_to_copy;
			}
		}

		/*
		 * If we're sending everything we've got, set PUSH.
		 * (This will keep happy those implementations which only
		 * give data to the user when a buffer fills or
		 * a PUSH comes in.)
		 */
		/* samkumar: Replaced call to sbused(&so->so_snd) with used_space. */
		if (off + len == used_space)
			flags |= TH_PUSH;
	}

	char outbuf[sizeof(struct tcphdr) + TCP_MAXOLEN];
	th = (struct tcphdr*) (&outbuf[0]);

	/*
	 * samkumar: I replaced the original call to tcpip_fillheaders with the
	 * one below.
	 */
	otMessageInfo ip6info;
	tcpip_fillheaders(tp, &ip6info, th);

	/*
	 * Fill in fields, remembering maximum advertised
	 * window for use in delaying messages about window sizes.
	 * If resending a FIN, be sure not to use a new sequence number.
	 */
	if (flags & TH_FIN && tp->t_flags & TF_SENTFIN &&
	    tp->snd_nxt == tp->snd_max)
		tp->snd_nxt--;
	/*
	 * If we are starting a connection, send ECN setup
	 * SYN packet. If we are on a retransmit, we may
	 * resend those bits a number of times as per
	 * RFC 3168.
	 */
	if (tp->t_state == TCPS_SYN_SENT && V_tcp_do_ecn) {
		if (tp->t_rxtshift >= 1) {
			if (tp->t_rxtshift <= V_tcp_ecn_maxretries)
				flags |= TH_ECE|TH_CWR;
		} else
			flags |= TH_ECE|TH_CWR;
	}

	/*
	 * samkumar: Make tcp_output reply with ECE flag in the SYN-ACK for
	 * ECN-enabled connections. The existing code in FreeBSD didn't have to do
	 * this, because it didn't use tcp_output to send the SYN-ACK; it
	 * constructed the SYN-ACK segment manually. Yet another consequnce of
	 * removing the SYN cache...
	 */
	if (tp->t_state == TCPS_SYN_RECEIVED && tp->t_flags & TF_ECN_PERMIT &&
		V_tcp_do_ecn) {
		flags |= TH_ECE;
	}

	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tp->t_flags & TF_ECN_PERMIT)) {
		/*
		 * If the peer has ECN, mark data packets with
		 * ECN capable transmission (ECT).
		 * Ignore pure ack packets, retransmissions and window probes.
		 */
		if (len > 0 && SEQ_GEQ(tp->snd_nxt, tp->snd_max) &&
		    !((tp->t_flags & TF_FORCEDATA) && len == 1)) {
			/*
			 * samkumar: Replaced ip6->ip6_flow |= htonl(IPTOS_ECN_ECT0 << 20);
			 * with the following code, which will cause OpenThread to set the
			 * ECT0 bit in the header.
			 */
			ip6info.mEcn = OT_ECN_CAPABLE_0;
		}

		/*
		 * Reply with proper ECN notifications.
		 */
		if (tp->t_flags & TF_ECN_SND_CWR) {
			flags |= TH_CWR;
			tp->t_flags &= ~TF_ECN_SND_CWR;
		}
		if (tp->t_flags & TF_ECN_SND_ECE)
			flags |= TH_ECE;
	}

	/*
	 * If we are doing retransmissions, then snd_nxt will
	 * not reflect the first unsent octet.  For ACK only
	 * packets, we do not want the sequence number of the
	 * retransmitted packet, we want the sequence number
	 * of the next unsent octet.  So, if there is no data
	 * (and no SYN or FIN), use snd_max instead of snd_nxt
	 * when filling in ti_seq.  But if we are in persist
	 * state, snd_max might reflect one byte beyond the
	 * right edge of the window, so use snd_nxt in that
	 * case, since we know we aren't doing a retransmission.
	 * (retransmit and persist are mutually exclusive...)
	 */
	if (sack_rxmit == 0) {
		if (len || (flags & (TH_SYN|TH_FIN)) ||
		    tcp_timer_active(tp, TT_PERSIST))
			th->th_seq = htonl(tp->snd_nxt);
		else
			th->th_seq = htonl(tp->snd_max);
	} else {
		th->th_seq = htonl(p->rxmit);
		p->rxmit += len;
		tp->sackhint.sack_bytes_rexmit += len;
	}

	/*
	 * samkumar: Check if this is a retransmission (added as part of TCPlp).
	 * This kind of stats collection is useful but not necessary for TCP, so
	 * I've left it as a comment in case we want to bring this back to measure
	 * performance.
	 */
#if 0
	if (len > 0 && !tcp_timer_active(tp, TT_PERSIST) && SEQ_LT(ntohl(th->th_seq), tp->snd_max)) {
		tcplp_totalRexmitCnt++;
	}
#endif

	th->th_ack = htonl(tp->rcv_nxt);
	if (optlen) {
		bcopy(opt, th + 1, optlen);
		th->th_off_x2 = ((sizeof (struct tcphdr) + optlen) >> 2) << TH_OFF_SHIFT;
	}
	th->th_flags = flags;
	/*
	 * Calculate receive window.  Don't shrink window,
	 * but avoid silly window syndrome.
	 */
	/* samkumar: Replaced so->so_rcv.sb_hiwat with this call to cbuf_size. */
	if (recwin < (long)(cbuf_size(&tp->recvbuf) / 4) &&
	    recwin < (long)tp->t_maxseg)
		recwin = 0;
	if (SEQ_GT(tp->rcv_adv, tp->rcv_nxt) &&
	    recwin < (long)(tp->rcv_adv - tp->rcv_nxt))
		recwin = (long)(tp->rcv_adv - tp->rcv_nxt);
	if (recwin > (long)TCP_MAXWIN << tp->rcv_scale)
		recwin = (long)TCP_MAXWIN << tp->rcv_scale;

	/*
	 * According to RFC1323 the window field in a SYN (i.e., a <SYN>
	 * or <SYN,ACK>) segment itself is never scaled.  The <SYN,ACK>
	 * case is handled in syncache.
	 */
	if (flags & TH_SYN)
		th->th_win = htons((uint16_t)
				(min(cbuf_size(&tp->recvbuf), TCP_MAXWIN)));
	else
		th->th_win = htons((uint16_t)(recwin >> tp->rcv_scale));

	/*
	 * Adjust the RXWIN0SENT flag - indicate that we have advertised
	 * a 0 window.  This may cause the remote transmitter to stall.  This
	 * flag tells soreceive() to disable delayed acknowledgements when
	 * draining the buffer.  This can occur if the receiver is attempting
	 * to read more data than can be buffered prior to transmitting on
	 * the connection.
	 */
	if (th->th_win == 0) {
		tp->t_flags |= TF_RXWIN0SENT;
	} else
		tp->t_flags &= ~TF_RXWIN0SENT;
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		th->th_urp = htons((uint16_t)(tp->snd_up - tp->snd_nxt));
		th->th_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull
		 * the urgent pointer to the left edge of the send window
		 * so that it doesn't drift into the send window on sequence
		 * number wraparound.
		 */
		tp->snd_up = tp->snd_una;		/* drag it along */

	/*
	 * samkumar: Removed code for TCP signatures.
	 */
	/*
	 * Put TCP length in extended header, and then
	 * checksum extended header and data.
	 */
	/*
	 * samkumar: The code to implement the above comment isn't relevant to us.
	 * Checksum computation is not handled using FreeBSD code, so we don't need
	 * to build an extended header.
	 */
	/*
	 * samkumar: Removed code for TCP Segmentation Offloading.
	 */
	/* samkumar: Removed mbuf-specific assertions an debug code. */
	/*
	 * Fill in IP length and desired time to live and
	 * send to IP level.  There should be a better way
	 * to handle ttl and tos; we could keep them in
	 * the template, but need a way to checksum without them.
	 */
	/*
	 * m->m_pkthdr.len should have been set before checksum calculation,
	 * because in6_cksum() need it.
	 */
	/*
	 * samkumar: The IPv6 packet length and hop limit are handled by the host
	 * network stack, not by TCPlp. I've also removed code for Path MTU
	 * discovery. And of course, I've removed debug code as well.
	 */
	/* samkumar: I've replaced the call to ip6_output with the following. */
	otMessageWrite(message, 0, outbuf, sizeof(struct tcphdr) + optlen);
	tcplp_sys_send_message(tp->instance, message, &ip6info);

out:
	/*
	 * In transmit state, time the transmission and arrange for
	 * the retransmit.  In persist state, just set snd_max.
	 */
	if ((tp->t_flags & TF_FORCEDATA) == 0 ||
	    !tcp_timer_active(tp, TT_PERSIST)) {
		tcp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment.
		 */
		if (flags & (TH_SYN|TH_FIN)) {
			if (flags & TH_SYN)
				tp->snd_nxt++;
			if (flags & TH_FIN) {
				tp->snd_nxt++;
				tp->t_flags |= TF_SENTFIN;
			}
		}
		if (sack_rxmit)
			goto timer;
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (tp->t_rtttime == 0) {
				tp->t_rtttime = ticks;
				tp->t_rtseq = startseq;
			}
		}

		/*
		 * Set retransmit timer if not currently set,
		 * and not doing a pure ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
timer:
		if (!tcp_timer_active(tp, TT_REXMT) &&
		    ((sack_rxmit && tp->snd_nxt != tp->snd_max) ||
		     (tp->snd_nxt != tp->snd_una))) {
			if (tcp_timer_active(tp, TT_PERSIST)) {
				tcp_timer_activate(tp, TT_PERSIST, 0);
				tp->t_rxtshift = 0;
			}
			tcp_timer_activate(tp, TT_REXMT, tp->t_rxtcur);
			/*
			 * samkumar: Replaced sbavail(&so->so_snd) with this call to
			 * lbuf_used_space.
			 */
		} else if (len == 0 && lbuf_used_space(&tp->sendbuf) &&
		    !tcp_timer_active(tp, TT_REXMT) &&
		    !tcp_timer_active(tp, TT_PERSIST)) {
			/*
			 * Avoid a situation where we do not set persist timer
			 * after a zero window condition. For example:
			 * 1) A -> B: packet with enough data to fill the window
			 * 2) B -> A: ACK for #1 + new data (0 window
			 *    advertisement)
			 * 3) A -> B: ACK for #2, 0 len packet
			 *
			 * In this case, A will not activate the persist timer,
			 * because it chose to send a packet. Unless tcp_output
			 * is called for some other reason (delayed ack timer,
			 * another input packet from B, socket syscall), A will
			 * not send zero window probes.
			 *
			 * So, if you send a 0-length packet, but there is data
			 * in the socket buffer, and neither the rexmt or
			 * persist timer is already set, then activate the
			 * persist timer.
			 */
			tp->t_rxtshift = 0;
			tcp_setpersist(tp);
		}
	} else {
		/*
		 * Persist case, update snd_max but since we are in
		 * persist mode (no window) we do not update snd_nxt.
		 */
		int xlen = len;
		if (flags & TH_SYN)
			++xlen;
		if (flags & TH_FIN) {
			++xlen;
			tp->t_flags |= TF_SENTFIN;
		}
		if (SEQ_GT(tp->snd_nxt + xlen, tp->snd_max))
			tp->snd_max = tp->snd_nxt + len;
	}

	if (error) {

		/*
		 * We know that the packet was lost, so back out the
		 * sequence number advance, if any.
		 *
		 * If the error is EPERM the packet got blocked by the
		 * local firewall.  Normally we should terminate the
		 * connection but the blocking may have been spurious
		 * due to a firewall reconfiguration cycle.  So we treat
		 * it like a packet loss and let the retransmit timer and
		 * timeouts do their work over time.
		 * XXX: It is a POLA question whether calling tcp_drop right
		 * away would be the really correct behavior instead.
		 */
		if (((tp->t_flags & TF_FORCEDATA) == 0 ||
		    !tcp_timer_active(tp, TT_PERSIST)) &&
		    ((flags & TH_SYN) == 0) &&
		    (error != EPERM)) {
			if (sack_rxmit) {
				p->rxmit -= len;
				tp->sackhint.sack_bytes_rexmit -= len;
				KASSERT(tp->sackhint.sack_bytes_rexmit >= 0,
				    ("sackhint bytes rtx >= 0"));
			} else
				tp->snd_nxt -= len;
		}
		switch (error) {
		case EPERM:
			tp->t_softerror = error;
			return (error);
		case ENOBUFS:
	                if (!tcp_timer_active(tp, TT_REXMT) &&
			    !tcp_timer_active(tp, TT_PERSIST))
	                        tcp_timer_activate(tp, TT_REXMT, tp->t_rxtcur);
			tp->snd_cwnd = tp->t_maxseg;
#ifdef INSTRUMENT_TCP
			tcplp_sys_log("TCP ALLOCFAIL %u %d", (unsigned int) tcplp_sys_get_millis(), (int) tp->snd_cwnd);
#endif
			return (0);
		case EMSGSIZE:
			/*
			 * For some reason the interface we used initially
			 * to send segments changed to another or lowered
			 * its MTU.
			 * If TSO was active we either got an interface
			 * without TSO capabilits or TSO was turned off.
			 * If we obtained mtu from ip_output() then update
			 * it and try again.
			 */
			/* samkumar: Removed code for TCP Segmentation Offloading. */
			if (mtu != 0) {
				tcp_mss_update(tp, -1, mtu, NULL, NULL);
				goto again;
			}
			return (error);
		case EHOSTDOWN:
		case EHOSTUNREACH:
		case ENETDOWN:
		case ENETUNREACH:
			if (TCPS_HAVERCVDSYN(tp->t_state)) {
				tp->t_softerror = error;
				return (0);
			}
			/* FALLTHROUGH */
		default:
			return (error);
		}
	}

	/*
	 * Data sent (as far as we can tell).
	 * If this advertises a larger window than any other segment,
	 * then remember the size of the advertised window.
	 * Any pending ACK has now been sent.
	 */
	if (recwin >= 0 && SEQ_GT(tp->rcv_nxt + recwin, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + recwin;
	tp->last_ack_sent = tp->rcv_nxt;
	tp->t_flags &= ~(TF_ACKNOW | TF_DELACK);
	if (tcp_timer_active(tp, TT_DELACK))
		tcp_timer_activate(tp, TT_DELACK, 0);

	/*
	 * samkumar: This was already commented out (using #if 0) in the original
	 * FreeBSD code.
	 */
#if 0
	/*
	 * This completely breaks TCP if newreno is turned on.  What happens
	 * is that if delayed-acks are turned on on the receiver, this code
	 * on the transmitter effectively destroys the TCP window, forcing
	 * it to four packets (1.5Kx4 = 6K window).
	 */
	if (sendalot && --maxburst)
		goto again;
#endif
	if (sendalot)
		goto again;
	return (0);
}

/*
 * Insert TCP options according to the supplied parameters to the place
 * optp in a consistent way.  Can handle unaligned destinations.
 *
 * The order of the option processing is crucial for optimal packing and
 * alignment for the scarce option space.
 *
 * The optimal order for a SYN/SYN-ACK segment is:
 *   MSS (4) + NOP (1) + Window scale (3) + SACK permitted (2) +
 *   Timestamp (10) + Signature (18) = 38 bytes out of a maximum of 40.
 *
 * The SACK options should be last.  SACK blocks consume 8*n+2 bytes.
 * So a full size SACK blocks option is 34 bytes (with 4 SACK blocks).
 * At minimum we need 10 bytes (to generate 1 SACK block).  If both
 * TCP Timestamps (12 bytes) and TCP Signatures (18 bytes) are present,
 * we only have 10 bytes for SACK options (40 - (12 + 18)).
 */
int
tcp_addoptions(struct tcpopt *to, uint8_t *optp)
{
	uint32_t mask, optlen = 0;

	for (mask = 1; mask < TOF_MAXOPT; mask <<= 1) {
		if ((to->to_flags & mask) != mask)
			continue;
		if (optlen == TCP_MAXOLEN)
			break;
		switch (to->to_flags & mask) {
		case TOF_MSS:
			while (optlen % 4) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_MAXSEG)
				continue;
			optlen += TCPOLEN_MAXSEG;
			*optp++ = TCPOPT_MAXSEG;
			*optp++ = TCPOLEN_MAXSEG;
			to->to_mss = htons(to->to_mss);
			bcopy((uint8_t *)&to->to_mss, optp, sizeof(to->to_mss));
			optp += sizeof(to->to_mss);
			break;
		case TOF_SCALE:
			while (!optlen || optlen % 2 != 1) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_WINDOW)
				continue;
			optlen += TCPOLEN_WINDOW;
			*optp++ = TCPOPT_WINDOW;
			*optp++ = TCPOLEN_WINDOW;
			*optp++ = to->to_wscale;
			break;
		case TOF_SACKPERM:
			while (optlen % 2) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_SACK_PERMITTED)
				continue;
			optlen += TCPOLEN_SACK_PERMITTED;
			*optp++ = TCPOPT_SACK_PERMITTED;
			*optp++ = TCPOLEN_SACK_PERMITTED;
			break;
		case TOF_TS:
			while (!optlen || optlen % 4 != 2) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_TIMESTAMP)
				continue;
			optlen += TCPOLEN_TIMESTAMP;
			*optp++ = TCPOPT_TIMESTAMP;
			*optp++ = TCPOLEN_TIMESTAMP;
			to->to_tsval = htonl(to->to_tsval);
			to->to_tsecr = htonl(to->to_tsecr);
			bcopy((uint8_t *)&to->to_tsval, optp, sizeof(to->to_tsval));
			optp += sizeof(to->to_tsval);
			bcopy((uint8_t *)&to->to_tsecr, optp, sizeof(to->to_tsecr));
			optp += sizeof(to->to_tsecr);
			break;
		case TOF_SIGNATURE:
			{
			int siglen = TCPOLEN_SIGNATURE - 2;

			while (!optlen || optlen % 4 != 2) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_SIGNATURE)
				continue;
			optlen += TCPOLEN_SIGNATURE;
			*optp++ = TCPOPT_SIGNATURE;
			*optp++ = TCPOLEN_SIGNATURE;
			to->to_signature = optp;
			while (siglen--)
				 *optp++ = 0;
			break;
			}
		case TOF_SACK:
			{
			int sackblks = 0;
			struct sackblk *sack = (struct sackblk *)to->to_sacks;
			tcp_seq sack_seq;

			while (!optlen || optlen % 4 != 2) {
				optlen += TCPOLEN_NOP;
				*optp++ = TCPOPT_NOP;
			}
			if (TCP_MAXOLEN - optlen < TCPOLEN_SACKHDR + TCPOLEN_SACK)
				continue;
			optlen += TCPOLEN_SACKHDR;
			*optp++ = TCPOPT_SACK;
			sackblks = min(to->to_nsacks,
					(TCP_MAXOLEN - optlen) / TCPOLEN_SACK);
			*optp++ = TCPOLEN_SACKHDR + sackblks * TCPOLEN_SACK;
			while (sackblks--) {
				sack_seq = htonl(sack->start);
				bcopy((uint8_t *)&sack_seq, optp, sizeof(sack_seq));
				optp += sizeof(sack_seq);
				sack_seq = htonl(sack->end);
				bcopy((uint8_t *)&sack_seq, optp, sizeof(sack_seq));
				optp += sizeof(sack_seq);
				optlen += TCPOLEN_SACK;
				sack++;
			}
			/* samkumar: Removed TCPSTAT_INC(tcps_sack_send_blocks); */
			break;
			}
		default:
			tcplp_sys_panic("PANIC: %s: unknown TCP option type", __func__);
			break;
		}
	}

	/* Terminate and pad TCP options to a 4 byte boundary. */
	if (optlen % 4) {
		optlen += TCPOLEN_EOL;
		*optp++ = TCPOPT_EOL;
	}
	/*
	 * According to RFC 793 (STD0007):
	 *   "The content of the header beyond the End-of-Option option
	 *    must be header padding (i.e., zero)."
	 *   and later: "The padding is composed of zeros."
	 */
	while (optlen % 4) {
		optlen += TCPOLEN_PAD;
		*optp++ = TCPOPT_PAD;
	}

	KASSERT(optlen <= TCP_MAXOLEN, ("%s: TCP options too long", __func__));
	return (optlen);
}
