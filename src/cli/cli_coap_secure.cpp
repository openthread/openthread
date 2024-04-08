/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements a simple CLI for the CoAP Secure service.
 */

#include "cli_coap_secure.hpp"

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#include <mbedtls/debug.h>
#include <openthread/random_noncrypto.h>

#include "cli/cli.hpp"

// header for place your x509 certificate and private key
#include "x509_cert_key.hpp"

namespace ot {
namespace Cli {

CoapSecure::CoapSecure(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
    , mShutdownFlag(false)
    , mUseCertificate(false)
    , mPskLength(0)
    , mPskIdLength(0)
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    , mBlockCount(1)
#endif
{
    ClearAllBytes(mResource);
    ClearAllBytes(mPsk);
    ClearAllBytes(mPskId);
    ClearAllBytes(mUriPath);
    strncpy(mResourceContent, "0", sizeof(mResourceContent));
    mResourceContent[sizeof(mResourceContent) - 1] = '\0';
}

void CoapSecure::PrintPayload(otMessage *aMessage)
{
    uint8_t  buf[kMaxBufferSize];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length       = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    if (length > 0)
    {
        OutputFormat(" with payload: ");

        while (length > 0)
        {
            bytesToPrint = Min(length, static_cast<uint16_t>(sizeof(buf)));
            otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);

            OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

            length -= bytesToPrint;
            bytesPrinted += bytesToPrint;
        }
    }

    OutputNewLine();
}

/**
 * @cli coaps resource (get,set)
 * @code
 * coaps resource test-resource
 * Done
 * @endcode
 * @code
 * coaps resource
 * test-resource
 * Done
 * @endcode
 * @cparam coaps resource [@ca{uri-path}]
 * @par
 * Gets or sets the URI path of the CoAPS server resource. @moreinfo{@coaps}.
 * @sa otCoapSecureAddBlockWiseResource
 */
template <> otError CoapSecure::Process<Cmd("resource")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (!aArgs[0].IsEmpty())
    {
        VerifyOrExit(aArgs[0].GetLength() < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);

        mResource.mUriPath = mUriPath;
        mResource.mContext = this;
        mResource.mHandler = &CoapSecure::HandleRequest;

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        mResource.mReceiveHook  = &CoapSecure::BlockwiseReceiveHook;
        mResource.mTransmitHook = &CoapSecure::BlockwiseTransmitHook;

        if (!aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[1].ParseAsUint32(mBlockCount));
        }
#endif

        strncpy(mUriPath, aArgs[0].GetCString(), sizeof(mUriPath) - 1);
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        otCoapSecureAddBlockWiseResource(GetInstancePtr(), &mResource);
#else
        otCoapSecureAddResource(GetInstancePtr(), &mResource);
#endif
    }
    else
    {
        OutputLine("%s", mResource.mUriPath != nullptr ? mResource.mUriPath : "");
    }

exit:
    return error;
}

/**
 * @cli coaps set
 * @code
 * coaps set Testing123
 * Done
 * @endcode
 * @cparam coaps set @ca{new-content}
 * @par
 * Sets the content sent by the resource on the CoAPS server. @moreinfo{@coaps}.
 */
template <> otError CoapSecure::Process<Cmd("set")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (!aArgs[0].IsEmpty())
    {
        VerifyOrExit(aArgs[0].GetLength() < sizeof(mResourceContent), error = OT_ERROR_INVALID_ARGS);
        strncpy(mResourceContent, aArgs[0].GetCString(), sizeof(mResourceContent));
        mResourceContent[sizeof(mResourceContent) - 1] = '\0';
    }
    else
    {
        OutputLine("%s", mResourceContent);
    }

exit:
    return error;
}

