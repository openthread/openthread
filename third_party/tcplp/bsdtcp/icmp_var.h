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
 *	@(#)icmp_var.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD$
 */

/*
 * samkumar: Removed some #if guards and several definitions/declarations that
 * weren't necessary (and often introduced additional dependencies).
 */

#ifndef TCPLP_NETINET_ICMP_VAR_H_
#define TCPLP_NETINET_ICMP_VAR_H_
/*
 * Identifiers for ICMP sysctl nodes
 */
#define	ICMPCTL_MASKREPL	1	/* allow replies to netmask requests */
#define	ICMPCTL_STATS		2	/* statistics (read-only) */
#define ICMPCTL_ICMPLIM		3

#define BANDLIM_UNLIMITED -1
#define BANDLIM_ICMP_UNREACH 0
#define BANDLIM_ICMP_ECHO 1
#define BANDLIM_ICMP_TSTAMP 2
#define BANDLIM_RST_CLOSEDPORT 3 /* No connection, and no listeners */
#define BANDLIM_RST_OPENPORT 4   /* No connection, listener */
#define BANDLIM_ICMP6_UNREACH 5
#define BANDLIM_SCTP_OOTB 6
#define BANDLIM_MAX 6

#endif
