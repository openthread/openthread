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
 *   This file includes definitions for a generic array.
 */

#ifndef ARRAY_HPP_
#define ARRAY_HPP_

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/const_cast.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/numeric_limits.hpp"
#include "common/type_traits.hpp"

namespace ot {

/**
 * Returns the length of a given array (number of elements in the array).
 *
 * This template function is `constexpr`. The template arguments are expected to be deduced by the compiler allowing
 * callers to simply use `GetArrayLength(aArray)`.
 *
 * @tparam  Type          The array element type.
 * @tparam  kArrayLength  The array length.
 *
 * @returns The array length (number of elements in the array).
 */
template <typename Type, uint16_t kArrayLength> constexpr inline uint16_t GetArrayLength(const Type (&)[kArrayLength])
{
    return kArrayLength;
}

/**
 * Returns a pointer to end of a given array (pointing to the past-the-end element).
 *
 * Note that the past-the-end element is a theoretical element that would follow the last element in the array. It does
 * not point to an actual element in array, and thus should not be dereferenced.
 *
 * @tparam  Type          The array element type.
 * @tparam  kArrayLength  The array length.
 *
 * @param[in] aArray   A reference to the array.
 *
 * @returns Pointer to the past-the-end element.
 */
template <typename Type, uint16_t kArrayLength> inline Type *GetArrayEnd(Type (&aArray)[kArrayLength])
{
    return &aArray[kArrayLength];
}

/**
 * Returns a pointer to end of a given array (pointing to the past-the-end element).
 *
 * Note that the past-the-end element is a theoretical element that would follow the last element in the array. It does
 * not point to an actual element in array, and thus should not be dereferenced.
 *
 * @tparam  Type          The array element type.
 * @tparam  kArrayLength  The array length.
 *
 * @param[in] aArray   A reference to the array.
 *
 * @returns Pointer to the past-the-end element.
 */
template <typename Type, uint16_t kArrayLength> inline const Type *GetArrayEnd(const Type (&aArray)[kArrayLength])
{
    return &aArray[kArrayLength];
}

/**
 * Represents an array of elements with a fixed max size.
 *
 * @tparam Type        The array element type.
 * @tparam kMaxSize    Specifies the max array size (maximum number of elements in the array).
 * @tparam SizeType    The type to be used for array size, length, and index. If not specified, a default `uint` type
 *                     is determined based on `kMaxSize`, i.e., if `kMaxSize <= 255` then `uint8_t` will be used,
 *                     otherwise `uint16_t` will be used.
 */
template <typename Type,
          uint16_t kMaxSize,
          typename SizeType =
              typename TypeTraits::Conditional<kMaxSize <= NumericLimits<uint8_t>::kMax, uint8_t, uint16_t>::Type>
class Array
{
    static_assert(kMaxSize != 0, "Array `kMaxSize` cannot be zero");

public:
    /**
     * Represents the length or index in array.
     *
     * It is typically either `uint8_t` or `uint16_t` (determined based on the maximum array size (`kMaxSize`)).
     */
    typedef SizeType IndexType;

    /**
     * Initializes the array as empty.
     */
    Array(void)
        : mLength(0)
    {
    }

    /**
     * Initializes the array by copying elements from another array.
     *
     * The method uses assignment `=` operator on `Type` to copy each element from @p aOtherArray into the elements of
     * the array.
     *
     * @param[in] aOtherArray  Another array to copy from.
     */
    Array(const Array &aOtherArray) { *this = aOtherArray; }

    /**
     * Initializes the array as empty and initializes its elements by calling `Init(Instance &)`
     * method on every element.
     *
     * Uses method `Init(Instance &aInstance)` on `Type`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit Array(Instance &aInstance)
        : mLength(0)
    {
        for (Type &element : mElements)
        {
            element.Init(aInstance);
        }
    }

    /**
     * Clears the array.
     */
    void Clear(void) { mLength = 0; }

    /**
     * Indicates whether or not the array is empty.
     *
     * @retval TRUE when array is empty.
     * @retval FALSE when array is not empty.
     */
    bool IsEmpty(void) const { return (mLength == 0); }

    /**
     * Indicates whether or not the array is full.
     *
     * @retval TRUE when array is full.
     * @retval FALSE when array is not full.
     */
    bool IsFull(void) const { return (mLength == GetMaxSize()); }

    /**
     * Returns the maximum array size (max number of elements).
     *
     * @returns The maximum array size (max number of elements that can be added to the array).
     */
    IndexType GetMaxSize(void) const { return static_cast<IndexType>(kMaxSize); }

    /**
     * Returns the current length of array (number of elements).
     *
     * @returns The current array length.
     */
    IndexType GetLength(void) const { return mLength; }

    /**
     * Sets the current length (number of elements) of the array.
     *
     * @param[in] aLength   The array length.
     */
    void SetLength(IndexType aLength) { mLength = aLength; }

