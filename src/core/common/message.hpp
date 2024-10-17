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
 *   This file includes definitions for the message buffer pool and message buffers.
 */

#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/message.h>
#include <openthread/nat64.h>
#include <openthread/platform/messagepool.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/const_cast.hpp"
#include "common/data.hpp"
#include "common/encoding.hpp"
#include "common/iterator_utils.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/offset_range.hpp"
#include "common/pool.hpp"
#include "common/timer.hpp"
#include "common/type_traits.hpp"
#include "mac/mac_types.hpp"
#include "thread/child_mask.hpp"
#include "thread/link_quality.hpp"

/**
 * Represents an opaque (and empty) type for an OpenThread message buffer.
 */
struct otMessage
{
};

namespace ot {

namespace Crypto {

class AesCcm;
class Sha256;
class HmacSha256;

} // namespace Crypto

/**
 * @addtogroup core-message
 *
 * @brief
 *   This module includes definitions for the message buffer pool and message buffers.
 *
 * @{
 */

/**
 * Frees a given message buffer if not `nullptr`.
 *
 * And the ones that follow contain small but common code patterns used in many of the core modules. They
 * are intentionally defined as macros instead of inline methods/functions to ensure that they are fully inlined.
 * Note that an `inline` method/function is not necessarily always inlined by the toolchain and not inlining such
 * small implementations can add a rather large code-size overhead.
 *
 * @param[in] aMessage    A pointer to a `Message` to free (can be `nullptr`).
 */
#define FreeMessage(aMessage)      \
    do                             \
    {                              \
        if ((aMessage) != nullptr) \
        {                          \
            (aMessage)->Free();    \
        }                          \
    } while (false)

/**
 * Frees a given message buffer if a given `Error` indicates an error.
 *
 * The parameter @p aMessage can be `nullptr` in which case this macro does nothing.
 *
 * @param[in] aMessage    A pointer to a `Message` to free (can be `nullptr`).
 * @param[in] aError      The `Error` to check.
 */
#define FreeMessageOnError(aMessage, aError)                     \
    do                                                           \
    {                                                            \
        if (((aError) != kErrorNone) && ((aMessage) != nullptr)) \
        {                                                        \
            (aMessage)->Free();                                  \
        }                                                        \
    } while (false)

/**
 * Frees a given message buffer if a given `Error` indicates an error and sets the `aMessage` to `nullptr`.
 *
 * @param[in] aMessage    A pointer to a `Message` to free (can be `nullptr`).
 * @param[in] aError      The `Error` to check.
 */
#define FreeAndNullMessageOnError(aMessage, aError)              \
    do                                                           \
    {                                                            \
        if (((aError) != kErrorNone) && ((aMessage) != nullptr)) \
        {                                                        \
            (aMessage)->Free();                                  \
            (aMessage) = nullptr;                                \
        }                                                        \
    } while (false)

constexpr uint16_t kNumBuffers = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;
constexpr uint16_t kBufferSize = OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE;

class Message;
class MessagePool;
class MessageQueue;
class PriorityQueue;
class ThreadLinkInfo;

/**
 * Represents a Message buffer.
 */
class Buffer : public otMessageBuffer, public LinkedListEntry<Buffer>
{
    friend class Message;

public:
    /**
     * Returns a pointer to the next message buffer.
     *
     * @returns A pointer to the next message buffer.
     */
    Buffer *GetNextBuffer(void) { return GetNext(); }

    /**
     * Returns a pointer to the next message buffer.
     *
     * @returns A pointer to the next message buffer.
     */
    const Buffer *GetNextBuffer(void) const { return GetNext(); }

    /**
     * Sets the pointer to the next message buffer.
     *
     * @param[in] aNext  A pointer to the next buffer.
     */
    void SetNextBuffer(Buffer *aNext) { SetNext(aNext); }

protected:
    struct Metadata
    {
        bool mDirectTx : 1;            // Whether a direct transmission is required.
        bool mLinkSecurity : 1;        // Whether link security is enabled.
        bool mInPriorityQ : 1;         // Whether the message is queued in normal or priority queue.
        bool mTxSuccess : 1;           // Whether the direct tx of the message was successful.
        bool mDoNotEvict : 1;          // Whether this message may be evicted.
        bool mMulticastLoop : 1;       // Whether this multicast message may be looped back.
        bool mResolvingAddress : 1;    // Whether the message is pending an address query resolution.
        bool mAllowLookbackToHost : 1; // Whether the message is allowed to be looped back to host.
        bool mIsDstPanIdBroadcast : 1; // Whether the dest PAN ID is broadcast.
#if OPENTHREAD_CONFIG_MULTI_RADIO
        bool mIsRadioTypeSet : 1; // Whether the radio type is set.
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        bool mTimeSync : 1; // Whether the message is also used for time sync purpose.
#endif
        uint8_t mPriority : 2; // The message priority level (higher value is higher priority).
        uint8_t mOrigin : 2;   // The origin of the message.
#if OPENTHREAD_CONFIG_MULTI_RADIO
        uint8_t mRadioType : 2; // The radio link type the message was received on, or should be sent on.
        static_assert(Mac::kNumRadioTypes <= (1 << 2), "mRadioType bitfield cannot store all radio type values");
#endif
        uint8_t mType : 3;    // The message type.
        uint8_t mSubType : 4; // The message sub type.
        uint8_t mMleCommand;  // The MLE command type (used when `mSubType is `Mle`).
        uint8_t mChannel;     // The message channel (used for MLE Announce).
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        uint8_t mTimeSyncSeq; // The time sync sequence.
#endif
        uint16_t mLength;      // Current message length (number of bytes).
        uint16_t mOffset;      // A byte offset within the message.
        uint16_t mReserved;    // Number of reserved bytes (for header).
        uint16_t mMeshDest;    // Used for unicast non-link-local messages.
        uint16_t mPanId;       // PAN ID (used for MLE Discover Request and Response).
        uint32_t mDatagramTag; // The datagram tag used for 6LoWPAN frags or IPv6fragmentation.
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        int64_t mNetworkTimeOffset; // The time offset to the Thread network time, in microseconds.
#endif
        TimeMilli    mTimestamp;   // The message timestamp.
        Message     *mNext;        // Next message in a doubly linked list.
        Message     *mPrev;        // Previous message in a doubly linked list.
        MessagePool *mMessagePool; // Message pool for this message.
        void        *mQueue;       // The queue where message is queued (if any). Queue type from `mInPriorityQ`.
        RssAverager  mRssAverager; // The averager maintaining the received signal strength (RSS) average.
        LqiAverager  mLqiAverager; // The averager maintaining the Link quality indicator (LQI) average.
#if OPENTHREAD_FTD
        ChildMask mChildMask; // ChildMask to indicate which sleepy children need to receive this.
#endif
    };

    static_assert(kBufferSize > sizeof(Metadata) + sizeof(otMessageBuffer), "Metadata does not fit in a single buffer");

    static constexpr uint16_t kBufferDataSize     = kBufferSize - sizeof(otMessageBuffer);
    static constexpr uint16_t kHeadBufferDataSize = kBufferDataSize - sizeof(Metadata);

    Metadata       &GetMetadata(void) { return mBuffer.mHead.mMetadata; }
    const Metadata &GetMetadata(void) const { return mBuffer.mHead.mMetadata; }

    uint8_t       *GetFirstData(void) { return mBuffer.mHead.mData; }
    const uint8_t *GetFirstData(void) const { return mBuffer.mHead.mData; }

