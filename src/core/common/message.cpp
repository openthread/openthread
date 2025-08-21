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
 *   This file implements the message buffer pool and message buffers.
 */

#include "message.hpp"

#include "instance/instance.hpp"
#include "thread/diagnostic_server.hpp"

#if OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE && OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
#error "OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE conflicts with OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT."
#endif

namespace ot {

RegisterLogModule("Message");

//---------------------------------------------------------------------------------------------------------------------
// MessagePool

MessagePool::MessagePool(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNumAllocated(0)
    , mMaxAllocated(0)
{
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    otPlatMessagePoolInit(&GetInstance(), kNumBuffers, sizeof(Buffer));
#endif
}

Message *MessagePool::Allocate(Message::Type aType, uint16_t aReserveHeader, const Message::Settings &aSettings)
{
    Error    error = kErrorNone;
    Message *message;

    VerifyOrExit((message = static_cast<Message *>(NewBuffer(aSettings.GetPriority()))) != nullptr);

    ClearAllBytes(*message);

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    message->GetMetadata().mInstance = &GetInstance();
#endif
    message->SetType(aType);
    message->SetReserved(aReserveHeader);
    message->SetLinkSecurityEnabled(aSettings.IsLinkSecurityEnabled());
    message->SetLoopbackToHostAllowed(OPENTHREAD_CONFIG_IP6_ALLOW_LOOP_BACK_HOST_DATAGRAMS);
    message->SetOrigin(Message::kOriginHostTrusted);
    message->MarkAsNotInAQueue();

    SuccessOrExit(error = message->SetPriority(aSettings.GetPriority()));
    SuccessOrExit(error = message->SetLength(0));

exit:
    if (error != kErrorNone)
    {
        Free(message);
        message = nullptr;
    }

    return message;
}

Message *MessagePool::Allocate(Message::Type aType) { return Allocate(aType, 0, Message::Settings::GetDefault()); }

Message *MessagePool::Allocate(Message::Type aType, uint16_t aReserveHeader)
{
    return Allocate(aType, aReserveHeader, Message::Settings::GetDefault());
}

void MessagePool::Free(Message *aMessage)
{
    OT_ASSERT(!aMessage->IsInAQueue());

    FreeBuffers(static_cast<Buffer *>(aMessage));
}

Buffer *MessagePool::NewBuffer(Message::Priority aPriority)
{
    Buffer *buffer = nullptr;

    while ((
#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
               buffer = static_cast<Buffer *>(Heap::CAlloc(1, sizeof(Buffer)))
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
               buffer = static_cast<Buffer *>(otPlatMessagePoolNew(&GetInstance()))
#else
               buffer = mBufferPool.Allocate()
#endif
                   ) == nullptr)
    {
        SuccessOrExit(ReclaimBuffers(aPriority));
    }

    mNumAllocated++;
    mMaxAllocated = Max(mMaxAllocated, mNumAllocated);

    buffer->SetNextBuffer(nullptr);

exit:
    if (buffer == nullptr)
    {
        LogInfo("No available message buffer");
    }

    return buffer;
}

void MessagePool::FreeBuffers(Buffer *aBuffer)
{
    while (aBuffer != nullptr)
    {
        Buffer *next = aBuffer->GetNextBuffer();
#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
        Heap::Free(aBuffer);
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        otPlatMessagePoolFree(&GetInstance(), aBuffer);
#else
        mBufferPool.Free(*aBuffer);
#endif
        mNumAllocated--;

        aBuffer = next;
    }
}

Error MessagePool::ReclaimBuffers(Message::Priority aPriority)
{
    Error error = kErrorNotFound;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_DIAG_SERVER_ENABLE
    error = Get<DiagnosticServer::Server>().EvictDiagCache(true);

    // TODO it might be desireable to only evict rx off diag messages
    // after trying to evict messages
    if (error != kErrorNone)
    {
        error = Get<DiagnosticServer::Server>().EvictDiagCache(false);
    }
#endif

    if (error != kErrorNone)
    {
        error = Get<MeshForwarder>().EvictMessage(aPriority);
    }

    return error;
}

uint16_t MessagePool::GetFreeBufferCount(void) const
{
    uint16_t rval;

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    rval = static_cast<uint16_t>(Instance::GetHeap().GetFreeSize() / sizeof(Buffer));
#else
    rval = NumericLimits<uint16_t>::kMax;
#endif
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    rval = otPlatMessagePoolNumFreeBuffers(&GetInstance());
#else
    rval = kNumBuffers - mNumAllocated;
#endif

    return rval;
}

uint16_t MessagePool::GetTotalBufferCount(void) const
{
    uint16_t rval;

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    rval = static_cast<uint16_t>(Instance::GetHeap().GetCapacity() / sizeof(Buffer));
#else
    rval = NumericLimits<uint16_t>::kMax;
#endif
#else
    rval = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;
#endif

    return rval;
}

//---------------------------------------------------------------------------------------------------------------------
// Message::Settings

const otMessageSettings Message::Settings::kDefault = {kWithLinkSecurity, kPriorityNormal};

Message::Settings::Settings(LinkSecurityMode aSecurityMode, Priority aPriority)
{
    mLinkSecurityEnabled = aSecurityMode;
    mPriority            = aPriority;
}

const Message::Settings &Message::Settings::From(const otMessageSettings *aSettings)
{
    return (aSettings == nullptr) ? GetDefault() : AsCoreType(aSettings);
}

//---------------------------------------------------------------------------------------------------------------------
// Message::Iterator

void Message::Iterator::Advance(void)
{
    mItem = mNext;
    mNext = NextMessage(mNext);
}

//---------------------------------------------------------------------------------------------------------------------
// Message

Error Message::ResizeMessage(uint16_t aLength)
{
    // This method adds or frees message buffers to meet the
    // requested length.

    Error    error     = kErrorNone;
    Buffer  *curBuffer = this;
    Buffer  *lastBuffer;
    uint16_t curLength = kHeadBufferDataSize;

    while (curLength < aLength)
    {
        if (curBuffer->GetNextBuffer() == nullptr)
        {
            curBuffer->SetNextBuffer(Get<MessagePool>().NewBuffer(GetPriority()));
            VerifyOrExit(curBuffer->GetNextBuffer() != nullptr, error = kErrorNoBufs);
        }

        curBuffer = curBuffer->GetNextBuffer();
        curLength += kBufferDataSize;
    }

    lastBuffer = curBuffer;
    curBuffer  = curBuffer->GetNextBuffer();
    lastBuffer->SetNextBuffer(nullptr);

    Get<MessagePool>().FreeBuffers(curBuffer);

exit:
    return error;
}

void Message::Free(void)
{
    // `TxCallback` is cleared once it is invoked. If the message is
    // freed before we know the TX outcome, it's treated as a dropped
    // message, signaling `kErrorDrop`.

    InvokeTxCallback(kErrorDrop);
    Get<MessagePool>().Free(this);
}

Error Message::SetLength(uint16_t aLength)
{
    Error    error              = kErrorNone;
    uint16_t totalLengthRequest = GetReserved() + aLength;

    VerifyOrExit(totalLengthRequest >= GetReserved(), error = kErrorInvalidArgs);

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    GetMetadata().mLength = aLength;

    // Correct the offset in case shorter length is set.
    if (GetOffset() > aLength)
    {
        SetOffset(aLength);
    }

exit:
    return error;
}

uint8_t Message::GetBufferCount(void) const
{
    uint8_t rval = 1;

    for (const Buffer *curBuffer = GetNextBuffer(); curBuffer; curBuffer = curBuffer->GetNextBuffer())
    {
        rval++;
    }

    return rval;
}

void Message::MoveOffset(int aDelta)
{
    OT_ASSERT(GetOffset() + aDelta <= GetLength());
    GetMetadata().mOffset += static_cast<int16_t>(aDelta);
    OT_ASSERT(GetMetadata().mOffset <= GetLength());
}

void Message::SetOffset(uint16_t aOffset)
{
    OT_ASSERT(aOffset <= GetLength());
    GetMetadata().mOffset = aOffset;
}

bool Message::IsMleCommand(Mle::Command aMleCommand) const
{
    return (GetSubType() == kSubTypeMle) && (GetMetadata().mMleCommand == aMleCommand);
}

Error Message::SetPriority(Priority aPriority)
{
    Error   error    = kErrorNone;
    uint8_t priority = static_cast<uint8_t>(aPriority);

    static_assert(kNumPriorities <= 4, "`Metadata::mPriority` as a 2-bit field cannot fit all `Priority` values");

    VerifyOrExit(priority < kNumPriorities, error = kErrorInvalidArgs);

    VerifyOrExit(!IsInAPriorityQueue(), error = kErrorInvalidState);

    GetMetadata().mPriority = priority;

exit:
    return error;
}

const char *Message::PriorityToString(Priority aPriority)
{
    static const char *const kPriorityStrings[] = {
        "low",    // (0) kPriorityLow
        "normal", // (1) kPriorityNormal
        "high",   // (2) kPriorityHigh
        "net",    // (3) kPriorityNet
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kPriorityLow);
        ValidateNextEnum(kPriorityNormal);
        ValidateNextEnum(kPriorityHigh);
        ValidateNextEnum(kPriorityNet);
    };

    return kPriorityStrings[aPriority];
}

void Message::RegisterTxCallback(TxCallback aCallback, void *aContext)
{
    GetMetadata().mTxCallback = aCallback;
    GetMetadata().mTxContext  = aContext;
}

void Message::InvokeTxCallback(Error aError)
{
    TxCallback callback = GetMetadata().mTxCallback;

    if (callback != nullptr)
    {
        GetMetadata().mTxCallback = nullptr;
        callback(this, aError, GetMetadata().mTxContext);
    }
}

Error Message::AppendBytes(const void *aBuf, uint16_t aLength)
{
    Error    error     = kErrorNone;
    uint16_t oldLength = GetLength();

    SuccessOrExit(error = SetLength(GetLength() + aLength));
    WriteBytes(oldLength, aBuf, aLength);

exit:
    return error;
}

Error Message::AppendBytesFromMessage(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    return AppendBytesFromMessage(aMessage, aOffsetRange.GetOffset(), aOffsetRange.GetLength());
}

Error Message::AppendBytesFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error    error       = kErrorNone;
    uint16_t writeOffset = GetLength();
    Chunk    chunk;

