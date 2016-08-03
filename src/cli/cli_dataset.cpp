/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the CLI interpreter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread.h>
#include <openthread-config.h>

#include "cli.hpp"
#include "cli_dataset.hpp"
#include <common/encoding.hpp>
#include <thread/meshcop_dataset.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

extern ThreadNetif *sThreadNetif;

namespace Cli {

const DatasetCommand Dataset::sCommands[] =
{
    { "help", &ProcessHelp },
    { "active", &ProcessActive },
    { "activetimestamp", &ProcessActiveTimestamp },
    { "channel", &ProcessChannel },
    { "clear", &ProcessClear },
    { "commit", &ProcessCommit },
    { "delay", &ProcessDelay },
    { "extpanid", &ProcessExtPanId },
    { "masterkey", &ProcessMasterKey },
    { "meshlocalprefix", &ProcessMeshLocalPrefix },
    { "networkname", &ProcessNetworkName },
    { "panid", &ProcessPanId },
    { "pending", &ProcessPending },
    { "timestamp", &ProcessTimestamp },
};

Server *Dataset::sServer;
MeshCoP::Dataset sDataset;

void Dataset::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        sServer->OutputFormat("%02x", aBytes[i]);
    }
}

ThreadError Dataset::Print(MeshCoP::Dataset &aDataset)
{
    MeshCoP::Tlv *tlv;

    sServer->OutputFormat("Timestamp: %d\r\n", aDataset.GetTimestamp().GetSeconds());

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kActiveTimestamp)) != NULL)
    {
        MeshCoP::ActiveTimestampTlv *tmp = static_cast<MeshCoP::ActiveTimestampTlv *>(tlv);
        sServer->OutputFormat("Active Timestamp: %d\r\n", tmp->GetSeconds());
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kChannel)) != NULL)
    {
        MeshCoP::ChannelTlv *tmp = static_cast<MeshCoP::ChannelTlv *>(tlv);
        sServer->OutputFormat("Channel: %d\r\n", tmp->GetChannel());
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kDelayTimer)) != NULL)
    {
        MeshCoP::DelayTimerTlv *tmp = static_cast<MeshCoP::DelayTimerTlv *>(tlv);
        sServer->OutputFormat("Delay: %d\r\n", tmp->GetDelayTimer());
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kExtendedPanId)) != NULL)
    {
        MeshCoP::ExtendedPanIdTlv *tmp = static_cast<MeshCoP::ExtendedPanIdTlv *>(tlv);
        sServer->OutputFormat("Ext PAN ID: ");
        OutputBytes(tmp->GetExtendedPanId(), OT_EXT_PAN_ID_SIZE);
        sServer->OutputFormat("\r\n");
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kMeshLocalPrefix)) != NULL)
    {
        MeshCoP::MeshLocalPrefixTlv *tmp = static_cast<MeshCoP::MeshLocalPrefixTlv *>(tlv);
        const uint8_t *prefix = tmp->GetMeshLocalPrefix();
        sServer->OutputFormat("Mesh Local Prefix: %x:%x:%x:%x/64\r\n",
                              (static_cast<uint16_t>(prefix[0]) << 16) | prefix[1],
                              (static_cast<uint16_t>(prefix[2]) << 16) | prefix[3],
                              (static_cast<uint16_t>(prefix[4]) << 16) | prefix[5],
                              (static_cast<uint16_t>(prefix[6]) << 16) | prefix[7]);
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kNetworkMasterKey)) != NULL)
    {
        MeshCoP::NetworkMasterKeyTlv *tmp = static_cast<MeshCoP::NetworkMasterKeyTlv *>(tlv);
        sServer->OutputFormat("Master Key: ");
        OutputBytes(tmp->GetNetworkMasterKey(), tmp->GetLength());
        sServer->OutputFormat("\r\n");
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kNetworkName)) != NULL)
    {
        MeshCoP::NetworkNameTlv *tmp = static_cast<MeshCoP::NetworkNameTlv *>(tlv);
        sServer->OutputFormat("Network Name: ");
        sServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_SIZE, tmp->GetNetworkName());
    }

    if ((tlv = aDataset.Get(MeshCoP::Tlv::kPanId)) != NULL)
    {
        MeshCoP::PanIdTlv *tmp = static_cast<MeshCoP::PanIdTlv *>(tlv);
        sServer->OutputFormat("PAN ID: 0x%04x\r\n", tmp->GetPanId());
    }

    return kThreadError_None;
}

ThreadError Dataset::Process(int argc, char *argv[], Server &aServer)
{
    ThreadError error = kThreadError_None;

    sServer = &aServer;

    if (argc == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

ThreadError Dataset::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    (void)argc;
    (void)argv;
    return kThreadError_None;
}

ThreadError Dataset::ProcessActive(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return Print(sThreadNetif->GetActiveDataset().GetLocal());
}

ThreadError Dataset::ProcessActiveTimestamp(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::ActiveTimestampTlv tlv;
    long value;

    VerifyOrExit(argc > 0, ;);

    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    tlv.Init();
    tlv.SetSeconds(value);
    tlv.SetTicks(0);
    tlv.SetAuthoritative(false);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessChannel(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::ChannelTlv tlv;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    tlv.Init();
    tlv.SetChannelPage(0);
    tlv.SetChannel(value);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessClear(int argc, char *argv[])
{
    sDataset.Clear();
    (void)argc;
    (void)argv;
    return kThreadError_None;
}

ThreadError Dataset::ProcessCommit(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, ;);

    if (strcmp(argv[0], "active") == 0)
    {
        sThreadNetif->GetActiveDataset().Set(sDataset);
    }
    else if (strcmp(argv[0], "pending") == 0)
    {
        sThreadNetif->GetPendingDataset().Set(sDataset);
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

    sThreadNetif->GetNetworkDataLeader().IncrementVersion();
    sThreadNetif->GetNetworkDataLeader().IncrementStableVersion();

exit:
    return error;
}

ThreadError Dataset::ProcessDelay(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::DelayTimerTlv tlv;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    tlv.Init();
    tlv.SetDelayTimer(value);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessExtPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::ExtendedPanIdTlv tlv;
    uint8_t extPanId[8];

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit(Interpreter::Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

    tlv.Init();
    tlv.SetExtendedPanId(extPanId);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessMasterKey(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::NetworkMasterKeyTlv tlv;
    int8_t keyLength;
    uint8_t key[16];

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit((keyLength = Interpreter::Hex2Bin(argv[0], key, sizeof(key))) == 16, error = kThreadError_Parse);
    tlv.Init();
    tlv.SetNetworkMasterKey(key);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessMeshLocalPrefix(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::MeshLocalPrefixTlv tlv;
    struct otIp6Address prefix;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = otIp6AddressFromString(argv[0], &prefix));
    tlv.Init();
    tlv.SetMeshLocalPrefix(prefix.mFields.m8);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::NetworkNameTlv tlv;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    tlv.Init();
    tlv.SetNetworkName(argv[0]);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::PanIdTlv tlv;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    tlv.Init();
    tlv.SetPanId(value);
    error = sDataset.Set(tlv);

exit:
    return error;
}

ThreadError Dataset::ProcessPending(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return Print(sThreadNetif->GetPendingDataset().GetLocal());
}

ThreadError Dataset::ProcessTimestamp(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    MeshCoP::Timestamp timestamp;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    timestamp.Init();
    timestamp.SetSeconds(value);
    sDataset.SetTimestamp(timestamp);

exit:
    return error;
}

}  // namespace Cli
}  // namespace Thread
