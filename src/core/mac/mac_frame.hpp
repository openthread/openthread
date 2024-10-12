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

#include "common/as_core_type.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/numeric_limits.hpp"
#include "mac/mac_header_ie.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/network_name.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

/**
 * Implements IEEE 802.15.4 MAC frame generation and parsing.
 */
class Frame : public otRadioFrame
{
public:
    /**
     * Represents the MAC frame type.
     *
     * Values match the Frame Type field in Frame Control Field (FCF)  as an `uint16_t`.
     */
    enum Type : uint16_t
    {
        kTypeBeacon       = 0, ///< Beacon Frame Type.
        kTypeData         = 1, ///< Data Frame Type.
        kTypeAck          = 2, ///< Ack Frame Type.
        kTypeMacCmd       = 3, ///< MAC Command Frame Type.
        kTypeMultipurpose = 5, ///< Multipurpose Frame Type.
    };

    /**
     * Represents the MAC frame version.
     *
     * Values match the Version field in Frame Control Field (FCF) as an `uint16_t`.
     */
    enum Version : uint16_t
    {
        kVersion2003 = 0 << 12, ///< 2003 Frame Version.
        kVersion2006 = 1 << 12, ///< 2006 Frame Version.
        kVersion2015 = 2 << 12, ///< 2015 Frame Version.
    };

    /**
     * Represents the MAC frame security level.
     *
     * Values match the Security Level field in Security Control Field as an `uint8_t`.
     */
    enum SecurityLevel : uint8_t
    {
        kSecurityNone      = 0, ///< No security.
        kSecurityMic32     = 1, ///< No encryption, MIC-32 authentication.
        kSecurityMic64     = 2, ///< No encryption, MIC-64 authentication.
        kSecurityMic128    = 3, ///< No encryption, MIC-128 authentication.
        kSecurityEnc       = 4, ///< Encryption, no authentication
        kSecurityEncMic32  = 5, ///< Encryption with MIC-32 authentication.
        kSecurityEncMic64  = 6, ///< Encryption with MIC-64 authentication.
        kSecurityEncMic128 = 7, ///< Encryption with MIC-128 authentication.
    };

    /**
     * Represents the MAC frame security key identifier mode.
     *
     * Values match the Key Identifier Mode field in Security Control Field as an `uint8_t`.
     */
    enum KeyIdMode : uint8_t
    {
        kKeyIdMode0 = 0 << 3, ///< Key ID Mode 0 - Key is determined implicitly.
        kKeyIdMode1 = 1 << 3, ///< Key ID Mode 1 - Key is determined from Key Index field.
        kKeyIdMode2 = 2 << 3, ///< Key ID Mode 2 - Key is determined from 4-bytes Key Source and Index fields.
        kKeyIdMode3 = 3 << 3, ///< Key ID Mode 3 - Key is determined from 8-bytes Key Source and Index fields.
    };

    /**
     * Represents a subset of MAC Command Identifiers.
     */
    enum CommandId : uint8_t
    {
        kMacCmdAssociationRequest         = 1,
        kMacCmdAssociationResponse        = 2,
        kMacCmdDisassociationNotification = 3,
        kMacCmdDataRequest                = 4,
        kMacCmdPanidConflictNotification  = 5,
        kMacCmdOrphanNotification         = 6,
        kMacCmdBeaconRequest              = 7,
        kMacCmdCoordinatorRealignment     = 8,
        kMacCmdGtsRequest                 = 9,
    };

    static constexpr uint16_t kInfoStringSize = 128; ///< Max chars for `InfoString` (ToInfoString()).

    static constexpr uint8_t kPreambleSize  = 4;
    static constexpr uint8_t kSfdSize       = 1;
    static constexpr uint8_t kPhrSize       = 1;
    static constexpr uint8_t kPhyHeaderSize = kPreambleSize + kSfdSize + kPhrSize;
    static constexpr uint8_t kFcfSize       = sizeof(uint16_t);
    static constexpr uint8_t kDsnSize       = sizeof(uint8_t);
    static constexpr uint8_t k154FcsSize    = sizeof(uint16_t);
    static constexpr uint8_t kImmAckLength  = kFcfSize + kDsnSize + k154FcsSize;

    /**
     * Defines the fixed-length `String` object returned from `ToInfoString()` method.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Indicates whether the frame is empty (no payload).
     *
     * @retval TRUE   The frame is empty (no PSDU payload).
     * @retval FALSE  The frame is not empty.
     */
    bool IsEmpty(void) const { return (mLength == 0); }

    /**
     * Validates the frame.
     *
     * @retval kErrorNone    Successfully parsed the MAC header.
     * @retval kErrorParse   Failed to parse through the MAC header.
     */
    Error ValidatePsdu(void) const;

    /**
     * Returns the IEEE 802.15.4 Frame Type.
     *
     * @returns The IEEE 802.15.4 Frame Type.
     */
    uint8_t GetType(void) const { return GetPsdu()[0] & kFcfFrameTypeMask; }

    /**
     * Returns whether the frame is an Ack frame.
     *
     * @retval TRUE   If this is an Ack.
     * @retval FALSE  If this is not an Ack.
     */
    bool IsAck(void) const { return GetType() == kTypeAck; }

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * This method returns whether the frame is an IEEE 802.15.4 Wake-up frame.
     *
     * @retval TRUE   If this is a Wake-up frame.
     * @retval FALSE  If this is not a Wake-up frame.
     */
    bool IsWakeupFrame(void) const;

    /**
     * This method returns the Rendezvous Time IE of a wake-up frame.
     *
     * @returns Pointer to the Rendezvous Time IE.
     */
    RendezvousTimeIe *GetRendezvousTimeIe(void) { return AsNonConst(AsConst(this)->GetRendezvousTimeIe()); }

    /**
     * This method returns the Rendezvous Time IE of a wake-up frame.
     *
     * @returns Const pointer to the Rendezvous Time IE.
     */
    const RendezvousTimeIe *GetRendezvousTimeIe(void) const
    {
        const uint8_t *ie = GetHeaderIe(RendezvousTimeIe::kHeaderIeId);

        return (ie != nullptr) ? reinterpret_cast<const RendezvousTimeIe *>(ie + sizeof(HeaderIe)) : nullptr;
    }

    /**
     * This method returns the Connection IE of a wake-up frame.
     *
     * @returns Pointer to the Connection IE.
     */
    ConnectionIe *GetConnectionIe(void) { return AsNonConst(AsConst(this)->GetConnectionIe()); }

