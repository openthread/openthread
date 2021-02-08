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
 *   This file includes compile-time configurations for the SRP (Service Registration Protocol) Server.
 *
 */

#ifndef CONFIG_SRP_SERVER_H_
#define CONFIG_SRP_SERVER_H_

/**
 * @def OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
 *
 * Define to 1 to enable SRP Server support.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
#define OPENTHREAD_CONFIG_SRP_SERVER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_NUMBER
 *
 * Specifies the Thread Network Data Service number for SRP Server.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_NUMBER
#define OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_NUMBER 0x5du
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_UPDATE_TIMEOUT
 *
 * Specifies the timeout value (in milliseconds) for the service update handler.
 *
 * The default timeout value is the sum of the maximum total mDNS probing delays
 * and a loose IPC timeout of 250ms. It is recommended that this configuration should
 * not use a value smaller than the default value here, if an Advertising Proxy is used
 * to handle the service update events.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_UPDATE_TIMEOUT
#define OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_UPDATE_TIMEOUT ((4 * 250u) + 250u)
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_SERVER_MAX_ADDRESSES_NUM
 *
 * Specifies the maximum number of addresses the SRP server can handle for a host.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_SERVER_MAX_ADDRESSES_NUM
#define OPENTHREAD_CONFIG_SRP_SERVER_MAX_ADDRESSES_NUM 2
#endif

#endif // CONFIG_SRP_SERVER_H_
