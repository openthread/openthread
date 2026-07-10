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
 *   This file includes definitions for generating and processing MAC LTVs.
 */

#ifndef OT_CORE_MAC_MAC_LTVS_HPP_
#define OT_CORE_MAC_MAC_LTVS_HPP_

#include "openthread-core-config.h"

#include "common/array.hpp"
#include "common/bit_utils.hpp"
#include "common/const_cast.hpp"
#include "common/error.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

/**
 * Implements Thread Header IE Length-Type-Value (LTV) processing and encoding.
 */
class Ltv
{
public:
    /**
     * Represents LTV information.
     */
    class Info
    {
    public:
        /**
         * Returns the LTV Type.
         *
         * @returns The LTV Type.
         */
        uint8_t GetType(void) const { return mType; }

        /**
         * Returns the LTV length in bytes.
         *
         * @returns The LTV length in bytes.
         */
        uint8_t GetLength(void) const { return mLength; }

    protected:
        Info(void) = default;

        uint8_t        mType;
        uint8_t        mLength;
        const uint8_t *mValue;
    };

    /**
     * Represents parsed LTV information.
     *
     * All parsing and search methods (`ParseFromAndAdvance()`, `FindIn()`, `FindInAndAdvance()`) automatically handle
     * packed and base (unpacked) LTV formats.
     */
    class ParsedInfo : public Info
    {
    public:
        /**
         * Returns a pointer to the parsed LTV value bytes.
         *
         * @returns A pointer to the LTV value bytes.
         */
        const void *GetValue(void) const { return mValue; }

        /**
         * Returns a pointer to the parsed LTV value cast to a specified `ValueType`.
         *
         * @tparam ValueType  The type to cast the value pointer to.
         *
         * @returns A pointer to the parsed LTV value cast to `const ValueType *`.
         */
        template <typename ValueType> const ValueType *GetValueAs(void) const
        {
            return reinterpret_cast<const ValueType *>(GetValue());
        }

