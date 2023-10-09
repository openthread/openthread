/*-
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.
 * Copyright (c) 2006-2007 Robert N. M. Watson
 * Copyright (c) 2010-2011 Juniper Networks, Inc.
 * All rights reserved.
 *
 * Portions of this software were developed by Robert N. M. Watson under
 * contract to Juniper Networks, Inc.
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
 *	From: @(#)tcp_usrreq.c	8.2 (Berkeley) 1/3/94
 */

#include <errno.h>
#include <string.h>
#include "../tcplp.h"
#include "../lib/cbuf.h"
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_var.h"
#include "tcp_timer.h"
#include "tcp_fastopen.h"
#include "ip6.h"

#include "tcp_const.h"

#include <openthread/tcp.h>

//static void	tcp_disconnect(struct tcpcb *);
static void	tcp_usrclosed(struct tcpcb *);

/*
 * samkumar: Removed tcp6_usr_bind, since checking if an address/port is free
 * needs to be done at the host system (along with other socket management
 * duties). TCPlp doesn't know what other sockets are in the system, or which
 * other addresses/ports are busy.
 */

/* samkumar: This is based on a function in in6_pcb.c. */
static int in6_pcbconnect(struct tcpcb* tp, struct sockaddr_in6* nam) {
	register struct sockaddr_in6 *sin6 = nam;
	tp->faddr = sin6->sin6_addr;
	tp->fport = sin6->sin6_port;
	return 0;
}

/*
 * Initiate connection to peer.
 * Create a template for use in transmissions on this connection.
 * Enter SYN_SENT state, and mark socket as connecting.
 * Start keep-alive timer, and seed output sequence space.
 * Send initial segment on connection.
 */
/*
 * samkumar: I removed locking, statistics, and inpcb management. The signature
 * used to be
 *
 * static int
 * tcp6_connect(struct tcpcb *tp, struct sockaddr *nam, struct thread *td)
 */
static int
tcp6_connect(struct tcpcb *tp, struct sockaddr_in6 *nam)
{
	int error;

	int sb_max = cbuf_free_space(&tp->recvbuf); // same as sendbuf

	/*
	 * samkumar: For autobind, the original BSD code assigned the port first
	 * (with logic that also looked at the address) and then the address. This
	 * was done by calling into other parts of the FreeBSD network stack,
	 * outside of the TCP stack. Here, we just use the tcplp_sys_autobind
	 * function to do all of that work.
	 */
	bool autobind_addr = IN6_IS_ADDR_UNSPECIFIED(&tp->laddr);
	bool autobind_port = (tp->lport == 0);
	if (autobind_addr || autobind_port) {
		otSockAddr foreign;
		otSockAddr local;

		memcpy(&foreign.mAddress, &nam->sin6_addr, sizeof(foreign.mAddress));
		foreign.mPort = ntohs(nam->sin6_port);

		if (!autobind_addr) {
			memcpy(&local.mAddress, &tp->laddr, sizeof(local.mAddress));
		}

		if (!autobind_port) {
			local.mPort = ntohs(tp->lport);
		}

		if (!tcplp_sys_autobind(tp->instance, &foreign, &local, autobind_addr, autobind_port)) {
			// Autobind failed
			error = EINVAL;
			goto out;
		}

		if (autobind_addr) {
			memcpy(&tp->laddr, &local.mAddress, sizeof(tp->laddr));
		}

		if (autobind_port) {
			tp->lport = htons(local.mPort);
		}
	}
	error = in6_pcbconnect(tp, nam);
	if (error != 0)
		goto out;

	/* Compute window scaling to request.  */
	while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
	    (TCP_MAXWIN << tp->request_r_scale) < sb_max)
		tp->request_r_scale++;

	tcp_state_change(tp, TCPS_SYN_SENT);
	tp->iss = tcp_new_isn(tp);
	tcp_sendseqinit(tp);

	return 0;

out:
	return error;
}

/*
 * samkumar: I removed locking, statistics, inpcb management, and debug probes.
 * I also remove codepaths that check for IPv6, since the address is assumed to
 * be IPv6. The signature used to be
 *
 * static int
 * tcp6_usr_connect(struct socket *so, struct sockaddr *nam, struct thread *td)
 */
