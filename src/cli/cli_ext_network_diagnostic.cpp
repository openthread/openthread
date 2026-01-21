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

#include "cli_ext_network_diagnostic.hpp"

#include "openthread/error.h"
#include "openthread/ext_network_diagnostic.h"
#include "openthread/message.h"

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE && OPENTHREAD_FTD

namespace ot {
namespace Cli {

ExtNetworkDiagnosticClient::ExtNetworkDiagnosticClient(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
{
}

otError ExtNetworkDiagnosticClient::Process(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "stop")
    {
        otExtNetworkDiagnosticStopClient(GetInstancePtr());
    }
    else if (aArgs[0] == "config")
    {
        otExtNetworkDiagnosticTlvSet *set = nullptr;

        if (aArgs[1] == "-h")
        {
            set = &mHostTlvs;
        }
        else if (aArgs[1] == "-c")
        {
            set = &mChildTlvs;
        }
        else if (aArgs[1] == "-n")
        {
            set = &mNeighborTlvs;
        }

        VerifyOrExit(set != nullptr, error = OT_ERROR_INVALID_ARGS);
        memset(set, 0, sizeof(*set));

        for (uint32_t i = 2; i < Arg::GetArgsLength(aArgs); i++)
        {
            uint8_t value;

            SuccessOrExit(error = aArgs[i].ParseAsUint8(value));
            IgnoreError(otExtNetworkDiagnosticSetTlv(set, value));
        }
    }
    else if (aArgs[0] == "start")
    {
        otExtNetworkDiagnosticStartClient(GetInstancePtr(), &mHostTlvs, &mChildTlvs, &mNeighborTlvs, HandleServerUpdate,
                                          this);
    }

exit:
    return error;
}

void ExtNetworkDiagnosticClient::HandleServerUpdate(const otMessage *aMessage,
                                                    uint16_t         aRloc16,
                                                    bool             aComplete,
                                                    void            *aContext)
{
    static_cast<ExtNetworkDiagnosticClient *>(aContext)->HandleServerUpdate(aMessage, aRloc16, aComplete);
}

void ExtNetworkDiagnosticClient::HandleServerUpdate(const otMessage *aMessage, uint16_t aRloc16, bool aComplete)
{
    Error error = OT_ERROR_NONE;

    otExtNetworkDiagnosticIterator contextIter = OT_EXT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
    otExtNetworkDiagnosticContext  context;
    otExtNetworkDiagnosticTlv      tlv;

    const char *ctxMode = " ";

    OutputLine("Update from %04X, Complete: %s, Message Bytes: %lu", aRloc16, aComplete ? "true" : "false",
               ToUlong(otMessageGetLength(aMessage)));
    OutputLine("|+/-| T | Rloc | Tlvs");

    while ((error = otExtNetworkDiagnosticGetNextContext(aMessage, &contextIter, &context)) == OT_ERROR_NONE)
    {
        if (context.mType == OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD ||
            context.mType == OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR)
        {
            switch (context.mUpdateMode)
            {
            case OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_ADDED:
                ctxMode = "+";
                break;

            case OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_REMOVED:
                ctxMode = "-";
                break;

            case OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_UPDATE:
                ctxMode = "U";
                break;
            }
        }

        switch (context.mType)
        {
        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_HOST:
            OutputFormat("|   | H |      |");
            break;

        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD:
            OutputFormat("| %s | C | %04X |", ctxMode, context.mRloc16);
            break;

        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR:
            OutputFormat("| %s | N | %04X |", ctxMode, context.mRloc16);
            break;
        }

        while ((error = otExtNetworkDiagnosticGetNextTlv(aMessage, &context, &tlv)) == OT_ERROR_NONE)
        {
            OutputFormat(" 0x%02X", tlv.mType);
        }

        OutputNewLine();

        if (error != OT_ERROR_NOT_FOUND)
        {
            break;
        }
    }

    if (error != OT_ERROR_NOT_FOUND)
    {
        OutputLine("Parse error: %s", otThreadErrorToString(error));
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE && OPENTHREAD_FTD
