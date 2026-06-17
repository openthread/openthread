/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for maintaining vendor information (name, model, etc).
 */

#ifndef OT_CORE_THREAD_VENDOR_INFO_HPP_
#define OT_CORE_THREAD_VENDOR_INFO_HPP_

#include "openthread-core-config.h"

#include <openthread/netdiag.h>

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/string.hpp"
#include "thread/net_diag_tlvs.hpp"

namespace ot {

class UnitTester;

/**
 * Represents the vendor information.
 */
class VendorInfo : public InstanceLocator, private NonCopyable
{
    friend class UnitTester;

public:
    /**
     * Represents a Thread Vendor OUI (Organizationally Unique Identifier).
     */
    class Oui : public otThreadVendorOui, public Clearable<Oui>, public Unequatable<Oui>
    {
        friend class UnitTester;

    public:
        /**
         * The minimum size of the OUI in bytes.
         */
        static constexpr uint8_t kMinSize = 3;

        /**
         * The maximum size of the OUI in bytes.
         */
        static constexpr uint8_t kMaxSize = OT_THREAD_VENDOR_OUI_MAX_SIZE;

        /**
         * The unspecified Vendor OUI value.
         */
        static constexpr uint32_t kUnspecified = OT_THREAD_UNSPECIFIED_VENDOR_OUI;

        /**
         * The size of `InfoString` (used in `ToString()`).
         */
        static constexpr uint16_t kInfoStringSize = OT_THREAD_VENDOR_OUI_STRING_SIZE;

        /**
         * Defines the fixed-length `String` returned from `ToString()`.
         */
        typedef String<kInfoStringSize> InfoString;

        /**
         * Indicates whether or not the OUI value is valid.
         *
         * @retval TRUE   The OUI is valid.
         * @retval FALSE  The OUI is not valid.
         */
        bool IsValid(void) const;

        /**
         * Returns the OUI prefix length in bits (24, 28, or 36)
         *
         * @returns The OUI prefix length in bits.
         */
        uint8_t GetBitLength(void) const { return mBitLength; }

        /**
         * Returns a pointer to the OUI bytes.
         *
         * @returns A pointer to the OUI bytes.
         */
        const uint8_t *GetBytes(void) const { return mBytes; }

        /**
         * Returns the OUI size in bytes.
         *
         * @returns The OUI size in bytes.
         */
        uint8_t GetSize(void) const { return BytesForBitSize(mBitLength); }

        /**
         * Sets the OUI from another OUI, tidying unused bits.
         *
         * @param[in] aOther  The other OUI to set from.
         */
        void SetFrom(const Oui &aOther);

        /**
         * Sets the OUI from a byte buffer.
         *
         * @param[in] aData  A pointer to the buffer.
         * @param[in] aSize  The size of the buffer in bytes.
         *
         * @retval kErrorNone         Successfully set the OUI.
         * @retval kErrorInvalidArgs  The @p aSize is not valid and not within valid range [kMinSize, kMaxSize].
         */
        Error SetFrom(const uint8_t *aData, uint16_t aSize);

        /**
         * Parses the OUI from a message.
         *
         * If @p aOffsetRange is longer than `kMaxSize`, the additional bytes beyond `kMaxSize` are ignored.
         *
         * @param[in] aMessage       The message to read from.
         * @param[in] aOffsetRange   The offset range in @p aMessage to read from.
         *
         * @retval kErrorNone   Successfully parsed the OUI from @p aMessage.
         * @retval kErrorParse  Failed to parse the OUI from @p aMessage.
         */
        Error ParseFrom(const Message &aMessage, OffsetRange aOffsetRange);

        /**
         * Returns the OUI as a 24-bit OUI value.
         *
         * @deprecated This method is deprecated.
         *
         * If the configured Vendor OUI has a prefix length greater than 24 bits, this method returns the most
         * significant 24 bits (first 3 bytes) of the OUI.
         *
         * @returns The 24-bit OUI value, or `kUnspecified` if it is not valid.
         */
        uint32_t GetAsOui24(void) const;

        /**
         * Overloads operator== to check for equality with another OUI.
         *
         * @param[in] aOther  The other OUI to compare with.
         *
         * @retval TRUE   The OUIs are equal.
         * @retval FALSE  The OUIs are not equal.
         */
        bool operator==(const Oui &aOther) const;

        /**
         * Converts the OUI to a human-readable string
         *
         * The generated string format is hyphen-separated uppercase hexadecimal bytes (e.g., "00-1A-2B" for a 24-bit
         * OUI). For 28-bit and 36-bit OUIs, the trailing 4-bit nibble is appended as a single hexadecimal digit
         * (e.g., "00-1A-2B-3" for a 28-bit OUI). If @p aOui is invalid, the string "unspecified" is returned.
         *
         * @returns An `InfoString` containing the string representation of the OUI.
         */
        InfoString ToString(void) const;

