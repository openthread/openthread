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
 * @brief
 *  This file defines the top-level functions for the OpenThread library.
 */

#ifndef OPENTHREAD_H_
#define OPENTHREAD_H_

#include <openthread/crypto.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/netdata.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup api  API
 * @brief
 *   This module includes the application programming interface to the OpenThread stack.
 *
 * @{
 *
 * @defgroup api-execution Execution
 *
 * @{
 *
 * @defgroup api-instance Instance
 * @defgroup api-tasklets Tasklets
 *
 * @}
 *
 * @defgroup api-net IPv6 Networking
 * @{
 *
 * @defgroup api-dhcp6 DHCPv6
 * @brief This module includes functions for DHCPv6 Client and Server.
 * @defgroup api-dns   DNSv6
 * @defgroup api-icmp6 ICMPv6
 * @defgroup api-ip6   IPv6
 * @defgroup api-udp   UDP
 *
 * @}
 *
 * @defgroup api-link Link
 *
 * @{
 *
 * @defgroup api-link-link Link
 * @defgroup api-link-raw  Raw Link
 *
 * @}
 *
 * @defgroup api-message Message
 *
 * @defgroup api-thread Thread
 *
 * @{
 *
 * @defgroup api-border-agent   Border Agent
 * @defgroup api-commissioner   Commissioner
 * @defgroup api-thread-general General
 * @brief This module includes functions for all Thread roles.
 * @defgroup api-joiner         Joiner
 * @defgroup api-thread-router  Router/Leader
 * @brief This module includes functions for Thread Routers and Leaders.
 *
 * @}
 *
 * @defgroup api-addons Add-Ons
 *
 * @{
 *
 * @defgroup api-child-supervision   Child Supervision
 * @defgroup api-coap                CoAP
 * @defgroup api-cli                 Command Line Interface
 * @defgroup api-crypto              Crypto
 * @defgroup api-factory-diagnostics Factory Diagnostics
 * @defgroup api-jam-detection       Jam Detection
 * @defgroup api-ncp                 Network Co-Processor
 *
 * @}
 *
 * @}
 *
 */

/**
 * @defgroup platform  Platform Abstraction
 * @brief
 *   This module includes the platform abstraction used by the OpenThread stack.
 *
 * @{
 *
 * @defgroup plat-alarm               Alarm
 * @defgroup plat-factory-diagnostics Factory Diagnostics
 * @defgroup plat-logging             Logging
 * @defgroup plat-memory              Memory
 * @defgroup plat-messagepool         Message Pool
 * @defgroup plat-misc                Miscellaneous
 * @defgroup plat-radio               Radio
 * @defgroup plat-random              Random
 * @defgroup plat-settings            Settings
 * @defgroup plat-spi-slave           SPI Slave
 * @defgroup plat-toolchain           Toolchain
 * @defgroup plat-uart                UART
 *
 * @}
 *
 */

/**
 * Get the OpenThread version string.
 *
 * @returns A pointer to the OpenThread version.
 *
 */
OTAPI const char *OTCALL otGetVersionString(void);

/**
 * This function converts an otError enum into a string.
 *
 * @param[in]  aError     An otError enum.
 *
 * @returns  A string representation of an otError.
 *
 */
OTAPI const char *OTCALL otThreadErrorToString(otError aError);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_H_
