/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file contains definitions for a simple CLI CoAP server and client.
 */

#ifndef CLI_COAP_HPP_
#define CLI_COAP_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_COAP_API_ENABLE

#include "coap/coap_message.hpp"

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI CoAP server and client.
 *
 */
class Coap
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit Coap(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgsLength  The number of elements in @p aArgs.
     * @param[in]  aArgs        An array of command line arguments.
     *
     */
    otError Process(uint8_t aArgsLength, char *aArgs[]);

private:
    enum
    {
        kMaxUriLength  = 32,
        kMaxBufferSize = 16
    };

    struct Command
    {
        const char *mName;
        otError (Coap::*mCommand)(uint8_t aArgsLength, char *aArgs[]);
    };

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    otError CancelResourceSubscription(void);
    void    CancelSubscriber(void);
#endif

    void PrintPayload(otMessage *aMessage) const;

    otError ProcessHelp(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    otError ProcessCancel(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessParameters(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRequest(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessResource(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessSet(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessStart(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessStop(uint8_t aArgsLength, char *aArgs[]);

    static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    static void HandleNotificationResponse(void *               aContext,
                                           otMessage *          aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aError);
    void        HandleNotificationResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);
#endif

    static void HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);
    void        HandleResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);

    const otCoapTxParameters *GetRequestTxParameters(void) const
    {
        return mUseDefaultRequestTxParameters ? nullptr : &mRequestTxParameters;
    }

    const otCoapTxParameters *GetResponseTxParameters(void) const
    {
        return mUseDefaultResponseTxParameters ? nullptr : &mResponseTxParameters;
    }

    static const Command sCommands[];
    Interpreter &        mInterpreter;

    bool mUseDefaultRequestTxParameters;
    bool mUseDefaultResponseTxParameters;

    otCoapTxParameters mRequestTxParameters;
    otCoapTxParameters mResponseTxParameters;

    otCoapResource mResource;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    otIp6Address mRequestAddr;
    otSockAddr   mSubscriberSock;
    char         mRequestUri[kMaxUriLength];
    uint8_t      mRequestToken[OT_COAP_MAX_TOKEN_LENGTH];
    uint8_t      mSubscriberToken[OT_COAP_MAX_TOKEN_LENGTH];
#endif
    char mUriPath[kMaxUriLength];
    char mResourceContent[kMaxBufferSize];
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    uint32_t mObserveSerial;
    uint8_t  mRequestTokenLength;
    uint8_t  mSubscriberTokenLength;
    bool     mSubscriberConfirmableNotifications;
#endif
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE

#endif // CLI_COAP_HPP_