int
tcp6_usr_connect(struct tcpcb* tp, struct sockaddr_in6* sin6p)
{
	int error = 0;

	if (tp->t_state != TCPS_CLOSED) { // samkumar: This is a check that I added
		return (EISCONN);
	}

	/* samkumar: I removed the following error check since we receive sin6p
	 * in the function argument and don't need to convert a struct sockaddr to
	 * a struct sockaddr_in6 anymore.
	 *
	 * if (nam->sa_len != sizeof (*sin6p))
	 * 	return (EINVAL);
	 */

	/*
	 * Must disallow TCP ``connections'' to multicast addresses.
	 */
	/* samkumar: I commented out the check on sin6p->sin6_family. */
	if (/*sin6p->sin6_family == AF_INET6
	    && */IN6_IS_ADDR_MULTICAST(&sin6p->sin6_addr))
		return (EAFNOSUPPORT);

	/*
	 * samkumar: There was some code here that obtained the TCB (struct tcpcb*)
	 * by getting the inpcb from the socket and the TCB from the inpcb. I
	 * removed that code.
	 */

	/*
	 * XXXRW: Some confusion: V4/V6 flags relate to binding, and
	 * therefore probably require the hash lock, which isn't held here.
	 * Is this a significant problem?
	 */
	if (IN6_IS_ADDR_V4MAPPED(&sin6p->sin6_addr)) {
		tcplp_sys_log("V4-Mapped Address!");

		/*
		 * samkumar: There used to be code that woulf handle the case of
		 * v4-mapped addresses. It would call in6_sin6_2_sin to convert the
		 * struct sockaddr_in6 to a struct sockaddr_in, set the INP_IPV4 flag
		 * and clear the INP_IPV6 flag on inp->inp_vflag, do some other
		 * processing, and finally call tcp_connect and tcp_output. However,
		 * it would first check if the IN6P_IPV6_V6ONLY flag was set in
		 * inp->inp_flags, and if so, it would return with EINVAL. In TCPlp, we
		 * support IPv6 only, so I removed the check for IN6P_IPV6_V6ONLY and
		 * always act as if that flag is set. I kept the code in that if
		 * statement making the check, and removed the other code that actually
		 * handled this case.
		 */
		error = EINVAL;
		goto out;
	}

	/*
	 * samkumar: I removed some code here that set/cleared some flags in the`
	 * inpcb and called prison_remote_ip6.
	 */

	/*
	 * samkumar: Originally, the struct thread *td was passed along to
	 * tcp6_connect.
	 */
	if ((error = tcp6_connect(tp, sin6p)) != 0)
		goto out;

	tcp_timer_activate(tp, TT_KEEP, TP_KEEPINIT(tp));
	error = tcp_output(tp);

out:
	return (error);
}

/*
 * Do a send by putting data in output queue and updating urgent
 * marker if URG set.  Possibly send more data.  Unlike the other
 * pru_*() routines, the mbuf chains are our responsibility.  We
 * must either enqueue them or free them.  The other pru_* routines
 * generally are caller-frees.
 */
/*
 * samkumar: I removed locking, statistics, inpcb management, and debug probes.
 * I also removed support for the urgent pointer.
 *
 * I changed the signature of this function. It used to be
 * static int
 * tcp_usr_send(struct socket *so, int flags, struct mbuf *m,
 *     struct sockaddr *nam, struct mbuf *control, struct thread *td)
 *
 * The new function signature works as follows. DATA is a new linked buffer to
 * add to the end of the send buffer. EXTENDBY is the number of bytes by which
 * to extend the final linked buffer of the send buffer. Either DATA should be
 * NULL, or EXTENDBY should be 0.
 */
