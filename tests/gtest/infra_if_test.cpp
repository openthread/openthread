/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>

#include <openthread-core-config.h>
#include <openthread/border_routing.h>
#include <openthread/error.h>
#include <openthread/logging.h>
#include <openthread/platform/infra_if.h>

#include "lib/platform/exit_code.h"
#include "posix/platform/infra_if.hpp"
#include "posix/platform/platform-posix.h"

otInstance *gInstance = nullptr;

namespace {

int      sDummyInstance           = 0;
int      sMockSendMsgErrno        = 0;
int      sMockSendMsgCallCount    = 0;
int      sStateChangedCallCount   = 0;
uint32_t sLastStateChangedIfIndex = 0;
otError  sStateChangedError       = OT_ERROR_NONE;

} // namespace

extern "C" {

ssize_t __real_sendmsg(int aSockFd, const struct msghdr *aMsg, int aFlags);

ssize_t __wrap_sendmsg(int aSockFd, const struct msghdr *aMsg, int aFlags)
{
    if (sMockSendMsgErrno != 0)
    {
        sMockSendMsgCallCount++;
        errno = sMockSendMsgErrno;
        return -1;
    }

    return __real_sendmsg(aSockFd, aMsg, aFlags);
}

otBorderRoutingState otBorderRoutingGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_BORDER_ROUTING_STATE_UNINITIALIZED;
}

otError otBorderRoutingInit(otInstance *aInstance, uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);
    OT_UNUSED_VARIABLE(aInfraIfIsRunning);
    return OT_ERROR_NONE;
}

otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnabled);
    return OT_ERROR_NONE;
}

otError otPlatInfraIfStateChanged(otInstance *aInstance, uint32_t aInfraIfIndex, bool aIsRunning)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aIsRunning);

    sStateChangedCallCount++;
    sLastStateChangedIfIndex = aInfraIfIndex;
    return sStateChangedError;
}

void otPlatInfraIfRecvIcmp6Nd(otInstance         *aInstance,
                              uint32_t            aInfraIfIndex,
                              const otIp6Address *aSrcAddress,
                              const uint8_t      *aBuffer,
                              uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);
    OT_UNUSED_VARIABLE(aSrcAddress);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLength);
}

const char *otThreadErrorToString(otError aError)
{
    OT_UNUSED_VARIABLE(aError);
    return "";
}

void otLogCritPlat(const char *aFormat, ...) { OT_UNUSED_VARIABLE(aFormat); }

void otLogWarnPlat(const char *aFormat, ...) { OT_UNUSED_VARIABLE(aFormat); }

void otLogInfoPlat(const char *aFormat, ...) { OT_UNUSED_VARIABLE(aFormat); }

void otLogPlatArgs(otLogLevel aLogLevel, const char *aPlatModuleName, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aPlatModuleName);
    OT_UNUSED_VARIABLE(aFormat);
    OT_UNUSED_VARIABLE(aArgs);
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
    OT_UNUSED_VARIABLE(aFilename);
    OT_UNUSED_VARIABLE(aLineNumber);
    std::abort();
}

} // extern "C"

namespace {

using ::testing::ExitedWithCode;
using ::testing::HasSubstr;
using ::testing::KilledBySignal;

const uint8_t kDummyRaPayload[16] = {134}; // ICMPv6 Router Advertisement header

struct SendOnExitContext
{
    uint32_t     mIfIndex;
    otIp6Address mDestAddress;
};

otIp6Address AllNodesMulticastAddress(void)
{
    otIp6Address addr;

    std::memset(&addr, 0, sizeof(addr));
    addr.mFields.m8[0]  = 0xff;
    addr.mFields.m8[1]  = 0x02;
    addr.mFields.m8[15] = 0x01;
    return addr;
}

void SendIcmp6OnExitHandler(uint8_t aExitCode, void *aContext)
{
    auto   *context = static_cast<const SendOnExitContext *>(aContext);
    otError error = ot::Posix::InfraNetif::Get().SendIcmp6Nd(context->mIfIndex, context->mDestAddress, kDummyRaPayload,
                                                             sizeof(kDummyRaPayload));

    std::fprintf(stderr, "INFRA_SEND:code=%u:failed=%d:send_calls=%d:state_calls=%d:exiting=%d\n",
                 static_cast<unsigned int>(aExitCode), (error == OT_ERROR_FAILED) ? 1 : 0, sMockSendMsgCallCount,
                 sStateChangedCallCount, otExitIsExiting() ? 1 : 0);
    std::fflush(stderr);
}

class InfraNetifSendIcmp6NdTest : public ::testing::Test
{
protected:
    void SetUp(void) override
    {
        int sock = socket(AF_INET6, SOCK_DGRAM, 0);

        ASSERT_GE(sock, 0);

        gInstance                = reinterpret_cast<otInstance *>(&sDummyInstance);
        sMockSendMsgErrno        = 0;
        sMockSendMsgCallCount    = 0;
        sStateChangedCallCount   = 0;
        sLastStateChangedIfIndex = 0;
        sStateChangedError       = OT_ERROR_NONE;

        ot::Posix::InfraNetif::Get().Init();
        ot::Posix::InfraNetif::Get().SetInfraNetif("lo", sock);
        mIfIndex = ot::Posix::InfraNetif::Get().GetNetifIndex();
        ASSERT_NE(mIfIndex, 0u);
    }