    /**
     * This method returns the Connection IE of a wake-up frame.
     *
     * @returns Const pointer to the Connection IE.
     */
    const ConnectionIe *GetConnectionIe(void) const
    {
        const uint8_t *ie = GetThreadIe(ConnectionIe::kThreadIeSubtype);

        return (ie != nullptr) ? reinterpret_cast<const ConnectionIe *>(ie + sizeof(HeaderIe)) : nullptr;
    }
#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

    /**
     * Returns the IEEE 802.15.4 Frame Version.
     *
     * @returns The IEEE 802.15.4 Frame Version.
     */
    uint16_t GetVersion(void) const { return GetFrameControlField() & kFcfFrameVersionMask; }

    /**
     * Returns if this IEEE 802.15.4 frame's version is 2015.
     *
     * @returns TRUE if version is 2015, FALSE otherwise.
     */
    bool IsVersion2015(void) const { return IsVersion2015(GetFrameControlField()); }

    /**
     * Indicates whether or not security is enabled.
     *
     * @retval TRUE   If security is enabled.
     * @retval FALSE  If security is not enabled.
     */
    bool GetSecurityEnabled(void) const { return IsSecurityEnabled(GetFrameControlField()); }

    /**
     * Indicates whether or not the Frame Pending bit is set.
     *
     * @retval TRUE   If the Frame Pending bit is set.
     * @retval FALSE  If the Frame Pending bit is not set.
     */
    bool GetFramePending(void) const { return IsFramePending(GetFrameControlField()); }

    /**
     * Sets the Frame Pending bit.
     *
     * @note This method must not be called on a Multipurpose frame with short Frame Control field.
     *
     * @param[in]  aFramePending  The Frame Pending bit.
     */
    void SetFramePending(bool aFramePending);

    /**
     * Indicates whether or not the Ack Request bit is set.
     *
     * @retval TRUE   If the Ack Request bit is set.
     * @retval FALSE  If the Ack Request bit is not set.
     */
    bool GetAckRequest(void) const { return IsAckRequest(GetFrameControlField()); }

    /**
     * Sets the Ack Request bit.
     *
     * @note This method must not be called on a Multipurpose frame with short Frame Control field.
     *
     * @param[in]  aAckRequest  The Ack Request bit.
     */
    void SetAckRequest(bool aAckRequest);

    /**
     * Indicates whether or not the PanId Compression bit is set.
     *
     * @note This method must not be called on a Multipurpose frame, which lacks this flag.
     *
     * @retval TRUE   If the PanId Compression bit is set.
     * @retval FALSE  If the PanId Compression bit is not set.
     */
    bool IsPanIdCompressed(void) const { return (GetFrameControlField() & kFcfPanidCompression) != 0; }

    /**
     * Indicates whether or not IEs present.
     *
     * @retval TRUE   If IEs present.
     * @retval FALSE  If no IE present.
     */
    bool IsIePresent(void) const { return IsIePresent(GetFrameControlField()); }

    /**
     * Sets the IE Present bit.
     *
     * @note This method must not be called on a Multipurpose frame with short Frame Control field.
     *
     * @param[in]  aIePresent   The IE Present bit.
     */
    void SetIePresent(bool aIePresent);

    /**
     * Returns the Sequence Number value.
     *
     * @returns The Sequence Number value.
     */
    uint8_t GetSequence(void) const;

    /**
     * Sets the Sequence Number value.
     *
     * @param[in]  aSequence  The Sequence Number value.
     */
    void SetSequence(uint8_t aSequence);

    /**
     * Indicates whether or not the Sequence Number is present.
     *
     * @returns TRUE if the Sequence Number is present, FALSE otherwise.
     */
    uint8_t IsSequencePresent(void) const { return IsSequencePresent(GetFrameControlField()); }

    /**
     * Indicates whether or not the Destination PAN ID is present.
     *
     * @returns TRUE if the Destination PAN ID is present, FALSE otherwise.
     */
    bool IsDstPanIdPresent(void) const { return IsDstPanIdPresent(GetFrameControlField()); }

    /**
     * Gets the Destination PAN Identifier.
     *
     * @param[out]  aPanId  The Destination PAN Identifier.
     *
     * @retval kErrorNone   Successfully retrieved the Destination PAN Identifier.
     * @retval kErrorParse  Failed to parse the PAN Identifier.
     */
    Error GetDstPanId(PanId &aPanId) const;

    /**
     * Indicates whether or not the Destination Address is present for this object.
     *
     * @retval TRUE   If the Destination Address is present.
     * @retval FALSE  If the Destination Address is not present.
     */
    bool IsDstAddrPresent() const { return IsDstAddrPresent(GetFrameControlField()); }

    /**
     * Gets the Destination Address.
     *
     * @param[out]  aAddress  The Destination Address.
     *
     * @retval kErrorNone  Successfully retrieved the Destination Address.
     */
    Error GetDstAddr(Address &aAddress) const;

    /**
     * Indicates whether or not the Source Address is present for this object.
     *
     * @retval TRUE   If the Source Address is present.
     * @retval FALSE  If the Source Address is not present.
     */
    bool IsSrcPanIdPresent(void) const { return IsSrcPanIdPresent(GetFrameControlField()); }

    /**
     * Gets the Source PAN Identifier.
     *
     * @param[out]  aPanId  The Source PAN Identifier.
     *
     * @retval kErrorNone   Successfully retrieved the Source PAN Identifier.
     */
    Error GetSrcPanId(PanId &aPanId) const;

    /**
     * Indicates whether or not the Source Address is present for this object.
     *
     * @retval TRUE   If the Source Address is present.
     * @retval FALSE  If the Source Address is not present.
     */
    bool IsSrcAddrPresent(void) const { return IsSrcAddrPresent(GetFrameControlField()); }

    /**
     * Gets the Source Address.
     *
     * @param[out]  aAddress  The Source Address.
     *
     * @retval kErrorNone  Successfully retrieved the Source Address.
     */
    Error GetSrcAddr(Address &aAddress) const;

    /**
     * Gets the Security Control Field.
     *
     * @param[out]  aSecurityControlField  The Security Control Field.
     *
     * @retval kErrorNone   Successfully retrieved the Security Level Identifier.
     * @retval kErrorParse  Failed to find the security control field in the frame.
     */
    Error GetSecurityControlField(uint8_t &aSecurityControlField) const;

    /**
     * Gets the Security Level Identifier.
     *
     * @param[out]  aSecurityLevel  The Security Level Identifier.
     *
     * @retval kErrorNone  Successfully retrieved the Security Level Identifier.
     */
    Error GetSecurityLevel(uint8_t &aSecurityLevel) const;