        /**
         * Validates whether the parsed LTV matches a specific `LtvType` structure and validates its value format.
         *
         * @tparam LtvType  The LTV structure type to validate against. `LtvType` MUST define a `uint8_t kType`
         *                  constant and a static `bool IsValid(const ParsedInfo &aInfo)` method.
         *
         * @retval kErrorNone   The LTV matches `LtvType::kType` and `LtvType::IsValid()` returns `true`.
         * @retval kErrorParse  The LTV does not match `LtvType::kType` or `LtvType::IsValid()` returns `false`.
         */
        template <typename LtvType> Error ValidateAs(void) const
        {
            return ((GetType() == LtvType::kType) && LtvType::IsValid(*this)) ? kErrorNone : kErrorParse;
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Parsing or Finding LTVs in a `FrameData`.

        /**
         * Parses an LTV element from a given `FrameData` and updates `aFrameData` to skip over it.
         *
         * @param[in,out] aFrameData  The frame data buffer to parse from.
         *
         * @retval kErrorNone      Successfully parsed the LTV element and advanced @p aFrameData.
         * @retval kErrorNotFound  No LTV element remains in @p aFrameData.
         * @retval kErrorParse     The LTV element is malformed.
         */
        Error ParseFromAndAdvance(FrameData &aFrameData);

        /**
         * Searches a `FrameData` for an LTV element matching a given Type.
         *
         * @param[in] aFrameData  The frame data buffer to search within.
         * @param[in] aType       The LTV Type to search for.
         *
         * @retval kErrorNone      Successfully found an LTV element matching @p aType.
         * @retval kErrorNotFound  No LTV element matching @p aType was found in @p aFrameData.
         * @retval kErrorParse     An LTV element in @p aFrameData is malformed.
         */
        Error FindIn(const FrameData &aFrameData, uint8_t aType);

        /**
         * Searches a `FrameData` for an LTV element matching a given Type and advances `aFrameData` past it.
         *
         * @param[in,out] aFrameData  The frame data buffer to search within and advance.
         * @param[in]     aType       The LTV Type to search for.
         *
         * @retval kErrorNone      Successfully found an LTV element matching @p aType and advanced @p aFrameData.
         * @retval kErrorNotFound  No LTV element matching @p aType was found in @p aFrameData.
         * @retval kErrorParse     An LTV element in @p aFrameData is malformed.
         */
        Error FindInAndAdvance(FrameData &aFrameData, uint8_t aType);

        /**
         * Searches a `FrameData` for an LTV element of a specific `LtvType` and validates it.
         *
         * @tparam LtvType        The LTV structure type to search for and validate against. `LtvType` MUST define a
         *                        `uint8_t kType` constant and a static `bool IsValid(const ParsedInfo &aInfo)` method.
         *
         * @param[in] aFrameData  The frame data buffer to search within.
         *
         * @retval kErrorNone      Successfully found an LTV element matching `LtvType::kType` and validated it.
         * @retval kErrorNotFound  No LTV element matching `LtvType::kType` was found in @p aFrameData.
         * @retval kErrorParse     An LTV element in @p aFrameData is malformed or invalid per `LtvType::IsValid()`.
         */
        template <typename LtvType> Error FindAndValidate(const FrameData &aFrameData)
        {
            Error error;

            SuccessOrExit(error = FindIn(aFrameData, LtvType::kType));
            VerifyOrExit(LtvType::IsValid(*this), error = kErrorParse);

        exit:
            return error;
        }

        /**
         * Searches a `FrameData` for an LTV element of a specific `LtvType`, validates it, and advances `aFrameData`
         * past it.
         *
         * @tparam LtvType            The LTV structure type to search for and validate against. `LtvType` MUST define a
         *                            `uint8_t kType` constant and a static `bool IsValid(const ParsedInfo &aInfo)`
         * method.
         *
         * @param[in,out] aFrameData  The frame data buffer to search within and advance.
         *
         * @retval kErrorNone      Successfully found an LTV element matching `LtvType::kType`, validated it, and
         *                         advanced @p aFrameData.
         * @retval kErrorNotFound  No LTV element matching `LtvType::kType` was found in @p aFrameData.
         * @retval kErrorParse     An LTV element in @p aFrameData is malformed or invalid per `LtvType::IsValid()`.
         */
        template <typename LtvType> Error FindValidateAndAdvance(FrameData &aFrameData)
        {
            Error error;

            SuccessOrExit(error = FindInAndAdvance(aFrameData, LtvType::kType));
            VerifyOrExit(LtvType::IsValid(*this), error = kErrorParse);

        exit:
            return error;
        }
    };

    /**
     * Represents information used to construct and append an LTV element.
     *
     * To encode and append one or more LTVs into a `FrameBuilder`:
     *
     * 1. Prepare an array of `AppendInfo` instances (one per LTV) and initialize each using the `Init...()` methods.
     * 2. Call `EncodeAndAppend()` passing the array of `AppendInfo` entries.
     * 3. `EncodeAndAppend()` determines whether each LTV can be encoded using the packed or base format.
     *    It appends the LTV headers to the `FrameBuilder` and allocates space for their value bytes.
     * 4. After a successful call to `EncodeAndAppend()`, the caller can use `GetValue()` or `GetValueAs()` on each
     *    LTV entry to populate its value bytes in the frame.
     */
    class AppendInfo : public Info
    {
        friend class Ltv;

    public:
        /**
         * Initializes an LTV `AppendInfo` instance with a Type and Length.
         *
         * @param[in] aType    The LTV Type.
         * @param[in] aLength  The LTV length in bytes.
         */
        void InitTypeLength(uint8_t aType, uint8_t aLength) { InitTypeLengthId(aType, aLength, 0); }

        /**
         * Initializes an `AppendInfo` instance with a Type, Length, and an identifier.
         *
         * The @p aId can be used to distinguish between multiple LTV instances of the same Type in an array.
         *
         * @param[in] aType    The LTV Type.
         * @param[in] aLength  The LTV length in bytes.
         * @param[in] aId      A caller-specified identifier.
         */
        void InitTypeLengthId(uint8_t aType, uint8_t aLength, uint16_t aId);

        /**
         * Initializes an `AppendInfo` instance for a specific `LtvType` with a given length and optional identifier.
         *
         * @tparam LtvType   The LTV structure type to initialize for. `LtvType` MUST define a `uint8_t kType` constant.
         *
         * @param[in] aLength The LTV length in bytes.
         * @param[in] aId     An optional caller-specified identifier.
         */
        template <typename LtvType> void InitAsWithLength(uint8_t aLength, uint16_t aId = 0)
        {
            InitTypeLengthId(LtvType::kType, aLength, aId);
        }

