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

#ifndef OT_CORE_MAC_MAC_FRAME_HPP_
#define OT_CORE_MAC_MAC_FRAME_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "common/numeric_limits.hpp"
#include "mac/mac_header_ie.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/network_name.hpp"
#include "radio/radio_frame.hpp"
#include "radio/radio_types.hpp"

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
class Frame : public Radio::Frame
{
public:
    /**
     * Represents the MAC frame type.
     *
     * Values match the Frame Type field in Frame Control Field (FCF).
     */
    enum Type : uint8_t
    {
        kTypeBeacon = 0, ///< Beacon Frame Type.
        kTypeData   = 1, ///< Data Frame Type.
        kTypeAck    = 2, ///< Ack Frame Type.
        kTypeMacCmd = 3, ///< MAC Command Frame Type.
    };

    /**
     * Represents the MAC frame version.
     *
     * Values match the raw (unshifted) Version sub-field (2-bit wide) in Frame Control Field (FCF). The enum does
     * not cover all possible 2-bit values.
     */
    enum Version : uint8_t
    {
        kVersion2003 = 0, ///< 2003 Frame Version.
        kVersion2006 = 1, ///< 2006 Frame Version.
        kVersion2015 = 2, ///< 2015 Frame Version.
    };

    /**
     * Represents the MAC frame security level.
     *
     * Values represent the raw (unshifted) Security Level sub-field (3-bit wide) from the Security Control field.
     * The enum covers all possible 3-bit values.
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
     * Values represent the raw (unshifted) Key ID Mode sub-field (2-bit wide) from the Security Control field.
     * The enum covers all possible 2-bit values.
     */
    enum KeyIdMode : uint8_t
    {
        kKeyIdMode0 = 0, ///< Key ID Mode 0 - Key is determined implicitly.
        kKeyIdMode1 = 1, ///< Key ID Mode 1 - Key is determined from Key Index field.
        kKeyIdMode2 = 2, ///< Key ID Mode 2 - Key is determined from 4-bytes Key Source and Index fields.
        kKeyIdMode3 = 3, ///< Key ID Mode 3 - Key is determined from 8-bytes Key Source and Index fields.
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

    static constexpr uint8_t kKeySourceSizeMode0 = 0; ///< Key Source size in bytes for Key ID Mode 0.
    static constexpr uint8_t kKeySourceSizeMode1 = 0; ///< Key Source size in bytes for Key ID Mode 1.
    static constexpr uint8_t kKeySourceSizeMode2 = 4; ///< Key Source size in bytes for Key ID Mode 2.
    static constexpr uint8_t kKeySourceSizeMode3 = 8; ///< Key Source size in bytes for Key ID Mode 3.

    static constexpr uint16_t kInfoStringSize = 128; ///< Max chars for `InfoString` (ToInfoString()).

    /**
     * Defines the fixed-length `String` object returned from `ToInfoString()` method.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Represents the length breakdown of a MAC frame.
     */
    struct Lengths
    {
        uint16_t mHeader;     ///< Header length (in bytes).
        uint16_t mPayload;    ///< Payload length (in bytes).
        uint16_t mFooter;     ///< Footer length (in bytes).
        uint16_t mMaxPayload; ///< Maximum allowed payload length (in bytes).
    };

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
    uint8_t GetType(void) const { return ReadType(GetFrameControlField()); }

    /**
     * Returns whether the frame is an Ack frame.
     *
     * @retval TRUE   If this is an Ack.
     * @retval FALSE  If this is not an Ack.
     */
    bool IsAck(void) const { return GetType() == kTypeAck; }

    /**
     * Returns whether the frame is a MAC Command frame.
     *
     * @retval TRUE   If this is a MAC Command frame.
     * @retval FALSE  If this is not a MAC Command Frame.
     */
    bool IsMacCommand(void) const { return GetType() == kTypeMacCmd; }