/**
 * @cli coaps start
 * @code
 * coaps start
 * Done
 * @endcode
 * @code
 * coaps start false
 * Done
 * @endcode
 * @code
 * coaps start 8
 * Done
 * @endcode
 * @cparam coaps start [@ca{check-peer-cert} | @ca{max-conn-attempts}]
 * The `check-peer-cert` parameter determines if the peer-certificate check is
 * enabled (default) or disabled.
 * The `max-conn-attempts` parameter sets the maximum number of allowed
 * attempts, successful or failed, to connect to the CoAP Secure server.
 * The default value of this parameter is `0`, which means that there is
 * no limit to the number of attempts.
 * The `check-peer-cert` and `max-conn-attempts` parameters work
 * together in the following combinations, even though you can only specify
 * one argument:
 *   * No argument specified: Defaults are used.
 *   * Setting `check-peer-cert` to `true`:
 *     Has the same effect as omitting the argument, which is that the
 *     `check-peer-cert` value is `true`, and the `max-conn-attempts` value is 0.
 *   * Setting `check-peer-cert` to `false`:
 *    `check-peer-cert` value is `false`, and the `max-conn-attempts` value is 0.
 *   * Specifying a number:
 *     `check-peer-cert` is `true`, and the `max-conn-attempts` value is the
 *     number specified in the argument.
 * @par
 * Starts the CoAP Secure service. @moreinfo{@coaps}.
 * @sa otCoapSecureStart
 * @sa otCoapSecureSetSslAuthMode
 * @sa otCoapSecureSetClientConnectedCallback
 */
template <> otError CoapSecure::Process<Cmd("start")>(Arg aArgs[])
{
    otError  error           = OT_ERROR_NONE;
    bool     verifyPeerCert  = true;
    uint16_t maxConnAttempts = 0;

    if (!aArgs[0].IsEmpty())
    {
        if (aArgs[0] == "false")
        {
            verifyPeerCert = false;
        }
        else if (aArgs[0] == "true")
        {
            verifyPeerCert = true;
        }
        else
        {
            SuccessOrExit(error = aArgs[0].ParseAsUint16(maxConnAttempts));
        }
    }

    otCoapSecureSetSslAuthMode(GetInstancePtr(), verifyPeerCert);
    otCoapSecureSetClientConnectedCallback(GetInstancePtr(), &CoapSecure::HandleConnected, this);

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
    otCoapSecureSetDefaultHandler(GetInstancePtr(), &CoapSecure::DefaultHandler, this);
#endif

    error = otCoapSecureStartWithMaxConnAttempts(GetInstancePtr(), OT_DEFAULT_COAP_SECURE_PORT, maxConnAttempts,
                                                 nullptr, nullptr);

exit:
    return error;
}

/**
 * @cli coaps stop
 * @code
 * coaps stop
 * Done
 * @endcode
 * @par
 * Stops the CoAP Secure service. @moreinfo{@coaps}.
 * @sa otCoapSecureStop
 */
template <> otError CoapSecure::Process<Cmd("stop")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    otCoapRemoveBlockWiseResource(GetInstancePtr(), &mResource);
#else
    otCoapRemoveResource(GetInstancePtr(), &mResource);
#endif

    if (otCoapSecureIsConnectionActive(GetInstancePtr()))
    {
        otCoapSecureDisconnect(GetInstancePtr());
        mShutdownFlag = true;
    }
    else
    {
        Stop();
    }

    return OT_ERROR_NONE;
}

/**
 * @cli coaps isclosed
 * @code
 * coaps isclosed
 * no
 * Done
 * @endcode
 * @par
 * Indicates if the CoAP Secure service is closed. @moreinfo{@coaps}.
 * @sa otCoapSecureIsClosed
 */
template <> otError CoapSecure::Process<Cmd("isclosed")>(Arg aArgs[])
{
    return ProcessIsRequest(aArgs, otCoapSecureIsClosed);
}

/**
 * @cli coaps isconnected
 * @code
 * coaps isconnected
 * yes
 * Done
 * @endcode
 * @par
 * Indicates if the CoAP Secure service is connected. @moreinfo{@coaps}.
 * @sa otCoapSecureIsConnected
 */
template <> otError CoapSecure::Process<Cmd("isconnected")>(Arg aArgs[])
{
    return ProcessIsRequest(aArgs, otCoapSecureIsConnected);
}

