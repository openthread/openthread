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
 *   This file includes definitions for generating and processing IEEE 802.15.4 MAC frames.
 */

#ifndef MAC_FRAME_HPP_
#define MAC_FRAME_HPP_

#include "openthread-core-config.h"

#include <limits.h>
#include <stdint.h>

#include "common/encoding.hpp"
#include "mac/mac_types.hpp"

namespace ot {

namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

/**
 * This class implements IEEE 802.15.4 IE (Information Element) generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class HeaderIe
{
public:
    enum
    {
        kIdOffset     = 7,
        kIdMask       = 0xff << kIdOffset,
        kLengthOffset = 0,
        kLengthMask   = 0x7f << kLengthOffset,
    };

    /**
     * This method initializes the Header IE.
     *
     */
    void Init(void) { mHeaderIe = 0; }

    /**
     * This method returns the IE Element Id.
     *
     * @returns the IE Element Id.
     *
     */
    uint16_t GetId(void) const { return (ot::Encoding::LittleEndian::HostSwap16(mHeaderIe) & kIdMask) >> kIdOffset; }

    /**
     * This method sets the IE Element Id.
     *
     * @param[in]  aID  The IE Element Id.
     *
     */
    void SetId(uint16_t aId)
    {
        mHeaderIe = ot::Encoding::LittleEndian::HostSwap16(
            (ot::Encoding::LittleEndian::HostSwap16(mHeaderIe) & ~kIdMask) | ((aId << kIdOffset) & kIdMask));
    }

    /**
     * This method returns the IE content length.
     *
     * @returns the IE content length.
     *
     */
    uint16_t GetLength(void) const
    {
        return (ot::Encoding::LittleEndian::HostSwap16(mHeaderIe) & kLengthMask) >> kLengthOffset;
    }

    /**
     * This method sets the IE content length.
     *
     * @param[in]  aLength  The IE content length.
     *
     */
    void SetLength(uint16_t aLength)
    {
        mHeaderIe =
            ot::Encoding::LittleEndian::HostSwap16((ot::Encoding::LittleEndian::HostSwap16(mHeaderIe) & ~kLengthMask) |
                                                   ((aLength << kLengthOffset) & kLengthMask));
    }

private:
    uint16_t mHeaderIe;
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
/**
 * This class implements vendor specific Header IE generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class VendorIeHeader
{
public:
    enum
    {
        kVendorOuiNest = 0x18b430,
        kVendorOuiSize = 3,
        kVendorIeTime  = 0x01,
    };

    /**
     * This method returns the Vendor OUI.
     *
     * @returns the Vendor OUI.
     *
     */
    const uint8_t *GetVendorOui(void) const { return mVendorOui; }

    /**
     * This method sets the Vendor OUI.
     *
     * @param[in]  aVendorOui  A pointer to the Vendor OUI.
     *
     */
    void SetVendorOui(const uint8_t *aVendorOui) { memcpy(mVendorOui, aVendorOui, kVendorOuiSize); }

    /**
     * This method returns the Vendor IE sub-type.
     *
     * @returns the Vendor IE sub-type.
     *
     */
    uint8_t GetSubType(void) const { return mSubType; }

    /**
     * This method sets the Vendor IE sub-type.
     *
     * @param[in] the Vendor IE sub-type.
     *
     */
    void SetSubType(uint8_t aSubType) { mSubType = aSubType; }

private:
    uint8_t mVendorOui[kVendorOuiSize];
    uint8_t mSubType;
} OT_TOOL_PACKED_END;

/**
 * This class implements Time Header IE generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TimeIe : public VendorIeHeader
{
public:
    /**
     * This method initializes the time IE.
     *
     */
    void Init(void)
    {
        uint8_t oui[3] = {VendorIeHeader::kVendorOuiNest & 0xff, (VendorIeHeader::kVendorOuiNest >> 8) & 0xff,
                          (VendorIeHeader::kVendorOuiNest >> 16) & 0xff};

        SetVendorOui(oui);
        SetSubType(VendorIeHeader::kVendorIeTime);
    }

    /**
     * This method returns the time sync sequence.
     *
     * @returns the time sync sequence.
     *
     */
    uint8_t GetSequence(void) const { return mSequence; }

    /**
     * This method sets the tine sync sequence.
     *
     * @param[in]  aSequence The time sync sequence.
     *
     */
    void SetSequence(uint8_t aSequence) { mSequence = aSequence; }

    /**
     * This method returns the network time.
     *
     * @returns the network time, in microseconds.
     *
     */
    uint64_t GetTime(void) const { return ot::Encoding::LittleEndian::HostSwap64(mTime); }

    /**
     * This method sets the network time.
     *
     * @param[in]  aTime  The network time.
     *
     */
    void SetTime(uint64_t aTime) { mTime = ot::Encoding::LittleEndian::HostSwap64(aTime); }