    /**
     * Returns the IEEE 802.15.4 Frame Version.
     *
     * @returns The IEEE 802.15.4 Frame Version.
     */
    uint8_t GetVersion(void) const { return ReadVersion(GetFrameControlField()); }

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
     * @param[in]  aFramePending  The Frame Pending bit.
     */
    void SetFramePending(bool aFramePending) { UpdateFcfFlag(aFramePending, kFcfFramePending); }

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
     * @param[in]  aAckRequest  The Ack Request bit.
     */
    void SetAckRequest(bool aAckRequest) { UpdateFcfFlag(aAckRequest, kFcfAckRequest); }

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
     * @param[in]  aIePresent   The IE Present bit.
     */
    void SetIePresent(bool aIePresent) { UpdateFcfFlag(aIePresent, kFcfIePresent); }

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
    bool IsSequencePresent(void) const { return IsSeqPresent(GetFrameControlField()); }

    /**
     * Gets the Destination PAN Identifier.
     *
     * @param[out]  aPanId  The Destination PAN Identifier.
     *
     * @retval kErrorNone      Successfully retrieved the Destination PAN Identifier.
     * @retval kErrorNotFound  Destination PAN Identifier is not present in the frame.
     * @retval kErrorParse     Failed to parse the frame.
     */
    Error GetDstPanId(PanId &aPanId) const;

    /**
     * Gets the Destination Address.
     *
     * @param[out]  aAddress  The Destination Address.
     *
     * @retval kErrorNone      Successfully retrieved the Destination Address.
     * @retval kErrorParse     Failed to parse the frame.
     */
    Error GetDstAddr(Address &aAddress) const;

    /**
     * Gets the Source PAN Identifier.
     *
     * @param[out]  aPanId  The Source PAN Identifier.
     *
     * @retval kErrorNone      Successfully retrieved the Source PAN Identifier.
     * @retval kErrorNotFound  Source PAN Identifier is not present in the frame.
     * @retval kErrorParse     Failed to parse the frame.
     */
    Error GetSrcPanId(PanId &aPanId) const;

    /**
     * Gets the Source Address.
     *
     * @param[out]  aAddress  The Source Address.
     *
     * @retval kErrorNone      Successfully retrieved the Source Address.
     * @retval kErrorParse     Failed to parse the frame.
     */
    Error GetSrcAddr(Address &aAddress) const;

    /**
     * Gets the Security Level Identifier.
     *
     * @param[out]  aSecurityLevel  The Security Level Identifier.
     *
     * @retval kErrorNone      Successfully retrieved the Security Level Identifier.
     * @retval kErrorNotFound  Frame does not have a security header (security is not enabled)
     * @retval kErrorParse     Failed to parse MAC or security header.
     */
    Error GetSecurityLevel(SecurityLevel &aSecurityLevel) const;

    /**
     * Indicates whether or not the frame has a specific Security Level.
     *
     * @param[in]  aSecurityLevel  The Security Level to check.
     *
     * @retval TRUE   The frame contains a valid security header matching @p aSecurityLevel.
     * @retval FALSE  The frame does not match @p aSecurityLevel or fails to parse MAC or security header.
     */
    bool HasSecurityLevel(SecurityLevel aSecurityLevel) const;

    /**
     * Gets the Key Identifier Mode.
     *
     * @param[out]  aKeyIdMode  The Key Identifier Mode.
     *
     * @retval kErrorNone   Successfully retrieved the Key Identifier Mode.
     * @retval kErrorParse  Failed to parse MAC or security header.
     */
    Error GetKeyIdMode(KeyIdMode &aKeyIdMode) const;

    /**
     * Indicates whether or not the frame has a specific Key Identifier Mode.
     *
     * @param[in]  aKeyIdMode  The Key Identifier Mode to check.
     *
     * @retval TRUE   The frame contains a valid security header matching @p aKeyIdMode.
     * @retval FALSE  The frame does not match @p aKeyIdMode or fails to parse MAC or security header.
     */
    bool HasKeyIdMode(KeyIdMode aKeyIdMode) const;