    /**
     * Returns the pointer to the start of underlying C array buffer serving as `Array` storage.
     *
     * @return The pointer to start of underlying C array buffer.
     */
    Type *GetArrayBuffer(void) { return mElements; }

    /**
     * Returns the pointer to the start of underlying C array buffer serving as `Array` storage.
     *
     * @return The pointer to start of underlying C array buffer.
     */
    const Type *GetArrayBuffer(void) const { return mElements; }

    /**
     * Overloads the `[]` operator to get the element at a given index.
     *
     * Does not perform index bounds checking. Behavior is undefined if @p aIndex is not valid.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A reference to the element in array at @p aIndex.
     */
    Type &operator[](IndexType aIndex) { return mElements[aIndex]; }

    /**
     * Overloads the `[]` operator to get the element at a given index.
     *
     * Does not perform index bounds checking. Behavior is undefined if @p aIndex is not valid.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A reference to the element in array at @p aIndex.
     */
    const Type &operator[](IndexType aIndex) const { return mElements[aIndex]; }

    /**
     * Gets a pointer to the element at a given index.
     *
     * Unlike `operator[]`, this method checks @p aIndex to be valid and within the current length.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A pointer to element in array at @p aIndex or `nullptr` if @p aIndex is not valid.
     */
    Type *At(IndexType aIndex) { return (aIndex < mLength) ? &mElements[aIndex] : nullptr; }

    /**
     * Gets a pointer to the element at a given index.
     *
     * Unlike `operator[]`, this method checks @p aIndex to be valid and within the current length.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A pointer to element in array at @p aIndex or `nullptr` if @p aIndex is not valid.
     */
    const Type *At(IndexType aIndex) const { return (aIndex < mLength) ? &mElements[aIndex] : nullptr; }

    /**
     * Gets a pointer to the element at the front of the array (first element).
     *
     * @returns A pointer to the front element or `nullptr` if array is empty.
     */
    Type *Front(void) { return At(0); }

    /**
     * Gets a pointer to the element at the front of the array (first element).
     *
     * @returns A pointer to the front element or `nullptr` if array is empty.
     */
    const Type *Front(void) const { return At(0); }

    /**
     * Gets a pointer to the element at the back of the array (last element).
     *
     * @returns A pointer to the back element or `nullptr` if array is empty.
     */
    Type *Back(void) { return At(mLength - 1); }

    /**
     * Gets a pointer to the element at the back of the array (last element).
     *
     * @returns A pointer to the back element or `nullptr` if array is empty.
     */
    const Type *Back(void) const { return At(mLength - 1); }

    /**
     * Appends a new entry to the end of the array.
     *
     * The method uses assignment `=` operator on `Type` to copy @p aEntry into the added array element.
     *
     * @param[in] aEntry     The new entry to push back.
     *
     * @retval kErrorNone    Successfully pushed back @p aEntry to the end of the array.
     * @retval kErrorNoBufs  Could not append the new element since array is full.
     */
    Error PushBack(const Type &aEntry) { return IsFull() ? kErrorNoBufs : (mElements[mLength++] = aEntry, kErrorNone); }

    /**
     * Appends a new entry to the end of the array.
     *
     * On success, this method returns a pointer to the newly appended element in the array for the caller to
     * initialize and use.
     *
     * @return A pointer to the newly appended element or `nullptr` if array is full.
     */
    Type *PushBack(void) { return IsFull() ? nullptr : &mElements[mLength++]; }

    /**
     * Removes the last element in the array.
     *
     * @returns A pointer to the removed element from the array, or `nullptr` if array is empty.
     */
    Type *PopBack(void) { return IsEmpty() ? nullptr : &mElements[--mLength]; }

    /**
     * Returns the index of an element in the array.
     *
     * The @p aElement MUST be from the array, otherwise the behavior of this method is undefined.
     *
     * @param[in] aElement  A reference to an element in the array.
     *
     * @returns The index of @p aElement in the array.
     */
    IndexType IndexOf(const Type &aElement) const { return static_cast<IndexType>(&aElement - &mElements[0]); }

    /**
     * Removes an element from the array.
     *
     * The @p aElement MUST be from the array, otherwise the behavior of this method is undefined.
     *
     * To remove @p aElement, it is replaced by the last element in array, so the order of items in the array can
     * change after a call to this method.
     *
     * The method uses assignment `=` operator on `Type` to copy the last element in place of @p aElement.
     */
    void Remove(Type &aElement)
    {
        Type *lastElement = PopBack();

        if (lastElement != &aElement)
        {
            aElement = *lastElement;
        }
    }

    /**
     * Finds the first match of a given entry in the array.
     *
     * Uses `==` operator on `Type` to compare the array element with @p aEntry.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @returns A pointer to matched array element, or `nullptr` if a match could not be found.
     */
    Type *Find(const Type &aEntry) { return AsNonConst(AsConst(this)->Find(aEntry)); }