/**
 * @cli coaps isconnactive
 * @code
 * coaps isconnactive
 * yes
 * Done
 * @endcode
 * @par
 * Indicates if the CoAP Secure service connection is active
 * (either already connected or in the process of establishing a connection).
 * @moreinfo{@coaps}.
 * @sa otCoapSecureIsConnectionActive
 */
template <> otError CoapSecure::Process<Cmd("isconnactive")>(Arg aArgs[])
{
    return ProcessIsRequest(aArgs, otCoapSecureIsConnectionActive);
}

otError CoapSecure::ProcessIsRequest(Arg aArgs[], bool (*IsChecker)(otInstance *))
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine("%s", IsChecker(GetInstancePtr()) ? "yes" : "no");

exit:
    return error;
}

/**
 * @cli coaps get
 * @code
 * coaps get test-resource
 * Done
 * @endcode
 * @code
 * coaps get test-resource block-1024
 * Done
 * @endcode
 * @cparam coaps get @ca{uri-path} [@ca{type}]
 *   * `uri-path`: URI path of the resource.
 *   * `type`:
 *       * `con`: Confirmable
 *       * `non-con`: Non-confirmable (default)
 *       * `block-`: Use this option, followed by the block-wise value,
 *         if the response should be transferred block-wise.
 *         Valid values are: `block-16`, `block-32`, `block-64`, `block-128`,
 *         `block-256`, `block-512`, or `block-1024`.
 * @par
 * Gets information about the specified CoAPS resource on the CoAPS server.
 * @moreinfo{@coaps}.
 */
template <> otError CoapSecure::Process<Cmd("get")>(Arg aArgs[]) { return ProcessRequest(aArgs, OT_COAP_CODE_GET); }

/**
 * @cli coaps post
 * @code
 * coaps post test-resource con hellothere
 * Done
 * @endcode
 * @code
 * coaps post test-resource block-1024 10
 * Done
 * @endcode
 * @cparam @ca{uri-path} [@ca{type}] [@ca{payload}]
 *   * `uri-path`: URI path of the resource.
 *   * `type`:
 *       * `con`: Confirmable
 *       * `non-con`: Non-confirmable (default)
 *       * `block-`: Use this option, followed by the block-wise value,
 *         to send blocks with a randomly generated number of bytes for the payload.
 *         Valid values are: `block-16`, `block-32`, `block-64`, `block-128`,
 *         `block-256`, `block-512`, or `block-1024`.
 *	 * `payload`:  CoAPS payload request, which if used is either a string
 *	   or an integer, depending on the `type`. If the `type` is `con` or `non-con`,
 *	   the payload parameter is optional. If you leave out the payload
 *	   parameter, an empty payload is sent. However, If you use the payload
 *	   parameter, its value must be a string, such as `hellothere`. If the
 *	   `type` is `block-`, the value of the payload parameter must be an
 *	   integer that specifies the number of blocks to send. The `block-` type
 *	   requires `OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE` to be set.
 * @par
 * Creates the specified CoAPS resource. @moreinfo{@coaps}.
 */
template <> otError CoapSecure::Process<Cmd("post")>(Arg aArgs[]) { return ProcessRequest(aArgs, OT_COAP_CODE_POST); }

/**
 * @cli coaps put
 * @code
 * coaps put test-resource con hellothere
 * Done
 * @endcode
 * @code
 * coaps put test-resource block-1024 10
 * Done
 * @endcode
 * @cparam @ca{uri-path} [@ca{type}] [@ca{payload}]
 *   * `uri-path`: URI path of the resource.
 *   * `type`:
 *       * `con`: Confirmable
 *       * `non-con`: Non-confirmable (default)
 *       * `block-`: Use this option, followed by the block-wise value,
 *         to send blocks with a randomly generated number of bytes for the payload.
 *         Valid values are: `block-16`, `block-32`, `block-64`, `block-128`,
 *         `block-256`, `block-512`, or `block-1024`.
 *	 * `payload`:  CoAPS payload request, which if used is either a string
 *	   or an integer, depending on the `type`. If the `type` is `con` or `non-con`,
 *	   the payload parameter is optional. If you leave out the payload
 *	   parameter, an empty payload is sent. However, If you use the payload
 *	   parameter, its value must be a string, such as `hellothere`. If the
 *	   `type` is `block-`, the value of the payload parameter must be an
 *	   integer that specifies the number of blocks to send. The `block-` type
 *	   requires `OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE` to be set.
 * @par
 * Modifies the specified CoAPS resource. @moreinfo{@coaps}.
 */