    /**
     * Gets the Frame Counter.
     *
     * @param[out]  aFrameCounter  The Frame Counter.
     *
     * @retval kErrorNone   Successfully retrieved the Frame Counter.
     * @retval kErrorParse  Failed to parse MAC or security header.
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
     * @param[out]  aKeySource   A `FrameData` to point to key source data bytes.
     */
    void GetKeySource(FrameData &aKeySource) const;

    /**
     * Sets the Key Source.
     *
     * @param[in]  aKeySource  A pointer to the Key Source value.
     */
    void SetKeySource(const uint8_t *aKeySource);

    /**
     * Gets the Key Index (sub-field of Key ID).
     *
     * @param[out]  aKeyIndex  The Key Index
     *
     * @retval kErrorNone      Successfully retrieved the Key Index.
     * @retval kErrorNotFound  Frame is using `kKeyIdMode0` which does not have any Key Index.
     * @retval kErrorParse     Failed to parse MAC or security header.
     */
    Error GetKeyIndex(uint8_t &aKeyIndex) const;

    /**
     * Sets the Key Index (sub-field of Key ID).
     *
     * @param[in]  aKeyIndex  The Key Index.
     */
    void SetKeyIndex(uint8_t aKeyIndex);

    /**
     * Gets the Command ID.
     *
     * @param[out]  aCommandId  The Command ID.
     *
     * @retval kErrorNone      Successfully retrieved the Command ID.
     * @retval kErrorNotFound  The frame is not a MAC command.
     * @retval kErrorParse     Failed to parse frame.
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
     * Gets the frame payload as `FrameData`.
     *
     * For MAC Command frames (`kTypeMacCmd`), the treatment of the Command ID field depends on the frame version:
     *   - For 2015 version , the Command ID is part of the payload, so @p aPayloadData includes it.
     *   - For earlier versions (2003/2006), the Command ID is part of the MAC header, so @p aPayloadData starts after
     *     the Command ID.
     *
     * @param[out] aPayloadData  A reference to a `FrameData` to return the frame payload.
     *
     * @retval kErrorNone   Successfully retrieved the frame payload.
     * @retval kErrorParse  Failed to parse the frame.
     */
    Error GetPayload(FrameData &aPayloadData) const;

    /**
     * Determines the length breakdown of the frame.
     *
     * @param[out] aLengths  A reference to a `Lengths` structure to return the frame lengths.
     *
     * @retval kErrorNone   Successfully calculated frame lengths.
     * @retval kErrorParse  Failed to parse the frame.
     */
    Error DetermineLengths(Lengths &aLengths) const;

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    /**
     * Finds a specific Information Element (IE) in the frame.
     *
     * This method searches the frame for a Header IE matching the Element ID of @p IeType and also validates that
     * the content of the IE is well-formed according to @p IeType.
     *
     * @tparam IeType  The IE subclass type to find.
     *
     * @returns A pointer to the IE, or `nullptr` if not found or if the IE content is malformed.
     */
    template <typename IeType> const IeType *Find(void) const
    {
        return static_cast<const IeType *>(FindHeaderIe(HeaderIe::ValidateAs<IeType>));
    }

    /**
     * Finds a specific Information Element (IE) in the frame.
     *
     * This method searches the frame for a Header IE matching the Element ID of @p IeType and also validates that
     * the content of the IE is well-formed according to @p IeType.
     *
     * @tparam IeType  The IE subclass type to find.
     *
     * @returns A pointer to the IE, or `nullptr` if not found or if the IE content is malformed.
     */
    template <typename IeType> IeType *Find(void) { return AsNonConst(AsConst(this)->Find<IeType>()); }

