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
 *   This file includes default compile-time configuration constants
 *   for Thread 1.2 in OpenThread's implementation.
 */

#ifndef OPENTHREAD_THREAD_V1_2_CORE_DEFAULT_CONFIG_H_
#define OPENTHREAD_THREAD_V1_2_CORE_DEFAULT_CONFIG_H_

#if OPENTHREAD_THREAD_VERSION >= OPENTHREAD_THREAD_VERSION_1_2

/**
 * @def OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE
 *
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE
#define OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVER_ENABLE
 *
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_RECEIVER_ENABLE
#define OPENTHREAD_CONFIG_CSL_RECEIVER_ENABLE 1
#endif

#if OPENTHREAD_CONFIG_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CSL_SYNCHRONIZED_TIMEOUT
 *
 * The default CSL sample duration in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CSL_SYNCHRONIZED_TIMEOUT
#define OPENTHREAD_CONFIG_DEFAULT_CSL_SYNCHRONIZED_TIMEOUT 100
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW
 *
 * The CSL sample window in 10 symbols.
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW
#define OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW 5
#endif

/**

 * @def OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 1
#endif

#endif // OPENTHREAD_CONFIG_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE
#endif // OPENTHREAD_THREAD_VERSION >= OPENTHREAD_THREAD_VERSION_1_2

#endif // OPENTHREAD_THREAD_V1_2_CORE_DEFAULT_CONFIG_H_
