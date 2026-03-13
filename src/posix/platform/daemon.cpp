/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "daemon.hpp"

#if OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
#include <cutils/sockets.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <openthread/cli.h>
#include <openthread/instance.h>

#if defined(HAVE_LIBEDIT)
#include <editline/readline.h>
#elif defined(HAVE_LIBREADLINE)
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include "platform-posix.h"
#include "utils.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_POSIX_CONFIG_DAEMON_CLI_ENABLE

#define OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME ".lock"
static_assert(sizeof(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME) < sizeof(sockaddr_un::sun_path),
              "OpenThread daemon socket name too long!");

namespace ot {
namespace Posix {

namespace {

char *TrimLine(char *aLine)
{
    char *end;

    while (isspace(static_cast<unsigned char>(*aLine)))
    {
        aLine++;
    }

    end = aLine + strlen(aLine);

    while (end > aLine && isspace(static_cast<unsigned char>(end[-1])))
    {
        end--;
    }

    *end = '\0';

    return aLine;
}

typedef char(Filename)[sizeof(sockaddr_un::sun_path)];

} // namespace

otError Daemon::SetOutputFd(int aFd)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!otCliIsCommandPending(), error = OT_ERROR_BUSY);

    mOutputFd = aFd;
    if (aFd != -1)
    {
        mExternalOutputCallback = nullptr;
    }

exit:
    return error;
}

void Daemon::Reject(const char *aFormat, ...)
{
    va_list ap;

    va_start(ap, aFormat);
    mExternalOutputCallback(mExternalOutputCallbackContext, aFormat, ap);
    va_end(ap);
    mExternalOutputCallbackContext = nullptr;
    mExternalOutputCallback        = nullptr;
}

void Daemon::ProcessLine(char *aLine, int aOutputFd)
{
    static constexpr char kBusyMessage[] = "Error: CLI is busy, please try again later.\r\n> ";

    aLine = TrimLine(aLine);
    if (SetOutputFd(aOutputFd) == OT_ERROR_NONE)
    {
        otCliInputLine(aLine);
    }
    else if (mExternalOutputCallback != nullptr)
    {
        Reject(kBusyMessage);
    }
    else if (aOutputFd == STDOUT_FILENO)
    {
        printf(kBusyMessage);
        fflush(stdout);
    }
    else if (aOutputFd != -1)
    {
        UnixSocketOutput(kBusyMessage);
    }
}

