/*
 *  Copyright (c) 2016-2024, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing IEEE 802.15.4 IE (Information Element).
 */

#ifndef OT_CORE_MAC_MAC_HEADER_IE_HPP_
#define OT_CORE_MAC_MAC_HEADER_IE_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/frame_builder.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "mac/mac_types.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

/**
 * Implements IEEE 802.15.4 IE (Information Element) generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class HeaderIe
{
public:
    static constexpr uint8_t kMaxLength = 127; ///< Maximum Header IE length in bytes.

    /**
     * Returns the IE Element ID.
     *
     * @returns the IE Element ID.
     */
    uint8_t GetId(void) const { return static_cast<uint8_t>(ReadBitsLittleEndian<uint16_t, kIdMask>(mLenIdType)); }

    /**
     * Returns the IE content length.
     *
     * @returns the IE content length.
     */
    uint8_t GetLength(void) const { return static_cast<uint8_t>(ReadBitsLittleEndian<uint16_t, kLenMask>(mLenIdType)); }

    /**
     * Returns the total size of the Header IE (descriptor header plus content length) in bytes.
     *
     * @note The total size fits in a `uint8_t` since the content length is limited to 7 bits (max 127 bytes).
     *
     * @returns The total size of the Header IE in bytes.
     */
    uint8_t GetSize(void) const { return GetLength() + sizeof(HeaderIe); }

    /**
     * Returns a pointer to the IE content bytes.
     *
     * @returns A pointer to the IE content bytes.
     */
    const uint8_t *GetContent(void) const { return GetBytes() + sizeof(HeaderIe); }

    /**
     * Returns a pointer to the IE content bytes.
     *
     * @returns A pointer to the IE content bytes.
     */
    uint8_t *GetContent(void) { return AsNonConst(AsConst(this)->GetContent()); }

    /**
     * Validates whether a given Header IE matches a specific IE subclass.
     *
     * This method checks whether @p aIe matches the Element ID of @p IeType (`IeType::kId`) and also casts @p aIe
     * to @p IeType to validate its content structure via `IeType::IsValid()`.
     *
     * @tparam IeType  The IE subclass type to validate against.
     *
     * @param[in] aIe  The Header IE to validate.
     *
     * @retval TRUE   @p aIe matches @p IeType and its content is well-formed.
     * @retval FALSE  @p aIe does not match @p IeType or its content is malformed.
     */
    template <typename IeType> static bool ValidateAs(const HeaderIe &aIe)
    {
        return (aIe.GetId() == IeType::kId) && static_cast<const IeType *>(&aIe)->IsValid();
    }

    /**
     * Represents the opaque type for a bookmark used by `StartIe()/EndIe()`.
     */
    typedef uint16_t Bookmark;

    /**
     * Starts appending a (variable-length) `HeaderIe` in a `FrameBuilder`.
     *
     * On success, this method appends a `HeaderIe` descriptor (with length initialized to zero) to @p aBuilder and
     * saves the current byte offset as @p aBookmark. The caller can then append the IE content bytes to @p aBuilder,
     * and finally call `EndIe()` to update the IE length field automatically.
     *
     * @param[in,out] aBuilder   The `FrameBuilder` instance to append to.
     * @param[in]     aId        The IE Element ID.
     * @param[out]    aBookmark  A reference to a `Bookmark` to save the start offset.
     *
     * @retval kErrorNone    Successfully started the `HeaderIe`.
     * @retval kErrorNoBufs  Insufficient space in @p aBuilder to append the `HeaderIe` header.
     */
    static Error StartIe(FrameBuilder &aBuilder, uint8_t aId, Bookmark &aBookmark);

    /**
     * Finishes appending a `HeaderIe` in a `FrameBuilder`.
     *
     * This method updates the length field of the `HeaderIe` previously started using `StartIe()`. It determines the
     * IE length based on the number of bytes appended to @p aBuilder since `StartIe()` was called.
     *
     * @param[in,out] aBuilder   The `FrameBuilder` instance.
     * @param[in]     aBookmark  The `Bookmark` used when calling `StartIe()`.
     *
     * @retval kErrorNone         Successfully finalized the `HeaderIe` length.
     * @retval kErrorInvalidArgs  The @p aBookmark is invalid or the appended IE length exceeds `kMaxLength`.
     */
    static Error EndIe(FrameBuilder &aBuilder, const Bookmark &aBookmark);

