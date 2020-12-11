/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements a simple CLI for the CoAP service.
 */

#include "cli_tcp.hpp"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include <openthread/message.h>
#include <openthread/tcp.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"
#include "utils/parse_cmdline.hpp"

using ot::Utils::CmdLineParser::ParseAsIp6Address;
using ot::Utils::CmdLineParser::ParseAsUint16;
using ot::Utils::CmdLineParser::ParseAsUint32;
using ot::Utils::CmdLineParser::ParseAsUint8;

namespace ot {
namespace Cli {

constexpr TcpExample::Command TcpExample::sCommands[];

TcpExample::TcpExample(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mEchoBufferLength(0)
    , mEchoServerEnabled(false)
    , mSwallowEnabled(false)
    , mEmitEnabled(false)
    , mEchoBytesSize(0)
    , mSwallowBytesSize(0)
    , mEmitBytesSize(0)
#endif
{
    memset(&mSocket, 0, sizeof(mSocket));
}

otError TcpExample::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError TcpExample::ProcessBind(uint8_t aArgsLength, char *aArgs[])
{
    otError    error;
    otSockAddr sockaddr;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], sockaddr.mAddress));
    SuccessOrExit(error = ParseAsUint16(aArgs[1], sockaddr.mPort));

    error = otTcpBind(&mSocket, &sockaddr);

exit:
    return error;
}

otError TcpExample::ProcessConnect(uint8_t aArgsLength, char *aArgs[])
{
    otError    error;
    otSockAddr sockaddr;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], sockaddr.mAddress));
    SuccessOrExit(error = ParseAsUint16(aArgs[1], sockaddr.mPort));

    error = otTcpConnect(&mSocket, &sockaddr);

exit:
    return error;
}

otError TcpExample::ProcessClose(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otTcpClose(&mSocket);
    return OT_ERROR_NONE;
}

otError TcpExample::ProcessAbort(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otTcpAbort(&mSocket);
    return OT_ERROR_NONE;
}

otError TcpExample::ProcessInit(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otTcpInitialize(mInterpreter.mInstance, &mSocket, TcpEventHandler, this);
    return OT_ERROR_NONE;
}

otError TcpExample::ProcessListen(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    return otTcpListen(&mSocket);
}

otError TcpExample::ProcessSend(uint8_t aArgsLength, char *aArgs[])
{
    otError     error         = OT_ERROR_NONE;
    uint8_t     curArg        = 0;
    uint16_t    payloadLength = 0;
    PayloadType payloadType   = kTypeText;
    uint16_t    writtenLength = 0;

    VerifyOrExit(aArgsLength >= 1 && aArgsLength <= 2, error = OT_ERROR_INVALID_ARGS);

    if (aArgsLength == 2)
    {
        uint8_t typePos = curArg++;

        if (strcmp(aArgs[typePos], "-s") == 0)
        {
            payloadType = kTypeAutoSize;
            SuccessOrExit(error = ParseAsUint16(aArgs[curArg], payloadLength));
        }
        else if (strcmp(aArgs[typePos], "-x") == 0)
        {
            payloadLength = static_cast<uint16_t>(strlen(aArgs[curArg]));
            payloadType   = kTypeHexString;
        }
        else if (strcmp(aArgs[typePos], "-t") == 0)
        {
            payloadType = kTypeText;
        }
    }

    switch (payloadType)
    {
    case kTypeText:
        writtenLength += otTcpWrite(&mSocket, reinterpret_cast<uint8_t *>(aArgs[curArg]),
                                    static_cast<uint16_t>(strlen(aArgs[curArg])));
        break;
    case kTypeAutoSize:
        // TODO: implement kTypeAutoSize
        OT_ASSERT(false);
        break;
    case kTypeHexString:
    {
        uint8_t     buf[50];
        uint16_t    bufLen;
        uint16_t    conversionLength = 0;
        const char *hexString        = aArgs[curArg];

        while (payloadLength > 0)
        {
            bufLen = sizeof(buf);
            SuccessOrExit(error = ParseAsHexString(hexString, bufLen, buf, Utils::CmdLineParser::kAllowTruncate));
            VerifyOrExit(bufLen > 0, error = OT_ERROR_INVALID_ARGS);

            conversionLength = static_cast<uint16_t>(bufLen * 2);

            if ((payloadLength & 0x01) != 0)
            {
                conversionLength -= 1;
            }

            hexString += conversionLength;
            payloadLength -= conversionLength;
            writtenLength += otTcpWrite(&mSocket, buf, bufLen);
        }
        break;
    }
    }

    mInterpreter.OutputFormat("%d written\r\n", writtenLength);

exit:

    return error;
}

