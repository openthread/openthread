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
 * @note
 *   The functions in this module require `OPENTHREAD_CONFIG_JOINER_ENABLE=1`.
 *
 * @{
 */

/**
 * Defines the Joiner State.
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

#define OT_JOINER_MAX_DISCERNER_LENGTH 64 ///< Maximum length of a Joiner Discerner in bits.

/**
 * Represents a Joiner Discerner.
 */
typedef struct otJoinerDiscerner
{
    uint64_t mValue;  ///< Discerner value (the lowest `mLength` bits specify the discerner).
    uint8_t  mLength; ///< Length (number of bits) - must be non-zero and at most `OT_JOINER_MAX_DISCERNER_LENGTH`.
} otJoinerDiscerner;

/**
 * Type defines Join operation identifiers for MeshCoP-Ext. It includes both CCM and non-CCM join operations.
 * Identifiers 0-15 SHOULD be used only for operations that go through a Joiner Router. Identifiers >= 16 are
 * for operations that don't use a Joiner Router or operations composed of multiple sub-operations.
 */
typedef enum otJoinOperation
{
    /**
     * Thread Autonomous Enrollment (AE) using the IETF cBRSKI onboarding protocol.
     * This includes receiving the cBRSKI Voucher and doing EST-coaps simple enrollment to get
     * an LDevID (=operational cert) for the local domain.
     */
    OT_JOIN_OPERATION_AE_CBRSKI = 1,

    /**
     * Thread Network Key Provisioning (NKP) to get Thread network key, authenticated by LDevID.
     * Prerequisite is storing an LDevID (=operational cert) for the local domain.
     */
    OT_JOIN_OPERATION_NKP = 2,

    /**
     * Any EST-coaps operation via Joiner Router authenticated with LDevID (e.g. reenrollment).
     * Prerequisite is having a valid/previous LDevID already stored in the Credentials store.
     */
    OT_JOIN_OPERATION_EST_COAPS = 3,

    /**
     * Thread MeshCoP commissioning.
     */
    OT_JOIN_OPERATION_MESHCOP = 8,

    /**
     * Border-Router-specific cBRSKI operation. This is equal to AE_CBRSKI except that the
     * local Thread interface is not used. Instead, the BR will use its infrastructure
     * network interface to contact the cBRSKI Registrar directly, without Thread relaying.
     */
    OT_JOIN_OPERATION_BR_CBRSKI = 16,

    /**
     * CCM do-all operation which will perform AE/cBRSKI, NKP, Thread-start as needed to get
     * a node attached to a Thread Network. It is a meta-operation that calls multiple other
     * operations under the hood.
     */
    OT_JOIN_OPERATION_CCM_ALL = 17,

} otJoinOperation;

/**
 * Pointer is called to notify the completion of a join operation.
 *
 * @param[in]  aError    OT_ERROR_NONE if the join process succeeded.
 *                       OT_ERROR_SECURITY if the join process failed due to security credentials.
 *                       OT_ERROR_NOT_FOUND if no joinable network was discovered.
 *                       OT_ERROR_RESPONSE_TIMEOUT if a response timed out.
 * @param[in]  aContext  A pointer to application-specific context.
 */
typedef void (*otJoinerCallback)(otError aError, void *aContext);

/**
 * Enables the Thread Joiner role for MeshCoP commissioning.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPskd             A pointer to the PSKd.
 * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
 * @param[in]  aVendorName       A pointer to the Vendor Name (may be NULL).
 * @param[in]  aVendorModel      A pointer to the Vendor Model (may be NULL).
 * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (may be NULL).
 * @param[in]  aVendorData       A pointer to the Vendor Data (may be NULL).
 * @param[in]  aCallback         A pointer to a function that is called when the join operation completes.
 * @param[in]  aContext          A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE              Successfully started the Joiner role.
 * @retval OT_ERROR_BUSY              The previous attempt is still on-going.
 * @retval OT_ERROR_INVALID_ARGS      @p aPskd or @p aProvisioningUrl is invalid.
 * @retval OT_ERROR_INVALID_STATE     The IPv6 stack is not enabled or Thread stack is fully enabled.
 */
otError otJoinerStart(otInstance      *aInstance,
                      const char      *aPskd,
                      const char      *aProvisioningUrl,
                      const char      *aVendorName,
                      const char      *aVendorModel,
                      const char      *aVendorSwVersion,
                      const char      *aVendorData,
                      otJoinerCallback aCallback,
                      void            *aContext);

/**
 * Enables the Thread CCM Joiner role and starts the selected join operation. Following operations are
 * supported:
 *
 * `OT_JOIN_OPERATION_AE_CBRSKI` : Thread Autonomous Enrollment (AE) using the IETF cBRSKI protocol.
 * The joiner will first attempt to retrieve a signed Voucher from its manufacturer, to check if joining
 * the current domain is approved. If ok, it will perform EST-CoAPS [RFC 9148] simple-enrollment to get
 * an LDevID (=operational cert). If this operation succeeds, the joiner is ready for NKP.
 *
 * `OT_JOIN_OPERATION_NKP` : Network Key Provisioning (NKP). The Joiner will attempt to retrieve Network
 * Credentials for a discovered ('best') Thread network that is part of its Thread Domain. The domain is
 * encoded in its LDevID X.509v3 certificate. If NKP succeeds, then the Joiner is ready to start Thread
 * and attach to this network. NKP requires an LDevID certificate stored in the device, obtained in some
 * way (e.g. via cBRSKI, or TCAT, or EST-CoAPS).
 *
 * Other methods are not (yet) supported here and will return OT_ERROR_INVALID_ARGS.
 *
 * @note Requires the feature `OPENTHREAD_CONFIG_CCM_ENABLE` to be enabled.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aOperation      The join operation to perform (see options above).
 * @param[in]  aCallback       A pointer to a function that is called when the join operation completes.
 * @param[in]  aContext        A pointer to application-specific context used in callback.
 *
 * @retval OT_ERROR_NONE           Successfully started the CCM Joiner role and requested operation.
 * @retval OT_ERROR_BUSY           A previous join attempt/operation is still on-going.
 * @retval OT_ERROR_INVALID_ARGS   The `aOperation` is not supported.
 * @retval OT_ERROR_INVALID_STATE  The present state is not suitable for the operation, e.g. if already connected to a
 *                                 Thread Network or missing a required prerequisite (e.g. LDevID).
 */
otError otJoinerStartCcm(otInstance *aInstance, otJoinOperation aOperation, otJoinerCallback aCallback, void *aContext);

/**
 * Disables the Thread Joiner role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otJoinerStop(otInstance *aInstance);

/**
 * Gets the Joiner State.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The joiner state.
 */
otJoinerState otJoinerGetState(otInstance *aInstance);

/**
 * Gets the Joiner ID.
 *
 * If a Joiner Discerner is not set, Joiner ID is the first 64 bits of the result of computing SHA-256 over
 * factory-assigned IEEE EUI-64. Otherwise the Joiner ID is calculated from the Joiner Discerner value.
 *
 * The Joiner ID is also used as the device's IEEE 802.15.4 Extended Address during the commissioning process.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 *
 * @returns A pointer to the Joiner ID.
 */
const otExtAddress *otJoinerGetId(otInstance *aInstance);

/**
 * Sets the Joiner Discerner.
 *
 * The Joiner Discerner is used to calculate the Joiner ID during the Thread Commissioning process. For more
 * information, refer to #otJoinerGetId.
 * @note The Joiner Discerner takes the place of the Joiner EUI-64 during the joiner session of Thread Commissioning.
 *
 * @param[in]   aInstance    A pointer to the OpenThread instance.
 * @param[in]   aDiscerner   A pointer to a Joiner Discerner. If NULL clears any previously set discerner.
 *
 * @retval OT_ERROR_NONE           The Joiner Discerner updated successfully.
 * @retval OT_ERROR_INVALID_ARGS   @p aDiscerner is not valid (specified length is not within valid range).
 * @retval OT_ERROR_INVALID_STATE  There is an ongoing Joining process so Joiner Discerner could not be changed.
 */
otError otJoinerSetDiscerner(otInstance *aInstance, otJoinerDiscerner *aDiscerner);

/**
 * Gets the Joiner Discerner. For more information, refer to #otJoinerSetDiscerner.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 *
 * @returns A pointer to Joiner Discerner or NULL if none is set.
 */
const otJoinerDiscerner *otJoinerGetDiscerner(otInstance *aInstance);

/**
 * Converts a given joiner state enumeration value to a human-readable string.
 *
 * @param[in] aState   The joiner state.
 *
 * @returns A human-readable string representation of @p aState.
 */
const char *otJoinerStateToString(otJoinerState aState);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_JOINER_H_