protected:
    void           Init(uint8_t aId, uint8_t aLen);
    uint8_t       *GetBytes(void) { return reinterpret_cast<uint8_t *>(this); }
    const uint8_t *GetBytes(void) const { return reinterpret_cast<const uint8_t *>(this); }

private:
    // IEEE 802.15.4 Header IE descriptor (2 bytes, little-endian):
    //
    // Bits 0-6  (7 bits) : Length of IE content in bytes (max 127).
    // Bits 7-14 (8 bits) : Element ID.
    // Bit 15    (1 bit)  : Type (0 for Header IE).

    static constexpr uint16_t kLenMask = 0x007f << 0;
    static constexpr uint16_t kIdMask  = 0x00ff << 7;

    void SetId(uint8_t aId) { mLenIdType = UpdateBitsLittleEndian<uint16_t, kIdMask>(mLenIdType, aId); }
    void SetLength(uint8_t aLength) { mLenIdType = UpdateBitsLittleEndian<uint16_t, kLenMask>(mLenIdType, aLength); }

    uint16_t mLenIdType;
} OT_TOOL_PACKED_END;

/**
 * Represents a CSL IE.
 */
OT_TOOL_PACKED_BEGIN
class CslIe : public HeaderIe
{
    friend class HeaderIe;

public:
    static constexpr uint8_t kId = 0x1a; ///< The CSL IE Element ID.

    /**
     * Initializes the CSL IE.
     */
    void Init(void) { HeaderIe::Init(kId, sizeof(CslIe) - sizeof(HeaderIe)); }

    /**
     * Returns the CSL Period.
     *
     * @returns the CSL Period.
     */
    uint16_t GetPeriod(void) const { return LittleEndian::HostSwap16(mPeriod); }

    /**
     * Sets the CSL Period.
     *
     * @param[in]  aPeriod  The CSL Period.
     */
    void SetPeriod(uint16_t aPeriod) { mPeriod = LittleEndian::HostSwap16(aPeriod); }

    /**
     * Returns the CSL Phase.
     *
     * @returns the CSL Phase.
     */
    uint16_t GetPhase(void) const { return LittleEndian::HostSwap16(mPhase); }

    /**
     * Sets the CSL Phase.
     *
     * @param[in]  aPhase  The CSL Phase.
     */
    void SetPhase(uint16_t aPhase) { mPhase = LittleEndian::HostSwap16(aPhase); }

private:
    bool IsValid(void) const { return GetSize() >= sizeof(CslIe); }

    uint16_t mPhase;
    uint16_t mPeriod;
} OT_TOOL_PACKED_END;

/**
 * Represents a Termination2 IE.
 */
OT_TOOL_PACKED_BEGIN
class Termination2Ie : public HeaderIe
{
    friend class HeaderIe;

public:
    static constexpr uint8_t kId = 0x7f; ///< The Termination2 IE Element ID.

    /**
     * Initializes the Termination2 IE.
     */
    void Init(void) { HeaderIe::Init(kId, sizeof(Termination2Ie) - sizeof(HeaderIe)); }

private:
    bool IsValid(void) const { return true; }
} OT_TOOL_PACKED_END;

/**
 * Represents a Vendor Header IE.
 */
OT_TOOL_PACKED_BEGIN
class VendorIe : public HeaderIe
{
public:
    static constexpr uint8_t kId = 0x00; ///< The Vendor Specific IE Element ID.

    /**
     * Returns the Vendor OUI.
     *
     * @returns The Vendor OUI.
     */
    uint32_t GetVendorOui(void) const { return LittleEndian::ReadUint24(mOui); }

    /**
     * Returns the Vendor IE sub-type.
     *
     * @returns The Vendor IE sub-type.
     */
    uint8_t GetSubType(void) const { return mSubType; }

protected:
    void SetVendorOui(uint32_t aVendorOui) { LittleEndian::WriteUint24(aVendorOui, mOui); }
    void SetSubType(uint8_t aSubType) { mSubType = aSubType; }

private:
    static constexpr uint8_t kOuiSize = 3;

    uint8_t mOui[kOuiSize];
    uint8_t mSubType;
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
/**
 * Represents a Time Header IE.
 *
 * This IE is not specified in the Thread specification and is a custom feature in OpenThread (using Vendor IE
 * with Nest OUI).
 */
OT_TOOL_PACKED_BEGIN
class TimeIe : public VendorIe
{
    friend class HeaderIe;

public:
    /**
     * Initializes the Time IE.
     */
    void Init(void)
    {
        HeaderIe::Init(kId, sizeof(TimeIe) - sizeof(HeaderIe));
        SetVendorOui(kVendorOuiNest);
        SetSubType(kSubType);
    }

