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

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-message
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
 * @retval OT_ERROR_NONE  Successfully freed the message buffer.
 *
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
otError otMessageFree(otMessage *aMessage);

/**
 * Get the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message length in bytes.
 *
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 * @sa otMessageSetLength
 */
uint16_t otMessageGetLength(otMessage *aMessage);

/**
 * Set the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aLength   A length in bytes.
 *
 * @retval OT_ERROR_NONE     Successfully set the message length.
 * @retval OT_ERROR_NO_BUFS  No available buffers to grow the message.
 *
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
otError otMessageSetLength(otMessage *aMessage, uint16_t aLength);

/**
 * Get the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message offset value.
 *
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
uint16_t otMessageGetOffset(otMessage *aMessage);

/**
 * Set the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 *
 * @retval OT_ERROR_NONE          Successfully set the message offset.
 * @retval OT_ERROR_INVALID_ARGS  The offset is beyond the message length.
 *
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
otError otMessageSetOffset(otMessage *aMessage, uint16_t aOffset);

/**
 * This function indicates whether or not link security is enabled for the message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval TRUE   If link security is enabled.
 * @retval FALSE  If link security is not enabled.
 *
 */
bool otMessageIsLinkSecurityEnabled(otMessage *aMessage);

/**
 * This function sets/forces the message to be forwarded using direct transmission.
 * Default setting for a new message is `false`.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aEnabled  If `true` message will be forced to use direct transmission. If `false` message will
 *                       follow the normal procedure.
 *
 */
void otMessageSetDirectTransmission(otMessage *aMessage, bool aEnabled);

/**
 * Append bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aBuf      A pointer to the data to append.
 * @param[in]  aLength   Number of bytes to append.
 *
 * @retval OT_ERROR_NONE     Successfully appended to the message
 * @retval OT_ERROR_NO_BUFS  No available buffers to grow the message.
 *
 * @sa otMessageFree
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength);

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
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageWrite
 */
int otMessageRead(otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength);

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
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 */
int otMessageWrite(otMessage *aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength);

/**
 * This structure represents an OpenThread message queue.
 */
typedef struct
{
    void *mData; ///< Opaque data used by the implementation.
} otMessageQueue;

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
 * @retval OT_ERROR_NONE     Successfully added the message to the queue.
 * @retval OT_ERROR_ALREADY  The message is already enqueued in a queue.
 *
 */
otError otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage *aMessage);

/**
 * This function removes a message from the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to remove.
 *
 * @retval OT_ERROR_NONE       Successfully removed the message from the queue.
 * @retval OT_ERROR_NOT_FOUND  The message is not enqueued in this queue.
 *
 */
otError otMessageQueueDequeue(otMessageQueue *aQueue, otMessage *aMessage);

/**
 * This function returns a pointer to the message at the head of the queue.
 *
 * @param[in]  aQueue    A pointer to a message queue.
 *
 * @returns  A pointer to the message at the head of queue or NULL if queue is empty.
 *
 */
otMessage *otMessageQueueGetHead(otMessageQueue *aQueue);

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
otMessage *otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage *aMessage);

/**
 * Get the Message Buffer information.
 *
 * @param[in]   aInstance    A pointer to the OpenThread instance.
 * @param[out]  aBufferInfo  A pointer where the message buffer information is written.
 *
 */
OTAPI void OTCALL otMessageGetBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MESSAGE_H_