    /**
     * Indicates whether or not the frame contains a specific Information Element (IE).
     *
     * This method checks whether the frame contains a Header IE matching the Element ID of @p IeType with valid
     * content according to @p IeType.
     *
     * @tparam IeType  The IE subclass type to check.
     *
     * @retval TRUE   The frame contains a valid instance of the IE.
     * @retval FALSE  The frame does not contain the IE or its content is malformed.
     */
    template <typename IeType> bool Has(void) const { return Find<IeType>() != nullptr; }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Updates CSL IE content in the frame.
     *
     * @param[in] aCslPeriod  CSL Period in CSL IE.
     * @param[in] aCslPhase   CSL Phase in CSL IE.
     */
    void UpdateCslIe(uint16_t aCslPeriod, uint16_t aCslPhase);
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    /**
     * Finds Enhanced ACK Probing (Vendor Specific) IE and updates its Link Metrics Data content.
     *
     * @param[in] aData   A pointer to the data to write.
     * @param[in] aLen    The length of @p aData.
     */
    void UpdateEnhAckProbingIe(const uint8_t *aData, uint8_t aLen);
#endif

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

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
    uint16_t GetFrameControlField(void) const { return LittleEndian::ReadUint16(mPsdu); }

    /**
     * Returns the Immediate Acknowledgment (Imm-Ack) frame length in bytes.
     *
     * @returns The Imm-Ack frame length in bytes.
     */
    static constexpr uint8_t GetImmAckLength(void) { return kImmAckLength; }

    /**
     * Constructs a Security Control byte from a given Security Level and Key ID Mode.
     *
     * @param[in]  aSecurityLevel   The Security Level.
     * @param[in]  aKeyIdMode       The Key Identifier Mode.
     *
     * @returns The constructed Security Control byte.
     */
    static constexpr uint8_t ConstructSecurityControlField(SecurityLevel aSecurityLevel, KeyIdMode aKeyIdMode)
    {
        return static_cast<uint8_t>(static_cast<uint8_t>(aSecurityLevel) |
                                    (static_cast<uint8_t>(aKeyIdMode) << kScfKeyIdModeShift));
    }

protected:
    static constexpr uint8_t kFcfSize      = sizeof(uint16_t);
    static constexpr uint8_t kDsnSize      = sizeof(uint8_t);
    static constexpr uint8_t kImmAckLength = kFcfSize + kDsnSize + k154FcsSize;

    static constexpr uint8_t kSecurityControlSize = sizeof(uint8_t);
    static constexpr uint8_t kFrameCounterSize    = sizeof(uint32_t);
    static constexpr uint8_t kCommandIdSize       = sizeof(uint8_t);
    static constexpr uint8_t kKeyIndexSize        = sizeof(uint8_t);

    static constexpr uint16_t kFcfFrameTypeMask = 7 << 0;

    enum AddrMode : uint8_t
    {
        kAddrModeNone     = 0,
        kAddrModeReserved = 1,
        kAddrModeShort    = 2,
        kAddrModeExt      = 3,
    };

    static constexpr uint16_t kFcfAddrMask = 3;

    // Frame Control field format for general MAC frame
    static constexpr uint16_t kFcfSecurityEnabled  = 1 << 3;
    static constexpr uint16_t kFcfFramePending     = 1 << 4;
    static constexpr uint16_t kFcfAckRequest       = 1 << 5;
    static constexpr uint16_t kFcfPanidCompression = 1 << 6;
    static constexpr uint16_t kFcfSeqSuppression   = 1 << 8;
    static constexpr uint16_t kFcfIePresent        = 1 << 9;
    static constexpr uint16_t kFcfDstAddrShift     = 10;
    static constexpr uint16_t kFcfDstAddrNone      = kAddrModeNone << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrShort     = kAddrModeShort << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrExt       = kAddrModeExt << kFcfDstAddrShift;
    static constexpr uint16_t kFcfDstAddrMask      = kFcfAddrMask << kFcfDstAddrShift;
    static constexpr uint16_t kFcfVersionShift     = 12;
    static constexpr uint16_t kFcfVersionMask      = 3 << kFcfVersionShift;
    static constexpr uint16_t kFcfSrcAddrShift     = 14;
    static constexpr uint16_t kFcfSrcAddrNone      = kAddrModeNone << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrShort     = kAddrModeShort << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrExt       = kAddrModeExt << kFcfSrcAddrShift;
    static constexpr uint16_t kFcfSrcAddrMask      = kFcfAddrMask << kFcfSrcAddrShift;

