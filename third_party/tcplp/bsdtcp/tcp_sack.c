/*-
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
 *	The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)tcp_sack.c	8.12 (Berkeley) 5/24/95
 */

/*-
 *	@@(#)COPYRIGHT	1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed at the Information
 *	Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

/* samkumar: Removed a bunch of #include's and VNET declarations. */

#include <strings.h>
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "tcp_var.h"
#include "sys/queue.h"

enum tcp_sack_consts {
	V_tcp_sack_maxholes = MAX_SACKHOLES
};

/*
 * samkumar: Removed tcp_sack_globalmaxholes and tcp_sack_globalholes.
 * There used to be a counter, V_tcp_sack_globalholes, that kept track of the
 * total number of SACK holes allocated across all TCP connections.
 */

/*
 * samkumar: I added these three functions. The first, tcp_sack_init,
 * initializes a per-connection pool of SACK holes.
 *
 * The next two, sackhole_alloc and sackhole_free, allocate and deallocate SACK
 * holes from the pool. Previously, the FreeBSD code would allocate SACK holes
 * dynamically, for example, using the code
 * "hole = (struct sackhole *)uma_zalloc(V_sack_hole_zone, M_NOWAIT);".
 * TCPlp avoids dynamic memory allocation in the TCP implementation, so we
 * replace it with this per-connection pool.
 */

void
tcp_sack_init(struct tcpcb* tp)
{
	bmp_init(tp->sackhole_bmp, SACKHOLE_BMP_SIZE);
}

struct sackhole* sackhole_alloc(struct tcpcb* tp) {
    size_t freeindex = bmp_countset(tp->sackhole_bmp, SACKHOLE_BMP_SIZE, 0, SACKHOLE_BMP_SIZE);
    if (freeindex >= SACKHOLE_BMP_SIZE) {
    	return NULL; // all sackholes are allocated already!
    }
    bmp_setrange(tp->sackhole_bmp, freeindex, 1);
    return &tp->sackhole_pool[freeindex];
}

void sackhole_free(struct tcpcb* tp, struct sackhole* tofree) {
	size_t freeindex = (size_t) (tofree - &tp->sackhole_pool[0]);
	KASSERT(tofree == &tp->sackhole_pool[freeindex], ("sackhole pool unaligned"));
	bmp_clrrange(tp->sackhole_bmp, freeindex, 1);
}

/*
 * samkumar: Throughout the remaining functions, I have replaced allocation and
 * deallocation of SACK holes, which previously used uma_zalloc and uma_zfree,
 * with calls to sackhole_alloc and sackhole_free. I've also removed code for
 * locking, global stats collection, global SACK hole limits, and debugging
 * probes.
 */


/*
 * This function is called upon receipt of new valid data (while not in
 * header prediction mode), and it updates the ordered list of sacks.
 */
void
tcp_update_sack_list(struct tcpcb *tp, tcp_seq rcv_start, tcp_seq rcv_end)
{
	/*
	 * First reported block MUST be the most recent one.  Subsequent
	 * blocks SHOULD be in the order in which they arrived at the
	 * receiver.  These two conditions make the implementation fully
	 * compliant with RFC 2018.
	 */
	struct sackblk head_blk, saved_blks[MAX_SACK_BLKS];
	int num_head, num_saved, i;

	/* Check arguments. */
	KASSERT(SEQ_LT(rcv_start, rcv_end), ("rcv_start < rcv_end"));

	/* SACK block for the received segment. */
	head_blk.start = rcv_start;
	head_blk.end = rcv_end;

	/*
	 * Merge updated SACK blocks into head_blk, and save unchanged SACK
	 * blocks into saved_blks[].  num_saved will have the number of the
	 * saved SACK blocks.
	 */
	num_saved = 0;
	for (i = 0; i < tp->rcv_numsacks; i++) {
		tcp_seq start = tp->sackblks[i].start;
		tcp_seq end = tp->sackblks[i].end;
		if (SEQ_GEQ(start, end) || SEQ_LEQ(start, tp->rcv_nxt)) {
			/*
			 * Discard this SACK block.
			 */
		} else if (SEQ_LEQ(head_blk.start, end) &&
			   SEQ_GEQ(head_blk.end, start)) {
			/*
			 * Merge this SACK block into head_blk.  This SACK
			 * block itself will be discarded.
			 */
			if (SEQ_GT(head_blk.start, start))
				head_blk.start = start;
			if (SEQ_LT(head_blk.end, end))
				head_blk.end = end;
		} else {
			/*
			 * Save this SACK block.
			 */
			saved_blks[num_saved].start = start;
			saved_blks[num_saved].end = end;
			num_saved++;
		}
	}

	/*
	 * Update SACK list in tp->sackblks[].
	 */
	num_head = 0;
	if (SEQ_GT(head_blk.start, tp->rcv_nxt)) {
		/*
		 * The received data segment is an out-of-order segment.  Put
		 * head_blk at the top of SACK list.
		 */
		tp->sackblks[0] = head_blk;
		num_head = 1;
		/*
		 * If the number of saved SACK blocks exceeds its limit,
		 * discard the last SACK block.
		 */
		if (num_saved >= MAX_SACK_BLKS)
			num_saved--;
	}
	if (num_saved > 0) {
		/*
		 * Copy the saved SACK blocks back.
		 */
		bcopy(saved_blks, &tp->sackblks[num_head],
		      sizeof(struct sackblk) * num_saved);
	}

	/* Save the number of SACK blocks. */
	tp->rcv_numsacks = num_head + num_saved;
}