    /**
     * Gets the Key Identifier Mode.
     *
     * @param[out]  aKeyIdMode  The Key Identifier Mode.
     *
     * @retval kErrorNone  Successfully retrieved the Key Identifier Mode.
     */
    Error GetKeyIdMode(uint8_t &aKeyIdMode) const;

    /**
     * Gets the Frame Counter.
     *
     * @param[out]  aFrameCounter  The Frame Counter.
     *
     * @retval kErrorNone  Successfully retrieved the Frame Counter.
     */
    Error GetFrameCounter(uint32_t &aFrameCounter) const;

    /**
     * Sets the Frame Counter.
     *
     * @param[in]  aFrameCounter  The Frame Counter.
     */
    void SetFrameCounter(uint32_t aFrameCounter);

    /**
     * Returns a pointer to the Key Source.
     *
     * @returns A pointer to the Key Source.
     */
    const uint8_t *GetKeySource(void) const;

    /**
     * Sets the Key Source.
     *
     * @param[in]  aKeySource  A pointer to the Key Source value.
     */
    void SetKeySource(const uint8_t *aKeySource);

    /**
     * Gets the Key Identifier.
     *
     * @param[out]  aKeyId  The Key Identifier.
     *
     * @retval kErrorNone  Successfully retrieved the Key Identifier.
     */
    Error GetKeyId(uint8_t &aKeyId) const;

    /**
     * Sets the Key Identifier.
     *
     * @param[in]  aKeyId  The Key Identifier.
     */
    void SetKeyId(uint8_t aKeyId);

    /**
     * Gets the Command ID.
     *
     * @param[out]  aCommandId  The Command ID.
     *
     * @retval kErrorNone  Successfully retrieved the Command ID.
     */
    Error GetCommandId(uint8_t &aCommandId) const;

    /**
     * Indicates whether the frame is a MAC Data Request command (data poll).
     *
     * For 802.15.4-2015 and above frame, the frame should be already decrypted.
     *
     * @returns TRUE if frame is a MAC Data Request command, FALSE otherwise.
     */
    bool IsDataRequestCommand(void) const;

    /**
     * Returns the MAC Frame Length, namely the IEEE 802.15.4 PSDU length.
     *
     * @returns The MAC Frame Length.
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Sets the MAC Frame Length.
     *
     * @param[in]  aLength  The MAC Frame Length.
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

    /**
     * Returns the MAC header size.
     *
     * @returns The MAC header size.
     */
    uint8_t GetHeaderLength(void) const;

    /**
     * Returns the MAC footer size.
     *
     * @returns The MAC footer size.
     */
    uint8_t GetFooterLength(void) const;

    /**
     * Returns the current MAC Payload length.
     *
     * @returns The current MAC Payload length.
     */
    uint16_t GetPayloadLength(void) const;

    /**
     * Returns the maximum MAC Payload length for the given MAC header and footer.
     *
     * @returns The maximum MAC Payload length for the given MAC header and footer.
     */
    uint16_t GetMaxPayloadLength(void) const;

    /**
     * Sets the MAC Payload length.
     */
    void SetPayloadLength(uint16_t aLength);

    /**
     * Returns the IEEE 802.15.4 channel used for transmission or reception.
     *
     * @returns The IEEE 802.15.4 channel used for transmission or reception.
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * Returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     */
    uint8_t *GetPsdu(void) { return mPsdu; }

    /**
     * Returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     */
    const uint8_t *GetPsdu(void) const { return mPsdu; }

    /**
     * Returns a pointer to the MAC Header.
     *
     * @returns A pointer to the MAC Header.
     */
    uint8_t *GetHeader(void) { return GetPsdu(); }

    /**
     * Returns a pointer to the MAC Header.
     *
     * @returns A pointer to the MAC Header.
     */
    const uint8_t *GetHeader(void) const { return GetPsdu(); }

    /**
     * Returns a pointer to the MAC Payload.
     *
     * @returns A pointer to the MAC Payload.
     */
    uint8_t *GetPayload(void) { return AsNonConst(AsConst(this)->GetPayload()); }

    /**
     * Returns a pointer to the MAC Payload.
     *
     * @returns A pointer to the MAC Payload.
     */
    const uint8_t *GetPayload(void) const;

    /**
     * Returns a pointer to the MAC Footer.
     *
     * @returns A pointer to the MAC Footer.
     */
    uint8_t *GetFooter(void) { return AsNonConst(AsConst(this)->GetFooter()); }

    /**
     * Returns a pointer to the MAC Footer.
     *
     * @returns A pointer to the MAC Footer.
     */
    const uint8_t *GetFooter(void) const;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    /**
     * Returns a pointer to the vendor specific Time IE.
     *
     * @returns A pointer to the Time IE, `nullptr` if not found.
     */
    TimeIe *GetTimeIe(void) { return AsNonConst(AsConst(this)->GetTimeIe()); }

    /**
     * Returns a pointer to the vendor specific Time IE.
     *
     * @returns A pointer to the Time IE, `nullptr` if not found.
     */
    const TimeIe *GetTimeIe(void) const;
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    /**
     * Returns a pointer to the Header IE.
     *
     * @param[in] aIeId  The Element Id of the Header IE.
     *
     * @returns A pointer to the Header IE, `nullptr` if not found.
     */
    uint8_t *GetHeaderIe(uint8_t aIeId) { return AsNonConst(AsConst(this)->GetHeaderIe(aIeId)); }

    /**
     * Returns a pointer to the Header IE.
     *
     * @param[in] aIeId  The Element Id of the Header IE.
     *
     * @returns A pointer to the Header IE, `nullptr` if not found.
     */
    const uint8_t *GetHeaderIe(uint8_t aIeId) const;

    /**
     * Returns a pointer to a specific Thread IE.
     *
     * A Thread IE is a vendor specific IE with Vendor OUI as `kVendorOuiThreadCompanyId`.
     *
     * @param[in] aSubType  The sub type of the Thread IE.
     *
     * @returns A pointer to the Thread IE, `nullptr` if not found.
     */
    uint8_t *GetThreadIe(uint8_t aSubType) { return AsNonConst(AsConst(this)->GetThreadIe(aSubType)); }

