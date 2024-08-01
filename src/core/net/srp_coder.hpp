/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef SRP_CODER_HPP_
#define SRP_CODER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE

#include <openthread/srp_client.h>
#include <openthread/platform/crypto.h>

#include "common/array.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/owned_ptr.hpp"
#include "common/string.hpp"
#include "crypto/ecdsa.hpp"
#include "net/dns_types.hpp"
#include "net/udp6.hpp"

/**
 * @file
 *   This file includes definitions for the SRP Coder used to encode/decode SRP Update messages.
 *
 */

namespace ot {
namespace Srp {

class UnitTester;

/**
 * Implements SRP Coder functionality.
 *
 * The SRP Coder can be used to encode an SRP message into a compact, compressed format, reducing the message size. The
 * received coded message can be decoded (on server) to reconstruct an exact replica of the original SRP message.
 *
 */
class Coder : public InstanceLocator
{
    friend class UnitTester;

    static constexpr uint16_t kMaxSavedOffsets = 16;

    typedef Array<OffsetRange, kMaxSavedOffsets> OffsetRangeArray;
    typedef Array<uint16_t, kMaxSavedOffsets>    OffsetArray;

public:
    /**
     * Initializes the SRP `Coder` instance
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Coder(Instance &aInstance);

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

    /**
     * Implements SRP message encoder.
     *
     * SRP client uses `MsgEncoder` to construct a coded SRP update message.
     *
     */
    class MsgEncoder
    {
    public:
        typedef otSrpClientService ClientService; ///< An SRP client service.

        /**
         * `MsgEncoder` constructor.
         *
         */
        MsgEncoder(void);

        /**
         * Allocates a message.
         *
         * `MsgEncoder` manages the lifetime of the allocated message. It transfers ownership of the message if it's
         * successfully sent using `SendMessage()`. Otherwise, the message is freed when the `MsgEncoder` object is
         * destroyed.
         *
         * @param[in] aSocket   The UDP socket to use for message allocation.
         *
         * @retval kErrorNone     A new message was allocated successfully.
         * @retval kErrorNotBufs  Failed to allocate new message.
         *
         */
        Error AllocateMessage(Ip6::Udp::Socket &aSocket);

        /**
         * Indicates whether or not a message is allocated (whether `MsgEncoder` is in-use).
         *
         * @retval TRUE  `MsgEncoder` has a message and is in-use.
         * @retval FALSE `MsgEncoder` has no message and is not in-use.
         *
         */
        bool HasMessage(void) const { return (mMessage != nullptr); }

        /**
         * Returns a pointer to the message.
         *
         * @returns A pointer to message, or `nullptr` if it has no message.
         *
         */
        const Message *GetMessage(void) const { return mMessage.Get(); }

        /**
         * Initializes the `MsgEncoder`, clearing any previously constructed message and state.
         *
         * This method can be called to re-initialize `MsgEncoder` starting over the construction of a coded message.
         *
         */
        void Init(void);

        /**
         * Encodes header block.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aMessageId     The SRP message ID.
         * @param[in] aDomainName    The domain name.
         * @param[in] aDefaulttl     The default TTL value.
         * @param[in] aHostName      The host name labels.
         *
         * @retval kErrorNone         Successfully encoded the header block, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes.
         * @retval kErrorInvalidArgs  Host name or domain name is not valid.
         *
         */
        Error EncodeHeaderBlock(uint16_t    aMessageId,
                                const char *aDomainName,
                                uint32_t    aDefaultTtl,
                                const char *aHostName);

        /**
         * Encodes service block.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aService       The SRP client's service to encode.
         * @param[in] aRemoving      Boolean to indicate whether service is being removed or added.
         *
         * @retval kErrorNone         Successfully encoded the service, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes
         * @retval kErrorInvalidArgs  @p aService is not valid.
         *
         */
        Error EncodeServiceBlock(const ClientService &aService, bool aRemoving);

        /**
         * Encodes host block dispatch byte.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aHasAnyAddress    Boolean to indicate whether host has any addresses.
         *
         * @retval kErrorNone         Successfully encoded the host dispatch byte, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes.
         *
         */
        Error EncodeHostDispatch(bool aHasAnyAddress);

        /**
         * Encodes a host address.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aAddress  The IPv6 address to encoder
         * @param[in] aHasMore  Boolean to indicate whether there are more addresses after this.
         *
         * @retval kErrorNone         Successfully encoded the host address, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes.
         *
         */
        Error EncodeHostAddress(const Ip6::Address &aAddress, bool aHasMore);

        /**
         * Encodes a host key.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aKey      The key to encode.
         *
         * @retval kErrorNone         Successfully encoded the host key, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes.
         *
         */
        Error EncodeHostKey(const Crypto::Ecdsa::P256::PublicKey &aKey);

