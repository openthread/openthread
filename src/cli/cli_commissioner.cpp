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
#include "cli/cli_server.hpp"

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

namespace ot {
namespace Cli {

const struct Commissioner::Command Commissioner::sCommands[] = {
    {"help", &Commissioner::ProcessHelp},           {"announce", &Commissioner::ProcessAnnounce},
    {"energy", &Commissioner::ProcessEnergy},       {"joiner", &Commissioner::ProcessJoiner},
    {"mgmtget", &Commissioner::ProcessMgmtGet},     {"mgmtset", &Commissioner::ProcessMgmtSet},
    {"panid", &Commissioner::ProcessPanId},         {"provisioningurl", &Commissioner::ProcessProvisioningUrl},
    {"sessionid", &Commissioner::ProcessSessionId}, {"start", &Commissioner::ProcessStart},
    {"stop", &Commissioner::ProcessStop},
};

otError Commissioner::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Commissioner::ProcessAnnounce(int argc, char *argv[])
{
    otError      error;
    long         mask;
    long         count;
    long         period;
    otIp6Address address;

    VerifyOrExit(argc > 4, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(argv[1], mask));
    SuccessOrExit(error = Interpreter::ParseLong(argv[2], count));
    SuccessOrExit(error = Interpreter::ParseLong(argv[3], period));
    SuccessOrExit(error = otIp6AddressFromString(argv[4], &address));

    SuccessOrExit(error = otCommissionerAnnounceBegin(mInterpreter.mInstance, static_cast<uint32_t>(mask),
                                                      static_cast<uint8_t>(count), static_cast<uint16_t>(period),
                                                      &address));

exit:
    return error;
}

otError Commissioner::ProcessEnergy(int argc, char *argv[])
{
    otError      error;
    long         mask;
    long         count;
    long         period;
    long         scanDuration;
    otIp6Address address;

    VerifyOrExit(argc > 5, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(argv[1], mask));
    SuccessOrExit(error = Interpreter::ParseLong(argv[2], count));
    SuccessOrExit(error = Interpreter::ParseLong(argv[3], period));
    SuccessOrExit(error = Interpreter::ParseLong(argv[4], scanDuration));
    SuccessOrExit(error = otIp6AddressFromString(argv[5], &address));

    SuccessOrExit(error = otCommissionerEnergyScan(mInterpreter.mInstance, static_cast<uint32_t>(mask),
                                                   static_cast<uint8_t>(count), static_cast<uint16_t>(period),
                                                   static_cast<uint16_t>(scanDuration), &address,
                                                   &Commissioner::HandleEnergyReport, this));

exit:
    return error;
}

otError Commissioner::ProcessJoiner(int argc, char *argv[])
{
    otError             error;
    otExtAddress        addr;
    const otExtAddress *addrPtr;

    VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[2], "*") == 0)
    {
        addrPtr = NULL;
    }
    else
    {
        VerifyOrExit(Interpreter::Hex2Bin(argv[2], addr.m8, sizeof(addr)) == sizeof(addr), error = OT_ERROR_PARSE);
        addrPtr = &addr;
    }

    if (strcmp(argv[1], "add") == 0)
    {
        VerifyOrExit(argc > 3, error = OT_ERROR_INVALID_ARGS);
        // Timeout parameter is optional - if not specified, use default value.
        unsigned long timeout = kDefaultJoinerTimeout;

        if (argc > 4)
        {
            SuccessOrExit(error = Interpreter::ParseUnsignedLong(argv[4], timeout));
        }

        SuccessOrExit(
            error = otCommissionerAddJoiner(mInterpreter.mInstance, addrPtr, argv[3], static_cast<uint32_t>(timeout)));
    }
    else if (strcmp(argv[1], "remove") == 0)
    {
        SuccessOrExit(error = otCommissionerRemoveJoiner(mInterpreter.mInstance, addrPtr));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Commissioner::ProcessMgmtGet(int argc, char *argv[])
{
    otError error;
    uint8_t tlvs[32];
    long    value;
    int     length = 0;

    for (uint8_t index = 1; index < argc; index++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (strcmp(argv[index], "locator") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_BORDER_AGENT_RLOC;
        }
        else if (strcmp(argv[index], "sessionid") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_COMM_SESSION_ID;
        }
        else if (strcmp(argv[index], "steeringdata") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_STEERING_DATA;
        }
        else if (strcmp(argv[index], "joinerudpport") == 0)
        {
            tlvs[length++] = OT_MESHCOP_TLV_JOINER_UDP_PORT;
        }
        else if (strcmp(argv[index], "binary") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            value = static_cast<long>(strlen(argv[index]) + 1) / 2;
            VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)),
                         error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs + length, static_cast<uint16_t>(value)) >= 0,
                         error = OT_ERROR_PARSE);
            length += value;
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

otError Commissioner::ProcessMgmtSet(int argc, char *argv[])
{
    otError                error;
    otCommissioningDataset dataset;
    uint8_t                tlvs[32];
    long                   value;
    int                    length = 0;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < argc; index++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (strcmp(argv[index], "locator") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsLocatorSet = true;
            SuccessOrExit(error = Interpreter::Interpreter::ParseLong(argv[index], value));
            dataset.mLocator = static_cast<uint16_t>(value);
        }
        else if (strcmp(argv[index], "sessionid") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsSessionIdSet = true;
            SuccessOrExit(error = Interpreter::Interpreter::ParseLong(argv[index], value));
            dataset.mSessionId = static_cast<uint16_t>(value);
        }
        else if (strcmp(argv[index], "steeringdata") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsSteeringDataSet = true;
            length                     = static_cast<int>((strlen(argv[index]) + 1) / 2);
            VerifyOrExit(static_cast<size_t>(length) <= OT_STEERING_DATA_MAX_LENGTH, error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[index], dataset.mSteeringData.m8, static_cast<uint16_t>(length)) >=
                             0,
                         error = OT_ERROR_PARSE);
            dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
            length                        = 0;
        }
        else if (strcmp(argv[index], "joinerudpport") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mIsJoinerUdpPortSet = true;
            SuccessOrExit(error = Interpreter::Interpreter::ParseLong(argv[index], value));
            dataset.mJoinerUdpPort = static_cast<uint16_t>(value);
        }
        else if (strcmp(argv[index], "binary") == 0)
        {
            VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
            length = static_cast<int>((strlen(argv[index]) + 1) / 2);
            VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                         error = OT_ERROR_PARSE);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error =
                      otCommissionerSendMgmtSet(mInterpreter.mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));

