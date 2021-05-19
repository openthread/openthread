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
//#include <sys/socket.h>
#include "ip6.h"

#include "tcp_const.h"

#include <openthread/tcp.h>

//static void	tcp_disconnect(struct tcpcb *);
static void	tcp_usrclosed(struct tcpcb *);

#if 0
static int
tcp6_usr_bind(struct socket *so, struct sockaddr *nam, struct thread *td)
{
	int error = 0;
	struct inpcb *inp;
	struct tcpcb *tp = NULL;
	struct sockaddr_in6 *sin6p;

	sin6p = (struct sockaddr_in6 *)nam;
	if (nam->sa_len != sizeof (*sin6p))
		return (EINVAL);
	/*
	 * Must check for multicast addresses and disallow binding
	 * to them.
	 */
	if (sin6p->sin6_family == AF_INET6 &&
	    IN6_IS_ADDR_MULTICAST(&sin6p->sin6_addr))
		return (EAFNOSUPPORT);

	TCPDEBUG0;
	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("tcp6_usr_bind: inp == NULL"));
	INP_WLOCK(inp);
	if (inp->inp_flags & (INP_TIMEWAIT | INP_DROPPED)) {
		error = EINVAL;
		goto out;
	}
	tp = intotcpcb(inp);
	TCPDEBUG1();
	INP_HASH_WLOCK(&V_tcbinfo);
	inp->inp_vflag &= ~INP_IPV4;
	inp->inp_vflag |= INP_IPV6;
#ifdef INET
	if ((inp->inp_flags & IN6P_IPV6_V6ONLY) == 0) {
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6p->sin6_addr))
			inp->inp_vflag |= INP_IPV4;
		else if (IN6_IS_ADDR_V4MAPPED(&sin6p->sin6_addr)) {
			struct sockaddr_in sin;

			in6_sin6_2_sin(&sin, sin6p);
			inp->inp_vflag |= INP_IPV4;
			inp->inp_vflag &= ~INP_IPV6;
			error = in_pcbbind(inp, (struct sockaddr *)&sin,
			    td->td_ucred);
			INP_HASH_WUNLOCK(&V_tcbinfo);
			goto out;
		}
	}
#endif
	error = in6_pcbbind(inp, nam, td->td_ucred);
	INP_HASH_WUNLOCK(&V_tcbinfo);
out:
	TCPDEBUG2(PRU_BIND);
	TCP_PROBE2(debug__user, tp, PRU_BIND);
	INP_WUNLOCK(inp);
	return (error);
}
#endif

/* Based on a function in in6_pcb.c. */
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
/* Signature used to be
static int
tcp6_connect(struct tcpcb *tp, struct sockaddr *nam, struct thread *td)
*/
static int
tcp6_connect(struct tcpcb *tp, struct sockaddr_in6 *nam)
{
//	struct inpcb *inp = tp->t_inpcb;
	int error;

	int sb_max = cbuf_free_space(&tp->recvbuf); // same as sendbuf
//	INP_WLOCK_ASSERT(inp);
//	INP_HASH_WLOCK(&V_tcbinfo);
	if (/*inp->inp_lport == 0*/tp->lport == 0) {
		/*error = in6_pcbbind(inp, (struct sockaddr *)0, td->td_ucred);
		if (error)
			goto out;*/
		error = EINVAL; // The port must be bound
		goto out;
	}
	if (IN6_IS_ADDR_UNSPECIFIED(&tp->laddr)) {
		const struct in6_addr* source = tcplp_sys_get_source_ipv6_address(tp->instance, &nam->sin6_addr); // choose address dynamically
		if (source == NULL) {
			error = EINVAL;
			goto out;
		}
		memcpy(&tp->laddr, source, sizeof(tp->laddr));
	}
	error = in6_pcbconnect(/*inp*/tp, nam/*, td->td_ucred*/);
	if (error != 0)
		goto out;
//	INP_HASH_WUNLOCK(&V_tcbinfo);

	/* Compute window scaling to request.  */
	while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
	    (TCP_MAXWIN << tp->request_r_scale) < sb_max)
		tp->request_r_scale++;

//	soisconnecting(inp->inp_socket);
//	TCPSTAT_INC(tcps_connattempt);
	tcp_state_change(tp, TCPS_SYN_SENT);
	tp->iss = tcp_new_isn(tp);
	tcp_sendseqinit(tp);

	return 0;

out:
//	INP_HASH_WUNLOCK(&V_tcbinfo);
	return error;
}