    // Security Control field
    static constexpr uint8_t kScfKeyIdModeShift = 3;
    static constexpr uint8_t kScfSecLevelMask   = 7 << 0;
    static constexpr uint8_t kScfKeyIdModeMask  = 3 << kScfKeyIdModeShift;

    static constexpr uint8_t kMic0Size   = 0;
    static constexpr uint8_t kMic32Size  = 32 / kBitsPerByte;
    static constexpr uint8_t kMic64Size  = 64 / kBitsPerByte;
    static constexpr uint8_t kMic128Size = 128 / kBitsPerByte;
    static constexpr uint8_t kMaxMicSize = kMic128Size;

    static constexpr uint8_t kInvalidIndex = 0xff;
    static constexpr uint8_t kInvalidSize  = kInvalidIndex;
    static constexpr uint8_t kMaxPsduSize  = kInvalidSize - 1;

    enum ParseMode : uint8_t
    {
        kParseAddrFields,
        kParseSecurityHeader,
        kParseFully,
    };

    class ParseInfo
    {
    public:
        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Mac Header Address Info
        Type      mType;
        Version   mVersion;
        bool      mIsSecurityEnabled : 1;
        bool      mIsFramePending : 1;
        bool      mIsAckRequest : 1;
        bool      mIsSeqNumPresent : 1;
        bool      mIsIePresent : 1;
        uint8_t   mSequenceNum;
        PanIds    mPanIds;
        Addresses mAddrs;

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Aux Security Header
        SecurityLevel mSecurityLevel;
        KeyIdMode     mKeyIdMode;
        uint8_t       mKeyIndex;
        uint8_t       mMicSize;
        uint32_t      mFrameCounter;
        FrameData     mKeySource;
        uint8_t      *mFrameCounterBytes;
        uint8_t      *mKeyIndexByte;

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Header IEs
        FrameData mIeData;

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // MAC Command ID
        uint8_t mCommandId;

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Header and Payload breakdown
        FrameData mHeader;
        FrameData mPayload;

        Error ParseFrom(const Frame &aFrame, ParseMode aMode);

    private:
        static Error ParseAddress(FrameData &aFrameData, AddrMode aAddrMode, Address &aAddress);
    };

    void UpdateFcfFlag(bool aSet, uint16_t aBitFlag);

    static uint8_t  ReadType(uint16_t aFcf) { return As<uint8_t>(ReadBits<uint16_t, kFcfFrameTypeMask>(aFcf)); }
    static AddrMode ReadDstAddrMode(uint16_t aFcf) { return As<AddrMode>(ReadBits<uint16_t, kFcfDstAddrMask>(aFcf)); }
    static AddrMode ReadSrcAddrMode(uint16_t aFcf) { return As<AddrMode>(ReadBits<uint16_t, kFcfSrcAddrMask>(aFcf)); }
    static bool     IsSeqSuppressed(uint16_t aFcf) { return IsVersion2015(aFcf) && ((aFcf & kFcfSeqSuppression) != 0); }
    static bool     IsSeqPresent(uint16_t aFcf) { return !IsSeqSuppressed(aFcf); }
    static bool     IsDstAddrPresent(uint16_t aFcf) { return ReadDstAddrMode(aFcf) != kAddrModeNone; }
    static bool     IsSrcAddrPresent(uint16_t aFcf) { return ReadSrcAddrMode(aFcf) != kAddrModeNone; }
    static bool     IsSecurityEnabled(uint16_t aFcf) { return (aFcf & kFcfSecurityEnabled) != 0; }
    static bool     IsFramePending(uint16_t aFcf) { return (aFcf & kFcfFramePending) != 0; }
    static bool     IsIePresent(uint16_t aFcf) { return IsVersion2015(aFcf) && ((aFcf & kFcfIePresent) != 0); }
    static bool     IsAckRequest(uint16_t aFcf) { return (aFcf & kFcfAckRequest) != 0; }
    static uint8_t  ReadVersion(uint16_t aFcf) { return As<uint8_t>(ReadBits<uint16_t, kFcfVersionMask>(aFcf)); }
    static bool     IsVersion2015(uint16_t aFcf) { return ReadVersion(aFcf) == kVersion2015; }
    static bool     IsDstPanIdPresent(uint16_t aFcf);
    static bool     IsSrcPanIdPresent(uint16_t aFcf);
    static AddrMode DetermineAddrMode(const Address &aAddress);
    static uint8_t  CalculateKeySourceSize(KeyIdMode aKeyIdMode);
    static uint8_t  CalculateMicSize(SecurityLevel aSecurityLevel);
    static uint16_t ConstructFrameControlField(Type aType, uint8_t aVersion);

