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
 *   This file includes compile-time configurations for the CLI service.
 *
 */

#ifndef CONFIG_CLI_H_
#define CONFIG_CLI_H_

#include "openthread-core-config.h"

#ifndef OPENTHREAD_POSIX
#if defined(__ANDROID__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__linux__) || defined(__NetBSD__) || \
    defined(__unix__)
#define OPENTHREAD_POSIX 1
#else
#define OPENTHREAD_POSIX 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 * The maximum size of the CLI line in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH 384
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_SERVICES
 *
 * The maximum number of service entries supported by SRP client.
 *
 * This is only applicable when SRP client is enabled, i.e. OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_SERVICES
#define OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_SERVICES 2
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_HOST_ADDRESSES
 *
 * The maximum number of host IPv6 address entries supported by SRP client.
 *
 * This is only applicable when SRP client is enabled, i.e. OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_HOST_ADDRESSES
#define OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_HOST_ADDRESSES 2
#endif

#endif // CONFIG_CLI_H_
