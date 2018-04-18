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
 *   This file implements the OpenThread CoAP Secure API.
 */

#include "openthread-core-config.h"

#include <openthread/coap_secure.h>

//#include "openthread-instance.h"
#include "coap/coap_header.hpp"

#include "coap/coap_secure.hpp"		// include cores coap secure
#include "common/instance.hpp"


#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

using namespace ot;


otError otCoapSecureStart(otInstance *aInstance, uint16_t aPort, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().Start(aPort, NULL, aContext);
}

otError otCoapSecureSetX509Certificate(otInstance *aInstance, const uint8_t *aX509Cert, uint32_t aX509Length)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().SetX509Certificate(aX509Cert, aX509Length);
}

otError otCoapSecureSetX509PrivateKey(otInstance *aInstance, const uint8_t *aPrivateKey, uint32_t aPrivateKeyLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().SetX509PrivateKey(aPrivateKey, aPrivateKeyLength);
}

otError otCoapSecureSetPSK(otInstance *aInstance, uint8_t *aPsk, uint16_t aPskLength,
                           uint8_t *aPskIdentity, uint16_t aPskIdLength)
{
	Instance &instance = *static_cast<Instance *>(aInstance);

	return instance.GetApplicationCoapSecure().SetPreSharedKey(aPsk, aPskLength,
	                                                           aPskIdentity, aPskIdLength);
}

otError otCoapSecureConnect(otInstance *aInstance,
                            const otMessageInfo * aMessageInfo,
                            otHandleSecureCoapClientConnect aHandler,
                            void *aContext )
{
	Instance &instance = *static_cast<Instance *>(aInstance);

	return instance.GetApplicationCoapSecure().Connect(*static_cast<const Ip6::MessageInfo *>(aMessageInfo),
	                                                   aHandler,
	                                                   aContext);
}

otError otCoapSecureDisconnect(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().Disconnect();
}

bool otCoapSecureIsConnected(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().IsConnected();
}

bool otCoapSecureIsConncetionActive(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().IsConnectionActive();
}

otError otCoapSecureStop(otInstance *aInstance)
{
	Instance &instance = *static_cast<Instance *>(aInstance);

	return instance.GetApplicationCoapSecure().Stop();
}

otError otCoapSecureSendMessage(otInstance *aInstance,
                                otMessage *aMessage,
                                otCoapResponseHandler aHandler = NULL,
                                void *aContext = NULL)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().SendMessage(*static_cast<Message *>(aMessage),
                                                            aHandler,
                                                            aContext);
}


#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
