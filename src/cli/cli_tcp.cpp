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
 *   This file implements a TCP CLI tool.
 */

#include "openthread-core-config.h"

#include "cli_config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE

#include "cli_tcp.hpp"

#include <openthread/nat64.h>
#include <openthread/tcp.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"
#include "common/timer.hpp"

#if OPENTHREAD_CONFIG_TLS_ENABLE
#include <mbedtls/debug.h>
#include <mbedtls/ecjpake.h>
#include "crypto/mbedtls.hpp"
#endif

namespace ot {
namespace Cli {

#if OPENTHREAD_CONFIG_TLS_ENABLE
const int TcpExample::sCipherSuites[] = {MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8,
                                         MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8, 0};
#endif

TcpExample::TcpExample(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Output(aInstance, aOutputImplementer)
    , mInitialized(false)
    , mEndpointConnected(false)
    , mEndpointConnectedFastOpen(false)
    , mSendBusy(false)
    , mUseCircularSendBuffer(true)
    , mUseTls(false)
    , mTlsHandshakeComplete(false)
    , mBenchmarkBytesTotal(0)
    , mBenchmarkBytesUnsent(0)
    , mBenchmarkTimeUsed(0)
{
    mEndpointAndCircularSendBuffer.mEndpoint   = &mEndpoint;
    mEndpointAndCircularSendBuffer.mSendBuffer = &mSendBuffer;
}

#if OPENTHREAD_CONFIG_TLS_ENABLE
void TcpExample::MbedTlsDebugOutput(void *ctx, int level, const char *file, int line, const char *str)
{
    TcpExample &tcpExample = *static_cast<TcpExample *>(ctx);
    tcpExample.OutputLine("%s:%d:%d: %s", file, line, level, str);
}
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

/**
 * @cli tcp init
 * @code
 * tcp init tls
 * Done
 * @endcode
 * @cparam tcp init [@ca{mode}] [@ca{size}]
 * * The `mode` has three possible values:
 *   * `tls`: Specifies that the TCP connection between two nodes should also
 *     use the TLS protocol on top of TCP. When two nodes communicate over TCP,
 *     both nodes must either use TLS or neither node should use TLS because
 *     a non-TLS endpoint cannot communicate with a TLS endpoint.
 *   * `linked` or `circular`: Either one of these options means that TLS
 *     is not to be used, and the specified buffering type should be used for TCP
 *     buffering. The behavior of `linked` and `circular` is identical. Examine the code
 *     for the differences between these two buffering types.
 *     Two endpoints of a TCP connection are not required to use the same buffering type.
 * * The `size` parameter sets the size of the receive buffer to associate with the
 *   example TCP endpoint. If left unspecified, the maximum size is used. The
 *   maximum size is set in `OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE`.
 * @par
 * Initializes the example TCP listener and the example TCP endpoint provided
 * by the `tcp` CLI.
 * @sa otTcpListenerInitialize
 * @sa otTcpEndpointInitialize
 */
template <> otError TcpExample::Process<Cmd("init")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    size_t  receiveBufferSize;

    VerifyOrExit(!mInitialized, error = OT_ERROR_ALREADY);

    if (aArgs[0].IsEmpty())
    {
        mUseCircularSendBuffer = true;
        mUseTls                = false;
        receiveBufferSize      = sizeof(mReceiveBufferBytes);
    }
    else
    {
        if (aArgs[0] == "linked")
        {
            mUseCircularSendBuffer = false;
            mUseTls                = false;
        }
        else if (aArgs[0] == "circular")
        {
            mUseCircularSendBuffer = true;
            mUseTls                = false;
        }
#if OPENTHREAD_CONFIG_TLS_ENABLE
        else if (aArgs[0] == "tls")
        {
            mUseCircularSendBuffer = true;
            mUseTls                = true;

            // mbedtls_debug_set_threshold(0);

            otPlatCryptoRandomInit();
            mbedtls_x509_crt_init(&mSrvCert);
            mbedtls_pk_init(&mPKey);

            mbedtls_ssl_init(&mSslContext);
            mbedtls_ssl_config_init(&mSslConfig);
            mbedtls_ssl_conf_rng(&mSslConfig, Crypto::MbedTls::CryptoSecurePrng, nullptr);
            // mbedtls_ssl_conf_dbg(&mSslConfig, MbedTlsDebugOutput, this);
            mbedtls_ssl_conf_authmode(&mSslConfig, MBEDTLS_SSL_VERIFY_NONE);
            mbedtls_ssl_conf_ciphersuites(&mSslConfig, sCipherSuites);

#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
            mbedtls_ssl_conf_min_tls_version(&mSslConfig, MBEDTLS_SSL_VERSION_TLS1_2);
            mbedtls_ssl_conf_max_tls_version(&mSslConfig, MBEDTLS_SSL_VERSION_TLS1_2);
#else
            mbedtls_ssl_conf_min_version(&mSslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
            mbedtls_ssl_conf_max_version(&mSslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
#endif

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
#include "crypto/mbedtls.hpp"
            int rv = mbedtls_pk_parse_key(&mPKey, reinterpret_cast<const unsigned char *>(sSrvKey), sSrvKeyLength,
                                          nullptr, 0, Crypto::MbedTls::CryptoSecurePrng, nullptr);
#else
            int rv = mbedtls_pk_parse_key(&mPKey, reinterpret_cast<const unsigned char *>(sSrvKey), sSrvKeyLength,
                                          nullptr, 0);
#endif
            if (rv != 0)
            {
                OutputLine("mbedtls_pk_parse_key returned %d", rv);
            }

            rv = mbedtls_x509_crt_parse(&mSrvCert, reinterpret_cast<const unsigned char *>(sSrvPem), sSrvPemLength);
            if (rv != 0)
            {
                OutputLine("mbedtls_x509_crt_parse (1) returned %d", rv);
            }
            rv = mbedtls_x509_crt_parse(&mSrvCert, reinterpret_cast<const unsigned char *>(sCasPem), sCasPemLength);
            if (rv != 0)
            {
                OutputLine("mbedtls_x509_crt_parse (2) returned %d", rv);
            }
            rv = mbedtls_ssl_setup(&mSslContext, &mSslConfig);
            if (rv != 0)
            {
                OutputLine("mbedtls_ssl_setup returned %d", rv);
            }
        }
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if (aArgs[1].IsEmpty())
        {
            receiveBufferSize = sizeof(mReceiveBufferBytes);
        }
        else
        {
            uint32_t windowSize;

            SuccessOrExit(error = aArgs[1].ParseAsUint32(windowSize));

            receiveBufferSize = windowSize + ((windowSize + 7) >> 3);
            VerifyOrExit(receiveBufferSize <= sizeof(mReceiveBufferBytes) && receiveBufferSize != 0,
                         error = OT_ERROR_INVALID_ARGS);
        }
    }

    otTcpCircularSendBufferInitialize(&mSendBuffer, mSendBufferBytes, sizeof(mSendBufferBytes));

    {
        otTcpEndpointInitializeArgs endpointArgs;

        memset(&endpointArgs, 0x00, sizeof(endpointArgs));
        endpointArgs.mEstablishedCallback = HandleTcpEstablishedCallback;
        if (mUseCircularSendBuffer)
        {
            endpointArgs.mForwardProgressCallback = HandleTcpForwardProgressCallback;
        }
        else
        {
            endpointArgs.mSendDoneCallback = HandleTcpSendDoneCallback;
        }
        endpointArgs.mReceiveAvailableCallback = HandleTcpReceiveAvailableCallback;
        endpointArgs.mDisconnectedCallback     = HandleTcpDisconnectedCallback;
        endpointArgs.mContext                  = this;
        endpointArgs.mReceiveBuffer            = mReceiveBufferBytes;
        endpointArgs.mReceiveBufferSize        = receiveBufferSize;

        SuccessOrExit(error = otTcpEndpointInitialize(GetInstancePtr(), &mEndpoint, &endpointArgs));
    }

    {
        otTcpListenerInitializeArgs listenerArgs;

        memset(&listenerArgs, 0x00, sizeof(listenerArgs));
        listenerArgs.mAcceptReadyCallback = HandleTcpAcceptReadyCallback;
        listenerArgs.mAcceptDoneCallback  = HandleTcpAcceptDoneCallback;
        listenerArgs.mContext             = this;

        error = otTcpListenerInitialize(GetInstancePtr(), &mListener, &listenerArgs);
        if (error != OT_ERROR_NONE)
        {
            IgnoreReturnValue(otTcpEndpointDeinitialize(&mEndpoint));
            ExitNow();
        }
    }

    mInitialized = true;

exit:
    return error;
}

/**
 * @cli tcp deinit
 * @code
 * tcp deinit
 * Done
 * @endcode
 * @par api_copy
 * #otTcpEndpointDeinitialize
 */
template <> otError TcpExample::Process<Cmd("deinit")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    otError endpointError;
    otError bufferError;
    otError listenerError;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls)
    {
        otPlatCryptoRandomDeinit();
        mbedtls_ssl_config_free(&mSslConfig);
        mbedtls_ssl_free(&mSslContext);

        mbedtls_pk_free(&mPKey);
        mbedtls_x509_crt_free(&mSrvCert);
    }
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

    endpointError = otTcpEndpointDeinitialize(&mEndpoint);
    mSendBusy     = false;

    otTcpCircularSendBufferForceDiscardAll(&mSendBuffer);
    bufferError = otTcpCircularSendBufferDeinitialize(&mSendBuffer);

    listenerError = otTcpListenerDeinitialize(&mListener);
    mInitialized  = false;

    SuccessOrExit(error = endpointError);
    SuccessOrExit(error = bufferError);
    SuccessOrExit(error = listenerError);

exit:
    return error;
}

/**
 * @cli tcp bind
 * @code
 * tcp bind :: 30000
 * Done
 * @endcode
 * @cparam tcp bind @ca{ip} @ca{port}
 * * `ip`: IPv6 address to bind to. If you wish to have the TCP/IPv6 stack assign
 *   the binding IPv6 address, use the unspecified IPv6 address: `::`.
 * * `port`: TCP port number to bind to.
 * @par
 * Associates an IPv6 address and a port to the example TCP endpoint provided by
 * the `tcp` CLI. Associating the TCP endpoint to an IPv6
 * address and port is referred to as "naming the TCP endpoint." This binds the
 * endpoint for communication.
 * @sa otTcpBind
 * @sa @tcp
 */
template <> otError TcpExample::Process<Cmd("bind")>(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;

    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(sockaddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otTcpBind(&mEndpoint, &sockaddr);

exit:
    return error;
}

/**
 * @cli tcp connect
 * @code
 * tcp connect fe80:0:0:0:a8df:580a:860:ffa4 30000
 * Done
 * TCP: Connection established
 * @endcode
 * @code
 * tcp connect 172.17.0.1 1234
 * Connecting to synthesized IPv6 address: fdde:ad00:beef:2:0:0:ac11:1
 * Done
 * @endcode
 * @cparam tcp connect @ca{ip} @ca{port} [@ca{fastopen}]
 * * `ip`: IP address of the peer The address can be an IPv4 address,
 *   which gets synthesized to an IPv6 address using the preferred
 *   NAT64 prefix from the network data. The command returns `InvalidState`
 *   when the preferred NAT64 prefix is unavailable.
 * * `port`: TCP port number of the peer.
 * * `fastopen`: This parameter is optional. If set to `fast`, TCP Fast Open is enabled
 *   for this connection. Otherwise, if this parameter is set to `slow` or not used,
 *   TCP Fast Open is disabled.
 * @par
 * Establishes a connection with the specified peer.
 * @par
 * If the connection establishment is successful, the resulting TCP connection
 * is associated with the example TCP endpoint.
 * @sa otTcpConnect
 * @sa @tcp
 */
template <> otError TcpExample::Process<Cmd("connect")>(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;
    bool       nat64SynthesizedAddress;
    uint32_t   flags;

    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(
        error = Interpreter::ParseToIp6Address(GetInstancePtr(), aArgs[0], sockaddr.mAddress, nat64SynthesizedAddress));
    if (nat64SynthesizedAddress)
    {
        OutputFormat("Connecting to synthesized IPv6 address: ");
        OutputIp6AddressLine(sockaddr.mAddress);
    }

    SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    if (aArgs[2].IsEmpty())
    {
        flags = OT_TCP_CONNECT_NO_FAST_OPEN;
    }
    else
    {
        if (aArgs[2] == "slow")
        {
            flags = OT_TCP_CONNECT_NO_FAST_OPEN;
        }
        else if (aArgs[2] == "fast")
        {
            flags = 0;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
        VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    }

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls)
    {
        int rv = mbedtls_ssl_config_defaults(&mSslConfig, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                             MBEDTLS_SSL_PRESET_DEFAULT);
        if (rv != 0)
        {
            OutputLine("mbedtls_ssl_config_defaults returned %d", rv);
        }
    }
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

    SuccessOrExit(error = otTcpConnect(&mEndpoint, &sockaddr, flags));
    mEndpointConnected         = true;
    mEndpointConnectedFastOpen = ((flags & OT_TCP_CONNECT_NO_FAST_OPEN) == 0);

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls && mEndpointConnectedFastOpen)
    {
        PrepareTlsHandshake();
        ContinueTlsHandshake();
    }
#endif

exit:
    return error;
}

/**
 * @cli tcp send
 * @code
 * tcp send hello
 * Done
 * @endcode
 * @cparam tcp send @ca{message}
 * The `message` parameter contains the message you want to send to the
 * remote TCP endpoint.
 * @par
 * Sends data over the TCP connection associated with the example TCP endpoint
 * that is provided with the `tcp` CLI.
 * @sa @tcp
 */
template <> otError TcpExample::Process<Cmd("send")>(Arg aArgs[])
{
    otError error;

    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mBenchmarkBytesTotal == 0, error = OT_ERROR_BUSY);
    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    if (mUseCircularSendBuffer)
    {
#if OPENTHREAD_CONFIG_TLS_ENABLE
        if (mUseTls)
        {
            int rv = mbedtls_ssl_write(&mSslContext, reinterpret_cast<unsigned char *>(aArgs[0].GetCString()),
                                       aArgs[0].GetLength());
            if (rv < 0 && rv != MBEDTLS_ERR_SSL_WANT_WRITE && rv != MBEDTLS_ERR_SSL_WANT_READ)
            {
                ExitNow(error = kErrorFailed);
            }
            error = kErrorNone;
        }
        else
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
        {
            size_t written;
            SuccessOrExit(error = otTcpCircularSendBufferWrite(&mEndpoint, &mSendBuffer, aArgs[0].GetCString(),
                                                               aArgs[0].GetLength(), &written, 0));
        }
    }
    else
    {
        VerifyOrExit(!mSendBusy, error = OT_ERROR_BUSY);

        mSendLink.mNext   = nullptr;
        mSendLink.mData   = mSendBufferBytes;
        mSendLink.mLength = OT_MIN(aArgs[0].GetLength(), sizeof(mSendBufferBytes));
        memcpy(mSendBufferBytes, aArgs[0].GetCString(), mSendLink.mLength);

        SuccessOrExit(error = otTcpSendByReference(&mEndpoint, &mSendLink, 0));
        mSendBusy = true;
    }

exit:
    return error;
}

template <> otError TcpExample::Process<Cmd("benchmark")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli tcp benchmark result
     * @code
     * tcp benchmark result
     * TCP Benchmark Status: Ongoing
     * Done
     * @endcode
     * @code
     * tcp benchmark result
     * TCP Benchmark Status: Completed
     * TCP Benchmark Complete: Transferred 73728 bytes in 7056 milliseconds
     * TCP Goodput: 83.592 kb/s
     * @endcode
     * @par
     * Shows the latest result of the TCP benchmark test. Possible status values:
     * * Ongoing
     * * Completed
     * * Untested
     * @par
     * This command is primarily intended for creating scripts that automate
     * the TCP benchmark test.
     */
    if (aArgs[0] == "result")
    {
        OutputFormat("TCP Benchmark Status: ");
        if (mBenchmarkBytesTotal != 0)
        {
            OutputLine("Ongoing");
        }
        else if (mBenchmarkTimeUsed != 0)
        {
            OutputLine("Completed");
            OutputBenchmarkResult();
        }
        else
        {
            OutputLine("Untested");
        }
    }
    /**
     * @cli tcp benchmark run
     * @code
     * tcp benchmark run
     * Done
     * TCP Benchmark Complete: Transferred 73728 bytes in 7233 milliseconds
     * TCP Goodput: 81.546 kb/s
     * @endcode
     * @cparam tcp benchmark run [@ca{size}]
     * Use the `size` parameter to specify the number of bytes to send
     * for the benchmark. If you do not use the `size` parameter, the default
     * value (`OPENTHREAD_CONFIG_CLI_TCP_DEFAULT_BENCHMARK_SIZE`) is used.
     * @par
     * Transfers the specified number of bytes using the TCP connection
     * currently associated with the example TCP endpoint provided by the `tcp` CLI.
     * @note You must establish a TCP connection before you run this command.
     */
    else if (aArgs[0] == "run")
    {
        VerifyOrExit(!mSendBusy, error = OT_ERROR_BUSY);
        VerifyOrExit(mBenchmarkBytesTotal == 0, error = OT_ERROR_BUSY);

        if (aArgs[1].IsEmpty())
        {
            mBenchmarkBytesTotal = OPENTHREAD_CONFIG_CLI_TCP_DEFAULT_BENCHMARK_SIZE;
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsUint32(mBenchmarkBytesTotal));
            VerifyOrExit(mBenchmarkBytesTotal != 0, error = OT_ERROR_INVALID_ARGS);
        }
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        mBenchmarkStart       = TimerMilli::GetNow();
        mBenchmarkBytesUnsent = mBenchmarkBytesTotal;