/*
 * Delete all receiver-side SACK information.
 */
void
tcp_clean_sackreport(struct tcpcb *tp)
{
	int i;

	tp->rcv_numsacks = 0;
	for (i = 0; i < MAX_SACK_BLKS; i++)
		tp->sackblks[i].start = tp->sackblks[i].end=0;
}

/*
 * Allocate struct sackhole.
 */
static struct sackhole *
tcp_sackhole_alloc(struct tcpcb *tp, tcp_seq start, tcp_seq end)
{
	struct sackhole *hole;

	/*
	 * samkumar: This if block also used to also return NULL if
	 * V_tcp_sack_globalholes >= V_tcp_sack_globalmaxholes
	 * but I removed that check since it doesn't make sense to enforce a global
	 * limit on SACK holes when we have a fixed-size pool (moreover, a separate
	 * pool per connection). The per-connection limit is sufficient.
	 */
	if (tp->snd_numholes >= V_tcp_sack_maxholes) {
		return NULL;
	}

	hole = sackhole_alloc(tp);
	if (hole == NULL)
		return NULL;

	hole->start = start;
	hole->end = end;
	hole->rxmit = start;

	tp->snd_numholes++;

	return hole;
}

/*
 * Free struct sackhole.
 */
static void
tcp_sackhole_free(struct tcpcb *tp, struct sackhole *hole)
{
	sackhole_free(tp, hole);

	tp->snd_numholes--;

	KASSERT(tp->snd_numholes >= 0, ("tp->snd_numholes >= 0"));
}

/*
 * Insert new SACK hole into scoreboard.
 */
static struct sackhole *
tcp_sackhole_insert(struct tcpcb *tp, tcp_seq start, tcp_seq end,
    struct sackhole *after)
{
	struct sackhole *hole;

	/* Allocate a new SACK hole. */
	hole = tcp_sackhole_alloc(tp, start, end);
	if (hole == NULL)
		return NULL;

	/* Insert the new SACK hole into scoreboard. */
	if (after != NULL)
		TAILQ_INSERT_AFTER(&tp->snd_holes, after, hole, scblink);
	else
		TAILQ_INSERT_TAIL(&tp->snd_holes, hole, scblink);

	/* Update SACK hint. */
	if (tp->sackhint.nexthole == NULL)
		tp->sackhint.nexthole = hole;

	return hole;
}

/*
 * Remove SACK hole from scoreboard.
 */
static void
tcp_sackhole_remove(struct tcpcb *tp, struct sackhole *hole)
{

	/* Update SACK hint. */
	if (tp->sackhint.nexthole == hole)
		tp->sackhint.nexthole = TAILQ_NEXT(hole, scblink);

	/* Remove this SACK hole. */
	TAILQ_REMOVE(&tp->snd_holes, hole, scblink);

	/* Free this SACK hole. */
	tcp_sackhole_free(tp, hole);
}

/*
 * Process cumulative ACK and the TCP SACK option to update the scoreboard.
 * tp->snd_holes is an ordered list of holes (oldest to newest, in terms of
 * the sequence space).
 */