    /**
     * Returns a pointer to a specific Thread IE.
     *
     * A Thread IE is a vendor specific IE with Vendor OUI as `kVendorOuiThreadCompanyId`.
     *
     * @param[in] aSubType  The sub type of the Thread IE.
     *
     * @returns A pointer to the Thread IE, `nullptr` if not found.
     */
    const uint8_t *GetThreadIe(uint8_t aSubType) const;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Finds CSL IE in the frame and modify its content.
     *
     * @param[in] aCslPeriod  CSL Period in CSL IE.
     * @param[in] aCslPhase   CSL Phase in CSL IE.
     */
    void SetCslIe(uint16_t aCslPeriod, uint16_t aCslPhase);

    /**
     * Indicates whether or not the frame contains CSL IE.
     *
     * @retval TRUE   If the frame contains CSL IE.
     * @retval FALSE  If the frame doesn't contain CSL IE.
     */
    bool HasCslIe(void) const;
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE)
    /**
     * Returns a pointer to a CSL IE.
     *
     * @returns A pointer to the CSL IE, `nullptr` if not found.
     */
    const CslIe *GetCslIe(void) const;

    /**
     * Returns a pointer to a CSL IE.
     *
     * @returns A pointer to the CSL IE, `nullptr` if not found.
     */
    CslIe *GetCslIe(void) { return AsNonConst(AsConst(this)->GetCslIe()); }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE)

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    /**
     * Finds Enhanced ACK Probing (Vendor Specific) IE and set its value.
     *
     * @param[in] aValue  A pointer to the value to set.
     * @param[in] aLen    The length of @p aValue.
     */
    void SetEnhAckProbingIe(const uint8_t *aValue, uint8_t aLen);
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_MULTI_RADIO
    /**
     * Gets the radio link type of the frame.
     *
     * @returns Frame's radio link type.
     */
    RadioType GetRadioType(void) const { return static_cast<RadioType>(mRadioType); }

    /**
     * Sets the radio link type of the frame.
     *
     * @param[in] aRadioType  A radio link type.
     */
    void SetRadioType(RadioType aRadioType) { mRadioType = static_cast<uint8_t>(aRadioType); }
#endif

    /**
     * Returns the maximum transmission unit size (MTU).
     *
     * @returns The maximum transmission unit (MTU).
     */
    uint16_t GetMtu(void) const
#if !OPENTHREAD_CONFIG_MULTI_RADIO && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        return OT_RADIO_FRAME_MAX_SIZE;
    }
#else
        ;
#endif

    /**
     * Returns the FCS size.
     *
     * @returns This method returns the FCS size.
     */
    uint8_t GetFcsSize(void) const
#if !OPENTHREAD_CONFIG_MULTI_RADIO && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        return k154FcsSize;
    }
#else
        ;
#endif

    /**
     * Returns information about the frame object as an `InfoString` object.
     *
     * @returns An `InfoString` containing info about the frame.
     */
    InfoString ToInfoString(void) const;

    /**
     * Returns the Frame Control field of the frame.
     *
     * @returns The Frame Control field.
     */
    uint16_t GetFrameControlField(void) const
    {
        uint16_t fcf = mPsdu[0];

#if OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME
        if (!IsShortFcf(fcf))
#endif
        {
            fcf |= (mPsdu[1] << 8);
        }

        return fcf;
    }

protected:
    static constexpr uint8_t kShortFcfSize        = sizeof(uint8_t);
    static constexpr uint8_t kSecurityControlSize = sizeof(uint8_t);
    static constexpr uint8_t kFrameCounterSize    = sizeof(uint32_t);
    static constexpr uint8_t kCommandIdSize       = sizeof(uint8_t);
    static constexpr uint8_t kKeyIndexSize        = sizeof(uint8_t);

    static constexpr uint16_t kFcfFrameTypeMask = 7 << 0;

    static constexpr uint16_t kFcfAddrNone  = 0;
    static constexpr uint16_t kFcfAddrShort = 2;
    static constexpr uint16_t kFcfAddrExt   = 3;
    static constexpr uint16_t kFcfAddrMask  = 3;

    // Frame Control field format for general MAC frame
    static constexpr uint16_t kFcfSecurityEnabled     = 1 << 3;
    static constexpr uint16_t kFcfFramePending        = 1 << 4;
    static constexpr uint16_t kFcfAckRequest          = 1 << 5;
    static constexpr uint16_t kFcfPanidCompression    = 1 << 6;
    static constexpr uint16_t kFcfSequenceSuppression = 1 << 8;
    static constexpr uint16_t kFcfIePresent           = 1 << 9;
    static constexpr uint16_t kFcfDstAddrShift        = 10;
    static constexpr uint16_t kFcfDstAddrNone         = kFcfAddrNone << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrShort        = kFcfAddrShort << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrExt          = kFcfAddrExt << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrMask         = kFcfAddrMask << kFcfDstAddrShift;
    static constexpr uint16_t kFcfFrameVersionMask    = 3 << 12;
    static constexpr uint16_t kFcfSrcAddrShift        = 14;
    static constexpr uint16_t kFcfSrcAddrNone         = kFcfAddrNone << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrShort        = kFcfAddrShort << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrExt          = kFcfAddrExt << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrMask         = kFcfAddrMask << kFcfSrcAddrShift;

    // Frame Control field format for MAC Multipurpose frame
    static constexpr uint16_t kMpFcfLongFrame           = 1 << 3;
    static constexpr uint16_t kMpFcfDstAddrShift        = 4;
    static constexpr uint16_t kMpFcfDstAddrNone         = kFcfAddrNone << kMpFcfDstAddrShift;
    static constexpr uint16_t kMpFcfDstAddrShort        = kFcfAddrShort << kMpFcfDstAddrShift;
    static constexpr uint16_t kMpFcfDstAddrExt          = kFcfAddrExt << kMpFcfDstAddrShift;
    static constexpr uint16_t kMpFcfDstAddrMask         = kFcfAddrMask << kMpFcfDstAddrShift;
    static constexpr uint16_t kMpFcfSrcAddrShift        = 6;
    static constexpr uint16_t kMpFcfSrcAddrNone         = kFcfAddrNone << kMpFcfSrcAddrShift;
    static constexpr uint16_t kMpFcfSrcAddrShort        = kFcfAddrShort << kMpFcfSrcAddrShift;
    static constexpr uint16_t kMpFcfSrcAddrExt          = kFcfAddrExt << kMpFcfSrcAddrShift;
    static constexpr uint16_t kMpFcfSrcAddrMask         = kFcfAddrMask << kMpFcfSrcAddrShift;
    static constexpr uint16_t kMpFcfPanidPresent        = 1 << 8;
    static constexpr uint16_t kMpFcfSecurityEnabled     = 1 << 9;
    static constexpr uint16_t kMpFcfSequenceSuppression = 1 << 10;
    static constexpr uint16_t kMpFcfFramePending        = 1 << 11;
    static constexpr uint16_t kMpFcfAckRequest          = 1 << 14;
    static constexpr uint16_t kMpFcfIePresent           = 1 << 15;

    static constexpr uint8_t kSecLevelMask  = 7 << 0;
    static constexpr uint8_t kKeyIdModeMask = 3 << 3;

    static constexpr uint8_t kMic0Size   = 0;
    static constexpr uint8_t kMic32Size  = 32 / kBitsPerByte;
    static constexpr uint8_t kMic64Size  = 64 / kBitsPerByte;
    static constexpr uint8_t kMic128Size = 128 / kBitsPerByte;
    static constexpr uint8_t kMaxMicSize = kMic128Size;

    static constexpr uint8_t kKeySourceSizeMode0 = 0;
    static constexpr uint8_t kKeySourceSizeMode1 = 0;
    static constexpr uint8_t kKeySourceSizeMode2 = 4;
    static constexpr uint8_t kKeySourceSizeMode3 = 8;

    static constexpr uint8_t kInvalidIndex = 0xff;
    static constexpr uint8_t kInvalidSize  = kInvalidIndex;
    static constexpr uint8_t kMaxPsduSize  = kInvalidSize - 1;

    void    SetFrameControlField(uint16_t aFcf);
    uint8_t SkipSequenceIndex(void) const;
    uint8_t FindDstPanIdIndex(void) const;
    uint8_t FindDstAddrIndex(void) const;
    uint8_t FindSrcPanIdIndex(void) const;
    uint8_t FindSrcAddrIndex(void) const;
    uint8_t SkipAddrFieldIndex(void) const;
    uint8_t FindSecurityHeaderIndex(void) const;
    uint8_t SkipSecurityHeaderIndex(void) const;
    uint8_t FindPayloadIndex(void) const;
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    uint8_t FindHeaderIeIndex(void) const;
#endif

