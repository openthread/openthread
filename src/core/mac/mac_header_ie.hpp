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

#ifndef MAC_HEADER_IE_HPP_
#define MAC_HEADER_IE_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/encoding.hpp"
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
 * Implements IEEE 802.15.4 IE (Information Element) header generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class HeaderIe
{
public:
    /**
     * Initializes the Header IE.
     */
    void Init(void) { mFields.m16 = 0; }

    /**
     * Initializes the Header IE with Id and Length.
     *
     * @param[in]  aId   The IE Element Id.
     * @param[in]  aLen  The IE content length.
     */
    void Init(uint16_t aId, uint8_t aLen);

    /**
     * Returns the IE Element Id.
     *
     * @returns the IE Element Id.
     */
    uint16_t GetId(void) const { return (LittleEndian::HostSwap16(mFields.m16) & kIdMask) >> kIdOffset; }

    /**
     * Sets the IE Element Id.
     *
     * @param[in]  aId  The IE Element Id.
     */
    void SetId(uint16_t aId)
    {
        mFields.m16 = LittleEndian::HostSwap16((LittleEndian::HostSwap16(mFields.m16) & ~kIdMask) |
                                               ((aId << kIdOffset) & kIdMask));
    }

    /**
     * Returns the IE content length.
     *
     * @returns the IE content length.
     */
    uint8_t GetLength(void) const { return mFields.m8[0] & kLengthMask; }

    /**
     * Sets the IE content length.
     *
     * @param[in]  aLength  The IE content length.
     */
    void SetLength(uint8_t aLength) { mFields.m8[0] = (mFields.m8[0] & ~kLengthMask) | (aLength & kLengthMask); }

private:
    // Header IE format:
    //
    // +-----------+------------+--------+
    // | Bits: 0-6 |    7-14    |   15   |
    // +-----------+------------+--------+
    // | Length    | Element ID | Type=0 |
    // +-----------+------------+--------+

    static constexpr uint8_t  kSize       = 2;
    static constexpr uint8_t  kIdOffset   = 7;
    static constexpr uint8_t  kLengthMask = 0x7f;
    static constexpr uint16_t kIdMask     = 0x00ff << kIdOffset;

    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[kSize];
        uint16_t m16;
    } mFields;

} OT_TOOL_PACKED_END;

/**
 * Implements CSL IE data structure.
 */
OT_TOOL_PACKED_BEGIN
class CslIe
{
public:
    static constexpr uint8_t kHeaderIeId    = 0x1a;
    static constexpr uint8_t kIeContentSize = sizeof(uint16_t) * 2;

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
    uint16_t mPhase;
    uint16_t mPeriod;
} OT_TOOL_PACKED_END;

/**
 * Implements Termination2 IE.
 *
 * Is empty for template specialization.
 */
class Termination2Ie
{
public:
    static constexpr uint8_t kHeaderIeId    = 0x7f;
    static constexpr uint8_t kIeContentSize = 0;
};

/**
 * Implements vendor specific Header IE generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class VendorIeHeader
{
public:
    static constexpr uint8_t kHeaderIeId    = 0x00;
    static constexpr uint8_t kIeContentSize = sizeof(uint8_t) * 4;

    /**
     * Returns the Vendor OUI.
     *
     * @returns The Vendor OUI.
     */
    uint32_t GetVendorOui(void) const { return LittleEndian::ReadUint24(mOui); }

    /**
     * Sets the Vendor OUI.
     *
     * @param[in]  aVendorOui  A Vendor OUI.
     */
    void SetVendorOui(uint32_t aVendorOui) { LittleEndian::WriteUint24(aVendorOui, mOui); }

    /**
     * Returns the Vendor IE sub-type.
     *
     * @returns The Vendor IE sub-type.
     */
    uint8_t GetSubType(void) const { return mSubType; }

    /**
     * Sets the Vendor IE sub-type.
     *
     * @param[in]  aSubType  The Vendor IE sub-type.
     */
    void SetSubType(uint8_t aSubType) { mSubType = aSubType; }