    // Security Control fields
    static SecurityLevel ReadSecurityLevel(uint8_t aSecCtl);
    static KeyIdMode     ReadKeyIdMode(uint8_t aSecCtl);

private:
    template <typename EnumType> static EnumType As(uint16_t aValue) { return static_cast<EnumType>(aValue); }

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    typedef bool (&HeaderIeMatcher)(const HeaderIe &aHeaderIe);

    const HeaderIe *FindHeaderIe(HeaderIeMatcher aMatcher) const;
    HeaderIe       *FindHeaderIe(HeaderIeMatcher aMatcher) { return AsNonConst(AsConst(this)->FindHeaderIe(aMatcher)); }
#endif
};

/**
 * Supports received IEEE 802.15.4 MAC frame processing.
 */
class RxFrame : public Frame, public Radio::RxFrameProperties<RxFrame>
{
public:
    friend class TxFrame;

    /**
     * Defines flags to indicate allowed Key ID Modes, used in `IsSecuredWith()`.
     */
    enum KeyIdModeFlag : uint8_t
    {
        kAllowKeyIdMode0 = (1 << 0), ///< Allow Key ID Mode 0.
        kAllowKeyIdMode1 = (1 << 1), ///< Allow Key ID Mode 1.
    };

    /**
     * Represents a set of `KeyIdModeFlag`s.
     */
    typedef uint8_t KeyIdModeFlags;

    /**
     * Indicates whether the frame is secured with a given set of allowed Key ID Modes.
     *
     * @param[in] aFlags  A bitmask of `KeyIdModeFlags` specifying the allowed modes.
     *
     * @retval TRUE   The frame has security enabled and uses one of the allowed Key ID Modes.
     * @retval FALSE  The frame does not have security enabled, or its Key ID Mode is not allowed.
     */
    bool IsSecuredWith(KeyIdModeFlags aFlags) const;

#if OPENTHREAD_FTD || OPENTHREAD_MTD
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
#endif
};

/**
 * Supports IEEE 802.15.4 MAC frame generation for transmission.
 */
class TxFrame : public Frame, public Radio::TxFrameProperties<TxFrame>
{
public:
    /**
     * Represents the information to use to build the frame.
     */
    class BuildInfo : public Clearable<BuildInfo>
    {
        friend class TxFrame;

    public:
        /**
         * Initializes the `BuildInfo` by clearing all its fields (setting all bytes to zero).
         */
        BuildInfo(void) { Clear(); }

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

    private:
        void PrepareHeadersIn(TxFrame &aTxFrame) const;
    };

    /**
     * Helper class for building the payload of a `TxFrame`.
     */
    class PayloadBuilder : public FrameBuilder
    {
        friend class TxFrame;

