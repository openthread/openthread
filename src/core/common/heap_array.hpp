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
 *   This file includes definitions for `Heap::Array` (a heap allocated array of flexible length).
 */

#ifndef HEAP_ARRAY_HPP_
#define HEAP_ARRAY_HPP_

#include "openthread-core-config.h"

#include <stdint.h>
#include <stdio.h>

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/heap.hpp"
#include "common/new.hpp"

namespace ot {
namespace Heap {

/**
 * This class represents a heap allocated array.
 *
 * The buffer to store the elements is allocated from heap and is managed by the `Heap::Array` class itself. The `Array`
 * implementation will automatically grow the buffer when new entries are added. The `Heap::Array` destructor will
 * always free the allocated buffer.
 *
 * The `Type` class MUST provide a move constructor `Type(Type &&aOther)` (or a copy constructor if no move constructor
 * is provided). This constructor is used to move existing elements when array buffer is grown (new buffer is
 * allocated) to make room for new elements.
 *
 * @tparam  Type                  The array element type.
 * @tparam  kCapacityIncrements   Number of elements to allocate at a time when updating the array buffer.
 *
 */
template <typename Type, uint16_t kCapacityIncrements = 2> class Array
{
public:
    using IndexType = uint16_t;

    /**
     * This constructor initializes the `Array` as empty.
     *
     */
    Array(void)
        : mArray(nullptr)
        , mLength(0)
        , mCapacity(0)
    {
    }

    /**
     * This is the destructor for `Array` object.
     *
     */
    ~Array(void) { Free(); }

    /**
     * This method frees any buffer allocated by the `Array`.
     *
     * The `Array` destructor will automatically call `Free()`. This method allows caller to free buffer explicitly.
     *
     */
    void Free(void)
    {
        Clear();
        Heap::Free(mArray);
        mArray    = nullptr;
        mCapacity = 0;
    }

    /**
     * This method clears the array.
     *
     * Note that `Clear()` method (unlike `Free()`) does not free the allocated buffer and therefore does not change
     * the current capacity of the array.
     *
     * This method invokes `Type` destructor on all cleared existing elements of array.
     *
     */
    void Clear(void)
    {
        for (Type &entry : *this)
        {
            entry.~Type();
        }

        mLength = 0;
    }

    /**
     * This method returns the current array length (number of elements in the array).
     *
     * @returns The array length.
     *
     */
    IndexType GetLength(void) const { return mLength; }

    /**
     * This method returns a raw pointer to the array buffer.
     *
     * The returned raw pointer is valid only while the `Array` remains unchanged.
     *
     * @returns A pointer to the array buffer or `nullptr` if the array is empty.
     *
     */
    const Type *AsCArray(void) const { return (mLength != 0) ? mArray : nullptr; }

    /**
     * This method returns the current capacity of array (number of elements that can fit in current allocated buffer).
     *
     * The allocated buffer and array capacity are automatically increased (by the `Array` itself) when new elements
     * are added to array. Removing elements does not change the buffer and the capacity. A desired capacity can be
     * reserved using `ReserveCapacity()` method.
     *
     * @returns The current capacity of the array.
     *
     */
    IndexType GetCapacity(void) const { return mCapacity; }

    /**
     * This method allocates buffer to reserve a given capacity for array.
     *
     * If the requested @p aCapacity is smaller than the current length of the array, capacity remains unchanged.
     *
     * @param[in] aCapacity   The target capacity for the array.
     *
     * @retval kErrorNone     Array was successfully updated to support @p aCapacity.
     * @retval kErrorNoBufs   Could not allocate buffer.
     *
     */
    Error ReserveCapacity(IndexType aCapacity) { return Allocate(aCapacity); }

    /**
     * This method sets the array by taking the buffer from another given array (using move semantics).
     *
     * @param[in] aOther    The other `Heap::Array` to take from (rvalue reference).
     *
     */
    void TakeFrom(Array &&aOther)
    {
        Free();
        mArray           = aOther.mArray;
        mLength          = aOther.mLength;
        mCapacity        = aOther.mCapacity;
        aOther.mArray    = nullptr;
        aOther.mLength   = 0;
        aOther.mCapacity = 0;
    }

    /**
     * This method overloads the `[]` operator to get the element at a given index.
     *
     * This method does not perform index bounds checking. Behavior is undefined if @p aIndex is not valid.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A reference to the element in array at @p aIndex.
     *
     */
    Type &operator[](IndexType aIndex) { return mArray[aIndex]; }

    /**
     * This method overloads the `[]` operator to get the element at a given index.
     *
     * This method does not perform index bounds checking. Behavior is undefined if @p aIndex is not valid.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A reference to the element in array at @p aIndex.
     *
     */
    const Type &operator[](IndexType aIndex) const { return mArray[aIndex]; }

    /**
     * This method gets a pointer to the element at a given index.
     *
     * Unlike `operator[]`, this method checks @p aIndex to be valid and within the current length. The returned
     * pointer is valid only while the `Array` remains unchanged.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A pointer to element in array at @p aIndex or `nullptr` if @p aIndex is not valid.
     *
     */
    Type *At(IndexType aIndex) { return (aIndex < mLength) ? &mArray[aIndex] : nullptr; }

    /**
     * This method gets a pointer to the element at a given index.
     *
     * Unlike `operator[]`, this method checks @p aIndex to be valid and within the current length. The returned
     * pointer is valid only while the `Array` remains unchanged.
     *
     * @param[in] aIndex  The index to get.
     *
     * @returns A pointer to element in array at @p aIndex or `nullptr` if @p aIndex is not valid.
     *
     */
    const Type *At(IndexType aIndex) const { return (aIndex < mLength) ? &mArray[aIndex] : nullptr; }

    /**
     * This method gets a pointer to the element at the front of the array (first element).
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @returns A pointer to the front element or `nullptr` if array is empty.
     *
     */
    Type *Front(void) { return At(0); }

    /**
     * This method gets a pointer to the element at the front of the array (first element).
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @returns A pointer to the front element or `nullptr` if array is empty.
     *
     */
    const Type *Front(void) const { return At(0); }

    /**
     * This method gets a pointer to the element at the back of the array (last element).
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @returns A pointer to the back element or `nullptr` if array is empty.
     *
     */
    Type *Back(void) { return (mLength > 0) ? &mArray[mLength - 1] : nullptr; }

    /**
     * This method gets a pointer to the element at the back of the array (last element).
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @returns A pointer to the back element or `nullptr` if array is empty.
     *
     */
    const Type *Back(void) const { return (mLength > 0) ? &mArray[mLength - 1] : nullptr; }

    /**
     * This method appends a new entry to the end of the array.
     *
     * This method requires the `Type` to provide a copy constructor of format `Type(const Type &aOther)` to init the
     * new element in the array from @p aEntry.
     *
     * @param[in] aEntry     The new entry to push back.
     *
     * @retval kErrorNone    Successfully pushed back @p aEntry to the end of the array.
     * @retval kErrorNoBufs  Could not allocate buffer to grow the array.
     *
     */
    Error PushBack(const Type &aEntry)
    {
        Error error = kErrorNone;

        if (mLength == mCapacity)
        {
            SuccessOrExit(error = Allocate(mCapacity + kCapacityIncrements));
        }

        new (&mArray[mLength++]) Type(aEntry);

    exit:
        return error;
    }

    /**
     * This method appends a new entry to the end of the array.
     *
     * This method requires the `Type` to provide a copy constructor of format `Type(Type &&aOther)` to init the
     * new element in the array from @p aEntry.
     *
     * @param[in] aEntry     The new entry to push back (an rvalue reference)
     *
     * @retval kErrorNone    Successfully pushed back @p aEntry to the end of the array.
     * @retval kErrorNoBufs  Could not allocate buffer to grow the array.
     *
     */
    Error PushBack(Type &&aEntry)
    {
        Error error = kErrorNone;

        if (mLength == mCapacity)
        {
            SuccessOrExit(error = Allocate(mCapacity + kCapacityIncrements));
        }

        new (&mArray[mLength++]) Type(static_cast<Type &&>(aEntry));

    exit:
        return error;
    }

    /**
     * This method appends a new entry to the end of the array.
     *
     * On success, this method returns a pointer to the newly appended element in the array for the caller to
     * initialize and use. This method uses the `Type(void)` default constructor on the newly appended element (if not
     * `nullptr`).
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @return A pointer to the newly appended element or `nullptr` if could not allocate buffer to grow the array
     *
     */
    Type *PushBack(void)
    {
        Type *newEntry = nullptr;

        if (mLength == mCapacity)
        {
            SuccessOrExit(Allocate(mCapacity + kCapacityIncrements));
        }

        newEntry = new (&mArray[mLength++]) Type();

    exit:
        return newEntry;
    }

    /**
     * This method removes the last element in the array.
     *
     * This method will invoke the `Type` destructor on the removed element.
     *
     */
    void PopBack(void)
    {
        if (mLength > 0)
        {
            mArray[mLength - 1].~Type();
            mLength--;
        }
    }

    /**
     * This method returns the index of an element in the array.
     *
     * The @p aElement MUST be from the array, otherwise the behavior of this method is undefined.
     *
     * @param[in] aElement  A reference to an element in the array.
     *
     * @returns The index of @p aElement in the array.
     *
     */
    IndexType IndexOf(const Type &aElement) const { return static_cast<IndexType>(&aElement - mArray); }

    /**
     * This method finds the first match of a given entry in the array.
     *
     * This method uses `==` operator on `Type` to compare the array element with @p aEntry. The returned pointer is
     * valid only while the `Array` remains unchanged.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @returns A pointer to matched array element, or `nullptr` if a match could not be found.
     *
     */
    Type *Find(const Type &aEntry) { return AsNonConst(AsConst(this)->Find(aEntry)); }

    /**
     * This method finds the first match of a given entry in the array.
     *
     * This method uses `==` operator to compare the array elements with @p aEntry. The returned pointer is valid only
     * while the `Array` remains unchanged.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @returns A pointer to matched array element, or `nullptr` if a match could not be found.
     *
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
     * This method indicates whether or not a match to given entry exists in the array.
     *
     * This method uses `==` operator on `Type` to compare the array elements with @p aEntry.
     *
     * @param[in] aEntry   The entry to search for within the array.
     *
     * @retval TRUE   The array contains a matching element with @p aEntry.
     * @retval FALSE  The array does not contain a matching element with @p aEntry.
     *
     */
    bool Contains(const Type &aEntry) const { return Find(aEntry) != nullptr; }

    /**
     * This template method finds the first element in the array matching a given indicator.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     *
     * @returns A pointer to the matched array element, or `nullptr` if a match could not be found.
     *
     */
    template <typename Indicator> Type *FindMatching(const Indicator &aIndicator)
    {
        return AsNonConst(AsConst(this)->FindMatching(aIndicator));
    }

    /**
     * This template method finds the first element in the array matching a given indicator.
     *
     * The template type `Indicator` specifies the type of @p aIndicator object which is used to match against elements
     * in the array. To check that an element matches the given indicator, the `Matches()` method is invoked on each
     * `Type` element in the array. The `Matches()` method should be provided by `Type` class accordingly:
     *
     *     bool Type::Matches(const Indicator &aIndicator) const
     *
     * The returned pointer is valid only while the `Array` remains unchanged.
     *
     * @param[in]  aIndicator  An indicator to match with elements in the array.
     *
     * @returns A pointer to the matched array element, or `nullptr` if a match could not be found.
     *
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
     * This template method indicates whether or not the array contains an element matching a given indicator.
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
     *
     */
    template <typename Indicator> bool ContainsMatching(const Indicator &aIndicator) const
    {
        return FindMatching(aIndicator) != nullptr;
    }

    // The following methods are intended to support range-based `for`
    // loop iteration over the array elements and should not be used
    // directly.

    Type *      begin(void) { return (mLength > 0) ? mArray : nullptr; }
    Type *      end(void) { return (mLength > 0) ? &mArray[mLength] : nullptr; }
    const Type *begin(void) const { return (mLength > 0) ? mArray : nullptr; }
    const Type *end(void) const { return (mLength > 0) ? &mArray[mLength] : nullptr; }

    Array(const Array &) = delete;
    Array &operator=(const Array &) = delete;

private:
    Error Allocate(IndexType aCapacity)
    {
        Error error = kErrorNone;
        Type *newArray;

        VerifyOrExit((aCapacity != mCapacity) && (aCapacity >= mLength));
        newArray = static_cast<Type *>(Heap::CAlloc(aCapacity, sizeof(Type)));
        VerifyOrExit(newArray != nullptr, error = kErrorNoBufs);

        for (IndexType index = 0; index < mLength; index++)
        {
            new (&newArray[index]) Type(static_cast<Type &&>(mArray[index]));
            mArray[index].~Type();
        }

        Heap::Free(mArray);
        mArray    = newArray;
        mCapacity = aCapacity;

    exit:
        return error;
    }

    Type *    mArray;
    IndexType mLength;
    IndexType mCapacity;
};

} // namespace Heap
} // namespace ot

#endif // HEAP_ARRAY_HPP_