exit:
    return error;
}

otError Commissioner::ProcessPanId(int argc, char *argv[])
{
    otError      error;
    long         panid;
    long         mask;
    otIp6Address address;

    VerifyOrExit(argc > 3, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(argv[1], panid));
    SuccessOrExit(error = Interpreter::ParseLong(argv[2], mask));
    SuccessOrExit(error = otIp6AddressFromString(argv[3], &address));

    SuccessOrExit(error = otCommissionerPanIdQuery(mInterpreter.mInstance, static_cast<uint16_t>(panid),
                                                   static_cast<uint32_t>(mask), &address,
                                                   &Commissioner::HandlePanIdConflict, this));

exit:
    return error;
}

otError Commissioner::ProcessProvisioningUrl(int argc, char *argv[])
{
    return otCommissionerSetProvisioningUrl(mInterpreter.mInstance, (argc > 1) ? argv[1] : NULL);
}

otError Commissioner::ProcessSessionId(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mInterpreter.mServer->OutputFormat("%d\r\n", otCommissionerGetSessionId(mInterpreter.mInstance));

    return OT_ERROR_NONE;
}

otError Commissioner::ProcessStart(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otCommissionerStart(mInterpreter.mInstance, &Commissioner::HandleStateChanged,
                               &Commissioner::HandleJoinerEvent, this);
}

void Commissioner::HandleStateChanged(otCommissionerState aState, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleStateChanged(aState);
}

void Commissioner::HandleStateChanged(otCommissionerState aState)
{
    mInterpreter.mServer->OutputFormat("Commissioner: ");

    switch (aState)
    {
    case OT_COMMISSIONER_STATE_DISABLED:
        mInterpreter.mServer->OutputFormat("disabled\r\n");
        break;
    case OT_COMMISSIONER_STATE_PETITION:
        mInterpreter.mServer->OutputFormat("petitioning\r\n");
        break;
    case OT_COMMISSIONER_STATE_ACTIVE:
        mInterpreter.mServer->OutputFormat("active\r\n");
        break;
    }
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent, const otExtAddress *aJoinerId, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerEvent(aEvent, aJoinerId);
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent, const otExtAddress *aJoinerId)
{
    mInterpreter.mServer->OutputFormat("Commissioner: Joiner ");

    switch (aEvent)
    {
    case OT_COMMISSIONER_JOINER_START:
        mInterpreter.mServer->OutputFormat("start ");
        break;
    case OT_COMMISSIONER_JOINER_CONNECTED:
        mInterpreter.mServer->OutputFormat("connect ");
        break;
    case OT_COMMISSIONER_JOINER_FINALIZE:
        mInterpreter.mServer->OutputFormat("finalize ");
        break;
    case OT_COMMISSIONER_JOINER_END:
        mInterpreter.mServer->OutputFormat("end ");
        break;
    case OT_COMMISSIONER_JOINER_REMOVED:
        mInterpreter.mServer->OutputFormat("remove ");
        break;
    }

    mInterpreter.OutputBytes(aJoinerId->m8, sizeof(*aJoinerId));

    mInterpreter.mServer->OutputFormat("\r\n");
}

otError Commissioner::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otCommissionerStop(mInterpreter.mInstance);
}

otError Commissioner::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc, argv);
                break;
            }
        }
    }

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
    mInterpreter.mServer->OutputFormat("Energy: %08x ", aChannelMask);

    for (uint8_t i = 0; i < aEnergyListLength; i++)
    {
        mInterpreter.mServer->OutputFormat("%d ", static_cast<int8_t>(aEnergyList[i]));
    }

    mInterpreter.mServer->OutputFormat("\r\n");
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    mInterpreter.mServer->OutputFormat("Conflict: %04x, %08x\r\n", aPanId, aChannelMask);
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