    /**
     * Finds the first match of a given entry in the array.
     *
     * Uses `==` operator to compare the array elements with @p aEntry.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @returns A pointer to matched array element, or `nullptr` if a match could not be found.
     */
    const Type *Find(const Type &aEntry) const
    {
        const Type *matched = nullptr;

        for (const Type &element : *this)
        {
            if (element == aEntry)
            {
                matched = &element;
                break;
            }
        }

        return matched;
    }

    /**
     * Indicates whether or not a match to given entry exists in the array.
     *
     * Uses `==` operator on `Type` to compare the array elements with @p aEntry.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @retval TRUE   The array contains a matching element with @p aEntry.
     * @retval FALSE  The array does not contain a matching element with @p aEntry.
     */
    bool Contains(const Type &aEntry) const { return Find(aEntry) != nullptr; }

    /**
     * Finds the first element in the array matching a given indicator.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     *
     * @returns A pointer to the matched array element, or `nullptr` if a match could not be found.
     */
    template <typename Indicator> Type *FindMatching(const Indicator &aIndicator)
    {
        return AsNonConst(AsConst(this)->FindMatching(aIndicator));
    }

    /**
     * Finds the first element in the array matching a given indicator.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     *
     * @returns A pointer to the matched array element, or `nullptr` if a match could not be found.
     */
    template <typename Indicator> const Type *FindMatching(const Indicator &aIndicator) const
    {
        const Type *matched = nullptr;

        for (const Type &element : *this)
        {
            if (element.Matches(aIndicator))
            {
                matched = &element;
                break;
            }
        }

        return matched;
    }

    /**
     * Indicates whether or not the array contains an element matching a given indicator.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     *
     * @retval TRUE   The array contains a matching element with @p aIndicator.
     * @retval FALSE  The array does not contain a matching element with @p aIndicator.
     */
    template <typename Indicator> bool ContainsMatching(const Indicator &aIndicator) const
    {
        return FindMatching(aIndicator) != nullptr;
    }

    /**
     * Removes the first element in the array matching a given indicator.
     *
     * Behaves similar to `Remove()`, i.e., the matched element (if found) is replaced with the last element
     * in the array (using `=` operator on `Type`). So the order of items in the array can change after a call to this
     * method.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     */
    template <typename Indicator> void RemoveMatching(const Indicator &aIndicator)
    {
        Type *entry = FindMatching(aIndicator);

        if (entry != nullptr)
        {
            Remove(*entry);
        }
    }

    /**
     * Removes all elements in the array matching a given indicator.
     *
     * Behaves similar to `Remove()`, i.e., a matched element is replaced with the last element in the
     * array (using `=` operator on `Type`). So the order of items in the array can change after a call to this method.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     */
    template <typename Indicator> void RemoveAllMatching(const Indicator &aIndicator)
    {
        for (IndexType index = 0; index < GetLength();)
        {
            Type &entry = mElements[index];

            if (entry.Matches(aIndicator))
            {
                Remove(entry);

                // When the entry is removed from the array it is
                // replaced with the last element. In this case, we do
                // not increment `index`.
            }
            else
            {
                index++;
            }
        }
    }

    /**
     * Overloads assignment `=` operator to copy elements from another array into the array.
     *
     * The method uses assignment `=` operator on `Type` to copy each element from @p aOtherArray into the elements of
     * the array.
     *
     * @param[in] aOtherArray  Another array to copy from.
     */
    Array &operator=(const Array &aOtherArray)
    {
        Clear();

        for (const Type &otherElement : aOtherArray)
        {
            IgnoreError(PushBack(otherElement));
        }

        return *this;
    }

    /**
     * Indicates whether a given entry pointer is from the array buffer.
     *
     * Does not check the current length of array and only checks that @p aEntry is pointing to an address
     * contained within underlying C array buffer.
     *
     * @param[in] aEntry   A pointer to an entry to check.
     *
     * @retval TRUE  The @p aEntry is from the array.
     * @retval FALSE The @p aEntry is not from the array.
     */
    bool IsInArrayBuffer(const Type *aEntry) const
    {
        return (&mElements[0] <= aEntry) && (aEntry < GetArrayEnd(mElements));
    }

    // The following methods are intended to support range-based `for`
    // loop iteration over the array elements and should not be used
    // directly.

    Type       *begin(void) { return &mElements[0]; }
    Type       *end(void) { return &mElements[mLength]; }
    const Type *begin(void) const { return &mElements[0]; }
    const Type *end(void) const { return &mElements[mLength]; }

private:
    Type      mElements[kMaxSize];
    IndexType mLength;
};

} // namespace ot

#endif // ARRAY_HPP_