    VerifyOrExit(aMessage.GetLength() >= aOffset + aLength, error = kErrorParse);
    SuccessOrExit(error = SetLength(GetLength() + aLength));

    aMessage.GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        WriteBytes(writeOffset, chunk.GetBytes(), chunk.GetLength());
        writeOffset += chunk.GetLength();
        aMessage.GetNextChunk(aLength, chunk);
    }

exit:
    return error;
}

Error Message::PrependBytes(const void *aBuf, uint16_t aLength)
{
    Error   error     = kErrorNone;
    Buffer *newBuffer = nullptr;

    while (aLength > GetReserved())
    {
        VerifyOrExit((newBuffer = Get<MessagePool>().NewBuffer(GetPriority())) != nullptr, error = kErrorNoBufs);

        newBuffer->SetNextBuffer(GetNextBuffer());
        SetNextBuffer(newBuffer);

        if (GetReserved() < sizeof(mBuffer.mHead.mData))
        {
            // Copy payload from the first buffer.
            memcpy(newBuffer->mBuffer.mHead.mData + GetReserved(), mBuffer.mHead.mData + GetReserved(),
                   sizeof(mBuffer.mHead.mData) - GetReserved());
        }

        SetReserved(GetReserved() + kBufferDataSize);
    }

    SetReserved(GetReserved() - aLength);
    GetMetadata().mLength += aLength;
    SetOffset(GetOffset() + aLength);

    if (aBuf != nullptr)
    {
        WriteBytes(0, aBuf, aLength);
    }

exit:
    return error;
}

