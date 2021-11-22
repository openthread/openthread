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

/*
 * samkumar: The V_nolocaltimewait variable corresponds to the
 * net.inet.tcp.nolocaltimewait option in FreeBSD. When set to 1, it skips the
 * TIME-WAIT state for TCP connections where both endpoints are local IP
 * addresses, to save resources on HTTP accelerators, database servers/clients,
 * etc. In TCPlp, I eliminated support for this feature, but I have kept the
 * code for it, commented out with "#if 0", in case we choose to bring it back
 * at a later time.
 *
 * See also the "#if 0" block in tcp_twstart.
 */
#if 0
enum tcp_timewait_consts {
	V_nolocaltimewait = 0
};
#endif

/*
 * samkumar: The FreeBSD code used a separate, smaller structure, called
 * struct tcptw, to respresent connections in the TIME-WAIT state. In TCPlp,
 * we use the full struct tcpcb structure even in the TIME-WAIT state. This
 * consumes more memory, but switching to a different structure like
 * struct tcptw to save memory would be difficult because the host system or
 * application has allocated these structures; we can't simply "free" the
 * struct tcpcb. It would have to have been done via a callback or something,
 * and in the common case of statically allocated sockets, this would actually
 * result in more memory (since an application would need to allocate both the
 * struct tcpcb and the struct tcptw, if it uses a static allocation approach).
 *
 * Below, I've changed the function signatures to accept "struct tcpcb* tp"
 * instead of "struct tcptw *tw" and I have reimplemented the functions
 * to work using tp (of type struct tcpcb) instead of tw (of type
 * struct tcptw).
 *
 * Conceptually, the biggest change is in how timers are handled. The FreeBSD
 * code had a 2MSL timer, which was set for sockets that enter certain
 * "closing" states of the TCP state machine. But when the TIME-WAIT state was
 * entered, the state is transferred from struct tcpcb into struct tcptw.
 * The final timeout is handled as follows; the function tcp_tw_2msl_scan is
 * called periodically on the slow timer, and it iterates over a linked list
 * of all the struct tcptw and checks the tw->tw_time field to identify which
 * TIME-WAIT sockets have expired.
 *
 * In our switch to using struct tcpcb even in the TIME-WAIT state, we rely on
 * the timer system for struct tcpcb. I modified the 2msl callback in
 * tcp_timer.c to check for the TIME-WAIT case and handle it correctly.
 */

static void
tcp_tw_2msl_reset(struct tcpcb* tp, int rearm)
{
	/*
	 * samkumar: This function used to set tw->tw_time to ticks + 2 * tcp_msl
	 * and insert tw into the linked list V_twq_2msl. I've replaced this, along
	 * with the associated locking logic, with the following call, which uses
	 * the timer system in place for full TCBs.
	 */
	tcp_timer_activate(tp, TT_2MSL, 2 * tcp_msl);
}

/*
 * samkumar: I've rewritten this code since I need to send out packets via the
 * host system for TCPlp: allocating buffers from the host system, populate
 * them, and then pass them back to the host system. I simplified the code by
 * only using the logic that was fully necessary, eliminating the code for IPv4
 * packets and keeping only the code for IPv6 packets. I also removed all of
 * the mbuf logic, instead using the logic for using the host system's
 * buffering.
 *
 * This rewritten code always returns 0. The original code would return
 * whatever is returned by ip_output or ip6_output (FreeBSD's functions for
 * sending out IP packets). I believe 0 indicates success, and a nonzero
 * value represents an error code. It seems that the return value of
 * tcp_twrespond is ignored by all instances of its use in TCPlp (maybe even
 * in all of FreeBSD), so this is a moot point.
 */
