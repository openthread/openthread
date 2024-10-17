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

#include <openthread/instance.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-message
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers.
 *
 * @{
 */

/**
 * An opaque representation of an OpenThread message buffer.
 */
typedef struct otMessage otMessage;

/**
 * Defines the OpenThread message priority levels.
 */
typedef enum otMessagePriority
{
    OT_MESSAGE_PRIORITY_LOW    = 0, ///< Low priority level.
    OT_MESSAGE_PRIORITY_NORMAL = 1, ///< Normal priority level.
    OT_MESSAGE_PRIORITY_HIGH   = 2, ///< High priority level.
} otMessagePriority;

/**
 * Defines the OpenThread message origins.
 */
typedef enum otMessageOrigin
{
    OT_MESSAGE_ORIGIN_THREAD_NETIF   = 0, ///< Message from Thread Netif.
    OT_MESSAGE_ORIGIN_HOST_TRUSTED   = 1, ///< Message from a trusted source on host.
    OT_MESSAGE_ORIGIN_HOST_UNTRUSTED = 2, ///< Message from an untrusted source on host.
} otMessageOrigin;

/**
 * Represents a message settings.
 */
typedef struct otMessageSettings
{
    bool    mLinkSecurityEnabled; ///< TRUE if the message should be secured at Layer 2.
    uint8_t mPriority;            ///< Priority level (MUST be a `OT_MESSAGE_PRIORITY_*` from `otMessagePriority`).
} otMessageSettings;

/**
 * Represents link-specific information for messages received from the Thread radio.
 */
typedef struct otThreadLinkInfo
{
    uint16_t mPanId;                   ///< Source PAN ID
    uint8_t  mChannel;                 ///< 802.15.4 Channel
    int8_t   mRss;                     ///< Received Signal Strength in dBm (averaged over fragments)
    uint8_t  mLqi;                     ///< Average Link Quality Indicator (averaged over fragments)
    bool     mLinkSecurity : 1;        ///< Indicates whether or not link security is enabled.
    bool     mIsDstPanIdBroadcast : 1; ///< Indicates whether or not destination PAN ID is broadcast.

    // Applicable/Required only when time sync feature (`OPENTHREAD_CONFIG_TIME_SYNC_ENABLE`) is enabled.
    uint8_t mTimeSyncSeq;       ///< The time sync sequence.
    int64_t mNetworkTimeOffset; ///< The time offset to the Thread network time, in microseconds.

    // Applicable only when OPENTHREAD_CONFIG_MULTI_RADIO feature is enabled.
    uint8_t mRadioType; ///< Radio link type.
} otThreadLinkInfo;

/**
 * Free an allocated message buffer.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageSetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
void otMessageFree(otMessage *aMessage);

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
uint16_t otMessageGetLength(const otMessage *aMessage);

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
uint16_t otMessageGetOffset(const otMessage *aMessage);

/**
 * Set the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 *
 * @sa otMessageFree
 * @sa otMessageAppend
 * @sa otMessageGetLength
 * @sa otMessageSetLength
 * @sa otMessageGetOffset
 * @sa otMessageRead
 * @sa otMessageWrite
 */
void otMessageSetOffset(otMessage *aMessage, uint16_t aOffset);

/**
 * Indicates whether or not link security is enabled for the message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval TRUE   If link security is enabled.
 * @retval FALSE  If link security is not enabled.
 */
bool otMessageIsLinkSecurityEnabled(const otMessage *aMessage);

/**
 * Indicates whether or not the message is allowed to be looped back to host.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval TRUE   If the message is allowed to be looped back to host.
 * @retval FALSE  If the message is not allowed to be looped back to host.
 */
bool otMessageIsLoopbackToHostAllowed(const otMessage *aMessage);

/**
 * Sets whether or not the message is allowed to be looped back to host.
 *
 * @param[in]  aMessage              A pointer to a message buffer.
 * @param[in]  aAllowLoopbackToHost  Whether to allow the message to be looped back to host.
 */
void otMessageSetLoopbackToHostAllowed(otMessage *aMessage, bool aAllowLoopbackToHost);

/**
 * Indicates whether the given message may be looped back in a case of a multicast destination address.
 *
 * If @p aMessage is used along with an `otMessageInfo`, the `mMulticastLoop` field from `otMessageInfo` structure
 * takes precedence and will be used instead of the the value set on @p aMessage.
 *
 * This API is mainly intended for use along with `otIp6Send()` which expects an already prepared IPv6 message.
 *
 * @param[in]  aMessage A pointer to the message.
 */
bool otMessageIsMulticastLoopEnabled(otMessage *aMessage);

/**
 * Controls whether the given message may be looped back in a case of a multicast destination address.
 *
 * @param[in]  aMessage  A pointer to the message.
 * @param[in]  aEnabled  The configuration value.
 */
void otMessageSetMulticastLoopEnabled(otMessage *aMessage, bool aEnabled);

/**
 * Gets the message origin.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message origin.
 */
otMessageOrigin otMessageGetOrigin(const otMessage *aMessage);

/**
 * Sets the message origin.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOrigin   The message origin.
 */
void otMessageSetOrigin(otMessage *aMessage, otMessageOrigin aOrigin);

/**
 * Sets/forces the message to be forwarded using direct transmission.
 * Default setting for a new message is `false`.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aEnabled  If `true`, the message is forced to use direct transmission. If `false`, the message follows
 *                       the normal procedure.
 */
void otMessageSetDirectTransmission(otMessage *aMessage, bool aEnabled);

/**
 * Returns the average RSS (received signal strength) associated with the message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The average RSS value (in dBm) or OT_RADIO_RSSI_INVALID if no average RSS is available.
 */
int8_t otMessageGetRss(const otMessage *aMessage);

/**
 * Retrieves the link-specific information for a message received over Thread radio.
 *
 * @param[in] aMessage    The message from which to retrieve `otThreadLinkInfo`.
 * @pram[out] aLinkInfo   A pointer to an `otThreadLinkInfo` to populate.
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the link info, @p `aLinkInfo` is updated.
 * @retval OT_ERROR_NOT_FOUND  Message origin is not `OT_MESSAGE_ORIGIN_THREAD_NETIF`.
 */
otError otMessageGetThreadLinkInfo(const otMessage *aMessage, otThreadLinkInfo *aLinkInfo);

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
uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength);

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
 * Represents an OpenThread message queue.
 */