int tcp_usr_send(struct tcpcb* tp, int moretocome, otLinkedBuffer* data, size_t extendby, struct sockaddr_in6* nam)
{
	int error = 0;
	int do_fastopen_implied_connect = (nam != NULL) && IS_FASTOPEN(tp->t_flags) && tp->t_state < TCPS_SYN_SENT;

	/*
	 * samkumar: This if statement and the next are checks that I added
	 */
	if (tp->t_state < TCPS_ESTABLISHED && !IS_FASTOPEN(tp->t_flags)) {
		error = ENOTCONN;
		goto out;
	}

	if (tpiscantsend(tp)) {
		error = EPIPE;
		goto out;
	}

	/*
	 * samkumar: There used to be logic here that acquired locks, dealt with
	 * INP_TIMEWAIT and INP_DROPPED flags on inp->inp_flags, and handled the
	 * control mbuf passed as an argument (which would result in an error since
	 * TCP doesn't support control information). I've deleted that code, but
	 * added the following if block based on those checks.
	 */
	 if ((tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED && !do_fastopen_implied_connect)) {
		error = ECONNRESET;
		goto out;
	}

	/*
	 * The following code used to be wrapped in an if statement:
	 * "if (!(flags & PRUS_OOB))", that only executed it if the "out of band"
	 * flag was not set. In TCP, "out of band" data is conveyed via the urgent
	 * pointer, and TCPlp does not support the urgent pointer. Therefore, I
	 * removed the "if" check and put its body below.
	 */

	/*
	 * samkumar: The FreeBSD code calls sbappendstream(&so->so_snd, m, flags);
	 * I've replaced it with the following logic, which appends to the
	 * send buffer according to TCPlp's data structures.
	 */
	if (data == NULL) {
		if (extendby == 0) {
			goto out;
		}
		lbuf_extend(&tp->sendbuf, extendby);
	} else {
		if (data->mLength == 0) {
			goto out;
		}
		lbuf_append(&tp->sendbuf, data);
	}

	/*
	 * samkumar: There used to be code here to handle "implied connect,"
	 * which initiates the TCP handshake if sending data on a socket that
	 * isn't yet connected. For now, I've special-cased this code to work
	 * only for TCP Fast Open for IPv6 (since implied connect is the only
	 * way to initiate a connection with TCP Fast Open).
	 */
	if (do_fastopen_implied_connect) {
		error = tcp6_connect(tp, nam);
		if (error)
			goto out;
		tcp_fastopen_connect(tp);
	}

	/*
	 * samkumar: There used to be code here handling the PRUS_EOF flag in
	 * the former flags parameter. I've removed this code.
	 */

	/*
	 * samkumar: The code below was previously wrapped in an if statement
	 * that checked that the INP_DROPPED flag in inp->inp_flags and the
	 * PRUS_NOTREADY flag in the former flags parameter were both clear.
	 * If either one was set, then tcp_output would not be called.
	 *
	 * The "more to come" functionality was previously triggered via the
	 * PRUS_MORETOCOME flag in the flags parameter to this function. Since
	 * that's the only flag that TCPlp uses here, I replaced the flags
	 * parameter with a "moretocome" parameter, which we check instead.
	 */
	if (moretocome)
		tp->t_flags |= TF_MORETOCOME;
	error = tcp_output(tp);
	if (moretocome)
		tp->t_flags &= ~TF_MORETOCOME;

	/*
	 * samkumar: This is where the "if (!(flags & PRUS_OOB))" block would end.
	 * There used to be a large "else" block handling out-of-band data, but I
	 * removed that entire block since we do not support the urgent pointer in
	 * TCPlp.
	 */
out:
	return (error);
}

/*
 * After a receive, possibly send window update to peer.
 */
int
tcp_usr_rcvd(struct tcpcb* tp)
{
	int error = 0;

	/*
	 * samkumar: There used to be logic here that acquired locks, dealt with
	 * INP_TIMEWAIT and INP_DROPPED flags on inp->inp_flags, and added debug
	 * probes I've deleted that code, but left the following if block.
	 */
	if ((tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED)) {
		error = ECONNRESET;
		goto out;
	}

	/*
	 * For passively-created TFO connections, don't attempt a window
	 * update while still in SYN_RECEIVED as this may trigger an early
	 * SYN|ACK.  It is preferable to have the SYN|ACK be sent along with
	 * application response data, or failing that, when the DELACK timer
	 * expires.
	 */
	if (IS_FASTOPEN(tp->t_flags) &&
	    (tp->t_state == TCPS_SYN_RECEIVED))
		goto out;

	tcp_output(tp);

out:
	return (error);
}

