/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes the definitions of the radio spinel metrics.
 */

#ifndef RADIO_SPINEL_METRICS_H_
#define RADIO_SPINEL_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents the radio spinel metrics.
 */
typedef struct otRadioSpinelMetrics
{
    uint32_t mRcpTimeoutCount;         ///< The number of RCP timeouts.
    uint32_t mRcpUnexpectedResetCount; ///< The number of RCP unexpected resets.
    uint32_t mRcpRestorationCount;     ///< The number of RCP restorations.
    uint32_t mSpinelParseErrorCount;   ///< The number of spinel frame parse errors.
} otRadioSpinelMetrics;

/**
 * Represents RCP interface metrics.
 */
typedef struct otRcpInterfaceMetrics
{
    uint8_t  mRcpInterfaceType;             ///< The RCP interface type.
    uint64_t mTransferredFrameCount;        ///< The number of transferred frames.
    uint64_t mTransferredValidFrameCount;   ///< The number of transferred valid frames.
    uint64_t mTransferredGarbageFrameCount; ///< The number of transferred garbage frames.
    uint64_t mRxFrameCount;                 ///< The number of received frames.
    uint64_t mRxFrameByteCount;             ///< The number of received bytes.
    uint64_t mTxFrameCount;                 ///< The number of transmitted frames.
    uint64_t mTxFrameByteCount;             ///< The number of transmitted bytes.
} otRcpInterfaceMetrics;

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // RADIO_SPINEL_METRICS_H_