private:
    uint8_t  mSequence;
    uint64_t mTime;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

/**
 * This class implements IEEE 802.15.4 MAC frame generation and parsing.
 *
 */
class Frame : public otRadioFrame
{
public:
    enum
    {
        kMtu                 = OT_RADIO_FRAME_MAX_SIZE,
        kFcfSize             = sizeof(uint16_t),
        kDsnSize             = sizeof(uint8_t),
        kSecurityControlSize = sizeof(uint8_t),
        kFrameCounterSize    = sizeof(uint32_t),
        kCommandIdSize       = sizeof(uint8_t),
        kFcsSize             = sizeof(uint16_t),

        kFcfFrameBeacon      = 0 << 0,
        kFcfFrameData        = 1 << 0,
        kFcfFrameAck         = 2 << 0,
        kFcfFrameMacCmd      = 3 << 0,
        kFcfFrameTypeMask    = 7 << 0,
        kFcfSecurityEnabled  = 1 << 3,
        kFcfFramePending     = 1 << 4,
        kFcfAckRequest       = 1 << 5,
        kFcfPanidCompression = 1 << 6,
        kFcfIePresent        = 1 << 9,
        kFcfDstAddrNone      = 0 << 10,
        kFcfDstAddrShort     = 2 << 10,
        kFcfDstAddrExt       = 3 << 10,
        kFcfDstAddrMask      = 3 << 10,
        kFcfFrameVersion2006 = 1 << 12,
        kFcfFrameVersion2015 = 2 << 12,
        kFcfFrameVersionMask = 3 << 12,
        kFcfSrcAddrNone      = 0 << 14,
        kFcfSrcAddrShort     = 2 << 14,
        kFcfSrcAddrExt       = 3 << 14,
        kFcfSrcAddrMask      = 3 << 14,

        kSecNone      = 0 << 0,
        kSecMic32     = 1 << 0,
        kSecMic64     = 2 << 0,
        kSecMic128    = 3 << 0,
        kSecEnc       = 4 << 0,
        kSecEncMic32  = 5 << 0,
        kSecEncMic64  = 6 << 0,
        kSecEncMic128 = 7 << 0,
        kSecLevelMask = 7 << 0,

        kMic0Size   = 0,
        kMic32Size  = 32 / CHAR_BIT,
        kMic64Size  = 64 / CHAR_BIT,
        kMic128Size = 128 / CHAR_BIT,
        kMaxMicSize = kMic128Size,

        kKeyIdMode0    = 0 << 3,
        kKeyIdMode1    = 1 << 3,
        kKeyIdMode2    = 2 << 3,
        kKeyIdMode3    = 3 << 3,
        kKeyIdModeMask = 3 << 3,

        kKeySourceSizeMode0 = 0,
        kKeySourceSizeMode1 = 0,
        kKeySourceSizeMode2 = 4,
        kKeySourceSizeMode3 = 8,

        kKeyIndexSize = sizeof(uint8_t),

        kMacCmdAssociationRequest         = 1,
        kMacCmdAssociationResponse        = 2,
        kMacCmdDisassociationNotification = 3,
        kMacCmdDataRequest                = 4,
        kMacCmdPanidConflictNotification  = 5,
        kMacCmdOrphanNotification         = 6,
        kMacCmdBeaconRequest              = 7,
        kMacCmdCoordinatorRealignment     = 8,
        kMacCmdGtsRequest                 = 9,

        kHeaderIeVendor       = 0x00,
        kHeaderIeTermination2 = 0x7f,