void Message::RemoveHeader(uint16_t aLength)
{
    OT_ASSERT(aLength <= GetMetadata().mLength);

    GetMetadata().mReserved += aLength;
    GetMetadata().mLength -= aLength;

    if (GetMetadata().mOffset > aLength)
    {
        GetMetadata().mOffset -= aLength;
    }
    else
    {
        GetMetadata().mOffset = 0;
    }
}

void Message::RemoveHeader(uint16_t aOffset, uint16_t aLength)
{
    // To shrink the header, we copy the header byte before `aOffset`
    // forward. Starting at offset `aLength`, we write bytes we read
    // from offset `0` onward and copy a total of `aOffset` bytes.
    // Then remove the first `aLength` bytes from message.
    //
    //
    // 0                   aOffset  aOffset + aLength
    // +-----------------------+---------+------------------------+
    // | / / / / / / / / / / / | x x x x |                        |
    // +-----------------------+---------+------------------------+
    //
    // 0       aLength                aOffset + aLength
    // +---------+-----------------------+------------------------+
    // |         | / / / / / / / / / / / |                        |
    // +---------+-----------------------+------------------------+
    //
    //  0                    aOffset
    //  +-----------------------+------------------------+
    //  | / / / / / / / / / / / |                        |
    //  +-----------------------+------------------------+
    //

    WriteBytesFromMessage(/* aWriteOffset */ aLength, *this, /* aReadOffset */ 0, /* aLength */ aOffset);
    RemoveHeader(aLength);
}