    uint8_t       *GetData(void) { return mBuffer.mData; }
    const uint8_t *GetData(void) const { return mBuffer.mData; }

private:
    union
    {
        struct
        {
            Metadata mMetadata;
            uint8_t  mData[kHeadBufferDataSize];
        } mHead;
        uint8_t mData[kBufferDataSize];
    } mBuffer;
};

static_assert(sizeof(Buffer) >= kBufferSize,
              "Buffer size is not valid. Increase OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE.");

/**
 * Represents a message.
 */
class Message : public otMessage, public Buffer, public GetProvider<Message>
{
    friend class Checksum;
    friend class Crypto::HmacSha256;
    friend class Crypto::Sha256;
    friend class Crypto::AesCcm;
    friend class MessagePool;
    friend class MessageQueue;
    friend class PriorityQueue;

public:
    /**
     * Represents the message type.
     */
    enum Type : uint8_t
    {
        kTypeIp6          = 0, ///< A full uncompressed IPv6 packet
        kType6lowpan      = 1, ///< A 6lowpan frame
        kTypeSupervision  = 2, ///< A child supervision frame.
        kTypeMacEmptyData = 3, ///< An empty MAC data frame.
        kTypeIp4          = 4, ///< A full uncompressed IPv4 packet, for NAT64.
        kTypeBle          = 5, ///< A BLE payload message.
        kTypeOther        = 6, ///< Other (data) message.
    };

    /**
     * Represents the message sub-type.
     */
    enum SubType : uint8_t
    {
        kSubTypeNone                   = 0, ///< None
        kSubTypeMle                    = 1, ///< MLE message
        kSubTypeMplRetransmission      = 2, ///< MPL next retransmission message
        kSubTypeJoinerEntrust          = 3, ///< Joiner Entrust
        kSubTypeJoinerFinalizeResponse = 4, ///< Joiner Finalize Response

    };

    enum Priority : uint8_t
    {
        kPriorityLow    = OT_MESSAGE_PRIORITY_LOW,      ///< Low priority level.
        kPriorityNormal = OT_MESSAGE_PRIORITY_NORMAL,   ///< Normal priority level.
        kPriorityHigh   = OT_MESSAGE_PRIORITY_HIGH,     ///< High priority level.
        kPriorityNet    = OT_MESSAGE_PRIORITY_HIGH + 1, ///< Network Control priority level.
    };

    static constexpr uint8_t kNumPriorities = 4; ///< Number of priority levels.

    /**
     * Represents the link security mode (used by `Settings` constructor).
     */
    enum LinkSecurityMode : bool
    {
        kNoLinkSecurity   = false, ///< Link security disabled (no link security).
        kWithLinkSecurity = true,  ///< Link security enabled.
    };

    /**
     * Represents the message ownership model when a `Message` instance is passed to a method/function.
     */
    enum Ownership : uint8_t
    {
        /**
         * This value indicates that the method/function receiving a `Message` instance should take custody of the
         * message (e.g., the method should `Free()` the message if no longer needed).
         */
        kTakeCustody,

        /**
         * This value indicates that the method/function receiving a `Message` instance does not own the message (e.g.,
         * it should not `Free()` or `Enqueue()` it in a queue). The receiving method/function should create a
         * copy/clone of the message to keep (if/when needed).
         */
        kCopyToUse,
    };

    /**
     * Represents an IPv6 message origin.
     */
    enum Origin : uint8_t
    {
        kOriginThreadNetif   = OT_MESSAGE_ORIGIN_THREAD_NETIF,   // Message from Thread Netif.
        kOriginHostTrusted   = OT_MESSAGE_ORIGIN_HOST_TRUSTED,   // Message from a trusted source on host.
        kOriginHostUntrusted = OT_MESSAGE_ORIGIN_HOST_UNTRUSTED, // Message from an untrusted source on host.
    };

    /**
     * Represents settings used for creating a new message.
     */
    class Settings : public otMessageSettings
    {
    public:
        /**
         * Initializes the `Settings` object.
         *
         * @param[in]  aSecurityMode  A link security mode.
         * @param[in]  aPriority      A message priority.
         */
        Settings(LinkSecurityMode aSecurityMode, Priority aPriority);

        /**
         * Initializes the `Settings` with a given message priority and link security enabled.
         *
         * @param[in]  aPriority      A message priority.
         */
        explicit Settings(Priority aPriority)
            : Settings(kWithLinkSecurity, aPriority)
        {
        }

        /**
         * Gets the message priority.
         *
         * @returns The message priority.
         */
        Priority GetPriority(void) const { return static_cast<Priority>(mPriority); }

        /**
         * Indicates whether the link security should be enabled.
         *
         * @returns TRUE if link security should be enabled, FALSE otherwise.
         */
        bool IsLinkSecurityEnabled(void) const { return mLinkSecurityEnabled; }

        /**
         * Converts a pointer to an `otMessageSettings` to a `Settings`.
         *
         * @param[in] aSettings  A pointer to `otMessageSettings` to convert from.
         *                       If it is `nullptr`, then the default settings `GetDefault()` will be used.
         *
         * @returns A reference to the converted `Settings` or the default if @p aSettings is `nullptr`.
         */
        static const Settings &From(const otMessageSettings *aSettings);

        /**
         * Returns the default settings with link security enabled and `kPriorityNormal` priority.
         *
         * @returns A reference to the default settings (link security enable and `kPriorityNormal` priority).
         */
        static const Settings &GetDefault(void) { return static_cast<const Settings &>(kDefault); }

    private:
        static const otMessageSettings kDefault;
    };

    /**
     * Represents footer data appended to the end of a `Message`.
     *
     * This data typically represents some metadata associated with the `Message` that is appended to its end. It can
     * be read later from the message, updated (re-written) in the message, or fully removed from it.
     *
     * Users of `FooterData` MUST follow CRTP-style inheritance, i.e., the `DataType` itself MUST publicly inherit
     * from `FooterData<DataType>`.
     *
     * @tparam DataType   The footer data type.
     */
    template <typename DataType> class FooterData
    {
    public:
        /**
         * Appends the footer data to the end of a given message.
         *
         * @param[in,out] aMessage   The message to append to.
         *
         * @retval kErrorNone    Successfully appended the footer data.
         * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
         */
        Error AppendTo(Message &aMessage) const { return aMessage.Append<DataType>(AsDataType()); }

        /**
         * Reads the footer data from a given message.
         *
         * Caller MUST ensure data was successfully appended to the message beforehand. Otherwise behavior is undefined.
         *
         * @param[in] aMessage   The message to read from.
         */
        void ReadFrom(const Message &aMessage)
        {
            IgnoreError(aMessage.Read<DataType>(aMessage.GetLength() - sizeof(DataType), AsDataType()));
        }

        /**
         * Updates the footer data in a given message (rewriting over the previously appended data).
         *
         * Caller MUST ensure data was successfully appended to the message beforehand. Otherwise behavior is undefined.
         *
         * @param[in,out] aMessage   The message to update.
         */
        void UpdateIn(Message &aMessage) const
        {
            aMessage.Write<DataType>(aMessage.GetLength() - sizeof(DataType), AsDataType());
        }

        /**
         * Removes the footer data from a given message.
         *
         * Caller MUST ensure data was successfully appended to the message beforehand. Otherwise behavior is undefined.
         *
         * @param[in,out] aMessage   The message to remove the data from.
         */
        void RemoveFrom(Message &aMessage) const { aMessage.RemoveFooter(sizeof(DataType)); }

    protected:
        FooterData(void) = default;

    private:
        const DataType &AsDataType(void) const { return static_cast<const DataType &>(*this); }
        DataType       &AsDataType(void) { return static_cast<DataType &>(*this); }
    };

    /**
     * Returns a reference to the OpenThread Instance which owns the `Message`.
     *
     * @returns A reference to the `Instance`.
     */
    Instance &GetInstance(void) const;

    /**
     * Frees this message buffer.
     */
    void Free(void);

    /**
     * Returns a pointer to the next message.
     *
     * @returns A pointer to the next message in the list or `nullptr` if at the end of the list.
     */
    Message *GetNext(void) const;

    /**
     * Returns the number of bytes in the message.
     *
     * @returns The number of bytes in the message.
     */
    uint16_t GetLength(void) const { return GetMetadata().mLength; }

