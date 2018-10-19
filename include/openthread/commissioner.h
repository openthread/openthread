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
 *   This file includes functions for the Thread Commissioner role.
 */

#ifndef OPENTHREAD_COMMISSIONER_H_
#define OPENTHREAD_COMMISSIONER_H_

#include <openthread/dataset.h>
#include <openthread/ip6.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-commissioner
 *
 * @brief
 *   This module includes functions for the Thread Commissioner role.
 *
 * @{
 *
 */

/**
 * This enumeration defines the Commissioner State.
 *
 */
typedef enum otCommissionerState
{
    OT_COMMISSIONER_STATE_DISABLED = 0, ///< Commissioner role is disabled.
    OT_COMMISSIONER_STATE_PETITION = 1, ///< Currently petitioning to become a Commissioner.
    OT_COMMISSIONER_STATE_ACTIVE   = 2, ///< Commissioner role is active.
} otCommissionerState;

#define OT_COMMISSIONING_PASSPHRASE_MIN_SIZE 6   ///< Minimum size of the Commissioning Passphrase
#define OT_COMMISSIONING_PASSPHRASE_MAX_SIZE 255 ///< Maximum size of the Commissioning Passphrase

#define OT_STEERING_DATA_MAX_LENGTH 16 ///< Max steering data length (bytes)

/**
 * This structure represents the steering data.
 *
 */
typedef struct otSteeringData
{
    uint8_t mLength;                         ///< Length of steering data (bytes)
    uint8_t m8[OT_STEERING_DATA_MAX_LENGTH]; ///< Byte values
} otSteeringData;

/**
 * This structure represents a Commissioning Dataset.
 *
 */
typedef struct otCommissioningDataset
{
    uint16_t       mLocator;       ///< Border Router RLOC16
    uint16_t       mSessionId;     ///< Commissioner Session Id
    otSteeringData mSteeringData;  ///< Steering Data
    uint16_t       mJoinerUdpPort; ///< Joiner UDP Port

    bool mIsLocatorSet : 1;       ///< TRUE if Border Router RLOC16 is set, FALSE otherwise.
    bool mIsSessionIdSet : 1;     ///< TRUE if Commissioner Session Id is set, FALSE otherwise.
    bool mIsSteeringDataSet : 1;  ///< TRUE if Steering Data is set, FALSE otherwise.
    bool mIsJoinerUdpPortSet : 1; ///< TRUE if Joiner UDP Port is set, FALSE otherwise.
} otCommissioningDataset;

/**
 * This function enables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully started the Commissioner role.
 * @retval OT_ERROR_INVALID_STATE  Commissioner is already started.
 *
 */
OTAPI otError OTCALL otCommissionerStart(otInstance *aInstance);

/**
 * This function disables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully stopped the Commissioner role.
 * @retval OT_ERROR_INVALID_STATE  Commissioner is already stopped.
 *
 */
OTAPI otError OTCALL otCommissionerStop(otInstance *aInstance);

/**
 * This function adds a Joiner entry.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aEui64             A pointer to the Joiner's IEEE EUI-64 or NULL for any Joiner.
 * @param[in]  aPSKd              A pointer to the PSKd.
 * @param[in]  aTimeout           A time after which a Joiner is automatically removed, in seconds.
 *
 * @retval OT_ERROR_NONE          Successfully added the Joiner.
 * @retval OT_ERROR_NO_BUFS       No buffers available to add the Joiner.
 * @retval OT_ERROR_INVALID_ARGS  @p aEui64 or @p aPSKd is invalid.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
OTAPI otError OTCALL otCommissionerAddJoiner(otInstance *        aInstance,
                                             const otExtAddress *aEui64,
                                             const char *        aPSKd,
                                             uint32_t            aTimeout);

/**
 * This function removes a Joiner entry.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aEui64             A pointer to the Joiner's IEEE EUI-64 or NULL for any Joiner.
 *
 * @retval OT_ERROR_NONE          Successfully removed the Joiner.
 * @retval OT_ERROR_NOT_FOUND     The Joiner specified by @p aEui64 was not found.
 * @retval OT_ERROR_INVALID_ARGS  @p aEui64 is invalid.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
OTAPI otError OTCALL otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aEui64);

/**
 * This function gets the Provisioning URL.
 *
 * @param[in]    aInstance       A pointer to an OpenThread instance.
 * @param[out]   aLength         A pointer to `uint16_t` to return the length (number of chars) in the URL string.
 *
 * Note that the returned URL string buffer is not necessarily null-terminated.
 *
 * @returns A pointer to char buffer containing the URL string, or NULL if @p aLength is NULL.
 *
 */
const char *otCommissionerGetProvisioningUrl(otInstance *aInstance, uint16_t *aLength);

/**
 * This function sets the Provisioning URL.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aProvisioningUrl      A pointer to the Provisioning URL (may be NULL).
 *
 * @retval OT_ERROR_NONE          Successfully set the Provisioning URL.
 * @retval OT_ERROR_INVALID_ARGS  @p aProvisioningUrl is invalid.
 *
 */
OTAPI otError OTCALL otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl);