Error Message::InsertHeader(uint16_t aOffset, uint16_t aLength)
{
    Error error;

    // To make space in header at `aOffset`, we first prepend
    // `aLength` bytes at front. Then copy the existing bytes
    // backwards. Starting at offset `0`, we write bytes we read
    // from offset `aLength` onward and copy a total of `aOffset`
    // bytes.
    //
    // 0                    aOffset
    // +-----------------------+------------------------+
    // | / / / / / / / / / / / |                        |
    // +-----------------------+------------------------+
    //
    // 0       aLength                aOffset + aLength
    // +---------+-----------------------+------------------------+
    // |         | / / / / / / / / / / / |                        |
    // +---------+-----------------------+------------------------+
    //
    // 0                   aOffset  aOffset + aLength
    // +-----------------------+---------+------------------------+
    // | / / / / / / / / / / / |  N E W  |                        |
    // +-----------------------+---------+------------------------+
    //

    SuccessOrExit(error = PrependBytes(nullptr, aLength));
    WriteBytesFromMessage(/* aWriteOffset */ 0, *this, /* aReadOffset */ aLength, /* aLength */ aOffset);

exit:
    return error;
}

void Message::RemoveFooter(uint16_t aLength) { IgnoreError(SetLength(GetLength() - Min(aLength, GetLength()))); }