template <> otError CoapSecure::Process<Cmd("put")>(Arg aArgs[]) { return ProcessRequest(aArgs, OT_COAP_CODE_PUT); }

/**
 * @cli coaps delete
 * @code
 * coaps delete test-resource con hellothere
 * Done
 * @endcode
 * @cparam coaps delete @ca{uri-path} [@ca{type}] [@ca{payload}]
 *   * `uri-path`: URI path of the resource.
 *   * `type`:
 *       * `con`: Confirmable
 *       * `non-con`: Non-confirmable (default)
 *   * `payload`: CoAPS payload request.
 * @par
 * The CoAPS payload string to delete.
 */
template <> otError CoapSecure::Process<Cmd("delete")>(Arg aArgs[])
{
    return ProcessRequest(aArgs, OT_COAP_CODE_DELETE);
}

otError CoapSecure::ProcessRequest(Arg aArgs[], otCoapCode aCoapCode)
{
    otError    error         = OT_ERROR_NONE;
    otMessage *message       = nullptr;
    uint16_t   payloadLength = 0;

    // Default parameters
    char       coapUri[kMaxUriLength];
    otCoapType coapType = OT_COAP_TYPE_NON_CONFIRMABLE;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    bool           coapBlock     = false;
    otCoapBlockSzx coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_16;
    BlockType      coapBlockType = (aCoapCode == OT_COAP_CODE_GET) ? kBlockType2 : kBlockType1;
#endif

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aArgs[0].GetLength() < sizeof(coapUri), error = OT_ERROR_INVALID_ARGS);
    strcpy(coapUri, aArgs[0].GetCString());

    if (!aArgs[1].IsEmpty())
    {
        if (aArgs[1] == "con")
        {
            coapType = OT_COAP_TYPE_CONFIRMABLE;
        }
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        else if (aArgs[1] == "block-16")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_16;
        }
        else if (aArgs[1] == "block-32")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_32;
        }
        else if (aArgs[1] == "block-64")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_64;
        }
        else if (aArgs[1] == "block-128")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_128;
        }
        else if (aArgs[1] == "block-256")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_256;
        }
        else if (aArgs[1] == "block-512")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_512;
        }
        else if (aArgs[1] == "block-1024")
        {
            coapType      = OT_COAP_TYPE_CONFIRMABLE;
            coapBlock     = true;
            coapBlockSize = OT_COAP_OPTION_BLOCK_SZX_1024;
        }
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    }

    message = otCoapNewMessage(GetInstancePtr(), nullptr);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, aCoapCode);
    otCoapMessageGenerateToken(message, OT_COAP_DEFAULT_TOKEN_LENGTH);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, coapUri));

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    if (coapBlock)
    {
        if (coapBlockType == kBlockType1)
        {
            SuccessOrExit(error = otCoapMessageAppendBlock1Option(message, 0, true, coapBlockSize));
        }
        else
        {
            SuccessOrExit(error = otCoapMessageAppendBlock2Option(message, 0, false, coapBlockSize));
        }
    }
#endif

    if (!aArgs[2].IsEmpty())
    {
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        if (coapBlock)
        {
            SuccessOrExit(error = aArgs[2].ParseAsUint32(mBlockCount));
        }
        else
        {
#endif
            payloadLength = aArgs[2].GetLength();

            if (payloadLength > 0)
            {
                SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));
            }
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        }
#endif
    }

    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, aArgs[2].GetCString(), payloadLength));
    }

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (aCoapCode == OT_COAP_CODE_GET))
    {
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        if (coapBlock)
        {
            error =
                otCoapSecureSendRequestBlockWise(GetInstancePtr(), message, &CoapSecure::HandleResponse, this,
                                                 &CoapSecure::BlockwiseTransmitHook, &CoapSecure::BlockwiseReceiveHook);
        }
        else
        {
#endif
            error = otCoapSecureSendRequest(GetInstancePtr(), message, &CoapSecure::HandleResponse, this);
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        }
#endif
    }
    else
    {
        error = otCoapSecureSendRequest(GetInstancePtr(), message, nullptr, nullptr);
    }

