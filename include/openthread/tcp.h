/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *  This file defines the OpenThread TCP API.
 *
 */

#ifndef OPENTHREAD_TCP_H_
#define OPENTHREAD_TCP_H_

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-tcp
 *
 * @brief
 *   This module includes functions that control TCP communication.
 *
 * @{
 *
 */

/**
 * This enum represents TCP states.
 *
 */
typedef enum otTcpState : uint8_t
{
    OT_TCP_STATE_CLOSED      = 0,  ///< CLOSED state.
    OT_TCP_STATE_LISTEN      = 1,  ///< LISTEN state.
    OT_TCP_STATE_SYN_SENT    = 2,  ///< SYN-SENT state.
    OT_TCP_STATE_SYN_RCVD    = 3,  ///< SYN-RECEIVED state.
    OT_TCP_STATE_ESTABLISHED = 4,  ///< ESTABLISHED state.
    OT_TCP_STATE_FIN_WAIT_1  = 5,  ///< FIN-WAIT-1 state.
    OT_TCP_STATE_FIN_WAIT_2  = 6,  ///< FIN-WAIT-2 state.
    OT_TCP_STATE_CLOSE_WAIT  = 7,  ///< CLOSE-WAIT state.
    OT_TCP_STATE_LAST_ACK    = 8,  ///< LAST-ACK state.
    OT_TCP_STATE_CLOSING     = 9,  ///< CLOSING state.
    OT_TCP_STATE_TIME_WAIT   = 10, ///< TIME-WAIT state.
} otTcpState;

/**
 * This eum represents TCP socket events.
 *
 */
typedef enum otTcpSocketEvent
{
    OT_TCP_SOCKET_DATA_RECEIVED = 1, ///< The TCP socket has received data.
    OT_TCP_SOCKET_DATA_SENT     = 2, ///< The TCP socket has sent data.
    OT_TCP_SOCKET_CONNECTED     = 3, ///< The TCP socket is connected.
    OT_TCP_SOCKET_DISCONNECTED  = 4, ///< The TCP socket is disconnected.
    OT_TCP_SOCKET_CLOSED        = 5, ///< The TCP socket is closed.
    OT_TCP_SOCKET_ABORTED       = 6, ///< The TCP socket is aborted.
} otTcpEvent;

/**
 * This structure represents a TCP socket.
 *
 */
struct otTcpSocket
{
#if __SIZEOF_POINTER__ == 8
    uint64_t mMemHolder[35];
#elif __SIZEOF_POINTER__ == 4
    uint32_t mMemHolder[50];
#else
#error "only 32bit and 64bit are supported"
#endif
};

/**
 * This callback allows OpenThread to inform the application of a TCP socket events.
 *
 * @param[in]  aSocket  The TCP socket.
 * @param[in]  aEvent   The TCP socket event.
 *
 */
typedef void (*otTcpEventHandler)(otTcpSocket *aSocket, otTcpEvent aEvent);

/**
 * Initialize a TCP/IPv6 socket. The TCP socket will be in CLOSED state after initialization.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aSocket        A pointer to a TCP socket structure.
 * @param[in]  aEventHandler  A pointer to the application callback function to receive TCP socket events.
 * @param[in]  aContext       A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE    Successfully opened the socket.
 * @retval OT_ERROR_FAILED  Failed to open the socket.
 *
 */
void otTcpInitialize(otInstance *aInstance, otTcpSocket *aSocket, otTcpEventHandler aEventHandler, void *aContext);

/**
 * Listen for a connection on the TCP socket.
 *
 * @param[in]  aSocket  A pointer to a TCP socket structure.
 *
 * @retval OT_ERROR_NONE           Successfully set TCP socket to LISTEN state.
 * @retval OT_ERROR_INVALID_STATE  Can not set TCP socket to LISTEN state.
 *
 */
otError otTcpListen(otTcpSocket *aSocket);

/**
 * Close a TCP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a TCP socket structure.
 *
 */
void otTcpClose(otTcpSocket *aSocket);

/**
 * Abort a TCP/IPv6 socket .
 *
 * @param[in]  aSocket    A pointer to a TCP socket structure.
 *
 * @retval OT_ERROR_NONE   Successfully aborted the socket.
 * @retval OT_ERROR_FAILED Failed to abort TCP Socket.
 *
 */
void otTcpAbort(otTcpSocket *aSocket);

/**
 * Bind a TCP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a TCP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 */
otError otTcpBind(otTcpSocket *aSocket, const otSockAddr *aSockName);

/**
 * Connect a TCP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a TCP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval OT_ERROR_NONE   Connect operation was successful.
 * @retval OT_ERROR_FAILED Failed to connect TCP socket.
 *
 */
otError otTcpConnect(otTcpSocket *aSocket, const otSockAddr *aSockName);

/**
 * Write data to a TCP socket.
 *
 * @param[in]  aSocket  A pointer to a TCP socket structure.
 * @param[in]  aData    A pointer to the data bytes.
 * @param[in]  aLength  The length of data bytes.
 *
 * @returns The number of bytes successfully written to the TCP socket.
 *
 */
uint16_t otTcpWrite(otTcpSocket *aSocket, uint8_t *aData, uint16_t aLength);

/**
 * Read data from a TCP socket.
 *
 * @param[in] aSocket  A pointer to the TCP socket structure to read.
 * @param[in] aBuf     A pointer to the buffer to output data.
 * @param[in] aSize    The size of the buffer.
 *
 * @returns The number of bytes successfully read from the TCP socket.
 *
 */
uint16_t otTcpRead(otTcpSocket *aSocket, uint8_t *aBuf, uint16_t aSize);

/**
 * Get the application-specific context of the TCP socket.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 *
 * @returns  The application-specific context of the TCP socket.
 *
 */
void *otTcpGetContext(otTcpSocket *aSocket);

/**
 * Get the TCP state of a TCP socket.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 *
 * @returns  The TCP state.
 *
 */
otTcpState otTcpGetState(otTcpSocket *aSocket);

/**
 * Get the string representation of a TCP state.
 *
 * @param[in] aState  The TCP state.
 *
 * @returns A null-terminated string representation of the TCP state.
 *
 */
const char *otTcpStateToString(otTcpState aState);

/**
 * Get the local socket name of a TCP socket.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 *
 * @returns  The local name of the TCP socket.
 *
 */
const otSockAddr *otTcpGetSockName(otTcpSocket *aSocket);

/**
 * Get the peer socket name of a TCP socket.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 *
 * @returns  The peer name of the TCP socket.
 *
 */
const otSockAddr *otTcpGetPeerName(otTcpSocket *aSocket);

/**
 * Configure the Round Trip Time (RTT) of a TCP socket.
 *
 * @param[in] aSocket  A pointer to the TCP socket to configure.
 * @param[in] aMinRTT  Minimal Round Trip Time.
 * @param[in] aMaxRTT  Maximal Round Trip Time.
 *
 * @retval OT_ERROR_NONE          Successfully configured Round Trip Times.
 * @retval OT_ERROR_INVALID_ARGS  If @p aMinRTT and @p aMaxRTT are not valid Round Trip Times.
 *
 */
otError otTcpConfigRoundTripTime(otTcpSocket *aSocket, uint32_t aMinRTT, uint32_t aMaxRTT);

/**
 * Configure a TCP socket to send RST for the next received segment.
 *
 * NOTE: Only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` configuration is used.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 *
 */
void otTcpResetNextSegment(otTcpSocket *aSocket);

/**
 * Configure the random segment drop probability of a TCP socket.
 *
 * NOTE: Only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` configuration is used.
 *
 * @param[in] aSocket  A pointer to a TCP socket structure.
 * @param[in] aProb    The random segment drop probability (0 ~ 100).
 *
 */
void otTcpSetSegmentRandomDropProb(otInstance *aInstance, uint8_t aProb);

/**
 * This structure defines TCP counters.
 *
 */
struct otTcpCounters
{
    uint32_t mTxSegment;
    uint32_t mRxSegment;
    uint32_t mTxFullSegment;
    uint32_t mRxFullSegment;
    uint32_t mTxAck;
    uint32_t mRxAck;
    uint32_t mRetx;
};

/**
 * Get the TCP counters.
 *
 * NOTE: Only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` configuration is used.
 *
 * @param[out] aCounters    A pointer to a `otTcpCounters` structure to output TCP counters.
 *
 */
void otTcpGetCounters(otInstance *aInstance, otTcpCounters *aCounters);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_TCP_H_
