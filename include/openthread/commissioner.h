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
#include <openthread/joiner.h>
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
 * Defines the Commissioner State.
 *
 */
typedef enum otCommissionerState
{
    OT_COMMISSIONER_STATE_DISABLED = 0, ///< Commissioner role is disabled.
    OT_COMMISSIONER_STATE_PETITION = 1, ///< Currently petitioning to become a Commissioner.
    OT_COMMISSIONER_STATE_ACTIVE   = 2, ///< Commissioner role is active.
} otCommissionerState;

/**
 * Defines a Joiner Event on the Commissioner.
 *
 */
typedef enum otCommissionerJoinerEvent
{
    OT_COMMISSIONER_JOINER_START     = 0,
    OT_COMMISSIONER_JOINER_CONNECTED = 1,
    OT_COMMISSIONER_JOINER_FINALIZE  = 2,
    OT_COMMISSIONER_JOINER_END       = 3,
    OT_COMMISSIONER_JOINER_REMOVED   = 4,
} otCommissionerJoinerEvent;

#define OT_COMMISSIONING_PASSPHRASE_MIN_SIZE 6   ///< Minimum size of the Commissioning Passphrase
#define OT_COMMISSIONING_PASSPHRASE_MAX_SIZE 255 ///< Maximum size of the Commissioning Passphrase

#define OT_PROVISIONING_URL_MAX_SIZE 64 ///< Max size (number of chars) in Provisioning URL string (excludes null char).

#define OT_STEERING_DATA_MAX_LENGTH 16 ///< Max steering data length (bytes)

/**
 * Represents the steering data.
 *
 */
typedef struct otSteeringData
{
    uint8_t mLength;                         ///< Length of steering data (bytes)
    uint8_t m8[OT_STEERING_DATA_MAX_LENGTH]; ///< Byte values
} otSteeringData;

/**
 * Represents a Commissioning Dataset.
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
    bool mHasExtraTlv : 1;        ///< TRUE if the Dataset contains any extra unknown sub-TLV, FALSE otherwise.
} otCommissioningDataset;

#define OT_JOINER_MAX_PSKD_LENGTH 32 ///< Maximum string length of a Joiner PSKd (does not include null char).

/**
 * Represents a Joiner PSKd.
 *
 */
typedef struct otJoinerPskd
{
    char m8[OT_JOINER_MAX_PSKD_LENGTH + 1]; ///< Char string array (must be null terminated - +1 is for null char).
} otJoinerPskd;

/**
 * Defines a Joiner Info Type.
 *
 */
typedef enum otJoinerInfoType
{
    OT_JOINER_INFO_TYPE_ANY       = 0, ///< Accept any Joiner (no EUI64 or Discerner is specified).
    OT_JOINER_INFO_TYPE_EUI64     = 1, ///< Joiner EUI-64 is specified (`mSharedId.mEui64` in `otJoinerInfo`).
    OT_JOINER_INFO_TYPE_DISCERNER = 2, ///< Joiner Discerner is specified (`mSharedId.mDiscerner` in `otJoinerInfo`).
} otJoinerInfoType;

/**
 * Represents a Joiner Info.
 *
 */
typedef struct otJoinerInfo
{
    otJoinerInfoType mType; ///< Joiner type.
    union
    {
        otExtAddress      mEui64;     ///< Joiner EUI64 (when `mType` is `OT_JOINER_INFO_TYPE_EUI64`)
        otJoinerDiscerner mDiscerner; ///< Joiner Discerner (when `mType` is `OT_JOINER_INFO_TYPE_DISCERNER`)
    } mSharedId;                      ///< Shared fields
    otJoinerPskd mPskd;               ///< Joiner PSKd
    uint32_t     mExpirationTime;     ///< Joiner expiration time in msec
} otJoinerInfo;

/**
 * Pointer is called whenever the commissioner state changes.
 *
 * @param[in]  aState    The Commissioner state.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otCommissionerStateCallback)(otCommissionerState aState, void *aContext);

/**
 * Pointer is called whenever the joiner state changes.
 *
 * @param[in]  aEvent       The joiner event type.
 * @param[in]  aJoinerInfo  A pointer to the Joiner Info.
 * @param[in]  aJoinerId    A pointer to the Joiner ID (if not known, it will be NULL).
 * @param[in]  aContext     A pointer to application-specific context.
 *
 */