private:
    static constexpr uint8_t kOuiSize = 3;

    uint8_t mOui[kOuiSize];
    uint8_t mSubType;
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
/**
 * Implements Time Header IE generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class TimeIe : public VendorIeHeader
{
public:
    static constexpr uint32_t kVendorOuiNest = 0x18b430;
    static constexpr uint8_t  kVendorIeTime  = 0x01;
    static constexpr uint8_t  kHeaderIeId    = VendorIeHeader::kHeaderIeId;
    static constexpr uint8_t  kIeContentSize = VendorIeHeader::kIeContentSize + sizeof(uint8_t) + sizeof(uint64_t);

    /**
     * Initializes the time IE.
     */
    void Init(void)
    {
        SetVendorOui(kVendorOuiNest);
        SetSubType(kVendorIeTime);
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

private:
    uint8_t  mSequence;
    uint64_t mTime;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

class ThreadIe
{
public:
    static constexpr uint8_t  kHeaderIeId               = VendorIeHeader::kHeaderIeId;
    static constexpr uint8_t  kIeContentSize            = VendorIeHeader::kIeContentSize;
    static constexpr uint32_t kVendorOuiThreadCompanyId = 0xeab89b;
    static constexpr uint8_t  kEnhAckProbingIe          = 0x00;
};

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
/**
 * Implements Thread Header IE generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class ThreadIeHeader
{
public:
    static constexpr uint8_t kHeaderIeId    = 0x21;
    static constexpr uint8_t kIeContentSize = sizeof(uint8_t);

    /**
     * Returns the command.
     *
     * @returns The command.
     */
    uint8_t GetCommand(void) const { return mCommand; }

    /**
     * Sets the command.
     *
     * @param[in]  aCommand  A command.
     */
    void SetCommand(uint8_t aCommand) { mCommand = aCommand; }

private:
    uint8_t mCommand;
} OT_TOOL_PACKED_END;

/**
 * This class implements Rendezvous Time IE data structure.
 *
 * IEEE 802.15.4 Rendezvous Time IE contains two fields, Rendezvous Time and
 * Wake-up Interval, but the Wake-up Interval is not used in Thread, so it is
 * not included in this class.
 */
OT_TOOL_PACKED_BEGIN
class RendezvousTimeIe
{
public:
    static constexpr uint8_t kHeaderIeId    = 0x1d;
    static constexpr uint8_t kIeContentSize = sizeof(uint16_t);

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
    uint16_t mRendezvousTime;
} OT_TOOL_PACKED_END;

/**
 * Implements Connection IE data structure.
 */
OT_TOOL_PACKED_BEGIN
class ConnectionIe : public VendorIeHeader
{
public:
    static constexpr uint8_t kHeaderIeId      = ThreadIe::kHeaderIeId;
    static constexpr uint8_t kIeContentSize   = ThreadIe::kIeContentSize + sizeof(uint16_t);
    static constexpr uint8_t kThreadIeSubtype = 0x01;

    enum WakeupTarget : uint8_t
    {
        kWakeupTargetPeer           = 0, ///< A Peer Thread device.
        kWakeupTargetSpecificParent = 1, ///< The current Parent of the WED as indicated by address.
        kWakeupTargetAnyParent      = 2, ///< Any Parent of the WED.
    };

    /**
     * Initializes the Connection IE.
     */
    void Init(void)
    {
        SetVendorOui(ThreadIe::kVendorOuiThreadCompanyId);
        SetSubType(kThreadIeSubtype);
        mConnectionWindow = 0;
        mFlags            = 0;
    }

    /**
     * Returns the Retry Interval.
     *
     * The Retry Interval defines how frequently the Wake-up End Device is
     * supposed to retry sending the Parent Request to the Wake-up Coordinator.
     *
     * @returns the Retry Interval in the units of Wake-up Intervals (7.5ms by default).
     */
    uint8_t GetRetryInterval(void) const { return (mConnectionWindow & kRetryIntervalMask) >> kRetryIntervalOffset; }

    /**
     * Sets the Retry Interval.
     *
     * @param[in]  aRetryInterval  The Retry Interval in the units of Wake-up Intervals (7.5ms by default).
     */
    void SetRetryInterval(uint8_t aRetryInterval)
    {
        mConnectionWindow = (aRetryInterval << kRetryIntervalOffset) | (mConnectionWindow & ~kRetryIntervalMask);
    }

    /**
     * Returns the Retry Count.
     *
     * The Retry Count defines how many times the Wake-up End Device is supposed
     * to retry sending the Parent Request to the Wakeup Coordinator.
     *
     * @returns the Retry Count.
     */
    uint8_t GetRetryCount(void) const { return (mConnectionWindow & kRetryCountMask) >> kRetryCountOffset; }

    /**
     * Sets the Retry Count.
     *
     * @param[in]  aRetryCount  The Retry Count.
     */
    void SetRetryCount(uint8_t aRetryCount)
    {
        mConnectionWindow = (aRetryCount << kRetryCountOffset) | (mConnectionWindow & ~kRetryCountMask);
    }

    /**
     * Sets the wake-up target.
     *
     * @param[in]  aTarget  The wake-up target.
     */
    void SetWakeupTarget(WakeupTarget aTarget)
    {
        mConnectionWindow = (aTarget << kWakeupTargetOffset) | (mConnectionWindow & ~kWakeupTargetMask);
    }