        /**
         * Encodes the footer block.
         *
         * If the `MsgEncoder` is not in use (`!HasMessage()`), this method does nothing and  returns `kErrorNone`.
         *
         * @param[in] aLease     The lease interval.
         * @param[in] aKeyLease  The key lease interval.
         * @param[in] aSignature The signature to append.
         *
         * @retval kErrorNone         Successfully encoded the footer block, or `MsgEncoder` is not in use.
         * @retval kErrorNoBufs       Could not grow the message to append the encoded bytes.
         *
         */
        Error EncodeFooterBlock(uint32_t aLease, uint32_t aKeyLease, const Crypto::Ecdsa::P256::Signature &aSignature);

        /**
         * Sends the prepared coded message.
         *
         * @param[in] aSocket  The UDP socket to use for tx of encoded message.
         *
         * @retval kErrorNone      Successfully send the prepared message.
         * @retval kErrorNotFound  No coded message to send.
         * @retval kErrorNoBufs    Insufficient available buffer to add the UDP and IPv6 headers.
         *
         */
        Error SendMessage(Ip6::Udp::Socket &aSocket);

    private:
        Error EncodeCompactUint(uint32_t aUint);
        Error EncodeCompactUint(uint32_t aUint, uint8_t aFirstSegBitLength, uint8_t aFirstSegValue);
        void  SaveLabelsOffsetRange(void);
        void  ExtendedCurrLabelsOffsetRange(void);
        Error EncodeName(const char *aName);
        Error EncodeLabel(const char *aLabel);

        OwnedPtr<Message> mMessage;
        OffsetRange      *mCurrLabelsOffsetRange;
        OffsetRangeArray  mSavedLabelOffsetRanges;
        OffsetArray       mSavedTxtDataOffsets;
    };
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

    /**
     * Checks whether a given SRP message is encoded.
     *
     * @param[in] aMessage  The message to check.
     *
     * @retval TRUE   The @p aMessage is an encoded message.
     * @retval FALSE  The @p aMessage is not an encoded message.
     *
     */
    bool IsEncoded(const Message &aMessage) const;

    /**
     * Checks whether a given SRP message is encoded.
     *
     * @param[in] aBuffer  A buffer containing the message.
     * @param[in] aLength  Number of bytes in @p aBuffer.
     *
     * @retval TRUE   The message is an encoded message.
     * @retval FALSE  The message is not an encoded message.
     *
     */
    bool IsEncoded(const uint8_t *aBuffer, uint16_t aLength) const;

    /**
     * Decodes an encoded message, reconstructing the original message.
     *
     * @param[in]  aCodedMessage    The encoded message to decode.
     * @param[out] aMessage         A reference to a `Message` where the decoded message will be written.
     *
     * @retval kErrorNone           Successfully decoded @p aCodedMessage. @p aMessage is updated.
     * @retval kErrorInvalidArgs    The @p aCodedMessage is not an encoded message.
     * @retval kErrorParse          Failed to parse @p aCodedMessage.
     * @retval kErrorNoBufs         Could not allocate buffer to construct the decoded message.
     *
     */
    Error Decode(const Message &aCodedMsg, Message &aMessage);

    /**
     * Decodes an encoded message, reconstructing the original message.
     *
     * Upon successful decoding, this method allocates and returns a new `Message` instance. The caller takes ownership
     *  of this allocated `Message` and is responsible for freeing it when it is no longer needed.
     *
     * @param[in]  aBuffer  A buffer containing the encoded message to decode.
     * @param[in]  aLength  The length of the encoded message (in bytes) within @p aBuffer.
     * @param[out] aError   A pointer to an `Error` to return the decoding result. Can be `nullptr` if not needed.
     *                      Possible errors are the same as those returned by the previous
     *                      `Decode(const Message &aCodedMsg, Message &aMessage)` method.
     *
     * @returns A pointer to the decoded `Message` (ownership transferred to the caller), or `nullptr` if an error
     *          occurs during decoding.
     *
     */
    Message *Decode(const uint8_t *aBuffer, uint16_t aLength, Error *aError);

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    /**
     * Represents a callback function for reporting the outcome of the `Decode()` method call.
     *
     * This is intended for testing only.
     *
     * @param[in] aCodedMsg   The original encoded message that was decoded.
     * @param[in] aMessage    The resulting decoded message, if successful.
     * @param[in] aError      The outcome (error code) of the `Decode()` call.
     *
     */
    typedef void (*DecodeCallback)(const Message &aCodedMsg, const Message &aMessage, Error aError);

    /**
     * Registers a callback function to be invoked after any `Decode()` call.
     *
     * This is intended for testing only.
     *
     * @param[in] aCallback    The callback function to register.
     *
     */
    void SetDecodeCallback(DecodeCallback aCallback) { mDecodeCallback = aCallback; }
#endif

private:
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Compact Uint