    /**
     * Sets the number of bytes in the message.
     *
     * @param[in]  aLength  Requested number of bytes in the message.
     *
     * @retval kErrorNone    Successfully set the length of the message.
     * @retval kErrorNoBufs  Failed to grow the size of the message because insufficient buffers were available.
     */
    Error SetLength(uint16_t aLength);

    /**
     * Returns the number of buffers in the message.
     */
    uint8_t GetBufferCount(void) const;

    /**
     * Returns the byte offset within the message.
     *
     * @returns A byte offset within the message.
     */
    uint16_t GetOffset(void) const { return GetMetadata().mOffset; }

    /**
     * Moves the byte offset within the message.
     *
     * @param[in]  aDelta  The number of bytes to move the current offset, which may be positive or negative.
     */
    void MoveOffset(int aDelta);

    /**
     * Sets the byte offset within the message.
     *
     * @param[in]  aOffset  The byte offset within the message.
     */
    void SetOffset(uint16_t aOffset);

    /**
     * Returns the type of the message.
     *
     * @returns The type of the message.
     */
    Type GetType(void) const { return static_cast<Type>(GetMetadata().mType); }

    /**
     * Sets the message type.
     *
     * @param[in]  aType  The message type.
     */
    void SetType(Type aType) { GetMetadata().mType = aType; }

    /**
     * Returns the sub type of the message.
     *
     * @returns The sub type of the message.
     */
    SubType GetSubType(void) const { return static_cast<SubType>(GetMetadata().mSubType); }

    /**
     * Sets the message sub type.
     *
     * @param[in]  aSubType  The message sub type.
     */
    void SetSubType(SubType aSubType) { GetMetadata().mSubType = aSubType; }

    /**
     * Indicates whether or not the message is of MLE sub type.
     *
     * @retval TRUE   The message is of MLE sub type.
     * @retval FALSE  The message is not of MLE sub type.
     */
    bool IsSubTypeMle(void) const { return (GetSubType() == kSubTypeMle); }

    /**
     * Indicates whether or not the message is a given MLE command.
     *
     * It checks `IsSubTypeMle()` and then if `GetMleCommand()` is the same as `aMleCommand`.
     *
     * @param[in] aMleCommand  The MLE command type.
     *
     * @retval TRUE  The message is an MLE command of @p aMleCommand type.
     * @retval FALSE The message is not an MLE command of @p aMleCommand type.
     */
    bool IsMleCommand(Mle::Command aMleCommand) const;

    /**
     * Gets the MLE command type.
     *
     * Caller MUST ensure that message sub type is `kSubTypeMle` before calling this method. Otherwise the returned
     * value is not meaningful.
     *
     * @returns The message's MLE command type.
     */
    Mle::Command GetMleCommand(void) const { return static_cast<Mle::Command>(GetMetadata().mMleCommand); }

    /**
     * Set the MLE command type of message.
     *
     * Caller should also set the sub type to `kSubTypeMle`.
     *
     * @param[in] aMleCommand  The MLE command type.
     */
    void SetMleCommand(Mle::Command aMleCommand) { GetMetadata().mMleCommand = aMleCommand; }

    /**
     * Checks whether this multicast message may be looped back.
     *
     * @retval TRUE   If message may be looped back.
     * @retval FALSE  If message must not be looped back.
     */
    bool GetMulticastLoop(void) const { return GetMetadata().mMulticastLoop; }

    /**
     * Sets whether multicast may be looped back.
     *
     * @param[in]  aMulticastLoop  Whether allow looping back multicast.
     */
    void SetMulticastLoop(bool aMulticastLoop) { GetMetadata().mMulticastLoop = aMulticastLoop; }

    /**
     * Returns the message priority level.
     *
     * @returns The priority level associated with this message.
     */
    Priority GetPriority(void) const { return static_cast<Priority>(GetMetadata().mPriority); }

    /**
     * Sets the messages priority.
     * If the message is already queued in a priority queue, changing the priority ensures to
     * update the message in the associated queue.
     *
     * @param[in]  aPriority  The message priority level.
     *
     * @retval kErrorNone          Successfully set the priority for the message.
     * @retval kErrorInvalidArgs   Priority level is not invalid.
     */
    Error SetPriority(Priority aPriority);

    /**
     * Convert a `Priority` to a string.
     *
     * @param[in] aPriority  The priority level.
     *
     * @returns A string representation of @p aPriority.
     */
    static const char *PriorityToString(Priority aPriority);

    /**
     * Prepends bytes to the front of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer (can be `nullptr` to grow message without writing bytes).
     * @param[in]  aLength  The number of bytes to prepend.
     *
     * @retval kErrorNone    Successfully prepended the bytes.
     * @retval kErrorNoBufs  Not enough reserved bytes in the message.
     */
    Error PrependBytes(const void *aBuf, uint16_t aLength);

    /**
     * Prepends an object to the front of the message.
     *
     * On success, this method grows the message by the size of the object.
     *
     * @tparam    ObjectType   The object type to prepend to the message.
     *
     * @param[in] aObject      A reference to the object to prepend to the message.
     *
     * @retval kErrorNone    Successfully prepended the object.
     * @retval kErrorNoBufs  Not enough reserved bytes in the message.
     */
    template <typename ObjectType> Error Prepend(const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return PrependBytes(&aObject, sizeof(ObjectType));
    }

    /**
     * Removes header bytes from the message at start of message.
     *
     * The caller MUST ensure that message contains the bytes to be removed, i.e. `aOffset` is smaller than the message
     * length.
     *
     * @param[in]  aLength  Number of header bytes to remove from start of `Message`.
     */
    void RemoveHeader(uint16_t aLength);

    /**
     * Removes header bytes from the message at a given offset.
     *
     * Shrinks the message. The existing header bytes before @p aOffset are copied forward and replace the
     * removed bytes.
     *
     * The caller MUST ensure that message contains the bytes to be removed, i.e. `aOffset + aLength` is smaller than
     * the message length.
     *
     * @param[in]  aOffset  The offset to start removing.
     * @param[in]  aLength  Number of header bytes to remove.
     */
    void RemoveHeader(uint16_t aOffset, uint16_t aLength);

    /**
     * Grows the message to make space for new header bytes at a given offset.
     *
     * Grows the message header (similar to `PrependBytes()`). The existing header bytes from start to
     * `aOffset + aLength` are then copied backward to make room for the new header bytes. Note that this method does
     * not change the bytes from @p aOffset up @p aLength (the new inserted header range). Caller can write to this
     * range to update the bytes after successful return from this method.
     *
     * @param[in] aOffset   The offset at which to insert the header bytes
     * @param[in] aLength   Number of header bytes to insert.
     *
     * @retval kErrorNone    Successfully grown the message and copied the existing header bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    Error InsertHeader(uint16_t aOffset, uint16_t aLength);

    /**
     * Removes footer bytes from the end of the message.
     *
     * The caller should ensure the message contains the bytes to be removed, otherwise as many bytes as available
     * will be removed.
     *
     * @param[in] aLength   Number of footer bytes to remove from end of the `Message`.
     */
    void RemoveFooter(uint16_t aLength);

    /**
     * Appends bytes to the end of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer (MUST not be `nullptr`).
     * @param[in]  aLength  The number of bytes to append.
     *
     * @retval kErrorNone    Successfully appended the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    Error AppendBytes(const void *aBuf, uint16_t aLength);

    /**
     * Appends bytes read from another or potentially the same message to the end of the current message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in] aMessage   The message to read the bytes from (it can be the same as the current message).
     * @param[in] aOffset    The offset in @p aMessage to start reading the bytes from.
     * @param[in] aLength    The number of bytes to read from @p aMessage and append.
     *
     * @retval kErrorNone    Successfully appended the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     * @retval kErrorParse   Not enough bytes in @p aMessage to read @p aLength bytes from @p aOffset.
     */
    Error AppendBytesFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