void Message::GetFirstChunk(uint16_t aOffset, uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the first message chunk (contiguous data
    // buffer) corresponding to a given offset and length. On exit
    // `aChunk` is updated such that `aChunk.GetBytes()` gives the
    // pointer to the start of chunk and `aChunk.GetLength()` gives
    // its length. The `aLength` is also decreased by the chunk
    // length.

    VerifyOrExit(aOffset < GetLength(), aChunk.SetLength(0));

    if (aOffset + aLength >= GetLength())
    {
        aLength = GetLength() - aOffset;
    }

    aOffset += GetReserved();

    aChunk.SetBuffer(this);

    // Special case for the first buffer

    if (aOffset < kHeadBufferDataSize)
    {
        aChunk.Init(GetFirstData() + aOffset, kHeadBufferDataSize - aOffset);
        ExitNow();
    }

    aOffset -= kHeadBufferDataSize;

    // Find the `Buffer` matching the offset

    while (true)
    {
        aChunk.SetBuffer(aChunk.GetBuffer()->GetNextBuffer());

        OT_ASSERT(aChunk.GetBuffer() != nullptr);

        if (aOffset < kBufferDataSize)
        {
            aChunk.Init(aChunk.GetBuffer()->GetData() + aOffset, kBufferDataSize - aOffset);
            ExitNow();
        }

        aOffset -= kBufferDataSize;
    }

exit:
    if (aChunk.GetLength() > aLength)
    {
        aChunk.SetLength(aLength);
    }

    aLength -= aChunk.GetLength();
}

void Message::GetNextChunk(uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the next message chunk. On input, the
    // `aChunk` should be the previous chunk. On exit, it is
    // updated to provide info about next chunk, and `aLength`
    // is decreased by the chunk length. If there is no more
    // chunk, `aChunk.GetLength()` would be zero.

    VerifyOrExit(aLength > 0, aChunk.SetLength(0));

    aChunk.SetBuffer(aChunk.GetBuffer()->GetNextBuffer());

    OT_ASSERT(aChunk.GetBuffer() != nullptr);

    aChunk.Init(aChunk.GetBuffer()->GetData(), kBufferDataSize);

    if (aChunk.GetLength() > aLength)
    {
        aChunk.SetLength(aLength);
    }

    aLength -= aChunk.GetLength();

exit:
    return;
}

uint16_t Message::ReadBytes(uint16_t aOffset, void *aBuf, uint16_t aLength) const
{
    uint8_t *bufPtr = reinterpret_cast<uint8_t *>(aBuf);
    Chunk    chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        chunk.CopyBytesTo(bufPtr);
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

    return static_cast<uint16_t>(bufPtr - reinterpret_cast<uint8_t *>(aBuf));
}

uint16_t Message::ReadBytes(const OffsetRange &aOffsetRange, void *aBuf) const
{
    return ReadBytes(aOffsetRange.GetOffset(), aBuf, aOffsetRange.GetLength());
}

Error Message::Read(uint16_t aOffset, void *aBuf, uint16_t aLength) const
{
    Error error = kErrorNone;

    VerifyOrExit(aOffset + aLength <= GetLength(), error = kErrorParse);
    ReadBytes(aOffset, aBuf, aLength);

exit:
    return error;
}

Error Message::Read(const OffsetRange &aOffsetRange, void *aBuf, uint16_t aLength) const
{
    Error error = kErrorNone;

    VerifyOrExit(aOffsetRange.Contains(aLength), error = kErrorParse);
    error = Read(aOffsetRange.GetOffset(), aBuf, aLength);

exit:
    return error;
}

bool Message::CompareBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength, ByteMatcher aMatcher) const
{
    uint16_t       bytesToCompare = aLength;
    const uint8_t *bufPtr         = reinterpret_cast<const uint8_t *>(aBuf);
    Chunk          chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        VerifyOrExit(chunk.MatchesBytesIn(bufPtr, aMatcher));
        bufPtr += chunk.GetLength();
        bytesToCompare -= chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

exit:
    return (bytesToCompare == 0);
}

bool Message::CompareBytes(const OffsetRange &aOffsetRange, const void *aBuf, ByteMatcher aMatcher) const
{
    return CompareBytes(aOffsetRange.GetOffset(), aBuf, aOffsetRange.GetLength(), aMatcher);
}

bool Message::CompareBytes(uint16_t       aOffset,
                           const Message &aOtherMessage,
                           uint16_t       aOtherOffset,
                           uint16_t       aLength,
                           ByteMatcher    aMatcher) const
{
    uint16_t bytesToCompare = aLength;
    Chunk    chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        VerifyOrExit(aOtherMessage.CompareBytes(aOtherOffset, chunk.GetBytes(), chunk.GetLength(), aMatcher));
        aOtherOffset += chunk.GetLength();
        bytesToCompare -= chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

exit:
    return (bytesToCompare == 0);
}

Error Message::ResizeRegion(uint16_t aOffset, uint16_t aOldLength, uint16_t aNewLength)
{
    Error    error = kErrorNone;
    uint16_t tail  = GetLength() - (aOffset + aOldLength);

    OT_ASSERT(aOffset + aOldLength <= GetLength());

    if (aNewLength < aOldLength)
    {
        if (tail > 0)
        {
            WriteBytesFromMessage(aOffset + aNewLength, *this, aOffset + aOldLength, tail);
        }
        SuccessOrExit(error = ResizeMessage(GetLength() - (aOldLength - aNewLength)));
    }
    else if (aNewLength > aOldLength)
    {
        SuccessOrExit(error = ResizeMessage(GetLength() + (aNewLength - aOldLength)));
        if (tail > 0)
        {
            WriteBytesFromMessage(aOffset + aNewLength, *this, aOffset + aOldLength, tail);
        }
    }

exit:
    return error;
}

void Message::WriteBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    const uint8_t *bufPtr = reinterpret_cast<const uint8_t *>(aBuf);
    MutableChunk   chunk;

    OT_ASSERT(aOffset + aLength <= GetLength());

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        memmove(chunk.GetBytes(), bufPtr, chunk.GetLength());
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }
}

