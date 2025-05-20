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
 *   This file includes compile-time configurations for Border Agent.
 */

#ifndef CONFIG_BORDER_AGENT_H_
#define CONFIG_BORDER_AGENT_H_

/**
 * @addtogroup config-border-agent
 *
 * @brief
 *   This module includes configuration variables for Border Agent.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
 *
 * Define to 1 to enable Border Agent support.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT
 *
 * Specifies the Border Agent UDP port, and use 0 for ephemeral port.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT
#define OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
 *
 * Define to 1 to enable Border Agent ID support.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
 *
 * Define to 1 to enable ephemeral key mechanism and its APIs in Border Agent.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_4)
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_FEATURE_ENABLED_BY_DEFAULT
 *
 * Whether or not the ephemeral key feature is enabled by default at run-time.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_FEATURE_ENABLED_BY_DEFAULT
#define OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_FEATURE_ENABLED_BY_DEFAULT \
    OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
 *
 * Define to 1 to enable Border Agent to manage registering/updating of the mDNS MeshCoP service(s) on the
 * infrastructure link
 *
 * This includes the ephemeral key service when the `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE \
    (OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE)
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME
 *
 * Specifies the base name to construct the service instance name used when advertising the mDNS `_meshcop._udp`
 * service by the Border Agent.
 *
 * Applicable when the `OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE` feature is enabled.
 *
 * The name can also be configured using the `otBorderAgentSetMeshCoPServiceBaseName()` API at run-time.
 *
 * Per the Thread specification, the service instance should be a user-friendly name identifying the device model or
 * product. A recommended format is "VendorName ProductName".
 *
 * The name MUST have a length less than or equal to `OT_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME_MAX_LENGTH` (47 chars).
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME
#define OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME "OpenThread BR (unspecified vendor) "
#endif

/**
 * @}
 */

#endif // CONFIG_BORDER_AGENT_H_