#if OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME
    static uint8_t GetFcfSize(uint16_t aFcf) { return IsShortFcf(aFcf) ? kShortFcfSize : kFcfSize; }
#else
    // clang-format off
    static uint8_t GetFcfSize(uint16_t /* aFcf */) { return kFcfSize; }
    // clang-format on
#endif

#if OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME
    template <uint16_t kValue, uint16_t kMpValue> static uint16_t Select(uint16_t aFcf)
    {
        return IsMultipurpose(aFcf) ? kMpValue : kValue;
    }
#else
    template <uint16_t kValue, uint16_t kMpValue> static uint16_t Select(uint16_t /* aFcf */) { return kValue; }
#endif

    template <uint16_t kValue, uint16_t kMpValue> static uint16_t MaskFcf(uint16_t aFcf)
    {
        return aFcf & Select<kValue, kMpValue>(aFcf);
    }

    static uint16_t GetFcfDstAddr(uint16_t aFcf)
    {
        return MaskFcf<kFcfDstAddrMask, kMpFcfDstAddrMask>(aFcf) >> Select<kFcfDstAddrShift, kMpFcfDstAddrShift>(aFcf);
    }

    static uint16_t GetFcfSrcAddr(uint16_t aFcf)
    {
        return MaskFcf<kFcfSrcAddrMask, kMpFcfSrcAddrMask>(aFcf) >> Select<kFcfSrcAddrShift, kMpFcfSrcAddrShift>(aFcf);
    }

    static bool IsMultipurpose(uint16_t aFcf) { return (aFcf & kFcfFrameTypeMask) == kTypeMultipurpose; }
    static bool IsShortFcf(uint16_t aFcf)
    {
        return (aFcf & (kFcfFrameTypeMask | kMpFcfLongFrame)) == (kTypeMultipurpose | 0);
    }
    static bool IsSequencePresent(uint16_t aFcf)
    {
        return !MaskFcf<kFcfSequenceSuppression, kMpFcfSequenceSuppression>(aFcf);
    }
    static bool IsDstAddrPresent(uint16_t aFcf) { return MaskFcf<kFcfDstAddrMask, kMpFcfDstAddrMask>(aFcf); }
    static bool IsDstPanIdPresent(uint16_t aFcf);
    static bool IsSrcAddrPresent(uint16_t aFcf) { return MaskFcf<kFcfSrcAddrMask, kMpFcfSrcAddrMask>(aFcf); }
    static bool IsSrcPanIdPresent(uint16_t aFcf);
    static bool IsSecurityEnabled(uint16_t aFcf) { return MaskFcf<kFcfSecurityEnabled, kMpFcfSecurityEnabled>(aFcf); }
    static bool IsFramePending(uint16_t aFcf) { return MaskFcf<kFcfFramePending, kMpFcfFramePending>(aFcf); }
    static bool IsIePresent(uint16_t aFcf) { return MaskFcf<kFcfIePresent, kMpFcfIePresent>(aFcf); }
    static bool IsAckRequest(uint16_t aFcf) { return MaskFcf<kFcfAckRequest, kMpFcfAckRequest>(aFcf); }
    static bool IsVersion2015(uint16_t aFcf) { return (aFcf & kFcfFrameVersionMask) == kVersion2015; }

    static uint16_t DetermineFcfAddrType(const Address &aAddress, uint16_t aBitShift);

    static uint8_t CalculateAddrFieldSize(uint16_t aFcf);
    static uint8_t CalculateSecurityHeaderSize(uint8_t aSecurityControl);
    static uint8_t CalculateKeySourceSize(uint8_t aSecurityControl);
    static uint8_t CalculateMicSize(uint8_t aSecurityControl);
};

/**
 * Supports received IEEE 802.15.4 MAC frame processing.
 */
class RxFrame : public Frame
{
public:
    friend class TxFrame;

    /**
     * Returns the RSSI in dBm used for reception.
     *
     * @returns The RSSI in dBm used for reception.
     */
    int8_t GetRssi(void) const { return mInfo.mRxInfo.mRssi; }

    /**
     * Sets the RSSI in dBm used for reception.
     *
     * @param[in]  aRssi  The RSSI in dBm used for reception.
     */
    void SetRssi(int8_t aRssi) { mInfo.mRxInfo.mRssi = aRssi; }

    /**
     * Returns the receive Link Quality Indicator.
     *
     * @returns The receive Link Quality Indicator.
     */
    uint8_t GetLqi(void) const { return mInfo.mRxInfo.mLqi; }