/*
The signature used to be
static int
tcp6_usr_connect(struct socket *so, struct sockaddr *nam, struct thread *td)
*/
int
tcp6_usr_connect(struct tcpcb* tp, struct sockaddr_in6* sin6p)
{
	int error = 0;

	if (tp->t_state != TCPS_CLOSED) { // This is a check that I added
		return (EISCONN);
	}
//	struct inpcb *inp;
//	struct tcpcb *tp = NULL;
//	struct sockaddr_in6 *sin6p;

//	TCPDEBUG0;

//	sin6p = (struct sockaddr_in6 *)nam;
//	if (nam->sa_len != sizeof (*sin6p))
//		return (EINVAL);
	/*
	 * Must disallow TCP ``connections'' to multicast addresses.
	 */
	if (/*sin6p->sin6_family == AF_INET6
	    && */IN6_IS_ADDR_MULTICAST(&sin6p->sin6_addr))
		return (EAFNOSUPPORT);
#if 0 // We already have the TCB
	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("tcp6_usr_connect: inp == NULL"));
	INP_WLOCK(inp);
	if (inp->inp_flags & INP_TIMEWAIT) {
		error = EADDRINUSE;
		goto out;
	}
	if (inp->inp_flags & INP_DROPPED) {
		error = ECONNREFUSED;
		goto out;
	}
	tp = intotcpcb(inp);
#endif
//	TCPDEBUG1();
//#ifdef INET
	/*
	 * XXXRW: Some confusion: V4/V6 flags relate to binding, and
	 * therefore probably require the hash lock, which isn't held here.
	 * Is this a significant problem?
	 */
	if (IN6_IS_ADDR_V4MAPPED(&sin6p->sin6_addr)) {
//		struct sockaddr_in sin;

        tcplp_sys_log("V4-Mapped Address!\n");

		if (/*(inp->inp_flags & IN6P_IPV6_V6ONLY) != 0*/1) {
			error = EINVAL;
			goto out;
		}
#if 0 // Not needed since we'll take the if branch anyway
		in6_sin6_2_sin(&sin, sin6p);
		inp->inp_vflag |= INP_IPV4;
		inp->inp_vflag &= ~INP_IPV6;
		if ((error = prison_remote_ip4(td->td_ucred,
		    &sin.sin_addr)) != 0)
			goto out;
		if ((error = tcp_connect(tp, (struct sockaddr *)&sin, td)) != 0)
			goto out;
#endif
#if 0
#ifdef TCP_OFFLOAD
		if (registered_toedevs > 0 &&
		    (so->so_options & SO_NO_OFFLOAD) == 0 &&
		    (error = tcp_offload_connect(so, nam)) == 0)
			goto out;
#endif
#endif
		error = tcp_output(tp);
		goto out;
	}
//#endif
//	inp->inp_vflag &= ~INP_IPV4;
//	inp->inp_vflag |= INP_IPV6;
//	inp->inp_inc.inc_flags |= INC_ISIPV6;
//	if ((error = prison_remote_ip6(td->td_ucred, &sin6p->sin6_addr)) != 0)
//		goto out;
	if ((error = tcp6_connect(tp, sin6p/*, td*/)) != 0)
		goto out;
#if 0
#ifdef TCP_OFFLOAD
	if (registered_toedevs > 0 &&
	    (so->so_options & SO_NO_OFFLOAD) == 0 &&
	    (error = tcp_offload_connect(so, nam)) == 0)
		goto out;
#endif
#endif
	tcp_timer_activate(tp, TT_KEEP, TP_KEEPINIT(tp));
	error = tcp_output(tp);

out:
#if 0
	TCPDEBUG2(PRU_CONNECT);
	TCP_PROBE2(debug__user, tp, PRU_CONNECT);
	INP_WUNLOCK(inp);
#endif
	return (error);
}

/*
 * Do a send by putting data in output queue and updating urgent
 * marker if URG set.  Possibly send more data.  Unlike the other
 * pru_*() routines, the mbuf chains are our responsibility.  We
 * must either enqueue them or free them.  The other pru_* routines
 * generally are caller-frees.
 */