static int
tcp_twrespond(struct tcpcb* tp, int flags)
{
	struct tcphdr* nth;
	struct tcpopt to;
	uint32_t optlen = 0;
	uint8_t opt[TCP_MAXOLEN];

	to.to_flags = 0;

	/*
	 * Send a timestamp and echo-reply if both our side and our peer
	 * have sent timestamps in our SYN's and this is not a RST.
	 */
	if ((tp->t_flags & TF_RCVD_TSTMP) && flags == TH_ACK) {
		to.to_flags |= TOF_TS;
		to.to_tsval = tcp_ts_getticks() + tp->ts_offset;
		to.to_tsecr = tp->ts_recent;
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

	char outbuf[sizeof(struct tcphdr) + optlen];
	nth = (struct tcphdr*) &outbuf[0];
	otMessageInfo ip6info;
	memset(&ip6info, 0x00, sizeof(ip6info));

	memcpy(&ip6info.mSockAddr, &tp->laddr, sizeof(ip6info.mSockAddr));
	memcpy(&ip6info.mPeerAddr, &tp->faddr, sizeof(ip6info.mPeerAddr));
	nth->th_sport = tp->lport;
	nth->th_dport = tp->fport;
	nth->th_seq = htonl(tp->snd_nxt);
	nth->th_ack = htonl(tp->rcv_nxt);
	nth->th_off_x2 = ((sizeof(struct tcphdr) + optlen) >> 2) << TH_OFF_SHIFT;
	nth->th_flags = flags;
	nth->th_win = htons(tp->tw_last_win);
	nth->th_urp = 0;
	nth->th_sum = 0;

	memcpy(nth + 1, opt, optlen);
	otMessageWrite(message, 0, outbuf, sizeof(struct tcphdr) + optlen);
	tcplp_sys_send_message(tp->instance, message, &ip6info);

	return 0;
}

/*
 * Move a TCP connection into TIME_WAIT state.
 *    tcbinfo is locked.
 *    inp is locked, and is unlocked before returning.
 */
/*
 * samkumar: Locking is removed (so above comments regarding locks are no
 * not relevant for TCPlp). Rather than allocating a struct tcptw and
 * discarding the struct tcpcb, this function just switches the tcpcb state
 * to correspond to TIME-WAIT (updating variables as appropriate). We also
 * eliminate the "V_nolocaltimewait" optimization.
 */
void
tcp_twstart(struct tcpcb *tp)
{
	int acknow;

	/*
	 * samkumar: The following code, commented out using "#if 0", handles the
	 * net.inet.tcp.nolocaltimewait option in FreeBSD. The option skips the
	 * TIME-WAIT state for TCP connections where both endpoints are local.
	 * I'm removing this optimization for TCPlp, but I've left the code
	 * commented out as it's a potentially useful feature that we may choose
	 * to restore later.
	 *
	 * See also the "#if 0" block near the top of this file.
	 */
#if 0
	if (V_nolocaltimewait) {
		int error = 0;
#ifdef INET6
		if (isipv6)
			error = in6_localaddr(&inp->in6p_faddr);
#endif
#if defined(INET6) && defined(INET)
		else
#endif
#ifdef INET
			error = in_localip(inp->inp_faddr);
#endif
		if (error) {
			tp = tcp_close(tp);
			if (tp != NULL)
				INP_WUNLOCK(inp);
			return;
		}
	}
#endif

	/*
	 * For use only by DTrace.  We do not reference the state
	 * after this point so modifying it in place is not a problem.
	 */
	/*
	 * samkumar: The above comment is not true anymore. I use this state, since
	 * I don't associate every struct tcpcb with a struct inpcb.
	 */
	tcp_state_change(tp, TCPS_TIME_WAIT);

	/*
	 * samkumar: There used to be code here to allocate a struct tcptw
	 * using "tw = uma_zalloc(V_tcptw_zone, M_NOWAIT);" and if it fails, close
	 * an existing TIME-WAIT connection, in LRU fashion, to allocate memory.
	 */

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
		/*
		 * samkumar: This used to do:
		 * tw->t_recent = tp->ts_recent;
		 * tw->ts_offset = tp->ts_offset;
		 * But since we're keeping the state in tp, we don't need to do this
		 * anymore. */
	} else {
		tp->ts_recent = 0;
		tp->ts_offset = 0;
	}

	/*
	 * samkumar: There used to be code here to populate various fields in
	 * tw based on their values in tp, but there's no need for that now since
	 * we can just read the values from tp. tw->tw_time was set to 0, but we
	 * don't need to do that either since we're relying on the old timer system
	 * anyway.
	 */

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
	/*
	 * samkumar: Below, I removed the code to discard tp, update inpcb and
	 * release a reference to socket, but kept the rest. I also added a call
	 * to cancel any pending timers on the TCB (which discarding it, as the
	 * original code did, would have done).
	 */
	tcp_cancel_timers(tp);
	if (acknow)
		tcp_twrespond(tp, TH_ACK);
	tcp_tw_2msl_reset(tp, 0);
}

/*
 * Returns 1 if the TIME_WAIT state was killed and we should start over,
 * looking for a pcb in the listen state.  Returns 0 otherwise.
 */
/*
 * samkumar: Old signature was
 * int
 * tcp_twcheck(struct inpcb *inp, struct tcpopt *to, struct tcphdr *th,
 *    struct mbuf *m, int tlen)
 */
int
tcp_twcheck(struct tcpcb* tp, struct tcphdr *th, int tlen)
{
	int thflags;
	tcp_seq seq;

	/*
	 * samkumar: There used to be code here that obtains the struct tcptw from
	 * the inpcb, and does "goto drop" if that fails.
	 */

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

	/*
	 * samkumar: This was commented out (using #if 0) in the original FreeBSD
	 * code.
	 */
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
	if ((thflags & TH_SYN) && SEQ_GT(th->th_seq, tp->rcv_nxt)) {
		/*
		 * samkumar: The FreeBSD code would call tcp_twclose(tw, 0); but we
		 * do it as below since TCPlp represents TIME-WAIT connects as
		 * struct tcpcb's.
		 */
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
		if (seq + 1 == tp->rcv_nxt)
			tcp_tw_2msl_reset(tp, 1);
	}

	/*
	 * Acknowledge the segment if it has data or is not a duplicate ACK.
	 */
	if (thflags != TH_ACK || tlen != 0 ||
	    th->th_seq != tp->rcv_nxt || th->th_ack != tp->snd_nxt)
		tcp_twrespond(tp, TH_ACK);
drop:
	return (0);
}