    void TearDown(void) override
    {
        sMockSendMsgErrno = 0;
        otExitSetHandler(nullptr, nullptr);
        ot::Posix::InfraNetif::Get().Deinit();
        gInstance = nullptr;
    }

    uint32_t mIfIndex = 0;
};

TEST_F(InfraNetifSendIcmp6NdTest, ChecksLinkStateOnLinkDownErrorsWhenNotExiting)
{
    const int          kErrors[] = {ENETDOWN, ENODEV, EADDRNOTAVAIL, ENXIO};
    const otIp6Address dest      = AllNodesMulticastAddress();

    for (int err : kErrors)
    {
        sMockSendMsgErrno        = err;
        sMockSendMsgCallCount    = 0;
        sStateChangedCallCount   = 0;
        sLastStateChangedIfIndex = 0;

        EXPECT_EQ(ot::Posix::InfraNetif::Get().SendIcmp6Nd(mIfIndex, dest, kDummyRaPayload, sizeof(kDummyRaPayload)),
                  OT_ERROR_FAILED);
        EXPECT_EQ(sMockSendMsgCallCount, 1);
        EXPECT_EQ(sStateChangedCallCount, 1);
        EXPECT_EQ(sLastStateChangedIfIndex, mIfIndex);
    }
}

TEST_F(InfraNetifSendIcmp6NdTest, SkipsLinkStateCheckDuringFatalExitAndPreservesExitCode)
{
    const int         kErrors[] = {ENETDOWN, ENODEV, EADDRNOTAVAIL, ENXIO};
    SendOnExitContext context{mIfIndex, AllNodesMulticastAddress()};

    for (int err : kErrors)
    {
        EXPECT_EXIT(
            {
                sMockSendMsgErrno      = err;
                sMockSendMsgCallCount  = 0;
                sStateChangedCallCount = 0;
                sStateChangedError     = OT_ERROR_FAILED;

                otExitSetHandler(SendIcmp6OnExitHandler, &context);
                DieNow(OT_EXIT_RADIO_SPINEL_NO_RESPONSE);
            },
            ExitedWithCode(OT_EXIT_RADIO_SPINEL_NO_RESPONSE),
            HasSubstr("INFRA_SEND:code=6:failed=1:send_calls=1:state_calls=0:exiting=1"));
    }
}

TEST_F(InfraNetifSendIcmp6NdTest, SkipsLinkStateCheckDuringFatalAbortAndPreservesSigabrt)
{
    const int         kErrors[] = {ENETDOWN, ENODEV};
    SendOnExitContext context{mIfIndex, AllNodesMulticastAddress()};

    for (int err : kErrors)
    {
        EXPECT_EXIT(
            {
                sMockSendMsgErrno      = err;
                sMockSendMsgCallCount  = 0;
                sStateChangedCallCount = 0;
                sStateChangedError     = OT_ERROR_FAILED;

                otExitSetHandler(SendIcmp6OnExitHandler, &context);
                otAbortNow(OT_EXIT_RADIO_SPINEL_RESET);
            },
            KilledBySignal(SIGABRT), HasSubstr("INFRA_SEND:code=4:failed=1:send_calls=1:state_calls=0:exiting=1"));
    }
}

} // namespace
