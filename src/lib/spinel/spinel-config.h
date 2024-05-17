/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes compile-time configuration constants for lib spinel.
 */

#ifndef SPINEL_CONFIG_H_
#define SPINEL_CONFIG_H_

/**
 * @def OPENTHREAD_LIB_SPINEL_RX_FRAME_BUFFER_SIZE
 *
 * Specifies the rx frame buffer size used by `SpinelInterface` in RCP host (posix) code. This is applicable/used when
 * `RadioSpinel` platform is used.
 *
 */
#ifndef OPENTHREAD_LIB_SPINEL_RX_FRAME_BUFFER_SIZE
#define OPENTHREAD_LIB_SPINEL_RX_FRAME_BUFFER_SIZE 8192
#endif

/**
 * @def OPENTHREAD_LIB_SPINEL_LOG_MAX_SIZE
 *
 * The maximum log string size (number of chars).
 *
 */
#ifndef OPENTHREAD_LIB_SPINEL_LOG_MAX_SIZE
#define OPENTHREAD_LIB_SPINEL_LOG_MAX_SIZE 1024
#endif

/**
 * @def OPENTHREAD_LIB_SPINEL_NCP_LOG_MAX_SIZE
 *
 * The maximum OpenThread log string size (number of chars) supported by NCP using Spinel `StreamWrite`.
 *
 */
#ifndef OPENTHREAD_LIB_SPINEL_NCP_LOG_MAX_SIZE
#define OPENTHREAD_LIB_SPINEL_NCP_LOG_MAX_SIZE 150
#endif

#endif // SPINEL_CONFIG_H_
