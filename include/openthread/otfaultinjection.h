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
 * @addtogroup api-faultinjection
 *
 * @brief
 *   This module includes functions that control OpenThread's fault injection Manager
 *
 * @{
 *
 */

typedef enum
{
    kFault_AllocBuffer,        /**< Fail the allocation of a Buffer */
    kFault_DropRadioRx,        /**< Drop radio frames in input */
    kFault_NumFaultIds,
} otFaultId;


/**
 * Configure a fault to trigger a given number of times, at some point in the future.
 *
 * @param[in]   id              The fault id.
 * @param[in]   numCallsToSkip  The number of instances of the fault that should be skipped.
 *                              before injecting the first failure.
 * @param[in]   numCallsToFail  The number of times the fault should be injected.
 *
 * @retval      0               Successfully configured the fault.
 * @retval      EINVAL          The fault id is out of range.
 */
int32_t otFIFailAtFault(otFaultId id, uint32_t numCallsToSkip, uint32_t numCallsToFail);

/**
 * Parse an nlfaultinjection configuration string to apply it to OpenThread's
 * faultinjection Manager.
 *
 * @param[in]   inStr           The configuration string. An example of a valid string that
 *                              enables two faults is "OpenThread_AllocBuffer_f5_s1:OpenThread_DropRadioRx_f1_s3"
 *                              The format of a single fault configuration is
 *                              "OpenThread_<faultName>_{f<numTimesToFail>[_s<numTimesToSkip>],p<randomFailurePercentage>}[_a<integer>]...".
 *                              All faults in the string must be in the scope of the OpenThread fault injection Manager.
 *
 * @retval true                 Successfully configured all faults.
 * @retval false                Parsing failed. If the string contains more.
 *                              than one fault, the configuration might have been partially applied.
 */
bool otFIParseFaultInjectionStr(char *inStr);

/**
 * Reset the fault injection counters.
 */
void otFIResetCounters(void);

/**
 * Reset the fault injection configuration.
 */
void otFIResetConfiguration(void);

/**
 * Return the name of OpenThread's fault injection Manager.
 *
 * @return The string "OpenThread"
 */
const char *otFIGetManagerName(void);

/**
 * Return the name of a faultId.
 *
 * @param[in]   id  The otFaultId.
 *
 * @return      The string containing the name of the fault. NULL if id is out of range.
 */
const char *otFIGetFaultName(otFaultId id);

/**
 * Return the number of times a fault id was evaluated for injection.
 *
 * @param[in]   id     The otFaultId.
 * @param[out]  value  A pointer to a uinsigned integer in which to store the value.
 *
 * @retval      0      Success
 * @retval      EINVAL If id is out of range.
 */
int otFIGetFaultCounterValue(otFaultId id, uint32_t *value);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_FAULT_INJECTION_H_
