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
 * @brief
 *  This file defines the top-level OpenThread APIs related to message buffer and queues.
 */

#ifndef OPENTHREAD_MESSAGE_H_
#define OPENTHREAD_MESSAGE_H_

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup api  API
 * @brief
 *   This module includes the application programming interface to the OpenThread stack.
 *
 * @{
 *
 * @defgroup execution Execution
 * @defgroup commands Commands
 * @defgroup config Configuration
 * @defgroup diags Diagnostics
 * @defgroup messages Message Buffers
 * @defgroup ip6 IPv6
 * @defgroup udp UDP
 * @defgroup coap CoAP
 *
 * @}
 *
 */

/**
 * @addtogroup messages  Message Buffers
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers
 *
 * @{
 *
 */

/**
 * Free an allocated message buffer.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval kThreadErrorNone  Successfully freed the message buffer.
 *
 * @sa otNewUdpMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otFreeMessage(otMessage aMessage);

/**
 * Get the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message length in bytes.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 * @sa otSetMessageLength
 */
uint16_t otGetMessageLength(otMessage aMessage);

/**
 * Set the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aLength   A length in bytes.
 *
 * @retval kThreadErrorNone    Successfully set the message length.
 * @retval kThreadErrorNoBufs  No available buffers to grow the message.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageLength(otMessage aMessage, uint16_t aLength);

/**
 * Get the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message offset value.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
uint16_t otGetMessageOffset(otMessage aMessage);

/**
 * Set the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 *
 * @retval kThreadErrorNone        Successfully set the message offset.
 * @retval kThreadErrorInvalidArg  The offset is beyond the message length.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageOffset(otMessage aMessage, uint16_t aOffset);

/**
 * Append bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aBuf      A pointer to the data to append.
 * @param[in]  aLength   Number of bytes to append.
 *
 * @retval kThreadErrorNone    Successfully appended to the message
 * @retval kThreadErrorNoBufs  No available buffers to grow the message.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otAppendMessage(otMessage aMessage, const void *aBuf, uint16_t aLength);

/**
 * Read bytes from a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are read to.
 * @param[in]  aLength   Number of bytes to read.
 *
 * @returns The number of bytes read.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otWriteMessage
 */
int otReadMessage(otMessage aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength);

/**
 * Write bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are written from.
 * @param[in]  aLength   Number of bytes to write.
 *
 * @returns The number of bytes written.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 */
int otWriteMessage(otMessage aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength);

/**
 *
 * Initialize the message queue.
 *
 * This function MUST be called once and only once for a `otMessageQueue` instance before any other `otMessageQueue`
 * functions. The behavior is undefined if other queue APIs are used with an `otMessageQueue` before it being
 * initialized or if it is initialized more than once.
 *
 * @param[in]  aQueue     A pointer to a message queue.
 *
 */
void otMessageQueueInit(otMessageQueue *aQueue);

/**
 * This function adds a message to the end of the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to add.
 *
 * @retval kThreadError_None     Successfully added the message to the queue.
 * @retval kThreadError_Already  The message is already enqueued in a queue.
 *
 */
ThreadError otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage aMessage);

/**
 * This function removes a message from the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to remove.
 *
 * @retval kThreadError_None      Successfully removed the message from the queue.
 * @retval kThreadError_NotFound  The message is not enqueued in this queue.
 *
 */
ThreadError otMessageQueueDequeue(otMessageQueue *aQueue, otMessage aMessage);

/**
 * This function returns a pointer to the message at the head of the queue.
 *
 * @param[in]  aQueue    A pointer to a message queue.
 *
 * @returns  A pointer to the message at the head of queue or NULL if queue is empty.
 *
 */
otMessage otMessageQueueGetHead(otMessageQueue *aQueue);

/**
 * This function returns a pointer to the next message in the queue by iterating forward (from head to tail).
 *
 * @param[in]  aQueue    A pointer to a message queue.
 * @param[in]  aMessage  A pointer to current message buffer.
 *
 * @returns  A pointer to the next message in the queue after `aMessage` or NULL if `aMessage is the tail of queue.
 *           NULL is returned if `aMessage` is not in the queue `aQueue`.
 *
 */
otMessage otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage aMessage);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_MESSAGE_H_
