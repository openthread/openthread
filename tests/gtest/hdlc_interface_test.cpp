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

#include <gtest/gtest.h>

#if defined(__APPLE__) || defined(__NetBSD__)
#include <util.h>
#elif defined(__FreeBSD__)
#include <libutil.h>
#else
#include <pty.h>
#endif

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include "lib/url/url.hpp"
#include "posix/platform/hdlc_interface.hpp"

#ifndef __THROW
#define __THROW
#endif

static tcflag_t sLastTermiosCflag = 0;

extern "C" int tcsetattr(int aFd, int aOptionalActions, const struct termios *aTermios) __THROW
{
    using TcsetattrFn = int (*)(int, int, const struct termios *);

    TcsetattrFn    realTcsetattr = reinterpret_cast<TcsetattrFn>(dlsym(RTLD_NEXT, "tcsetattr"));
    struct termios sanitized     = *aTermios;

    sLastTermiosCflag = aTermios->c_cflag;

    // Linux's PTY driver (`pty_set_termios`) ignores `PARENB | PARODD | CSTOPB`, causing glibc's
    // `tcsetattr()` readback check to fail with `EINVAL` on a PTY slave. Strip framing bits before
    // forwarding to the real `tcsetattr()` while recording the requested `c_cflag` for assertions.
    sanitized.c_cflag &= ~static_cast<tcflag_t>(PARENB | PARODD | CSTOPB);

    return realTcsetattr(aFd, aOptionalActions, &sanitized);
}

namespace ot {
namespace Posix {
namespace {

class ScopedPty
{
public:
    ScopedPty(void)
        : mMasterFd(-1)
        , mSlaveFd(-1)
    {
        mSlavePath[0] = '\0';
    }

    ~ScopedPty(void)
    {
        if (mSlaveFd != -1)
        {
            close(mSlaveFd);
        }

        if (mMasterFd != -1)
        {
            close(mMasterFd);
        }
    }

    bool Open(void) { return openpty(&mMasterFd, &mSlaveFd, mSlavePath, nullptr, nullptr) == 0; }

    const char *GetSlavePath(void) const { return mSlavePath; }

private:
    int  mMasterFd;
    int  mSlaveFd;
    char mSlavePath[PATH_MAX];
};

void DummyReceiveCallback(void *) {}

TEST(PosixHdlcInterface, CalculateBusSpeedAccountsForFramingBits)
{
    // 8N1: 10 wire bits per 8-bit byte.
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(460800, /* aHasParity */ false, 1), 368640u);
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(115200, /* aHasParity */ false, 1), 92160u);
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(4000000, /* aHasParity */ false, 1), 3200000u);

    // 8E1 / 8O1: 11 wire bits per 8-bit byte.
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(460800, /* aHasParity */ true, 1), 335127u);
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(115200, /* aHasParity */ true, 1), 83781u);

    // 8N2: 11 wire bits per 8-bit byte.
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(115200, /* aHasParity */ false, 2), 83781u);

    // 8O2 / 8E2: 12 wire bits per 8-bit byte.
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(115200, /* aHasParity */ true, 2), 76800u);

    // Out-of-range stop bits (1 + 8 + 247 = 256) must not wrap to 0 or divide by zero.
    EXPECT_EQ(HdlcInterface::CalculateBusSpeed(115200, /* aHasParity */ false, 247), 3600u);
}

TEST(PosixHdlcInterface, PtyUartInitAndDeinitUpdatesBusSpeed)
{
    struct TestCase
    {
        const char *mParams;
        uint32_t    mExpectedBusSpeed;
        tcflag_t    mExpectedFramingFlags;
    };

    static const TestCase kTestCases[] = {
        {"uart-baudrate=115200", 92160u, 0},
        {"uart-baudrate=115200&uart-parity=even", 83781u, static_cast<tcflag_t>(PARENB)},
        {"uart-baudrate=115200&uart-parity=odd", 83781u, static_cast<tcflag_t>(PARENB | PARODD)},
        {"uart-baudrate=115200&uart-stop=2", 83781u, static_cast<tcflag_t>(CSTOPB)},
        {"uart-baudrate=115200&uart-parity=odd&uart-stop=2", 76800u, static_cast<tcflag_t>(PARENB | PARODD | CSTOPB)},
    };

    ScopedPty pty;

    ASSERT_TRUE(pty.Open());

    for (const TestCase &testCase : kTestCases)
    {
        Spinel::SpinelInterface::RxFrameBuffer rxBuffer;
        char                                   urlStr[PATH_MAX + 128];
        Url::Url                               url;

        snprintf(urlStr, sizeof(urlStr), "spinel+hdlc+uart://%s?%s", pty.GetSlavePath(), testCase.mParams);
        ASSERT_EQ(url.Init(urlStr), OT_ERROR_NONE) << testCase.mParams;

        sLastTermiosCflag = 0;

        HdlcInterface interface(url);
        EXPECT_EQ(interface.GetBusSpeed(), 0u) << testCase.mParams;
        ASSERT_EQ(interface.Init(DummyReceiveCallback, nullptr, rxBuffer), OT_ERROR_NONE) << testCase.mParams;
        EXPECT_EQ(interface.GetBusSpeed(), testCase.mExpectedBusSpeed) << testCase.mParams;
        EXPECT_EQ(sLastTermiosCflag & static_cast<tcflag_t>(PARENB | PARODD | CSTOPB), testCase.mExpectedFramingFlags)
            << testCase.mParams;

        interface.Deinit();
        EXPECT_EQ(interface.GetBusSpeed(), 0u) << testCase.mParams;
    }
}

TEST(PosixHdlcInterface, UnixSocketBusSpeedIsZero)
{
    char                                   sockTemplate[] = "/tmp/ot-hdlc-test-XXXXXX";
    char                                  *dirPath        = mkdtemp(sockTemplate);
    char                                   sockPath[PATH_MAX];
    char                                   urlStr[PATH_MAX + 32];
    int                                    listenFd = -1;
    struct sockaddr_un                     addr;
    Url::Url                               url;
    Spinel::SpinelInterface::RxFrameBuffer rxBuffer;

    ASSERT_NE(dirPath, nullptr);
    snprintf(sockPath, sizeof(sockPath), "%s/rcp.sock", dirPath);

    listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(listenFd, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    ASSERT_LT(strlen(sockPath), sizeof(addr.sun_path));
    memcpy(addr.sun_path, sockPath, strlen(sockPath));

    ASSERT_EQ(bind(listenFd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)), 0);
    ASSERT_EQ(listen(listenFd, 1), 0);

    snprintf(urlStr, sizeof(urlStr), "spinel+hdlc+unix://%s", sockPath);
    ASSERT_EQ(url.Init(urlStr), OT_ERROR_NONE);

    {
        HdlcInterface interface(url);
        ASSERT_EQ(interface.Init(DummyReceiveCallback, nullptr, rxBuffer), OT_ERROR_NONE);
        EXPECT_EQ(interface.GetBusSpeed(), 0u);
    }

    close(listenFd);
    unlink(sockPath);
    rmdir(dirPath);
}

} // namespace
} // namespace Posix
} // namespace ot