    /**
     * Sets the receive Link Quality Indicator.
     *
     * @param[in]  aLqi  The receive Link Quality Indicator.
     */
    void SetLqi(uint8_t aLqi) { mInfo.mRxInfo.mLqi = aLqi; }

    /**
     * Indicates whether or not the received frame is acknowledged with frame pending set.
     *
     * @retval TRUE   This frame is acknowledged with frame pending set.
     * @retval FALSE  This frame is acknowledged with frame pending not set.
     */
    bool IsAckedWithFramePending(void) const { return mInfo.mRxInfo.mAckedWithFramePending; }

    /**
     * Returns the timestamp when the frame was received.
     *
     * The value SHALL be the time of the local radio clock in
     * microseconds when the end of the SFD (or equivalently: the start
     * of the first symbol of the PHR) was present at the local antenna,
     * see the definition of a "symbol boundary" in IEEE 802.15.4-2020,
     * section 6.5.2 or equivalently the RMARKER definition in section
     * 6.9.1 (albeit both unrelated to OT).
     *
     * The time is relative to the local radio clock as defined by
     * `Radio::GetNow()`.
     *
     * @returns The timestamp in microseconds.
     */
    const uint64_t &GetTimestamp(void) const { return mInfo.mRxInfo.mTimestamp; }

    /**
     * Performs AES CCM on the frame which is received.
     *
     * @param[in]  aExtAddress  A reference to the extended address, which will be used to generate nonce
     *                          for AES CCM computation.
     * @param[in]  aMacKey      A reference to the MAC key to decrypt the received frame.
     *
     * @retval kErrorNone      Process of received frame AES CCM succeeded.
     * @retval kErrorSecurity  Received frame MIC check failed.
     */
    Error ProcessReceiveAesCcm(const ExtAddress &aExtAddress, const KeyMaterial &aMacKey);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Gets the offset to network time.
     *
     * @returns  The offset to network time.
     */
    int64_t ComputeNetworkTimeOffset(void) const
    {
        return static_cast<int64_t>(GetTimeIe()->GetTime() - GetTimestamp());
    }

    /**
     * Gets the time sync sequence.
     *
     * @returns  The time sync sequence.
     */
    uint8_t ReadTimeSyncSeq(void) const { return GetTimeIe()->GetSequence(); }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
};

/**
 * Supports IEEE 802.15.4 MAC frame generation for transmission.
 */
class TxFrame : public Frame
{
public:
    /**
     * Represents header information.
     */
    struct Info : public Clearable<Info>
    {
        /**
         * Initializes the `Info` by clearing all its fields (setting all bytes to zero).
         */
        Info(void) { Clear(); }

        /**
         * Prepares MAC headers based on `Info` fields in a given `TxFrame`.
         *
         * This method uses the `Info` structure to construct the MAC address and security headers in @p aTxFrame.
         * It determines the Frame Control Field (FCF), including setting the appropriate frame type, security level,
         * and addressing mode flags. It populates the source and destination addresses and PAN IDs within the MAC
         * header based on the information provided in the `Info` structure.
         *
         * It sets the Ack Request bit in the FCF if the following criteria are met:
         *   - A destination address is present
         *   - The destination address is not the broadcast address
         *   - The frame type is not an ACK frame
         *
         * The header IE entries are prepared based on `mAppendTimeIe` and `mAppendCslIe` flags and the IE Present
         * flag in FCF is determined accordingly.
         *
         * The Frame Pending flag in FCF is not set. It may need to be set separately depending on the specific
         * requirements of the frame being transmitted.
         *
         * @param[in,out] aTxFrame  The `TxFrame` instance in which to prepare and append the MAC headers.
         */
        void PrepareHeadersIn(TxFrame &aTxFrame) const;

        Type          mType;                 ///< Frame type.
        Version       mVersion;              ///< Frame version.
        Addresses     mAddrs;                ///< Frame source and destination addresses.
        PanIds        mPanIds;               ///< Source and destination PAN Ids.
        SecurityLevel mSecurityLevel;        ///< Frame security level.
        KeyIdMode     mKeyIdMode;            ///< Frame security key ID mode.
        CommandId     mCommandId;            ///< Command ID (applicable when `mType == kTypeMacCmd`).
        bool          mSuppressSequence : 1; ///< Whether to suppress seq number.

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        bool mAppendTimeIe : 1; ///< Whether to append Time IE.
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        bool mAppendCslIe : 1; ///< Whether to append CSL IE.
#endif
        bool mEmptyPayload : 1; ///< Whether payload is empty (to decide about appending Termination2 IE).
#endif
    };

    /**
     * Sets the channel on which to send the frame.
     *
     * It also sets the `RxChannelAfterTxDone` to the same channel.
     *
     * @param[in]  aChannel  The channel used for transmission.
     */
    void SetChannel(uint8_t aChannel)
    {
        mChannel = aChannel;
        SetRxChannelAfterTxDone(aChannel);
    }

    /**
     * Sets TX power to send the frame.
     *
     * @param[in]  aTxPower  The tx power used for transmission.
     */
    void SetTxPower(int8_t aTxPower) { mInfo.mTxInfo.mTxPower = aTxPower; }

    /**
     * Gets the RX channel after frame TX is done.
     *
     * @returns The RX channel after frame TX is done.
     */
    uint8_t GetRxChannelAfterTxDone(void) const { return mInfo.mTxInfo.mRxChannelAfterTxDone; }

    /**
     * Sets the RX channel after frame TX is done.
     *
     * @param[in] aChannel   The RX channel after frame TX is done.
     */
    void SetRxChannelAfterTxDone(uint8_t aChannel) { mInfo.mTxInfo.mRxChannelAfterTxDone = aChannel; }

    /**
     * Returns the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access
     *          failure.
     */
    uint8_t GetMaxCsmaBackoffs(void) const { return mInfo.mTxInfo.mMaxCsmaBackoffs; }

    /**
     * Sets the maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel
     * access failure.
     *
     * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxCsmaBackoffs  The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring
     *                               a channel access failure.
     */
    void SetMaxCsmaBackoffs(uint8_t aMaxCsmaBackoffs) { mInfo.mTxInfo.mMaxCsmaBackoffs = aMaxCsmaBackoffs; }

    /**
     * Returns the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @returns The maximum number of retries allowed after a transmission failure.
     */
    uint8_t GetMaxFrameRetries(void) const { return mInfo.mTxInfo.mMaxFrameRetries; }