        if (mUseCircularSendBuffer)
        {
            SuccessOrExit(error = ContinueBenchmarkCircularSend());
        }
        else
        {
            uint32_t benchmarkLinksLeft =
                (mBenchmarkBytesTotal + sizeof(mSendBufferBytes) - 1) / sizeof(mSendBufferBytes);
            uint32_t toSendOut = OT_MIN(OT_ARRAY_LENGTH(mBenchmarkLinks), benchmarkLinksLeft);

            /* We could also point the linked buffers directly to sBenchmarkData. */
            memset(mSendBufferBytes, 'a', sizeof(mSendBufferBytes));

            for (uint32_t i = 0; i != toSendOut; i++)
            {
                mBenchmarkLinks[i].mNext   = nullptr;
                mBenchmarkLinks[i].mData   = mSendBufferBytes;
                mBenchmarkLinks[i].mLength = sizeof(mSendBufferBytes);
                if (i == 0 && mBenchmarkBytesTotal % sizeof(mSendBufferBytes) != 0)
                {
                    mBenchmarkLinks[i].mLength = mBenchmarkBytesTotal % sizeof(mSendBufferBytes);
                }
                error = otTcpSendByReference(&mEndpoint, &mBenchmarkLinks[i],
                                             i == toSendOut - 1 ? 0 : OT_TCP_SEND_MORE_TO_COME);
                VerifyOrExit(error == OT_ERROR_NONE, mBenchmarkBytesTotal = 0);
            }
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

/**
 * @cli tcp sendend
 * @code
 * tcp sendend
 * Done
 * @endcode
 * @par
 * Sends the "end of stream" signal over the TCP connection
 * associated with the example TCP endpoint provided by the `tcp` CLI. This
 * alerts the peer that it will not receive any more data over this TCP connection.
 * @sa otTcpSendEndOfStream
 */
template <> otError TcpExample::Process<Cmd("sendend")>(Arg aArgs[])
{
    otError error;

    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otTcpSendEndOfStream(&mEndpoint);

exit:
    return error;
}

/**
 * @cli tcp abort
 * @code
 * tcp abort
 * TCP: Connection reset
 * Done
 * @endcode
 * @par
 * Unceremoniously ends the TCP connection associated with the
 * example TCP endpoint, transitioning the TCP endpoint to the closed state.
 * @sa otTcpAbort
 */
template <> otError TcpExample::Process<Cmd("abort")>(Arg aArgs[])
{
    otError error;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = otTcpAbort(&mEndpoint));
    mEndpointConnected         = false;
    mEndpointConnectedFastOpen = false;

exit:
    return error;
}

/**
 * @cli tcp listen
 * @code
 * tcp listen :: 30000
 * Done
 * @endcode
 * @cparam tcp listen @ca{ip} @ca{port}
 * The following parameters are required:
 * * `ip`: IPv6 address or the unspecified IPv6 address (`::`) of the example
 *   TCP listener provided by the `tcp` CLI.
 * * `port`: TCP port of the example TCP listener.
 *   If no TCP connection is associated with the example TCP endpoint, then any
 *   incoming connections matching the specified IPv6 address and port are accepted
 *   and are associated with the example TCP endpoint.
 * @par
 * Uses the example TCP listener to listen for incoming connections on the
 * specified IPv6 address and port.
 * @sa otTcpListen
 * @sa @tcp
 */
template <> otError TcpExample::Process<Cmd("listen")>(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;

    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(sockaddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otTcpStopListening(&mListener));
    error = otTcpListen(&mListener, &sockaddr);

exit:
    return error;
}

/**
 * @cli tcp stoplistening
 * @code
 * tcp stoplistening
 * Done
 * @endcode
 * @par
 * Instructs the example TCP listener to stop listening for incoming TCP connections.
 * @sa otTcpStopListening
 */
template <> otError TcpExample::Process<Cmd("stoplistening")>(Arg aArgs[])
{
    otError error;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mInitialized, error = OT_ERROR_INVALID_STATE);