    static constexpr uint8_t kCompactUintValueMask           = 0x7f;
    static constexpr uint8_t kCompactUintContinuationFlag    = (1 << 7);
    static constexpr uint8_t kCompactUintBitsPerByteSegement = 7;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // DNS label dispatch byte

    static constexpr uint8_t kLabelDispatchTypeMask = (0x3 << 6); // 0b1100_0000

    static constexpr uint8_t kLabelDispatchNormal       = (0 << 6);
    static constexpr uint8_t kLabelDispatchService      = (1 << 6);
    static constexpr uint8_t kLabelDispatchReferOffset  = (2 << 6);
    static constexpr uint8_t kLabelDispatchCommonlyUsed = (3 << 6);

    static constexpr uint8_t kLabelDispatchGenerativeFlag = (1 << 5);

    static constexpr uint8_t kLabelDispatchLengthMask = 0x3f; // 0b0011_1111
    static constexpr uint8_t kLabelDispatchOffsetMask = 0x3f; // 0b0011_1111
    static constexpr uint8_t kLabelDispatchCodeMask   = 0x1f; // 0b0001_1111

    static constexpr uint8_t kLabelDispatchOffsetFirstSegBitLength = 6;

    enum LabelGenerativeCode : uint8_t
    {
        kLabelGenerativeHexString           = 0, // 16-char hex string using capital letters.
        kLabelGenerativeTwoHexStrings       = 1, // Two 16-char hex strings separated by hyphen '-'.
        kLabelGenerativeCharHexString       = 2, // "_<ch><hhhh>" with 16-char hex string.
        kLabelGenerativeCharHexStringOffset = 3, // "_<ch><hhhh>" with 16-char hex string, offset referral to value.
    };

    static constexpr uint8_t kHexValueSize = 8;

    typedef uint8_t HexValue[kHexValueSize];

    struct LabelGenerativeInfo
    {
        LabelGenerativeCode mCode;
        HexValue            mFirstHexValue;
        HexValue            mSecondHexValue;
        char                mChar;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Header dispatch

    static constexpr uint8_t kHeaderDispatchCodeMask = 0xfc; // 0b1111_1100
    static constexpr uint8_t kHeaderDispatchCode     = 0x2c; // 0b0010_1100
    static constexpr uint8_t kHeaderDispatchZoneFlag = (1 << 1);
    static constexpr uint8_t kHeaderDispatchTtlFlag  = (1 << 0);

    static constexpr uint32_t kDefaultTtl      = 7200;
    static constexpr uint32_t kDefaultLease    = 7200;
    static constexpr uint32_t kDefaultKeyLease = 1209600;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Dispatch type

    static constexpr uint8_t kDispatchTypeMask = (3 << 6);

    static constexpr uint8_t kDispatchAddServiceType    = (0 << 6);
    static constexpr uint8_t kDispatchRemoveServiceType = (1 << 6);
    static constexpr uint8_t kDispatchHostType          = (2 << 6);
    static constexpr uint8_t kDispatchFooterType        = (3 << 6);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add service dispatch byte

