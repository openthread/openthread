/*-
 * Copyright (c) 1982, 1986, 1993, 1994, 1995
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
 *	@(#)tcp_var.h	8.4 (Berkeley) 5/24/95
 * $FreeBSD$
 */

#ifndef TCPLP_NETINET_TCP_VAR_H_
#define TCPLP_NETINET_TCP_VAR_H_

/* For memmove(). */
#include <string.h>

/* Dependencies on OpenThread. */
#include <openthread/ip6.h>
#include <openthread/message.h>

/* Dependencies on TCPlp buffering libraries. */
#include "../lib/bitmap.h"
#include "../lib/cbuf.h"
#include "../lib/lbuf.h"

#include "cc.h"
#include "tcp.h"
#include "types.h"
#include "ip6.h"

/* Implement byte-order-specific functions using OpenThread. */
uint16_t tcplp_sys_hostswap16(uint16_t hostport);
uint32_t tcplp_sys_hostswap32(uint32_t hostport);

/*
 * It seems that these are defined as macros in Mac OS X, which is why we need
 * the #ifndef checks. Simply redefining them as functions would break the
 * build.
 */

#ifndef htons
static inline uint16_t htons(uint16_t hostport) {
	return tcplp_sys_hostswap16(hostport);
}
#endif

#ifndef ntohs
static inline uint16_t ntohs(uint16_t hostport) {
	return tcplp_sys_hostswap16(hostport);
}
#endif

#ifndef htonl
static inline uint32_t htonl(uint32_t hostport) {
	return tcplp_sys_hostswap32(hostport);
}
#endif

#ifndef ntohl
static inline uint32_t ntohl(uint32_t hostport) {
	return tcplp_sys_hostswap32(hostport);
}
#endif

// From ip_compat.h
#ifndef bcopy
#define	bcopy(a,b,c)	memmove(b,a,c)
#endif

#ifndef bzero
#define bzero(a,b)	memset(a,0x00,b)
#endif

struct sackblk {
	tcp_seq start;		/* start seq no. of sack block */
	tcp_seq end;		/* end seq no. */
};

struct sackhole {
	tcp_seq start;		/* start seq no. of hole */
	tcp_seq end;		/* end seq no. */
	tcp_seq rxmit;		/* next seq. no in hole to be retransmitted */

	/*
	 * samkumar: I'm using this instead of the TAILQ_ENTRY macro to avoid
	 * including sys/queue.h from this file. It's undesirable to include
	 * sys/queue.h from this file because this file is part of the external
	 * interface to TCPlp, and sys/queue.h pollutes the global namespace.
	 * The original code that uses TAILQ_ENTRY is in a comment below.
	 */
	struct {
		struct sackhole *tqe_next;	/* next element */
		struct sackhole **tqe_prev;	/* address of previous next element */
	} scblink;	/* scoreboard linkage */
	// TAILQ_ENTRY(sackhole) scblink;	/* scoreboard linkage */
};

struct sackhint {
	struct sackhole	*nexthole;
	int		sack_bytes_rexmit;
	tcp_seq		last_sack_ack;	/* Most recent/largest sacked ack */
};

struct tcptemp {
	uint8_t	tt_ipgen[40]; /* the size must be of max ip header, now IPv6 */
	struct	tcphdr tt_t;
};

#define tcp6cb		tcpcb  /* for KAME src sync over BSD*'s */

/* Abridged TCB for passive sockets. */
struct tcpcb_listen {
    int t_state;     /* Always CLOSED or LISTEN. */
    otInstance* instance;
	struct in6_addr laddr;
    uint16_t lport;
};

#define TCB_CANTRCVMORE 0x20
#define TCB_CANTSENDMORE 0x40

#define TCB_PASSIVE 0x80

