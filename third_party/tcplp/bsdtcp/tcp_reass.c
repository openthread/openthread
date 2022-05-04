/*-
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
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
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 */

#include "../tcplp.h"
#include "../lib/bitmap.h"
#include "../lib/cbuf.h"
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_var.h"

/*
 * samkumar: Segments are only reassembled within the window; data outside the
 * window is thrown away. So, the total amount of reassembly data cannot exceed
 * the size of the receive window.
 *
 * I have essentially rewritten it to use TCPlp's data structure for the
 * reassembly buffer. I have kept the original code as a comment below this
 * function, for reference.
 *
 * Looking at the usage of this function in tcp_input, this just has to set
 * *tlenp to 0 if the received segment is already completely buffered; it does
 * not need to update it if only part of the segment is trimmed off.
 */
int
tcp_reass(struct tcpcb* tp, struct tcphdr* th, int* tlenp, otMessage* data, off_t data_offset, struct tcplp_signals* sig)
{
	size_t mergeable, written;
	size_t offset;
	size_t start_index;
	size_t usedbefore;
	int tlen;
	size_t merged = 0;
	int flags = 0;

	/*
	 * Call with th==NULL after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (th == NULL)
		goto present;

	tlen = *tlenp;

	/* Insert the new segment queue entry into place. */
	KASSERT(SEQ_GEQ(th->th_seq, tp->rcv_nxt), ("Adding past segment to the reassembly queue"));
	offset = (size_t) (th->th_seq - tp->rcv_nxt);

	if (cbuf_reass_count_set(&tp->recvbuf, (size_t) offset, tp->reassbmp, tlen) >= (size_t) tlen) {
		*tlenp = 0;
		goto present;
	}
	written = cbuf_reass_write(&tp->recvbuf, (size_t) offset, data, data_offset, tlen, tp->reassbmp, &start_index, cbuf_copy_from_message);

	if ((th->th_flags & TH_FIN) && (tp->reass_fin_index == -1)) {
		tp->reass_fin_index = (int16_t) (start_index + tlen);
	}
	KASSERT(written == tlen, ("Reassembly write out of bounds: tried to write %d, but wrote %d", tlen, (int) written));

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	mergeable = cbuf_reass_count_set(&tp->recvbuf, 0, tp->reassbmp, (size_t) 0xFFFFFFFF);
	usedbefore = cbuf_used_space(&tp->recvbuf);
	if (!tpiscantrcv(tp) || usedbefore == 0) {
		/* If usedbefore == 0, but we can't receive more, then we still need to move the buffer
		   along by merging and then popping, in case we receive a FIN later on. */
		if (tp->reass_fin_index >= 0 && cbuf_reass_within_offset(&tp->recvbuf, mergeable, (size_t) tp->reass_fin_index)) {
			tp->reass_fin_index = -2; // So we won't consider any more FINs
			flags = TH_FIN;
		}
		merged = cbuf_reass_merge(&tp->recvbuf, mergeable, tp->reassbmp);
		KASSERT(merged == mergeable, ("Reassembly merge out of bounds: tried to merge %d, but merged %d", (int) mergeable, (int) merged));
		if (tpiscantrcv(tp)) {
			cbuf_pop(&tp->recvbuf, merged); // So no data really enters the buffer
		} else if (merged > 0) {
			sig->recvbuf_added = true;
		}
	} else {
		/* If there is data in the buffer AND we can't receive more, then that must be because we received a FIN,
		   but the user hasn't yet emptied the buffer of its contents. */
		KASSERT (tp->reass_fin_index == -2, ("Can't receive more, and data in buffer, but haven't received a FIN"));
	}

	tp->rcv_nxt += mergeable;

	return flags;
}
#if 0
int
tcp_reass(struct tcpcb *tp, struct tcphdr *th, int *tlenp, struct mbuf *m)
{
	struct tseg_qent *q;
	struct tseg_qent *p = NULL;
	struct tseg_qent *nq;
	struct tseg_qent *te = NULL;
	struct socket *so = tp->t_inpcb->inp_socket;
	char *s = NULL;
	int flags;
	struct tseg_qent tqs;

	INP_WLOCK_ASSERT(tp->t_inpcb);

	/*
	 * XXX: tcp_reass() is rather inefficient with its data structures
	 * and should be rewritten (see NetBSD for optimizations).
	 */

	/*
	 * Call with th==NULL after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (th == NULL)
		goto present;

	/*
	 * Limit the number of segments that can be queued to reduce the
	 * potential for mbuf exhaustion. For best performance, we want to be
	 * able to queue a full window's worth of segments. The size of the
	 * socket receive buffer determines our advertised window and grows
	 * automatically when socket buffer autotuning is enabled. Use it as the
	 * basis for our queue limit.
	 * Always let the missing segment through which caused this queue.
	 * NB: Access to the socket buffer is left intentionally unlocked as we
	 * can tolerate stale information here.
	 *
	 * XXXLAS: Using sbspace(so->so_rcv) instead of so->so_rcv.sb_hiwat
	 * should work but causes packets to be dropped when they shouldn't.
	 * Investigate why and re-evaluate the below limit after the behaviour
	 * is understood.
	 */
	if ((th->th_seq != tp->rcv_nxt || !TCPS_HAVEESTABLISHED(tp->t_state)) &&
	    tp->t_segqlen >= (so->so_rcv.sb_hiwat / tp->t_maxseg) + 1) {
		TCPSTAT_INC(tcps_rcvreassfull);
		*tlenp = 0;
		if ((s = tcp_log_addrs(&tp->t_inpcb->inp_inc, th, NULL, NULL))) {
			log(LOG_DEBUG, "%s; %s: queue limit reached, "
			    "segment dropped\n", s, __func__);
			free(s, M_TCPLOG);
		}
		m_freem(m);
		return (0);
	}

	/*
	 * Allocate a new queue entry. If we can't, or hit the zone limit
	 * just drop the pkt.
	 *
	 * Use a temporary structure on the stack for the missing segment
	 * when the zone is exhausted. Otherwise we may get stuck.
	 */
	te = uma_zalloc(tcp_reass_zone, M_NOWAIT);
	if (te == NULL) {
		if (th->th_seq != tp->rcv_nxt || !TCPS_HAVEESTABLISHED(tp->t_state)) {
			TCPSTAT_INC(tcps_rcvmemdrop);
			m_freem(m);
			*tlenp = 0;
			if ((s = tcp_log_addrs(&tp->t_inpcb->inp_inc, th, NULL,
			    NULL))) {
				log(LOG_DEBUG, "%s; %s: global zone limit "
				    "reached, segment dropped\n", s, __func__);
				free(s, M_TCPLOG);
			}
			return (0);
		} else {
			bzero(&tqs, sizeof(struct tseg_qent));
			te = &tqs;
			if ((s = tcp_log_addrs(&tp->t_inpcb->inp_inc, th, NULL,
			    NULL))) {
				log(LOG_DEBUG,
				    "%s; %s: global zone limit reached, using "
				    "stack for missing segment\n", s, __func__);
				free(s, M_TCPLOG);
			}
		}
	}
	tp->t_segqlen++;

	/*
	 * Find a segment which begins after this one does.
	 */
	LIST_FOREACH(q, &tp->t_segq, tqe_q) {
		if (SEQ_GT(q->tqe_th->th_seq, th->th_seq))
			break;
		p = q;
	}

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if (p != NULL) {
		int i;
		/* conversion to int (in i) handles seq wraparound */
		i = p->tqe_th->th_seq + p->tqe_len - th->th_seq;
		if (i > 0) {
			if (i >= *tlenp) {
				TCPSTAT_INC(tcps_rcvduppack);
				TCPSTAT_ADD(tcps_rcvdupbyte, *tlenp);
				m_freem(m);
				if (te != &tqs)
					uma_zfree(tcp_reass_zone, te);
				tp->t_segqlen--;
				/*
				 * Try to present any queued data
				 * at the left window edge to the user.
				 * This is needed after the 3-WHS
				 * completes.
				 */
				goto present;	/* ??? */
			}
			m_adj(m, i);
			*tlenp -= i;
			th->th_seq += i;
		}
	}
	tp->t_rcvoopack++;
	TCPSTAT_INC(tcps_rcvoopack);
	TCPSTAT_ADD(tcps_rcvoobyte, *tlenp);

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q) {
		int i = (th->th_seq + *tlenp) - q->tqe_th->th_seq;
		if (i <= 0)
			break;
		if (i < q->tqe_len) {
			q->tqe_th->th_seq += i;
			q->tqe_len -= i;
			m_adj(q->tqe_m, i);
			break;
		}

		nq = LIST_NEXT(q, tqe_q);
		LIST_REMOVE(q, tqe_q);
		m_freem(q->tqe_m);
		uma_zfree(tcp_reass_zone, q);
		tp->t_segqlen--;
		q = nq;
	}

	/* Insert the new segment queue entry into place. */
	te->tqe_m = m;
	te->tqe_th = th;
	te->tqe_len = *tlenp;

	if (p == NULL) {
		LIST_INSERT_HEAD(&tp->t_segq, te, tqe_q);
	} else {
		KASSERT(te != &tqs, ("%s: temporary stack based entry not "
		    "first element in queue", __func__));
		LIST_INSERT_AFTER(p, te, tqe_q);
	}

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (!TCPS_HAVEESTABLISHED(tp->t_state))
		return (0);
	q = LIST_FIRST(&tp->t_segq);
	if (!q || q->tqe_th->th_seq != tp->rcv_nxt)
		return (0);
	SOCKBUF_LOCK(&so->so_rcv);
	do {
		tp->rcv_nxt += q->tqe_len;
		flags = q->tqe_th->th_flags & TH_FIN;
		nq = LIST_NEXT(q, tqe_q);
		LIST_REMOVE(q, tqe_q);
		if (so->so_rcv.sb_state & SBS_CANTRCVMORE)
			m_freem(q->tqe_m);
		else
			sbappendstream_locked(&so->so_rcv, q->tqe_m, 0);
		if (q != &tqs)
			uma_zfree(tcp_reass_zone, q);
		tp->t_segqlen--;
		q = nq;
	} while (q && q->tqe_th->th_seq == tp->rcv_nxt);
	sorwakeup_locked(so);
	return (flags);
}
#endif
