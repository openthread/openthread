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
 *  This file defines the OpenThread provisional P2P (peer-to-peer) API.
 */

#ifndef OPENTHREAD_PROVISIONAL_P2P_H_
#define OPENTHREAD_PROVISIONAL_P2P_H_

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/provisional/link.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @addtogroup api-provisional-p2p
 *
 * @brief
 *   This module includes provisional functions for the Thread P2P link.
 *
 * @note
 *   The functions in this module require `OPENTHREAD_CONFIG_P2P_ENABLE=1`.
 *
 * @{
 */

/**
 * Represents a request for establishing P2P links.
 */
typedef struct otP2pRequest
{
    otWakeupRequest mWakeupRequest; ///< Wake-up request.
} otP2pRequest;

/**
 * Notifies the caller that the P2P link establishment process has ended.
 *
 * @param[in] aContext A pointer to the application-specific context.
 */
typedef void (*otP2pLinkDoneCallback)(void *aContext);

/**
 * Attempts to wake up peers and establish P2P links with peers.
 *
 * If the @p aP2pRequest indicates a group wake-up, this method establishes multiple P2P links with peers.
 * Otherwise, it establishes at most one P2P link.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aP2pRequest  A pointer to P2P request.
 * @param[in] aCallback    A pointer to the function that is called when the P2P link establishment process ends .
 * @param[in] aContext     A pointer to the callback application-specific context.
 *
 * @retval OT_ERROR_NONE           Successfully started to establish P2P links.
 * @retval OT_ERROR_BUSY           Establishing a P2P link was in progress.
 * @retval OT_ERROR_INVALID_STATE  Device was disabled or not fully configured.
 * @retval OT_ERROR_NO_BUFS        Insufficient buffer space to establish a P2P link.
 */
otError otP2pWakeupAndLink(otInstance           *aInstance,
                           const otP2pRequest   *aP2pRequest,
                           otP2pLinkDoneCallback aCallback,
                           void                 *aContext);

/**
 * Notifies the caller that the P2P link tear down process has ended.
 *
 * @param[in] aContext A pointer to application-specific context.
 */
typedef void (*otP2pUnlinkDoneCallback)(void *aContext);

/**
 * Tears down the P2P link specified by the Extended Address.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aExtAddress  A pointer to the P2P peer's Extended Address.
 * @param[in] aCallback    A pointer to function that is called when the P2P link tear down process has ended.
 * @param[in] aContext     A pointer to callback application-specific context.
 *
 * @retval OT_ERROR_NONE       Successfully started to tear down the P2P link.
 * @retval OT_ERROR_BUSY       Tearing down or establishing a P2P link process is in progress.
 * @retval OT_ERROR_NOT_FOUND  The P2P link identified by the @p aExtAddress was not found.
 */
otError otP2pUnlink(otInstance             *aInstance,
                    const otExtAddress     *aExtAddress,
                    otP2pUnlinkDoneCallback aCallback,
                    void                   *aContext);

/**
 * Defines events of the P2P link.
 */
typedef enum otP2pEvent
{
    OT_P2P_EVENT_LINKED   = 0, ///< The P2P link has been established.
    OT_P2P_EVENT_UNLINKED = 1, ///< The P2P link has been torn down.
} otP2pEvent;

/**
 * Callback function pointer to signal events of the P2P link.
 *
 * @param[in] aEvent       The P2P link event.
 * @param[in] aExtAddress  A pointer to the peer's Extended Address of the P2P link.
 * @param[in] aContext     A pointer to the application-specific context.
 */
typedef void (*otP2pEventCallback)(otP2pEvent aEvent, const otExtAddress *aExtAddress, void *aContext);

/**
 * Sets the callback function to notify event changes of P2P links.
 *
 * A subsequent call to this function will replace any previously set callback.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aCallback  The callback function pointer.
 * @param[in] aContext   A pointer to the callback application-specific context.
 */
void otP2pSetEventCallback(otInstance *aInstance, otP2pEventCallback aCallback, void *aContext);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PROVISIONAL_P2P_H_