    /**
     * Appends bytes read from another or potentially the same message to the end of the current message.
     *
     * @param[in] aMessage     The message to read the bytes from (it can be the same as the current message).
     * @param[in] aOffsetRange The offset range in @p aMessage to read bytes from.
     *
     * @retval kErrorNone    Successfully appended the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     * @retval kErrorParse   Not enough bytes in @p aMessage to read @p aOffsetRange.
     */
    Error AppendBytesFromMessage(const Message &aMessage, const OffsetRange &aOffsetRange);

    /**
     * Appends an object to the end of the message.
     *
     * On success, this method grows the message by the size of the appended object
     *
     * @tparam    ObjectType   The object type to append to the message.
     *
     * @param[in] aObject      A reference to the object to append to the message.
     *
     * @retval kErrorNone    Successfully appended the object.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    template <typename ObjectType> Error Append(const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return AppendBytes(&aObject, sizeof(ObjectType));
    }

    /**
     * Appends bytes from a given `Data` instance to the end of the message.
     *
     * On success, this method grows the message by the size of the appended data.
     *
     * @tparam    kDataLengthType   Determines the data length type (`uint8_t` or `uint16_t`).
     *
     * @param[in] aData      A reference to `Data` to append to the message.
     *
     * @retval kErrorNone    Successfully appended the bytes from @p aData.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    template <DataLengthType kDataLengthType> Error AppendData(const Data<kDataLengthType> &aData)
    {
        return AppendBytes(aData.GetBytes(), aData.GetLength());
    }

    /**
     * Reads bytes from the message.
     *
     * The provided buffer @p aBuf MUST contain at least @p aLength bytes.
     *
     * If there are fewer bytes available in the message than the requested @p aLength, the available bytes are read
     * and copied into @p aBuf. This method returns the actual number of bytes successfully read from the message and
     * written into @p aBuf.
     *
     * @param[in]  aOffset  Byte offset within the message to begin reading.
     * @param[out] aBuf     A pointer to a data buffer to copy the read bytes into.
     * @param[in]  aLength  Number of bytes to read.
     *
     * @returns The number of bytes read.
     */
    uint16_t ReadBytes(uint16_t aOffset, void *aBuf, uint16_t aLength) const;

    /**
     * Reads bytes from the message.
     *
     * If there are fewer bytes available in the message than the provided length in @p aOffsetRange, the available
     * bytes are read and copied into @p aBuf. This method returns the actual number of bytes successfully read from
     * the message and written into @p aBuf.
     *
     * @param[in]  aOffsetRange  The offset range in the message to read bytes from.
     * @param[out] aBuf          A pointer to a data buffer to copy the read bytes into.
     *
     * @returns The number of bytes read.
     */
    uint16_t ReadBytes(const OffsetRange &aOffsetRange, void *aBuf) const;

    /**
     * Reads a given number of bytes from the message.
     *
     * @param[in]  aOffset  Byte offset within the message to begin reading.
     * @param[out] aBuf     A pointer to a data buffer to copy the read bytes into.
     * @param[in]  aLength  Number of bytes to read.
     *
     * @retval kErrorNone     @p aLength bytes were successfully read from message.
     * @retval kErrorParse    Not enough bytes remaining in message to read the entire object.
     */
    Error Read(uint16_t aOffset, void *aBuf, uint16_t aLength) const;

    /**
     * Reads a given number of bytes from the message.
     *
     * @param[in]  aOffsetRange  The offset range in the message to read from.
     * @param[out] aBuf          A pointer to a data buffer to copy the read bytes into.
     * @param[in]  aLength       Number of bytes to read.
     *
     * @retval kErrorNone     Requested bytes were successfully read from message.
     * @retval kErrorParse    Not enough bytes remaining to read the requested @p aLength.
     */
    Error Read(const OffsetRange &aOffsetRange, void *aBuf, uint16_t aLength) const;

    /**
     * Reads an object from the message.
     *
     * @tparam     ObjectType   The object type to read from the message.
     *
     * @param[in]  aOffset      Byte offset within the message to begin reading.
     * @param[out] aObject      A reference to the object to read into.
     *
     * @retval kErrorNone     Object @p aObject was successfully read from message.
     * @retval kErrorParse    Not enough bytes remaining in message to read the entire object.
     */
    template <typename ObjectType> Error Read(uint16_t aOffset, ObjectType &aObject) const
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return Read(aOffset, &aObject, sizeof(ObjectType));
    }

    /**
     * Reads an object from the message.
     *
     * @tparam     ObjectType   The object type to read from the message.
     *
     * @param[in]  aOffsetRange  The offset range in the message to read from.
     * @param[out] aObject       A reference to the object to read into.
     *
     * @retval kErrorNone     Object @p aObject was successfully read from message.
     * @retval kErrorParse    Not enough bytes remaining in message to read the entire object.
     */
    template <typename ObjectType> Error Read(const OffsetRange &aOffsetRange, ObjectType &aObject) const
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return Read(aOffsetRange, &aObject, sizeof(ObjectType));
    }

    /**
     * Compares the bytes in the message at a given offset with a given byte array.
     *
     * If there are fewer bytes available in the message than the requested @p aLength, the comparison is treated as
     * failure (returns FALSE).
     *
     * @param[in]  aOffset    Byte offset within the message to read from for the comparison.
     * @param[in]  aBuf       A pointer to a data buffer to compare with the bytes from message.
     * @param[in]  aLength    Number of bytes in @p aBuf.
     * @param[in]  aMatcher   A `ByteMatcher` function pointer to match the bytes. If `nullptr` then bytes are directly
     *                        compared.
     *
     * @returns TRUE if there are enough bytes available in @p aMessage and they match the bytes from @p aBuf,
     *          FALSE otherwise.
     */
    bool CompareBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength, ByteMatcher aMatcher = nullptr) const;

    /**
     * Compares the bytes in the message at a given offset with bytes read from another message.
     *
     * If either message has fewer bytes available than the requested @p aLength, the comparison is treated as failure
     * (returns FALSE).
     *
     * @param[in]  aOffset        Byte offset within the message to read from for the comparison.
     * @param[in]  aOtherMessage  The other message to compare with.
     * @param[in]  aOtherOffset   Byte offset within @p aOtherMessage to read from for the comparison.
     * @param[in]  aLength        Number of bytes to compare.
     * @param[in]  aMatcher       A `ByteMatcher` function pointer to match the bytes. If `nullptr` then bytes are
     *                            directly compared.
     *
     * @returns TRUE if there are enough bytes available in both messages and they all match. FALSE otherwise.
     */
    bool CompareBytes(uint16_t       aOffset,
                      const Message &aOtherMessage,
                      uint16_t       aOtherOffset,
                      uint16_t       aLength,
                      ByteMatcher    aMatcher = nullptr) const;

    /**
     * Compares the bytes in the message at a given offset with an object.
     *
     * The bytes in the message are compared with the bytes in @p aObject. If there are fewer bytes available in the
     * message than the requested object size, it is treated as failed comparison (returns FALSE).
     *
     * @tparam     ObjectType   The object type to compare with the bytes in message.
     *
     * @param[in] aOffset      Byte offset within the message to read from for the comparison.
     * @param[in] aObject      A reference to the object to compare with the message bytes.
     *
     * @returns TRUE if there are enough bytes available in @p aMessage and they match the bytes in @p aObject,
     *          FALSE otherwise.
     */
    template <typename ObjectType> bool Compare(uint16_t aOffset, const ObjectType &aObject) const
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return CompareBytes(aOffset, &aObject, sizeof(ObjectType));
    }

    /**
     * Writes bytes to the message.
     *
     * Will not resize the message. The given data to write (with @p aLength bytes) MUST fit within the
     * existing message buffer (from the given offset @p aOffset up to the message's length).
     *
     * @param[in]  aOffset  Byte offset within the message to begin writing.
     * @param[in]  aBuf     A pointer to a data buffer.
     * @param[in]  aLength  Number of bytes to write.
     */
    void WriteBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength);

    /**
     * Writes bytes read from another or potentially the same message to the message at a given offset.
     *
     * Will not resize the message. The bytes to write (with @p aLength) MUST fit within the existing
     * message buffer (from the given @p aWriteOffset up to the message's length).
     *
     * Can be used to copy bytes within the same message in either direction, i.e., copy forward where
     * `aWriteOffset > aReadOffset` or copy backward where `aWriteOffset < aReadOffset`.
     *
     * @param[in] aWriteOffset  Byte offset within this message to begin writing.
     * @param[in] aMessage      The message to read the bytes from.
     * @param[in] aReadOffset   The offset in @p aMessage to start reading the bytes from.
     * @param[in] aLength       The number of bytes to read from @p aMessage and write.
     */
    void WriteBytesFromMessage(uint16_t aWriteOffset, const Message &aMessage, uint16_t aReadOffset, uint16_t aLength);

    /**
     * Writes an object to the message.
     *
     * Will not resize the message. The entire given object (all its bytes) MUST fit within the existing
     * message buffer (from the given offset @p aOffset up to the message's length).
     *
     * @tparam     ObjectType   The object type to write to the message.
     *
     * @param[in]  aOffset      Byte offset within the message to begin writing.
     * @param[in]  aObject      A reference to the object to write.
     */
    template <typename ObjectType> void Write(uint16_t aOffset, const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        WriteBytes(aOffset, &aObject, sizeof(ObjectType));
    }

    /**
     * Writes bytes from a given `Data` instance to the message.
     *
     * Will not resize the message. The given data to write MUST fit within the existing message buffer
     * (from the given offset @p aOffset up to the message's length).
     *
     * @tparam     kDataLengthType   Determines the data length type (`uint8_t` or `uint16_t`).
     *
     * @param[in]  aOffset    Byte offset within the message to begin writing.
     * @param[in]  aData      The `Data` to write to the message.
     */
    template <DataLengthType kDataLengthType> void WriteData(uint16_t aOffset, const Data<kDataLengthType> &aData)
    {
        WriteBytes(aOffset, aData.GetBytes(), aData.GetLength());
    }

    /**
     * Creates a copy of the message.
     *
     * It allocates the new message from the same message pool as the original one and copies @p aLength octets
     * of the payload. The `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the
     * cloned message are also copied from the original one.
     *
     * @param[in] aLength  Number of payload bytes to copy.
     *
     * @returns A pointer to the message or nullptr if insufficient message buffers are available.
     */
    Message *Clone(uint16_t aLength) const;

    /**
     * Creates a copy of the message.
     *
     * It allocates the new message from the same message pool as the original one and copies the entire payload. The
     * `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the cloned message are also
     * copied from the original one.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *Clone(void) const { return Clone(GetLength()); }

    /**
     * Returns the datagram tag used for 6LoWPAN fragmentation or the identification used for IPv6
     * fragmentation.
     *
     * @returns The 6LoWPAN datagram tag or the IPv6 fragment identification.
     */
    uint32_t GetDatagramTag(void) const { return GetMetadata().mDatagramTag; }

    /**
     * Sets the datagram tag used for 6LoWPAN fragmentation.
     *
     * @param[in]  aTag  The 6LoWPAN datagram tag.
     */
    void SetDatagramTag(uint32_t aTag) { GetMetadata().mDatagramTag = aTag; }