        /**
         * Initializes an `AppendInfo` instance for a specific fixed-length `LtvType` with an optional identifier.
         *
         * @tparam LtvType   The LTV structure type to initialize for. `LtvType` MUST define `uint8_t kType` and
         *                   `uint8_t kLength` constants.
         *
         * @param[in] aId     An optional caller-specified identifier.
         */
        template <typename LtvType> void InitAs(uint16_t aId = 0)
        {
            InitTypeLengthId(LtvType::kType, LtvType::kLength, aId);
        }

        /**
         * Returns the optional identifier assigned to this LTV instance.
         *
         * @returns The caller-specified identifier.
         */
        uint16_t GetId(void) const { return mId; }

        /**
         * Returns a pointer to the appended LTV value buffer.
         *
         * The caller can write the LTV value directly into this buffer after a successful call to `EncodeAndAppend()`.
         *
         * @returns A pointer to the LTV value buffer.
         */
        void *GetValue(void) { return AsNonConst(mValue); }

        /**
         * Returns a pointer to the appended LTV value buffer cast to a specified `ValueType`.
         *
         * The caller can update the LTV value using the returned pointer after a successful call to
         * `EncodeAndAppend()`.
         *
         * @tparam ValueType  The type to cast the value pointer to.
         *
         * @returns A pointer to the LTV value buffer cast to `ValueType *`.
         */
        template <typename ValueType> ValueType *GetValueAs(void) { return reinterpret_cast<ValueType *>(GetValue()); }

    private:
        bool CanUsePacked(uint8_t aLenBitSize) const;
        void DetermineIfPackable(uint32_t &aTotalLength);

        uint16_t mId;
        uint8_t  mHeaderByte;
        bool     mIsPackable;
    };

    /**
     * Validates all LTV elements contained in a `FrameData`.
     *
     * @param[in] aFrameData  The frame data buffer containing LTV elements.
     *
     * @retval kErrorNone   All LTV elements in @p aFrameData were successfully parsed and validated.
     * @retval kErrorParse  An LTV element in @p aFrameData is malformed.
     */
    static Error ValidateAllIn(const FrameData &aFrameData);

    /**
     * Encodes and appends a list of LTV elements into a `FrameBuilder`.
     *
     * This method determines whether each LTV can use the packed or base format based on its Type and cumulative
     * remaining length. It appends the LTV headers to @p aBuilder and reserves space for their value bytes.
     *
     * After this method successfully returns, the LTV value bytes in @p aBuilder are allocated but left uninitialized.
     * The caller can populate the value bytes using `GetValue()` or `GetValueAs()` on each entry in @p aLtvList.
     *
     * @param[in,out] aLtvList   An array of `AppendInfo` entries representing the LTVs to append.
     * @param[in]     aNumLtvs   The number of LTV elements in @p aLtvList.
     * @param[in,out] aBuilder   The `FrameBuilder` instance to append to.
     *
     * @retval kErrorNone      Successfully encoded and appended all LTVs.
     * @retval kErrorNoBufs    Insufficient space in @p aBuilder to append the LTV elements.
     */
    static Error EncodeAndAppend(AppendInfo *aLtvList, uint16_t aNumLtvs, FrameBuilder &aBuilder);

private:
    Ltv(void) = delete;
};

//---------------------------------------------------------------------------------------------------------------------

/**
 * Defines Target ID LTV constants and types and helper methods.
 */
struct TargetIdLtv
{
    static constexpr uint8_t kType = 0x01; ///< Target ID LTV Type.

    static constexpr uint8_t kMinLength = 1; ///< Minimum Target ID LTV length in bytes.

    /**
     * Validates whether a parsed LTV is a valid Target ID LTV.
     *
     * @param[in] aLtv  The parsed LTV to validate.
     *
     * @retval TRUE   @p aLtv is a valid Target ID LTV.
     * @retval FALSE  @p aLtv is not a valid Target ID LTV.
     */
    static bool IsValid(const Ltv::Info &aLtv) { return aLtv.GetLength() >= kMinLength; }
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_LTVS_HPP_