/*
 * samkumar: Removed the tcp_disconnect function. It is meant to be a
 * "friendly" disconnect to complement the unceremonious "abort" functionality
 * that is also provbided. The FreeBSD implementation called it from
 * tcp_usr_close, which we removed (see the comment below for the reason why).
 * It's not called from anywhere else, so I'm removing this function entirely.
 */

/*
 * Mark the connection as being incapable of further output.
 */
/*
 * samkumar: Modified to remove locking, socket/inpcb handling, and debug
 * probes.
 */
int
tcp_usr_shutdown(struct tcpcb* tp)
{
	int error = 0;

	/*
	 * samkumar: replaced checks on the INP_TIMEWAIT and INP_DROPPED flags on
	 * inp->inp_flags with these checks on tp->t_state.
	 */
	if ((tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED)) {
		error = ECONNRESET;
		goto out;
	}

	/* samkumar: replaced socantsendmore with tpcantsendmore */
	tpcantsendmore(tp);
	tcp_usrclosed(tp);

	/*
	 * samkumar: replaced check on INP_DROPPED flag in inp->inp_flags with
	 * this check on tp->t_state.
	 */
	if (tp->t_state != TCPS_CLOSED)
		error = tcp_output(tp);

out:
	return (error);
}


/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
/*
 * Removed locking, TCP Offload, and socket/inpcb handling.
 */
static void
tcp_usrclosed(struct tcpcb *tp)
{
	switch (tp->t_state) {
	case TCPS_LISTEN:
		tcp_state_change(tp, TCPS_CLOSED);
		/* FALLTHROUGH */
	case TCPS_CLOSED:
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		/*
		 * tcp_close() should never return NULL here as the socket is
		 * still open.
		 */
		KASSERT(tp != NULL,
		    ("tcp_usrclosed: tcp_close() returned NULL"));
		break;

	case TCPS_SYN_SENT:
	case TCPS_SYN_RECEIVED:
		tp->t_flags |= TF_NEEDFIN;
		break;

	case TCPS_ESTABLISHED:
		tcp_state_change(tp, TCPS_FIN_WAIT_1);
		break;

	case TCPS_CLOSE_WAIT:
		tcp_state_change(tp, TCPS_LAST_ACK);
		break;
	}
	if (tp->t_state >= TCPS_FIN_WAIT_2) {
		/* samkumar: commented out the following "soisdisconnected" line. */
		// soisdisconnected(tp->t_inpcb->inp_socket);
		/* Prevent the connection hanging in FIN_WAIT_2 forever. */
		if (tp->t_state == TCPS_FIN_WAIT_2) {
			int timeout;

			timeout = (tcp_fast_finwait2_recycle) ?
			    tcp_finwait2_timeout : TP_MAXIDLE(tp);
			tcp_timer_activate(tp, TT_2MSL, timeout);
		}
	}
}

/*
 * samkumar: I removed the tcp_usr_close function. It was meant to be called in
 * case the socket is closed. It calls tcp_disconnect if the underlying TCP
 * connection is still alive when the socket is closed ("full TCP state").
 * In TCPlp, we can't handle this because we want to free up the underlying
 * memory immediately when the user deallocates a TCP connection, making it
 * unavailable for the somewhat more ceremonious closing that tcp_disconnect
 * would allow. The host system is expected to simply abort the connection if
 * the application deallocates it.
 */

/*
 * Abort the TCP.  Drop the connection abruptly.
 */
/*
 * samkumar: Modified to remove locking, socket/inpcb handling, and debug
 * probes.
 */
void
tcp_usr_abort(struct tcpcb* tp)
{
	/*
	 * If we still have full TCP state, and we're not dropped, drop.
	 */
	/*
	 * I replaced the checks on inp->inp_flags (which tested for the absence of
	 * INP_TIMEWAIT and INP_DROPPED flags), with the following checks on
	 * tp->t_state.
	 */
	if (tp->t_state != TCP6S_TIME_WAIT &&
		tp->t_state != TCP6S_CLOSED) {
		tcp_drop(tp, ECONNABORTED);
	} else if (tp->t_state == TCPS_TIME_WAIT) { // samkumar: I added this clause
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
	}
}