#define tpcantrcvmore(tp) (tp)->miscflags |= TCB_CANTRCVMORE
#define tpcantsendmore(tp) (tp)->miscflags |= TCB_CANTSENDMORE
#define tpiscantrcv(tp) (((tp)->miscflags & TCB_CANTRCVMORE) != 0)
#define tpiscantsend(tp) (((tp)->miscflags & TCB_CANTSENDMORE) != 0)
#define tpmarktimeractive(tp, timer) (tp)->miscflags |= timer
#define tpistimeractive(tp, timer) (((tp)->miscflags & timer) != 0)
#define tpcleartimeractive(tp, timer) (tp)->miscflags &= ~timer
#define tpmarkpassiveopen(tp) (tp)->miscflags |= TCB_PASSIVE
#define tpispassiveopen(tp) (((tp)->miscflags & TCB_PASSIVE) != 0)

#define REASSBMP_SIZE(tp) BITS_TO_BYTES((tp)->recvbuf.size)

/* These estimates are used to allocate sackholes (see tcp_sack.c). */
#define AVG_SACKHOLES 2 // per TCB
#define MAX_SACKHOLES 5 // per TCB
#define SACKHOLE_POOL_SIZE MAX_SACKHOLES
#define SACKHOLE_BMP_SIZE BITS_TO_BYTES(SACKHOLE_POOL_SIZE)

struct tcplp_signals;

/*
 * Tcp control block, one per tcp; fields:
 * Organized for 16 byte cacheline efficiency.
 */
/*
 * samkumar: I added some fields for TCPlp to the beginning of this structure,
 * replaced the fields for buffering and timers, and deleted unused fields to
 * save memory. I've left some of the deleted fields in, as comments, for
 * clarity. At times, I reduced the configurability of the implementation
 * (e.g., by removing the ability to set keepalive parameters) in order to
 * reduce the size of this structure.
 */
struct tcpcb {
	/*
	 * samkumar: Extra fields that I added. TCPlp doesn't have a struct inpcb,
	 * so some of the fields I added represent data that would normally be
	 * stored in the inpcb.
	 */
	otInstance *instance;

	struct tcpcb_listen* accepted_from;

	struct lbufhead sendbuf;
	struct cbufhead recvbuf;
	uint8_t* reassbmp;
	int32_t reass_fin_index;

	struct in6_addr laddr; // local IP address
	struct in6_addr faddr; // foreign IP address

	uint16_t lport; // local port, network byte order
	uint16_t fport; // foreign port, network byte order
	uint8_t miscflags;

	/* samkumar: This field was there previously. */
	uint8_t	t_state;		/* state of this connection */

	/* Pool of SACK holes (on per-connection basis, for OpenThread port). */
	struct sackhole sackhole_pool[SACKHOLE_POOL_SIZE];
	uint8_t sackhole_bmp[SACKHOLE_BMP_SIZE];

#if 0
	struct	tsegqe_head t_segq;	/* segment reassembly queue */
	void	*t_pspare[2];		/* new reassembly queue */
	int	t_segqlen;		/* segment reassembly queue length */
#endif

	int32_t	t_dupacks;		/* consecutive dup acks recd */

#if 0
	struct tcp_timer *t_timers;	/* All the TCP timers in one struct */

	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
#endif

	uint16_t	tw_last_win; // samkumar: Taken from struct tcptw

	uint32_t	t_flags;

//	struct	vnet *t_vnet;		/* back pointer to parent vnet */

	tcp_seq	snd_una;		/* sent but unacknowledged */
	tcp_seq	snd_max;		/* highest sequence number sent;
					 * used to recognize retransmits
					 */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	snd_up;			/* send urgent pointer */

	tcp_seq	snd_wl1;		/* window update seg seq number */
	tcp_seq	snd_wl2;		/* window update seg ack number */
	tcp_seq	iss;			/* initial send sequence number */
	tcp_seq	irs;			/* initial receive sequence number */

	tcp_seq	rcv_nxt;		/* receive next */
	tcp_seq	rcv_adv;		/* advertised window */
	tcp_seq	rcv_up;			/* receive urgent pointer */
	uint64_t	rcv_wnd;		/* receive window */