        /**
         * Converts the OUI to a human-readable string.
         *
         * The generated string format is hyphen-separated uppercase hexadecimal bytes (e.g., "00-1A-2B" for a 24-bit
         * OUI). For 28-bit and 36-bit OUIs, the trailing 4-bit nibble is appended as a single hexadecimal digit
         * (e.g., "00-1A-2B-3" for a 28-bit OUI). If @p aOui is invalid, the string "unspecified" is returned.
         *
         * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be
         * truncated but the outputted string is always null-terminated.
         *
         * @param[out] aBuffer  A pointer to a character buffer to output the string.
         * @param[in]  aSize    The size of @p aBuffer in bytes.
         */
        void ToString(char *aBuffer, uint16_t aSize) const;

    private:
        static constexpr uint8_t kTopNibbleMask = 0xf0;

        static const uint8_t kValidBitLengths[];

        Error SetBitLengthFromSize(uint16_t aSize);
        void  Tidy(void);
        void  ToString(StringWriter &aWriter) const;
    };

    /**
     * Initializes the `VendorInfo`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit VendorInfo(Instance &aInstance);

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    /**
     * Returns the vendor name string.
     *
     * @returns The vendor name string.
     */
    const char *GetName(void) const { return mName; }

    /**
     * Sets the vendor name string.
     *
     * @param[in] aName     The vendor name string.
     *
     * If `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled, @p aName must start with the "RD:" prefix.
     * This is enforced to ensure reference devices are identifiable. If @p aName does not follow this pattern,
     * the name is rejected, and `kErrorInvalidArgs` is returned.
     *
     * @retval kErrorNone         Successfully set the vendor name.
     * @retval kErrorInvalidArgs  @p aName is not valid. It is too long, is not UTF-8, or does not start with
     *                            the "RD:" prefix when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     */
    Error SetName(const char *aName);

    /**
     * Returns the vendor model string.
     *
     * @returns The vendor model string.
     */
    const char *GetModel(void) const { return mModel; }

    /**
     * Sets the vendor model string.
     *
     * @param[in] aModel     The vendor model string.
     *
     * @retval kErrorNone         Successfully set the vendor model.
     * @retval kErrorInvalidArgs  @p aModel is not valid (too long or not UTF8).
     */
    Error SetModel(const char *aModel);

    /**
     * Returns the vendor software version string.
     *
     * @returns The vendor software version string.
     */
    const char *GetSwVersion(void) const { return mSwVersion; }

    /**
     * Sets the vendor sw version string
     *
     * @param[in] aSwVersion     The vendor sw version string.
     *
     * @retval kErrorNone         Successfully set the vendor sw version.
     * @retval kErrorInvalidArgs  @p aSwVersion is not valid (too long or not UTF8).
     */
    Error SetSwVersion(const char *aSwVersion);

    /**
     * Returns the vendor app URL string.
     *
     * @returns the vendor app URL string.
     */
    const char *GetAppUrl(void) const { return mAppUrl; }

    /**
     * Sets the vendor app URL string.
     *
     * @param[in] aAppUrl     The vendor app URL string
     *
     * @retval kErrorNone         Successfully set the vendor app URL.
     * @retval kErrorInvalidArgs  @p aAppUrl is not valid (too long or not UTF8).
     */
    Error SetAppUrl(const char *aAppUrl);

    /**
     * Returns the Vendor OUI.
     *
     * If the Vendor OUI is not set, the returned `Oui` object's `IsValid()` will return `false`.
     *
     * @returns The Vendor OUI.
     */
    const Oui &GetOui(void) const { return mOui; }

    /**
     * Sets the Vendor OUI.
     *
     * @param[in] aOui  The Vendor OUI.
     *
     * @retval kErrorNone         Successfully set the Vendor OUI.
     * @retval kErrorInvalidArgs  @p aOui is not valid.
     */
    Error SetOui(const Oui &aOui);

#else
    const char *GetName(void) const { return kName; }
    const char *GetModel(void) const { return kModel; }
    const char *GetSwVersion(void) const { return kSwVersion; }
    const char *GetAppUrl(void) const { return kAppUrl; }
    const Oui  &GetOui(void) const { return static_cast<const Oui &>(kOui); }
#endif // OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

private:
    // String buffer types (max size specified by corresponding TLV)
    typedef NetDiag::VendorNameTlv::StringType      NameStringType;
    typedef NetDiag::VendorModelTlv::StringType     ModelStringType;
    typedef NetDiag::VendorSwVersionTlv::StringType SwVersionStringType;
    typedef NetDiag::VendorAppUrlTlv::StringType    AppUrlStringType;

