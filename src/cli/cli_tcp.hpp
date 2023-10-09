/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file contains definitions for a TCP CLI tool.
 */

#ifndef CLI_TCP_EXAMPLE_HPP_
#define CLI_TCP_EXAMPLE_HPP_

#include "openthread-core-config.h"

#include <openthread/tcp.h>
#include <openthread/tcp_ext.h>

#if OPENTHREAD_CONFIG_TLS_ENABLE

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#endif

#include "cli/cli_config.h"
#include "cli/cli_output.hpp"
#include "common/time.hpp"

namespace ot {
namespace Cli {

/**
 * Implements a CLI-based TCP example.
 *
 */
class TcpExample : private Output
{
public:
    using Arg = Utils::CmdLineParser::Arg;

    /**
     * Constructor
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     *
     */
    TcpExample(otInstance *aInstance, OutputImplementer &aOutputImplementer);

    /**
     * Processes a CLI sub-command.
     *
     * @param[in]  aArgs     An array of command line arguments.
     *
     * @retval OT_ERROR_NONE              Successfully executed the CLI command.
     * @retval OT_ERROR_PENDING           The CLI command was successfully started but final result is pending.
     * @retval OT_ERROR_INVALID_COMMAND   Invalid or unknown CLI command.
     * @retval OT_ERROR_INVALID_ARGS      Invalid arguments.
     * @retval ...                        Error during execution of the CLI command.
     *
     */
    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<TcpExample>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ContinueBenchmarkCircularSend(void);
    void    CompleteBenchmark(void);

#if OPENTHREAD_CONFIG_TLS_ENABLE
    void PrepareTlsHandshake(void);
    bool ContinueTlsHandshake(void);
#endif

    static void HandleTcpEstablishedCallback(otTcpEndpoint *aEndpoint);
    static void HandleTcpSendDoneCallback(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData);
    static void HandleTcpForwardProgressCallback(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog);
    static void HandleTcpReceiveAvailableCallback(otTcpEndpoint *aEndpoint,
                                                  size_t         aBytesAvailable,
                                                  bool           aEndOfStream,
                                                  size_t         aBytesRemaining);
    static void HandleTcpDisconnectedCallback(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason);
    static otTcpIncomingConnectionAction HandleTcpAcceptReadyCallback(otTcpListener    *aListener,
                                                                      const otSockAddr *aPeer,
                                                                      otTcpEndpoint   **aAcceptInto);
    static void                          HandleTcpAcceptDoneCallback(otTcpListener    *aListener,
                                                                     otTcpEndpoint    *aEndpoint,
                                                                     const otSockAddr *aPeer);

    void HandleTcpEstablished(otTcpEndpoint *aEndpoint);
    void HandleTcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData);
    void HandleTcpForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog);
    void HandleTcpReceiveAvailable(otTcpEndpoint *aEndpoint,
                                   size_t         aBytesAvailable,
                                   bool           aEndOfStream,
                                   size_t         aBytesRemaining);
    void HandleTcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason);
    otTcpIncomingConnectionAction HandleTcpAcceptReady(otTcpListener    *aListener,
                                                       const otSockAddr *aPeer,
                                                       otTcpEndpoint   **aAcceptInto);
    void HandleTcpAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer);

#if OPENTHREAD_CONFIG_TLS_ENABLE
    static void MbedTlsDebugOutput(void *ctx, int level, const char *file, int line, const char *str);
#endif

    void OutputBenchmarkResult(void);

    otTcpEndpoint mEndpoint;
    otTcpListener mListener;

    bool mInitialized;
    bool mEndpointConnected;
    bool mEndpointConnectedFastOpen;
    bool mSendBusy;
    bool mUseCircularSendBuffer;
    bool mUseTls;
    bool mTlsHandshakeComplete;

    otTcpCircularSendBuffer mSendBuffer;
    otLinkedBuffer          mSendLink;
    uint8_t                 mSendBufferBytes[OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE];
    uint8_t                 mReceiveBufferBytes[OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE];

    otLinkedBuffer
              mBenchmarkLinks[(sizeof(mReceiveBufferBytes) + sizeof(mSendBufferBytes) - 1) / sizeof(mSendBufferBytes)];
    uint32_t  mBenchmarkBytesTotal;
    uint32_t  mBenchmarkBytesUnsent;
    TimeMilli mBenchmarkStart;
    uint32_t  mBenchmarkTimeUsed;
    uint32_t  mBenchmarkLastBytesTotal;
    otTcpEndpointAndCircularSendBuffer mEndpointAndCircularSendBuffer;

#if OPENTHREAD_CONFIG_TLS_ENABLE
    mbedtls_ssl_context     mSslContext;
    mbedtls_ssl_config      mSslConfig;
    mbedtls_x509_crt        mSrvCert;
    mbedtls_pk_context      mPKey;
    mbedtls_entropy_context mEntropy;
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

    static constexpr const char *sBenchmarkData =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    static constexpr const size_t sBenchmarkDataLength = 1040;