#if OPENTHREAD_FTD
    /**
     * Gets the indirect transmission `ChildMask` associated with this `Message`.
     *
     * The `ChildMask` indicates the set of children for which this message is scheduled for indirect transmission.
     *
     * @returns A reference to the indirect transmission `ChildMask`.
     */
    ChildMask &GetIndirectTxChildMask(void) { return GetMetadata().mChildMask; }

    /**
     * Gets the indirect transmission `ChildMask` associated with this `Message`.
     *
     * The `ChildMask` indicates the set of children for which this message is scheduled for indirect transmission.
     *
     * @returns A reference to the indirect transmission `ChildMask`.
     */
    const ChildMask &GetIndirectTxChildMask(void) const { return GetMetadata().mChildMask; }
#endif

    /**
     * Returns the RLOC16 of the mesh destination.
     *
     * @note Only use this for non-link-local unicast messages.
     *
     * @returns The IEEE 802.15.4 Destination PAN ID.
     */
    uint16_t GetMeshDest(void) const { return GetMetadata().mMeshDest; }

    /**
     * Sets the RLOC16 of the mesh destination.
     *
     * @note Only use this when sending non-link-local unicast messages.
     *
     * @param[in]  aMeshDest  The IEEE 802.15.4 Destination PAN ID.
     */
    void SetMeshDest(uint16_t aMeshDest) { GetMetadata().mMeshDest = aMeshDest; }

    /**
     * Returns the IEEE 802.15.4 Source or Destination PAN ID.
     *
     * For a message received over the Thread radio, specifies the Source PAN ID when present in MAC header, otherwise
     * specifies the Destination PAN ID.
     *
     * For a message to be sent over the Thread radio, this is set and used for MLE Discover Request or Response
     * messages.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     */
    uint16_t GetPanId(void) const { return GetMetadata().mPanId; }

    /**
     * Sets the IEEE 802.15.4 Destination PAN ID.
     *
     * @note Only use this when sending MLE Discover Request or Response messages.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 Destination PAN ID.
     */
    void SetPanId(uint16_t aPanId) { GetMetadata().mPanId = aPanId; }

    /**
     * Indicates whether the Destination PAN ID is broadcast.
     *
     * This is applicable for messages received over Thread radio.
     *
     * @retval TRUE   The Destination PAN ID is broadcast.
     * @retval FALSE  The Destination PAN ID is not broadcast.
     */
    bool IsDstPanIdBroadcast(void) const { return GetMetadata().mIsDstPanIdBroadcast; }

    /**
     * Returns the IEEE 802.15.4 Channel to use for transmission.
     *
     * @note Only use this when sending MLE Announce messages.
     *
     * @returns The IEEE 802.15.4 Channel to use for transmission.
     */
    uint8_t GetChannel(void) const { return GetMetadata().mChannel; }

    /**
     * Sets the IEEE 802.15.4 Channel to use for transmission.
     *
     * @note Only use this when sending MLE Announce messages.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Channel to use for transmission.
     */
    void SetChannel(uint8_t aChannel) { GetMetadata().mChannel = aChannel; }

    /**
     * Returns the message timestamp.
     *
     * @returns The message timestamp.
     */
    TimeMilli GetTimestamp(void) const { return GetMetadata().mTimestamp; }

    /**
     * Sets the message timestamp to a given time.
     *
     * @param[in] aTimestamp   The timestamp value.
     */
    void SetTimestamp(TimeMilli aTimestamp) { GetMetadata().mTimestamp = aTimestamp; }

    /**
     * Sets the message timestamp to the current time.
     */
    void SetTimestampToNow(void) { SetTimestamp(TimerMilli::GetNow()); }

    /**
     * Returns whether or not message forwarding is scheduled for direct transmission.
     *
     * @retval TRUE   If message forwarding is scheduled for direct transmission.
     * @retval FALSE  If message forwarding is not scheduled for direct transmission.
     */
    bool IsDirectTransmission(void) const { return GetMetadata().mDirectTx; }

    /**
     * Unschedules forwarding using direct transmission.
     */
    void ClearDirectTransmission(void) { GetMetadata().mDirectTx = false; }

    /**
     * Schedules forwarding using direct transmission.
     */
    void SetDirectTransmission(void) { GetMetadata().mDirectTx = true; }

    /**
     * Indicates whether the direct transmission of message was successful.
     *
     * @retval TRUE   If direct transmission of message was successful (all fragments were delivered and acked).
     * @retval FALSE  If direct transmission of message failed (at least one fragment failed).
     */
    bool GetTxSuccess(void) const { return GetMetadata().mTxSuccess; }

    /**
     * Sets whether the direct transmission of message was successful.
     *
     * @param[in] aTxSuccess   TRUE if the direct transmission is successful, FALSE otherwise (i.e., at least one
     *                         fragment transmission failed).
     */
    void SetTxSuccess(bool aTxSuccess) { GetMetadata().mTxSuccess = aTxSuccess; }

    /**
     * Indicates whether the message may be evicted.
     *
     * @retval TRUE   If the message must not be evicted.
     * @retval FALSE  If the message may be evicted.
     */
    bool GetDoNotEvict(void) const { return GetMetadata().mDoNotEvict; }

    /**
     * Sets whether the message may be evicted.
     *
     * @param[in]  aDoNotEvict  TRUE if the message may not be evicted, FALSE otherwise.
     */
    void SetDoNotEvict(bool aDoNotEvict) { GetMetadata().mDoNotEvict = aDoNotEvict; }

    /**
     * Indicates whether the message is waiting for an address query resolution.
     *
     * @retval TRUE   If the message is waiting for address query resolution.
     * @retval FALSE  If the message is not waiting for address query resolution.
     */
    bool IsResolvingAddress(void) const { return GetMetadata().mResolvingAddress; }

    /**
     * Sets whether the message is waiting for an address query resolution.
     *
     * @param[in] aResolvingAddress    TRUE if message is waiting for address resolution, FALSE otherwise.
     */
    void SetResolvingAddress(bool aResolvingAddress) { GetMetadata().mResolvingAddress = aResolvingAddress; }

    /**
     * Indicates whether the message is allowed to be looped back to host.
     *
     * @retval TRUE   If the message is allowed to be looped back to host.
     * @retval FALSE  If the message is not allowed to be looped back to host.
     */
    bool IsLoopbackToHostAllowed(void) const { return GetMetadata().mAllowLookbackToHost; }

    /**
     * Sets whether or not allow the message to be looped back to host.
     *
     * @param[in] aAllowLoopbackToHost  Whether or not allow the message to be looped back to host.
     */
    void SetLoopbackToHostAllowed(bool aAllowLoopbackToHost)
    {
        GetMetadata().mAllowLookbackToHost = aAllowLoopbackToHost;
    }

    /**
     * Gets the message origin.
     *
     * @returns An enum representing the origin of the message.
     */
    Origin GetOrigin(void) const { return static_cast<Origin>(GetMetadata().mOrigin); }

    /**
     * Sets the message origin.
     *
     * @param[in]  aOrigin  An enum representing the origin of the message.
     */
    void SetOrigin(Origin aOrigin) { GetMetadata().mOrigin = aOrigin; }

    /**
     * Indicates whether or not the message origin is Thread Netif.
     *
     * @retval TRUE   If the message origin is Thread Netif.
     * @retval FALSE  If the message origin is not Thread Netif.
     */
    bool IsOriginThreadNetif(void) const { return GetOrigin() == kOriginThreadNetif; }

    /**
     * Indicates whether or not the message origin is a trusted source on host.
     *
     * @retval TRUE   If the message origin is a trusted source on host.
     * @retval FALSE  If the message origin is not a trusted source on host.
     */
    bool IsOriginHostTrusted(void) const { return GetOrigin() == kOriginHostTrusted; }

    /**
     * Indicates whether or not the message origin is an untrusted source on host.
     *
     * @retval TRUE   If the message origin is an untrusted source on host.
     * @retval FALSE  If the message origin is not an untrusted source on host.
     */
    bool IsOriginHostUntrusted(void) const { return GetOrigin() == kOriginHostUntrusted; }

    /**
     * Indicates whether or not link security is enabled for the message.
     *
     * @retval TRUE   If link security is enabled.
     * @retval FALSE  If link security is not enabled.
     */
    bool IsLinkSecurityEnabled(void) const { return GetMetadata().mLinkSecurity; }

    /**
     * Sets whether or not link security is enabled for the message.
     *
     * @param[in]  aEnabled  TRUE if link security is enabled, FALSE otherwise.
     */
    void SetLinkSecurityEnabled(bool aEnabled) { GetMetadata().mLinkSecurity = aEnabled; }

    /**
     * Updates the average RSS (Received Signal Strength) associated with the message by adding the given
     * RSS value to the average. Note that a message can be composed of multiple 802.15.4 data frame fragments each
     * received with a different signal strength.
     *
     * @param[in] aRss A new RSS value (in dBm) to be added to average.
     */
    void AddRss(int8_t aRss) { IgnoreError(GetMetadata().mRssAverager.Add(aRss)); }

    /**
     * Returns the average RSS (Received Signal Strength) associated with the message.
     *
     * @returns The current average RSS value (in dBm) or `Radio::kInvalidRssi` if no average is available.
     */
    int8_t GetAverageRss(void) const { return GetMetadata().mRssAverager.GetAverage(); }

    /**
     * Returns a const reference to RssAverager of the message.
     *
     * @returns A const reference to the RssAverager of the message.
     */
    const RssAverager &GetRssAverager(void) const { return GetMetadata().mRssAverager; }

    /**
     * Updates the average LQI (Link Quality Indicator) associated with the message.
     *
     * The given LQI value would be added to the average. Note that a message can be composed of multiple 802.15.4
     * frame fragments each received with a different signal strength.
     *
     * @param[in] aLqi A new LQI value (has no unit) to be added to average.
     */
    void AddLqi(uint8_t aLqi) { GetMetadata().mLqiAverager.Add(aLqi); }

    /**
     * Returns the average LQI (Link Quality Indicator) associated with the message.
     *
     * @returns The current average LQI value (in dBm) or OT_RADIO_LQI_NONE if no average is available.
     */
    uint8_t GetAverageLqi(void) const { return GetMetadata().mLqiAverager.GetAverage(); }

    /**
     * Returns the count of frames counted so far.
     *
     * @returns The count of frames that have been counted.
     */
    uint8_t GetPsduCount(void) const { return GetMetadata().mLqiAverager.GetCount(); }

    /**
     * Returns a const reference to LqiAverager of the message.
     *
     * @returns A const reference to the LqiAverager of the message.
     */
    const LqiAverager &GetLqiAverager(void) const { return GetMetadata().mLqiAverager; }

    /**
     * Retrieves `ThreadLinkInfo` from the message if received over Thread radio with origin `kOriginThreadNetif`.
     *
     * @pram[out] aLinkInfo     A reference to a `ThreadLinkInfo` to populate.
     *
     * @retval kErrorNone       Successfully retrieved the link info, @p `aLinkInfo` is updated.
     * @retval kErrorNotFound   Message origin is not `kOriginThreadNetif`.
     */
    Error GetLinkInfo(ThreadLinkInfo &aLinkInfo) const;

    /**
     * Sets the message's link info properties (PAN ID, link security, RSS) from a given `ThreadLinkInfo`.
     *
     * @param[in] aLinkInfo   The `ThreadLinkInfo` instance from which to set message's related properties.
     */
    void UpdateLinkInfoFrom(const ThreadLinkInfo &aLinkInfo);

    /**
     * Returns a pointer to the message queue (if any) where this message is queued.
     *
     * @returns A pointer to the message queue or `nullptr` if not in any message queue.
     */
    MessageQueue *GetMessageQueue(void) const
    {
        return !GetMetadata().mInPriorityQ ? static_cast<MessageQueue *>(GetMetadata().mQueue) : nullptr;
    }

    /**
     * Returns a pointer to the priority message queue (if any) where this message is queued.
     *
     * @returns A pointer to the priority queue or `nullptr` if not in any priority queue.
     */
    PriorityQueue *GetPriorityQueue(void) const
    {
        return GetMetadata().mInPriorityQ ? static_cast<PriorityQueue *>(GetMetadata().mQueue) : nullptr;
    }

    /**
     * Indicates whether or not the message is also used for time sync purpose.
     *
     * When OPENTHREAD_CONFIG_TIME_SYNC_ENABLE is 0, this method always return false.
     *
     * @retval TRUE   If the message is also used for time sync purpose.
     * @retval FALSE  If the message is not used for time sync purpose.
     */
    bool IsTimeSync(void) const;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Sets whether or not the message is also used for time sync purpose.
     *
     * @param[in]  aEnabled  TRUE if the message is also used for time sync purpose, FALSE otherwise.
     */
    void SetTimeSync(bool aEnabled) { GetMetadata().mTimeSync = aEnabled; }

    /**
     * Sets the offset to network time.
     *
     * @param[in]  aNetworkTimeOffset  The offset to network time.
     */
    void SetNetworkTimeOffset(int64_t aNetworkTimeOffset) { GetMetadata().mNetworkTimeOffset = aNetworkTimeOffset; }

    /**
     * Gets the offset to network time.
     *
     * @returns  The offset to network time.
     */
    int64_t GetNetworkTimeOffset(void) const { return GetMetadata().mNetworkTimeOffset; }

    /**
     * Sets the time sync sequence.
     *
     * @param[in]  aTimeSyncSeq  The time sync sequence.
     */
    void SetTimeSyncSeq(uint8_t aTimeSyncSeq) { GetMetadata().mTimeSyncSeq = aTimeSyncSeq; }

    /**
     * Gets the time sync sequence.
     *
     * @returns  The time sync sequence.
     */
    uint8_t GetTimeSyncSeq(void) const { return GetMetadata().mTimeSyncSeq; }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO
    /**
     * Indicates whether the radio type is set.
     *
     * @retval TRUE   If the radio type is set.
     * @retval FALSE  If the radio type is not set.
     */
    bool IsRadioTypeSet(void) const { return GetMetadata().mIsRadioTypeSet; }

    /**
     * Gets the radio link type the message was received on, or should be sent on.
     *
     * Should be used only when `IsRadioTypeSet()` returns `true`.
     *
     * @returns The radio link type of the message.
     */
    Mac::RadioType GetRadioType(void) const { return static_cast<Mac::RadioType>(GetMetadata().mRadioType); }

    /**
     * Sets the radio link type the message was received on, or should be sent on.
     *
     * @param[in] aRadioType   A radio link type of the message.
     */
    void SetRadioType(Mac::RadioType aRadioType)
    {
        GetMetadata().mIsRadioTypeSet = true;
        GetMetadata().mRadioType      = aRadioType;
    }

    /**
     * Clears any previously set radio type on the message.
     *
     * After calling this method, `IsRadioTypeSet()` returns false until radio type is set (`SetRadioType()`).
     */
    void ClearRadioType(void) { GetMetadata().mIsRadioTypeSet = false; }