otError TcpExample::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError        error = OT_ERROR_INVALID_ARGS;
    const Command *command;

    VerifyOrExit(aArgsLength != 0, IgnoreError(ProcessHelp(0, nullptr)));

    command = Utils::LookupTable::Find(aArgs[0], sCommands);
    VerifyOrExit(command != nullptr, error = OT_ERROR_INVALID_COMMAND);

    error = (this->*command->mHandler)(aArgsLength - 1, aArgs + 1);

exit:
    return error;
}

void TcpExample::TcpEventHandler(otTcpSocket *aSocket, otTcpSocketEvent aEvent)
{
    static_cast<TcpExample *>(otTcpGetContext(aSocket))->HandleTcpEvent(*aSocket, aEvent);
}

void TcpExample::HandleTcpEvent(otTcpSocket &aSocket, otTcpSocketEvent aEvent)
{
    switch (aEvent)
    {
    case OT_TCP_SOCKET_CONNECTED:
        mInterpreter.OutputFormat("TCP connected: ");
        mInterpreter.OutputIp6Address(otTcpGetSockName(&aSocket)->mAddress);
        mInterpreter.OutputFormat(":%d <- ", otTcpGetSockName(&aSocket)->mPort);
        mInterpreter.OutputIp6Address(otTcpGetPeerName(&aSocket)->mAddress);
        mInterpreter.OutputFormat(":%d", otTcpGetPeerName(&aSocket)->mPort);
        mInterpreter.OutputLine("");
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (mEmitEnabled)
        {
            HandleEmit();
        }
#endif
        break;

    case OT_TCP_SOCKET_DISCONNECTED:
        mInterpreter.OutputFormat("TCP disconnected: ");
        mInterpreter.OutputIp6Address(otTcpGetSockName(&aSocket)->mAddress);
        mInterpreter.OutputFormat(":%d <- ", otTcpGetSockName(&aSocket)->mPort);
        mInterpreter.OutputIp6Address(otTcpGetPeerName(&aSocket)->mAddress);
        mInterpreter.OutputFormat(":%d", otTcpGetPeerName(&aSocket)->mPort);
        mInterpreter.OutputLine("");
        break;

    case OT_TCP_SOCKET_ABORTED:
        mInterpreter.OutputFormat("TCP aborted\r\n");
        break;

    case OT_TCP_SOCKET_CLOSED:
        mInterpreter.OutputFormat("TCP closed\r\n");
        break;

    case OT_TCP_SOCKET_DATA_RECEIVED:
    case OT_TCP_SOCKET_DATA_SENT:
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        mInterpreter.OutputLine("TCP received/sent! echo=%s, swallow=%s", mEchoServerEnabled ? "Y" : "N",
                                mSwallowEnabled ? "Y" : "N");

        if (mEchoServerEnabled)
        {
            HandleEcho();
        }
        else if (mSwallowEnabled)
        {
            HandleSwallow();
        }

        if (mEmitEnabled)
        {
            HandleEmit();
        }
#endif
        break;
    }
}

otError TcpExample::ProcessState(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otTcpState state = otTcpGetState(&mSocket);

    mInterpreter.OutputLine(otTcpStateToString(state));

    return OT_ERROR_NONE;
}

otError TcpExample::ProcessRecv(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    uint8_t  buf[64];
    uint16_t readLen;

    readLen = otTcpRead(&mSocket, buf, sizeof(buf));
    OT_ASSERT(readLen <= sizeof(buf));

    mInterpreter.OutputFormat("TCP[");
    mInterpreter.OutputBytes(buf, static_cast<uint8_t>(readLen));
    mInterpreter.OutputLine("]");

    return OT_ERROR_NONE;
}

otError TcpExample::ProcessRoundTripTime(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    otError error = OT_ERROR_NONE;

    uint32_t minRTT, maxRTT;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint32(aArgs[0], minRTT));
    SuccessOrExit(error = ParseAsUint32(aArgs[1], maxRTT));

    error = otTcpConfigRoundTripTime(&mSocket, minRTT, maxRTT);

exit:
    return error;
}