    /**
     * Returns the time sync sequence.
     *
     * @returns the time sync sequence.
     */
    uint8_t GetSequence(void) const { return mSequence; }

    /**
     * Sets the tine sync sequence.
     *
     * @param[in]  aSequence The time sync sequence.
     */
    void SetSequence(uint8_t aSequence) { mSequence = aSequence; }

    /**
     * Returns the network time.
     *
     * @returns the network time, in microseconds.
     */
    uint64_t GetTime(void) const { return LittleEndian::HostSwap64(mTime); }

    /**
     * Sets the network time.
     *
     * @param[in]  aTime  The network time.
     */
    void SetTime(uint64_t aTime) { mTime = LittleEndian::HostSwap64(aTime); }

    /**
     * Returns a pointer to the start of Time IE specific data content (i.e., sequence field).
     *
     * @returns A pointer to the Time IE data content bytes.
     */
    const uint8_t *GetData(void) const { return &mSequence; }

private:
    bool IsValid(void) const
    {
        return (GetSize() >= sizeof(TimeIe)) && (GetVendorOui() == kVendorOuiNest) && (GetSubType() == kSubType);
    }

    static constexpr uint32_t kVendorOuiNest = 0x18b430;
    static constexpr uint8_t  kSubType       = 0x01;

    uint8_t  mSequence;
    uint64_t mTime;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

/**
 * Represents a Thread Vendor IE.
 */
OT_TOOL_PACKED_BEGIN
class ThreadVendorIe : public VendorIe
{
protected:
    static constexpr uint32_t kVendorOuiThread = 0xeab89b;
} OT_TOOL_PACKED_END;

/**
 * Represents a Link Metrics Probing IE (using in Enhanced Ack).
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsProbingIe : public ThreadVendorIe
{
    friend class HeaderIe;

public:
    /**
     * The maximum length of Link Metrics Data in bytes (Thread specification limits metrics to 2).
     */
    static constexpr uint8_t kMaxMetricsDataLen = 2;

    /**
     * Initializes the Link Metrics Probing IE.
     *
     * @param[in] aMetricsDataLen  The requested length of Link Metrics Data. If greater than `kMaxMetricsDataLen`,
     *                             then `kMaxMetricsDataLen` is used instead.
     */
    void Init(uint8_t aMetricsDataLen)
    {
        HeaderIe::Init(kId, sizeof(LinkMetricsProbingIe) - sizeof(HeaderIe) + Min(aMetricsDataLen, kMaxMetricsDataLen));
        SetVendorOui(kVendorOuiThread);
        SetSubType(kSubType);
    }

    /**
     * Returns the length of Link Metrics Data in bytes.
     *
     * @returns The length of Link Metrics Data in bytes.
     */
    uint8_t GetMetricsDataLen(void) const { return GetSize() - sizeof(LinkMetricsProbingIe); }

    /**
     * Returns a pointer to the Link Metrics Data bytes.
     *
     * @returns A pointer to the Link Metrics Data bytes.
     */
    const uint8_t *GetMetricsData(void) const { return GetBytes() + sizeof(LinkMetricsProbingIe); }

    /**
     * Writes Link Metrics Data content from a given buffer.
     *
     * @param[in] aData  A pointer to a buffer containing the data to write. The caller must ensure that at least
     *                   `GetMetricsDataLen()` bytes are available in @p aData.
     */
    void WriteMetricsDataFrom(const uint8_t *aData)
    {
        memcpy(AsNonConst(GetMetricsData()), aData, GetMetricsDataLen());
    }

private:
    static constexpr uint8_t kSubType = 0x00;