    /**
     * Returns the wake-up target.
     *
     * @returns the wake-up target.
     */
    WakeupTarget GetWakeupTarget(void) const
    {
        return static_cast<WakeupTarget>((mConnectionWindow & kWakeupTargetMask) >> kWakeupTargetOffset);
    }

    /**
     * Sets the Group Wake-up flag.
     *
     * @param[in]  aGroupWakeupFlag  The Group Wake-up flag.
     */
    void SetGroupWakeupFlag(bool aGroupWakeupFlag) { SetFlag(kGroupWakeupFlagOffset, aGroupWakeupFlag); }

    /**
     * Indicates whether or not the Group Wake-up flag is set.
     *
     * @returns TRUE if the Group Wake-up flag is set, FALSE otherwise.
     */
    bool GetGroupWakeupFlag(void) const { return GetFlag(kGroupWakeupFlagOffset); }

    /**
     * Sets the Attached flag.
     *
     * @param[in]  aAttachedFlag  The Attached flag.
     */
    void SetAttachedFlag(bool aAttachedFlag) { SetFlag(kAttachedFlagOffset, aAttachedFlag); }

    /**
     * Indicates whether or not the Attached flag is set.
     *
     * @returns TRUE if the Attached flag is set, FALSE otherwise.
     */
    bool GetAttachedFlag(void) const { return GetFlag(kAttachedFlagOffset); }

    /**
     * Sets the Router flag.
     *
     * @param[in]  aRouterFlag  The Router flag.
     */
    void SetRouterFlag(bool aRouterFlag) { SetFlag(kRouterFlagOffset, aRouterFlag); }

    /**
     * Indicates whether or not the Router flag is set.
     *
     * @returns TRUE if the Router flag is set, FALSE otherwise.
     */
    bool GetRouterFlag(void) const { return GetFlag(kRouterFlagOffset); }

    /**
     * Sets the Network Data flag.
     *
     * @param[in]  aNetworkDataFlag  The Network Data flag.
     */
    void SetNetworkDataFlag(bool aNetworkDataFlag) { SetFlag(kNetworkDataFlagOffset, aNetworkDataFlag); }

    /**
     * Indicates whether or not the Network Data flag is set.
     *
     * @returns TRUE if the Network Data flag is set, FALSE otherwise.
     */
    bool GetNetworkDataFlag(void) const { return GetFlag(kNetworkDataFlagOffset); }

    /**
     * Sets the Wake-up Identifier.
     *
     * @param[in]  aWakeupId  A reference to the Wake-up Identifier.
     *
     * @retval kErrorNone         Successfully set the Wake-up Identifier.
     * @retval kErrorInvalidArgs  The length of the given Wake-up Identifier didn't match the reserved length.
     */
    Error SetWakeupId(WakeupId aWakeupId);

    /**
     * Gets the Wake-up Identifier.
     *
     * @param[out]  aWakeupId  A reference to the Wake-up Identifier.
     *
     * @retval kErrorNone      Successfully got the Wake-up Identifier.
     * @retval kErrorNotFound  The Wake-up Identifier is not found.
     */
    Error GetWakeupId(WakeupId &aWakeupId) const;

    /**
     * Gets the pointer to the HeaderIe of this ConnectionIe.
     *
     * @returns A pointer to the HeaderIe.
     */
    const HeaderIe *GetHeaderIe(void) const
    {
        return reinterpret_cast<const HeaderIe *>(reinterpret_cast<const uint8_t *>(this) - sizeof(HeaderIe));
    }

private:
    static constexpr uint8_t kWakeupTargetOffset  = 0;
    static constexpr uint8_t kWakeupTargetMask    = 0x3 << kWakeupTargetOffset;
    static constexpr uint8_t kRetryIntervalOffset = 2;
    static constexpr uint8_t kRetryIntervalMask   = 0x3 << kRetryIntervalOffset;
    static constexpr uint8_t kRetryCountOffset    = 4;
    static constexpr uint8_t kRetryCountMask      = 0xf << kRetryCountOffset;

    static constexpr uint8_t kGroupWakeupFlagOffset = 0;
    static constexpr uint8_t kAttachedFlagOffset    = 1;
    static constexpr uint8_t kRouterFlagOffset      = 2;
    static constexpr uint8_t kNetworkDataFlagOffset = 3;

