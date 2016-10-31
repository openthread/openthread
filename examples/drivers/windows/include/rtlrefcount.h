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
 * @brief
 *  This module contains routines and type definitions for managing reference
 *  counts.
 *
 *  N.B. The functions defined here use the minimum fencing required for correct
 *       management of the reference count contract. No additional memory
 *       ordering should be assumed.
 */

#pragma once

//
// Architecture support macros.
// (Undefined at the bottom to avoid global namespace pollution)
//

#if defined(_WIN64)

#define RtlIncrementLongPtrNoFence InterlockedIncrementNoFence64
#define RtlDecrementLongPtrRelease InterlockedDecrementRelease64
#define RtlExchangeAddLongPtrNoFence InterlockedExchangeAddNoFence64
#define RtlExchangeAddLongPtrRelease InterlockedExchangeAddRelease64
#define RtlCompareExchangeLongPtrNoFence InterlockedCompareExchangeNoFence64
#define RtlCompareExchangeLongPtrRelease InterlockedCompareExchangeRelease64

#else

#define RtlIncrementLongPtrNoFence InterlockedIncrementNoFence
#define RtlDecrementLongPtrRelease InterlockedDecrementRelease
#define RtlExchangeAddLongPtrNoFence InterlockedExchangeAddNoFence
#define RtlExchangeAddLongPtrRelease InterlockedExchangeAddRelease
#define RtlCompareExchangeLongPtrNoFence InterlockedCompareExchangeNoFence
#define RtlCompareExchangeLongPtrRelease InterlockedCompareExchangeRelease

#endif

#if defined(_X86_) || defined(_AMD64_)

#define RtlBarrierAfterInterlock()

#elif defined(_ARM64_)

#define RtlBarrierAfterInterlock()  __dmb(_ARM64_BARRIER_ISH)

#elif defined(_ARM_)

#define RtlBarrierAfterInterlock()  __dmb(_ARM_BARRIER_ISH)

#else

#define Unsupported architecture.

#endif

#define RTL_REF_COUNT_INIT 1

FORCEINLINE
VOID
RtlInitializeReferenceCount (
    _Out_ PRTL_REFERENCE_COUNT RefCount
    )

/*++

Routine Description:

    This function initializes a reference count to 1.

Arguments:

    RefCount - Supplies a pointer to a reference count to initialize.

Return Value:

    None.

--*/

{

    *RefCount = RTL_REF_COUNT_INIT;
    return;
}

FORCEINLINE
VOID
RtlInitializeReferenceCountEx (
    _Out_ PRTL_REFERENCE_COUNT RefCount,
    _In_ ULONG Bias
    )

/*++

Routine Description:

    This function initializes a reference count to a positive value.

Arguments:

    RefCount - Supplies a pointer to a reference count to initialize.

    Bias - Supplies an initial reference count (must be positive).

Return Value:

    None.

--*/

{

    *RefCount = Bias;
    return;
}

FORCEINLINE
VOID
RtlIncrementReferenceCount (
    _Inout_ PRTL_REFERENCE_COUNT RefCount
    )

/*++

Routine Description:

    This function increments the specified reference count, preventing object
    deletion.

Arguments:

    RefCount - Supplies a pointer to a reference count.

Return Value:

    None.

--*/

{

    if (RtlIncrementLongPtrNoFence(RefCount) > 1) {
        return;
    }

    __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
}

FORCEINLINE
VOID
RtlIncrementReferenceCountEx (
    _Inout_ PRTL_REFERENCE_COUNT RefCount,
    _In_ ULONG Bias
    )

/*++

Routine Description:

    This function increases the specified reference count by the specified bias,
    preventing object deletion.

Arguments:

    RefCount - Supplies a pointer to a reference count.

    Bias - Supplies a reference bias amount.

Return Value:

    None.

--*/

{

    if (RtlExchangeAddLongPtrNoFence(RefCount, Bias) > 0) {
        return;
    }

    __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
}

FORCEINLINE
BOOLEAN
RtlIncrementReferenceCountNonZero (
    _Inout_ volatile RTL_REFERENCE_COUNT *RefCount,
    _In_ ULONG Bias
    )

/*++

Routine Description:

    This function increases the specified reference count by the specified bias,
    unless the reference count was previously zero.

Arguments:

    RefCount - Supplies a pointer to a reference count.

    Bias - Supplies a reference bias amount.

Return Value:

    TRUE if the reference count was incremented, FALSE otherwise.

--*/