/* I changed the signature of this function. */
/*static int
tcp_usr_send(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *nam, struct mbuf *control, struct thread *td)*/
/* Returns error condition, and stores bytes sent into SENT. */
int tcp_usr_send(struct tcpcb* tp, int moretocome, otLinkedBuffer* data)
{
	int error = 0;
//	struct inpcb *inp;
//	struct tcpcb *tp = NULL;
#if 0
#ifdef INET6
	int isipv6;
#endif
	TCPDEBUG0;
#endif
	if (tp->t_state < TCPS_ESTABLISHED) { // This if statement and the next are checks that I added
		error = ENOTCONN;
		goto out;
	}

    /* For the TinyOS version I used ESHUTDOWN, but apparently it doesn't
     * come by default when you include errno.h: you need to also #define
     * __LINUX_ERRNO_EXTENSIONS__. So I switched to EPIPE.
     */
	if (tpiscantsend(tp)) {
		//error = ESHUTDOWN;
        error = EPIPE;
		goto out;
	}

	if ((tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED)) { // copied from the commented-out code from below
		error = ECONNRESET;
		goto out;
	}

	/*
	 * We require the pcbinfo lock if we will close the socket as part of
	 * this call.
	 */
#if 0
	if (flags & PRUS_EOF)
		INP_INFO_RLOCK(&V_tcbinfo);
	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("tcp_usr_send: inp == NULL"));
	INP_WLOCK(inp);
	if (inp->inp_flags & (INP_TIMEWAIT | INP_DROPPED)) {
		if (control)
			m_freem(control);
		/*
		 * In case of PRUS_NOTREADY, tcp_usr_ready() is responsible
		 * for freeing memory.
		 */
		if (m && (flags & PRUS_NOTREADY) == 0)
			m_freem(m);
		error = ECONNRESET;
		goto out;
	}
#ifdef INET6
	isipv6 = nam && nam->sa_family == AF_INET6;
#endif /* INET6 */
	tp = intotcpcb(inp);
	TCPDEBUG1();
	if (control) {
		/* TCP doesn't do control messages (rights, creds, etc) */
		if (control->m_len) {
			m_freem(control);
			if (m)
				m_freem(m);
			error = EINVAL;
			goto out;
		}
		m_freem(control);	/* empty control, just free it */
	}
	if (!(flags & PRUS_OOB)) {
#endif // DON'T SUPPORT URGENT DATA
		/*sbappendstream(&so->so_snd, m, flags);*/
        lbuf_append(&tp->sendbuf, data);
        if (data->mLength == 0) {
             goto out;
        }
#if 0 // DON'T SUPPORT IMPLIED CONNECTION
		if (nam && tp->t_state < TCPS_SYN_SENT) {
			/*
			 * Do implied connect if not yet connected,
			 * initialize window to default value, and
			 * initialize maxseg/maxopd using peer's cached
			 * MSS.
			 */
#ifdef INET6
			if (isipv6)
				error = tcp6_connect(tp, nam, td);
#endif /* INET6 */
#if defined(INET6) && defined(INET)
			else
#endif
#ifdef INET
				error = tcp_connect(tp, nam, td);
#endif
			if (error)
				goto out;
			tp->snd_wnd = TTCP_CLIENT_SND_WND;
			tcp_mss(tp, -1);
		}
#endif
#if 0
		if (flags & PRUS_EOF) {
			/*
			 * Close the send side of the connection after
			 * the data is sent.
			 */
			INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
			socantsendmore(so);
			tcp_usrclosed(tp);
		}
#endif
//		if (!(inp->inp_flags & INP_DROPPED) &&
//		    !(flags & PRUS_NOTREADY)) {
			if (/*flags & PRUS_MORETOCOME*/ moretocome)
				tp->t_flags |= TF_MORETOCOME;
			error = tcp_output(tp);
			if (/*flags & PRUS_MORETOCOME*/ moretocome)
				tp->t_flags &= ~TF_MORETOCOME;
//		}
#if 0 // DON'T SUPPORT OUT-OF-BAND DATA (URGENT POINTER IN TCP CASE)
	} else {
		/*
		 * XXXRW: PRUS_EOF not implemented with PRUS_OOB?
		 */
		SOCKBUF_LOCK(&so->so_snd);
		if (sbspace(&so->so_snd) < -512) {
			SOCKBUF_UNLOCK(&so->so_snd);
			m_freem(m);
			error = ENOBUFS;
			goto out;
		}
		/*
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section.
		 * Otherwise, snd_up should be one lower.
		 */
		sbappendstream_locked(&so->so_snd, m, flags);
		SOCKBUF_UNLOCK(&so->so_snd);
		if (nam && tp->t_state < TCPS_SYN_SENT) {
			/*
			 * Do implied connect if not yet connected,
			 * initialize window to default value, and
			 * initialize maxseg/maxopd using peer's cached
			 * MSS.
			 */
#ifdef INET6
			if (isipv6)
				error = tcp6_connect(tp, nam, td);
#endif /* INET6 */
#if defined(INET6) && defined(INET)
			else
#endif
#ifdef INET
				error = tcp_connect(tp, nam, td);
#endif
			if (error)
				goto out;
			tp->snd_wnd = TTCP_CLIENT_SND_WND;
			tcp_mss(tp, -1);
		}
		tp->snd_up = tp->snd_una + sbavail(&so->so_snd);
		if (!(flags & PRUS_NOTREADY)) {
			tp->t_flags |= TF_FORCEDATA;
			error = tcp_output(tp);
			tp->t_flags &= ~TF_FORCEDATA;
		}
	}
