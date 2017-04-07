/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 * @brief
 *  This file defines the top-level functions for the OpenThread library.
 */

#ifndef OPENTHREAD_H_
#define OPENTHREAD_H_

#include "openthread/types.h"

#include "openthread/crypto.h"
#include "openthread/dataset.h"
#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/link.h"
#include "openthread/message.h"
#include "openthread/netdata.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup api  API
 * @brief
 *   This module includes the application programming interface to the OpenThread stack.
 *
 * @{
 *
 * @defgroup execution Execution
 * @defgroup commands Commands
 * @defgroup config Configuration
 * @defgroup diags Diagnostics
 * @defgroup messages Message Buffers
 * @defgroup ip6 IPv6
 * @defgroup udp UDP
 * @defgroup coap CoAP
 *
 * @}
 *
 */

/**
 * @defgroup platform  Platform Abstraction
 * @brief
 *   This module includes the platform abstraction used by the OpenThread stack.
 *
 * @{
 * @}
 *
 */

/**
 * @defgroup core Core
 * @brief
 *   This module includes the core OpenThread stack.
 *
 * @{
 *
 * @defgroup core-6lowpan 6LoWPAN
 * @defgroup core-coap CoAP
 * @defgroup core-global-address Global IPv6 Address
 * @defgroup core-ipv6 IPv6
 * @defgroup core-mac MAC
 * @defgroup core-mesh-forwarding Mesh Forwarding
 * @defgroup core-data-poll-manager Data Poll Manager
 * @defgroup core-source-match-controller Source Address Match Controller
 * @defgroup core-message Message
 * @defgroup core-mle MLE
 * @defgroup core-netdata Network Data
 * @defgroup core-netif Network Interface
 * @defgroup core-arp RLOC Mapping
 * @defgroup core-security Security
 * @defgroup core-tasklet Tasklet
 * @defgroup core-timer Timer
 * @defgroup core-udp UDP
 * @defgroup core-tcp TCP
 * @defgroup core-link-quality Link Quality
 *
 * @}
 *
 */


/**
 * Get the OpenThread version string.
 *
 * @returns A pointer to the OpenThread version.
 *
 */
OTAPI const char *OTCALL otGetVersionString(void);

/**
 * This function converts a ThreadError enum into a string.
 *
 * @param[in]  aError     A ThreadError enum.
 *
 * @returns  A string representation of a ThreadError.
 *
 */
OTAPI const char *OTCALL otThreadErrorToString(ThreadError aError);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_H_