    error = otTcpStopListening(&mListener);

exit:
    return error;
}

otError TcpExample::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                  \
    {                                                             \
        aCommandString, &TcpExample::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("abort"), CmdEntry("benchmark"), CmdEntry("bind"), CmdEntry("connect"), CmdEntry("deinit"),
        CmdEntry("init"),  CmdEntry("listen"),    CmdEntry("send"), CmdEntry("sendend"), CmdEntry("stoplistening"),
    };

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void TcpExample::HandleTcpEstablishedCallback(otTcpEndpoint *aEndpoint)
{
    static_cast<TcpExample *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpEstablished(aEndpoint);
}

void TcpExample::HandleTcpSendDoneCallback(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
{
    static_cast<TcpExample *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpSendDone(aEndpoint, aData);
}

void TcpExample::HandleTcpForwardProgressCallback(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog)
{
    static_cast<TcpExample *>(otTcpEndpointGetContext(aEndpoint))
        ->HandleTcpForwardProgress(aEndpoint, aInSendBuffer, aBacklog);
}

void TcpExample::HandleTcpReceiveAvailableCallback(otTcpEndpoint *aEndpoint,
                                                   size_t         aBytesAvailable,
                                                   bool           aEndOfStream,
                                                   size_t         aBytesRemaining)
{
    static_cast<TcpExample *>(otTcpEndpointGetContext(aEndpoint))
        ->HandleTcpReceiveAvailable(aEndpoint, aBytesAvailable, aEndOfStream, aBytesRemaining);
}

void TcpExample::HandleTcpDisconnectedCallback(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    static_cast<TcpExample *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpDisconnected(aEndpoint, aReason);
}

otTcpIncomingConnectionAction TcpExample::HandleTcpAcceptReadyCallback(otTcpListener    *aListener,
                                                                       const otSockAddr *aPeer,
                                                                       otTcpEndpoint   **aAcceptInto)
{
    return static_cast<TcpExample *>(otTcpListenerGetContext(aListener))
        ->HandleTcpAcceptReady(aListener, aPeer, aAcceptInto);
}

void TcpExample::HandleTcpAcceptDoneCallback(otTcpListener    *aListener,
                                             otTcpEndpoint    *aEndpoint,
                                             const otSockAddr *aPeer)
{
    static_cast<TcpExample *>(otTcpListenerGetContext(aListener))->HandleTcpAcceptDone(aListener, aEndpoint, aPeer);
}

void TcpExample::HandleTcpEstablished(otTcpEndpoint *aEndpoint)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OutputLine("TCP: Connection established");
#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls && !mEndpointConnectedFastOpen)
    {
        PrepareTlsHandshake();
        ContinueTlsHandshake();
    }
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
}

