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
 *   This file contains definitions for a simple CLI CoAP Secure server and client.
 */

#ifndef CLI_COAP_SECURE_HPP_
#define CLI_COAP_SECURE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#include <mbedtls/ssl.h>

#include <openthread/coap_secure.h>

#include "cli/cli_utils.hpp"

#ifndef CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
#define CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER 0
#endif

namespace ot {
namespace Cli {

/**
 * Implements the CLI CoAP Secure server and client.
 */
class CoapSecure : private Utils
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     */
    CoapSecure(otInstance *aInstance, OutputImplementer &aOutputImplementer);

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
     */
    otError Process(Arg aArgs[]);

private:
    static constexpr uint16_t kMaxUriLength   = 32;
    static constexpr uint16_t kMaxBufferSize  = 16;
    static constexpr uint8_t  kPskMaxLength   = 32;
    static constexpr uint8_t  kPskIdMaxLength = 32;

    using Command = CommandEntry<CoapSecure>;

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    enum BlockType : uint8_t{
        kBlockType1,
        kBlockType2,
    };
#endif

    void PrintPayload(otMessage *aMessage);

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ProcessRequest(Arg aArgs[], otCoapCode aCoapCode);
    otError ProcessIsRequest(Arg aArgs[], bool (*IsChecker)(otInstance *));

    void Stop(void);

    static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static void HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);
    void        HandleResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

    static otError BlockwiseReceiveHook(void          *aContext,
                                        const uint8_t *aBlock,
                                        uint32_t       aPosition,
                                        uint16_t       aBlockLength,
                                        bool           aMore,
                                        uint32_t       aTotalLength);
    otError        BlockwiseReceiveHook(const uint8_t *aBlock,
                                        uint32_t       aPosition,
                                        uint16_t       aBlockLength,
                                        bool           aMore,
                                        uint32_t       aTotalLength);
    static otError BlockwiseTransmitHook(void     *aContext,
                                         uint8_t  *aBlock,
                                         uint32_t  aPosition,
                                         uint16_t *aBlockLength,
                                         bool     *aMore);
    otError        BlockwiseTransmitHook(uint8_t *aBlock, uint32_t aPosition, uint16_t *aBlockLength, bool *aMore);
#endif

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
    static void DefaultHandler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        DefaultHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo);
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER

    static void HandleConnectEvent(otCoapSecureConnectEvent aEvent, void *aContext);
    void        HandleConnectEvent(otCoapSecureConnectEvent aEvent);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    otCoapBlockwiseResource mResource;
#else
    otCoapResource mResource;
#endif
    char mUriPath[kMaxUriLength];
    char mResourceContent[kMaxBufferSize];

    bool    mShutdownFlag;
    bool    mUseCertificate;
    uint8_t mPsk[kPskMaxLength];
    uint8_t mPskLength;
    uint8_t mPskId[kPskIdMaxLength];
    uint8_t mPskIdLength;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    uint32_t mBlockCount;
#endif
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#endif // CLI_COAP_SECURE_HPP_