exit:

    if ((error != OT_ERROR_NONE) && (message != nullptr))
    {
        otMessageFree(message);
    }

    return error;
}

/**
 * @cli coaps connect
 * @code
 * coaps connect fdde:ad00:beef:0:9903:14b:27e0:5744
 * Done
 * coaps connected
 * @endcode
 * @cparam coaps connect @ca{address}
 * The `address` parameter is the IPv6 address of the peer.
 * @par
 * Initializes a Datagram Transport Layer Security (DTLS) session with a peer.
 * @moreinfo{@coaps}.
 * @sa otCoapSecureConnect
 */
template <> otError CoapSecure::Process<Cmd("connect")>(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;

    ClearAllBytes(sockaddr);
    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(sockaddr.mAddress));
    sockaddr.mPort = OT_DEFAULT_COAP_SECURE_PORT;

    if (!aArgs[1].IsEmpty())
    {
        SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    }

    SuccessOrExit(error = otCoapSecureConnect(GetInstancePtr(), &sockaddr, &CoapSecure::HandleConnected, this));

exit:
    return error;
}

/**
 * @cli coaps disconnect
 * @code
 * coaps disconnect
 * coaps disconnected
 * Done
 * @endcode
 * @par
 * Stops the DTLS session.
 * @sa otCoapSecureDisconnect
 */
template <> otError CoapSecure::Process<Cmd("disconnect")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otCoapSecureDisconnect(GetInstancePtr());

    return OT_ERROR_NONE;
}

/**
 * <!--- This tag is before the IF statement so that Doxygen imports the command. --->
 * @cli coaps psk
 * @code
 * coaps psk 1234 key1
 * Done
 * @endcode
 * @cparam coaps psk @ca{psk-value} @ca{psk-id}
 *   * `psk-value`: The pre-shared key
 *   * `psk-id`: The pre-shared key identifier.
 * @par
 * Sets the pre-shared key (PSK) and cipher suite DTLS_PSK_WITH_AES_128_CCM_8.
 * @note This command requires the build-time feature
 * `MBEDTLS_KEY_EXCHANGE_PSK_ENABLED` to be enabled.
 * @sa #otCoapSecureSetPsk
 */
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
template <> otError CoapSecure::Process<Cmd("psk")>(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint16_t length;

    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    length = aArgs[0].GetLength();
    VerifyOrExit(length <= sizeof(mPsk), error = OT_ERROR_INVALID_ARGS);
    mPskLength = static_cast<uint8_t>(length);
    memcpy(mPsk, aArgs[0].GetCString(), mPskLength);

    length = aArgs[1].GetLength();
    VerifyOrExit(length <= sizeof(mPskId), error = OT_ERROR_INVALID_ARGS);
    mPskIdLength = static_cast<uint8_t>(length);
    memcpy(mPskId, aArgs[1].GetCString(), mPskIdLength);

    otCoapSecureSetPsk(GetInstancePtr(), mPsk, mPskLength, mPskId, mPskIdLength);
    mUseCertificate = false;

exit:
    return error;
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

/**
 * <!--- This tag is before the IF statement so that Doxygen imports the command. --->
 * @cli coaps x509
 * @code
 * coaps x509
 * Done
 * @endcode
 * @par
 * Sets the X509 certificate of the local device with the corresponding private key for
 * the DTLS session with `DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8`.
 * @note This command requires `MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED=1`
 * to be enabled.
 * The X.509 certificate is stored in the location: `src/cli/x509_cert_key.hpp`.
 * @sa otCoapSecureSetCertificate
 * @sa otCoapSecureSetCaCertificateChain
 */
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
template <> otError CoapSecure::Process<Cmd("x509")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otCoapSecureSetCertificate(GetInstancePtr(), reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_X509_CERT),
                               sizeof(OT_CLI_COAPS_X509_CERT), reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_PRIV_KEY),
                               sizeof(OT_CLI_COAPS_PRIV_KEY));

    otCoapSecureSetCaCertificateChain(GetInstancePtr(),
                                      reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE),
                                      sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE));
    mUseCertificate = true;

    return OT_ERROR_NONE;
}
#endif