void TcpExample::HandleTcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_ASSERT(!mUseCircularSendBuffer); // this callback is not used when using the circular send buffer

    if (mBenchmarkBytesTotal == 0)
    {
        // If the benchmark encountered an error, we might end up here. So,
        // tolerate some benchmark links finishing in this case.
        if (aData == &mSendLink)
        {
            OT_ASSERT(mSendBusy);
            mSendBusy = false;
        }
    }
    else
    {
        OT_ASSERT(aData != &mSendLink);
        OT_ASSERT(mBenchmarkBytesUnsent >= aData->mLength);
        mBenchmarkBytesUnsent -= aData->mLength; // could be less than sizeof(mSendBufferBytes) for the first link
        if (mBenchmarkBytesUnsent >= OT_ARRAY_LENGTH(mBenchmarkLinks) * sizeof(mSendBufferBytes))
        {
            aData->mLength = sizeof(mSendBufferBytes);
            if (otTcpSendByReference(&mEndpoint, aData, 0) != OT_ERROR_NONE)
            {
                OutputLine("TCP Benchmark Failed");
                mBenchmarkBytesTotal = 0;
            }
        }
        else if (mBenchmarkBytesUnsent == 0)
        {
            CompleteBenchmark();
        }
    }
}

void TcpExample::HandleTcpForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_UNUSED_VARIABLE(aBacklog);
    OT_ASSERT(mUseCircularSendBuffer); // this callback is only used when using the circular send buffer

    otTcpCircularSendBufferHandleForwardProgress(&mSendBuffer, aInSendBuffer);

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls)
    {
        ContinueTlsHandshake();
    }