typedef void (*otCommissionerJoinerCallback)(otCommissionerJoinerEvent aEvent,
                                             const otJoinerInfo       *aJoinerInfo,
                                             const otExtAddress       *aJoinerId,
                                             void                     *aContext);

/**
 * Enables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aStateCallback    A pointer to a function that is called when the commissioner state changes.
 * @param[in]  aJoinerCallback   A pointer to a function that is called with a joiner event occurs.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE           Successfully started the Commissioner service.
 * @retval OT_ERROR_ALREADY        Commissioner is already started.
 * @retval OT_ERROR_INVALID_STATE  Device is not currently attached to a network.
 *
 */
otError otCommissionerStart(otInstance                  *aInstance,
                            otCommissionerStateCallback  aStateCallback,
                            otCommissionerJoinerCallback aJoinerCallback,
                            void                        *aCallbackContext);

/**
 * Disables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE     Successfully stopped the Commissioner service.
 * @retval OT_ERROR_ALREADY  Commissioner is already stopped.
 *
 */
otError otCommissionerStop(otInstance *aInstance);

/**
 * Returns the Commissioner Id.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @returns The Commissioner Id.
 *
 */
const char *otCommissionerGetId(otInstance *aInstance);

/**
 * Sets the Commissioner Id.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aId           A pointer to a string character array. Must be null terminated.
 *
 * @retval OT_ERROR_NONE            Successfully set the Commissioner Id.
 * @retval OT_ERROR_INVALID_ARGS    Given name is too long.
 * @retval OT_ERROR_INVALID_STATE   The commissioner is active and id cannot be changed.
 *
 */
otError otCommissionerSetId(otInstance *aInstance, const char *aId);

/**
 * Adds a Joiner entry.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aEui64             A pointer to the Joiner's IEEE EUI-64 or NULL for any Joiner.
 * @param[in]  aPskd              A pointer to the PSKd.
 * @param[in]  aTimeout           A time after which a Joiner is automatically removed, in seconds.
 *
 * @retval OT_ERROR_NONE          Successfully added the Joiner.
 * @retval OT_ERROR_NO_BUFS       No buffers available to add the Joiner.
 * @retval OT_ERROR_INVALID_ARGS  @p aEui64 or @p aPskd is invalid.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
otError otCommissionerAddJoiner(otInstance         *aInstance,
                                const otExtAddress *aEui64,
                                const char         *aPskd,
                                uint32_t            aTimeout);

/**
 * Adds a Joiner entry with a given Joiner Discerner value.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aDiscerner         A pointer to the Joiner Discerner.
 * @param[in]  aPskd              A pointer to the PSKd.
 * @param[in]  aTimeout           A time after which a Joiner is automatically removed, in seconds.
 *
 * @retval OT_ERROR_NONE          Successfully added the Joiner.
 * @retval OT_ERROR_NO_BUFS       No buffers available to add the Joiner.
 * @retval OT_ERROR_INVALID_ARGS  @p aDiscerner or @p aPskd is invalid.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
otError otCommissionerAddJoinerWithDiscerner(otInstance              *aInstance,
                                             const otJoinerDiscerner *aDiscerner,
                                             const char              *aPskd,
                                             uint32_t                 aTimeout);

/**
 * Get joiner info at aIterator position.
 *
 * @param[in]      aInstance   A pointer to instance.
 * @param[in,out]  aIterator   A pointer to the Joiner Info iterator context.
 * @param[out]     aJoiner     A reference to Joiner info.
 *
 * @retval OT_ERROR_NONE       Successfully get the Joiner info.
 * @retval OT_ERROR_NOT_FOUND  Not found next Joiner.
 *
 */
otError otCommissionerGetNextJoinerInfo(otInstance *aInstance, uint16_t *aIterator, otJoinerInfo *aJoiner);