otError CoapSecure::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                  \
    {                                                             \
        aCommandString, &CoapSecure::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("connect"),  CmdEntry("delete"),       CmdEntry("disconnect"),  CmdEntry("get"),
        CmdEntry("isclosed"), CmdEntry("isconnactive"), CmdEntry("isconnected"), CmdEntry("post"),
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
        CmdEntry("psk"),
#endif
        CmdEntry("put"),      CmdEntry("resource"),     CmdEntry("set"),         CmdEntry("start"),
        CmdEntry("stop"),
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        CmdEntry("x509"),
#endif
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? OT_ERROR_INVALID_ARGS : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void CoapSecure::Stop(void)
{
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    otCoapRemoveBlockWiseResource(GetInstancePtr(), &mResource);
#else
    otCoapRemoveResource(GetInstancePtr(), &mResource);
#endif
    otCoapSecureStop(GetInstancePtr());
}

void CoapSecure::HandleConnected(bool aConnected, void *aContext)
{
    static_cast<CoapSecure *>(aContext)->HandleConnected(aConnected);
}

void CoapSecure::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        OutputLine("coaps connected");
    }
    else
    {
        OutputLine("coaps disconnected");

        if (mShutdownFlag)
        {
            Stop();
            mShutdownFlag = false;
        }
    }
}

void CoapSecure::HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->HandleRequest(aMessage, aMessageInfo);
}

void CoapSecure::HandleRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = nullptr;
    otCoapCode responseCode    = OT_COAP_CODE_EMPTY;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    uint64_t             blockValue   = 0;
    bool                 blockPresent = false;
    otCoapOptionIterator iterator;
#endif

    OutputFormat("coaps request from ");
    OutputIp6Address(aMessageInfo->mPeerAddr);
    OutputFormat(" ");

    switch (otCoapMessageGetCode(aMessage))
    {
    case OT_COAP_CODE_GET:
        OutputFormat("GET");
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        SuccessOrExit(error = otCoapOptionIteratorInit(&iterator, aMessage));
        if (otCoapOptionIteratorGetFirstOptionMatching(&iterator, OT_COAP_OPTION_BLOCK2) != nullptr)
        {
            SuccessOrExit(error = otCoapOptionIteratorGetOptionUintValue(&iterator, &blockValue));
            blockPresent = true;
        }
#endif
        break;

    case OT_COAP_CODE_DELETE:
        OutputFormat("DELETE");
        break;

    case OT_COAP_CODE_PUT:
        OutputFormat("PUT");
        break;

    case OT_COAP_CODE_POST:
        OutputFormat("POST");
        break;

    default:
        OutputLine("Undefined");
        return;
    }

    PrintPayload(aMessage);

    if ((otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE) ||
        (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET))
    {
        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            responseCode = OT_COAP_CODE_CONTENT;
        }
        else
        {
            responseCode = OT_COAP_CODE_VALID;
        }

        responseMessage = otCoapNewMessage(GetInstancePtr(), nullptr);
        VerifyOrExit(responseMessage != nullptr, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(
            error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode));

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
            if (blockPresent)
            {
                SuccessOrExit(error = otCoapMessageAppendBlock2Option(responseMessage,
                                                                      static_cast<uint32_t>(blockValue >> 4), true,
                                                                      static_cast<otCoapBlockSzx>(blockValue & 0x7)));
            }
#endif
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(responseMessage));
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
            if (!blockPresent)
            {
#endif
                SuccessOrExit(error = otMessageAppend(responseMessage, &mResourceContent,
                                                      static_cast<uint16_t>(strlen(mResourceContent))));
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
            }