void
tcp_sack_doack(struct tcpcb *tp, struct tcpopt *to, tcp_seq th_ack)
{
	struct sackhole *cur, *temp;
	struct sackblk sack, sack_blocks[TCP_MAX_SACK + 1], *sblkp;
	int i, j, num_sack_blks;

	num_sack_blks = 0;
	/*
	 * If SND.UNA will be advanced by SEG.ACK, and if SACK holes exist,
	 * treat [SND.UNA, SEG.ACK) as if it is a SACK block.
	 */
	if (SEQ_LT(tp->snd_una, th_ack) && !TAILQ_EMPTY(&tp->snd_holes)) {
		sack_blocks[num_sack_blks].start = tp->snd_una;
		sack_blocks[num_sack_blks++].end = th_ack;
	}
	/*
	 * Append received valid SACK blocks to sack_blocks[], but only if we
	 * received new blocks from the other side.
	 */
	if (to->to_flags & TOF_SACK) {
		for (i = 0; i < to->to_nsacks; i++) {
			bcopy((to->to_sacks + i * TCPOLEN_SACK),
			    &sack, sizeof(sack));
			sack.start = ntohl(sack.start);
			sack.end = ntohl(sack.end);
			if (SEQ_GT(sack.end, sack.start) &&
			    SEQ_GT(sack.start, tp->snd_una) &&
			    SEQ_GT(sack.start, th_ack) &&
			    SEQ_LT(sack.start, tp->snd_max) &&
			    SEQ_GT(sack.end, tp->snd_una) &&
			    SEQ_LEQ(sack.end, tp->snd_max))
				sack_blocks[num_sack_blks++] = sack;
		}
	}
	/*
	 * Return if SND.UNA is not advanced and no valid SACK block is
	 * received.
	 */
	if (num_sack_blks == 0)
		return;

	/*
	 * Sort the SACK blocks so we can update the scoreboard with just one
	 * pass. The overhead of sorting upto 4+1 elements is less than
	 * making upto 4+1 passes over the scoreboard.
	 */
	for (i = 0; i < num_sack_blks; i++) {
		for (j = i + 1; j < num_sack_blks; j++) {
			if (SEQ_GT(sack_blocks[i].end, sack_blocks[j].end)) {
				sack = sack_blocks[i];
				sack_blocks[i] = sack_blocks[j];
				sack_blocks[j] = sack;
			}
		}
	}
	if (TAILQ_EMPTY(&tp->snd_holes))
		/*
		 * Empty scoreboard. Need to initialize snd_fack (it may be
		 * uninitialized or have a bogus value). Scoreboard holes
		 * (from the sack blocks received) are created later below
		 * (in the logic that adds holes to the tail of the
		 * scoreboard).
		 */
		tp->snd_fack = SEQ_MAX(tp->snd_una, th_ack);
	/*
	 * In the while-loop below, incoming SACK blocks (sack_blocks[]) and
	 * SACK holes (snd_holes) are traversed from their tails with just
	 * one pass in order to reduce the number of compares especially when
	 * the bandwidth-delay product is large.
	 *
	 * Note: Typically, in the first RTT of SACK recovery, the highest
	 * three or four SACK blocks with the same ack number are received.
	 * In the second RTT, if retransmitted data segments are not lost,
	 * the highest three or four SACK blocks with ack number advancing
	 * are received.
	 */
	sblkp = &sack_blocks[num_sack_blks - 1];	/* Last SACK block */
	tp->sackhint.last_sack_ack = sblkp->end;
	if (SEQ_LT(tp->snd_fack, sblkp->start)) {
		/*
		 * The highest SACK block is beyond fack.  Append new SACK
		 * hole at the tail.  If the second or later highest SACK
		 * blocks are also beyond the current fack, they will be
		 * inserted by way of hole splitting in the while-loop below.
		 */
		temp = tcp_sackhole_insert(tp, tp->snd_fack,sblkp->start,NULL);
		if (temp != NULL) {
			tp->snd_fack = sblkp->end;
			/* Go to the previous sack block. */
			sblkp--;
		} else {
			/*
			 * We failed to add a new hole based on the current
			 * sack block.  Skip over all the sack blocks that
			 * fall completely to the right of snd_fack and
			 * proceed to trim the scoreboard based on the
			 * remaining sack blocks.  This also trims the
			 * scoreboard for th_ack (which is sack_blocks[0]).
			 */
			while (sblkp >= sack_blocks &&
			       SEQ_LT(tp->snd_fack, sblkp->start))
				sblkp--;
			if (sblkp >= sack_blocks &&
			    SEQ_LT(tp->snd_fack, sblkp->end))
				tp->snd_fack = sblkp->end;
		}
	} else if (SEQ_LT(tp->snd_fack, sblkp->end))
		/* fack is advanced. */
		tp->snd_fack = sblkp->end;
	/* We must have at least one SACK hole in scoreboard. */
	KASSERT(!TAILQ_EMPTY(&tp->snd_holes),
	    ("SACK scoreboard must not be empty"));
	cur = TAILQ_LAST(&tp->snd_holes, sackhole_head); /* Last SACK hole. */
	/*
	 * Since the incoming sack blocks are sorted, we can process them
	 * making one sweep of the scoreboard.
	 */
	while (sblkp >= sack_blocks  && cur != NULL) {
		if (SEQ_GEQ(sblkp->start, cur->end)) {
			/*
			 * SACKs data beyond the current hole.  Go to the
			 * previous sack block.
			 */
			sblkp--;
			continue;
		}
		if (SEQ_LEQ(sblkp->end, cur->start)) {
			/*
			 * SACKs data before the current hole.  Go to the
			 * previous hole.
			 */
			cur = TAILQ_PREV(cur, sackhole_head, scblink);
			continue;
		}
		tp->sackhint.sack_bytes_rexmit -= (cur->rxmit - cur->start);
		KASSERT(tp->sackhint.sack_bytes_rexmit >= 0,
		    ("sackhint bytes rtx >= 0"));
		if (SEQ_LEQ(sblkp->start, cur->start)) {
			/* Data acks at least the beginning of hole. */
			if (SEQ_GEQ(sblkp->end, cur->end)) {
				/* Acks entire hole, so delete hole. */
				temp = cur;
				cur = TAILQ_PREV(cur, sackhole_head, scblink);
				tcp_sackhole_remove(tp, temp);
				/*
				 * The sack block may ack all or part of the
				 * next hole too, so continue onto the next
				 * hole.
				 */
				continue;
			} else {
				/* Move start of hole forward. */
				cur->start = sblkp->end;
				cur->rxmit = SEQ_MAX(cur->rxmit, cur->start);
			}
		} else {
			/* Data acks at least the end of hole. */
			if (SEQ_GEQ(sblkp->end, cur->end)) {
				/* Move end of hole backward. */
				cur->end = sblkp->start;
				cur->rxmit = SEQ_MIN(cur->rxmit, cur->end);
			} else {
				/*
				 * ACKs some data in middle of a hole; need
				 * to split current hole
				 */
				temp = tcp_sackhole_insert(tp, sblkp->end,
				    cur->end, cur);
				if (temp != NULL) {
					if (SEQ_GT(cur->rxmit, temp->rxmit)) {
						temp->rxmit = cur->rxmit;
						tp->sackhint.sack_bytes_rexmit
						    += (temp->rxmit
						    - temp->start);
					}
					cur->end = sblkp->start;
					cur->rxmit = SEQ_MIN(cur->rxmit,
					    cur->end);
				}
			}
		}
		tp->sackhint.sack_bytes_rexmit += (cur->rxmit - cur->start);
		/*
		 * Testing sblkp->start against cur->start tells us whether
		 * we're done with the sack block or the sack hole.
		 * Accordingly, we advance one or the other.
		 */
		if (SEQ_LEQ(sblkp->start, cur->start))
			cur = TAILQ_PREV(cur, sackhole_head, scblink);
		else
			sblkp--;
	}
}