#endif // #if OPENTHREAD_CONFIG_MULTI_RADIO

protected:
    class ConstIterator : public ItemPtrIterator<const Message, ConstIterator>
    {
        friend class ItemPtrIterator<const Message, ConstIterator>;

    public:
        ConstIterator(void) = default;

        explicit ConstIterator(const Message *aMessage)
            : ItemPtrIterator(aMessage)
        {
        }

    private:
        void Advance(void) { mItem = mItem->GetNext(); }
    };

    class Iterator : public ItemPtrIterator<Message, Iterator>
    {
        friend class ItemPtrIterator<Message, Iterator>;

    public:
        Iterator(void)
            : mNext(nullptr)
        {
        }

        explicit Iterator(Message *aMessage)
            : ItemPtrIterator(aMessage)
            , mNext(NextMessage(aMessage))
        {
        }

    private:
        void            Advance(void);
        static Message *NextMessage(Message *aMessage) { return (aMessage != nullptr) ? aMessage->GetNext() : nullptr; }

        Message *mNext;
    };

    uint16_t GetReserved(void) const { return GetMetadata().mReserved; }
    void     SetReserved(uint16_t aReservedHeader) { GetMetadata().mReserved = aReservedHeader; }

private:
    class Chunk : public Data<kWithUint16Length>
    {
    public:
        const Buffer *GetBuffer(void) const { return mBuffer; }
        void          SetBuffer(const Buffer *aBuffer) { mBuffer = aBuffer; }

    private:
        const Buffer *mBuffer; // Buffer containing the chunk
    };

    class MutableChunk : public Chunk
    {
    public:
        uint8_t *GetBytes(void) { return AsNonConst(Chunk::GetBytes()); }
    };

    void GetFirstChunk(uint16_t aOffset, uint16_t &aLength, Chunk &aChunk) const;
    void GetNextChunk(uint16_t &aLength, Chunk &aChunk) const;

    void GetFirstChunk(uint16_t aOffset, uint16_t &aLength, MutableChunk &aChunk)
    {
        AsConst(this)->GetFirstChunk(aOffset, aLength, static_cast<Chunk &>(aChunk));
    }

    void GetNextChunk(uint16_t &aLength, MutableChunk &aChunk)
    {
        AsConst(this)->GetNextChunk(aLength, static_cast<Chunk &>(aChunk));
    }

    MessagePool *GetMessagePool(void) const { return GetMetadata().mMessagePool; }
    void         SetMessagePool(MessagePool *aMessagePool) { GetMetadata().mMessagePool = aMessagePool; }

    bool IsInAQueue(void) const { return (GetMetadata().mQueue != nullptr); }
    void SetMessageQueue(MessageQueue *aMessageQueue);
    void SetPriorityQueue(PriorityQueue *aPriorityQueue);

    void SetRssAverager(const RssAverager &aRssAverager) { GetMetadata().mRssAverager = aRssAverager; }
    void SetLqiAverager(const LqiAverager &aLqiAverager) { GetMetadata().mLqiAverager = aLqiAverager; }

    Message       *&Next(void) { return GetMetadata().mNext; }
    Message *const &Next(void) const { return GetMetadata().mNext; }
    Message       *&Prev(void) { return GetMetadata().mPrev; }

    static Message       *NextOf(Message *aMessage) { return (aMessage != nullptr) ? aMessage->Next() : nullptr; }
    static const Message *NextOf(const Message *aMessage) { return (aMessage != nullptr) ? aMessage->Next() : nullptr; }

    Error ResizeMessage(uint16_t aLength);
};

