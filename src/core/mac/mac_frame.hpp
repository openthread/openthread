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
protected:
    enum AddrMode : uint8_t;

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

    /**
     * Specifies the parsing mode for `ParseInfo::ParseFrom()`.
     *
     * In `kParseSecurityHeader` mode, the frame is explicitly required to have security enabled in FCF; otherwise
     * `kErrorNotFound` is returned.
     */
    enum ParseMode : uint8_t
    {
        kParseAddrFields,     ///< Parse up through address fields (FCF, SecNum, PAN IDs, Addrs) and FCS.
        kParseSecurityHeader, ///< Parse up through Auxiliary Security Header (requires security enabled).
        kParseFully,          ///< Parse all headers fully.
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
     * Represents parsed information from a MAC frame header.
     */
    class ParseInfo : public Clearable<ParseInfo>
    {
        friend class Frame; // TODO: At some point we should no longer need this, remove it.

    public:
        /**
         * Initializes the `ParseInfo` object.
         */
        ParseInfo(void) { Clear(); }

        /**
         * Parses the MAC frame header according to the specified parsing mode.
         *
         * @param[in] aFrame  The frame to parse from.
         * @param[in] aMode   The parsing mode.
         *
         * @retval kErrorNone      Successfully parsed the frame according to @p aMode.
         * @retval kErrorNotFound  Security is not enabled when @p aMode is `kParseSecurityHeader`.
         * @retval kErrorParse     Failed to parse the frame (frame is malformed).
         */
        Error ParseFrom(const Frame &aFrame, ParseMode aMode);

        /**
         * Returns human-readable string corresponding to the frame information.
         *
         * @returns An `InfoString` containing info about the frame.
         */
        InfoString ToInfoString(void) const;

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

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

        // - - - - - - - - - - - - - - - - - - - - - - - - -

        const Frame *mFrame; ///< The parsed frame.

        bool mParsedAddrFields : 1;     ///< TRUE if address fields are successfully parsed.
        bool mParsedSecurityHeader : 1; ///< TRUE if Auxiliary Security Header is present and successfully parsed.
        bool mParsedFully : 1;          ///< TRUE if the frame is fully parsed.

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Mac Header Address Info

        bool      mIsSecurityEnabled : 1; ///< TRUE if security is enabled, FALSE otherwise.
        bool      mIsFramePending : 1;    ///< TRUE if frame pending bit is set, FALSE otherwise.
        bool      mIsAckRequest : 1;      ///< TRUE if ACK request bit is set, FALSE otherwise.
        bool      mIsSeqNumPresent : 1;   ///< TRUE if sequence number is present, FALSE otherwise.
        bool      mIsIePresent : 1;       ///< TRUE if IE present bit is set, FALSE otherwise.
        Type      mType;                  ///< The frame type.
        Version   mVersion;               ///< The frame version.
        uint8_t   mSequenceNum;           ///< The sequence number (valid if `mIsSeqNumPresent`).
        PanIds    mPanIds;                ///< Source and Destination PAN IDs.
        Addresses mAddrs;                 ///< Source and Destination addresses.

        // - - - - - - - - - - - - - - - - - - - - - - - - -
        // Aux Security Header (valid if `mIsSecurityEnabled`)

        SecurityLevel mSecurityLevel; ///< The security level.
        KeyIdMode     mKeyIdMode;     ///< The Key ID mode.
        uint8_t       mKeyIndex;      ///< The Key Index.
        uint8_t       mMicSize;       ///< The MIC size in bytes.
        uint32_t      mFrameCounter;  ///< The security frame counter.
        FrameData     mKeySource;     ///< The Key Source data.

        // - - - - - - - - - - - - - - - - - - - - - - - - -

        FrameData mIeData; ///< The Header IE data.

        // - - - - - - - - - - - - - - - - - - - - - - - - -

        uint8_t mCommandId; ///< The MAC Command ID (valid if `mType == kTypeMacCmd`).

        // - - - - - - - - - - - - - - - - - - - - - - - - -

        FrameData mHeader; ///< The frame header (MHR) sub-range (see `mPayload` for treatment of Command ID).

        /**
         * The frame payload (MAC payload) sub-range.
         *
         * For MAC Command frames (`kTypeMacCmd`), the treatment of the Command ID field depends on the frame version:
         *  - For 2015 version, the Command ID is part of the payload and included in `mPayload`.
         *  - For earlier versions (2003/2006), the Command ID is part of the MAC header, so `mPayload` starts after
         *    the Command ID. In this case the Command ID is part of `mHeader`.
         */
        FrameData mPayload;

    protected:
        enum AesCcmOperation : uint8_t
        {
            kEncrypt,
            kDecrypt,
        };

        Error PerformAesCcm(AesCcmOperation aOperation, const ExtAddress &aExtAddress, const KeyMaterial &aMacKey);

        uint8_t *mKeyIndexByte;
        uint8_t *mFrameCounterBytes;

    private:
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
        typedef bool (&HeaderIeMatcher)(const HeaderIe &aHeaderIe);

        const HeaderIe *FindHeaderIe(HeaderIeMatcher aMatcher) const;
#endif

        static Error ParseAddress(FrameData &aFrameData, AddrMode aAddrMode, Address &aAddress);
    };

    /**
     * Sets the Frame Pending bit.
     *
     * @param[in]  aFramePending  The Frame Pending bit.
     */
    void SetFramePending(bool aFramePending) { UpdateFcfFlag(aFramePending, kFcfFramePending); }

    /**
     * Sets the Ack Request bit.
     *
     * @param[in]  aAckRequest  The Ack Request bit.
     */
    void SetAckRequest(bool aAckRequest) { UpdateFcfFlag(aAckRequest, kFcfAckRequest); }

    /**
     * Sets the IE Present bit.
     *
     * @param[in]  aIePresent   The IE Present bit.
     */
    void SetIePresent(bool aIePresent) { UpdateFcfFlag(aIePresent, kFcfIePresent); }

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
    static constexpr uint8_t kSeqNumIndex  = kFcfSize;
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

    uint16_t GetFrameControlField(void) const { return LittleEndian::ReadUint16(mPsdu); }
    void     UpdateFcfFlag(bool aSet, uint16_t aBitFlag);

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
};