        kInfoStringSize = 110, ///< Max chars needed for the info string representation (@sa ToInfoString()).
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToInfoString()` method.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This method indicates whether the frame is empty (no payload).
     *
     * @retval TRUE  The frame is empty (no PSDU payload).
     * @retval FALSE The frame is not empty.
     *
     */
    bool IsEmpty(void) const { return (mLength == 0); }

    /**
     * This method initializes the MAC header.
     *
     * @param[in]  aFcf          The Frame Control field.
     * @param[in]  aSecurityCtl  The Security Control field.
     *
     */
    void InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl);

    /**
     * This method validates the frame.
     *
     * @retval OT_ERROR_NONE    Successfully parsed the MAC header.
     * @retval OT_ERROR_PARSE   Failed to parse through the MAC header.
     *
     */
    otError ValidatePsdu(void) const;

    /**
     * This method returns the IEEE 802.15.4 Frame Type.
     *
     * @returns The IEEE 802.15.4 Frame Type.
     *
     */
    uint8_t GetType(void) const { return GetPsdu()[0] & kFcfFrameTypeMask; }

    /**
     * This method returns the IEEE 802.15.4 Frame Version.
     *
     * @returns The IEEE 802.15.4 Frame Version.
     *
     */
    uint16_t GetVersion(void) const { return GetFrameControlField() & kFcfFrameVersionMask; }

    /**
     * This method indicates whether or not security is enabled.
     *
     * @retval TRUE   If security is enabled.
     * @retval FALSE  If security is not enabled.
     *
     */
    bool GetSecurityEnabled(void) const { return (GetPsdu()[0] & kFcfSecurityEnabled) != 0; }

    /**
     * This method indicates whether or not the Frame Pending bit is set.
     *
     * @retval TRUE   If the Frame Pending bit is set.
     * @retval FALSE  If the Frame Pending bit is not set.
     *
     */
    bool GetFramePending(void) const { return (GetPsdu()[0] & kFcfFramePending) != 0; }

    /**
     * This method sets the Frame Pending bit.
     *
     * @param[in]  aFramePending  The Frame Pending bit.
     *
     */
    void SetFramePending(bool aFramePending);

    /**
     * This method indicates whether or not the Ack Request bit is set.
     *
     * @retval TRUE   If the Ack Request bit is set.
     * @retval FALSE  If the Ack Request bit is not set.
     *
     */
    bool GetAckRequest(void) const { return (GetPsdu()[0] & kFcfAckRequest) != 0; }

    /**
     * This method sets the Ack Request bit.
     *
     * @param[in]  aAckRequest  The Ack Request bit.
     *
     */
    void SetAckRequest(bool aAckRequest);

    /**
     * This method indicates whether or not IEs present.
     *
     * @retval TRUE   If IEs present.
     * @retval FALSE  If no IE present.
     *
     */
    bool IsIePresent(void) const { return (GetFrameControlField() & kFcfIePresent) != 0; }

    /**
     * This method returns the Sequence Number value.
     *
     * @returns The Sequence Number value.
     *
     */
    uint8_t GetSequence(void) const { return GetPsdu()[kSequenceIndex]; }

    /**
     * This method sets the Sequence Number value.
     *
     * @param[in]  aSequence  The Sequence Number value.
     *
     */
    void SetSequence(uint8_t aSequence) { GetPsdu()[kSequenceIndex] = aSequence; }

    /**
     * This method gets the Destination PAN Identifier.
     *
     * @param[out]  aPanId  The Destination PAN Identifier.
     *
     * @retval OT_ERROR_NONE   Successfully retrieved the Destination PAN Identifier.
     * @retval OT_ERROR_PARSE  Failed to parse the PAN Identifier.
     *
     */
    otError GetDstPanId(PanId &aPanId) const;

    /**
     * This method sets the Destination PAN Identifier.
     *
     * @param[in]  aPanId  The Destination PAN Identifier.
     *
     */
    void SetDstPanId(PanId aPanId);

    /**
     * This method gets the Destination Address.
     *
     * @param[out]  aAddress  The Destination Address.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Destination Address.
     *
     */
    otError GetDstAddr(Address &aAddress) const;

    /**
     * This method sets the Destination Address.
     *
     * @param[in]  aShortAddress  The Destination Address.
     *
     */
    void SetDstAddr(ShortAddress aShortAddress);

    /**
     * This method sets the Destination Address.
     *
     * @param[in]  aExtAddress  The Destination Address.
     *
     */
    void SetDstAddr(const ExtAddress &aExtAddress);

    /**
     * This method sets the Destination Address.
     *
     * @param[in]  aAddress  The Destination Address.
     *
     */
    void SetDstAddr(const Address &aAddress);

    /**
     * This method indicates whether or not the Src PanId is present.
     *
     * @returns TRUE if the Src PanId is present, FALSE otherwise.
     *
     */
    bool IsSrcPanIdPresent(uint16_t aFcf) const;

    /**
     * This method gets the Source PAN Identifier.
     *
     * @param[out]  aPanId  The Source PAN Identifier.
     *
     * @retval OT_ERROR_NONE   Successfully retrieved the Source PAN Identifier.
     *
     */
    otError GetSrcPanId(PanId &aPanId) const;

    /**
     * This method sets the Source PAN Identifier.
     *
     * @param[in]  aPanId  The Source PAN Identifier.
     *
     * @retval OT_ERROR_NONE   Successfully set the Source PAN Identifier.
     *
     */
    otError SetSrcPanId(PanId aPanId);

    /**
     * This method gets the Source Address.
     *
     * @param[out]  aAddress  The Source Address.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Source Address.
     *
     */
    otError GetSrcAddr(Address &aAddress) const;

    /**
     * This method sets the Source Address.
     *
     * @param[in]  aShortAddress  The Source Address.
     *
     */
    void SetSrcAddr(ShortAddress aShortAddress);

    /**
     * This method sets the Source Address.
     *
     * @param[in]  aExtAddress  The Source Address.
     *
     */
    void SetSrcAddr(const ExtAddress &aExtAddress);

    /**
     * This method sets the Source Address.
     *
     * @param[in]  aAddress  The Source Address.
     *
     */
    void SetSrcAddr(const Address &aAddress);

    /**
     * This method gets the Security Level Identifier.
     *
     * @param[out]  aSecurityLevel  The Security Level Identifier.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Security Level Identifier.
     *
     */
    otError GetSecurityLevel(uint8_t &aSecurityLevel) const;

    /**
     * This method gets the Key Identifier Mode.
     *
     * @param[out]  aSecurityLevel  The Key Identifier Mode.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Key Identifier Mode.
     *
     */
    otError GetKeyIdMode(uint8_t &aKeyIdMode) const;

    /**
     * This method gets the Frame Counter.
     *
     * @param[out]  aFrameCounter  The Frame Counter.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Frame Counter.
     *
     */
    otError GetFrameCounter(uint32_t &aFrameCounter) const;

    /**
     * This method sets the Frame Counter.
     *
     * @param[in]  aFrameCounter  The Frame Counter.
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter);

    /**
     * This method returns a pointer to the Key Source.
     *
     * @returns A pointer to the Key Source.
     *
     */
    const uint8_t *GetKeySource(void) const;

    /**
     * This method sets the Key Source.
     *
     * @param[in]  aKeySource  A pointer to the Key Source value.
     *
     */
    void SetKeySource(const uint8_t *aKeySource);

    /**
     * This method gets the Key Identifier.
     *
     * @param[out]  aKeyId  The Key Identifier.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Key Identifier.
     *
     */
    otError GetKeyId(uint8_t &aKeyId) const;

    /**
     * This method sets the Key Identifier.
     *
     * @param[in]  aKeyId  The Key Identifier.
     *
     */
    void SetKeyId(uint8_t aKeyId);

    /**
     * This method gets the Command ID.
     *
     * @param[out]  aCommandId  The Command ID.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the Command ID.
     *
     */
    otError GetCommandId(uint8_t &aCommandId) const;

    /**
     * This method sets the Command ID.
     *
     * @param[in]  aCommandId  The Command ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the Command ID.
     *
     */
    otError SetCommandId(uint8_t aCommandId);

    /**
     * This method indicates whether the frame is a MAC Data Request command (data poll)
     *
     * @returns TRUE if frame is a MAC Data Request command, FALSE otherwise.
     *
     */
    bool IsDataRequestCommand(void) const;

    /**
     * This method returns the MAC Frame Length.
     *
     * @returns The MAC Frame Length.
     *
     */
    uint16_t GetLength(void) const { return GetPsduLength(); }

    /**
     * This method sets the MAC Frame Length.
     *
     * @param[in]  aLength  The MAC Frame Length.
     *
     */
    void SetLength(uint16_t aLength) { SetPsduLength(aLength); }

    /**
     * This method returns the MAC header size.
     *
     * @returns The MAC header size.
     *
     */
    uint8_t GetHeaderLength(void) const;

    /**
     * This method returns the MAC footer size.
     *
     * @returns The MAC footer size.
     *
     */
    uint8_t GetFooterLength(void) const;

    /**
     * This method returns the current MAC Payload length.
     *
     * @returns The current MAC Payload length.
     *
     */
    uint16_t GetPayloadLength(void) const;

    /**
     * This method returns the maximum MAC Payload length for the given MAC header and footer.
     *
     * @returns The maximum MAC Payload length for the given MAC header and footer.
     *
     */
    uint16_t GetMaxPayloadLength(void) const;

    /**
     * This method sets the MAC Payload length.
     *
     */
    void SetPayloadLength(uint16_t aLength);

    /**
     * This method returns the IEEE 802.15.4 channel used for transmission or reception.
     *
     * @returns The IEEE 802.15.4 channel used for transmission or reception.
     *
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * This method sets the IEEE 802.15.4 channel used for transmission or reception.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 channel used for transmission or reception.
     *
     */
    void SetChannel(uint8_t aChannel) { mChannel = aChannel; }

    /**
     * This method returns the IEEE 802.15.4 PSDU length.
     *
     * @returns The IEEE 802.15.4 PSDU length.
     *
     */
    uint16_t GetPsduLength(void) const { return mLength; }

    /**
     * This method sets the IEEE 802.15.4 PSDU length.
     *
     * @param[in]  aLength  The IEEE 802.15.4 PSDU length.
     *
     */
    void SetPsduLength(uint16_t aLength) { mLength = aLength; }

    /**
     * This method returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     *
     */
    uint8_t *GetPsdu(void) { return mPsdu; }

    /**
     * This const method returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     *
     */
    const uint8_t *GetPsdu(void) const { return mPsdu; }

    /**
     * This method returns a pointer to the MAC Header.
     *
     * @returns A pointer to the MAC Header.
     *
     */
    uint8_t *GetHeader(void) { return GetPsdu(); }

    /**
     * This const method returns a pointer to the MAC Header.
     *
     * @returns A pointer to the MAC Header.
     *
     */
    const uint8_t *GetHeader(void) const { return GetPsdu(); }

    /**
     * This method returns a pointer to the MAC Payload.
     *
     * @returns A pointer to the MAC Payload.
     *
     */
    uint8_t *GetPayload(void) { return const_cast<uint8_t *>(const_cast<const Frame *>(this)->GetPayload()); }

    /**
     * This const method returns a pointer to the MAC Payload.
     *
     * @returns A pointer to the MAC Payload.
     *
     */
    const uint8_t *GetPayload(void) const;

    /**
     * This method returns a pointer to the MAC Footer.
     *
     * @returns A pointer to the MAC Footer.
     *
     */
    uint8_t *GetFooter(void) { return const_cast<uint8_t *>(const_cast<const Frame *>(this)->GetFooter()); }

    /**
     * This const method returns a pointer to the MAC Footer.
     *
     * @returns A pointer to the MAC Footer.
     *
     */
    const uint8_t *GetFooter(void) const;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    /**
     * This method returns a pointer to the vendor specific Time IE.
     *
     * @returns A pointer to the Time IE, NULL if not found.
     *
     */
    TimeIe *GetTimeIe(void) { return const_cast<TimeIe *>(const_cast<const Frame *>(this)->GetTimeIe()); }

    /**
     * This method returns a pointer to the vendor specific Time IE.
     *
     * @returns A pointer to the Time IE, NULL if not found.
     *
     */
    const TimeIe *GetTimeIe(void) const;
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    /**
     * This method appends Header IEs to MAC header.
     *
     * @param[in]   aIeList  The pointer to the Header IEs array.
     * @param[in]   aIeCount The number of Header IEs in the array.
     *
     * @retval OT_ERROR_NONE    Successfully appended the Header IEs.
     * @retval OT_ERROR_FAILED  If IE Present bit is not set.
     *
     */
    otError AppendHeaderIe(HeaderIe *aIeList, uint8_t aIeCount);

    /**
     * This method returns a pointer to the Header IE.
     *
     * @param[in] aIeId  The Element Id of the Header IE.
     *
     * @returns A pointer to the Header IE, NULL if not found.
     *
     */
    uint8_t *GetHeaderIe(uint8_t aIeId)
    {
        return const_cast<uint8_t *>(const_cast<const Frame *>(this)->GetHeaderIe(aIeId));
    }

    /**
     * This method returns a pointer to the Header IE.
     *
     * @param[in] aIeId  The Element Id of the Header IE.
     *
     * @returns A pointer to the Header IE, NULL if not found.
     *
     */
    const uint8_t *GetHeaderIe(uint8_t aIeId) const;
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

    /**
     * This method returns the maximum transmission unit size (MTU).
     *
     * @returns The maximum transmission unit (MTU).
     *
     */
    uint16_t GetMtu(void) const { return kMtu; }

    /**
     * This method returns the FCS size.
     *
     * @returns This method returns the FCS size.
     *
     */
    uint16_t GetFcsSize(void) const { return kFcsSize; }

    /**
     * This method returns information about the frame object as an `InfoString` object.
     *
     * @returns An `InfoString` containing info about the frame.
     *
     */
    InfoString ToInfoString(void) const;

private:
    enum
    {
        kInvalidIndex  = 0xff,
        kSequenceIndex = kFcfSize,
    };

    uint16_t GetFrameControlField(void) const;
    uint8_t  FindDstPanIdIndex(void) const;
    uint8_t  FindDstAddrIndex(void) const;
    uint8_t  FindSrcPanIdIndex(void) const;
    uint8_t  FindSrcAddrIndex(void) const;
    uint8_t  FindSecurityHeaderIndex(void) const;
    uint8_t  SkipSecurityHeaderIndex(void) const;
    uint8_t  FindPayloadIndex(void) const;
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    uint8_t FindHeaderIeIndex(void) const;
#endif

    static uint8_t GetKeySourceLength(uint8_t aKeyIdMode);
};

/**
 * This class supports received IEEE 802.15.4 MAC frame processing.
 *
 */
class RxFrame : public Frame
{
public:
    /**
     * This method returns the RSSI in dBm used for reception.
     *
     * @returns The RSSI in dBm used for reception.
     *
     */
    int8_t GetRssi(void) const { return mInfo.mRxInfo.mRssi; }

    /**
     * This method sets the RSSI in dBm used for reception.
     *
     * @param[in]  aRssi  The RSSI in dBm used for reception.
     *
     */
    void SetRssi(int8_t aRssi) { mInfo.mRxInfo.mRssi = aRssi; }

    /**
     * This method returns the receive Link Quality Indicator.
     *
     * @returns The receive Link Quality Indicator.
     *
     */
    uint8_t GetLqi(void) const { return mInfo.mRxInfo.mLqi; }

    /**
     * This method sets the receive Link Quality Indicator.
     *
     * @param[in]  aLqi  The receive Link Quality Indicator.
     *
     */
    void SetLqi(uint8_t aLqi) { mInfo.mRxInfo.mLqi = aLqi; }

    /**
     * This method indicates whether or not the received frame is acknowledged with frame pending set.
     *
     * @retval TRUE   This frame is acknowledged with frame pending set.
     * @retval FALSE  This frame is acknowledged with frame pending not set.
     *
     */
    bool IsAckedWithFramePending(void) const { return mInfo.mRxInfo.mAckedWithFramePending; }

    /**
     * This method returns the timestamp when the frame was received.
     *
     * @returns The timestamp when the frame was received, in microseconds.
     *
     */
    const uint64_t &GetTimestamp(void) const { return mInfo.mRxInfo.mTimestamp; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * This method gets the offset to network time.
     *
     * @returns  The offset to network time.
     *
     */
    int64_t ComputeNetworkTimeOffset(void) const
    {
        return static_cast<int64_t>(GetTimeIe()->GetTime() - GetTimestamp());
    }

    /**
     * This method gets the time sync sequence.
     *
     * @returns  The time sync sequence.
     *
     */
    uint8_t ReadTimeSyncSeq(void) const { return GetTimeIe()->GetSequence(); }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
};

/**
 * This class supports IEEE 802.15.4 MAC frame generation for transmission.
 *
 */
class TxFrame : public Frame
{
public:
    /**
     * This method returns the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access
     *          failure.
     *
     */
    uint8_t GetMaxCsmaBackoffs(void) const { return mInfo.mTxInfo.mMaxCsmaBackoffs; }

    /**
     * This method sets the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxCsmaBackoffs  The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring
     *                               a channel access failure.
     *
     */
    void SetMaxCsmaBackoffs(uint8_t aMaxCsmaBackoffs) { mInfo.mTxInfo.mMaxCsmaBackoffs = aMaxCsmaBackoffs; }

    /**
     * This method returns the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of retries allowed after a transmission failure.
     *
     */
    uint8_t GetMaxFrameRetries(void) const { return mInfo.mTxInfo.mMaxFrameRetries; }

    /**
     * This method sets the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxFrameRetries  The maximum number of retries allowed after a transmission failure.
     *
     */
    void SetMaxFrameRetries(uint8_t aMaxFrameRetries) { mInfo.mTxInfo.mMaxFrameRetries = aMaxFrameRetries; }

    /**
     * This method indicates whether or not the frame is a retransmission.
     *
     * @retval TRUE   Frame is a retransmission
     * @retval FALSE  This is a new frame and not a retransmission of an earlier frame.
     *
     */
    bool IsARetransmission(void) const { return mInfo.mTxInfo.mIsARetx; }

    /**
     * This method sets the retransmission flag attribute.
     *
     * @param[in]  aIsARetx  TRUE if frame is a retransmission of an earlier frame, FALSE otherwise.
     *
     */
    void SetIsARetransmission(bool aIsARetx) { mInfo.mTxInfo.mIsARetx = aIsARetx; }

    /**
     * This method indicates whether or not CSMA-CA is enabled.
     *
     * @retval TRUE  CSMA-CA is enabled.
     * @retval FALSE CSMA-CA is not enabled is not enabled.
     *
     */
    bool IsCsmaCaEnabled(void) const { return mInfo.mTxInfo.mCsmaCaEnabled; }

    /**
     * This method sets the CSMA-CA enabled attribute.
     *
     * @param[in]  aCsmaCaEnabled  TRUE if CSMA-CA must be enabled for this packet, FALSE otherwise.
     *
     */
    void SetCsmaCaEnabled(bool aCsmaCaEnabled) { mInfo.mTxInfo.mCsmaCaEnabled = aCsmaCaEnabled; }

    /**
     * This method returns the key used for frame encryption and authentication (AES CCM).
     *
     * @returns The pointer to the key.
     *
     */
    const uint8_t *GetAesKey(void) const { return mInfo.mTxInfo.mAesKey; }

    /**
     * This method sets the key used for frame encryption and authentication (AES CCM).
     *
     * @param[in]  aAesKey  The pointer to the key.
     *
     */
    void SetAesKey(const uint8_t *aAesKey) { mInfo.mTxInfo.mAesKey = aAesKey; }

    /**
     * This method copies the PSDU and all attributes from another frame.
     *
     * @note This method performs a deep copy meaning the content of PSDU buffer from the given frame is copied into
     * the PSDU buffer of the current frame.

     * @param[in] aFromFrame  The frame to copy from.
     *
     */
    void CopyFrom(const TxFrame &aFromFrame);

    /**
     * This method performs AES CCM on the frame which is going to be sent.
     *
     * @param[in]  aExtAddress  A reference to the extended address, which will be used to generate nonce
     *                          for AES CCM computation.
     *
     */
    void ProcessTransmitAesCcm(const ExtAddress &aExtAddress);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * This method sets the Time IE offset.
     *
     * @param[in]  aOffset  The Time IE offset, 0 means no Time IE.
     *
     */
    void SetTimeIeOffset(uint8_t aOffset) { mInfo.mTxInfo.mIeInfo->mTimeIeOffset = aOffset; }

    /**
     * This method sets the offset to network time.
     *
     * @param[in]  aNetworkTimeOffset  The offset to network time.
     *
     */
    void SetNetworkTimeOffset(int64_t aNetworkTimeOffset)
    {
        mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset = aNetworkTimeOffset;
    }

    /**
     * This method sets the time sync sequence.
     *
     * @param[in]  aTimeSyncSeq  The time sync sequence.
     *
     */
    void SetTimeSyncSeq(uint8_t aTimeSyncSeq) { mInfo.mTxInfo.mIeInfo->mTimeSyncSeq = aTimeSyncSeq; }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
};

OT_TOOL_PACKED_BEGIN
class Beacon
{
public:
    enum
    {
        kSuperFrameSpec = 0x0fff, ///< Superframe Specification value.
    };

    /**
     * This method initializes the Beacon message.
     *
     */
    void Init(void)
    {
        mSuperframeSpec     = ot::Encoding::LittleEndian::HostSwap16(kSuperFrameSpec);
        mGtsSpec            = 0;
        mPendingAddressSpec = 0;
    }

    /**
     * This method indicates whether or not the beacon appears to be a valid Thread Beacon message.
     *
     * @retval TRUE  if the beacon appears to be a valid Thread Beacon message.
     * @retval FALSE if the beacon does not appear to be a valid Thread Beacon message.
     *
     */
    bool IsValid(void) const
    {
        return (mSuperframeSpec == ot::Encoding::LittleEndian::HostSwap16(kSuperFrameSpec)) && (mGtsSpec == 0) &&
               (mPendingAddressSpec == 0);
    }

    /**
     * This method returns the pointer to the beacon payload.
     *
     * @returns A pointer to the beacon payload.
     *
     */
    uint8_t *GetPayload(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    /**
     * This method returns the pointer to the beacon payload.
     *
     * @returns A pointer to the beacon payload.
     *
     */
    const uint8_t *GetPayload(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

private:
    uint16_t mSuperframeSpec;
    uint8_t  mGtsSpec;
    uint8_t  mPendingAddressSpec;
} OT_TOOL_PACKED_END;

/**
 * This class implements IEEE 802.15.4 Beacon Payload generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BeaconPayload
{
public:
    enum
    {
        kProtocolId     = 3,  ///< Thread Protocol ID.
        kInfoStringSize = 92, ///< Max chars for the info string (@sa ToInfoString()).
    };

    enum
    {
        kProtocolVersion = 2,                     ///< Thread Protocol version.
        kVersionOffset   = 4,                     ///< Version field bit offset.
        kVersionMask     = 0xf << kVersionOffset, ///< Version field mask.
        kNativeFlag      = 1 << 3,                ///< Native Commissioner flag.
        kJoiningFlag     = 1 << 0,                ///< Joining Permitted flag.
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToInfoString()` method.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This method initializes the Beacon Payload.
     *
     */
    void Init(void)
    {
        mProtocolId = kProtocolId;
        mFlags      = kProtocolVersion << kVersionOffset;
    }