#endif

    /* Handle case where we're in a benchmark. */
    if (mBenchmarkBytesTotal != 0)
    {
        if (mBenchmarkBytesUnsent != 0)
        {
            /* Continue sending out data if there's data we haven't sent. */
            IgnoreError(ContinueBenchmarkCircularSend());
        }
        else if (aInSendBuffer == 0)
        {
            /* Handle case where all data is sent out and the send buffer has drained. */
            CompleteBenchmark();
        }
    }
}

void TcpExample::HandleTcpReceiveAvailable(otTcpEndpoint *aEndpoint,
                                           size_t         aBytesAvailable,
                                           bool           aEndOfStream,
                                           size_t         aBytesRemaining)
{
    OT_UNUSED_VARIABLE(aBytesRemaining);
    OT_ASSERT(aEndpoint == &mEndpoint);

    /* If we get data before the handshake completes, then this is a TFO connection. */
    if (!mEndpointConnected)
    {
        mEndpointConnected         = true;
        mEndpointConnectedFastOpen = true;

#if OPENTHREAD_CONFIG_TLS_ENABLE
        if (mUseTls)
        {
            PrepareTlsHandshake();
        }
#endif
    }

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls && ContinueTlsHandshake())
    {
        return;
    }
#endif

    if ((mTlsHandshakeComplete || !mUseTls) && aBytesAvailable > 0)
    {
#if OPENTHREAD_CONFIG_TLS_ENABLE
        if (mUseTls)
        {
            uint8_t buffer[500];
            for (;;)
            {
                int rv = mbedtls_ssl_read(&mSslContext, buffer, sizeof(buffer));
                if (rv < 0)
                {
                    if (rv == MBEDTLS_ERR_SSL_WANT_READ)
                    {
                        break;
                    }
                    OutputLine("TLS receive failure: %d", rv);
                }
                else
                {
                    OutputLine("TLS: Received %u bytes: %.*s", static_cast<unsigned>(rv), rv,
                               reinterpret_cast<const char *>(buffer));
                }
            }
            OutputLine("(TCP: Received %u bytes)", static_cast<unsigned>(aBytesAvailable));
        }
        else
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
        {
            const otLinkedBuffer *data;
            size_t                totalReceived = 0;
            IgnoreError(otTcpReceiveByReference(aEndpoint, &data));
            for (; data != nullptr; data = data->mNext)
            {
                OutputLine("TCP: Received %u bytes: %.*s", static_cast<unsigned>(data->mLength),
                           static_cast<unsigned>(data->mLength), reinterpret_cast<const char *>(data->mData));
                totalReceived += data->mLength;
            }
            OT_ASSERT(aBytesAvailable == totalReceived);
            IgnoreReturnValue(otTcpCommitReceive(aEndpoint, totalReceived, 0));
        }
    }

    if (aEndOfStream)
    {
        OutputLine("TCP: Reached end of stream");
    }
}