#if OPENTHREAD_CONFIG_TLS_ENABLE
    static constexpr const char  *sCasPem       = "-----BEGIN CERTIFICATE-----\r\n"
                                                  "MIIBtDCCATqgAwIBAgIBTTAKBggqhkjOPQQDAjBLMQswCQYDVQQGEwJOTDERMA8G\r\n"
                                                  "A1UEChMIUG9sYXJTU0wxKTAnBgNVBAMTIFBvbGFyU1NMIFRlc3QgSW50ZXJtZWRp\r\n"
                                                  "YXRlIEVDIENBMB4XDTE1MDkwMTE0MDg0M1oXDTI1MDgyOTE0MDg0M1owSjELMAkG\r\n"
                                                  "A1UEBhMCVUsxETAPBgNVBAoTCG1iZWQgVExTMSgwJgYDVQQDEx9tYmVkIFRMUyBU\r\n"
                                                  "ZXN0IGludGVybWVkaWF0ZSBDQSAzMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE\r\n"
                                                  "732fWHLNPMPsP1U1ibXvb55erlEVMlpXBGsj+KYwVqU1XCmW9Z9hhP7X/5js/DX9\r\n"
                                                  "2J/utoHyjUtVpQOzdTrbsaMQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNo\r\n"
                                                  "ADBlAjAJRxbGRas3NBmk9MnGWXg7PT1xnRELHRWWIvfLdVQt06l1/xFg3ZuPdQdt\r\n"
                                                  "Qh7CK80CMQD7wa1o1a8qyDKBfLN636uKmKGga0E+vYXBeFCy9oARBangGCB0B2vt\r\n"
                                                  "pz590JvGWfM=\r\n"
                                                  "-----END CERTIFICATE-----\r\n";
    static constexpr const size_t sCasPemLength = 665; // includes NUL byte

    static constexpr const char  *sSrvPem       = "-----BEGIN CERTIFICATE-----\r\n"
                                                  "MIICHzCCAaWgAwIBAgIBCTAKBggqhkjOPQQDAjA+MQswCQYDVQQGEwJOTDERMA8G\r\n"
                                                  "A1UEChMIUG9sYXJTU0wxHDAaBgNVBAMTE1BvbGFyc3NsIFRlc3QgRUMgQ0EwHhcN\r\n"
                                                  "MTMwOTI0MTU1MjA0WhcNMjMwOTIyMTU1MjA0WjA0MQswCQYDVQQGEwJOTDERMA8G\r\n"
                                                  "A1UEChMIUG9sYXJTU0wxEjAQBgNVBAMTCWxvY2FsaG9zdDBZMBMGByqGSM49AgEG\r\n"
                                                  "CCqGSM49AwEHA0IABDfMVtl2CR5acj7HWS3/IG7ufPkGkXTQrRS192giWWKSTuUA\r\n"
                                                  "2CMR/+ov0jRdXRa9iojCa3cNVc2KKg76Aci07f+jgZ0wgZowCQYDVR0TBAIwADAd\r\n"
                                                  "BgNVHQ4EFgQUUGGlj9QH2deCAQzlZX+MY0anE74wbgYDVR0jBGcwZYAUnW0gJEkB\r\n"
                                                  "PyvLeLUZvH4kydv7NnyhQqRAMD4xCzAJBgNVBAYTAk5MMREwDwYDVQQKEwhQb2xh\r\n"
                                                  "clNTTDEcMBoGA1UEAxMTUG9sYXJzc2wgVGVzdCBFQyBDQYIJAMFD4n5iQ8zoMAoG\r\n"
                                                  "CCqGSM49BAMCA2gAMGUCMQCaLFzXptui5WQN8LlO3ddh1hMxx6tzgLvT03MTVK2S\r\n"
                                                  "C12r0Lz3ri/moSEpNZWqPjkCMCE2f53GXcYLqyfyJR078c/xNSUU5+Xxl7VZ414V\r\n"
                                                  "fGa5kHvHARBPc8YAIVIqDvHH1Q==\r\n"
                                                  "-----END CERTIFICATE-----\r\n";
    static constexpr const size_t sSrvPemLength = 813; // includes NUL byte

    static constexpr const char  *sSrvKey       = "-----BEGIN EC PRIVATE KEY-----\r\n"
                                                  "MHcCAQEEIPEqEyB2AnCoPL/9U/YDHvdqXYbIogTywwyp6/UfDw6noAoGCCqGSM49\r\n"
                                                  "AwEHoUQDQgAEN8xW2XYJHlpyPsdZLf8gbu58+QaRdNCtFLX3aCJZYpJO5QDYIxH/\r\n"
                                                  "6i/SNF1dFr2KiMJrdw1VzYoqDvoByLTt/w==\r\n"
                                                  "-----END EC PRIVATE KEY-----\r\n";
    static constexpr const size_t sSrvKeyLength = 233; // includes NUL byte

    static constexpr const char  *sEcjpakePassword       = "TLS-over-TCPlp";
    static constexpr const size_t sEcjpakePasswordLength = 14;
    static const int              sCipherSuites[];
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
};

} // namespace Cli
} // namespace ot

#endif // CLI_TCP_EXAMPLE_HPP_