    const uint8_t *GetWakeupIdData(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }
    uint8_t       *GetWakeupIdData(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    void SetFlag(uint8_t aOffset, bool aValue)
    {
        if (aValue)
        {
            mFlags = mFlags | (1 << aOffset);
        }
        else
        {
            mFlags = mFlags & (~(1 << aOffset));
        }
    }

    bool GetFlag(uint8_t aOffset) const { return mFlags & (1 << aOffset); }

    uint8_t mConnectionWindow;
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * Implements CSA (Scheduled Channel Access) IE data structure.
 */
OT_TOOL_PACKED_BEGIN
class ScaIe : public VendorIeHeader
{
public:
    static constexpr uint8_t kHeaderIeId      = ThreadIe::kHeaderIeId;
    static constexpr uint8_t kIeContentSize   = ThreadIe::kIeContentSize + sizeof(uint16_t) + sizeof(uint32_t);
    static constexpr uint8_t kThreadIeSubtype = 0x02;

    /**
     * Initializes the Connection IE.
     */
    void Init(void)
    {
        SetVendorOui(ThreadIe::kVendorOuiThreadCompanyId);
        SetSubType(kThreadIeSubtype);
        mPhaseAndDuration = 0;
    }

    /**
     * Gets the pointer to the HeaderIe of this ConnectionIe.
     *
     * @returns A pointer to the HeaderIe.
     */
    const HeaderIe *GetHeaderIe(void) const
    {
        return reinterpret_cast<const HeaderIe *>(reinterpret_cast<const uint8_t *>(this) - sizeof(HeaderIe));
    }

    /**
     * Sets the time of the first symbol of the frame relative to start of the 1st slot, ranging in [-1024, 1023] us
     *
     * @param[in]  aPhase  The radio availability map phase.
     */
    void SetRamPhase(uint16_t aPhase)
    {
        mPhaseAndDuration = LittleEndian::HostSwap16(((aPhase << kPhaseOffset) & kPhaseMask) |
                                                     (LittleEndian::HostSwap16(mPhaseAndDuration) & (~kPhaseMask)));
    }

    /**
     * Gets the pointer to the radio availability map phase.
     *
     * @returns The radio availability map phase.
     */
    uint16_t GetRamPhase(void) const
    {
        return (LittleEndian::HostSwap16(mPhaseAndDuration) & kPhaseMask) >> kPhaseOffset;
    }

    void SetNumBits(uint8_t aDuration)
    {
        mPhaseAndDuration = LittleEndian::HostSwap16(((aDuration << kDurationOffset) & kDurationMask) |
                                                     (LittleEndian::HostSwap16(mPhaseAndDuration) & (~kDurationMask)));
    }

    uint8_t GetNumBits(void) const
    {
        return (LittleEndian::HostSwap16(mPhaseAndDuration) & kDurationMask) >> kDurationOffset;
    }

    void SetRamBits(uint32_t aRamBits) { mRamBits = LittleEndian::HostSwap32(aRamBits); }

    uint32_t GetRamBits(void) const { return LittleEndian::HostSwap32(mRamBits); }

    /**
     * Returns the ECSL Period.
     *
     * @returns the ECSL Period.
     */
    uint16_t GetPeriod(void) const { return LittleEndian::HostSwap16(mECslPeriod); }

    /**
     * Sets the ECSL Period.
     *
     * @param[in]  aPeriod  The ECSL Period.
     */
    void SetPeriod(uint16_t aPeriod) { mECslPeriod = LittleEndian::HostSwap16(aPeriod); }

    /**
     * Returns the ECSL Phase.
     *
     * @returns the ECSL Phase.
     */
    uint16_t GetPhase(void) const { return LittleEndian::HostSwap16(mECslPhase); }

    /**
     * Sets the ECSL Phase.
     *
     * @param[in]  aPhase  The ECSL Phase.
     */
    void SetPhase(uint16_t aPhase) { mECslPhase = LittleEndian::HostSwap16(aPhase); }

private:
    static constexpr uint8_t  kPhaseOffset    = 0;
    static constexpr uint16_t kPhaseMask      = 0x7ff << kPhaseOffset;
    static constexpr uint8_t  kDurationOffset = 11;
    static constexpr uint16_t kDurationMask   = 0x1f << kDurationOffset;
    static constexpr uint8_t  kRxPeriodOffset = 0;
    static constexpr uint32_t kRxPeriodMask   = 0xfff << kRxPeriodOffset;
    static constexpr uint8_t  kRxPhaseOffset  = 12;
    static constexpr uint32_t kRxPhaseMask    = 0xfff << kRxPhaseOffset;

    const uint8_t *GetBitsData(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }
    uint8_t       *GetBitsData(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    const uint32_t *GetRxSample(void) const
    {
        uint8_t numBits  = GetNumBits();
        uint8_t numBytes = (numBits + kBitsPerByte - 1) / kBitsPerByte;
        return reinterpret_cast<const uint32_t *>(GetBitsData() + numBytes);
    }

    uint32_t *GetRxSample(void) { return AsNonConst(AsConst(this)->GetRxSample()); }

    uint16_t mPhaseAndDuration;
    uint32_t mRamBits;
    uint16_t mECslPeriod;
    uint16_t mECslPhase;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // MAC_HEADER_IE_HPP_