/**
 * Supports received IEEE 802.15.4 MAC frame processing.
 */
class RxFrame : public Frame, public Radio::RxFrameProperties<RxFrame>
{
    friend class TxFrame;

public:
    /**
     * Represents parsed information from a received MAC frame.
     */
    class ParseInfo : public Frame::ParseInfo
    {
    public:
        /**
         * Returns a pointer to the associated `RxFrame`.
         *
         * @returns A pointer to the `RxFrame`.
         */
        const RxFrame *GetRxFrame(void) const { return static_cast<const RxFrame *>(mFrame); }

        /**
         * Returns a pointer to the associated `RxFrame`.
         *
         * @returns A pointer to the `RxFrame`.
         */
        RxFrame *GetRxFrame(void) { return AsNonConst(AsConst(this)->GetRxFrame()); }

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
};

/**
 * Supports IEEE 802.15.4 MAC frame generation for transmission.
 */
class TxFrame : public Frame, public Radio::TxFrameProperties<TxFrame>
{
public:
    /**
     * Represents parsed information from a transmitted MAC frame.
     */
    class ParseInfo : public Frame::ParseInfo
    {
    public:
        /**
         * Returns a pointer to the associated `TxFrame`.
         *
         * @returns A pointer to the `TxFrame`.
         */
        const TxFrame *GetTxFrame(void) const { return static_cast<const TxFrame *>(mFrame); }

        /**
         * Returns a pointer to the associated `TxFrame`.
         *
         * @returns A pointer to the `TxFrame`.
         */
        TxFrame *GetTxFrame(void) { return AsNonConst(AsConst(this)->GetTxFrame()); }

        /**
         * Writes the Sequence Number value in the frame.
         *
         * The Address fields MUST be parsed successfully before calling this method. If the Sequence Number is not
         * present in the frame, this method performs no action.
         *
         * @param[in] aSequenceNum  The Sequence Number value.
         */
        void WriteSequenceNum(uint8_t aSequenceNum);

        /**
         * Writes the Key Index (sub-field of Key ID) in the frame.
         *
         * If the Auxiliary Security Header is not parsed or not present, or if the Key ID Mode is `kKeyIdMode0`, this
         * method performs no action.
         *
         * @param[in] aKeyIndex  The Key Index.
         */
        void WriteKeyIndex(uint8_t aKeyIndex);

        /**
         * Writes the security Frame Counter in the frame.
         *
         * If the Auxiliary Security Header is not parsed or not present, this method performs no action. Otherwise,
         * it writes the Frame Counter and marks the frame header as updated (`SetIsHeaderUpdated(true)`).
         *
         * @param[in] aFrameCounter  The Frame Counter.
         */
        void WriteFrameCounter(uint32_t aFrameCounter);

        /**
         * Writes the Key Source in the frame.
         *
         * If the Auxiliary Security Header is not parsed or not present, or if the Key ID Mode does not require a Key
         * Source, this method performs no action.
         *
         * @param[in] aKeySource  A pointer to the Key Source value.
         */
        void WriteKeySource(const uint8_t *aKeySource);

        /**
         * Performs AES-CCM encryption on the frame to be transmitted.
         *
         * The frame MUST be fully parsed before calling this method. If security is enabled on the frame,
         * this method encrypts the payload, appends the MIC tag, and marks security as processed
         * (`SetIsSecurityProcessed(true)`). If security is not enabled, this method performs no action.
         *
         * @param[in] aExtAddress  A reference to the extended address used to generate the AES-CCM nonce.
         */
        void ProcessTransmitAesCcm(const ExtAddress &aExtAddress);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE
        /**
         * Restores transmit security by decrypting the frame for retransmission.
         *
         * The frame MUST be fully parsed before calling this method. If security is enabled and was previously
         * processed, this method decrypts the frame in-place using AES-CCM and resets both the security-processed
         * and header-updated flags (`SetIsSecurityProcessed(false)` and `SetIsHeaderUpdated(false)`).
         *
         * @param[in] aExtAddress  A reference to the extended address used to generate the AES-CCM nonce.
         */
        void RestoreTransmitSecurity(const ExtAddress &aExtAddress);
#endif
    };

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

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
    /**
     * Generate IEEE 802.15.4 Wake-up frame.
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
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_FRAME_HPP_