    /**
     * This method indicates whether or not the beacon appears to be a valid Thread Beacon Payload.
     *
     * @retval TRUE  if the beacon appears to be a valid Thread Beacon Payload.
     * @retval FALSE if the beacon does not appear to be a valid Thread Beacon Payload.
     *
     */
    bool IsValid(void) const { return (mProtocolId == kProtocolId); }

    /**
     * This method returns the Protocol ID value.
     *
     * @returns the Protocol ID value.
     *
     */
    uint8_t GetProtocolId(void) const { return mProtocolId; }

    /**
     * This method returns the Protocol Version value.
     *
     * @returns The Protocol Version value.
     *
     */
    uint8_t GetProtocolVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   if the Native Commissioner flag is set.
     * @retval FALSE  if the Native Commissioner flag is not set.
     *
     */
    bool IsNative(void) const { return (mFlags & kNativeFlag) != 0; }

    /**
     * This method clears the Native Commissioner flag.
     *
     */
    void ClearNative(void) { mFlags &= ~kNativeFlag; }

    /**
     * This method sets the Native Commissioner flag.
     *
     */
    void SetNative(void) { mFlags |= kNativeFlag; }

    /**
     * This method indicates whether or not the Joining Permitted flag is set.
     *
     * @retval TRUE   if the Joining Permitted flag is set.
     * @retval FALSE  if the Joining Permitted flag is not set.
     *
     */
    bool IsJoiningPermitted(void) const { return (mFlags & kJoiningFlag) != 0; }