	uint64_t	snd_wnd;		/* send window */
	uint64_t	snd_cwnd;		/* congestion-controlled window */
//	uint64_t	snd_spare1;		/* unused */
	uint64_t	snd_ssthresh;		/* snd_cwnd size threshold for
					 * for slow start exponential to
					 * linear switch
					 */
//	uint64_t	snd_spare2;		/* unused */
	tcp_seq	snd_recover;		/* for use in NewReno Fast Recovery */

	uint32_t	t_maxopd;		/* mss plus options */

	uint32_t	t_rcvtime;		/* inactivity time */
	uint32_t	t_starttime;		/* time connection was established */
	uint32_t	t_rtttime;		/* RTT measurement start time */
	tcp_seq	t_rtseq;		/* sequence number being timed */

//	uint32_t	t_bw_spare1;		/* unused */
//	tcp_seq	t_bw_spare2;		/* unused */

	int32_t	t_rxtcur;		/* current retransmit value (ticks) */
	uint32_t	t_maxseg;		/* maximum segment size */
	int32_t	t_srtt;			/* smoothed round-trip time */
	int32_t	t_rttvar;		/* variance in round-trip time */

	int32_t	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	uint32_t	t_rttmin;		/* minimum rtt allowed */
	uint32_t	t_rttbest;		/* best rtt we've seen */

	int32_t	t_softerror;		/* possible error not yet reported */

	uint64_t	t_rttupdated;		/* number of times rtt sampled */
	uint64_t	max_sndwnd;		/* largest window peer has offered */
/* out-of-band data */
//	char	t_oobflags;		/* have some */
//	char	t_iobc;			/* input character */

	tcp_seq	last_ack_sent;
/* experimental */
	tcp_seq	snd_recover_prev;	/* snd_recover prior to retransmit */
	uint64_t	snd_cwnd_prev;		/* cwnd prior to retransmit */
	uint64_t	snd_ssthresh_prev;	/* ssthresh prior to retransmit */
//	int	t_sndzerowin;		/* zero-window updates sent */
	uint32_t	t_badrxtwin;		/* window for retransmit recovery */
	uint8_t	snd_limited;		/* segments limited transmitted */

/* RFC 1323 variables */
/*
 * samkumar: Moved "RFC 1323 variables" after "experimental" to reduce
 * compiler-inserted padding.
 */
	uint8_t	snd_scale;		/* window scaling for send window */
	uint8_t	rcv_scale;		/* window scaling for recv window */
	uint8_t	request_r_scale;	/* pending window scaling */
	u_int32_t  ts_recent;		/* timestamp echo data */
	uint32_t	ts_recent_age;		/* when last updated */
	u_int32_t  ts_offset;		/* our timestamp offset */

/* SACK related state */
	int32_t	snd_numholes;		/* number of holes seen by sender */
	/*
	 * samkumar: I replaced the TAILQ_HEAD macro invocation (commented out
	 * below) with the code it stands for, to avoid having to #include
	 * sys/queue.h in this file. See the comment above in struct sackhole for
	 * more info.
	 */
	struct sackhole_head {
		struct sackhole *tqh_first;	/* first element */
		struct sackhole **tqh_last;	/* addr of last next element */
	} snd_holes;
	// TAILQ_HEAD(sackhole_head, sackhole) snd_holes;
					/* SACK scoreboard (sorted) */
	tcp_seq	snd_fack;		/* last seq number(+1) sack'd by rcv'r*/
	int32_t	rcv_numsacks;		/* # distinct sack blks present */
	struct sackblk sackblks[MAX_SACK_BLKS]; /* seq nos. of sack blocks */
	tcp_seq sack_newdata;		/* New data xmitted in this recovery
					   episode starts at this seq number */
	struct sackhint	sackhint;	/* SACK scoreboard hint */

