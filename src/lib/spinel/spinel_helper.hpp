/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 * @file This file includes definitions for spinel helper methods.
 */

#ifndef SPINEL_HELPER_HPP_
#define SPINEL_HELPER_HPP_

#include <openthread/error.h>

#include "lib/spinel/spinel.h"

namespace ot {
namespace Spinel {

/**
 * Convert the Spinel status code to OpenThread error code.
 *
 * @param[in]  aStatus  The Spinel status code.
 *
 * @retval  OT_ERROR_NONE                    The operation has completed successfully.
 * @retval  OT_ERROR_DROP                    The packet was dropped.
 * @retval  OT_ERROR_NO_BUFS                 The operation has been prevented due to memory pressure.
 * @retval  OT_ERROR_BUSY                    The device is currently performing a mutuallyexclusive operation.
 * @retval  OT_ERROR_PARSE                   An error has occurred while parsing the command.
 * @retval  OT_ERROR_INVALID_ARGS            An argument to the given operation is invalid.
 * @retval  OT_ERROR_NOT_IMPLEMENTED         The given operation has not been implemented.
 * @retval  OT_ERROR_INVALID_STATE           The given operation is invalid for the current state of the device.
 * @retval  OT_ERROR_NO_ACK                  The packet was not acknowledged.
 * @retval  OT_ERROR_NOT_FOUND               The given property is not recognized.
 * @retval  OT_ERROR_FAILED                  The given operation has failed for some undefined reason.
 * @retval  OT_ERROR_CHANNEL_ACCESS_FAILURE  The packet was not sent due to a CCA failure.
 * @retval  OT_ERROR_ALREADY                 The operation is already in progress or the property was already set
 *                                           to the given value.
 */
otError SpinelStatusToOtError(spinel_status_t aStatus);

} // namespace Spinel
} // namespace ot

#endif // SPINEL_HELPER_HPP_
