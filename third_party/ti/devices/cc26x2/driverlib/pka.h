/******************************************************************************
*  Filename:       pka.h
*  Revised:        2017-03-28 11:12:40 +0200 (Tue, 28 Mar 2017)
*  Revision:       48729
*
*  Description:    PKA header file.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
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
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup pka_api
//! @{
//
//*****************************************************************************

#ifndef __PKA_H__
#define __PKA_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_pka.h"
#include "../inc/hw_pka_ram.h"
#include "interrupt.h"
#include "sys_ctrl.h"
#include "debug.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define PKAGetOpsStatus                 NOROM_PKAGetOpsStatus
    #define PKABigNumModStart               NOROM_PKABigNumModStart
    #define PKABigNumModGetResult           NOROM_PKABigNumModGetResult
    #define PKABigNumCmpStart               NOROM_PKABigNumCmpStart
    #define PKABigNumCmpGetResult           NOROM_PKABigNumCmpGetResult
    #define PKABigNumInvModStart            NOROM_PKABigNumInvModStart
    #define PKABigNumInvModGetResult        NOROM_PKABigNumInvModGetResult
    #define PKABigNumMultiplyStart          NOROM_PKABigNumMultiplyStart
    #define PKABigNumMultGetResult          NOROM_PKABigNumMultGetResult
    #define PKABigNumAddStart               NOROM_PKABigNumAddStart
    #define PKABigNumAddGetResult           NOROM_PKABigNumAddGetResult
    #define PKAEccMultiplyStart             NOROM_PKAEccMultiplyStart
    #define PKAEccMultiplyGetResult         NOROM_PKAEccMultiplyGetResult
    #define PKAEccAddStart                  NOROM_PKAEccAddStart
    #define PKAEccAddGetResult              NOROM_PKAEccAddGetResult
#endif

//*****************************************************************************
//
// Function return values
//
//*****************************************************************************
#define PKA_STATUS_SUCCESS             0 //!< Success
#define PKA_STATUS_FAILURE             1 //!< Failure
#define PKA_STATUS_INVALID_PARAM       2 //!< Invalid parameter
#define PKA_STATUS_BUF_UNDERFLOW       3 //!< Buffer underflow
#define PKA_STATUS_RESULT_0            4 //!< Result is all zeros
#define PKA_STATUS_A_GR_B              5 //!< Big number compare return status if the first big number is greater than the second.
#define PKA_STATUS_A_LT_B              6 //!< Big number compare return status if the first big number is less than the second.
#define PKA_STATUS_OPERATION_BUSY      7 //!< PKA operation is in progress.
#define PKA_STATUS_OPERATION_RDY 	   8 //!< No PKA operation is in progress.

//*****************************************************************************
//
//! \brief A structure which contains the pointers to the values of x and y
//! coordinates of the Elliptical Curve point.
//
//*****************************************************************************
typedef struct ECC_CurvePoint {
  uint32_t *x;  //!< Pointer to value of the x coordinate of the EC point.
  uint32_t *y;  //!< Pointer to value of the y coordinate of the EC point.
}
ECC_CurvePoint;

//*****************************************************************************
//
//! \brief A structure which contains the necessary elements of the
//! Elliptical Curve Cryptography's (ECC) prime curve.
//!
//! The equation used to define the curve is expressed in the short Weierstrass
//! form y^3 = x^2 + a*x + b
//
//*****************************************************************************
typedef struct ECC_Curve {
  const uint32_t  lengthInWords; //!< Size of the curve in 32-bit word.
  const uint32_t  *p;            //!< The prime that defines the field of the curve.
  const uint32_t  *n;            //!< Order of the curve.
  const uint32_t  *a;            //!< Coefficient a of the equation.
  const uint32_t  *b;            //!< Coefficient b of the equation.
  const ECC_CurvePoint g;        //!< Generator point of the curve.
}
ECC_Curve;


//*****************************************************************************
//
//! \brief A structure representing the NIST P-256 eliptic curve.
//!
//! The curve structure links constants formatted for use with PKA driverlib.
//
//*****************************************************************************
extern const ECC_Curve NIST_P256;

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Gets the PKA operation status.
//!
//! This function gets information on whether any PKA operation is in
//! progress or not. This function allows to check the PKA operation status
//! before starting any new PKA operation.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA operation is in progress.
//! - \ref PKA_STATUS_OPERATION_RDY if the PKA operation is not in progress.
//
//*****************************************************************************
extern uint32_t  PKAGetOpsStatus(void);

//*****************************************************************************
//
//! \brief Starts a big number modulus operation.
//!
//! This function starts the modulo operation on the big number \c bigNum
//! using the divisor \c modulus. The PKA RAM location where the result
//! will be available is stored in \c resultPKAMemAddr.
//!
//! \param [in] bigNum is the pointer to the big number on which modulo operation
//!        needs to be carried out.
//!
//! \param [in] bigNumLengthInWords is the size of the big number \c bigNum in 32-bit words.
//!
//! \param [in] modulus is the pointer to the divisor.
//!
//! \param [in] modulusLengthInWords is the size of the divisor \c modulus in 32-bit words.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY, if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumModGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumModStart(uint32_t* bigNum, uint32_t bigNumLengthInWords, uint32_t* modulus, uint32_t modulusLengthInWords, uint32_t* resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the big number modulus operation.
//!
//! This function gets the result of the big number modulus operation which was
//! previously started using the function PKABigNumModStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to
//!        be stored.
//!
//! \param [in] lengthInWords is the size of the provided buffer in 32-bit words.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumModStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the \c lengthInWords is less than the length
//!        of the result.
//!
//! \sa PKABigNumModStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumModGetResult(uint32_t* resultBuf, uint32_t lengthInWords, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts the comparison of two big numbers.
//!
//! This function starts the comparison of two big numbers pointed by
//! \c bigNum1 and \c bigNum2.
//!
//! \note \c bigNum1 and \c bigNum2 must have same size.
//!
//! \param [in] bigNum1 is the pointer to the first big number.
//!
//! \param [in] bigNum2 is the pointer to the second big number.
//!
//! \param [in] lengthInWords is the size of the big numbers in 32-bit words.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumCmpGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumCmpStart(uint32_t* bigNum1, uint32_t* bigNum2, uint32_t lengthInWords);

//*****************************************************************************
//
//! \brief Gets the result of the comparison operation of two big numbers.
//!
//! This function provides the results of the comparison of two big numbers
//! which was started using the PKABigNumCmpStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_OPERATION_BUSY if the operation is in progress.
//! - \ref PKA_STATUS_SUCCESS if the two big numbers are equal.
//! - \ref PKA_STATUS_A_GR_B  if the first number is greater than the second.
//! - \ref PKA_STATUS_A_LT_B if the first number is less than the second.
//!
//! \sa PKABigNumCmpStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumCmpGetResult(void);

//*****************************************************************************
//
//! \brief Starts a big number inverse modulo operation.
//!
//! This function starts the inverse modulo operation on \c bigNum
//! using the divisor \c modulus.
//!
//! \param [in] bigNum is the pointer to the buffer containing the big number
//!        (dividend).
//!
//! \param [in] bigNumLengthInWords is the size of the \c bigNum in 32-bit words.
//!
//! \param [in] modulus is the pointer to the buffer containing the divisor.
//!
//! \param [in] modulusLengthInWords is the size of the divisor in 32-bit words.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumInvModGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumInvModStart(uint32_t* bigNum, uint32_t bigNumLengthInWords, uint32_t* modulus, uint32_t modulusLengthInWords, uint32_t* resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Gets the result of the big number inverse modulo operation.
//!
//! This function gets the result of the big number inverse modulo operation
//! previously started using the function PKABigNumInvModStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to be
//!        stored.
//!
//! \param [in] lengthInWords is the size of the provided buffer in 32-bit words.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumInvModStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        than the result.
//!
//! \sa PKABigNumInvModStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumInvModGetResult(uint32_t* resultBuf, uint32_t lengthInWords, uint32_t resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Starts the multiplication of two big numbers.
//!
//! \param [in] multiplicand is the pointer to the buffer containing the big
//!        number multiplicand.
//!
//! \param [in] multiplicandLengthInWords is the size of the multiplicand in 32-bit words.
//!
//! \param [in] multiplier is the pointer to the buffer containing the big
//!        number multiplier.
//!
//! \param [in] multiplierLengthInWords is the size of the multiplier in 32-bit words.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumMultGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumMultiplyStart(uint32_t* multiplicand, uint32_t multiplicandLengthInWords, uint32_t* multiplier, uint32_t multiplierLengthInWords, uint32_t* resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Gets the result of the big number multiplication.
//!
//! This function gets the result of the multiplication of two big numbers
//! operation previously started using the function PKABigNumMultiplyStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to be
//!        stored.
//!
//! \param [in, out] resultLengthInWords is the address of the variable containing the length of the
//!        buffer in 32-bit words. After the operation, the actual length of the resultant is stored
//!        at this address.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumMultiplyStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        then the length of the result.
//!
//! \sa PKABigNumMultiplyStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumMultGetResult(uint32_t* resultBuf, uint32_t* resultLengthInWords, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts the addition of two big numbers.
//!
//! \param [in] bigNum1 is the pointer to the buffer containing the first
//!        big number.
//!
//! \param [in] bigNum1LengthInWords is the size of the first big number in 32-bit words.
//!
//! \param [in] bigNum2 is the pointer to the buffer containing the second
//!        big number.
//!
//! \param [in] bigNum2LengthInWords is the size of the second big number in 32-bit words.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumAddGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumAddStart(uint32_t* bigNum1, uint32_t bigNum1LengthInWords, uint32_t* bigNum2, uint32_t bigNum2LengthInWords, uint32_t* resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the addition operation on two big numbers.
//!
//! \param [out] resultBuf is the pointer to buffer where the result
//!        needs to be stored.
//!
//! \param [in, out] resultLengthInWords is the address of the variable containing
//!        the length of the buffer.  After the operation the actual length of the
//!        resultant is stored at this address.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumAddStart().
//!
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        then the length of the result.
//!
//! \sa PKABigNumAddStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumAddGetResult(uint32_t* resultBuf, uint32_t* resultLengthInWords, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts ECC multiplication.
//!
//! \param [in] scalar is pointer to the buffer containing the scalar
//!        value to be multiplied.
//!
//! \param [in] curvePoint is the pointer to the structure containing the
//!        elliptic curve point to be multiplied.  The point must be on
//!        the given curve.
//!
//! \param [in] curve is the pointer to the structure containing the curve
//!        info.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKAEccMultiplyGetResult()
//
//*****************************************************************************
extern uint32_t  PKAEccMultiplyStart(const uint32_t* scalar, const ECC_CurvePoint* curvePoint, const ECC_Curve* curve, uint32_t* resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of ECC multiplication
//!
//! This function gets the result of ECC point multiplication operation on the
//! EC point and the scalar value, previously started using the function
//! PKAEccMultiplyStart().
//!
//! \param [out] curvePoint is the pointer to the structure where the resultant EC
//!        point will be stored.  The application is responsible for allocating the
//!        memory for the EC point structure and the x and y coordinate as well.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKAEccMultiplyStart().
//!
//! \param [in] curve is a pointer to a structure that defines the elliptic
//!        curve in use.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//!
//! \sa PKAEccMultiplyStart()
//
//*****************************************************************************
extern uint32_t  PKAEccMultiplyGetResult(ECC_CurvePoint* curvePoint, uint32_t resultPKAMemAddr, const ECC_Curve *curve);

//*****************************************************************************
//
//! \brief Starts the ECC addition.
//!
//! \param [in] curvePoint1 is the pointer to the structure containing the first
//!        ECC point.
//!
//! \param [in] curvePoint2 is the pointer to the structure containing the
//!        second ECC point.
//!
//! \param [in] curve is the pointer to the structure containing the curve
//!        info.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKAEccAddGetResult()
//
//*****************************************************************************
extern uint32_t  PKAEccAddStart(const ECC_CurvePoint* curvePoint1, const ECC_CurvePoint* curvePoint2, const ECC_Curve* curve, uint32_t* resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the ECC addition
//!
//! This function gets the result of ECC point addition operation on the
//! on the two given EC points, previously started using the function
//! PKAEccAddStart().
//!
//! \param [out] curvePoint is the pointer to the structure where the resultant
//!        point will be stored. The application is responsible for allocating memory
//!        for the EC point structure including the memory for x and y
//!        coordinate values.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the function PKAEccAddStart().
//!
//! \param [in] curve is a pointer to a structure that defines the elliptic
//!        curve in use.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//!
//! \sa PKAEccAddStart()
//
//*****************************************************************************
extern uint32_t  PKAEccAddGetResult(ECC_CurvePoint* curvePoint, uint32_t resultPKAMemAddr, const ECC_Curve *curve);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_PKAGetOpsStatus
        #undef  PKAGetOpsStatus
        #define PKAGetOpsStatus                 ROM_PKAGetOpsStatus
    #endif
    #ifdef ROM_PKABigNumModStart
        #undef  PKABigNumModStart
        #define PKABigNumModStart               ROM_PKABigNumModStart
    #endif
    #ifdef ROM_PKABigNumModGetResult
        #undef  PKABigNumModGetResult
        #define PKABigNumModGetResult           ROM_PKABigNumModGetResult
    #endif
    #ifdef ROM_PKABigNumCmpStart
        #undef  PKABigNumCmpStart
        #define PKABigNumCmpStart               ROM_PKABigNumCmpStart
    #endif
    #ifdef ROM_PKABigNumCmpGetResult
        #undef  PKABigNumCmpGetResult
        #define PKABigNumCmpGetResult           ROM_PKABigNumCmpGetResult
    #endif
    #ifdef ROM_PKABigNumInvModStart
        #undef  PKABigNumInvModStart
        #define PKABigNumInvModStart            ROM_PKABigNumInvModStart
    #endif
    #ifdef ROM_PKABigNumInvModGetResult
        #undef  PKABigNumInvModGetResult
        #define PKABigNumInvModGetResult        ROM_PKABigNumInvModGetResult
    #endif
    #ifdef ROM_PKABigNumMultiplyStart
        #undef  PKABigNumMultiplyStart
        #define PKABigNumMultiplyStart          ROM_PKABigNumMultiplyStart
    #endif
    #ifdef ROM_PKABigNumMultGetResult
        #undef  PKABigNumMultGetResult
        #define PKABigNumMultGetResult          ROM_PKABigNumMultGetResult
    #endif
    #ifdef ROM_PKABigNumAddStart
        #undef  PKABigNumAddStart
        #define PKABigNumAddStart               ROM_PKABigNumAddStart
    #endif
    #ifdef ROM_PKABigNumAddGetResult
        #undef  PKABigNumAddGetResult
        #define PKABigNumAddGetResult           ROM_PKABigNumAddGetResult
    #endif
    #ifdef ROM_PKAEccMultiplyStart
        #undef  PKAEccMultiplyStart
        #define PKAEccMultiplyStart             ROM_PKAEccMultiplyStart
    #endif
    #ifdef ROM_PKAEccMultiplyGetResult
        #undef  PKAEccMultiplyGetResult
        #define PKAEccMultiplyGetResult         ROM_PKAEccMultiplyGetResult
    #endif
    #ifdef ROM_PKAEccAddStart
        #undef  PKAEccAddStart
        #define PKAEccAddStart                  ROM_PKAEccAddStart
    #endif
    #ifdef ROM_PKAEccAddGetResult
        #undef  PKAEccAddGetResult
        #define PKAEccAddGetResult              ROM_PKAEccAddGetResult
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // __PKA_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
