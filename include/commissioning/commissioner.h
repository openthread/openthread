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
 *   This file includes the platform abstraction for the Thread Commissioner role.
 */

#ifndef OPENTHREAD_COMMISSIONER_H_
#define OPENTHREAD_COMMISSIONER_H_

#include <platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup core-commissioning
 *
 * @{
 *
 */

/**
 * This function enables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None     Successfully started the Commissioner role.
 *
 */
OTAPI ThreadError OTCALL otCommissionerStart(otInstance *aInstance);

/**
 * This function disables the Thread Commissioner role.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None     Successfully started the Commissioner role.
 *
 */
OTAPI ThreadError OTCALL otCommissionerStop(otInstance *aInstance);

/**
 * This function adds a Joiner entry.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aExtAddress           A pointer to the Joiner's extended address or NULL for any Joiner.
 * @param[in]  aPSKd                 A pointer to the PSKd.
 *
 * @retval kThreadError_None         Successfully added the Joiner.
 * @retval kThreadError_NoBufs       No buffers available to add the Joiner.
 * @retval kThreadError_InvalidArgs  @p aExtAddress or @p aPSKd is invalid.
 *
 */
OTAPI ThreadError OTCALL otCommissionerAddJoiner(otInstance *aInstance, const otExtAddress *aExtAddress,
                                                 const char *aPSKd);

/**
 * This function removes a Joiner entry.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aExtAddress           A pointer to the Joiner's extended address or NULL for any Joiner.
 *
 * @retval kThreadError_None         Successfully added the Joiner.
 * @retval kThreadError_NotFound     The Joiner specified by @p aExtAddress was not found.
 * @retval kThreadError_InvalidArgs  @p aExtAddress is invalid.
 *
 */
OTAPI ThreadError OTCALL otCommissionerRemoveJoiner(otInstance *aIntsance, const otExtAddress *aExtAddress);

/**
 * This function sets the Provisioning URL.
 *
 * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
 *
 * @retval kThreadError_None         Successfully added the Joiner.
 * @retval kThreadError_InvalidArgs  @p aProvisioningUrl is invalid.
 *
 */
OTAPI ThreadError OTCALL otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl);

/**
 * This function sends an Announce Begin message.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aChannelMask   The channel mask value.
 * @param[in]  aCount         The number of energy measurements per channel.
 * @param[in]  aPeriod        The time between energy measurements (milliseconds).
 * @param[in]  aAddress       A pointer to the IPv6 destination.
 *
 * @retval kThreadError_None    Successfully enqueued the Announce Begin message.
 * @retval kThreadError_NoBufs  Insufficient buffers to generate an Announce Begin message.
 *
 */
OTAPI ThreadError OTCALL otCommissionerAnnounceBegin(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount,
                                                     uint16_t aPeriod,
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
typedef void (OTCALL *otCommissionerEnergyReportCallback)(uint32_t aChannelMask, const uint8_t *aEnergyList,
                                                          uint8_t aEnergyListLength, void *aContext);

/**
 * This function sends an Energy Scan Query message.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aChannelMask   The channel mask value.
 * @param[in]  aCount         The number of energy measurements per channel.
 * @param[in]  aPeriod        The time between energy measurements (milliseconds).
 * @param[in]  aScanDuration  The scan duration for each energy measurement (milliseconds).
 * @param[in]  aAddress       A pointer to the IPv6 destination.
 * @param[in]  aCallback      A pointer to a function called on receiving an Energy Report message.
 * @param[in]  aContext       A pointer to application-specific context.
 *
 * @retval kThreadError_None    Successfully enqueued the Energy Scan Query message.
 * @retval kThreadError_NoBufs  Insufficient buffers to generate an Energy Scan Query message.
 *
 */
OTAPI ThreadError OTCALL otCommissionerEnergyScan(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount,
                                                  uint16_t aPeriod, uint16_t aScanDuration, const otIp6Address *aAddress,
                                                  otCommissionerEnergyReportCallback aCallback, void *aContext);

/**
 * This function pointer is called when the Commissioner receives a PAN ID Conflict message.
 *
 * @param[in]  aPanId             The PAN ID value.
 * @param[in]  aChannelMask       The channel mask value.
 * @param[in]  aContext           A pointer to application-specific context.
 *
 */
typedef void (OTCALL *otCommissionerPanIdConflictCallback)(uint16_t aPanId, uint32_t aChannelMask, void *aContext);

/**
 * This function sends a PAN ID Query message.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aPanId         The PAN ID to query.
 * @param[in]  aChannelMask   The channel mask value.
 * @param[in]  aAddress       A pointer to the IPv6 destination.
 * @param[in]  aCallback      A pointer to a function called on receiving an Energy Report message.
 * @param[in]  aContext       A pointer to application-specific context.
 *
 * @retval kThreadError_None    Successfully enqueued the PAN ID Query message.
 * @retval kThreadError_NoBufs  Insufficient buffers to generate a PAN ID Query message.
 *
 */
OTAPI ThreadError OTCALL otCommissionerPanIdQuery(otInstance *aInstance, uint16_t aPanId, uint32_t aChannelMask,
                                                  const otIp6Address *aAddress,
                                                  otCommissionerPanIdConflictCallback aCallback, void *aContext);

/**
 * This function sends MGMT_COMMISSIONER_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendMgmtCommissionerGet(otInstance *, const uint8_t *aTlvs, uint8_t aLength);

/**
 * This function sends MGMT_COMMISSIONER_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to commissioning dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otSendMgmtCommissionerSet(otInstance *, const otCommissioningDataset *aDataset,
                                                   const uint8_t *aTlvs, uint8_t aLength);

/**
 * This function returns the origian Commissioner Session ID.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current commissioner session id.
 *
 */
OTAPI uint16_t OTCALL otCommissionerGetSessionId(otInstance *);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // OPENTHREAD_COMMISSIONER_H_
