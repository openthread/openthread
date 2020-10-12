/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a simple CLI for the Commissioner role.
 */

#include "cli_commissioner.hpp"

#include "cli/cli.hpp"
#include "utils/parse_cmdline.hpp"

using ot::Utils::CmdLineParser::ParseAsHexString;
using ot::Utils::CmdLineParser::ParseAsIp6Address;
using ot::Utils::CmdLineParser::ParseAsUint16;
using ot::Utils::CmdLineParser::ParseAsUint32;
using ot::Utils::CmdLineParser::ParseAsUint8;

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

namespace ot {
namespace Cli {

constexpr Commissioner::Command Commissioner::sCommands[];

otError Commissioner::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError Commissioner::ProcessAnnounce(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    uint32_t     mask;
    uint8_t      count;
    uint16_t     period;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 4, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint32(aArgs[1], mask));
    SuccessOrExit(error = ParseAsUint8(aArgs[2], count));
    SuccessOrExit(error = ParseAsUint16(aArgs[3], period));
    SuccessOrExit(error = ParseAsIp6Address(aArgs[4], address));

    SuccessOrExit(error = otCommissionerAnnounceBegin(mInterpreter.mInstance, mask, count, period, &address));

exit:
    return error;
}

otError Commissioner::ProcessEnergy(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    uint32_t     mask;
    uint8_t      count;
    uint16_t     period;
    uint16_t     scanDuration;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 5, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint32(aArgs[1], mask));
    SuccessOrExit(error = ParseAsUint8(aArgs[2], count));
    SuccessOrExit(error = ParseAsUint16(aArgs[3], period));
    SuccessOrExit(error = ParseAsUint16(aArgs[4], scanDuration));
    SuccessOrExit(error = ParseAsIp6Address(aArgs[5], address));

    SuccessOrExit(error = otCommissionerEnergyScan(mInterpreter.mInstance, mask, count, period, scanDuration, &address,
                                                   &Commissioner::HandleEnergyReport, this));

exit:
    return error;
}

otError Commissioner::ProcessJoiner(uint8_t aArgsLength, char *aArgs[])
{
    otError             error;
    otExtAddress        addr;
    const otExtAddress *addrPtr = nullptr;
    otJoinerDiscerner   discerner;

    VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

    memset(&discerner, 0, sizeof(discerner));

    if (strcmp(aArgs[2], "*") == 0)
    {
        // Intentionally empty
    }
    else if ((error = Interpreter::ParseJoinerDiscerner(aArgs[2], discerner)) == OT_ERROR_NOT_FOUND)
    {
        SuccessOrExit(error = ParseAsHexString(aArgs[2], addr.m8));
        addrPtr = &addr;
    }
    else if (error != OT_ERROR_NONE)
    {
        ExitNow();
    }

    if (strcmp(aArgs[1], "add") == 0)
    {
        VerifyOrExit(aArgsLength > 3, error = OT_ERROR_INVALID_ARGS);
        // Timeout parameter is optional - if not specified, use default value.
        uint32_t timeout = kDefaultJoinerTimeout;

        if (aArgsLength > 4)
        {
            SuccessOrExit(error = ParseAsUint32(aArgs[4], timeout));
        }

        if (discerner.mLength)
        {
            SuccessOrExit(
                error = otCommissionerAddJoinerWithDiscerner(mInterpreter.mInstance, &discerner, aArgs[3], timeout));
        }
        else
        {
            SuccessOrExit(error = otCommissionerAddJoiner(mInterpreter.mInstance, addrPtr, aArgs[3], timeout));
        }
    }
    else if (strcmp(aArgs[1], "remove") == 0)
    {
        if (discerner.mLength)
        {
            SuccessOrExit(error = otCommissionerRemoveJoinerWithDiscerner(mInterpreter.mInstance, &discerner));
        }
        else
        {
            SuccessOrExit(error = otCommissionerRemoveJoiner(mInterpreter.mInstance, addrPtr));
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Commissioner::ProcessMgmtGet(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t tlvs[32];
    uint8_t length = 0;

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (strcmp(aArgs[index], "locator") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_BORDER_AGENT_RLOC;
        }
        else if (strcmp(aArgs[index], "sessionid") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_COMM_SESSION_ID;
        }
        else if (strcmp(aArgs[index], "steeringdata") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_STEERING_DATA;
        }
        else if (strcmp(aArgs[index], "joinerudpport") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_JOINER_UDP_PORT;
        }
        else if (strcmp(aArgs[index], "-x") == 0)
        {
            uint16_t readLength;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            readLength = static_cast<uint16_t>(sizeof(tlvs) - length);
            SuccessOrExit(error = ParseAsHexString(aArgs[index], readLength, tlvs + length));
            length += static_cast<uint8_t>(readLength);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otCommissionerSendMgmtGet(mInterpreter.mInstance, tlvs, static_cast<uint8_t>(length)));

exit:
    return error;
}

otError Commissioner::ProcessMgmtSet(uint8_t aArgsLength, char *aArgs[])
{
    otError                error;
    otCommissioningDataset dataset;
    uint8_t                tlvs[32];
    uint8_t                tlvsLength = 0;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        if (strcmp(aArgs[index], "locator") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsLocatorSet = true;
            SuccessOrExit(error = ParseAsUint16(aArgs[index], dataset.mLocator));
        }
        else if (strcmp(aArgs[index], "sessionid") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsSessionIdSet = true;
            SuccessOrExit(error = ParseAsUint16(aArgs[index], dataset.mSessionId));
        }
        else if (strcmp(aArgs[index], "steeringdata") == 0)
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsSteeringDataSet = true;
            length                     = sizeof(dataset.mSteeringData.m8);
            SuccessOrExit(error = ParseAsHexString(aArgs[index], length, dataset.mSteeringData.m8));
            dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
        }
        else if (strcmp(aArgs[index], "joinerudpport") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsJoinerUdpPortSet = true;
            SuccessOrExit(error = ParseAsUint16(aArgs[index], dataset.mJoinerUdpPort));
        }
        else if (strcmp(aArgs[index], "-x") == 0)
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = sizeof(tlvs);
            SuccessOrExit(error = ParseAsHexString(aArgs[index], length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otCommissionerSendMgmtSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength));

exit:
    return error;
}

otError Commissioner::ProcessPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    uint16_t     panId;
    uint32_t     mask;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 3, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint16(aArgs[1], panId));
    SuccessOrExit(error = ParseAsUint32(aArgs[2], mask));
    SuccessOrExit(error = ParseAsIp6Address(aArgs[3], address));

    SuccessOrExit(error = otCommissionerPanIdQuery(mInterpreter.mInstance, panId, mask, &address,
                                                   &Commissioner::HandlePanIdConflict, this));

exit:
    return error;
}