    /**
     * This method clears the Joining Permitted flag.
     *
     */
    void ClearJoiningPermitted(void) { mFlags &= ~kJoiningFlag; }

    /**
     * This method sets the Joining Permitted flag.
     *
     */
    void SetJoiningPermitted(void)
    {
        mFlags |= kJoiningFlag;

#if OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION != 2 // check against kProtocolVersion
        mFlags &= ~kVersionMask;
        mFlags |= OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION << kVersionOffset;
#endif
    }

    /**
     * This method gets the Network Name field.
     *
     * @returns The Network Name field as `NetworkName::Data`.
     *
     */
    NetworkName::Data GetNetworkName(void) const { return NetworkName::Data(mNetworkName, sizeof(mNetworkName)); }

    /**
     * This method sets the Network Name field.
     *
     * @param[in]  aNameData  The Network Name (as a `NetworkName::Data`).
     *
     */
    void SetNetworkName(const NetworkName::Data &aNameData) { aNameData.CopyTo(mNetworkName, sizeof(mNetworkName)); }

    /**
     * This method returns the Extended PAN ID field.
     *
     * @returns The Extended PAN ID field.
     *
     */
    const ExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the Extended PAN ID field.
     *
     * @param[in]  aExtPanId  An Extended PAN ID.
     *
     */
    void SetExtendedPanId(const ExtendedPanId &aExtPanId) { mExtendedPanId = aExtPanId; }

    /**
     * This method returns information about the Beacon as a `InfoString`.
     *
     * @returns An `InfoString` representing the beacon payload.
     *
     */
    InfoString ToInfoString(void) const;

private:
    uint8_t       mProtocolId;
    uint8_t       mFlags;
    char          mNetworkName[NetworkName::kMaxSize];
    ExtendedPanId mExtendedPanId;
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // MAC_FRAME_HPP_