	int32_t	t_rttlow;		/* smallest observed RTT */
#if 0
	u_int32_t	rfbuf_ts;	/* recv buffer autoscaling timestamp */
	int	rfbuf_cnt;		/* recv buffer autoscaling byte count */
	struct toedev	*tod;		/* toedev handling this connection */
#endif
//	int	t_sndrexmitpack;	/* retransmit packets sent */
//	int	t_rcvoopack;		/* out-of-order packets received */
//	void	*t_toe;			/* TOE pcb pointer */
	int32_t	t_bytes_acked;		/* # bytes acked during current RTT */
//	struct cc_algo	*cc_algo;	/* congestion control algorithm */
	struct cc_var	ccv[1];		/* congestion control specific vars */
#if 0
	struct osd	*osd;		/* storage for Khelp module data */
#endif
#if 0 // Just use the default values for the KEEP constants (see tcp_timer.h)
	uint32_t	t_keepinit;		/* time to establish connection */
	uint32_t	t_keepidle;		/* time before keepalive probes begin */
	uint32_t	t_keepintvl;		/* interval between keepalives */
	uint32_t	t_keepcnt;		/* number of keepalives before close */
#endif
#if 0 // Don't support TCP Segment Offloading
	uint32_t	t_tsomax;		/* TSO total burst length limit in bytes */
	uint32_t	t_tsomaxsegcount;	/* TSO maximum segment count */
	uint32_t	t_tsomaxsegsize;	/* TSO maximum segment size in bytes */
#endif
//	uint32_t	t_pmtud_saved_maxopd;	/* pre-blackhole MSS */
	uint32_t	t_flags2;		/* More tcpcb flags storage */

//	uint32_t t_ispare[8];		/* 5 UTO, 3 TBD */
//	void	*t_pspare2[4];		/* 1 TCP_SIGNATURE, 3 TBD */
#if 0
#if defined(_KERNEL) && defined(TCPPCAP)
	struct mbufq t_inpkts;		/* List of saved input packets. */
	struct mbufq t_outpkts;		/* List of saved output packets. */
#ifdef _LP64
	uint64_t _pad[0];		/* all used! */
#else
	uint64_t _pad[2];		/* 2 are available */
#endif /* _LP64 */
#else
	uint64_t _pad[6];
#endif /* defined(_KERNEL) && defined(TCPPCAP) */
#endif
};

/* Defined in tcp_subr.c. */
void initialize_tcb(struct tcpcb* tp);

/* Copied from the "dead" portions below. */

void	 tcp_init(void);
void	 tcp_state_change(struct tcpcb *, int);
tcp_seq tcp_new_isn(struct tcpcb *);
struct tcpcb *tcp_close(struct tcpcb *);
struct tcpcb *tcp_drop(struct tcpcb *, int);
void
tcp_respond(struct tcpcb *tp, otInstance* instance, struct ip6_hdr* ip6gen, struct tcphdr *thgen,
    tcp_seq ack, tcp_seq seq, int flags);
void	 tcp_setpersist(struct tcpcb *);
void	cc_cong_signal(struct tcpcb *tp, struct tcphdr *th, uint32_t type);

/* Added, since there is no header file for tcp_usrreq.c. */
int tcp6_usr_connect(struct tcpcb* tp, struct sockaddr_in6* sinp6);
int tcp_usr_send(struct tcpcb* tp, int moretocome, struct otLinkedBuffer* data, size_t extendby);
int tcp_usr_rcvd(struct tcpcb* tp);
int tcp_usr_shutdown(struct tcpcb* tp);
void tcp_usr_abort(struct tcpcb* tp);

/*
 * Flags and utility macros for the t_flags field.
 */
