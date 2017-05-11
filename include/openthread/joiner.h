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
 *   This file includes functions for the Thread Joiner role.
 */

#ifndef OPENTHREAD_JOINER_H_
#define OPENTHREAD_JOINER_H_

#include <openthread/types.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-joiner
 *
 * @brief
 *   This module includes functions for the Thread Joiner role.
 *
 * @{
 *
 */

/**
 * This function pointer is called to notify the completion of a join operation.
 *
 * @param[in]  aError    kThreadError_None if the join process succeeded.
 *                       kThreadError_Security if the join process failed due to security credentials.
 *                       kThreadError_NotFound if no joinable network was discovered.
 *                       kThreadError_ResponseTimeout if a response timed out.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otJoinerCallback)(ThreadError aError, void *aContext);

/**
 * This function enables the Thread Joiner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPSKd             A pointer to the PSKd.
 * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
 * @param[in]  aVendorName       A pointer to the Vendor Name (must be static).
 * @param[in]  aVendorModel      A pointer to the Vendor Model (must be static).
 * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (must be static).
 * @param[in]  aVendorData       A pointer to the Vendor Data (must be static).
 * @param[in]  aCallback         A pointer to a function that is called when the join operation completes.
 * @param[in]  aContext          A pointer to application-specific context.
 *
 * @retval kThreadError_None         Successfully started the Commissioner role.
 * @retval kThreadError_InvalidArgs  @p aPSKd or @p aProvisioningUrl is invalid.
 *
 */
OTAPI ThreadError OTCALL otJoinerStart(otInstance *aInstance, const char *aPSKd, const char *aProvisioningUrl,
                                       const char *aVendorName, const char *aVendorModel,
                                       const char *aVendorSwVersion, const char *aVendorData,
                                       otJoinerCallback aCallback, void *aContext);

/**
 * This function disables the Thread Joiner role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
OTAPI ThreadError OTCALL otJoinerStop(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // OPENTHREAD_JOINER_H_