#endif
out:
#if 0 // REMOVE THEIR SYNCHRONIZATION
	TCPDEBUG2((flags & PRUS_OOB) ? PRU_SENDOOB :
		  ((flags & PRUS_EOF) ? PRU_SEND_EOF : PRU_SEND));
	TCP_PROBE2(debug__user, tp, (flags & PRUS_OOB) ? PRU_SENDOOB :
		   ((flags & PRUS_EOF) ? PRU_SEND_EOF : PRU_SEND));
	INP_WUNLOCK(inp);
	if (flags & PRUS_EOF)
		INP_INFO_RUNLOCK(&V_tcbinfo);
#endif
	return (error);
}

/*
 * After a receive, possibly send window update to peer.
 */
int
tcp_usr_rcvd(struct tcpcb* tp/*, int flags*/)
{
//	struct inpcb *inp;
//	struct tcpcb *tp = NULL;
	int error = 0;
	if ((tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED)) {
		error = ECONNRESET;
		goto out;
	}
#if 0
	TCPDEBUG0;
	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("tcp_usr_rcvd: inp == NULL"));
	INP_WLOCK(inp);
	if (inp->inp_flags & (INP_TIMEWAIT | INP_DROPPED)) {
		error = ECONNRESET;
		goto out;
	}
	tp = intotcpcb(inp);
	TCPDEBUG1();
#ifdef TCP_OFFLOAD
	if (tp->t_flags & TF_TOE)
		tcp_offload_rcvd(tp);
	else
#endif
#endif
	tcp_output(tp);

out:
//	TCPDEBUG2(PRU_RCVD);
//	TCP_PROBE2(debug__user, tp, PRU_RCVD);
//	INP_WUNLOCK(inp);
	return (error);
}

#if 0

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
static void
tcp_disconnect(struct tcpcb *tp)
{
//	struct inpcb *inp = tp->t_inpcb;
//	struct socket *so = inp->inp_socket;

//	INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
//	INP_WLOCK_ASSERT(inp);

	/*
	 * Neither tcp_close() nor tcp_drop() should return NULL, as the
	 * socket is still open.
	 */
	if (tp->t_state < TCPS_ESTABLISHED) {
		tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
		KASSERT(tp != NULL,
		    ("tcp_disconnect: tcp_close() returned NULL"));
	}/* else if ((so->so_options & SO_LINGER) && so->so_linger == 0) {
		tp = tcp_drop(tp, 0);
		KASSERT(tp != NULL,
		    ("tcp_disconnect: tcp_drop() returned NULL"));
	}*/ else {
//		soisdisconnecting(so);
//		sbflush(&so->so_rcv);
		tcp_usrclosed(tp);
		if (/*!(inp->inp_flags & INP_DROPPED)*/tp->t_state != TCPS_CLOSED)
			tcp_output(tp);
	}
}

#endif

/*
 * Mark the connection as being incapable of further output.
 */
int
tcp_usr_shutdown(struct tcpcb* tp)
{
	int error = 0;
#if 0
	struct inpcb *inp;
	struct tcpcb *tp = NULL;

	TCPDEBUG0;
	INP_INFO_RLOCK(&V_tcbinfo);
	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("inp == NULL"));
	INP_WLOCK(inp);
#endif
	if (/*inp->inp_flags & (INP_TIMEWAIT | INP_DROPPED)*/
	    (tp->t_state == TCPS_TIME_WAIT) || (tp->t_state == TCPS_CLOSED)) {
		error = ECONNRESET;
		goto out;
	}
#if 0
	tp = intotcpcb(inp);
	TCPDEBUG1();
#endif
//	socantsendmore(so);
	tpcantsendmore(tp);
	tcp_usrclosed(tp);
	if (/*!(inp->inp_flags & INP_DROPPED)*/tp->t_state != TCPS_CLOSED)
		error = tcp_output(tp);

out:
#if 0
	TCPDEBUG2(PRU_SHUTDOWN);
	TCP_PROBE2(debug__user, tp, PRU_SHUTDOWN);
	INP_WUNLOCK(inp);
	INP_INFO_RUNLOCK(&V_tcbinfo);