/**
 * This function sends an Announce Begin message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aChannelMask          The channel mask value.
 * @param[in]  aCount                The number of Announcement messages per channel.
 * @param[in]  aPeriod               The time between two successive MLE Announce transmissions (in milliseconds).
 * @param[in]  aAddress              A pointer to the IPv6 destination.
 *
 * @retval OT_ERROR_NONE          Successfully enqueued the Announce Begin message.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate an Announce Begin message.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
OTAPI otError OTCALL otCommissionerAnnounceBegin(otInstance *        aInstance,
                                                 uint32_t            aChannelMask,
                                                 uint8_t             aCount,
                                                 uint16_t            aPeriod,
                                                 const otIp6Address *aAddress);

/**
 * This function pointer is called when the Commissioner receives an Energy Report.
 *
 * @param[in]  aChannelMask       The channel mask value.
 * @param[in]  aEnergyList        A pointer to the energy measurement list.
 * @param[in]  aEnergyListLength  Number of entries in @p aEnergyListLength.
 * @param[in]  aContext           A pointer to application-specific context.
 *
 */
typedef void(OTCALL *otCommissionerEnergyReportCallback)(uint32_t       aChannelMask,
                                                         const uint8_t *aEnergyList,
                                                         uint8_t        aEnergyListLength,
                                                         void *         aContext);

/**
 * This function sends an Energy Scan Query message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aChannelMask          The channel mask value.
 * @param[in]  aCount                The number of energy measurements per channel.
 * @param[in]  aPeriod               The time between energy measurements (milliseconds).
 * @param[in]  aScanDuration         The scan duration for each energy measurement (milliseconds).
 * @param[in]  aAddress              A pointer to the IPv6 destination.
 * @param[in]  aCallback             A pointer to a function called on receiving an Energy Report message.
 * @param[in]  aContext              A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE          Successfully enqueued the Energy Scan Query message.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate an Energy Scan Query message.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
OTAPI otError OTCALL otCommissionerEnergyScan(otInstance *                       aInstance,
                                              uint32_t                           aChannelMask,
                                              uint8_t                            aCount,
                                              uint16_t                           aPeriod,
                                              uint16_t                           aScanDuration,
                                              const otIp6Address *               aAddress,
                                              otCommissionerEnergyReportCallback aCallback,
                                              void *                             aContext);

/**
 * This function pointer is called when the Commissioner receives a PAN ID Conflict message.
 *
 * @param[in]  aPanId             The PAN ID value.
 * @param[in]  aChannelMask       The channel mask value.
 * @param[in]  aContext           A pointer to application-specific context.
 *
 */
typedef void(OTCALL *otCommissionerPanIdConflictCallback)(uint16_t aPanId, uint32_t aChannelMask, void *aContext);

/**
 * This function sends a PAN ID Query message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aPanId                The PAN ID to query.
 * @param[in]  aChannelMask          The channel mask value.
 * @param[in]  aAddress              A pointer to the IPv6 destination.
 * @param[in]  aCallback             A pointer to a function called on receiving a PAN ID Conflict message.
 * @param[in]  aContext              A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE          Successfully enqueued the PAN ID Query message.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate a PAN ID Query message.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
OTAPI otError OTCALL otCommissionerPanIdQuery(otInstance *                        aInstance,
                                              uint16_t                            aPanId,
                                              uint32_t                            aChannelMask,
                                              const otIp6Address *                aAddress,
                                              otCommissionerPanIdConflictCallback aCallback,
                                              void *                              aContext);

/**
 * This function sends MGMT_COMMISSIONER_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 */
OTAPI otError OTCALL otCommissionerSendMgmtGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength);

/**
 * This function sends MGMT_COMMISSIONER_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to commissioning dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 */
OTAPI otError OTCALL otCommissionerSendMgmtSet(otInstance *                  aInstance,
                                               const otCommissioningDataset *aDataset,
                                               const uint8_t *               aTlvs,
                                               uint8_t                       aLength);

/**
 * This function returns the Commissioner Session ID.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current commissioner session id.
 *
 */
OTAPI uint16_t OTCALL otCommissionerGetSessionId(otInstance *aInstance);

/**
 * This function returns the Commissioner State.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_COMMISSIONER_STATE_DISABLED  Commissioner disabled.
 * @retval OT_COMMISSIONER_STATE_PETITION  Becoming the commissioner.
 * @retval OT_COMMISSIONER_STATE_ACTIVE    Commissioner enabled.
 *
 */
OTAPI otCommissionerState OTCALL otCommissionerGetState(otInstance *aInstance);

/**
 * This method generates PSKc.
 *
 * PSKc is used to establish the Commissioner Session.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPassPhrase   The commissioning passphrase.
 * @param[in]  aNetworkName  The network name for PSKc computation.
 * @param[in]  aExtPanId     The extended pan id for PSKc computation.
 * @param[out] aPSKc         A pointer to the generated PSKc.
 *
 * @retval OT_ERROR_NONE          Successfully generate PSKc.
 * @retval OT_ERROR_INVALID_ARGS  If any of the input arguments is invalid.
 *
 */
OTAPI otError OTCALL otCommissionerGeneratePSKc(otInstance *           aInstance,
                                                const char *           aPassPhrase,
                                                const char *           aNetworkName,
                                                const otExtendedPanId *aExtPanId,
                                                uint8_t *              aPSKc);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_COMMISSIONER_H_