#define	TF_ACKNOW	0x000001	/* ack peer immediately */
#define	TF_DELACK	0x000002	/* ack, but try to delay it */
#define	TF_NODELAY	0x000004	/* don't delay packets to coalesce */
#define	TF_NOOPT	0x000008	/* don't use tcp options */
#define	TF_SENTFIN	0x000010	/* have sent FIN */
#define	TF_REQ_SCALE	0x000020	/* have/will request window scaling */
#define	TF_RCVD_SCALE	0x000040	/* other side has requested scaling */
#define	TF_REQ_TSTMP	0x000080	/* have/will request timestamps */
#define	TF_RCVD_TSTMP	0x000100	/* a timestamp was received in SYN */
#define	TF_SACK_PERMIT	0x000200	/* other side said I could SACK */
#define	TF_NEEDSYN	0x000400	/* send SYN (implicit state) */
#define	TF_NEEDFIN	0x000800	/* send FIN (implicit state) */
#define	TF_NOPUSH	0x001000	/* don't push */
#define	TF_PREVVALID	0x002000	/* saved values for bad rxmit valid */
#define	TF_MORETOCOME	0x010000	/* More data to be appended to sock */
#define	TF_LQ_OVERFLOW	0x020000	/* listen queue overflow */
#define	TF_LASTIDLE	0x040000	/* connection was previously idle */
#define	TF_RXWIN0SENT	0x080000	/* sent a receiver win 0 in response */
#define	TF_FASTRECOVERY	0x100000	/* in NewReno Fast Recovery */
#define	TF_WASFRECOVERY	0x200000	/* was in NewReno Fast Recovery */
#define	TF_SIGNATURE	0x400000	/* require MD5 digests (RFC2385) */
#define	TF_FORCEDATA	0x800000	/* force out a byte */
#define	TF_TSO		0x1000000	/* TSO enabled on this connection */
#define	TF_TOE		0x2000000	/* this connection is offloaded */
#define	TF_ECN_PERMIT	0x4000000	/* connection ECN-ready */
#define	TF_ECN_SND_CWR	0x8000000	/* ECN CWR in queue */
#define	TF_ECN_SND_ECE	0x10000000	/* ECN ECE in queue */
#define	TF_CONGRECOVERY	0x20000000	/* congestion recovery mode */
#define	TF_WASCRECOVERY	0x40000000	/* was in congestion recovery */

#define	IN_FASTRECOVERY(t_flags)	(t_flags & TF_FASTRECOVERY)
#define	ENTER_FASTRECOVERY(t_flags)	t_flags |= TF_FASTRECOVERY
#define	EXIT_FASTRECOVERY(t_flags)	t_flags &= ~TF_FASTRECOVERY

#define	IN_CONGRECOVERY(t_flags)	(t_flags & TF_CONGRECOVERY)
#define	ENTER_CONGRECOVERY(t_flags)	t_flags |= TF_CONGRECOVERY
#define	EXIT_CONGRECOVERY(t_flags)	t_flags &= ~TF_CONGRECOVERY

#define	IN_RECOVERY(t_flags) (t_flags & (TF_CONGRECOVERY | TF_FASTRECOVERY))
#define	ENTER_RECOVERY(t_flags) t_flags |= (TF_CONGRECOVERY | TF_FASTRECOVERY)
#define	EXIT_RECOVERY(t_flags) t_flags &= ~(TF_CONGRECOVERY | TF_FASTRECOVERY)

#define	BYTES_THIS_ACK(tp, th)	(th->th_ack - tp->snd_una)

/*
 * Flags for the t_oobflags field.
 */
#define	TCPOOB_HAVEDATA	0x01
#define	TCPOOB_HADDATA	0x02

#ifdef TCP_SIGNATURE
/*
 * Defines which are needed by the xform_tcp module and tcp_[in|out]put
 * for SADB verification and lookup.
 */
#define	TCP_SIGLEN	16	/* length of computed digest in bytes */
#define	TCP_KEYLEN_MIN	1	/* minimum length of TCP-MD5 key */
#define	TCP_KEYLEN_MAX	80	/* maximum length of TCP-MD5 key */
/*
 * Only a single SA per host may be specified at this time. An SPI is
 * needed in order for the KEY_ALLOCSA() lookup to work.
 */
#define	TCP_SIG_SPI	0x1000
#endif /* TCP_SIGNATURE */

/*
 * Flags for PLPMTU handling, t_flags2
 */
#define	TF2_PLPMTU_BLACKHOLE	0x00000001 /* Possible PLPMTUD Black Hole. */
#define	TF2_PLPMTU_PMTUD	0x00000002 /* Allowed to attempt PLPMTUD. */
#define	TF2_PLPMTU_MAXSEGSNT	0x00000004 /* Last seg sent was full seg. */

/*
 * Structure to hold TCP options that are only used during segment
 * processing (in tcp_input), but not held in the tcpcb.
 * It's basically used to reduce the number of parameters
 * to tcp_dooptions and tcp_addoptions.
 * The binary order of the to_flags is relevant for packing of the
 * options in tcp_addoptions.
 */
