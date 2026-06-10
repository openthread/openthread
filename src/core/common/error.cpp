/*
 *  Copyright (c) 2016-2021, The OpenThread Authors.
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
 *   This file implements the error code functions used by OpenThread core modules.
 */

#include "error.hpp"

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/enum_to_string.hpp"

namespace ot {

const char *ErrorToString(Error aError)
{
#define ErrorMapList(_)                                               \
    _(kErrorNone, "OK")                                               \
    _(kErrorFailed, "Failed")                                         \
    _(kErrorDrop, "Drop")                                             \
    _(kErrorNoBufs, "NoBufs")                                         \
    _(kErrorNoRoute, "NoRoute")                                       \
    _(kErrorBusy, "Busy")                                             \
    _(kErrorParse, "Parse")                                           \
    _(kErrorInvalidArgs, "InvalidArgs")                               \
    _(kErrorSecurity, "Security")                                     \
    _(kErrorAddressQuery, "AddressQuery")                             \
    _(kErrorNoAddress, "NoAddress")                                   \
    _(kErrorAbort, "Abort")                                           \
    _(kErrorNotImplemented, "NotImplemented")                         \
    _(kErrorInvalidState, "InvalidState")                             \
    _(kErrorNoAck, "NoAck")                                           \
    _(kErrorChannelAccessFailure, "ChannelAccessFailure")             \
    _(kErrorDetached, "Detached")                                     \
    _(kErrorFcs, "FcsErr")                                            \
    _(kErrorNoFrameReceived, "NoFrameReceived")                       \
    _(kErrorUnknownNeighbor, "UnknownNeighbor")                       \
    _(kErrorInvalidSourceAddress, "InvalidSourceAddress")             \
    _(kErrorAddressFiltered, "AddressFiltered")                       \
    _(kErrorDestinationAddressFiltered, "DestinationAddressFiltered") \
    _(kErrorNotFound, "NotFound")                                     \
    _(kErrorAlready, "Already")                                       \
    _(25, "ReservedError25")                                          \
    _(kErrorIp6AddressCreationFailure, "Ipv6AddressCreationFailure")  \
    _(kErrorNotCapable, "NotCapable")                                 \
    _(kErrorResponseTimeout, "ResponseTimeout")                       \
    _(kErrorDuplicated, "Duplicated")                                 \
    _(kErrorReassemblyTimeout, "ReassemblyTimeout")                   \
    _(kErrorNotTmf, "NotTmf")                                         \
    _(kErrorNotLowpanDataFrame, "NonLowpanDataFrame")                 \
    _(33, "ReservedError33")                                          \
    _(kErrorLinkMarginLow, "LinkMarginLow")                           \
    _(kErrorInvalidCommand, "InvalidCommand")                         \
    _(kErrorPending, "Pending")                                       \
    _(kErrorRejected, "Rejected")

    DefineEnumStringArray(ErrorMapList);

    return static_cast<size_t>(aError) < GetArrayLength(kStrings) ? kStrings[aError] : "UnknownErrorType";
}

} // namespace ot