    /**
     * Sets the maximum number of retries allowed after a transmission failure.
     *
     * Equivalent to macMaxFrameRetries in IEEE 802.15.4-2006.
     *
     * @param[in]  aMaxFrameRetries  The maximum number of retries allowed after a transmission failure.
     */
    void SetMaxFrameRetries(uint8_t aMaxFrameRetries) { mInfo.mTxInfo.mMaxFrameRetries = aMaxFrameRetries; }

    /**
     * Indicates whether or not the frame is a retransmission.
     *
     * @retval TRUE   Frame is a retransmission
     * @retval FALSE  This is a new frame and not a retransmission of an earlier frame.
     */
    bool IsARetransmission(void) const { return mInfo.mTxInfo.mIsARetx; }

    /**
     * Sets the retransmission flag attribute.
     *
     * @param[in]  aIsARetx  TRUE if frame is a retransmission of an earlier frame, FALSE otherwise.
     */
    void SetIsARetransmission(bool aIsARetx) { mInfo.mTxInfo.mIsARetx = aIsARetx; }

    /**
     * Indicates whether or not CSMA-CA is enabled.
     *
     * @retval TRUE   CSMA-CA is enabled.
     * @retval FALSE  CSMA-CA is not enabled is not enabled.
     */
    bool IsCsmaCaEnabled(void) const { return mInfo.mTxInfo.mCsmaCaEnabled; }

    /**
     * Sets the CSMA-CA enabled attribute.
     *
     * @param[in]  aCsmaCaEnabled  TRUE if CSMA-CA must be enabled for this packet, FALSE otherwise.
     */
    void SetCsmaCaEnabled(bool aCsmaCaEnabled) { mInfo.mTxInfo.mCsmaCaEnabled = aCsmaCaEnabled; }

    /**
     * Returns the key used for frame encryption and authentication (AES CCM).
     *
     * @returns The pointer to the key.
     */
    const Mac::KeyMaterial &GetAesKey(void) const
    {
        return *static_cast<const Mac::KeyMaterial *>(mInfo.mTxInfo.mAesKey);
    }

    /**
     * Sets the key used for frame encryption and authentication (AES CCM).
     *
     * @param[in]  aAesKey  The pointer to the key.
     */
    void SetAesKey(const Mac::KeyMaterial &aAesKey) { mInfo.mTxInfo.mAesKey = &aAesKey; }

    /**
     * Copies the PSDU and all attributes (except for frame link type) from another frame.
     *
     * @note This method performs a deep copy meaning the content of PSDU buffer from the given frame is copied into
     * the PSDU buffer of the current frame.

     * @param[in] aFromFrame  The frame to copy from.
     */
    void CopyFrom(const TxFrame &aFromFrame);

    /**
     * Performs AES CCM on the frame which is going to be sent.
     *
     * @param[in]  aExtAddress  A reference to the extended address, which will be used to generate nonce
     *                          for AES CCM computation.
     */
    void ProcessTransmitAesCcm(const ExtAddress &aExtAddress);

    /**
     * Indicates whether or not the frame has security processed.
     *
     * @retval TRUE   The frame already has security processed.
     * @retval FALSE  The frame does not have security processed.
     */
    bool IsSecurityProcessed(void) const { return mInfo.mTxInfo.mIsSecurityProcessed; }

    /**
     * Sets the security processed flag attribute.
     *
     * @param[in]  aIsSecurityProcessed  TRUE if the frame already has security processed.
     */
    void SetIsSecurityProcessed(bool aIsSecurityProcessed)
    {
        mInfo.mTxInfo.mIsSecurityProcessed = aIsSecurityProcessed;
    }

    /**
     * Indicates whether or not the frame contains the CSL IE.
     *
     * @retval TRUE   The frame contains the CSL IE.
     * @retval FALSE  The frame does not contain the CSL IE.
     */
    bool IsCslIePresent(void) const { return mInfo.mTxInfo.mCslPresent; }

    /**
     * Sets the CSL IE present flag.
     *
     * @param[in]  aCslPresent  TRUE if the frame contains the CSL IE.
     */
    void SetCslIePresent(bool aCslPresent) { mInfo.mTxInfo.mCslPresent = aCslPresent; }

    /**
     * Indicates whether or not the frame header is updated.
     *
     * @retval TRUE   The frame already has the header updated.
     * @retval FALSE  The frame does not have the header updated.
     */
    bool IsHeaderUpdated(void) const { return mInfo.mTxInfo.mIsHeaderUpdated; }

    /**
     * Sets the header updated flag attribute.
     *
     * @param[in]  aIsHeaderUpdated  TRUE if the frame header is updated.
     */
    void SetIsHeaderUpdated(bool aIsHeaderUpdated) { mInfo.mTxInfo.mIsHeaderUpdated = aIsHeaderUpdated; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Sets the Time IE offset.
     *
     * @param[in]  aOffset  The Time IE offset, 0 means no Time IE.
     */
    void SetTimeIeOffset(uint8_t aOffset) { mInfo.mTxInfo.mIeInfo->mTimeIeOffset = aOffset; }

    /**
     * Gets the Time IE offset.
     *
     * @returns The Time IE offset, 0 means no Time IE.
     */
    uint8_t GetTimeIeOffset(void) const { return mInfo.mTxInfo.mIeInfo->mTimeIeOffset; }

    /**
     * Sets the offset to network time.
     *
     * @param[in]  aNetworkTimeOffset  The offset to network time.
     */
    void SetNetworkTimeOffset(int64_t aNetworkTimeOffset)
    {
        mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset = aNetworkTimeOffset;
    }

    /**
     * Sets the time sync sequence.
     *
     * @param[in]  aTimeSyncSeq  The time sync sequence.
     */
    void SetTimeSyncSeq(uint8_t aTimeSyncSeq) { mInfo.mTxInfo.mIeInfo->mTimeSyncSeq = aTimeSyncSeq; }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    /**
     * Generate Imm-Ack in this frame object.
     *
     * @param[in]    aFrame             A reference to the frame received.
     * @param[in]    aIsFramePending    Value of the ACK's frame pending bit.
     */
    void GenerateImmAck(const RxFrame &aFrame, bool aIsFramePending);

    /**
     * Generate Enh-Ack in this frame object.
     *
     * @param[in]    aRxFrame           A reference to the received frame.
     * @param[in]    aIsFramePending    Value of the ACK's frame pending bit.
     * @param[in]    aIeData            A pointer to the IE data portion of the ACK to be sent.
     * @param[in]    aIeLength          The length of IE data portion of the ACK to be sent.
     *
     * @retval  kErrorNone           Successfully generated Enh Ack.
     * @retval  kErrorParse          @p aRxFrame has incorrect format.
     */
    Error GenerateEnhAck(const RxFrame &aRxFrame, bool aIsFramePending, const uint8_t *aIeData, uint8_t aIeLength);

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    /**
     * Generate IEE 802.15.4 Wake-up frame.
     *
     * @param[in]    aPanId     A destination PAN identifier
     * @param[in]    aDest      A destination address (short or extended)
     * @param[in]    aSource    A source address (short or extended)
     *
     * @retval  kErrorNone        Successfully generated Wake-up frame.
     * @retval  kErrorInvalidArgs @p aDest or @p aSource have incorrect type.
     */
    Error GenerateWakeupFrame(PanId aPanId, const Address &aDest, const Address &aSource);
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    /**
     * Set TX delay field for the frame.
     *
     * @param[in]    aTxDelay    The delay time for the TX frame.
     */
    void SetTxDelay(uint32_t aTxDelay) { mInfo.mTxInfo.mTxDelay = aTxDelay; }

    /**
     * Set TX delay base time field for the frame.
     *
     * @param[in]    aTxDelayBaseTime    The delay base time for the TX frame.
     */
    void SetTxDelayBaseTime(uint32_t aTxDelayBaseTime) { mInfo.mTxInfo.mTxDelayBaseTime = aTxDelayBaseTime; }
#endif
};

OT_TOOL_PACKED_BEGIN
class Beacon
{
public:
    static constexpr uint16_t kSuperFrameSpec = 0x0fff; ///< Superframe Specification value.

