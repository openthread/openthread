/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#ifndef OPENTHREAD_CORE_TORANJ_CONFIG_POSIX_H_
#define OPENTHREAD_CORE_TORANJ_CONFIG_POSIX_H_

/**
 * This header file defines the OpenThread core configuration options for toranj with POSIX platform.
 */

// Include the common configuration for all platforms.
#include "openthread-core-toranj-config.h"

#define OPENTHREAD_CONFIG_PLATFORM_INFO "POSIX-toranj"

#define OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE 1

#define OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE 1

#define OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE 1

#define OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE 0

#ifdef __linux__
#define OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE 0
#endif

#define OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE 1

#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED

#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 1

#define OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE 1

#define OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE 1

#define OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE 1

// Disabled explicitly on posix `toranj` to validate the build with this config
#define OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE 0

#endif /* OPENTHREAD_CORE_TORANJ_CONFIG_POSIX_H_ */
