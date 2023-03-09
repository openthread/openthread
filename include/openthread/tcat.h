/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *  This file defines the top-level functions for the OpenThread TCAT.
 *
 *  @note
 *   The functions in this module require the build-time feature `OPENTHREAD_CONFIG_BLE_TCAT_ENABLE=1`.
 *
 *  @note
 *   To enable cipher suite DTLS_PSK_WITH_AES_128_CCM_8, MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
 *    must be enabled in mbedtls-config.h
 *   To enable cipher suite DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
 *    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED must be enabled in mbedtls-config.h.
 */

#ifndef OPENTHREAD_TCAT_H_
#define OPENTHREAD_TCAT_H_

#include <stdint.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-ble-secure
 *
 * @brief
 *   This module includes functions that implement TCAT communication.
 *
 *   The functions in this module are available when TCAT feature
 *   (`OPENTHREAD_CONFIG_BLE_TCAT_ENABLE`) is enabled.
 *
 * @{
 *
 */

#define OT_TCAT_MAX_SERVICE_NAME_LENGTH \
    15 ///< Maximum string length of a UDP or TCP service name (does not include null char).

/**
 * Represents TCAT status code.
 *
 */
typedef enum otTcatStatusCode
{
    OT_TCAT_STATUS_SUCCESS       = 0, ///< Command or request was successfully processed
    OT_TCAT_STATUS_UNSUPPORTED   = 1, ///< Requested command or received TLV is not supported
    OT_TCAT_STATUS_PARSE_ERROR   = 2, ///< Request / command could not be parsed correctly
    OT_TCAT_STATUS_VALUE_ERROR   = 3, ///< The value of the transmitted TLV has an error
    OT_TCAT_STATUS_GENERAL_ERROR = 4, ///< An error not matching any other category occurred
    OT_TCAT_STATUS_BUSY          = 5, ///< Command cannot be executed because the resource is busy
    OT_TCAT_STATUS_UNDEFINED    = 6, ///< The requested value, data or service is not defined (currently) or not present
    OT_TCAT_STATUS_HASH_ERROR   = 7, ///< The hash value presented by the commissioner was incorrect
    OT_TCAT_STATUS_UNAUTHORIZED = 16, ///< Sender does not have sufficient authorization for the given command

} otTcatStatusCode;

/**
 * Represents TCAT application protocol.
 *
 */
typedef enum otTcatApplicationProtocol
{
    OT_TCAT_APPLICATION_PROTOCOL_NONE   = 0, ///< Message which has been sent without activating the TCAT agent
    OT_TCAT_APPLICATION_PROTOCOL_STATUS = 1, ///< Message directed to a UDP service
    OT_TCAT_APPLICATION_PROTOCOL_TCP    = 2, ///< Message directed to a TCP service

} otTcatApplicationProtocol;

/**
 * Represents a TCAT command class.
 *
 */
typedef enum otTcatCommandClass
{
    OT_TCAT_COMMAND_CLASS_GENERAL         = 0, ///< TCAT commands related to general operations
    OT_TCAT_COMMAND_CLASS_COMMISSIONING   = 1, ///< TCAT commands related to commissioning
    OT_TCAT_COMMAND_CLASS_EXTRACTION      = 2, ///< TCAT commands related to key extraction
    OT_TCAT_COMMAND_CLASS_DECOMMISSIONING = 3, ///< TCAT commands related to de-commissioning
    OT_TCAT_COMMAND_CLASS_APPLICATION     = 4, ///< TCAT commands related to application layer

} otTcatCommandClass;

/**
 * This structure represents a TCAT vendor information.
 *
 * The content of this structure MUST persist and remain unchanged while a TCAT session is running.
 *
 */
typedef struct otTcatVendorInfo
{
    const char *mProvisioningUrl; ///< Provisioning URL path string
    const char *mVendorName;      ///< Vendor name string
    const char *mVendorModel;     ///< Vendor model string
    const char *mVendorSwVersion; ///< Vendor software version string
    const char *mVendorData;      ///< Vendor specific data string
    const char *mPskdString;      ///< Vendor managed pre-shared key for device
    const char *mInstallCode;     ///< Vendor managed install code string
    const char *mDeviceId; ///< Vendor managed device ID string (if NULL: device ID is set to EUI-64 in binary format)

} otTcatVendorInfo;

/**
 * Pointer to call when application data was received over a TCAT TLS connection.
 *
 * @param[in]  aInstance                 A pointer to an OpenThread instance.
 * @param[in]  aMessage                  A pointer to the message.
 * @param[in]  aOffset                   The offset where the application data begins.
 * @param[in]  aTcatApplicationProtocol  The protocol type of the message received.
 * @param[in]  aServiceName              The name of the service the message is direced to.
 * @param[in]  aContext                  A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleTcatApplicationDataReceive)(otInstance               *aInstance,
                                                   const otMessage          *aMessage,
                                                   int32_t                   aOffset,
                                                   otTcatApplicationProtocol aTcatApplicationProtocol,
                                                   const char               *aServiceName,
                                                   void                     *aContext);

/**
 * Pointer to call to notify the completion of a join operation.
 *
 * @param[in]  aError           OT_ERROR_NONE if the join process succeeded.
 *                              OT_ERROR_SECURITY if the join process failed due to security credentials.
 * @param[in]  aContext         A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleTcatJoin)(otError aError, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OPENTHREAD_TCAT_H_ */
