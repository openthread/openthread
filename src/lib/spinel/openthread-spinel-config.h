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

/**
 * @file
 *   This file includes compile-time configuration constants for OpenThread.
 */

#ifndef OPENTHREAD_SPINEL_CONFIG_H_
#define OPENTHREAD_SPINEL_CONFIG_H_

#include "openthread-core-config.h"

/**
 * @def OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE
 *
 * Define 1 to enable feeding an OpenThread message to encoder/decoder.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE
#define OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT
 *
 * Defines the max count of RCP failures allowed to be recovered.
 * 0 means to disable RCP failure recovering.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT
#define OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT 0
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_ABORT_ON_UNEXPECTED_RCP_RESET_ENABLE
 *
 * Define 1 to abort the host when receiving unexpected reset from RCP.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_ABORT_ON_UNEXPECTED_RCP_RESET_ENABLE
#define OPENTHREAD_SPINEL_CONFIG_ABORT_ON_UNEXPECTED_RCP_RESET_ENABLE 0
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_RCP_TIME_SYNC_INTERVAL
 *
 * This setting configures the interval (in units of microseconds) for host-rcp
 * time sync. The host will recalculate the time offset between host and RCP
 * every interval.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_RCP_TIME_SYNC_INTERVAL
#define OPENTHREAD_SPINEL_CONFIG_RCP_TIME_SYNC_INTERVAL (60 * 1000 * 1000)
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_BROADCAST_IID
 *
 * Define broadcast IID for spinel frames dedicated to all hosts in multipan configuration.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_BROADCAST_IID
#define OPENTHREAD_SPINEL_CONFIG_BROADCAST_IID SPINEL_HEADER_IID_3
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE
 *
 * Enables compilation of vendor specific code for Spinel
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE
#define OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE 0
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER
 *
 * Header file defining class VendorRadioSpinel
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER
#define OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER "lib/spinel/example_vendor_hook.hpp"
#endif

/**
 * @def OPENTHREAD_SPINEL_CONFIG_RCP_TX_WAIT_TIME_SECS
 *
 * Defines the Tx wait duration in seconds.
 *
 */
#ifndef OPENTHREAD_SPINEL_CONFIG_RCP_TX_WAIT_TIME_SECS
#define OPENTHREAD_SPINEL_CONFIG_RCP_TX_WAIT_TIME_SECS 5
#endif

#endif // OPENTHREAD_SPINEL_CONFIG_H_