    static constexpr uint8_t kAddServiceDispatchPtrTtlFlag    = (1 << 5);
    static constexpr uint8_t kAddServiceDispatchSrvTxtTtlFlag = (1 << 4);
    static constexpr uint8_t kAddServiceDispatchSubTypeFlag   = (1 << 3);
    static constexpr uint8_t kAddServiceDispatchPriorityFlag  = (1 << 2);
    static constexpr uint8_t kAddServiceDispatchWeightFlag    = (1 << 1);
    static constexpr uint8_t kAddServiceDispatchTxtDataFlag   = (1 << 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // TXT data dispatch

    static constexpr uint8_t kTxtDataDispatchReferFlag            = (1 << 7);
    static constexpr uint8_t kTxtDataDispatchSizeSegmentBitLength = 7;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Host dispatch byte

    static constexpr uint8_t kHostDispatchAddrTtlFlag  = (1 << 5);
    static constexpr uint8_t kHostDispatchAddrListFlag = (1 << 4);
    static constexpr uint8_t kHostDispatchKeyTtlFlag   = (1 << 3);
    static constexpr uint8_t kHostDispatchKeyFlag      = (1 << 2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Address dispatch byte

    static constexpr uint8_t kAddrDispatchContextFlag   = (1 << 7);
    static constexpr uint8_t kAddrDispatchMoreFlag      = (1 << 6);
    static constexpr uint8_t kAddrDispatchContextIdMask = 15;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Footer (lease and signature) dispatch byte

    static constexpr uint8_t kFooterDispatchLeaseFlag    = (1 << 4);
    static constexpr uint8_t kFooterDispatchKeyLeaseFlag = (1 << 3);
    static constexpr uint8_t kFooterDispatchSignMask     = (3 << 0);

    static constexpr uint8_t kFooterDispatchSignElided = (0 << 0);
    static constexpr uint8_t kFooterDispatchSign64     = (1 << 0);
    static constexpr uint8_t kFooterDispatchSignShort  = (2 << 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    static constexpr uint8_t kEcdsaKeySize       = OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE;
    static constexpr uint8_t kEcdsaSignatureSize = OT_CRYPTO_ECDSA_SIGNATURE_SIZE;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    using LabelBuffer = Dns::Name::LabelBuffer;
    using NameBuffer  = Dns::Name::Buffer;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    OT_TOOL_PACKED_BEGIN
    class Header
    {
    public:
        uint16_t GetMessageId(void) const { return BigEndian::HostSwap16(mMessageId); }
        void     SetMessageId(uint16_t aMessageId) { mMessageId = BigEndian::HostSwap16(aMessageId); }
        uint8_t  GetDispatch(void) const { return mDispatch; }
        void     SetDispatch(uint8_t aDispatch) { mDispatch = aDispatch; }
        bool     IsValid(void) const { return (mDispatch & kHeaderDispatchCodeMask) == kHeaderDispatchCode; }

    private:
        uint16_t mMessageId;
        uint8_t  mDispatch;
    } OT_TOOL_PACKED_END;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    class MsgDecoder
    {
    public:
        MsgDecoder(const Message &aCodedMsg, Message &aMessage);
        Error Decode(void);

    private:
        static constexpr uint16_t kUdpPayloadSize = Ip6::kMaxDatagramLength - sizeof(Ip6::Udp::Header);
        static constexpr uint16_t kUnknownOffset  = 0;

        Error DecodeHeaderBlock(void);
        Error DecodeServiceBlock(uint8_t aDispatch);
        Error DecodeHostBlock(uint8_t aDispatch);
        Error DecodeFooterBlock(void);

        Error DecodeLabel(LabelBuffer &aLabel);
        Error DecodeName(NameBuffer &aName);
        Error DecodeUint32(uint32_t &aUint32);
        Error DecodeUint16(uint16_t &aUint16, uint8_t aFirstSegBitLength = kBitsPerByte);
        void  UpdateHeaderRecordCounts(void);
        void  UpdateRecordLengthInMessage(Dns::ResourceRecord &aRecord, uint16_t aOffset);
        Error AppendDeleteAllRrsets(void);
        Error AppendHostName(void);

        const Message &mCodedMsg;
        Message       &mMessage;
        OffsetRange    mOffsetRange;
        NameBuffer     mHostName;
        uint32_t       mDefaultTtl;
        uint16_t       mUpdateRecordCount;
        uint16_t       mAddnlRecordCount;
        uint16_t       mDomainNameOffset;
        uint16_t       mHostNameOffset;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    static Error AppendCompactUint(Message &aMessage,
                                   uint32_t aUint,
                                   uint8_t  aFirstSegBitLength = kBitsPerByte,
                                   uint8_t  aFirstSegValue     = 0);
    static Error ReadCompactUint(const Message &aMessage, OffsetRange &aOffsetRange, uint32_t &aUint);
    static Error ReadCompactUint(const Message &aMessage,
                                 OffsetRange   &aOffsetRange,
                                 uint32_t      &aUint,
                                 uint8_t        aFirstSegBitLength);
    static Error AppendName(Message &aMessage, const char *aName, const OffsetRangeArray &aPrevLabelOffsetRanges);
    static Error AppendLabel(Message &aMessage, const char *aLabel, const OffsetRangeArray &aPrevLabelOffsetRanges);
    static Error ReadName(const Message &aMessage, OffsetRange &aOffsetRange, NameBuffer &aName);
    static Error ReadLabel(const Message &aMessage, OffsetRange &aOffsetRange, LabelBuffer &aLabel);
    static Error ReadAndAppendHexValueToLabel(const Message &aMessage,
                                              OffsetRange   &aOffsetRange,
                                              StringWriter  &aLabelWriter);
    static Error ReadReferOffset(const Message &aMessage,
                                 OffsetRange   &aOffsetRange,
                                 OffsetRange   &aReferOffsetRange,
                                 uint8_t        aFirstSegBitLength = kBitsPerByte);
    static bool  CanEncodeAsCommonlyUsedLabel(const char *aLabel, uint8_t &aCode);
    static bool  CanEncodeAsGenerativeLabel(const char *aLabel, LabelGenerativeInfo &aInfo);
    static Error ReadHexValue(const char *&aLabel, HexValue &aHexValue);
    static Error ReadHexDigit(const char *&aLabel, uint8_t &aDigit);

    static const char        kDefaultDomainName[];
    static const char *const kCommonlyUsedLabels[];

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    DecodeCallback mDecodeCallback;
#endif
};

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CODER_ENABLE

#endif // SRP_CODER_HPP_