    bool IsValid(void) const
    {
        return (GetSize() >= sizeof(LinkMetricsProbingIe)) && (GetVendorOui() == kVendorOuiThread) &&
               (GetSubType() == kSubType);
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Rendezvous Time IE data structure.
 *
 * IEEE 802.15.4 Rendezvous Time IE contains two fields, Rendezvous Time and
 * Wake-up Interval, but the Wake-up Interval is not used in Thread, so it is
 * not included in this class.
 */
OT_TOOL_PACKED_BEGIN
class RendezvousTimeIe : public HeaderIe
{
    friend class HeaderIe;

public:
    static constexpr uint8_t kId = 0x1d; ///< The Rendezvous Time IE Element ID.

    /**
     * Initializes the Rendezvous Time IE.
     */
    void Init(void) { HeaderIe::Init(kId, sizeof(RendezvousTimeIe) - sizeof(HeaderIe)); }

    /**
     * This method returns the Rendezvous Time.
     *
     * @returns the Rendezvous Time in the units of 10 symbols.
     */
    uint16_t GetRendezvousTime(void) const { return LittleEndian::HostSwap16(mRendezvousTime); }

    /**
     * This method sets the Rendezvous Time.
     *
     * @param[in]  aRendezvousTime  The Rendezvous Time in the units of 10 symbols.
     */
    void SetRendezvousTime(uint16_t aRendezvousTime) { mRendezvousTime = LittleEndian::HostSwap16(aRendezvousTime); }

private:
    bool IsValid(void) const { return GetSize() >= sizeof(RendezvousTimeIe); }

    uint16_t mRendezvousTime;
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

/**
 * Implements Connection IE data structure.
 */
OT_TOOL_PACKED_BEGIN
class ConnectionIe : public ThreadVendorIe
{
    friend class HeaderIe;

public:
    /**
     * Initializes the Connection IE.
     *
     * @param[in] aWakeupIdLength  The length of the Wakeup ID field in bytes.
     */
    void Init(uint8_t aWakeupIdLength)
    {
        HeaderIe::Init(kId, sizeof(ConnectionIe) - sizeof(HeaderIe) + aWakeupIdLength);
        SetVendorOui(kVendorOuiThread);
        SetSubType(kSubType);
        mConnectionWindow = 0;
    }

    /**
     * Returns the Retry Interval.
     *
     * The Retry Interval defines how frequently the Wake-up End Device is
     * supposed to retry sending the Parent Request to the Wake-up Coordinator.
     *
     * @returns the Retry Interval in the units of Wake-up Intervals (7.5ms by default).
     */
    uint8_t GetRetryInterval(void) const { return ReadBits<uint8_t, kRetryIntervalMask>(mConnectionWindow); }

    /**
     * Sets the Retry Interval.
     *
     * @param[in]  aRetryInterval  The Retry Interval in the units of Wake-up Intervals (7.5ms by default).
     */
    void SetRetryInterval(uint8_t aRetryInterval)
    {
        WriteBits<uint8_t, kRetryIntervalMask>(mConnectionWindow, aRetryInterval);
    }

    /**
     * Returns the Retry Count.
     *
     * The Retry Count defines how many times the Wake-up End Device is supposed
     * to retry sending the Parent Request to the Wakeup Coordinator.
     *
     * @returns the Retry Count.
     */
    uint8_t GetRetryCount(void) const { return ReadBits<uint8_t, kRetryCountMask>(mConnectionWindow); }

    /**
     * Sets the Retry Count
     *
     * @param[in]  aRetryCount  The Retry Count.
     */
    void SetRetryCount(uint8_t aRetryCount) { WriteBits<uint8_t, kRetryCountMask>(mConnectionWindow, aRetryCount); }

    /**
     * Sets the Wake-up Identifier.
     *
     * @param[in]  aWakeupId  The Wake-up Identifier.
     *
     * @retval kErrorNone   Successfully set the Wake-up Identifier.
     * @retval kErrorParse  The length of the given Wake-up Identifier didn't match the reserved length.
     */
    Error SetWakeupId(WakeupId aWakeupId);

    /**
     * Gets the Wake-up Identifier.
     *
     * @param[out]  aWakeupId  A reference to the Wake-up Identifier.
     *
     * @retval kErrorNone    Successfully got the Wake-up Identifier.
     * @retval kErrorParse   Failed to parse the Wake-up Identifier from the Connection IE.
     */
    Error GetWakeupId(WakeupId &aWakeupId) const;

private:
    static constexpr uint8_t kSubType = 0x01;

    static constexpr uint8_t kRetryIntervalMask = 0x3 << 4;
    static constexpr uint8_t kRetryCountMask    = 0xf << 0;

    bool IsValid(void) const
    {
        return (GetSize() >= sizeof(ConnectionIe)) && (GetVendorOui() == kVendorOuiThread) &&
               (GetSubType() == kSubType);
    }

    uint8_t mConnectionWindow;
    // Followed by variable length Wakeup ID
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_HEADER_IE_HPP_
