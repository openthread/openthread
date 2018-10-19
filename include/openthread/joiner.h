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

#include <openthread/platform/radio.h>
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
 * This enumeration defines the Joiner State.
 *
 */
typedef enum otJoinerState
{
    OT_JOINER_STATE_IDLE      = 0,
    OT_JOINER_STATE_DISCOVER  = 1,
    OT_JOINER_STATE_CONNECT   = 2,
    OT_JOINER_STATE_CONNECTED = 3,
    OT_JOINER_STATE_ENTRUST   = 4,
    OT_JOINER_STATE_JOINED    = 5,
} otJoinerState;

/**
 * This function pointer is called to notify the completion of a join operation.
 *
 * @param[in]  aError    OT_ERROR_NONE if the join process succeeded.
 *                       OT_ERROR_SECURITY if the join process failed due to security credentials.
 *                       OT_ERROR_NOT_FOUND if no joinable network was discovered.
 *                       OT_ERROR_RESPONSE_TIMEOUT if a response timed out.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void(OTCALL *otJoinerCallback)(otError aError, void *aContext);

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
 * @retval OT_ERROR_NONE              Successfully started the Commissioner role.
 * @retval OT_ERROR_INVALID_ARGS      @p aPSKd or @p aProvisioningUrl is invalid.
 * @retval OT_ERROR_DISABLED_FEATURE  The Joiner feature is not enabled in this build.
 *
 */
OTAPI otError OTCALL otJoinerStart(otInstance *     aInstance,
                                   const char *     aPSKd,
                                   const char *     aProvisioningUrl,
                                   const char *     aVendorName,
                                   const char *     aVendorModel,
                                   const char *     aVendorSwVersion,
                                   const char *     aVendorData,
                                   otJoinerCallback aCallback,
                                   void *           aContext);

/**
 * This function disables the Thread Joiner role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE              Successfully disabled the Joiner role.
 * @retval OT_ERROR_DISABLED_FEATURE  The Joiner feature is not enabled in this build.
 *
 */
OTAPI otError OTCALL otJoinerStop(otInstance *aInstance);

/**
 * This function returns the Joiner State.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_JOINER_STATE_IDLE
 * @retval OT_JOINER_STATE_DISCOVER
 * @retval OT_JOINER_STATE_CONNECT
 * @retval OT_JOINER_STATE_CONNECTED
 * @retval OT_JOINER_STATE_ENTRUST
 * @retval OT_JOINER_STATE_JOINED
 *
 */
OTAPI otJoinerState OTCALL otJoinerGetState(otInstance *aInstance);

/**
 * Get the Joiner ID.
 *
 * Joiner ID is the first 64 bits of the result of computing SHA-256 over factory-assigned
 * IEEE EUI-64, which is used as IEEE 802.15.4 Extended Address during commissioning process.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 * @param[out]  aJoinerId  A pointer to where the Joiner ID is placed.
 *
 * @retval OT_ERROR_NONE              Successfully retrieved the Joiner ID.
 * @retval OT_ERROR_DISABLED_FEATURE  The Joiner feature is not enabled in this build.
 *
 */
OTAPI otError OTCALL otJoinerGetId(otInstance *aInstance, otExtAddress *aJoinerId);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_JOINER_H_
