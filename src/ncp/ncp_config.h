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
 *   This file includes compile-time configurations for the NCP (Network Co-Processor).
 *
 * @note
 *   All configuration macros in this file follow the pattern:
 *   - Wrapped in `#ifndef` guards so they can be overridden by a platform-specific config.
 *   - Default to the safest/most minimal value (usually 0 or a conservative buffer size).
 */

#ifndef OT_NCP_NCP_CONFIG_H_
#define OT_NCP_NCP_CONFIG_H_

#ifndef OPENTHREAD_RADIO
#define OPENTHREAD_RADIO 0
#endif

/*---------------------------------------------------------------------------*/
/* Transport Layer                                                            */
/*---------------------------------------------------------------------------*/

/**
 * @def OPENTHREAD_CONFIG_NCP_SPI_ENABLE
 *
 * Define to 1 to enable NCP SPI transport support.
 *
 * SPI is recommended for high-throughput or low-latency host-NCP communication.
 * Only one transport (SPI or HDLC) should be enabled at a time.
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPI_ENABLE
#define OPENTHREAD_CONFIG_NCP_SPI_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_HDLC_ENABLE
 *
 * Define to 1 to enable NCP HDLC (UART) transport support.
 *
 * HDLC over UART is commonly used for simpler host-NCP setups.
 * Only one transport (SPI or HDLC) should be enabled at a time.
 */
#ifndef OPENTHREAD_CONFIG_NCP_HDLC_ENABLE
#define OPENTHREAD_CONFIG_NCP_HDLC_ENABLE 0
#endif

/* Sanity check: both transports should not be enabled simultaneously */
#if OPENTHREAD_CONFIG_NCP_SPI_ENABLE && OPENTHREAD_CONFIG_NCP_HDLC_ENABLE
#error "OPENTHREAD_CONFIG_NCP_SPI_ENABLE and OPENTHREAD_CONFIG_NCP_HDLC_ENABLE cannot both be enabled."
#endif

/*---------------------------------------------------------------------------*/
/* Buffer Sizes                                                               */
/*---------------------------------------------------------------------------*/

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 * The size of the NCP outgoing message buffer in bytes.
 *
 * Larger values improve throughput for high-frequency transmissions but consume
 * more RAM. Reduce for memory-constrained devices.
 */
#ifndef OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE 2048
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_HDLC_TX_CHUNK_SIZE
 *
 * The size of each HDLC TX chunk in bytes sent to the host per write cycle.
 *
 * Tune this value based on the UART DMA buffer size or host read granularity.
 */
#ifndef OPENTHREAD_CONFIG_NCP_HDLC_TX_CHUNK_SIZE
#define OPENTHREAD_CONFIG_NCP_HDLC_TX_CHUNK_SIZE 2048
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_HDLC_RX_BUFFER_SIZE
 *
 * The size of the NCP HDLC receive buffer in bytes.
 *
 * Radio-only builds use a smaller buffer (512 bytes) since they handle
 * fewer and simpler Spinel frames. Full NCP builds default to 1300 bytes
 * to accommodate larger payloads such as IPv6 packets.
 */
#ifndef OPENTHREAD_CONFIG_NCP_HDLC_RX_BUFFER_SIZE
#if OPENTHREAD_RADIO
#define OPENTHREAD_CONFIG_NCP_HDLC_RX_BUFFER_SIZE 512
#else
#define OPENTHREAD_CONFIG_NCP_HDLC_RX_BUFFER_SIZE 1300
#endif
#endif // OPENTHREAD_CONFIG_NCP_HDLC_RX_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
 *
 * The size of the NCP SPI RX and TX buffer in bytes.
 *
 * This buffer is shared between SPI receive and transmit paths.
 * Radio-only builds default to 512 bytes; full NCP builds default to 1300 bytes.
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
#if OPENTHREAD_RADIO
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE 512
#else
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE 1300
#endif
#endif // OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE

/*---------------------------------------------------------------------------*/
/* Spinel Protocol                                                            */
/*---------------------------------------------------------------------------*/

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
 *
 * Extra bytes to reserve in the UART/SPI buffer for use by the NCP Spinel
 * Encrypter (e.g., for an authentication tag or IV appended to frames).
 *
 * Set to 0 if `OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER` is disabled.
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE
 *
 * The maximum length (in characters) of an OpenThread log string that can
 * be forwarded to the host via Spinel `StreamWrite`.
 *
 * Log strings longer than this value will be truncated before transmission.
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE 150
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE
 *
 * The maximum number of simultaneous Spinel command responses the NCP can queue.
 *
 * The Spinel protocol supports Transaction IDs (TID) from 1 to 15 (TID 0 is
 * reserved for fire-and-forget frames requiring no response), giving a protocol
 * maximum of 15 concurrent commands. Host drivers may use fewer (e.g., wpantund
 * limits to 2). Lower this value to reduce RAM usage on constrained devices.
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE 15
#endif

/*---------------------------------------------------------------------------*/
/* Features                                                                  */
/*---------------------------------------------------------------------------*/

