/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 * Simulation toranj config variant with IP6 runtime external address pool
 * enabled, used for the ip6-ext-addr-pool toranj tests.
 */

#ifndef OT_TORANJ_OPENTHREAD_CORE_TORANJ_CONFIG_IP6_POOL_TEST_H_
#define OT_TORANJ_OPENTHREAD_CORE_TORANJ_CONFIG_IP6_POOL_TEST_H_

/* Pull in the standard simulation toranj config first. */
#include "openthread-core-toranj-config-simulation.h"

/* Override the pool flag that the simulation config sets to 0. */
#undef OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE
#define OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE 1

/* Enable the CLI 'ifconfig init' and 'thread childtableinit' commands (testing-only). */
#define OPENTHREAD_CONFIG_CLI_IFCONFIG_INIT_ENABLE 1

#endif /* OT_TORANJ_OPENTHREAD_CORE_TORANJ_CONFIG_IP6_POOL_TEST_H_ */