void TcpExample::HandleTcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    static const char *const kReasonStrings[] = {
        "Disconnected",            // (0) OT_TCP_DISCONNECTED_REASON_NORMAL
        "Connection refused",      // (1) OT_TCP_DISCONNECTED_REASON_REFUSED
        "Connection reset",        // (2) OT_TCP_DISCONNECTED_REASON_RESET
        "Entered TIME-WAIT state", // (3) OT_TCP_DISCONNECTED_REASON_TIME_WAIT
        "Connection timed out",    // (4) OT_TCP_DISCONNECTED_REASON_TIMED_OUT
    };

    OT_UNUSED_VARIABLE(aEndpoint);

    static_assert(0 == OT_TCP_DISCONNECTED_REASON_NORMAL, "OT_TCP_DISCONNECTED_REASON_NORMAL value is incorrect");
    static_assert(1 == OT_TCP_DISCONNECTED_REASON_REFUSED, "OT_TCP_DISCONNECTED_REASON_REFUSED value is incorrect");
    static_assert(2 == OT_TCP_DISCONNECTED_REASON_RESET, "OT_TCP_DISCONNECTED_REASON_RESET value is incorrect");
    static_assert(3 == OT_TCP_DISCONNECTED_REASON_TIME_WAIT, "OT_TCP_DISCONNECTED_REASON_TIME_WAIT value is incorrect");
    static_assert(4 == OT_TCP_DISCONNECTED_REASON_TIMED_OUT, "OT_TCP_DISCONNECTED_REASON_TIMED_OUT value is incorrect");

    OutputLine("TCP: %s", Stringify(aReason, kReasonStrings));

