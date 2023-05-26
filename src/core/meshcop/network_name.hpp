/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for managing the Network Name.
 *
 */

#ifndef MESHCOP_NETWORK_NAME_HPP_
#define MESHCOP_NETWORK_NAME_HPP_

#include "openthread-core-config.h"

#include <openthread/dataset.h>

#include "common/as_core_type.hpp"
#include "common/data.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/string.hpp"

namespace ot {
namespace MeshCoP {

/**
 * Represents a name string as data (pointer to a char buffer along with a length).
 *
 * @note The char array does NOT need to be null terminated.
 *
 */
class NameData : private Data<kWithUint8Length>
{
    friend class NetworkName;

public:
    /**
     * Initializes the NameData object.
     *
     * @param[in] aBuffer   A pointer to a `char` buffer (does not need to be null terminated).
     * @param[in] aLength   The length (number of chars) in the buffer.
     *
     */
    NameData(const char *aBuffer, uint8_t aLength) { Init(aBuffer, aLength); }

    /**
     * Returns the pointer to char buffer (not necessarily null terminated).
     *
     * @returns The pointer to the char buffer.
     *
     */
    const char *GetBuffer(void) const { return reinterpret_cast<const char *>(GetBytes()); }

    /**
     * Returns the length (number of chars in buffer).
     *
     * @returns The name length.
     *
     */
    uint8_t GetLength(void) const { return Data<kWithUint8Length>::GetLength(); }

    /**
     * Copies the name data into a given char buffer with a given size.
     *
     * The given buffer is cleared (`memset` to zero) before copying the name into it. The copied string
     * in @p aBuffer is NOT necessarily null terminated.
     *
     * @param[out] aBuffer   A pointer to a buffer where to copy the name into.
     * @param[in]  aMaxSize  Size of @p aBuffer (maximum number of chars to write into @p aBuffer).
     *
     * @returns The actual number of chars copied into @p aBuffer.
     *
     */
    uint8_t CopyTo(char *aBuffer, uint8_t aMaxSize) const;
};

/**
 * Represents an Network Name.
 *
 */
class NetworkName : public otNetworkName, public Unequatable<NetworkName>
{
public:
    static constexpr const char *kNetworkNameInit = "OpenThread";
    static constexpr const char *kDomainNameInit  = "DefaultDomain";

    /**
     * This constant specified the maximum number of chars in Network Name (excludes null char).
     *
     */
    static constexpr uint8_t kMaxSize = OT_NETWORK_NAME_MAX_SIZE;

    /**
     * Initializes the Network Name as an empty string.
     *
     */
    NetworkName(void) { m8[0] = '\0'; }

    /**
     * Gets the Network Name as a null terminated C string.
     *
     * @returns The Network Name as a null terminated C string array.
     *
     */
    const char *GetAsCString(void) const { return m8; }

    /**
     * Gets the Network Name as NameData.
     *
     * @returns The Network Name as NameData.
     *
     */
    NameData GetAsData(void) const;

    /**
     * Sets the Network Name from a given null terminated C string.
     *
     * Also validates that the given @p aNameString follows UTF-8 encoding and can fit in `kMaxSize`
     * chars.
     *
     * @param[in] aNameString      A name C string.
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorAlready       The name is already set to the same string.
     * @retval kErrorInvalidArgs   Given name is invalid (too long or does not follow UTF-8 encoding).
     *
     */
    Error Set(const char *aNameString);

    /**
     * Sets the Network Name.
     *
     * @param[in]  aNameData           A reference to name data.
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorAlready       The name is already set to the same string.
     * @retval kErrorInvalidArgs   Given name is too long.
     *
     */
    Error Set(const NameData &aNameData);

    /**
     * Overloads operator `==` to evaluate whether or not two given `NetworkName` objects are equal.
     *
     * @param[in]  aOther  The other `NetworkName` to compare with.
     *
     * @retval TRUE   If the two are equal.
     * @retval FALSE  If the two are not equal.
     *
     */
    bool operator==(const NetworkName &aOther) const;
};

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
/**
 * Represents a Thread Domain Name.
 *
 */
typedef NetworkName DomainName;
#endif

/**
 * Manages the Network Name value.
 *
 */
class NetworkNameManager : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit NetworkNameManager(Instance &aInstance);

    /**
     * Returns the Network Name.
     *
     * @returns The Network Name.
     *
     */
    const NetworkName &GetNetworkName(void) const { return mNetworkName; }

    /**
     * Sets the Network Name.
     *
     * @param[in]  aNameString   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     *
     */
    Error SetNetworkName(const char *aNameString);

    /**
     * Sets the Network Name.
     *
     * @param[in]  aNameData     A name data (pointer to char buffer and length).
     *
     * @retval kErrorNone          Successfully set the Network Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     *
     */
    Error SetNetworkName(const NameData &aNameData);

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    /**
     * Returns the Thread Domain Name.
     *
     * @returns The Thread Domain Name.
     *
     */
    const DomainName &GetDomainName(void) const { return mDomainName; }

    /**
     * Sets the Thread Domain Name.
     *
     * @param[in]  aNameString   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone          Successfully set the Thread Domain Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     *
     */
    Error SetDomainName(const char *aNameString);

    /**
     * Sets the Thread Domain Name.
     *
     * @param[in]  aNameData     A name data (pointer to char buffer and length).
     *
     * @retval kErrorNone          Successfully set the Thread Domain Name.
     * @retval kErrorInvalidArgs   Given name is too long.
     *
     */
    Error SetDomainName(const NameData &aNameData);
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

private:
    Error SignalNetworkNameChange(Error aError);

    NetworkName mNetworkName;

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    DomainName mDomainName;
#endif
};

} // namespace MeshCoP

DefineCoreType(otNetworkName, MeshCoP::NetworkName);

} // namespace ot

#endif // MESHCOP_EXTENDED_PANID_HPP_
