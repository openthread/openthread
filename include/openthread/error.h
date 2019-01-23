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
 *  This file defines the errors used in the OpenThread.
 */

#ifndef OPENTHREAD_ERROR_H_
#define OPENTHREAD_ERROR_H_

#include <openthread/platform/toolchain.h>

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
 * @defgroup api-error Error
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
 * @defgroup api-dns   DNSv6
 * @defgroup api-icmp6 ICMPv6
 * @defgroup api-ip6   IPv6
 * @defgroup api-udp-group   UDP
 *
 * @{
 *
 * @defgroup api-udp         UDP
 * @defgroup api-udp-forward UDP Forward
 *
 * @}
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
 * @defgroup api-border-router  Border Router
 * @defgroup api-commissioner   Commissioner
 * @defgroup api-thread-general General
 * @brief This module includes functions for all Thread roles.
 * @defgroup api-joiner         Joiner
 * @defgroup api-thread-router  Router/Leader
 * @brief This module includes functions for Thread Routers and Leaders.
 * @defgroup api-server         Server
 *
 * @}
 *
 * @defgroup api-addons Add-Ons
 *
 * @{
 *
 * @defgroup api-channel-manager     Channel Manager
 * @defgroup api-channel-monitor     Channel Monitoring
 * @defgroup api-child-supervision   Child Supervision
 * @defgroup api-coap-group          CoAP
 *
 * @{
 *
 * @defgroup api-coap                CoAP
 * @defgroup api-coap-secure         CoAP Secure
 *
 * @}
 *
 * @defgroup api-cli                 Command Line Interface
 * @defgroup api-crypto              Crypto
 * @defgroup api-factory-diagnostics Factory Diagnostics
 * @defgroup api-jam-detection       Jam Detection
 * @defgroup api-logging             Logging
 * @defgroup api-ncp                 Network Co-Processor
 * @defgroup api-network-time        Network Time Synchronization
 * @defgroup api-sntp                SNTP
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
 * @defgroup plat-ble                 BLE Host
 * @defgroup plat-factory-diagnostics Factory Diagnostics
 * @defgroup plat-logging             Logging
 * @defgroup plat-memory              Memory
 * @defgroup plat-messagepool         Message Pool
 * @defgroup plat-misc                Miscellaneous
 * @defgroup plat-radio               Radio
 * @defgroup plat-random              Random
 * @defgroup plat-settings            Settings
 * @defgroup plat-spi-slave           SPI Slave
 * @defgroup plat-time                Time Service
 * @defgroup plat-toolchain           Toolchain
 * @defgroup plat-uart                UART
 *
 * @}
 *
 */

/**
 * @addtogroup api-error
 *
 * @brief
 *   This module includes error definitions used in OpenThread.
 *
 * @{
 *
 */

/**
 * This enumeration represents error codes used throughout OpenThread.
 *
 */
typedef enum otError
{
    /**
     * No error.
     */
    OT_ERROR_NONE = 0,

    /**
     * Operational failed.
     */
    OT_ERROR_FAILED = 1,

    /**
     * Message was dropped.
     */
    OT_ERROR_DROP = 2,

    /**
     * Insufficient buffers.
     */
    OT_ERROR_NO_BUFS = 3,

    /**
     * No route available.
     */
    OT_ERROR_NO_ROUTE = 4,

    /**
     * Service is busy and could not service the operation.
     */
    OT_ERROR_BUSY = 5,

    /**
     * Failed to parse message or arguments.
     */
    OT_ERROR_PARSE = 6,

    /**
     * Input arguments are invalid.
     */
    OT_ERROR_INVALID_ARGS = 7,

    /**
     * Security checks failed.
     */
    OT_ERROR_SECURITY = 8,

    /**
     * Address resolution requires an address query operation.
     */
    OT_ERROR_ADDRESS_QUERY = 9,

    /**
     * Address is not in the source match table.
     */
    OT_ERROR_NO_ADDRESS = 10,

    /**
     * Operation was aborted.
     */
    OT_ERROR_ABORT = 11,

    /**
     * Function or method is not implemented.
     */
    OT_ERROR_NOT_IMPLEMENTED = 12,

    /**
     * Cannot complete due to invalid state.
     */
    OT_ERROR_INVALID_STATE = 13,

    /**
     * No acknowledgment was received after macMaxFrameRetries (IEEE 802.15.4-2006).
     */
    OT_ERROR_NO_ACK = 14,

    /**
     * A transmission could not take place due to activity on the channel, i.e., the CSMA-CA mechanism has failed
     * (IEEE 802.15.4-2006).
     */
    OT_ERROR_CHANNEL_ACCESS_FAILURE = 15,

    /**
     * Not currently attached to a Thread Partition.
     */
    OT_ERROR_DETACHED = 16,

    /**
     * FCS check failure while receiving.
     */
    OT_ERROR_FCS = 17,

    /**
     * No frame received.
     */
    OT_ERROR_NO_FRAME_RECEIVED = 18,

    /**
     * Received a frame from an unknown neighbor.
     */
    OT_ERROR_UNKNOWN_NEIGHBOR = 19,

    /**
     * Received a frame from an invalid source address.
     */
    OT_ERROR_INVALID_SOURCE_ADDRESS = 20,

    /**
     * Received a frame filtered by the address filter (whitelisted or blacklisted).
     */
    OT_ERROR_ADDRESS_FILTERED = 21,

    /**
     * Received a frame filtered by the destination address check.
     */
    OT_ERROR_DESTINATION_ADDRESS_FILTERED = 22,

    /**
     * The requested item could not be found.
     */
    OT_ERROR_NOT_FOUND = 23,

    /**
     * The operation is already in progress.
     */
    OT_ERROR_ALREADY = 24,

    /**
     * The creation of IPv6 address failed.
     */
    OT_ERROR_IP6_ADDRESS_CREATION_FAILURE = 26,

    /**
     * Operation prevented by mode flags
     */
    OT_ERROR_NOT_CAPABLE = 27,

    /**
     * Coap response or acknowledgment or DNS, SNTP response not received.
     */
    OT_ERROR_RESPONSE_TIMEOUT = 28,

    /**
     * Received a duplicated frame.
     */
    OT_ERROR_DUPLICATED = 29,

    /**
     * Message is being dropped from reassembly list due to timeout.
     */
    OT_ERROR_REASSEMBLY_TIMEOUT = 30,

    /**
     * Message is not a TMF Message.
     */
    OT_ERROR_NOT_TMF = 31,

    /**
     * Received a non-lowpan data frame.
     */
    OT_ERROR_NOT_LOWPAN_DATA_FRAME = 32,

    /**
     * A feature/functionality disabled by build-time configuration options.
     */
    OT_ERROR_DISABLED_FEATURE = 33,

    /**
     * The link margin was too low.
     */
    OT_ERROR_LINK_MARGIN_LOW = 34,

    /**
     * Generic error (should not use).
     */
    OT_ERROR_GENERIC = 255,
} otError;

/**
 * This function converts an otError enum into a string.
 *
 * @param[in]  aError     An otError enum.
 *
 * @returns  A string representation of an otError.
 *
 */
OTAPI const char *OTCALL otThreadErrorToString(otError aError);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_ERROR_H_
