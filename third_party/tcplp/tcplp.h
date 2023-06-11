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

/**
 * @file
 *   This file defines the interface between TCPlp and the host network stack
 *   (OpenThread, in this case). It is based on content taken from TCPlp,
 *   modified to work with this port of TCPlp to OpenThread.
 */

#ifndef TCPLP_H_
#define TCPLP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include "bsdtcp/ip6.h"
#include "bsdtcp/tcp.h"
#include "bsdtcp/tcp_fsm.h"
#include "bsdtcp/tcp_timer.h"
#include "bsdtcp/tcp_var.h"

#define RELOOKUP_REQUIRED -1
#define CONN_LOST_NORMAL 0

struct tcplp_signals
{
    struct tcpcb* accepted_connection;
    uint32_t      links_popped;
    uint32_t      bytes_acked;
    bool          conn_established;
    bool          recvbuf_added;
    bool          rcvd_fin;
};

/*
 * Functions that the TCP protocol logic can call to interact with the rest of
 * the system.
 */
otMessage *   tcplp_sys_new_message(otInstance *aInstance);
void          tcplp_sys_free_message(otInstance *aInstance, otMessage *aMessage);
void          tcplp_sys_send_message(otInstance *aInstance, otMessage *aMessage, otMessageInfo *aMessageInfo);
uint32_t      tcplp_sys_get_ticks();
uint32_t      tcplp_sys_get_millis();
void          tcplp_sys_set_timer(struct tcpcb *aTcb, uint8_t aTimerFlag, uint32_t aDelay);
void          tcplp_sys_stop_timer(struct tcpcb *aTcb, uint8_t aTimerFlag);
struct tcpcb *tcplp_sys_accept_ready(struct tcpcb_listen *aTcbListen, struct in6_addr *aAddr, uint16_t aPort);
bool          tcplp_sys_accepted_connection(struct tcpcb_listen *aTcbListen,
                                            struct tcpcb *       aAccepted,
                                            struct in6_addr *    aAddr,
                                            uint16_t             aPort);
void          tcplp_sys_connection_lost(struct tcpcb *aTcb, uint8_t aErrNum);
void          tcplp_sys_on_state_change(struct tcpcb *aTcb, int aNewState);
void          tcplp_sys_log(const char *aFormat, ...);
void          tcplp_sys_panic(const char *aFormat, ...);
bool          tcplp_sys_autobind(otInstance *      aInstance,
                                 const otSockAddr *aPeer,
                                 otSockAddr *      aToBind,
                                 bool              aBindAddress,
                                 bool              aBindPort);
uint32_t      tcplp_sys_generate_isn();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TCPLP_H_
