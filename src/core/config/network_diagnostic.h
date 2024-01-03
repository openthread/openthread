/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the Network Diagnostics.
 *
 */

#ifndef CONFIG_NETWORK_DIAGNOSTIC_H_
#define CONFIG_NETWORK_DIAGNOSTIC_H_

/**
 * @addtogroup config-network-diagnostic
 *
 * @brief
 *   This module includes configuration variables for Network Diagnostics.
 *
 * @{
 *
 */

/**
 * @def OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME
 *
 * Specifies the default Vendor Name string.
 *
 */
#ifndef OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME
#define OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME ""
#endif

/**
 * @def OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL
 *
 * Specifies the default Vendor Model string.
 *
 */
#ifndef OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL
#define OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL ""
#endif

/**
 * @def OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION
 *
 * Specifies the default Vendor SW Version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION
#define OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION ""
#endif

/**
 * @def OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
 *
 * Define as 1 to add APIs to allow Vendor Name, Model, SW Version to change at run-time.
 *
 * It is recommended that Vendor Name, Model, and SW Version are set at build time using the OpenThread configurations
 * `OPENTHREAD_CONFIG_NET_DIAG_VENDOR_*`. This way they are treated as constants and won't consume RAM.
 *
 * However, for situations where the OpenThread stack is integrated as a library into different projects/products, this
 * config can be used to add API to change Vendor Name, Model, and SW Version at run-time. In this case, the strings in
 * `OPENTHREAD_CONFIG_NET_DIAG_VENDOR_*` are treated as the default values (used when OT stack is initialized).
 *
 */
#ifndef OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
#define OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE 0
#endif

/**
 * @}
 *
 */

#endif // CONFIG_NETWORK_DIAGNOSTIC_H_