#if OPENTHREAD_CONFIG_TLS_ENABLE
    if (mUseTls)
    {
        mbedtls_ssl_session_reset(&mSslContext);
    }
#endif

    // We set this to false even for the TIME-WAIT state, so that we can reuse
    // the active socket if an incoming connection comes in instead of waiting
    // for the 2MSL timeout.
    mEndpointConnected         = false;
    mEndpointConnectedFastOpen = false;
    mSendBusy                  = false;

    // Mark the benchmark as inactive if the connection was disconnected.
    mBenchmarkBytesTotal  = 0;
    mBenchmarkBytesUnsent = 0;

    otTcpCircularSendBufferForceDiscardAll(&mSendBuffer);
}

otTcpIncomingConnectionAction TcpExample::HandleTcpAcceptReady(otTcpListener    *aListener,
                                                               const otSockAddr *aPeer,
                                                               otTcpEndpoint   **aAcceptInto)
{
    otTcpIncomingConnectionAction action;

    OT_UNUSED_VARIABLE(aListener);

    if (mEndpointConnected)
    {
        OutputFormat("TCP: Ignoring incoming connection request from ");
        OutputSockAddr(*aPeer);
        OutputLine(" (active socket is busy)");

        ExitNow(action = OT_TCP_INCOMING_CONNECTION_ACTION_DEFER);
    }

    *aAcceptInto = &mEndpoint;
    action       = OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;

#if OPENTHREAD_CONFIG_TLS_ENABLE
    /*
     * Natural to wait until the AcceptDone callback but with TFO we could get data before that
     * so it doesn't make sense to wait until then.
     */
    if (mUseTls)
    {
        int rv;

        rv = mbedtls_ssl_config_defaults(&mSslConfig, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT);
        if (rv != 0)
        {
            OutputLine("mbedtls_ssl_config_defaults returned %d", rv);
        }
        mbedtls_ssl_conf_ca_chain(&mSslConfig, mSrvCert.next, nullptr);
        rv = mbedtls_ssl_conf_own_cert(&mSslConfig, &mSrvCert, &mPKey);
        if (rv != 0)
        {
            OutputLine("mbedtls_ssl_conf_own_cert returned %d", rv);
        }
    }
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

exit:
    return action;
}