    template <uint64_t kOuiConfig> class OuiParser
    {
        // Helper template class to parse and validate the compile-time configured OUI.
        // It supports both the legacy 24-bit OUI (implicit length of 24) and the new 48-bit
        // format (explicit length in the high byte) to maintain backward compatibility.

    public:
        static constexpr bool IsSpecified(void) { return (kOuiConfig != VendorInfo::Oui::kUnspecified); }

        static constexpr uint8_t GetBitLength(void)
        {
            return !IsSpecified() ? 0 : (IsShort24BitLayout() ? 24 : ReadBits(kLenMask));
        }

        static constexpr bool IsValid(void)
        {
            return !IsSpecified() || IsShort24BitLayout() || Is24Bit() || Is28Bit() || Is36Bit();
        }

        static constexpr uint8_t GetByte0(void)
        {
            return !IsSpecified() ? 0 : IsShort24BitLayout() ? ReadBits(kShortLayoutByte0Mask) : ReadBits(kByte0Mask);
        }

        static constexpr uint8_t GetByte1(void)
        {
            return !IsSpecified() ? 0 : IsShort24BitLayout() ? ReadBits(kShortLayoutByte1Mask) : ReadBits(kByte1Mask);
        }

        static constexpr uint8_t GetByte2(void)
        {
            return !IsSpecified() ? 0 : IsShort24BitLayout() ? ReadBits(kShortLayoutByte2Mask) : ReadBits(kByte2Mask);
        }

        static constexpr uint8_t GetByte3(void)
        {
            return !IsSpecified() ? 0 : IsShort24BitLayout() ? 0 : ReadBits(kByte3Mask);
        }

        static constexpr uint8_t GetByte4(void)
        {
            return !IsSpecified() ? 0 : IsShort24BitLayout() ? 0 : ReadBits(kByte4Mask);
        }

    private:
        static constexpr uint64_t kMaxUint48            = 0xffffffffffffULL; // 6 bytes
        static constexpr uint64_t kLenMask              = 0xff0000000000ULL;
        static constexpr uint64_t kByte0Mask            = 0x00ff00000000ULL;
        static constexpr uint64_t kByte1Mask            = 0x0000ff000000ULL;
        static constexpr uint64_t kByte2Mask            = 0x000000ff0000ULL;
        static constexpr uint64_t kByte3Mask            = 0x00000000ff00ULL;
        static constexpr uint64_t kByte4Mask            = 0x0000000000ffULL;
        static constexpr uint64_t kMaxUint24            = 0x000000ffffffULL; // 3 bytes
        static constexpr uint64_t kShortLayoutByte0Mask = 0x000000ff0000ULL;
        static constexpr uint64_t kShortLayoutByte1Mask = 0x00000000ff00ULL;
        static constexpr uint64_t kShortLayoutByte2Mask = 0x0000000000ffULL;

        static constexpr bool IsShort24BitLayout(void) { return (kOuiConfig <= kMaxUint24); }
        static constexpr bool IsConfigValueValid(void) { return (kOuiConfig <= kMaxUint48); }

        static constexpr bool Is24Bit(void)
        {
            return IsConfigValueValid() && (GetBitLength() == 24) && (GetByte3() == 0) && (GetByte4() == 0);
        }

        static constexpr bool Is28Bit(void)
        {
            return IsConfigValueValid() && GetBitLength() == 28 && ((GetByte3() & 0x0f) == 0) && (GetByte4() == 0);
        }

        static constexpr bool Is36Bit(void)
        {
            return IsConfigValueValid() && GetBitLength() == 36 && ((GetByte4() & 0x0f) == 0);
        }

        static constexpr uint8_t ReadBits(uint64_t aMask)
        {
            return static_cast<uint8_t>((kOuiConfig & aMask) >> BitOffsetOfMask(aMask));
        }
    };

    static const char              kName[];
    static const char              kModel[];
    static const char              kSwVersion[];
    static const char              kAppUrl[];
    static const otThreadVendorOui kOui;

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    NameStringType      mName;
    ModelStringType     mModel;
    SwVersionStringType mSwVersion;
    AppUrlStringType    mAppUrl;
    Oui                 mOui;
#endif
};

DefineCoreType(otThreadVendorOui, VendorInfo::Oui);

} // namespace ot

#endif // OT_CORE_THREAD_VENDOR_INFO_HPP_
