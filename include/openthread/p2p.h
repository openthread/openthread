/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *  This file defines the OpenThread P2P (peer-to-peer) API.
 */

#ifndef OPENTHREAD_P2P_H_
#define OPENTHREAD_P2P_H_

#include <openthread/link.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a request for waking up the peer to establish P2P links.
 */
typedef struct otP2pRequest
{
    otWakeupAddress mWakeupAddress; ///< Wake-up address of the peer.
} otP2pRequest;

/**
 * Informs the caller about the result of establishing P2P links with peers.
 *
 * @param[in] aError   OT_ERROR_NONE    Indicates that at least one P2P link has been successfully established. The
 *                                      P2P link state changes can be observed by @sa otP2pEventCallback().
 *                     OT_ERROR_TIMEOUT Indicates that no P2P link has been established before the wake-up window ends.
 * @param[in] aContext A pointer to application-specific context.
 */
typedef void (*otP2pLinkedCallback)(otError aError, void *aContext);

/**
 * Attempts to establish P2P links with peers.
 *
 * If the @p aP2pRequest indicates a group identifier, this method establishes multiple P2P links with peers.
 * Otherwise, it establishes at most one P2P link.
 *
 * P2P links established by this method keep alive until released via @sa otP2pUnlink(), or the peer tore down the
 * P2P link or the peer is out of synchronization.
 *
 * @param[in] aInstance    A pointer to an OpenThread instance.
 * @param[in] aP2pRequest  A pointer to P2P request.
 * @param[in] aCallback    A pointer to function that is called when the P2P link succeeds or fails.
 * @param[in] aContext     A pointer to callback application-specific context.
 *
 * @retval OT_ERROR_NONE           Successfully started to establish P2P links.
 * @retval OT_ERROR_BUSY           Establishing a P2P link in progress.
 * @retval OT_ERROR_INVALID_STATE  Device was disabled or not fully configured (missing or incomplete Active Dataset).
 * @retval OT_ERROR_NO_BUFS        Insufficient buffer space to establish a P2P link.
 */
otError otP2pLink(otInstance         *aInstance,
                  const otP2pRequest *aP2pRequest,
                  otP2pLinkedCallback aCallback,
                  void               *aContext);

/**
 * Informs the caller that the P2P link has been successfully tore down.
 *
 * The P2P link state changes can be observed by @sa otP2pEventCallback().
 *
 * @param[in] aContext A pointer to application-specific context.
 */
typedef void (*otP2pUnlinkedCallback)(void *aContext);

/**
 * Tears down the P2P link specified by the Ipv6 address.
 *
 * @param[in] aInstance    A pointer to an OpenThread instance.
 * @param[in] aIp6Address  A pointer to the peer's Ipv6 Address.
 * @param[in] aCallback    A pointer to function that is called when the P2P link was successfully tore down.
 * @param[in] aContext     A pointer to callback application-specific context.
 *
 * @retval OT_ERROR_NONE       Successfully started to tear down the P2P link.
 * @retval OT_ERROR_BUSY       Tearing down a P2P process is in progress.
 * @retval OT_ERROR_NOT_FOUND  The P2P link identified by the @p aIp6Address was not found.
 */
otError otP2pUnlink(otInstance           *aInstance,
                    const otIp6Address   *aIp6Address,
                    otP2pUnlinkedCallback aCallback,
                    void                 *aContext);

/**
 * Defines events of the P2P link.
 */
typedef enum
{
    OT_P2P_EVENT_LINKED   = 0, ///< The P2P link has been established.
    OT_P2P_EVENT_UNLINKED = 1, ///< The P2P link has been tore down.
} otP2pEvent;

/**
 * Callback function pointer to signal events of the P2P link.
 *
 * @param[in] aEvent       The P2P link event.
 * @param[in] aIp6Address  A pointer to the peer's Ipv6 Address of the P2P link.
 * @param[in] aContext     A pointer to application-specific context.
 */
typedef void (*otP2pEventCallback)(otP2pEvent aEvent, const otIp6Address *aIp6Address, void *aContext);

/**
 * Sets the callback function to notify event changes of P2P links.
 *
 * A subsequent call to this function will replace any previously set callback.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aCallback  The callback function pointer.
 * @param[in] aContext   A pointer to callback application-specific context.
 */
void otP2pSetEventCallback(otInstance *aInstance, otP2pEventCallback aCallback, void *aContext);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_P2P_H_