#endif
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
static void
tcp_usrclosed(struct tcpcb *tp)
{

//	INP_INFO_RLOCK_ASSERT(&V_tcbinfo);
//	INP_WLOCK_ASSERT(tp->t_inpcb);

	switch (tp->t_state) {
	case TCPS_LISTEN:
//#ifdef TCP_OFFLOAD
//		tcp_offload_listen_stop(tp);
//#endif
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
//		soisdisconnected(tp->t_inpcb->inp_socket);
		/* Prevent the connection hanging in FIN_WAIT_2 forever. */
		if (tp->t_state == TCPS_FIN_WAIT_2) {
			int timeout;

			timeout = (tcp_fast_finwait2_recycle) ?
			    tcp_finwait2_timeout : TP_MAXIDLE(tp);
			tcp_timer_activate(tp, TT_2MSL, timeout);
		}
	}
}

#if 0
/*
 * TCP socket is closed.  Start friendly disconnect.
 */
static void
tcp_usr_close(struct tcpcb* tp/*struct socket *so*/)
{
//	struct inpcb *inp;
//	struct tcpcb *tp = NULL;
//	TCPDEBUG0;

//	inp = sotoinpcb(so);
//	KASSERT(inp != NULL, ("tcp_usr_close: inp == NULL"));

//	INP_INFO_RLOCK(&V_tcbinfo);
//	INP_WLOCK(inp);
//	KASSERT(inp->inp_socket != NULL,
//	    ("tcp_usr_close: inp_socket == NULL"));

	/*
	 * If we still have full TCP state, and we're not dropped, initiate
	 * a disconnect.
	 */
	if ((tp->t_state != TCP6S_TIME_WAIT) && (tp->t_state != TCPS_CLOSED)/*!(inp->inp_flags & INP_TIMEWAIT) &&
	    !(inp->inp_flags & INP_DROPPED)*/) {
//		tp = intotcpcb(inp);
//		TCPDEBUG1();
		tpcantsendmore(tp);
		tpcantrcvmore(tp); /* Added by Sam: This would be probably be done at the socket layer. */
		tcp_disconnect(tp);
//		TCPDEBUG2(PRU_CLOSE);
//		TCP_PROBE2(debug__user, tp, PRU_CLOSE);
	}
#if 0
	if (!(inp->inp_flags & INP_DROPPED)) {
		SOCK_LOCK(so);
		so->so_state |= SS_PROTOREF;
		SOCK_UNLOCK(so);
		inp->inp_flags |= INP_SOCKREF;
	}
#endif
//	INP_WUNLOCK(inp);
//	INP_INFO_RUNLOCK(&V_tcbinfo);
}
#endif

/*
 * Abort the TCP.  Drop the connection abruptly.
 */
void
tcp_usr_abort(/*struct socket *so*/struct tcpcb* tp)
{
#if 0
	struct inpcb *inp;
	struct tcpcb *tp = NULL;
	TCPDEBUG0;

	inp = sotoinpcb(so);
	KASSERT(inp != NULL, ("tcp_usr_abort: inp == NULL"));

	INP_INFO_RLOCK(&V_tcbinfo);
	INP_WLOCK(inp);
	KASSERT(inp->inp_socket != NULL,
	    ("tcp_usr_abort: inp_socket == NULL"));
#endif
	/*
	 * If we still have full TCP state, and we're not dropped, drop.
	 */
	if (/*!(inp->inp_flags & INP_TIMEWAIT) &&
	    !(inp->inp_flags & INP_DROPPED)*/
	    tp->t_state != TCP6S_TIME_WAIT &&
	    tp->t_state != TCP6S_CLOSED) {
//		tp = intotcpcb(inp);
//		TCPDEBUG1();
		tcp_drop(tp, ECONNABORTED);
//		TCPDEBUG2(PRU_ABORT);
//		TCP_PROBE2(debug__user, tp, PRU_ABORT);
	} else if (tp->t_state == TCPS_TIME_WAIT) { // This clause added by Sam
	    tp = tcp_close(tp);
		tcplp_sys_connection_lost(tp, CONN_LOST_NORMAL);
	}
#if 0
	if (!(inp->inp_flags & INP_DROPPED)) {
		SOCK_LOCK(so);
		so->so_state |= SS_PROTOREF;
		SOCK_UNLOCK(so);
		inp->inp_flags |= INP_SOCKREF;
	}
	INP_WUNLOCK(inp);
	INP_INFO_RUNLOCK(&V_tcbinfo);
#endif
}
