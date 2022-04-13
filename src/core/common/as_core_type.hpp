/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes helper functions to convert between public OT C structs and corresponding core C++ classes.
 */

#ifndef AS_CORE_TYPE_HPP_
#define AS_CORE_TYPE_HPP_

#include "openthread-core-config.h"

namespace ot {

/**
 * This structure relates a given public OT type to its corresponding core C++ class/type.
 *
 * @tparam FromType  The public OT type.
 *
 * Specializations of this template struct are provided for different `FromType` which include a member type definition
 * named `Type` to provide the corresponding core class/type related to `FromType.
 *
 * For example, `CoreType<otIp6Address>::Type` is defined as `Ip6::Address`.
 *
 */
template <typename FromType> struct CoreType;

/**
 * This function converts a pointer to public OT type (C struct) to the corresponding core C++ type reference.
 *
 * @tparam Type   The public OT type to convert.
 *
 * @param[in] aObject   A pointer to the object to convert.
 *
 * @returns A reference of the corresponding C++ type matching @p aObject.
 *
 */
template <typename Type> typename CoreType<Type>::Type &AsCoreType(Type *aObject)
{
    return *static_cast<typename CoreType<Type>::Type *>(aObject);
}

/**
 * This function converts a const pointer to public OT type (C struct) to the corresponding core C++ type reference.
 *
 * @tparam Type   The public OT type to convert.
 *
 * @param[in] aObject   A const pointer to the object to convert.
 *
 * @returns A const reference of the corresponding C++ type matching @p aObject.
 *
 */
template <typename Type> const typename CoreType<Type>::Type &AsCoreType(const Type *aObject)
{
    return *static_cast<const typename CoreType<Type>::Type *>(aObject);
}

/**
 * This function converts a pointer to public OT type (C struct) to the corresponding core C++ type pointer.
 *
 * @tparam Type   The public OT type to convert.
 *
 * @param[in] aObject   A pointer to the object to convert.
 *
 * @returns A pointer of the corresponding C++ type matching @p aObject.
 *
 */
template <typename Type> typename CoreType<Type>::Type *AsCoreTypePtr(Type *aObject)
{
    return static_cast<typename CoreType<Type>::Type *>(aObject);
}

/**
 * This function converts a const pointer to public OT type (C struct) to the corresponding core C++ type pointer.
 *
 * @tparam Type   The public OT type to convert.
 *
 * @param[in] aObject   A pointer to the object to convert.
 *
 * @returns A const pointer of the corresponding C++ type matching @p aObject.
 *
 */
template <typename Type> const typename CoreType<Type>::Type *AsCoreTypePtr(const Type *aObject)
{
    return static_cast<const typename CoreType<Type>::Type *>(aObject);
}

/**
 * This structure maps two enumeration types.
 *
 * @tparam FromEnumType  The enum type.
 *
 * Specializations of this template struct are provided which include a member type definition named `Type` to provide
 * the related `enum` type mapped with `FromEnumType`.
 *
 * For example, `MappedEnum<otMacFilterAddressMode>::Type` is defined as `Mac::Filter::Mode`.
 *
 */
template <typename FromEnumType> struct MappedEnum;

/**
 * This function convert an enumeration type value to a related enumeration type value.
 *
 * @param[in] aValue   The enumeration value to convert
 *
 * @returns The matching enumeration value.
 *
 */
template <typename EnumType> const typename MappedEnum<EnumType>::Type MapEnum(EnumType aValue)
{
    return static_cast<typename MappedEnum<EnumType>::Type>(aValue);
}

// Helper macro to define specialization of `CoreType` struct relating `BaseType` with `SubType`.
#define DefineCoreType(BaseType, SubType) \
    template <> struct CoreType<BaseType> \
    {                                     \
        using Type = SubType;             \
    }

// Helper macro to map two related enumeration types.
#define DefineMapEnum(FirstEnumType, SecondEnumType) \
    template <> struct MappedEnum<FirstEnumType>     \
    {                                                \
        using Type = SecondEnumType;                 \
    };                                               \
                                                     \
    template <> struct MappedEnum<SecondEnumType>    \
    {                                                \
        using Type = FirstEnumType;                  \
    }

} // namespace ot

#endif // AS_CORE_TYPE_HPP_