void Daemon::ProcessCommand(const char *aLine, void *aContext, otCliOutputCallback aCallback)
{
    char line[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH];

    mExternalOutputCallback        = aCallback;
    mExternalOutputCallbackContext = aContext;

    if (aLine != nullptr && aCallback != nullptr)
    {
        strncpy(line, aLine, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        ProcessLine(const_cast<char *>(aLine), -1);
    }
}

int Daemon::OutputCallback(void *aContext, const char *aFormat, va_list aArguments)
{
    Daemon *daemon = static_cast<Daemon *>(aContext);
    int     rval   = 0;

    if (daemon->mExternalOutputCallback != nullptr)
    {
        rval = daemon->mExternalOutputCallback(daemon->mExternalOutputCallbackContext, aFormat, aArguments);
        if (!otCliIsCommandPending())
        {
            daemon->mExternalOutputCallbackContext = nullptr;
            daemon->mExternalOutputCallback        = nullptr;
        }
    }
    else if (daemon->mOutputFd == STDOUT_FILENO)
    {
        rval = vdprintf(STDOUT_FILENO, aFormat, aArguments);
    }
    else if (daemon->mOutputFd != -1)
    {
        rval = daemon->UnixSocketOutputV(aFormat, aArguments);
    }
    return rval;
}

// using macro to avoid the warning about format-nonliteral
#define GetFilename(aFilename, aPattern)                                                                       \
    do                                                                                                         \
    {                                                                                                          \
        int rval = snprintf(aFilename, sizeof(aFilename), aPattern,                                            \
                            (gNetifName[0] ? gNetifName : OPENTHREAD_POSIX_CONFIG_THREAD_NETIF_DEFAULT_NAME)); \
        VerifyOrDie(rval > 0 && static_cast<size_t>(rval) < sizeof(aFilename), OT_EXIT_INVALID_ARGUMENTS);     \
    } while (0)

const char Daemon::kLogModuleName[] = "Daemon";

int Daemon::UnixSocketOutput(const char *aFormat, ...)
{
    int     ret;
    va_list ap;

    va_start(ap, aFormat);
    ret = UnixSocketOutputV(aFormat, ap);
    va_end(ap);

    return ret;
}

int Daemon::UnixSocketOutputV(const char *aFormat, va_list aArguments)
{
    static constexpr char truncatedMsg[] = "(truncated ...)";
    char                  buf[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH];
    int                   rval;

    static_assert(sizeof(truncatedMsg) < OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH,
                  "OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH is too short!");

    rval = vsnprintf(buf, sizeof(buf), aFormat, aArguments);
    VerifyOrExit(rval >= 0, LogWarn("Failed to format CLI output: %s", strerror(errno)));

    if (rval >= static_cast<int>(sizeof(buf)))
    {
        rval = static_cast<int>(sizeof(buf) - 1);
        memcpy(buf + sizeof(buf) - sizeof(truncatedMsg), truncatedMsg, sizeof(truncatedMsg));
    }

    VerifyOrExit(mSessionSocket != -1);

#ifdef __linux__
    // Don't die on SIGPIPE
    rval = send(mSessionSocket, buf, static_cast<size_t>(rval), MSG_NOSIGNAL);
#else
    rval = static_cast<int>(write(mSessionSocket, buf, static_cast<size_t>(rval)));
#endif

    if (rval < 0)
    {
        LogWarn("Failed to write CLI output: %s", strerror(errno));
        close(mSessionSocket);
        mSessionSocket = -1;
    }

exit:
    return rval;
}

void Daemon::InitializeSessionSocket(void)
{
    int newSessionSocket;
    int rval;

    VerifyOrExit((newSessionSocket = accept(mListenSocket, nullptr, nullptr)) != -1, rval = -1);

    VerifyOrExit((rval = fcntl(newSessionSocket, F_GETFD, 0)) != -1);

    rval |= FD_CLOEXEC;

    VerifyOrExit((rval = fcntl(newSessionSocket, F_SETFD, rval)) != -1);

#ifndef __linux__
    // some platforms (macOS, Solaris) don't have MSG_NOSIGNAL
    // SOME of those (macOS, but NOT Solaris) support SO_NOSIGPIPE
    // if we have SO_NOSIGPIPE, then set it. Otherwise, we're going
    // to simply ignore it.
#if defined(SO_NOSIGPIPE)
    rval = setsockopt(newSessionSocket, SOL_SOCKET, SO_NOSIGPIPE, &rval, sizeof(rval));
    VerifyOrExit(rval != -1);
#else
#warning "no support for MSG_NOSIGNAL or SO_NOSIGPIPE"
#endif
#endif // __linux__

    if (mSessionSocket != -1)
    {
        close(mSessionSocket);
    }
    mSessionSocket = newSessionSocket;

exit:
    if (rval == -1)
    {
        LogWarn("Failed to initialize session socket: %s", strerror(errno));
        if (newSessionSocket != -1)
        {
            close(newSessionSocket);
        }
    }
    else
    {
        LogInfo("Session socket is ready");
    }
}

#if OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
void Daemon::createListenSocketOrDie(void)
{
    Filename socketFile;

    // Don't use OPENTHREAD_POSIX_DAEMON_SOCKET_NAME because android_get_control_socket
    // below already assumes parent /dev/socket dir
    GetFilename(socketFile, "ot-daemon/%s.sock");

    // This returns the init-managed stream socket which is already bind to
    // /dev/socket/ot-daemon/<interface-name>.sock
    mListenSocket = android_get_control_socket(socketFile);

    if (mListenSocket == -1)
    {
        DieNowWithMessage("android_get_control_socket", OT_EXIT_ERROR_ERRNO);
    }
}
#else
void Daemon::UnixSocketCreate(void)
{
    struct sockaddr_un sockname;
    int                ret;

    class AllowAllGuard
    {
    public:
        AllowAllGuard(void)
        {
            const char *allowAll = getenv("OT_DAEMON_ALLOW_ALL");
            mAllowAll            = (allowAll != nullptr && strcmp("1", allowAll) == 0);

            if (mAllowAll)
            {
                mMode = umask(0);
            }
        }
        ~AllowAllGuard(void)
        {
            if (mAllowAll)
            {
                umask(mMode);
            }
        }

    private:
        bool   mAllowAll = false;
        mode_t mMode     = 0;
    };

    mListenSocket = SocketWithCloseExec(AF_UNIX, SOCK_STREAM, 0, kSocketNonBlock);

    if (mListenSocket == -1)
    {
        DieNow(OT_EXIT_FAILURE);
    }

    {
        static_assert(sizeof(OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK) == sizeof(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME),
                      "sock and lock file name pattern should have the same length!");
        Filename lockfile;

        GetFilename(lockfile, OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK);

        mDaemonLock = open(lockfile, O_CREAT | O_RDONLY | O_CLOEXEC, 0600);
    }

    if (mDaemonLock == -1)
    {
        DieNowWithMessage("open", OT_EXIT_ERROR_ERRNO);
    }

    if (flock(mDaemonLock, LOCK_EX | LOCK_NB) == -1)
    {
        DieNowWithMessage("flock", OT_EXIT_ERROR_ERRNO);
    }

    memset(&sockname, 0, sizeof(struct sockaddr_un));

    sockname.sun_family = AF_UNIX;
    GetFilename(sockname.sun_path, OPENTHREAD_POSIX_DAEMON_SOCKET_NAME);
    (void)unlink(sockname.sun_path);

    {
        AllowAllGuard allowAllGuard;

        ret = bind(mListenSocket, reinterpret_cast<const struct sockaddr *>(&sockname), sizeof(struct sockaddr_un));
    }

    if (ret == -1)
    {
        DieNowWithMessage("bind", OT_EXIT_ERROR_ERRNO);
    }
}
#endif // OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE

void Daemon::SetUp(uint8_t aMode)
{
    int ret;

    mDaemonMode = aMode;

    otCliInit(gInstance, OutputCallback, this);

    if (mDaemonMode & OT_POSIX_DAEMON_MODE_CONSOLE)
    {
#if defined(HAVE_LIBEDIT) || defined(HAVE_LIBREADLINE)
        mReadline.Init();
#else
        mStdio.Init();
#endif
    }

    VerifyOrExit(mDaemonMode & OT_POSIX_DAEMON_MODE_UNIX_SOCKET);

    // This allows implementing pseudo reset.
    VerifyOrExit(mListenSocket == -1);
    UnixSocketCreate();

    //
    // only accept 1 connection.
    //
    ret = listen(mListenSocket, 1);
    if (ret == -1)
    {
        DieNowWithMessage("listen", OT_EXIT_ERROR_ERRNO);
    }

exit:
    Mainloop::Manager::Get().Add(*this);
}

void Daemon::TearDown(void)
{
    Mainloop::Manager::Get().Remove(*this);

    if (mDaemonMode & OT_POSIX_DAEMON_MODE_CONSOLE)
    {
#if defined(HAVE_LIBEDIT) || defined(HAVE_LIBREADLINE)
        mReadline.Deinit();
#else
        mStdio.Deinit();
#endif
    }

    if (mSessionSocket != -1)
    {
        close(mSessionSocket);
        mSessionSocket = -1;
    }

#if !OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
    // The `mListenSocket` is managed by `init` on Android
    if (mListenSocket != -1)
    {
        close(mListenSocket);
        mListenSocket = -1;
    }

    if (gPlatResetReason != OT_PLAT_RESET_REASON_SOFTWARE)
    {
        Filename sockfile;

        GetFilename(sockfile, OPENTHREAD_POSIX_DAEMON_SOCKET_NAME);
        LogDebg("Removing daemon socket: %s", sockfile);
        (void)unlink(sockfile);
    }

    if (mDaemonLock != -1)
    {
        (void)flock(mDaemonLock, LOCK_UN);
        close(mDaemonLock);
        mDaemonLock = -1;
    }
#endif
}

void Daemon::Update(Mainloop::Context &aContext)
{
    // Do not process new input if it's busy.
    VerifyOrExit(!otCliIsCommandPending());

    Mainloop::AddToReadFdSet(mListenSocket, aContext);
    Mainloop::AddToErrorFdSet(mListenSocket, aContext);

    Mainloop::AddToReadFdSet(mSessionSocket, aContext);
    Mainloop::AddToErrorFdSet(mSessionSocket, aContext);

    if (mDaemonMode & OT_POSIX_DAEMON_MODE_CONSOLE)
    {
#if defined(HAVE_LIBEDIT) || defined(HAVE_LIBREADLINE)
        mReadline.Update(aContext);
#else
        mStdio.Update(aContext);
#endif
    }

exit:
    return;
}

void Daemon::Process(const Mainloop::Context &aContext)
{
    ssize_t rval;

    VerifyOrExit(mListenSocket != -1);

    if (Mainloop::HasFdErrored(mListenSocket, aContext))
    {
        DieNowWithMessage("daemon socket error", OT_EXIT_FAILURE);
    }
    else if (Mainloop::IsFdReadable(mListenSocket, aContext))
    {
        InitializeSessionSocket();
    }

    VerifyOrExit(mSessionSocket != -1);

    if (Mainloop::HasFdErrored(mSessionSocket, aContext))
    {
        close(mSessionSocket);
        mSessionSocket = -1;
    }
    else if (Mainloop::IsFdReadable(mSessionSocket, aContext))
    {
        uint8_t buffer[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH];

        // leave 1 byte for the null terminator
        rval = read(mSessionSocket, buffer, sizeof(buffer) - 1);

        if (rval > 0)
        {
            buffer[rval] = '\0';
            ProcessLine(reinterpret_cast<char *>(buffer), mSessionSocket);
        }
        else
        {
            if (rval < 0)
            {
                LogWarn("Daemon read: %s", strerror(errno));
            }
            close(mSessionSocket);
            mSessionSocket = -1;
        }
    }

exit:
    if (mDaemonMode & OT_POSIX_DAEMON_MODE_CONSOLE)
    {
#if defined(HAVE_LIBEDIT) || defined(HAVE_LIBREADLINE)
        mReadline.Process(aContext);
#else
        mStdio.Process(aContext);
#endif
    }
}

#if defined(HAVE_LIBEDIT) || defined(HAVE_LIBREADLINE)
void Daemon::Readline::Init(void)
{
    rl_instream           = stdin;
    rl_outstream          = stdout;
    rl_inhibit_completion = true;

    rl_set_screen_size(0, OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH);

    rl_callback_handler_install("> ", InputCallback);
    rl_already_prompted = true;
}

void Daemon::Readline::Deinit(void) { rl_callback_handler_remove(); }

void Daemon::Readline::Update(Mainloop::Context &aContext)
{
    Mainloop::AddToReadFdSet(STDIN_FILENO, aContext);
    Mainloop::AddToErrorFdSet(STDIN_FILENO, aContext);
}

void Daemon::Readline::Process(const Mainloop::Context &aContext)
{
    if (Mainloop::HasFdErrored(STDIN_FILENO, aContext))
    {
        exit(OT_EXIT_FAILURE);
    }

    if (Mainloop::IsFdReadable(STDIN_FILENO, aContext))
    {
        rl_callback_read_char();
    }
}

void Daemon::Readline::InputCallback(char *aLine)
{
    if (aLine != nullptr)
    {
        if (!isspace(aLine[0]))
        {
            add_history(aLine);
        }

        Daemon::Get().ProcessLine(aLine, STDOUT_FILENO);

        free(aLine);
    }
    else
    {
        exit(OT_EXIT_SUCCESS);
    }
}
#else
void Daemon::Stdio::Init(void) {}

void Daemon::Stdio::Deinit(void) {}

void Daemon::Stdio::Update(Mainloop::Context &aContext)
{
    Mainloop::AddToReadFdSet(STDIN_FILENO, aContext);
    Mainloop::AddToErrorFdSet(STDIN_FILENO, aContext);
}

void Daemon::Stdio::Process(const Mainloop::Context &aContext)
{
    if (Mainloop::HasFdErrored(STDIN_FILENO, aContext))
    {
        exit(OT_EXIT_FAILURE);
    }

    if (Mainloop::IsFdReadable(STDIN_FILENO, aContext))
    {
        char buffer[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH];

        if (fgets(buffer, sizeof(buffer), stdin) != nullptr)
        {
            Daemon::Get().ProcessLine(buffer, STDOUT_FILENO);
        }
        else
        {
            exit(OT_EXIT_SUCCESS);
        }
    }
}
#endif

Daemon &Daemon::Get(void)
{
    static Daemon sInstance;

    return sInstance;
}

void otSysCliProcessCommand(void *aContext, otCliOutputCallback aCallback, const char *aLine)
{
    Daemon::Get().ProcessCommand(aLine, aContext, aCallback);
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_CLI_ENABLE
