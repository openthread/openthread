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
 * @brief
 *   This file includes the abstraction for the platform UDP service.
 */

#ifndef OPENTHREAD_PLATFORM_UDP_H_
#define OPENTHREAD_PLATFORM_UDP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function initializes the UDP socket by platform.
 *
 * @param[in]   aUdpSocket  A pointer to the UDP socket.
 *
 * @retval  OT_ERROR_NONE   Successfully initialized UDP socket by platform.
 * @retval  OT_ERROR_FAILED Failed to initialize UDP Socket.
 *
 */
otError otPlatUdpSocket(otUdpSocket *aUdpSocket);

/**
 * This function closes the UDP socket by platform.
 *
 * @param[in]   aUdpSocket  A pointer to the UDP socket.
 *
 * @retval  OT_ERROR_NONE   Successfully closed UDP socket by platform.
 * @retval  OT_ERROR_FAILED Failed to close UDP Socket.
 *
 */
otError otPlatUdpClose(otUdpSocket *aUdpSocket);

/**
 * This function binds the UDP socket by platform.
 *
 * @param[in]   aUdpSocket  A pointer to the UDP socket.
 *
 * @retval  OT_ERROR_NONE   Successfully binded UDP socket by platform.
 * @retval  OT_ERROR_FAILED Failed to bind UDP socket.
 *
 */
otError otPlatUdpBind(otUdpSocket *aUdpSocket);

/**
 * This function connects UDP socket by platform.
 *
 * @param[in]   aUdpSocket  A pointer to the UDP socket.
 *
 * @retval  OT_ERROR_NONE   Successfully connected by platform.
 * @retval  OT_ERROR_FAILED Failed to connect UDP socket.
 *
 */
otError otPlatUdpConnect(otUdpSocket *aUdpSocket);

/**
 * This function sends UDP payload by platform.
 *
 * @param[in]   aUdpSocket      A pointer to the UDP socket.
 * @param[in]   aMessage        A pointer to the message to send.
 * @param[in]   aMessageInfo    A pointer to the message info associated with @p aMessage.
 *
 * @retval  OT_ERROR_NONE   Successfully sent by platform, and @p aMessage is freed.
 * @retval  OT_ERROR_FAILED Failed to binded UDP socket.
 *
 */
otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_UDP_H_