/*
 * Free all SACK holes to clear the scoreboard.
 */
void
tcp_free_sackholes(struct tcpcb *tp)
{
	struct sackhole *q;

	while ((q = TAILQ_FIRST(&tp->snd_holes)) != NULL)
		tcp_sackhole_remove(tp, q);
	tp->sackhint.sack_bytes_rexmit = 0;

	KASSERT(tp->snd_numholes == 0, ("tp->snd_numholes == 0"));
	KASSERT(tp->sackhint.nexthole == NULL,
		("tp->sackhint.nexthole == NULL"));
}

/*
 * Partial ack handling within a sack recovery episode.  Keeping this very
 * simple for now.  When a partial ack is received, force snd_cwnd to a value
 * that will allow the sender to transmit no more than 2 segments.  If
 * necessary, a better scheme can be adopted at a later point, but for now,
 * the goal is to prevent the sender from bursting a large amount of data in
 * the midst of sack recovery.
 */
void
tcp_sack_partialack(struct tcpcb *tp, struct tcphdr *th)
{
	int num_segs = 1;

	tcp_timer_activate(tp, TT_REXMT, 0);
	tp->t_rtttime = 0;
	/* Send one or 2 segments based on how much new data was acked. */
	if ((BYTES_THIS_ACK(tp, th) / tp->t_maxseg) >= 2)
		num_segs = 2;
	tp->snd_cwnd = (tp->sackhint.sack_bytes_rexmit +
	    (tp->snd_nxt - tp->sack_newdata) + num_segs * tp->t_maxseg);
	if (tp->snd_cwnd > tp->snd_ssthresh)
		tp->snd_cwnd = tp->snd_ssthresh;
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
}