void TcpExample::HandleTcpAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer)
{
    OT_UNUSED_VARIABLE(aListener);
    OT_UNUSED_VARIABLE(aEndpoint);

    mEndpointConnected = true;
    OutputFormat("Accepted connection from ");
    OutputSockAddrLine(*aPeer);
}

otError TcpExample::ContinueBenchmarkCircularSend(void)
{
    otError error = OT_ERROR_NONE;
    size_t  freeSpace;

    while (mBenchmarkBytesUnsent != 0 && (freeSpace = otTcpCircularSendBufferGetFreeSpace(&mSendBuffer)) != 0)
    {
        size_t   toSendThisIteration = OT_MIN(mBenchmarkBytesUnsent, sBenchmarkDataLength);
        uint32_t flag                = (toSendThisIteration < freeSpace && toSendThisIteration < mBenchmarkBytesUnsent)
                                           ? OT_TCP_CIRCULAR_SEND_BUFFER_WRITE_MORE_TO_COME
                                           : 0;
        size_t   written             = 0;

#if OPENTHREAD_CONFIG_TLS_ENABLE
        if (mUseTls)
        {
            int rv = mbedtls_ssl_write(&mSslContext, reinterpret_cast<const unsigned char *>(sBenchmarkData),
                                       toSendThisIteration);
            if (rv > 0)
            {
                written = static_cast<size_t>(rv);
                OT_ASSERT(written <= mBenchmarkBytesUnsent);
            }
            else if (rv != MBEDTLS_ERR_SSL_WANT_WRITE && rv != MBEDTLS_ERR_SSL_WANT_READ)
            {
                ExitNow(error = kErrorFailed);
            }
            error = kErrorNone;
        }
        else
#endif // OPENTHREAD_CONFIG_TLS_ENABLE
        {
            SuccessOrExit(error = otTcpCircularSendBufferWrite(&mEndpoint, &mSendBuffer, sBenchmarkData,
                                                               toSendThisIteration, &written, flag));
        }
        mBenchmarkBytesUnsent -= written;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        OutputLine("TCP Benchmark Failed");
        mBenchmarkBytesTotal  = 0;
        mBenchmarkBytesUnsent = 0;
    }

    return error;
}

void TcpExample::OutputBenchmarkResult(void)
{
    uint32_t thousandTimesGoodput =
        (1000 * (mBenchmarkLastBytesTotal << 3) + (mBenchmarkTimeUsed >> 1)) / mBenchmarkTimeUsed;

    OutputLine("TCP Benchmark Complete: Transferred %lu bytes in %lu milliseconds", ToUlong(mBenchmarkLastBytesTotal),
               ToUlong(mBenchmarkTimeUsed));
    OutputLine("TCP Goodput: %lu.%03u kb/s", ToUlong(thousandTimesGoodput / 1000),
               static_cast<uint16_t>(thousandTimesGoodput % 1000));
}

void TcpExample::CompleteBenchmark(void)
{
    mBenchmarkTimeUsed       = TimerMilli::GetNow() - mBenchmarkStart;
    mBenchmarkLastBytesTotal = mBenchmarkBytesTotal;

    OutputBenchmarkResult();

    mBenchmarkBytesTotal = 0;
}

#if OPENTHREAD_CONFIG_TLS_ENABLE
void TcpExample::PrepareTlsHandshake(void)
{
    int rv;
    rv = mbedtls_ssl_set_hostname(&mSslContext, "localhost");
    if (rv != 0)
    {
        OutputLine("mbedtls_ssl_set_hostname returned %d", rv);
    }
    rv = mbedtls_ssl_set_hs_ecjpake_password(&mSslContext, reinterpret_cast<const unsigned char *>(sEcjpakePassword),
                                             sEcjpakePasswordLength);
    if (rv != 0)
    {
        OutputLine("mbedtls_ssl_set_hs_ecjpake_password returned %d", rv);
    }
    mbedtls_ssl_set_bio(&mSslContext, &mEndpointAndCircularSendBuffer, otTcpMbedTlsSslSendCallback,
                        otTcpMbedTlsSslRecvCallback, nullptr);
    mTlsHandshakeComplete = false;
}

bool TcpExample::ContinueTlsHandshake(void)
{
    bool wasNotAlreadyDone = false;
    int  rv;

    if (!mTlsHandshakeComplete)
    {
        rv = mbedtls_ssl_handshake(&mSslContext);
        if (rv == 0)
        {
            OutputLine("TLS Handshake Complete");
            mTlsHandshakeComplete = true;
        }
        else if (rv != MBEDTLS_ERR_SSL_WANT_READ && rv != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            OutputLine("TLS Handshake Failed: %d", rv);
        }
        wasNotAlreadyDone = true;
    }

    return wasNotAlreadyDone;
}
#endif // OPENTHREAD_CONFIG_TLS_ENABLE

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
