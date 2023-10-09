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
 *	@(#)tcp.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD$
 */

/*
 * samkumar: In one instance (in struct tcphdr), where there are Big Endian
 * and Little Endian alternatives for ordering two fields, I hardcoded the
 * Little Endian alternative. If ever we port this to a Big Endian platform,
 * we should look at that very closely, and consider rewriting it.
 */

#ifndef TCPLP_NETINET_TCP_H_
#define TCPLP_NETINET_TCP_H_

#include <stdint.h>
#include <stdio.h>

#define KASSERT(COND, MSG) if (!(COND)) tcplp_sys_panic MSG

typedef	uint32_t tcp_seq;

#define tcp6_seq	tcp_seq	/* for KAME src sync over BSD*'s */
#define tcp6hdr		tcphdr	/* for KAME src sync over BSD*'s */

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
	uint16_t	th_sport;		/* source port */
	uint16_t	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */

	/*
	 * samkumar: The original FreeBSD code used bit fields for the offset and
	 * unused bits, within this byte. I've rewritten it to avoid the use of
	 * bit fields, so that the code is more portable. The original code, which
	 * defined the order of bit fields based on the platform's endianness, is
	 * included below, commented out using "#if 0".
	 */
	uint8_t th_off_x2; /* data offset and unused bits */
#define	TH_OFF_SHIFT	4

#if 0
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t	th_off:4,		/* data offset */
		th_x2:4;		/* (unused) */
#endif
#endif

	uint8_t	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
#define	TH_ECE	0x40
#define	TH_CWR	0x80
#define	TH_FLAGS	(TH_FIN|TH_SYN|TH_RST|TH_PUSH|TH_ACK|TH_URG|TH_ECE|TH_CWR)
#define	PRINT_TH_FLAGS	"\20\1FIN\2SYN\3RST\4PUSH\5ACK\6URG\7ECE\10CWR"

	uint16_t	th_win;			/* window */
	uint16_t	th_sum;			/* checksum */
	uint16_t	th_urp;			/* urgent pointer */
};

#define	TCPOPT_EOL		0
#define	   TCPOLEN_EOL			1
#define	TCPOPT_PAD		0		/* padding after EOL */
#define	   TCPOLEN_PAD			1
#define	TCPOPT_NOP		1
#define	   TCPOLEN_NOP			1
#define	TCPOPT_MAXSEG		2
#define    TCPOLEN_MAXSEG		4
#define TCPOPT_WINDOW		3
#define    TCPOLEN_WINDOW		3
#define TCPOPT_SACK_PERMITTED	4
#define    TCPOLEN_SACK_PERMITTED	2
#define TCPOPT_SACK		5
#define	   TCPOLEN_SACKHDR		2
#define    TCPOLEN_SACK			8	/* 2*sizeof(tcp_seq) */
#define TCPOPT_TIMESTAMP	8
#define    TCPOLEN_TIMESTAMP		10
#define    TCPOLEN_TSTAMP_APPA		(TCPOLEN_TIMESTAMP+2) /* appendix A */
#define	TCPOPT_SIGNATURE	19		/* Keyed MD5: RFC 2385 */
#define	   TCPOLEN_SIGNATURE		18
#define	TCPOPT_FAST_OPEN	34
#define	   TCPOLEN_FAST_OPEN_EMPTY	2

/* Miscellaneous constants */
#define	MAX_SACK_BLKS	6	/* Max # SACK blocks stored at receiver side */
#define	TCP_MAX_SACK	4	/* MAX # SACKs sent in any segment */


/*
 * The default maximum segment size (MSS) to be used for new TCP connections
 * when path MTU discovery is not enabled.
 *
 * RFC879 derives the default MSS from the largest datagram size hosts are
 * minimally required to handle directly or through IP reassembly minus the
 * size of the IP and TCP header.  With IPv6 the minimum MTU is specified
 * in RFC2460.
 *
 * For IPv4 the MSS is 576 - sizeof(struct tcpiphdr)
 * For IPv6 the MSS is IPV6_MMTU - sizeof(struct ip6_hdr) - sizeof(struct tcphdr)
 *
 * We use explicit numerical definition here to avoid header pollution.
 */
#define	TCP_MSS		536
#define	TCP6_MSS	1220

/*
 * Limit the lowest MSS we accept for path MTU discovery and the TCP SYN MSS
 * option.  Allowing low values of MSS can consume significant resources and
 * be used to mount a resource exhaustion attack.
 * Connections requesting lower MSS values will be rounded up to this value
 * and the IP_DF flag will be cleared to allow fragmentation along the path.
 *
 * See tcp_subr.c tcp_minmss SYSCTL declaration for more comments.  Setting
 * it to "0" disables the minmss check.
 *
 * The default value is fine for TCP across the Internet's smallest official
 * link MTU (256 bytes for AX.25 packet radio).  However, a connection is very
 * unlikely to come across such low MTU interfaces these days (anno domini 2003).
 */
#define	TCP_MINMSS 216

#define	TCP_MAXWIN	65535	/* largest value for (unscaled) window */
#define	TTCP_CLIENT_SND_WND	4096	/* dflt send window for T/TCP client */

#define TCP_MAX_WINSHIFT	14	/* maximum window shift */

#define TCP_MAXBURST		4	/* maximum segments in a burst */

#define TCP_MAXHLEN	(0xf<<2)	/* max length of header in bytes */
#define TCP_MAXOLEN	(TCP_MAXHLEN - sizeof(struct tcphdr))
					/* max space left for options */
#define TCP_FASTOPEN_MIN_COOKIE_LEN	4	/* Per RFC7413 */
#define TCP_FASTOPEN_MAX_COOKIE_LEN	16	/* Per RFC7413 */
#define TCP_FASTOPEN_PSK_LEN		16	/* Same as TCP_FASTOPEN_KEY_LEN */

/*
 * User-settable options (used with setsockopt).  These are discrete
 * values and are not masked together.  Some values appear to be
 * bitmasks for historical reasons.
 */
#define	TCP_NODELAY	1	/* don't delay send to coalesce packets */
#define	TCP_MAXSEG	2	/* set maximum segment size */
#define TCP_NOPUSH	4	/* don't push last block of write */
#define TCP_NOOPT	8	/* don't use TCP options */
#define TCP_MD5SIG	16	/* use MD5 digests (RFC2385) */
#define	TCP_INFO	32	/* retrieve tcp_info structure */
#define	TCP_CONGESTION	64	/* get/set congestion control algorithm */
#define	TCP_KEEPINIT	128	/* N, time to establish connection */
#define	TCP_KEEPIDLE	256	/* L,N,X start keeplives after this period */
#define	TCP_KEEPINTVL	512	/* L,N interval between keepalives */
#define	TCP_KEEPCNT	1024	/* L,N number of keepalives before close */
#define	TCP_PCAP_OUT	2048	/* number of output packets to keep */
#define	TCP_PCAP_IN	4096	/* number of input packets to keep */

/* Start of reserved space for third-party user-settable options. */
#define	TCP_VENDOR	SO_VENDOR

#define	TCP_CA_NAME_MAX	16	/* max congestion control name length */

#define	TCPI_OPT_TIMESTAMPS	0x01
#define	TCPI_OPT_SACK		0x02
#define	TCPI_OPT_WSCALE		0x04
#define	TCPI_OPT_ECN		0x08
#define	TCPI_OPT_TOE		0x10

#endif /* !_NETINET_TCP_H_ */
