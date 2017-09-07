/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *  This file defines the top-level OpenThread APIs related to fault injection
 */

#ifndef OPENTHREAD_FAULT_INJECTION_H_
#define OPENTHREAD_FAULT_INJECTION_H_

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-message
 *
 * @brief
 *   This module includes functions that control OpenThread's fault injection Manager
 *
 * @{
 *
 */

typedef enum
{   
    kFault_AllocBuffer,                /**< Fail the allocation of a Buffer */
    kFault_DropRadioRx,                /**< Drop radio frames in input */
    kFault_NumItems,
} otFaultId;


int32_t otFIFailAtFault(otFaultId id, uint32_t numCallsToSkip, uint32_t numCallsToFail);

bool otFIParseFaultInjectionStr(char *inStr);


/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_FAULT_INJECTION_H_