/**
 * Removes a Joiner entry.
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
otError otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aEui64);

/**
 * Removes a Joiner entry.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aDiscerner         A pointer to the Joiner Discerner.
 *
 * @retval OT_ERROR_NONE          Successfully removed the Joiner.
 * @retval OT_ERROR_NOT_FOUND     The Joiner specified by @p aEui64 was not found.
 * @retval OT_ERROR_INVALID_ARGS  @p aDiscerner is invalid.
 * @retval OT_ERROR_INVALID_STATE The commissioner is not active.
 *
 * @note Only use this after successfully starting the Commissioner role with otCommissionerStart().
 *
 */
otError otCommissionerRemoveJoinerWithDiscerner(otInstance *aInstance, const otJoinerDiscerner *aDiscerner);

/**
 * Gets the Provisioning URL.
 *
 * @param[in]    aInstance       A pointer to an OpenThread instance.
 *
 * @returns A pointer to the URL string.
 *
 */
const char *otCommissionerGetProvisioningUrl(otInstance *aInstance);

/**
 * Sets the Provisioning URL.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aProvisioningUrl      A pointer to the Provisioning URL (may be NULL to set as empty string).
 *
 * @retval OT_ERROR_NONE          Successfully set the Provisioning URL.
 * @retval OT_ERROR_INVALID_ARGS  @p aProvisioningUrl is invalid (too long).
 *
 */
otError otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl);

/**
 * Sends an Announce Begin message.
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
otError otCommissionerAnnounceBegin(otInstance         *aInstance,
                                    uint32_t            aChannelMask,
                                    uint8_t             aCount,
                                    uint16_t            aPeriod,
                                    const otIp6Address *aAddress);

/**
 * Pointer is called when the Commissioner receives an Energy Report.
 *
 * @param[in]  aChannelMask       The channel mask value.
 * @param[in]  aEnergyList        A pointer to the energy measurement list.
 * @param[in]  aEnergyListLength  Number of entries in @p aEnergyListLength.
 * @param[in]  aContext           A pointer to application-specific context.
 *
 */
typedef void (*otCommissionerEnergyReportCallback)(uint32_t       aChannelMask,
                                                   const uint8_t *aEnergyList,
                                                   uint8_t        aEnergyListLength,
                                                   void          *aContext);

/**
 * Sends an Energy Scan Query message.
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
otError otCommissionerEnergyScan(otInstance                        *aInstance,
                                 uint32_t                           aChannelMask,
                                 uint8_t                            aCount,
                                 uint16_t                           aPeriod,
                                 uint16_t                           aScanDuration,
                                 const otIp6Address                *aAddress,
                                 otCommissionerEnergyReportCallback aCallback,
                                 void                              *aContext);

/**
 * Pointer is called when the Commissioner receives a PAN ID Conflict message.
 *
 * @param[in]  aPanId             The PAN ID value.
 * @param[in]  aChannelMask       The channel mask value.
 * @param[in]  aContext           A pointer to application-specific context.
 *
 */
typedef void (*otCommissionerPanIdConflictCallback)(uint16_t aPanId, uint32_t aChannelMask, void *aContext);

/**
 * Sends a PAN ID Query message.
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
otError otCommissionerPanIdQuery(otInstance                         *aInstance,
                                 uint16_t                            aPanId,
                                 uint32_t                            aChannelMask,
                                 const otIp6Address                 *aAddress,
                                 otCommissionerPanIdConflictCallback aCallback,
                                 void                               *aContext);

/**
 * Sends MGMT_COMMISSIONER_GET.
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
otError otCommissionerSendMgmtGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength);

/**
 * Sends MGMT_COMMISSIONER_SET.
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
otError otCommissionerSendMgmtSet(otInstance                   *aInstance,
                                  const otCommissioningDataset *aDataset,
                                  const uint8_t                *aTlvs,
                                  uint8_t                       aLength);

/**
 * Returns the Commissioner Session ID.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current commissioner session id.
 *
 */
uint16_t otCommissionerGetSessionId(otInstance *aInstance);

/**
 * Returns the Commissioner State.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_COMMISSIONER_STATE_DISABLED  Commissioner disabled.
 * @retval OT_COMMISSIONER_STATE_PETITION  Becoming the commissioner.
 * @retval OT_COMMISSIONER_STATE_ACTIVE    Commissioner enabled.
 *
 */
otCommissionerState otCommissionerGetState(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_COMMISSIONER_H_