void Message::WriteBytesFromMessage(uint16_t       aWriteOffset,
                                    const Message &aMessage,
                                    uint16_t       aReadOffset,
                                    uint16_t       aLength)
{
    if ((&aMessage != this) || (aReadOffset >= aWriteOffset))
    {
        Chunk chunk;

        aMessage.GetFirstChunk(aReadOffset, aLength, chunk);

        while (chunk.GetLength() > 0)
        {
            WriteBytes(aWriteOffset, chunk.GetBytes(), chunk.GetLength());
            aWriteOffset += chunk.GetLength();
            aMessage.GetNextChunk(aLength, chunk);
        }
    }
    else
    {
        // We are copying bytes within the same message forward.
        // To ensure copy forward works, we read and write from
        // end of range and move backwards.

        static constexpr uint16_t kBufSize = 32;

        uint8_t buf[kBufSize];

        aWriteOffset += aLength;
        aReadOffset += aLength;

        while (aLength > 0)
        {
            uint16_t copyLength = Min(kBufSize, aLength);

            aLength -= copyLength;
            aReadOffset -= copyLength;
            aWriteOffset -= copyLength;

            ReadBytes(aReadOffset, buf, copyLength);
            WriteBytes(aWriteOffset, buf, copyLength);
        }
    }
}

Message *Message::Clone(uint16_t aLength) const
{
    Error    error = kErrorNone;
    Message *messageCopy;
    Settings settings(IsLinkSecurityEnabled() ? kWithLinkSecurity : kNoLinkSecurity, GetPriority());
    uint16_t offset;

    aLength     = Min(GetLength(), aLength);
    messageCopy = Get<MessagePool>().Allocate(GetType(), GetReserved(), settings);
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = messageCopy->AppendBytesFromMessage(*this, 0, aLength));

    // Copy selected message information.

    offset = Min(GetOffset(), aLength);
    messageCopy->SetOffset(offset);

    messageCopy->SetSubType(GetSubType());
    messageCopy->SetLoopbackToHostAllowed(IsLoopbackToHostAllowed());
    messageCopy->SetOrigin(GetOrigin());
    messageCopy->SetTimestamp(GetTimestamp());
    messageCopy->SetMeshDest(GetMeshDest());
    messageCopy->SetPanId(GetPanId());
    messageCopy->SetChannel(GetChannel());
    messageCopy->SetRssAverager(GetRssAverager());
    messageCopy->SetLqiAverager(GetLqiAverager());
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    messageCopy->SetTimeSync(IsTimeSync());
#endif

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

Error Message::GetLinkInfo(ThreadLinkInfo &aLinkInfo) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsOriginThreadNetif(), error = kErrorNotFound);

    aLinkInfo.Clear();

    aLinkInfo.mPanId               = GetPanId();
    aLinkInfo.mChannel             = GetChannel();
    aLinkInfo.mRss                 = GetAverageRss();
    aLinkInfo.mLqi                 = GetAverageLqi();
    aLinkInfo.mLinkSecurity        = IsLinkSecurityEnabled();
    aLinkInfo.mIsDstPanIdBroadcast = IsDstPanIdBroadcast();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    aLinkInfo.mTimeSyncSeq       = GetTimeSyncSeq();
    aLinkInfo.mNetworkTimeOffset = GetNetworkTimeOffset();
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    aLinkInfo.mRadioType = GetRadioType();
#endif

exit:
    return error;
}

void Message::UpdateLinkInfoFrom(const ThreadLinkInfo &aLinkInfo)
{
    SetPanId(aLinkInfo.mPanId);
    SetChannel(aLinkInfo.mChannel);
    AddRss(aLinkInfo.mRss);
    AddLqi(aLinkInfo.mLqi);
    SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
    GetMetadata().mIsDstPanIdBroadcast = aLinkInfo.IsDstPanIdBroadcast();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SetTimeSyncSeq(aLinkInfo.mTimeSyncSeq);
    SetNetworkTimeOffset(aLinkInfo.mNetworkTimeOffset);
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    SetRadioType(static_cast<Mac::RadioType>(aLinkInfo.mRadioType));
#endif
}

bool Message::IsTimeSync(void) const
{
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    return GetMetadata().mTimeSync;
#else
    return false;
#endif
}