{

    RTL_REFERENCE_COUNT NewValue;
    RTL_REFERENCE_COUNT OldValue;

    PrefetchForWrite(RefCount);
    OldValue = ReadLongPtrNoFence(RefCount);
    for (;;) {
        NewValue = OldValue + Bias;
        if ((ULONG_PTR)NewValue > Bias) {
            NewValue = RtlCompareExchangeLongPtrNoFence(RefCount,
                                                        NewValue,
                                                        OldValue);

            if (NewValue == OldValue) {
                return TRUE;
            }

            OldValue = NewValue;

        } else if ((ULONG_PTR)NewValue == Bias) {
            return FALSE;

        } else {
            __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
        }
    }
}

FORCEINLINE
BOOLEAN
RtlDecrementReferenceCount (
    _Inout_ PRTL_REFERENCE_COUNT RefCount
    )

/*++

Routine Description:

    This function reduces the specified reference count, potentially triggering
    the destruction of the guarded object.

Arguments:

    RefCount - Supplies a pointer to a reference count.

Return Value:

    TRUE if the object should be destroyed, FALSE otherwise.

--*/

{

    RTL_REFERENCE_COUNT NewValue;

    //
    // A release fence is required to ensure all guarded memory accesses are
    // complete before any thread can begin destroying the object.
    //

    NewValue = RtlDecrementLongPtrRelease(RefCount);
    if (NewValue > 0) {
        return FALSE;

    } else if (NewValue == 0) {

        //
        // An acquire fence is required before object destruction to ensure
        // that the destructor cannot observe values changing on other threads.
        //

        RtlBarrierAfterInterlock();
        return TRUE;
    }

    __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
    return FALSE;
}

FORCEINLINE
BOOLEAN
RtlDecrementReferenceCountEx (
    _Inout_ PRTL_REFERENCE_COUNT RefCount,
    _In_ ULONG Bias
    )

/*++

Routine Description:

    This function reduces the specified reference count by the specified amount,
    potentially triggering the destruction of the guarded object.

Arguments:

    RefCount - Supplies a pointer to a reference count.

    Bias - Supplies a reference bias amount.

Return Value:

    TRUE if the object should be destroyed, FALSE otherwise.

--*/

{

    RTL_REFERENCE_COUNT NewValue;

    //
    // A release fence is required to ensure all guarded memory accesses are
    // complete before any thread can begin destroying the object.
    //

    NewValue = RtlExchangeAddLongPtrRelease(RefCount, -(LONG)Bias) - Bias;
    if (NewValue > 0) {
        return FALSE;

    } else if (NewValue == 0) {

        //
        // An acquire fence is required before object destruction to ensure
        // that the destructor cannot observe values changing on other threads.
        //

        RtlBarrierAfterInterlock();
        return TRUE;
    }

    __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
    return FALSE;
}

FORCEINLINE
BOOLEAN
RtlDecrementReferenceCountNonZero (
    _Inout_ volatile RTL_REFERENCE_COUNT *RefCount,
    _In_ ULONG Bias
    )

/*++

Routine Description:

    This function reduces the specified reference count by the specified amount,
    unless doing so would result in a zero value.

Arguments:

    RefCount - Supplies a pointer to a reference count.

    Bias - Supplies a reference bias amount.

Return Value:

    TRUE if the reference count would be zero, FALSE otherwise.

--*/

{

    RTL_REFERENCE_COUNT NewValue;
    RTL_REFERENCE_COUNT OldValue;

    PrefetchForWrite(RefCount);
    OldValue = ReadLongPtrNoFence(RefCount);
    for (;;) {
        NewValue = OldValue - Bias;
        if (NewValue > 0) {

            //
            // A release fence is required to ensure all guarded memory
            // accesses are complete before any thread can begin destroying
            // the object.
            //

            NewValue = RtlCompareExchangeLongPtrRelease(RefCount,
                                                        NewValue,
                                                        OldValue);

            if (NewValue == OldValue) {
                return FALSE;
            }

            OldValue = NewValue;

        } else if (NewValue == 0) {
            return TRUE;

        } else {
            __fastfail(FAST_FAIL_INVALID_REFERENCE_COUNT);
        }
    }
}

#undef RtlIncrementLongPtrNoFence
#undef RtlDecrementLongPtrRelease
#undef RtlExchangeAddLongPtrNoFence
#undef RtlExchangeAddLongPtrRelease
#undef RtlCompareExchangeLongPtrNoFence
#undef RtlCompareExchangeLongPtrRelease
#undef RtlBarrierAfterInterlock