struct tcpopt {
	u_int64_t	to_flags;	/* which options are present */
#define	TOF_MSS		0x0001		/* maximum segment size */
#define	TOF_SCALE	0x0002		/* window scaling */
#define	TOF_SACKPERM	0x0004		/* SACK permitted */
#define	TOF_TS		0x0010		/* timestamp */
#define	TOF_SIGNATURE	0x0040		/* TCP-MD5 signature option (RFC2385) */
#define	TOF_SACK	0x0080		/* Peer sent SACK option */
#define	TOF_MAXOPT	0x0100
	u_int32_t	to_tsval;	/* new timestamp */
	u_int32_t	to_tsecr;	/* reflected timestamp */
	uint8_t		*to_sacks;	/* pointer to the first SACK blocks */
	uint8_t		*to_signature;	/* pointer to the TCP-MD5 signature */
	u_int16_t	to_mss;		/* maximum segment size */
	u_int8_t	to_wscale;	/* window scaling */
	u_int8_t	to_nsacks;	/* number of SACK blocks */
	u_int32_t	to_spare;	/* UTO */
};

/*
 * Flags for tcp_dooptions.
 */
#define	TO_SYN		0x01		/* parse SYN-only options */

struct hc_metrics_lite {	/* must stay in sync with hc_metrics */
	uint64_t	rmx_mtu;	/* MTU for this path */
	uint64_t	rmx_ssthresh;	/* outbound gateway buffer limit */
	uint64_t	rmx_rtt;	/* estimated round trip time */
	uint64_t	rmx_rttvar;	/* estimated rtt variance */
	uint64_t	rmx_bandwidth;	/* estimated bandwidth */
	uint64_t	rmx_cwnd;	/* congestion window */
	uint64_t	rmx_sendpipe;   /* outbound delay-bandwidth product */
	uint64_t	rmx_recvpipe;   /* inbound delay-bandwidth product */
};

/*
 * Used by tcp_maxmtu() to communicate interface specific features
 * and limits at the time of connection setup.
 */
struct tcp_ifcap {
	int	ifcap;
	uint32_t	tsomax;
	uint32_t	tsomaxsegcount;
	uint32_t	tsomaxsegsize;
};

void	 tcp_mss(struct tcpcb *, int);
void	 tcp_mss_update(struct tcpcb *, int, int, struct hc_metrics_lite *,
	    struct tcp_ifcap *);

/*
 * The smoothed round-trip time and estimated variance
 * are stored as fixed point numbers scaled by the values below.
 * For convenience, these scales are also used in smoothing the average
 * (smoothed = (1/scale)sample + ((scale-1)/scale)smoothed).
 * With these scales, srtt has 3 bits to the right of the binary point,
 * and thus an "ALPHA" of 0.875.  rttvar has 2 bits to the right of the
 * binary point, and is smoothed with an ALPHA of 0.75.
 */
#define	TCP_RTT_SCALE		32	/* multiplier for srtt; 3 bits frac. */
#define	TCP_RTT_SHIFT		5	/* shift for srtt; 3 bits frac. */
#define	TCP_RTTVAR_SCALE	16	/* multiplier for rttvar; 2 bits */
#define	TCP_RTTVAR_SHIFT	4	/* shift for rttvar; 2 bits */
#define	TCP_DELTA_SHIFT		2	/* see tcp_input.c */

/* My definition of the max macro */
#define max(x, y) ((x) > (y) ? (x) : (y))

/*
 * The initial retransmission should happen at rtt + 4 * rttvar.
 * Because of the way we do the smoothing, srtt and rttvar
 * will each average +1/2 tick of bias.  When we compute
 * the retransmit timer, we want 1/2 tick of rounding and
 * 1 extra tick because of +-1/2 tick uncertainty in the
 * firing of the timer.  The bias will give us exactly the
 * 1.5 tick we need.  But, because the bias is
 * statistical, we have to test that we don't drop below
 * the minimum feasible timer (which is 2 ticks).
 * This version of the macro adapted from a paper by Lawrence
 * Brakmo and Larry Peterson which outlines a problem caused
 * by insufficient precision in the original implementation,
 * which results in inappropriately large RTO values for very
 * fast networks.
 */