#endif
        }

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        if (blockPresent)
        {
            SuccessOrExit(error = otCoapSecureSendResponseBlockWise(GetInstancePtr(), responseMessage, aMessageInfo,
                                                                    this, mResource.mTransmitHook));
        }
        else
        {
#endif
            SuccessOrExit(error = otCoapSecureSendResponse(GetInstancePtr(), responseMessage, aMessageInfo));
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        }
#endif
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        if (responseMessage != nullptr)
        {
            OutputLine("coaps send response error %d: %s", error, otThreadErrorToString(error));
            otMessageFree(responseMessage);
        }
    }
    else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
    {
        OutputLine("coaps response sent");
    }
}

void CoapSecure::HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    static_cast<CoapSecure *>(aContext)->HandleResponse(aMessage, aMessageInfo, aError);
}

void CoapSecure::HandleResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    if (aError != OT_ERROR_NONE)
    {
        OutputLine("coaps receive response error %d: %s", aError, otThreadErrorToString(aError));
    }
    else
    {
        OutputFormat("coaps response from ");
        OutputIp6Address(aMessageInfo->mPeerAddr);

        PrintPayload(aMessage);
    }
}

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
void CoapSecure::DefaultHandler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->DefaultHandler(aMessage, aMessageInfo);
}

void CoapSecure::DefaultHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = nullptr;

    if ((otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE) ||
        (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET))
    {
        responseMessage = otCoapNewMessage(GetInstancePtr(), nullptr);
        VerifyOrExit(responseMessage != nullptr, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE,
                                                        OT_COAP_CODE_NOT_FOUND));

        SuccessOrExit(error = otCoapSecureSendResponse(GetInstancePtr(), responseMessage, aMessageInfo));
    }

exit:
    if (error != OT_ERROR_NONE && responseMessage != nullptr)
    {
        otMessageFree(responseMessage);
    }
}
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
otError CoapSecure::BlockwiseReceiveHook(void          *aContext,
                                         const uint8_t *aBlock,
                                         uint32_t       aPosition,
                                         uint16_t       aBlockLength,
                                         bool           aMore,
                                         uint32_t       aTotalLength)
{
    return static_cast<CoapSecure *>(aContext)->BlockwiseReceiveHook(aBlock, aPosition, aBlockLength, aMore,
                                                                     aTotalLength);
}

otError CoapSecure::BlockwiseReceiveHook(const uint8_t *aBlock,
                                         uint32_t       aPosition,
                                         uint16_t       aBlockLength,
                                         bool           aMore,
                                         uint32_t       aTotalLength)
{
    OT_UNUSED_VARIABLE(aMore);
    OT_UNUSED_VARIABLE(aTotalLength);

    OutputLine("received block: Num %i Len %i", aPosition / aBlockLength, aBlockLength);

    for (uint16_t i = 0; i < aBlockLength / 16; i++)
    {
        OutputBytesLine(&aBlock[i * 16], 16);
    }

    return OT_ERROR_NONE;
}

otError CoapSecure::BlockwiseTransmitHook(void     *aContext,
                                          uint8_t  *aBlock,
                                          uint32_t  aPosition,
                                          uint16_t *aBlockLength,
                                          bool     *aMore)
{
    return static_cast<CoapSecure *>(aContext)->BlockwiseTransmitHook(aBlock, aPosition, aBlockLength, aMore);
}

otError CoapSecure::BlockwiseTransmitHook(uint8_t *aBlock, uint32_t aPosition, uint16_t *aBlockLength, bool *aMore)
{
    static uint32_t blockCount = 0;
    OT_UNUSED_VARIABLE(aPosition);

    // Send a random payload
    otRandomNonCryptoFillBuffer(aBlock, *aBlockLength);

    OutputLine("send block: Num %i Len %i", blockCount, *aBlockLength);

    for (uint16_t i = 0; i < *aBlockLength / 16; i++)
    {
        OutputBytesLine(&aBlock[i * 16], 16);
    }

    if (blockCount == mBlockCount - 1)
    {
        blockCount = 0;
        *aMore     = false;
    }
    else
    {
        *aMore = true;
        blockCount++;
    }

    return OT_ERROR_NONE;
}
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