otError TcpExample::ProcessInfo(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    otError           error    = OT_ERROR_NONE;
    const otSockAddr *sockAddr = otTcpGetSockName(&mSocket);
    const otSockAddr *peerAddr = otTcpGetPeerName(&mSocket);

    mInterpreter.OutputLine("State: %s", otTcpStateToString(otTcpGetState(&mSocket)));
    mInterpreter.OutputFormat("LocalAddr: [");
    mInterpreter.OutputIp6Address(sockAddr->mAddress);
    mInterpreter.OutputLine("]:%d", sockAddr->mPort);
    mInterpreter.OutputFormat("PeerAddr: [");
    mInterpreter.OutputIp6Address(peerAddr->mAddress);
    mInterpreter.OutputLine("]:%d", peerAddr->mPort);

    return error;
}
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError TcpExample::ProcessEcho(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mSwallowEnabled, error = OT_ERROR_INVALID_STATE);

    mEchoServerEnabled = true;
    HandleSwallow();
exit:
    return error;
}

otError TcpExample::ProcessSwallow(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mEchoServerEnabled, error = OT_ERROR_INVALID_STATE);

    mSwallowEnabled = true;
exit:
    return error;
}

otError TcpExample::ProcessEmit(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    mEmitEnabled = true;
    HandleEmit();
    return OT_ERROR_NONE;
}

void TcpExample::HandleEcho(void)
{
    uint16_t n;

    while (true)
    {
        n = otTcpRead(&mSocket, mEchoBuffer + mEchoBufferLength, sizeof(mEchoBuffer) - mEchoBufferLength);
        OT_ASSERT(n <= sizeof(mEchoBuffer) - mEchoBufferLength);
        mEchoBufferLength += n;

        n = TryEchoWrite();

        if (n == 0)
        {
            break;
        }
    }
}

uint16_t TcpExample::TryEchoWrite(void)
{
    uint16_t n = 0;

    n = otTcpWrite(&mSocket, mEchoBuffer, mEchoBufferLength);
    OT_ASSERT(n <= mEchoBufferLength);

    if (n > 0)
    {
        mEchoBytesSize += n;
        mEchoBufferLength -= n;
        memmove(mEchoBuffer, mEchoBuffer + n, mEchoBufferLength);
        mInterpreter.OutputLine("TCP echoed: %d", n);
    }

    return n;
}

void TcpExample::HandleSwallow(void)
{
    uint8_t  buf[128];
    uint32_t swallowBytesNum = 0;

    while (true)
    {
        uint16_t n = otTcpRead(&mSocket, buf, sizeof(buf));

        if (n == 0)
        {
            break;
        }

        swallowBytesNum += n;
    }

    mSwallowBytesSize += swallowBytesNum;
    mInterpreter.OutputLine("TCP swallowed %uB", swallowBytesNum);
}

void TcpExample::HandleEmit(void)
{
    uint8_t buf[129] = "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234"
                       "567890123456789012345678901234567";
    uint32_t emitBytesNum = 0;

    while (true)
    {
        uint16_t n = otTcpWrite(&mSocket, buf, sizeof(buf) - 1);

        if (n == 0)
        {
            break;
        }

        emitBytesNum += n;
    }

    mEmitBytesSize += emitBytesNum;
    mInterpreter.OutputLine("TCP emitted %uB", emitBytesNum);
}

otError TcpExample::ProcessResetNextSegment(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otTcpResetNextSegment(&mSocket);

    return OT_ERROR_NONE;
}

otError TcpExample::ProcessCounters(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    mInterpreter.OutputLine("echo=%u", mEchoBytesSize);
    mInterpreter.OutputLine("swallow=%u", mSwallowBytesSize);
    mInterpreter.OutputLine("emit=%u", mEmitBytesSize);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    {
        otTcpCounters counters;

        otTcpGetCounters(mInterpreter.mInstance, &counters);

        mInterpreter.OutputLine("tx_seg=%u", counters.mTxSegment);
        mInterpreter.OutputLine("tx_seg_full=%u", counters.mTxFullSegment);
        mInterpreter.OutputLine("tx_ack=%u", counters.mTxAck);
        mInterpreter.OutputLine("rx_seg=%u", counters.mRxSegment);
        mInterpreter.OutputLine("rx_seg_full=%u", counters.mRxFullSegment);
        mInterpreter.OutputLine("rx_ack=%u", counters.mRxAck);
        mInterpreter.OutputLine("retx=%u", counters.mRetx);
    };
#endif

    return OT_ERROR_NONE;
}

otError TcpExample::ProcessDrop(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;
    uint8_t dropProb;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint8(aArgs[0], dropProb));
    VerifyOrExit(dropProb <= 100, error = OT_ERROR_INVALID_ARGS);

    otTcpSetSegmentRandomDropProb(mInterpreter.mInstance, dropProb);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