/**
 * Implements a message queue.
 */
class MessageQueue : public otMessageQueue
{
    friend class Message;
    friend class PriorityQueue;

public:
    typedef otMessageQueueInfo Info; ///< This struct represents info (number of messages/buffers) about a queue.

    /**
     * Represents a position (head or tail) in the queue. This is used to specify where a new message
     * should be added in the queue.
     */
    enum QueuePosition : uint8_t
    {
        kQueuePositionHead, ///< Indicates the head (front) of the list.
        kQueuePositionTail, ///< Indicates the tail (end) of the list.
    };

    /**
     * Initializes the message queue.
     */
    MessageQueue(void) { SetTail(nullptr); }

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    Message *GetHead(void) { return Message::NextOf(GetTail()); }

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    const Message *GetHead(void) const { return Message::NextOf(GetTail()); }

    /**
     * Adds a message to the end of the list.
     *
     * @param[in]  aMessage  The message to add.
     */
    void Enqueue(Message &aMessage) { Enqueue(aMessage, kQueuePositionTail); }

    /**
     * Adds a message at a given position (head/tail) of the list.
     *
     * @param[in]  aMessage  The message to add.
     * @param[in]  aPosition The position (head or tail) where to add the message.
     */
    void Enqueue(Message &aMessage, QueuePosition aPosition);

    /**
     * Removes a message from the list.
     *
     * @param[in]  aMessage  The message to remove.
     */
    void Dequeue(Message &aMessage);

    /**
     * Removes a message from the queue and frees it.
     *
     * @param[in]  aMessage  The message to remove and free.
     */
    void DequeueAndFree(Message &aMessage);

    /**
     * Removes and frees all messages from the queue.
     */
    void DequeueAndFreeAll(void);