    public:
        /**
         * Gets the header length of the frame being built.
         *
         * @returns The header length (in bytes).
         */
        uint16_t GetHeaderLength(void) const { return mLengths.mHeader; }

        /**
         * Gets the footer length of the frame being built.
         *
         * @returns The footer length (in bytes).
         */
        uint16_t GetFooterLength(void) const { return mLengths.mFooter; }

    private:
        void     InitFrom(TxFrame &aFrame);
        uint16_t GetTotalLength(void) const { return GetLength() + mLengths.mHeader + mLengths.mFooter; }

        Lengths mLengths;
    };

    /**
     * Prepares MAC headers in the frame based on `BuildInfo` settings and initializes a `PayloadBuilder`.
     *
     * This method uses the `BuildInfo` structure to construct the MAC address and security headers in the frame.
     * It determines the Frame Control Field (FCF), including setting the appropriate frame type, security level,
     * and addressing mode flags. It populates the source and destination addresses and PAN IDs within the MAC
     * header based on the information provided in the `BuildInfo` structure.
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
     * The provided @p aPayloadBuilder is initialized to allow building and appending payload bytes directly
     * into the frame buffer following the prepared headers. It is set up with the maximum available payload
     * capacity based on the frame header and footer lengths and its MTU. Callers can use @p aPayloadBuilder to
     * construct the frame payload. Once payload construction is complete, `FinishPayload()` can be called to update
     * and finalize the total frame length.
     *
     * For MAC Command frames (`kTypeMacCmd`), whether the Command ID field is treated as part of the header or
     * payload depends on the frame version (see `GetPayload()`). The same rules apply to @p aPayloadBuilder here:
     *   - For 2015 version, the Command ID is part of the payload, so @p aPayloadBuilder starts before the Command ID.
     *   - For earlier versions (2003/2006), the Command ID is part of the MAC header, so @p aPayloadBuilder starts
     *     after the Command ID.
     *
     * @param[in]  aBuildInfo       The `BuildInfo` containing settings for the MAC headers.
     * @param[out] aPayloadBuilder  A reference to a `PayloadBuilder` to initialize for payload construction.
     */
    void PrepareHeaders(const BuildInfo &aBuildInfo, PayloadBuilder &aPayloadBuilder);

    /**
     * Finishes building the frame payload and updates the total frame length.
     *
     * @param[in] aPayloadBuilder  The `PayloadBuilder` used to construct the payload.
     */
    void FinishPayload(const PayloadBuilder &aPayloadBuilder) { SetLength(aPayloadBuilder.GetTotalLength()); }

    /**
     * Prepares MAC headers in the frame assuming an empty payload.
     *
     * See `PrepareHeaders()` for more details on how the MAC headers are constructed.
     *
     * @param[in] aBuildInfo  The `BuildInfo` structure containing settings for the MAC headers.
     */
    void PrepareHeadersWithEmptyPayload(const BuildInfo &aBuildInfo) { aBuildInfo.PrepareHeadersIn(*this); }

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
     * Restore the frame for transmit processing.
     *
     * @param[in]  aExtAddress  A reference to the extended address, which will be used to generate nonce
     *                          for AES CCM computation.
     */
    void RestoreTransmitSecurity(const ExtAddress &aExtAddress);

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
     * @param[in]    aPanId          A destination PAN identifier
     * @param[in]    aWakeupRequest  A const reference to the wake-up request.
     * @param[in]    aSource         A source address (short or extended)
     *
     * @retval  kErrorNone        Successfully generated Wake-up frame.
     * @retval  kErrorInvalidArgs @p aDest or @p aSource have incorrect type.
     */
    Error GenerateWakeupFrame(PanId, const WakeupRequest &, const Address &) { return kErrorNotImplemented; }
#endif

private:
    enum AesCcmOperation : uint8_t
    {
        kEncrypt,
        kDecrypt,
    };

    Error PerformAesCcm(AesCcmOperation aOperation, const ExtAddress &aExtAddress);
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_FRAME_HPP_
