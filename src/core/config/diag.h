/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the DIAG service.
 */

#ifndef OT_CORE_CONFIG_DIAG_H_
#define OT_CORE_CONFIG_DIAG_H_

/**
 * @addtogroup config-diag
 *
 * @brief
 *   This module includes configuration variables for the DIAG service.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_DIAG_ENABLE
 *
 * Define to 1 to enable Factory Diagnostics support.
 */
#ifndef OPENTHREAD_CONFIG_DIAG_ENABLE
#define OPENTHREAD_CONFIG_DIAG_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode output buffer size in bytes
 */
#ifndef OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 256
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
 *
 * Define OpenThread diagnostic mode max command line arguments.
 */
#ifndef OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX 32
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode command line buffer size in bytes.
 */
#ifndef OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE 256
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_TX_DONE_TIMEOUT
 *
 * Define the maximum time in milliseconds that `diag stop` and `diag repeat stop` wait for an in-flight frame
 * transmission to complete before aborting the frame and finishing the command anyway.
 *
 * The radio driver may never report the completion of an in-flight frame, e.g., when the frame requests an ACK that
 * is never received and the radio does not implement an ACK timeout, or when the RCP stops responding and RCP
 * restoration is disabled. This timeout ensures these commands always complete.
 *
 * The default value is longer than the default `OPENTHREAD_SPINEL_CONFIG_RCP_TX_WAIT_TIME_SECS` so that, in RCP mode,
 * the host gets the chance to detect an RCP transmit timeout and recover before this timeout fires.
 */
#ifndef OPENTHREAD_CONFIG_DIAG_TX_DONE_TIMEOUT
#define OPENTHREAD_CONFIG_DIAG_TX_DONE_TIMEOUT 6000
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_DIAG_H_