    /**
     * Gets the information about number of messages and buffers in the queue.
     *
     * Updates `aInfo` and adds number of message/buffers in the message queue to the corresponding member
     * variable in `aInfo`. The caller needs to make sure `aInfo` is initialized before calling this method (e.g.,
     * clearing `aInfo`). Same `aInfo` can be passed in multiple calls of `GetInfo(aInfo)` on different queues to add
     * up the number of messages/buffers on different queues.
     *
     * @param[out] aInfo  A reference to `Info` structure to update.ni
     */
    void GetInfo(Info &aInfo) const;

    // The following methods are intended to support range-based `for`
    // loop iteration over the queue entries and should not be used
    // directly. The range-based `for` works correctly even if the
    // current entry is removed from the queue during iteration.

    Message::Iterator begin(void);
    Message::Iterator end(void) { return Message::Iterator(); }

    Message::ConstIterator begin(void) const;
    Message::ConstIterator end(void) const { return Message::ConstIterator(); }

private:
    Message       *GetTail(void) { return static_cast<Message *>(mData); }
    const Message *GetTail(void) const { return static_cast<const Message *>(mData); }
    void           SetTail(Message *aMessage) { mData = aMessage; }
};

/**
 * Implements a priority queue.
 */
class PriorityQueue : private Clearable<PriorityQueue>
{
    friend class Message;
    friend class MessageQueue;
    friend class MessagePool;
    friend class Clearable<PriorityQueue>;

public:
    typedef otMessageQueueInfo Info; ///< This struct represents info (number of messages/buffers) about a queue.

    /**
     * Initializes the priority queue.
     */
    PriorityQueue(void) { Clear(); }

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    Message *GetHead(void) { return AsNonConst(AsConst(this)->GetHead()); }

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    const Message *GetHead(void) const;

    /**
     * Returns a pointer to the first message for a given priority level.
     *
     * @param[in] aPriority   Priority level.
     *
     * @returns A pointer to the first message with given priority level or `nullptr` if there is no messages with
     *          this priority level.
     */
    Message *GetHeadForPriority(Message::Priority aPriority)
    {
        return AsNonConst(AsConst(this)->GetHeadForPriority(aPriority));
    }

    /**
     * Returns a pointer to the first message for a given priority level.
     *
     * @param[in] aPriority   Priority level.
     *
     * @returns A pointer to the first message with given priority level or `nullptr` if there is no messages with
     *          this priority level.
     */
    const Message *GetHeadForPriority(Message::Priority aPriority) const;

    /**
     * Adds a message to the queue.
     *
     * @param[in]  aMessage  The message to add.
     */
    void Enqueue(Message &aMessage);

    /**
     * Removes a message from the list.
     *
     * @param[in]  aMessage  The message to remove.
     */
    void Dequeue(Message &aMessage);

    /**
     * Removes a message from the queue and frees it.
     *
     * @param[in]  aMessage  The message to remove and free.
     */
    void DequeueAndFree(Message &aMessage);

    /**
     * Removes and frees all messages from the queue.
     */
    void DequeueAndFreeAll(void);

    /**
     * Returns the tail of the list (last message in the list).
     *
     * @returns A pointer to the tail of the list.
     */
    Message *GetTail(void) { return AsNonConst(AsConst(this)->GetTail()); }

    /**
     * Returns the tail of the list (last message in the list).
     *
     * @returns A pointer to the tail of the list.
     */
    const Message *GetTail(void) const;

    /**
     * Gets the information about number of messages and buffers in the priority queue.
     *
     * Updates `aInfo` array and adds number of message/buffers in the message queue to the corresponding
     * member variable in `aInfo`. The caller needs to make sure `aInfo` is initialized before calling this method
     * (e.g., clearing `aInfo`). Same `aInfo` can be passed in multiple calls of `GetInfo(aInfo)` on different queues
     * to add up the number of messages/buffers on different queues.
     *
     * @param[out] aInfo  A reference to an `Info` structure to update.
     */
    void GetInfo(Info &aInfo) const;

    // The following methods are intended to support range-based `for`
    // loop iteration over the queue entries and should not be used
    // directly. The range-based `for` works correctly even if the
    // current entry is removed from the queue during iteration.

    Message::Iterator begin(void);
    Message::Iterator end(void) { return Message::Iterator(); }

    Message::ConstIterator begin(void) const;
    Message::ConstIterator end(void) const { return Message::ConstIterator(); }

private:
    uint8_t PrevPriority(uint8_t aPriority) const
    {
        return (aPriority == Message::kNumPriorities - 1) ? 0 : (aPriority + 1);
    }

    const Message *FindFirstNonNullTail(Message::Priority aStartPriorityLevel) const;

    Message *FindFirstNonNullTail(Message::Priority aStartPriorityLevel)
    {
        return AsNonConst(AsConst(this)->FindFirstNonNullTail(aStartPriorityLevel));
    }

    Message *mTails[Message::kNumPriorities]; // Tail pointers associated with different priority levels.
};

/**
 * Represents a message pool
 */
class MessagePool : public InstanceLocator, private NonCopyable
{
    friend class Message;
    friend class MessageQueue;
    friend class PriorityQueue;

public:
    /**
     * Initializes the object.
     */
    explicit MessagePool(Instance &aInstance);

    /**
     * Allocates a new message with specified settings.
     *
     * @param[in]  aType           The message type.
     * @param[in]  aReserveHeader  The number of header bytes to reserve.
     * @param[in]  aSettings       The message settings.
     *
     * @returns A pointer to the message or `nullptr` if no message buffers are available.
     */
    Message *Allocate(Message::Type aType, uint16_t aReserveHeader, const Message::Settings &aSettings);

    /**
     * Allocates a new message of a given type using default settings.
     *
     * @param[in]  aType           The message type.
     *
     * @returns A pointer to the message or `nullptr` if no message buffers are available.
     */
    Message *Allocate(Message::Type aType);

    /**
     * Allocates a new message with a given type and reserved length using default settings.
     *
     * @param[in]  aType           The message type.
     * @param[in]  aReserveHeader  The number of header bytes to reserve.
     *
     * @returns A pointer to the message or `nullptr` if no message buffers are available.
     */
    Message *Allocate(Message::Type aType, uint16_t aReserveHeader);

    /**
     * Is used to free a message and return all message buffers to the buffer pool.
     *
     * @param[in]  aMessage  The message to free.
     */
    void Free(Message *aMessage);

    /**
     * Returns the number of free buffers.
     *
     * @returns The number of free buffers, or 0xffff (UINT16_MAX) if number is unknown.
     */
    uint16_t GetFreeBufferCount(void) const;

    /**
     * Returns the total number of buffers.
     *
     * @returns The total number of buffers, or 0xffff (UINT16_MAX) if number is unknown.
     */
    uint16_t GetTotalBufferCount(void) const;

    /**
     * Returns the maximum number of buffers in use at the same time since OT stack initialization or
     * since last call to `ResetMaxUsedBufferCount()`.
     *
     * @returns The maximum number of buffers in use at the same time so far (buffer allocation watermark).
     */
    uint16_t GetMaxUsedBufferCount(void) const { return mMaxAllocated; }

    /**
     * Resets the tracked maximum number of buffers in use.
     *
     * @sa GetMaxUsedBufferCount
     */
    void ResetMaxUsedBufferCount(void) { mMaxAllocated = mNumAllocated; }

private:
    Buffer *NewBuffer(Message::Priority aPriority);
    void    FreeBuffers(Buffer *aBuffer);
    Error   ReclaimBuffers(Message::Priority aPriority);

#if !OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT && !OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
    Pool<Buffer, kNumBuffers> mBufferPool;
#endif
    uint16_t mNumAllocated;
    uint16_t mMaxAllocated;
};

inline Instance &Message::GetInstance(void) const { return GetMessagePool()->GetInstance(); }

/**
 * @}
 */

DefineCoreType(otMessageBuffer, Buffer);
DefineCoreType(otMessageSettings, Message::Settings);
DefineCoreType(otMessage, Message);
DefineCoreType(otMessageQueue, MessageQueue);

DefineMapEnum(otMessageOrigin, Message::Origin);

} // namespace ot

#endif // MESSAGE_HPP_