/**
 * @def OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
 *
 * Define to 1 to enable peek/poke (arbitrary memory read/write) support on NCP.
 *
 * This allows the host to read from and write to arbitrary memory addresses on
 * the NCP device via Spinel. Intended strictly for debugging and development.
 *
 * @warning Do NOT enable in production firmware. This feature exposes raw memory
 *          access and can compromise device security and stability.
 */
#ifndef OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
#define OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL
 *
 * Define to 1 to enable host control over the NCP MCU's power state.
 *
 * The power state determines how the NCP MCU behaves when the OS is idle,
 * and whether the host needs to send an external "poke" signal before
 * communicating with the NCP.
 *
 * When enabled, the platform must implement:
 *   - `otPlatSetMcuPowerState()`
 *   - `otPlatGetMcuPowerState()`
 * (see `openthread/platform/misc.h`)
 *
 * The host can then control the MCU power state via `SPINEL_PROP_MCU_POWER_STATE`.
 */
#ifndef OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL
#define OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE
 *
 * Define to 1 to enable the NCP-based implementation of platform InfraIf APIs.
 *
 * When enabled, the NCP provides infrastructure network interface support,
 * allowing the Thread Border Router to route traffic through the NCP's
 * infrastructure (e.g., Wi-Fi or Ethernet) interface.
 */
#ifndef OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE
#define OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE
 *
 * Define to 1 to enable the NCP-based implementation of platform DNS-SD APIs.
 *
 * When enabled, the NCP handles DNS Service Discovery operations on behalf
 * of the host, supporting service registration and browsing via Spinel.
 */
#ifndef OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE
#define OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_CLI_STREAM_ENABLE
 *
 * Define to 1 to enable the NCP CLI Stream feature.
 *
 * When enabled, the NCP supports sending and receiving CLI input/output
 * over a dedicated Spinel stream (`SPINEL_PROP_STREAM_CLI`). This allows
 * the host to interact with the OpenThread CLI running on the NCP without
 * requiring a separate UART or debug port.
 */
#ifndef OPENTHREAD_CONFIG_NCP_CLI_STREAM_ENABLE
#define OPENTHREAD_CONFIG_NCP_CLI_STREAM_ENABLE 0
#endif

/**
 * @def OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
 *
 * Define to 1 to enable vendor-specific NCP hook support.
 *
 * When enabled, the NCP will invoke vendor-defined handler functions at key
 * points in the Spinel command processing pipeline, allowing vendors to extend
 * or customize NCP behavior without modifying the core OpenThread source code.
 *
 * The vendor is expected to provide implementations of:
 *   - `NcpBase::VendorCommandHandler()` — handles vendor-defined Spinel commands.
 *   - `NcpBase::VendorHandleFrameRemovedFromNcpBuffer()` — called when a frame
 *     is removed from the NCP TX buffer, useful for flow control or logging.
 *
 * Typical use cases include adding proprietary Spinel properties, handling
 * vendor-defined commands, or injecting vendor metadata into NCP responses.
 */
#ifndef OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
#define OPENTHREAD_ENABLE_NCP_VENDOR_HOOK 0
#endif

/**
 * @def OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
 *
 * Define to 1 to enable Spinel frame encryption between the host and NCP.
 *
 * When enabled, all Spinel frames are passed through a user-provided encrypter
 * before transmission and decrypted upon reception. This can be used to secure
 * the host-NCP communication channel on untrusted physical interfaces.
 *
 * Requires `OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE` to be set
 * to the size needed by the encrypter for its overhead (e.g., IV + auth tag).
 */
#ifndef OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
#define OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER 0
#endif

/*---------------------------------------------------------------------------*/
/* New: Diagnostics & Observability                                           */
/*---------------------------------------------------------------------------*/

/**
 * @def OPENTHREAD_CONFIG_NCP_ENABLE_FRAME_STATS
 *
 * Define to 1 to enable NCP frame statistics tracking.
 *
 * When enabled, the NCP tracks per-direction counters for frames sent,
 * received, dropped, and errored. These counters can be exposed to the host
 * via a vendor Spinel property or through the CLI stream.
 *
 * Useful for diagnosing transport reliability issues in production deployments.
 */
#ifndef OPENTHREAD_CONFIG_NCP_ENABLE_FRAME_STATS
#define OPENTHREAD_CONFIG_NCP_ENABLE_FRAME_STATS 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_WATCHDOG_TIMEOUT_MS
 *
 * Timeout in milliseconds for the NCP host-communication watchdog.
 *
 * If the NCP does not receive any valid Spinel frame from the host within
 * this period, it may trigger a platform-specific recovery action (e.g.,
 * reset or alert). Set to 0 to disable the watchdog.
 *
 * Default: 0 (disabled).
 */
#ifndef OPENTHREAD_CONFIG_NCP_WATCHDOG_TIMEOUT_MS
#define OPENTHREAD_CONFIG_NCP_WATCHDOG_TIMEOUT_MS 0
#endif

#endif // OT_NCP_NCP_CONFIG_H_