otError Commissioner::ProcessProvisioningUrl(uint8_t aArgsLength, char *aArgs[])
{
    return otCommissionerSetProvisioningUrl(mInterpreter.mInstance, (aArgsLength > 1) ? aArgs[1] : nullptr);
}

otError Commissioner::ProcessSessionId(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    mInterpreter.OutputLine("%d", otCommissionerGetSessionId(mInterpreter.mInstance));

    return OT_ERROR_NONE;
}

otError Commissioner::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    return otCommissionerStart(mInterpreter.mInstance, &Commissioner::HandleStateChanged,
                               &Commissioner::HandleJoinerEvent, this);
}

void Commissioner::HandleStateChanged(otCommissionerState aState, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleStateChanged(aState);
}

void Commissioner::HandleStateChanged(otCommissionerState aState)
{
    mInterpreter.OutputLine("Commissioner: %s", StateToString(aState));
}

const char *Commissioner::StateToString(otCommissionerState aState)
{
    const char *rval = "unknown";

    switch (aState)
    {
    case OT_COMMISSIONER_STATE_DISABLED:
        rval = "disabled";
        break;
    case OT_COMMISSIONER_STATE_PETITION:
        rval = "petitioning";
        break;
    case OT_COMMISSIONER_STATE_ACTIVE:
        rval = "active";
        break;
    }

    return rval;
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent,
                                     const otJoinerInfo *      aJoinerInfo,
                                     const otExtAddress *      aJoinerId,
                                     void *                    aContext)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerEvent(aEvent, aJoinerInfo, aJoinerId);
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent,
                                     const otJoinerInfo *      aJoinerInfo,
                                     const otExtAddress *      aJoinerId)
{
    OT_UNUSED_VARIABLE(aJoinerInfo);

    mInterpreter.OutputFormat("Commissioner: Joiner ");

    switch (aEvent)
    {
    case OT_COMMISSIONER_JOINER_START:
        mInterpreter.OutputFormat("start ");
        break;
    case OT_COMMISSIONER_JOINER_CONNECTED:
        mInterpreter.OutputFormat("connect ");
        break;
    case OT_COMMISSIONER_JOINER_FINALIZE:
        mInterpreter.OutputFormat("finalize ");
        break;
    case OT_COMMISSIONER_JOINER_END:
        mInterpreter.OutputFormat("end ");
        break;
    case OT_COMMISSIONER_JOINER_REMOVED:
        mInterpreter.OutputFormat("remove ");
        break;
    }

    if (aJoinerId != nullptr)
    {
        mInterpreter.OutputExtAddress(*aJoinerId);
    }

    mInterpreter.OutputLine("");
}

otError Commissioner::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    return otCommissionerStop(mInterpreter.mInstance);
}

otError Commissioner::ProcessState(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    mInterpreter.OutputLine(StateToString(otCommissionerGetState(mInterpreter.mInstance)));

    return OT_ERROR_NONE;
}

otError Commissioner::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    VerifyOrExit(aArgsLength != 0, IgnoreError(ProcessHelp(0, nullptr)));

    command = Utils::LookupTable::Find(aArgs[0], sCommands);
    VerifyOrExit(command != nullptr, OT_NOOP);

    error = (this->*command->mHandler)(aArgsLength, aArgs);

exit:
    return error;
}

void Commissioner::HandleEnergyReport(uint32_t       aChannelMask,
                                      const uint8_t *aEnergyList,
                                      uint8_t        aEnergyListLength,
                                      void *         aContext)
{
    static_cast<Commissioner *>(aContext)->HandleEnergyReport(aChannelMask, aEnergyList, aEnergyListLength);
}

void Commissioner::HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength)
{
    mInterpreter.OutputFormat("Energy: %08x ", aChannelMask);

    for (uint8_t i = 0; i < aEnergyListLength; i++)
    {
        mInterpreter.OutputFormat("%d ", static_cast<int8_t>(aEnergyList[i]));
    }

    mInterpreter.OutputLine("");
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    mInterpreter.OutputLine("Conflict: %04x, %08x", aPanId, aChannelMask);
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
