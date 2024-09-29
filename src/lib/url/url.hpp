/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#ifndef OT_LIB_URL_URL_HPP_
#define OT_LIB_URL_URL_HPP_

#include <stdint.h>
#include <openthread/error.h>

/**
 * @struct otUrl
 *
 * Represents a URL.
 */
struct otUrl
{
    const char *mProtocol; ///< The URL protocol.
    const char *mPath;     ///< The path.
    const char *mQuery;    ///< The start of the URL query string.
    const char *mEnd;      ///< The end of the URL buffer.
};

namespace ot {
namespace Url {

/**
 * Implements the URL processing.
 */
class Url : public otUrl
{
public:
    Url(void);

    /**
     * Initializes the URL.
     *
     * @param[in]   aUrl    A buffer stores the null-terminated URL string.
     *
     * @retval  OT_ERROR_NONE   Successfully parsed the URL.
     * @retval  OT_ERROR_PARSE  Failed to parse the string as a URL.
     */
    otError Init(char *aUrl);

    /**
     * Gets the path in URL.
     *
     * @returns The path in URL.
     */
    const char *GetPath(void) const { return mPath; }

    /**
     * Gets the value of parameter @p aName.
     *
     * @param[in] aName       The parameter name.
     * @param[in] aLastValue  The last iterated parameter value, nullptr for the first value.
     *
     * @returns The parameter value.
     */
    const char *GetValue(const char *aName, const char *aLastValue = nullptr) const;

    /**
     * Returns the URL protocol.
     *
     * @returns The URL protocol.
     */
    const char *GetProtocol(void) const { return mProtocol; }

    /**
     * Indicates whether or not the url contains the parameter.
     *
     * @param[in]  aName  A pointer to the parameter name.
     *
     * @retval TRUE   The url contains the parameter.
     * @retval FALSE  The url doesn't support the parameter.
     */
    bool HasParam(const char *aName) const { return (GetValue(aName) != nullptr); }

    /**
     * Parses a `uint32_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `uint32_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseUint32(const char *aName, uint32_t &aValue) const;

    /**
     * Parses a `uint16_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `uint16_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseUint16(const char *aName, uint16_t &aValue) const;

    /**
     * Parses a `uint8_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `uint16_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseUint8(const char *aName, uint8_t &aValue) const;

    /**
     * Parses a `int32_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string
     * can start with `+`/`-` sign.
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `int32_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseInt32(const char *aName, int32_t &aValue) const;

    /**
     * Parses a `int16_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string
     * can start with `+`/`-` sign.
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `int16_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseInt16(const char *aName, int16_t &aValue) const;

    /**
     * Parses a `int8_t` parameter value.
     *
     * The parameter value in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string
     * can start with `+`/`-` sign.
     *
     * @param[in]  aName    A pointer to the parameter name.
     * @param[out] aValue   A reference to an `int8_t` variable to output the parameter value.
     *                      The original value of @p aValue won't change if failed to get the value.
     *
     * @retval OT_ERROR_NONE          The parameter value was parsed successfully.
     * @retval OT_ERROR_NOT_FOUND     The parameter name was not found.
     * @retval OT_ERROR_INVALID_ARGS  The parameter value was not contain valid number (e.g., value out of range).
     */
    otError ParseInt8(const char *aName, int8_t &aValue) const;
};

} // namespace Url
} // namespace ot

#endif // OT_LIB_URL_URL_HPP_