void Message::MarkAsNotInAQueue(void)
{
    // To indicate that a message is no longer in a queue, we set
    // its 'mPrev' pointer to point back to itself. This state is
    // unique and won't occur if the message is part of a queue. We
    // also set 'mNext' to 'nullptr' so that 'GetNext()' correctly
    // returns 'nullptr' for a dequeued message.

    Next() = nullptr;
    Prev() = this;

    GetMetadata().mInPriorityQ = false;
}

//---------------------------------------------------------------------------------------------------------------------
// MessageQueue

void MessageQueue::Enqueue(Message &aMessage, QueuePosition aPosition)
{
    OT_ASSERT(!aMessage.IsInAQueue());

    aMessage.GetMetadata().mInPriorityQ = false;

    if (GetHead() == nullptr)
    {
        aMessage.Next() = nullptr;
        aMessage.Prev() = nullptr;

        SetHead(&aMessage);
        SetTail(&aMessage);
    }
    else
    {
        switch (aPosition)
        {
        case kQueuePositionHead:
            aMessage.Next()   = GetHead();
            aMessage.Prev()   = nullptr;
            GetHead()->Prev() = &aMessage;
            SetHead(&aMessage);
            break;

        case kQueuePositionTail:
            aMessage.Next()   = nullptr;
            aMessage.Prev()   = GetTail();
            GetTail()->Next() = &aMessage;
            SetTail(&aMessage);
            break;
        }
    }
}

void MessageQueue::Dequeue(Message &aMessage)
{
    if (&aMessage == GetHead())
    {
        SetHead(aMessage.Next());
    }

    if (&aMessage == GetTail())
    {
        SetTail(aMessage.Prev());
    }

    if (aMessage.Prev() != nullptr)
    {
        aMessage.Prev()->Next() = aMessage.Next();
    }

    if (aMessage.Next() != nullptr)
    {
        aMessage.Next()->Prev() = aMessage.Prev();
    }

    aMessage.MarkAsNotInAQueue();
}

void MessageQueue::DequeueAndFree(Message &aMessage)
{
    Dequeue(aMessage);
    aMessage.Free();
}

void MessageQueue::DequeueAndFreeAll(void)
{
    Message *message;

    while ((message = GetHead()) != nullptr)
    {
        DequeueAndFree(*message);
    }
}

Message::Iterator MessageQueue::begin(void) { return Message::Iterator(GetHead()); }

Message::ConstIterator MessageQueue::begin(void) const { return Message::ConstIterator(GetHead()); }

void MessageQueue::GetInfo(Info &aInfo) const
{
    ClearAllBytes(aInfo);

    for (const Message &message : *this)
    {
        aInfo.mNumMessages++;
        aInfo.mNumBuffers += message.GetBufferCount();
        aInfo.mTotalBytes += message.GetLength();
    }
}

void MessageQueue::AddQueueInfos(Info &aInfo, const Info &aOther)
{
    aInfo.mNumMessages += aOther.mNumMessages;
    aInfo.mNumBuffers += aOther.mNumBuffers;
    aInfo.mTotalBytes += aOther.mTotalBytes;
}

//---------------------------------------------------------------------------------------------------------------------
// PriorityQueue

const Message *PriorityQueue::FindTailForPriorityOrHigher(uint8_t aPriority) const
{
    // This method finds the tail (last message entry in the list)
    // that has priority level that is greater than or equal to
    // `aPriority` and currently has messages in the queue.
    //
    // The `PriorityQueue` uses the `mTails` array to store pointers
    // to the last message for each priority level. If a specific
    // priority level has no messages, its corresponding `mTails`
    // entry will be `nullptr`.
    //
    // The method iterates from `aPriority` upwards through higher
    // priority levels, returning the first non-null tail pointer it
    // encounters.
    //
    // The returned `Message` pointer indicates where a new message
    // with `aPriority` should be inserted: it should be placed
    // immediately after this returned message. If `nullptr` is
    // returned, it means the new message will be the first of its
    // priority level (or any higher priority) in the queue, and
    // should be placed at the head.

    const Message *tail = nullptr;

    for (uint8_t priority = aPriority; priority < Message::kNumPriorities; priority++)
    {
        tail = mTails[priority];

        if (tail != nullptr)
        {
            break;
        }
    }

    return tail;
}

