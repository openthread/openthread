/*
 *    Copyright (c) 2025, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the data poll accelerator platform callbacks into OpenThread and default/weak platform APIs.
 */

#include <openthread/platform/poll_accelerator.h>

#include "instance/instance.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_POLL_ACCELERATOR_ENABLE

extern "C" OT_TOOL_WEAK otError otPlatPollAcceleratorStart(otInstance                    *aInstance,
                                                           otRadioFrame                  *aFrame,
                                                           const otPollAcceleratorConfig *aConfig)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aConfig);

    return OT_ERROR_NOT_IMPLEMENTED;
}

extern "C" OT_TOOL_WEAK otError otPlatPollAcceleratorStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

extern "C" void otPlatPollAcceleratorDone(otInstance   *aInstance,
                                          uint32_t      aIterationsDone,
                                          otRadioFrame *aPrevAckFrame,
                                          otRadioFrame *aTxFrame,
                                          otRadioFrame *aAckFrame,
                                          otError       aTxError,
                                          otRadioFrame *aRxFrame,
                                          otError       aRxError)
{
    Mac::RxFrame *prevAckFrame = static_cast<Mac::RxFrame *>(aPrevAckFrame);
    Mac::TxFrame &txFrame      = *static_cast<Mac::TxFrame *>(aTxFrame);
    Mac::RxFrame *ackFrame     = static_cast<Mac::RxFrame *>(aAckFrame);
    Mac::RxFrame *rxFrame      = static_cast<Mac::RxFrame *>(aRxFrame);

    OT_ASSERT(aInstance != nullptr);

    ot::Instance &instance = ot::AsCoreType(aInstance);
    instance.Get<ot::PollAccelerator>().HandlePollDone(aIterationsDone, prevAckFrame, txFrame, ackFrame, aTxError,
                                                       rxFrame, aRxError);
}

#endif