    /**
     * Initializes the Beacon message.
     */
    void Init(void)
    {
        mSuperframeSpec     = LittleEndian::HostSwap16(kSuperFrameSpec);
        mGtsSpec            = 0;
        mPendingAddressSpec = 0;
    }

    /**
     * Indicates whether or not the beacon appears to be a valid Thread Beacon message.
     *
     * @retval TRUE   If the beacon appears to be a valid Thread Beacon message.
     * @retval FALSE  If the beacon does not appear to be a valid Thread Beacon message.
     */
    bool IsValid(void) const
    {
        return (mSuperframeSpec == LittleEndian::HostSwap16(kSuperFrameSpec)) && (mGtsSpec == 0) &&
               (mPendingAddressSpec == 0);
    }

    /**
     * Returns the pointer to the beacon payload.
     *
     * @returns A pointer to the beacon payload.
     */
    uint8_t *GetPayload(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    /**
     * Returns the pointer to the beacon payload.
     *
     * @returns A pointer to the beacon payload.
     */
    const uint8_t *GetPayload(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

private:
    uint16_t mSuperframeSpec;
    uint8_t  mGtsSpec;
    uint8_t  mPendingAddressSpec;
} OT_TOOL_PACKED_END;

/**
 * Implements IEEE 802.15.4 Beacon Payload generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class BeaconPayload
{
public:
    static constexpr uint8_t kProtocolId      = 3;                     ///< Thread Protocol ID.
    static constexpr uint8_t kProtocolVersion = 2;                     ///< Thread Protocol version.
    static constexpr uint8_t kVersionOffset   = 4;                     ///< Version field bit offset.
    static constexpr uint8_t kVersionMask     = 0xf << kVersionOffset; ///< Version field mask.
    static constexpr uint8_t kNativeFlag      = 1 << 3;                ///< Native Commissioner flag.
    static constexpr uint8_t kJoiningFlag     = 1 << 0;                ///< Joining Permitted flag.

    /**
     * Initializes the Beacon Payload.
     */
    void Init(void)
    {
        mProtocolId = kProtocolId;
        mFlags      = kProtocolVersion << kVersionOffset;
    }

    /**
     * Indicates whether or not the beacon appears to be a valid Thread Beacon Payload.
     *
     * @retval TRUE   If the beacon appears to be a valid Thread Beacon Payload.
     * @retval FALSE  If the beacon does not appear to be a valid Thread Beacon Payload.
     */
    bool IsValid(void) const { return (mProtocolId == kProtocolId); }

    /**
     * Returns the Protocol ID value.
     *
     * @returns the Protocol ID value.
     */
    uint8_t GetProtocolId(void) const { return mProtocolId; }

    /**
     * Returns the Protocol Version value.
     *
     * @returns The Protocol Version value.
     */
    uint8_t GetProtocolVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * Indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     */
    bool IsNative(void) const { return (mFlags & kNativeFlag) != 0; }

    /**
     * Clears the Native Commissioner flag.
     */
    void ClearNative(void) { mFlags &= ~kNativeFlag; }

    /**
     * Sets the Native Commissioner flag.
     */
    void SetNative(void) { mFlags |= kNativeFlag; }

    /**
     * Indicates whether or not the Joining Permitted flag is set.
     *
     * @retval TRUE   If the Joining Permitted flag is set.
     * @retval FALSE  If the Joining Permitted flag is not set.
     */
    bool IsJoiningPermitted(void) const { return (mFlags & kJoiningFlag) != 0; }

    /**
     * Clears the Joining Permitted flag.
     */
    void ClearJoiningPermitted(void) { mFlags &= ~kJoiningFlag; }

    /**
     * Sets the Joining Permitted flag.
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
     * Gets the Network Name field.
     *
     * @returns The Network Name field as `NameData`.
     */
    MeshCoP::NameData GetNetworkName(void) const { return MeshCoP::NameData(mNetworkName, sizeof(mNetworkName)); }

    /**
     * Sets the Network Name field.
     *
     * @param[in]  aNameData  The Network Name (as a `NameData`).
     */
    void SetNetworkName(const MeshCoP::NameData &aNameData) { aNameData.CopyTo(mNetworkName, sizeof(mNetworkName)); }

    /**
     * Returns the Extended PAN ID field.
     *
     * @returns The Extended PAN ID field.
     */
    const otExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * Sets the Extended PAN ID field.
     *
     * @param[in]  aExtPanId  An Extended PAN ID.
     */
    void SetExtendedPanId(const otExtendedPanId &aExtPanId) { mExtendedPanId = aExtPanId; }

private:
    uint8_t         mProtocolId;
    uint8_t         mFlags;
    char            mNetworkName[MeshCoP::NetworkName::kMaxSize];
    otExtendedPanId mExtendedPanId;
} OT_TOOL_PACKED_END;

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // MAC_FRAME_HPP_
