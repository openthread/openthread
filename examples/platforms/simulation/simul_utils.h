/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef PLATFORM_SIMULATION_SOCKET_UTILS_H_
#define PLATFORM_SIMULATION_SOCKET_UTILS_H_

#include "platform-simulation.h"

/**
 * Represents a socket for communication with other simulation node.
 *
 * This is used for emulation of 15.4 radio or other interfaces.
 */
typedef struct utilsSocket
{
    bool     mInitialized; ///< Whether or not initialized.
    bool     mUseIp6;      ///< Whether IPv6 or IPv4.
    int      mTxFd;        ///< RX file descriptor.
    int      mRxFd;        ///< TX file descriptor.
    uint16_t mPortBase;    ///< Base port number value.
    uint16_t mPort;        ///< The port number used by this node
    union
    {
        struct sockaddr_in  mSockAddr4; ///< The IPv4 group sock address.
        struct sockaddr_in6 mSockAddr6; ///< The IPv4 group sock address.
    } mGroupAddr;                       ///< The group sock address for simulating radio.
} utilsSocket;

extern const char *gLocalInterface; ///< Local interface name or address to use for sockets

/**
 * Adds a file descriptor (FD) to a given FD set.
 *
 * @param[in] aFd      The FD to add.
 * @param[in] aFdSet   The FD set to add to.
 * @param[in] aMaxFd   A pointer to track maximum FD in @p aFdSet (can be NULL).
 */
void utilsAddFdToFdSet(int aFd, fd_set *aFdSet, int *aMaxFd);

/**
 * Initializes the socket.
 *
 * @param[in] aSocket     The socket to initialize.
 * @param[in] aPortBase   The base port number value. Nodes will determine their port as `aPortBased + gNodeId`.
 */
void utilsInitSocket(utilsSocket *aSocket, uint16_t aPortBase);

/**
 * De-initializes the socket.
 *
 * @param[in] aSocket   The socket to de-initialize.
 */
void utilsDeinitSocket(utilsSocket *aSocket);

/**
 * Adds sockets RX FD to a given FD set.
 *
 * @param[in] aSocket   The socket.
 * @param[in] aFdSet    The (read) FD set to add to.
 * @param[in] aMaxFd    A pointer to track maximum FD in @p aFdSet (can be NULL).
 */
void utilsAddSocketRxFd(const utilsSocket *aSocket, fd_set *aFdSet, int *aMaxFd);

/**
 * Adds sockets TX FD to a given FD set.
 *
 * @param[in] aSocket   The socket.
 * @param[in] aFdSet    The (write) FD set to add to.
 * @param[in] aMaxFd    A pointer to track maximum FD in @p aFdSet (can be NULL).
 */
void utilsAddSocketTxFd(const utilsSocket *aSocket, fd_set *aFdSet, int *aMaxFd);

/**
 * Indicates whether the socket can receive.
 *
 * @param[in] aSocket       The socket.
 * @param[in] aReadFdSet    The read FD set.
 *
 * @retval TRUE   The socket RX FD is in @p aReadFdSet, and socket can receive.
 * @retval FALSE  The socket RX FD is not in @p aReadFdSet. Socket is not ready to receive.
 */
bool utilsCanSocketReceive(const utilsSocket *aSocket, const fd_set *aReadFdSet);

/**
 * Indicates whether the socket can send.
 *
 * @param[in] aSocket   The socket.
 * @param[in] aFdSet    The write FD set.
 *
 * @retval TRUE   The socket TX FD is in @p aWriteFdSet, and socket can send.
 * @retval FALSE  The socket TX FD is not in @p aWriteFdSet. Socket is not ready to send.
 */
bool utilsCanSocketSend(const utilsSocket *aSocket, const fd_set *aWriteFdSet);

/**
 * Receives data from socket.
 *
 * MUST be used when `utilsCanSocketReceive()` returns `TRUE.
 *
 * @param[in] aSocket          The socket.
 * @param[out] aBuffer         The buffer to output the read content.
 * @param[in]  aBufferSize     Maximum size of buffer in bytes.
 * @param[out] aSenderNodeId   A pointer to return the Node ID of the sender (derived from the port number).
 *                             Can be NULL if not needed.
 *
 * @returns The number of received bytes written into @p aBuffer.
 */
uint16_t utilsReceiveFromSocket(const utilsSocket *aSocket,
                                void              *aBuffer,
                                uint16_t           aBufferSize,
                                uint16_t          *aSenderNodeId);

/**
 * Sends data over the socket.
 *
 * @param[in] aSocket         The socket.
 * @param[in] aBuffer         The buffer containing the bytes to sent.
 * @param[in]  aBufferSize    Size of data in @p buffer in bytes.
 */
void utilsSendOverSocket(const utilsSocket *aSocket, const void *aBuffer, uint16_t aBufferLength);

#endif // PLATFORM_SIMULATION_SOCKET_UTILS_H_