/*
 * samkumar: Removed this function for now, but I left it in as a comment
 * (using #if 0) in case it is useful later for debugging.
 */
#if 0
/*
 * Debug version of tcp_sack_output() that walks the scoreboard.  Used for
 * now to sanity check the hint.
 */
static struct sackhole *
tcp_sack_output_debug(struct tcpcb *tp, int *sack_bytes_rexmt)
{
	struct sackhole *p;

	INP_WLOCK_ASSERT(tp->t_inpcb);
	*sack_bytes_rexmt = 0;
	TAILQ_FOREACH(p, &tp->snd_holes, scblink) {
		if (SEQ_LT(p->rxmit, p->end)) {
			if (SEQ_LT(p->rxmit, tp->snd_una)) {/* old SACK hole */
				continue;
			}
			*sack_bytes_rexmt += (p->rxmit - p->start);
			break;
		}
		*sack_bytes_rexmt += (p->rxmit - p->start);
	}
	return (p);
}
#endif

/*
 * Returns the next hole to retransmit and the number of retransmitted bytes
 * from the scoreboard.  We store both the next hole and the number of
 * retransmitted bytes as hints (and recompute these on the fly upon SACK/ACK
 * reception).  This avoids scoreboard traversals completely.
 *
 * The loop here will traverse *at most* one link.  Here's the argument.  For
 * the loop to traverse more than 1 link before finding the next hole to
 * retransmit, we would need to have at least 1 node following the current
 * hint with (rxmit == end).  But, for all holes following the current hint,
 * (start == rxmit), since we have not yet retransmitted from them.
 * Therefore, in order to traverse more 1 link in the loop below, we need to
 * have at least one node following the current hint with (start == rxmit ==
 * end).  But that can't happen, (start == end) means that all the data in
 * that hole has been sacked, in which case, the hole would have been removed
 * from the scoreboard.
 */
struct sackhole *
tcp_sack_output(struct tcpcb *tp, int *sack_bytes_rexmt)
{
	struct sackhole *hole = NULL;

	*sack_bytes_rexmt = tp->sackhint.sack_bytes_rexmit;
	hole = tp->sackhint.nexthole;
	if (hole == NULL || SEQ_LT(hole->rxmit, hole->end))
		goto out;
	while ((hole = TAILQ_NEXT(hole, scblink)) != NULL) {
		if (SEQ_LT(hole->rxmit, hole->end)) {
			tp->sackhint.nexthole = hole;
			break;
		}
	}
out:
	return (hole);
}

/*
 * After a timeout, the SACK list may be rebuilt.  This SACK information
 * should be used to avoid retransmitting SACKed data.  This function
 * traverses the SACK list to see if snd_nxt should be moved forward.
 */
void
tcp_sack_adjust(struct tcpcb *tp)
{
	struct sackhole *p, *cur = TAILQ_FIRST(&tp->snd_holes);

	if (cur == NULL)
		return; /* No holes */
	if (SEQ_GEQ(tp->snd_nxt, tp->snd_fack))
		return; /* We're already beyond any SACKed blocks */
	/*-
	 * Two cases for which we want to advance snd_nxt:
	 * i) snd_nxt lies between end of one hole and beginning of another
	 * ii) snd_nxt lies between end of last hole and snd_fack
	 */
	while ((p = TAILQ_NEXT(cur, scblink)) != NULL) {
		if (SEQ_LT(tp->snd_nxt, cur->end))
			return;
		if (SEQ_GEQ(tp->snd_nxt, p->start))
			cur = p;
		else {
			tp->snd_nxt = p->start;
			return;
		}
	}
	if (SEQ_LT(tp->snd_nxt, cur->end))
		return;
	tp->snd_nxt = tp->snd_fack;
}
