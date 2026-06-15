/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes the platform abstraction for Thread Direct link operations.
 */

#ifndef OPENTHREAD_PLATFORM_THREAD_DIRECT_H_
#define OPENTHREAD_PLATFORM_THREAD_DIRECT_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/thread_direct.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-thread-direct
 *
 * @brief
 *   This module includes the platform abstraction for Thread Direct.
 *
 * @{
 *
 */

/**
 * Pre-configures the IE payload to inject into the Enh-ACK sent in response
 * to a TD Link Command frame received from @p aWiExtAddress.
 *
 * The stack calls this during the connection window, before the TD Link Command
 * is expected, because the Enh-ACK must be transmitted within the 192 us
 * IEEE 802.15.4 turnaround window.
 *
 * Pass @p aIeData = NULL / @p aIeLength = 0 to remove a previously registered
 * entry for @p aWiExtAddress.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aWiExtAddress Extended address of the WI peer.
 * @param[in] aIeData       IE payload bytes to inject, or NULL to remove.
 * @param[in] aIeLength     Length of @p aIeData in bytes (0 to remove).
 *
 * @retval OT_ERROR_NONE             Entry registered or removed successfully.
 * @retval OT_ERROR_NO_BUFS         Platform Enh-ACK IE table is full.
 * @retval OT_ERROR_NOT_FOUND       @p aIeData is NULL and no entry exists for @p aWiExtAddress.
 * @retval OT_ERROR_NOT_IMPLEMENTED Feature is not implemented.
 */
otError otPlatRadioConfigureThreadDirectEnhAckIe(otInstance         *aInstance,
                                                 const otExtAddress *aWiExtAddress,
                                                 const uint8_t      *aIeData,
                                                 uint16_t            aIeLength);

/**
 * Enables or disables the Scheduled Listen Window (SLW) receive schedule for
 * the Thread Direct link with @p aWiExtAddress.
 *
 * Called by the WL after a TD link is established to program the platform with
 * its SLW period.  Pass @p aSlwPeriod = 0 to disable SLW scheduling for this
 * peer.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aSlwPeriod     SLW period in 160 us slots (0 = disable).
 * @param[in] aWiExtAddress  WI peer extended address.
 *
 * @retval OT_ERROR_NONE             SLW schedule updated.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Feature is not implemented.
 */
otError otPlatRadioEnableThreadDirectSlw(otInstance *aInstance, uint16_t aSlwPeriod, const otExtAddress *aWiExtAddress);

/**
 * Updates the next SLW sample time for the Thread Direct link with @p aWiExtAddress.
 *
 * Called after each received or missed frame to advance the window to the next
 * expected arrival time.  @p aSlwSampleTime is in microseconds on the local
 * radio clock (see `otPlatRadioGetNow()`).
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aWiExtAddress  WI peer extended address.
 * @param[in] aSlwSampleTime Next expected frame arrival time in us.
 */
void otPlatRadioUpdateThreadDirectSlwSampleTime(otInstance         *aInstance,
                                                const otExtAddress *aWiExtAddress,
                                                uint32_t            aSlwSampleTime);

/**
 * Returns the worst-case clock accuracy of the local radio in PPM for Thread
 * Direct SLW transmit scheduling.
 *
 * Used by the WI to calculate the guard time when scheduling a unicast TX to
 * arrive within the WL's SLW window.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns Worst-case clock accuracy in PPM.
 */
uint8_t otPlatRadioGetThreadDirectSlwAccuracy(otInstance *aInstance);

/**
 * Returns the fixed arrival-time uncertainty for Thread Direct SLW frames in
 * units of 10 us.
 *
 * Used by the WI alongside `otPlatRadioGetThreadDirectSlwAccuracy()` to compute
 * the total guard window.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns Fixed SLW arrival-time uncertainty in units of 10 us.
 */
uint8_t otPlatRadioGetThreadDirectSlwUncertainty(otInstance *aInstance);

/**
 * Retrieves the current Radio Availability Mask (RAM) parameters from the
 * platform radio driver.
 *
 * Called when `OPENTHREAD_CONFIG_THREAD_DIRECT_COEX_ENABLE` is set.  The
 * platform fills @p aParams with the current CoEx constraints (bitmap of
 * unavailable slots within the SLW period, signed offset, and RAM Duration
 * code).  Platforms without CoEx support may return `OT_ERROR_NOT_IMPLEMENTED`.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aParams    Filled with current CoEx constraints.
 *
 * @retval OT_ERROR_NONE             @p aParams populated.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Feature is not implemented.
 */
otError otPlatRadioGetThreadDirectRamParams(otInstance *aInstance, otThreadDirectRamParams *aParams);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_THREAD_DIRECT_H_