#define	TCP_REXMTVAL(tp) \
	max((tp)->t_rttmin, (((tp)->t_srtt >> (TCP_RTT_SHIFT - TCP_DELTA_SHIFT))  \
	  + (tp)->t_rttvar) >> TCP_DELTA_SHIFT)

/* Copied from below. */
static inline void
tcp_fields_to_host(struct tcphdr *th)
{

	th->th_seq = ntohl(th->th_seq);
	th->th_ack = ntohl(th->th_ack);
	th->th_win = ntohs(th->th_win);
	th->th_urp = ntohs(th->th_urp);
}

void	 tcp_twstart(struct tcpcb*);
void	 tcp_twclose(struct tcpcb*, int);
int	 tcp_twcheck(struct tcpcb*, struct tcphdr *, int);
void tcp_dropwithreset(struct ip6_hdr* ip6, struct tcphdr *th, struct tcpcb *tp, otInstance* instance,
    int tlen, int rstreason);
int tcp_input(struct ip6_hdr* ip6, struct tcphdr* th, otMessage* msg, struct tcpcb* tp, struct tcpcb_listen* tpl,
          struct tcplp_signals* sig);
int	 tcp_output(struct tcpcb *);
void tcpip_maketemplate(struct tcpcb *, struct tcptemp*);
void	 tcpip_fillheaders(struct tcpcb *, otMessageInfo *, void *);
uint64_t	 tcp_maxmtu6(struct tcpcb*, struct tcp_ifcap *);
int	 tcp_addoptions(struct tcpopt *, uint8_t *);
int	 tcp_mssopt(struct tcpcb*);
int	 tcp_reass(struct tcpcb *, struct tcphdr *, int *, otMessage *, off_t, struct tcplp_signals*);
void tcp_sack_init(struct tcpcb *); // Sam: new function that I added
void	 tcp_sack_doack(struct tcpcb *, struct tcpopt *, tcp_seq);
void	 tcp_update_sack_list(struct tcpcb *tp, tcp_seq rcv_laststart, tcp_seq rcv_lastend);
void	 tcp_clean_sackreport(struct tcpcb *tp);
void	 tcp_sack_adjust(struct tcpcb *tp);
struct sackhole *tcp_sack_output(struct tcpcb *tp, int *sack_bytes_rexmt);
void	 tcp_sack_partialack(struct tcpcb *, struct tcphdr *);
void	 tcp_free_sackholes(struct tcpcb *tp);

#define	tcps_rcvmemdrop	tcps_rcvreassfull	/* compat */0

/*
 * Identifiers for TCP sysctl nodes
 */
#define	TCPCTL_DO_RFC1323	1	/* use RFC-1323 extensions */
#define	TCPCTL_MSSDFLT		3	/* MSS default */
#define TCPCTL_STATS		4	/* statistics (read-only) */
#define	TCPCTL_RTTDFLT		5	/* default RTT estimate */
#define	TCPCTL_KEEPIDLE		6	/* keepalive idle timer */
#define	TCPCTL_KEEPINTVL	7	/* interval to send keepalives */
#define	TCPCTL_SENDSPACE	8	/* send buffer space */
#define	TCPCTL_RECVSPACE	9	/* receive buffer space */
#define	TCPCTL_KEEPINIT		10	/* timeout for establishing syn */
#define	TCPCTL_PCBLIST		11	/* list of all outstanding PCBs */
#define	TCPCTL_DELACKTIME	12	/* time before sending delayed ACK */
#define	TCPCTL_V6MSSDFLT	13	/* MSS default for IPv6 */
#define	TCPCTL_SACK		14	/* Selective Acknowledgement,rfc 2018 */
#define	TCPCTL_DROP		15	/* drop tcp connection */

#endif /* _NETINET_TCP_VAR_H_ */
