/*
 *  Copyright (c) 2018, Sam Kumar <samkumar@cs.berkeley.edu>
 *  Copyright (c) 2018, University of California, Berkeley
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * samkumar: I created this file to store many of the constants shared by the
 * various files in the FreeBSD protocol logic. The original variables were
 * often virtualized ("V_"-prefixed) variables. I've changed the definitions to
 * be enumerations rather than globals, to save some memory. These variables
 * often serve to enable, disable, or configure certain TCP-related features.
 */

#ifndef TCPLP_TCP_CONST_H_
#define TCPLP_TCP_CONST_H_

#include "../tcplp.h"

#include "tcp_var.h"
#include "tcp_timer.h"

/*
 * samkumar: these are TCPlp-specific constants that I added. They were not
 * present in the FreeBSD-derived code.
 */

#define FRAMES_PER_SEG 5
#define FRAMECAP_6LOWPAN (122 - 11 - 5)
#define IP6HDR_SIZE (2 + 1 + 1 + 16 + 16) // IPHC header (2) + Next header (1) + Hop count (1) + Dest. addr (16) + Src. addr (16)
#define MSS_6LOWPAN ((FRAMES_PER_SEG * FRAMECAP_6LOWPAN) - IP6HDR_SIZE - sizeof(struct tcphdr))

/*
 * samkumar: The remaining constants were present in the original FreeBSD code,
 * but I set their values.
 */

#define hz 1000 // number of ticks per second, assuming millisecond ticks

enum tcp_input_consts {
	tcp_keepcnt = TCPTV_KEEPCNT,
	tcp_fast_finwait2_recycle = 0,
	tcprexmtthresh = 3,
	V_drop_synfin = 0,
	V_tcp_do_ecn = 1,
	V_tcp_ecn_maxretries = 3,
	V_tcp_do_rfc3042 = 1,
	V_path_mtu_discovery = 0,
	V_tcp_delack_enabled = 1,
	V_tcp_initcwnd_segments = 0,
	V_tcp_do_rfc3390 = 0,
	V_tcp_abc_l_var = 2 // this is what was in the original tcp_input.c
};

enum tcp_subr_consts {
	tcp_delacktime = TCPTV_DELACK,
	tcp_keepinit = TCPTV_KEEP_INIT,
	tcp_keepidle = TCPTV_KEEP_IDLE,
	tcp_keepintvl = TCPTV_KEEPINTVL,
	tcp_maxpersistidle = TCPTV_KEEP_IDLE,
	tcp_msl = TCPTV_MSL,
	tcp_rexmit_slop = TCPTV_CPU_VAR,
	tcp_finwait2_timeout = TCPTV_FINWAIT2_TIMEOUT,

	V_tcp_do_rfc1323 = 1,
	V_tcp_v6mssdflt = MSS_6LOWPAN,
	/* Normally, this is used to prevent DoS attacks by sending tiny MSS values in the options. */
	V_tcp_minmss = TCP_MAXOLEN + 1, // Must have enough space for TCP options, and one more byte for data. Default is 216.
	V_tcp_do_sack = 1
};

enum tcp_timer_consts {
//	V_tcp_v6pmtud_blackhole_mss = FRAMECAP_6LOWPAN - sizeof(struct ip6_hdr) - sizeof(struct tcphdr), // Doesn't matter unless blackhole_detect is 1.
	tcp_rexmit_drop_options = 0, // drop options after a few retransmits
	always_keepalive = 1
};

enum tcp_fastopen_consts {
	V_tcp_fastopen_client_enable = 1,
	V_tcp_fastopen_server_enable = 1,
	V_tcp_fastopen_acceptany = 1,
	V_tcp_fastopen_numkeys = 4
};
#define TCP_RFC7413

/*
 * Force a time value to be in a certain range.
 */
#define	TCPT_RANGESET(tv, value, tvmin, tvmax) do { \
	(tv) = (value) + tcp_rexmit_slop; \
	if ((uint64_t)(tv) < (uint64_t)(tvmin)) \
		(tv) = (tvmin); \
	if ((uint64_t)(tv) > (uint64_t)(tvmax)) \
		(tv) = (tvmax); \
} while(0)

#endif
