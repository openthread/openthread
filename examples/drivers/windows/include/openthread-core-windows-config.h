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
 *   This file includes Windows compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_WINDOWS_CONFIG_H_
#define OPENTHREAD_CORE_WINDOWS_CONFIG_H_

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS               2048

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#define OPENTHREAD_CONFIG_MAX_CHILDREN                      32

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The minimum number of supported IPv6 address registrations per child.
 *
 */
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                6

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner entries maintained by the Commissioner.
 *
 */
#define OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES                16

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
 *
 * The message pool is managed by platform defined logic when this flag is set.
 * This feature would typically be used when operating in a multi-threaded system
 * and multiple threads need to access the message pool.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT       1

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level.
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL                         OT_LOG_LEVEL_DEBG

 /**
 * @def OPENTHREAD_CONFIG_LOG_PKT_DUMP
 *
 * Define to enable log content of packets.
 *
 */
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP                      0

#endif  // OPENTHREAD_CORE_WINDOWS_CONFIG_H_