typedef struct
{
    void *mData; ///< Opaque data used by the implementation.
} otMessageQueue;

/**
 * Represents information about a message queue.
 */
typedef struct otMessageQueueInfo
{
    uint16_t mNumMessages; ///< Number of messages in the queue.
    uint16_t mNumBuffers;  ///< Number of data buffers used by messages in the queue.
    uint32_t mTotalBytes;  ///< Total number of bytes used by all messages in the queue.
} otMessageQueueInfo;

/**
 * Represents the message buffer information for different queues used by OpenThread stack.
 */
typedef struct otBufferInfo
{
    uint16_t mTotalBuffers; ///< The total number of buffers in the messages pool (0xffff if unknown).
    uint16_t mFreeBuffers;  ///< The number of free buffers (0xffff if unknown).

    /**
     * The maximum number of used buffers at the same time since OT stack initialization or last call to
     * `otMessageResetBufferInfo()`.
     */
    uint16_t mMaxUsedBuffers;

    otMessageQueueInfo m6loSendQueue;         ///< Info about 6LoWPAN send queue.
    otMessageQueueInfo m6loReassemblyQueue;   ///< Info about 6LoWPAN reassembly queue.
    otMessageQueueInfo mIp6Queue;             ///< Info about IPv6 send queue.
    otMessageQueueInfo mMplQueue;             ///< Info about MPL send queue.
    otMessageQueueInfo mMleQueue;             ///< Info about MLE delayed message queue.
    otMessageQueueInfo mCoapQueue;            ///< Info about CoAP/TMF send queue.
    otMessageQueueInfo mCoapSecureQueue;      ///< Info about CoAP secure send queue.
    otMessageQueueInfo mApplicationCoapQueue; ///< Info about application CoAP send queue.
} otBufferInfo;

/**
 * Initialize the message queue.
 *
 * MUST be called once and only once for a `otMessageQueue` instance before any other `otMessageQueue`
 * functions. The behavior is undefined if other queue APIs are used with an `otMessageQueue` before it being
 * initialized or if it is initialized more than once.
 *
 * @param[in]  aQueue     A pointer to a message queue.
 */
void otMessageQueueInit(otMessageQueue *aQueue);

/**
 * Adds a message to the end of the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to add.
 */
void otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage *aMessage);

/**
 * Adds a message at the head/front of the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to add.
 */
void otMessageQueueEnqueueAtHead(otMessageQueue *aQueue, otMessage *aMessage);

/**
 * Removes a message from the given message queue.
 *
 * @param[in]  aQueue    A pointer to the message queue.
 * @param[in]  aMessage  The message to remove.
 */
void otMessageQueueDequeue(otMessageQueue *aQueue, otMessage *aMessage);

/**
 * Returns a pointer to the message at the head of the queue.
 *
 * @param[in]  aQueue    A pointer to a message queue.
 *
 * @returns  A pointer to the message at the head of queue or NULL if queue is empty.
 */
otMessage *otMessageQueueGetHead(otMessageQueue *aQueue);

/**
 * Returns a pointer to the next message in the queue by iterating forward (from head to tail).
 *
 * @param[in]  aQueue    A pointer to a message queue.
 * @param[in]  aMessage  A pointer to current message buffer.
 *
 * @returns  A pointer to the next message in the queue after `aMessage` or NULL if `aMessage is the tail of queue.
 *           NULL is returned if `aMessage` is not in the queue `aQueue`.
 */
otMessage *otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage *aMessage);

/**
 * Get the Message Buffer information.
 *
 * @param[in]   aInstance    A pointer to the OpenThread instance.
 * @param[out]  aBufferInfo  A pointer where the message buffer information is written.
 */
void otMessageGetBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo);

/**
 * Reset the Message Buffer information counter tracking the maximum number buffers in use at the same time.
 *
 * This resets `mMaxUsedBuffers` in `otBufferInfo`.
 *
 * @param[in]   aInstance    A pointer to the OpenThread instance.
 */
void otMessageResetBufferInfo(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MESSAGE_H_