const Message *PriorityQueue::GetHeadForPriority(Message::Priority aPriority) const
{
    const Message *head     = nullptr;
    uint8_t        priority = static_cast<uint8_t>(aPriority);

    if (mTails[priority] == nullptr)
    {
        head = nullptr;
    }
    else if (mHead->GetPriority() == aPriority)
    {
        head = mHead;
    }
    else
    {
        const Message *previousTail = FindTailForPriorityOrHigher(priority + 1);

        OT_ASSERT(previousTail != nullptr);
        head = previousTail->Next();
    }

    return head;
}

const Message *PriorityQueue::GetTail(void) const { return FindTailForPriorityOrHigher(Message::kPriorityLow); }

void PriorityQueue::Enqueue(Message &aMessage)
{
    uint8_t priority;

    OT_ASSERT(!aMessage.IsInAQueue());

    aMessage.GetMetadata().mInPriorityQ = true;

    priority = aMessage.GetPriority();

    // We insert the new `aMessage` immediately after the message
    // returned by `FindTailForPriorityOrHigher()`.
    //
    // If `FindTailForPriorityOrHigher()` returns `nullptr`, it means
    // `aMessage` has the highest priority of all existing messages
    // or is the first message in the queue, so we add it at the head
    // and update `mHead` accordingly.
    //
    // We first set the `Prev` and `Next` pointers within `aMessage`
    // itself. Afterward, we update the `Next()` pointer of the
    // preceding message (if any) and the `Prev()` pointer of the
    // succeeding message (if any) to point back to the newly
    // inserted `aMessage`, maintaining the linked list.

    aMessage.Prev() = FindTailForPriorityOrHigher(priority);

    if (aMessage.Prev() == nullptr)
    {
        aMessage.Next() = mHead;
        mHead           = &aMessage;
    }
    else
    {
        aMessage.Next()         = aMessage.Prev()->Next();
        aMessage.Prev()->Next() = &aMessage;
    }

    if (aMessage.Next() != nullptr)
    {
        aMessage.Next()->Prev() = &aMessage;
    }

    mTails[priority] = &aMessage;
}

void PriorityQueue::Dequeue(Message &aMessage)
{
    Message::Priority priority;

    OT_ASSERT(aMessage.IsInAPriorityQueue());

    priority = aMessage.GetPriority();

    // If `aMessage` is the current tail for its priority, update
    // `mTails[priority]`. The new tail becomes the preceding
    // message entry. If the preceding message has a different
    // (higher) priority, or if there's no preceding message, it
    // means `aMessage` was the last of its priority, and the
    // `mTails[priority]` is set to `nullptr`.

    if (&aMessage == mTails[priority])
    {
        mTails[priority] = aMessage.Prev();

        if ((mTails[priority] != nullptr) && (mTails[priority]->GetPriority() != priority))
        {
            mTails[priority] = nullptr;
        }
    }

    if (&aMessage == mHead)
    {
        mHead = aMessage.Next();
    }

    if (aMessage.Next() != nullptr)
    {
        aMessage.Next()->Prev() = aMessage.Prev();
    }

    if (aMessage.Prev() != nullptr)
    {
        aMessage.Prev()->Next() = aMessage.Next();
    }

    aMessage.MarkAsNotInAQueue();
}

void PriorityQueue::DequeueAndFree(Message &aMessage)
{
    Dequeue(aMessage);
    aMessage.Free();
}

void PriorityQueue::DequeueAndFreeAll(void)
{
    Message *message;

    while ((message = GetHead()) != nullptr)
    {
        DequeueAndFree(*message);
    }
}

Message::Iterator PriorityQueue::begin(void) { return Message::Iterator(GetHead()); }

Message::ConstIterator PriorityQueue::begin(void) const { return Message::ConstIterator(GetHead()); }

void PriorityQueue::GetInfo(Info &aInfo) const
{
    ClearAllBytes(aInfo);

    for (const Message &message : *this)
    {
        aInfo.mNumMessages++;
        aInfo.mNumBuffers += message.GetBufferCount();
        aInfo.mTotalBytes += message.GetLength();
    }
}

} // namespace ot
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
