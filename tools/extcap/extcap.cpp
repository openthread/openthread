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

// C++ Standard Library headers
#include <atomic>
#include <string>

// C Standard Library headers
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

// OpenThread Public headers
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/logging.h>
#include <openthread/openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

// Local headers
#include "pcapng.hpp"
#include "common/code_utils.hpp"

using namespace ot::Extcap;

namespace {

// Translation-unit-local State (prefixed with s to denote static duration)
std::atomic<bool> sShouldQuit{false};
PcapngWriter      sPcapngWriter;
bool              sDebugEnabled = false;

void Log(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);
void LogAndPrint(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

void Log(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    vsyslog(LOG_INFO, aFormat, args);
    va_end(args);
}

void LogAndPrint(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    vsyslog(LOG_ERR, aFormat, args);
    va_end(args);

    va_start(args, aFormat);
    vfprintf(stderr, aFormat, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void InitLogging(void)
{
    openlog("ot-extcap", LOG_PID | LOG_CONS, LOG_DAEMON);
    setlogmask(sDebugEnabled ? LOG_UPTO(LOG_DEBUG) : LOG_UPTO(LOG_WARNING));
}

void SignalHandler(int aSignal)
{
    if (aSignal == SIGINT || aSignal == SIGTERM)
    {
        sShouldQuit = true;
    }
}

uint64_t GetRealTimeUs(void)
{
    struct timespec now = {0, 0};

    clock_gettime(CLOCK_REALTIME, &now);

    return static_cast<uint64_t>(now.tv_sec) * 1000000 + static_cast<uint64_t>(now.tv_nsec) / 1000;
}

/**
 * Maps radio timestamps (the RCP's local clock in microseconds) to microseconds since the Unix epoch, as
 * expected by PCAPNG.
 *
 * A constant offset, anchored on the first received frame (so it includes that frame's delivery latency), is
 * applied to all frames, so the inter-frame timing measured by the RCP is preserved exactly. The offset is
 * re-anchored only if the radio clock goes backwards (e.g. after an RCP reset, or when an RCP reporting 32-bit
 * timestamps wraps around).
 */
class EpochTimeMapper
{
public:
    uint64_t Map(uint64_t aRadioTimeUs)
    {
        if (aRadioTimeUs == 0)
        {
            // The RCP did not provide a timestamp
            return GetRealTimeUs();
        }

        if (aRadioTimeUs < mLastRadioTimeUs)
        {
            Log("Radio clock went backwards (RCP reset?), re-anchoring capture timestamps");
            mAnchored = false;
        }

        if (!mAnchored)
        {
            mOffsetUs = GetRealTimeUs() - aRadioTimeUs;
            mAnchored = true;
        }

        mLastRadioTimeUs = aRadioTimeUs;

        // Unsigned arithmetic (modulo 2^64) also handles a negative offset.
        return aRadioTimeUs + mOffsetUs;
    }

private:
    bool     mAnchored{false};
    uint64_t mOffsetUs{0};
    uint64_t mLastRadioTimeUs{0};
};

EpochTimeMapper sEpochTime;

void HandlePcapCallback(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    otError error;

    OT_UNUSED_VARIABLE(aContext);

    // The sniffer never transmits, so only received frames are captured.
    VerifyOrExit(aFrame != nullptr && !aIsTx && sPcapngWriter.IsOpen());

    error = sPcapngWriter.WriteFrame(*aFrame, sEpochTime.Map(aFrame->mInfo.mRxInfo.mTimestamp));

    if (error != OT_ERROR_NONE)
    {
        if (error == OT_ERROR_FAILED)
        {
            // If the write failed due to EPIPE (broken pipe), it means Wireshark
            // has closed the pipe to stop the capture. We should handle this silently.
            if (errno != EPIPE)
            {
                syslog(LOG_ERR, "Failed to write frame to PCAPNG: %s", strerror(errno));
            }
            sShouldQuit = true;
        }
        else
        {
            syslog(LOG_ERR, "Failed to process frame for PCAPNG: %s", otThreadErrorToString(error));
        }
    }

exit:
    return;
}

void ListInterfaces(void)
{
    printf("extcap {version=1.0.0}{display=OpenThread Sniffer}{help=https://openthread.io}\n");
    printf("interface {value=spinel_radio_url}{display=OpenThread Sniffer over Radio URL...}\n");
}

void ListConfig(const char *aInterface)
{
    printf("arg {number=0}{call=--channel}{display=Channel}{tooltip=IEEE 802.15.4 "
           "channel}{type=selector}{required=true}{default=11}\n");
    // TODO: Retrieve the supported channel mask from the sniffer.
    for (int i = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN; i <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX; ++i)
    {
        printf("value {arg=0}{value=%d}{display=%d}{default=%s}\n", i, i,
               (i == OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN ? "true" : "false"));
    }

    // Expose a checkbox for verbose debugging in the Wireshark GUI
    printf("arg {number=1}{call=--debug}{display=Verbose Debugging}{tooltip=Enable verbose debug "
           "logging}{type=boolflag}{default=false}\n");

    // If the special custom generic interface is selected, dynamically expose the free-form text input
    if (aInterface != nullptr && strcmp(aInterface, "spinel_radio_url") == 0)
    {
        printf("arg {number=2}{call=--radio-url}{display=Radio URL}{tooltip=Enter full Radio URL, e.g. "
               "spinel+hdlc+forkpty://[path_to_rcp] or spinel+hdlc+uart://[port]}{type=string}{required=true}\n");
    }
}

void ListDlts(void) { printf("dlt {number=283}{name=IEEE802_15_4_TAP}{display=IEEE 802.15.4 TAP}\n"); }

std::string ResolveInterfaceToRadioUrl(const char *aInterface, const char *aRadioUrl)
{
    std::string resolvedUrl;
    std::string interfaceVal = aInterface ? aInterface : "";

    if (interfaceVal == "spinel_radio_url")
    {
        VerifyOrExit(aRadioUrl != nullptr && aRadioUrl[0] != '\0',
                     { LogAndPrint("Error: Radio URL must be provided for custom interface"); });

        resolvedUrl = aRadioUrl;
    }
    else
    {
        LogAndPrint("Error: Unknown interface '%s'", interfaceVal.c_str());
    }

exit:
    return resolvedUrl;
}

int RunCapture(const char *aInterface, const char *aFifo, uint8_t aChannel, const char *aRadioUrl)
{
    int              rval     = EXIT_FAILURE;
    otError          error    = OT_ERROR_NONE;
    const char      *step     = nullptr;
    otInstance      *instance = nullptr;
    otPlatformConfig config;
    std::string      radioUrl;
    struct sigaction sa;

    VerifyOrExit(aFifo != nullptr, LogAndPrint("The fifo must be provided to capture"));

    // Quit the main loop on SIGINT/SIGTERM. Ignore SIGPIPE: write errors (EPIPE) are handled when Wireshark
    // closes the FIFO.
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    // 1. Resolve the interface value to a full Radio URL
    radioUrl = ResolveInterfaceToRadioUrl(aInterface, aRadioUrl);
    VerifyOrExit(!radioUrl.empty());

    // 2. Open PCAPNG Writer (blocks until Wireshark opens the FIFO for reading)
    Log("Opening FIFO: %s (blocking until Wireshark opens it)", aFifo);
    VerifyOrExit(sPcapngWriter.Open(aFifo) == OT_ERROR_NONE,
                 LogAndPrint("Failed to open FIFO for writing: %s: %s", aFifo, strerror(errno)));

    // 3. Initialize OpenThread POSIX Platform
    Log("Initializing OpenThread POSIX platform with URL: %s", radioUrl.c_str());
    memset(&config, 0, sizeof(config));
    config.mCoprocessorUrls.mUrls[0] = radioUrl.c_str();
    config.mCoprocessorUrls.mNum     = 1;
    config.mSpeedUpFactor            = 1;

    config.mCoprocessorType = otSysInitCoprocessor(&config.mCoprocessorUrls);
    VerifyOrExit(config.mCoprocessorType == OT_COPROCESSOR_RCP, LogAndPrint("Coprocessor must be RCP"));

    instance = otSysInit(&config);
    VerifyOrExit(instance != nullptr, LogAndPrint("Failed to initialize OpenThread instance"));

    // OpenThread starts at debug level (see CMakeLists.txt). It can only be changed once the instance exists.
    IgnoreError(otLoggingSetLevel(sDebugEnabled ? OT_LOG_LEVEL_DEBG : OT_LOG_LEVEL_WARN));

    // 4. Capture all received frames on the channel: enable the link layer (the interface stays down), set
    //    the channel and enable promiscuous mode.
    otLinkSetPcapCallback(instance, HandlePcapCallback, nullptr);

    step = "enable link layer";
    SuccessOrExit(error = otLinkSetEnabled(instance, true));
    step = "set channel";
    SuccessOrExit(error = otLinkSetChannel(instance, aChannel));
    step = "enable promiscuous mode";
    SuccessOrExit(error = otLinkSetPromiscuous(instance, true));

    Log("Capturing on channel %d...", aChannel);
    rval = EXIT_SUCCESS;

    // 5. Main Loop
    while (!sShouldQuit)
    {
        otSysMainloopContext mainloopContext;

        mainloopContext.mMaxFd = -1;
        FD_ZERO(&mainloopContext.mReadFdSet);
        FD_ZERO(&mainloopContext.mWriteFdSet);
        FD_ZERO(&mainloopContext.mErrorFdSet);
        mainloopContext.mTimeout.tv_sec  = 10;
        mainloopContext.mTimeout.tv_usec = 0;

        otTaskletsProcess(instance);
        otSysMainloopUpdate(instance, &mainloopContext);

        if (otSysMainloopPoll(&mainloopContext) >= 0)
        {
            otSysMainloopProcess(instance, &mainloopContext);
        }
        else if (errno != EINTR)
        {
            LogAndPrint("select() failed in main loop: %s", strerror(errno));
            rval = EXIT_FAILURE;
            break;
        }
    }

    IgnoreError(otLinkSetPromiscuous(instance, false));
    IgnoreError(otLinkSetEnabled(instance, false));

exit:
    if (error != OT_ERROR_NONE)
    {
        LogAndPrint("Failed to %s: %s", step, otThreadErrorToString(error));
    }

    sPcapngWriter.Close();

    if (instance != nullptr)
    {
        otLinkSetPcapCallback(instance, nullptr, nullptr);
    }

    // Also finalizes the instance, or tears down the Spinel manager if the coprocessor is not an RCP.
    otSysDeinit();

    return rval;
}

void PrintHelp(void)
{
    printf("OpenThread Wireshark Extcap Sniffer (ot-extcap)\n\n");
    printf("Usage:\n");
    printf("  ot-extcap [options]\n\n");
    printf("Options:\n");
    printf("  --capture                       Start live packet capture\n");
    printf("  --channel <ch>                  IEEE 802.15.4 channel to capture (range: 11-26, default: 11)\n");
    printf("  --debug                         Enable verbose debug logging to syslog (LOG_DEBUG)\n");
    printf("  --extcap-config                 List configuration options for a selected interface\n");
    printf("  --extcap-dlts                   List DLTs (Data Link Types) for a selected interface\n");
    printf("  --extcap-interface <interface>  Specify the interface for capture or configuration\n");
    printf("  --extcap-interfaces             List available OpenThread sniffer interfaces\n");
    printf("  --extcap-version <version>      Specify the Wireshark version string\n");
    printf("  --fifo <path>                   Specify the FIFO pipe for dumping captured PCAPNG packets\n");
    printf("  -h, --help                      Display this help message\n");
    printf("  --radio-url <url>               Specify the Radio URL for generic custom interface\n");
}

} // namespace

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    syslog(LOG_ERR, "OpenThread instance requested reset, exiting");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int rval = EXIT_SUCCESS;

    if (getuid() != geteuid() || getgid() != getegid())
    {
        fprintf(stderr, "Error: Running with elevated privileges (setuid/setgid) is not allowed.\n");
        return EXIT_FAILURE;
    }

    // Wireshark's extcap protocol only uses long options. Long-only options use IDs outside the ASCII range.
    enum Option : int
    {
        kOptHelp = 'h',

        kOptCapture = 256,
        kOptChannel,
        kOptDebug,
        kOptExtcapConfig,
        kOptExtcapDlts,
        kOptExtcapInterface,
        kOptExtcapInterfaces,
        kOptExtcapVersion,
        kOptFifo,
        kOptRadioUrl,
    };

    static const struct option sLongOptions[] = {
        {"capture", no_argument, nullptr, kOptCapture},
        {"channel", required_argument, nullptr, kOptChannel},
        {"debug", no_argument, nullptr, kOptDebug},
        {"extcap-config", no_argument, nullptr, kOptExtcapConfig},
        {"extcap-dlts", no_argument, nullptr, kOptExtcapDlts},
        {"extcap-interface", required_argument, nullptr, kOptExtcapInterface},
        {"extcap-interfaces", no_argument, nullptr, kOptExtcapInterfaces},
        {"extcap-version", required_argument, nullptr, kOptExtcapVersion},
        {"fifo", required_argument, nullptr, kOptFifo},
        {"help", no_argument, nullptr, kOptHelp},
        {"radio-url", required_argument, nullptr, kOptRadioUrl},
        {nullptr, 0, nullptr, 0},
    };

    bool        optInterfaces = false;
    bool        optConfig     = false;
    bool        optDlts       = false;
    bool        optCapture    = false;
    bool        optHelp       = false;
    const char *optInterface  = nullptr;
    const char *optFifo       = nullptr;
    const char *optRadioUrl   = nullptr;
    uint8_t     optChannel    = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN;

    for (int c; (c = getopt_long(argc, argv, "h", sLongOptions, nullptr)) != -1;)
    {
        switch (c)
        {
        case kOptCapture:
            optCapture = true;
            break;
        case kOptChannel:
        {
            char *endPtr = nullptr;
            long  val    = strtol(optarg, &endPtr, 10);

            if (endPtr == optarg || *endPtr != '\0' || val < OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN ||
                val > OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX)
            {
                fprintf(stderr, "Invalid channel %s. Channel must be in the range 11-26.\n", optarg);
                exit(EXIT_FAILURE);
            }
            optChannel = static_cast<uint8_t>(val);
            break;
        }
        case kOptDebug:
            sDebugEnabled = true;
            break;
        case kOptExtcapConfig:
            optConfig = true;
            break;
        case kOptExtcapDlts:
            optDlts = true;
            break;
        case kOptExtcapInterface:
            optInterface = optarg;
            break;
        case kOptExtcapInterfaces:
            optInterfaces = true;
            break;
        case kOptExtcapVersion:
            if (optarg != nullptr)
            {
                int major = 0;
                int minor = 0;

                if (sscanf(optarg, "%d.%d", &major, &minor) >= 1)
                {
                    if (major < 3)
                    {
                        fprintf(stderr, "Unsupported Wireshark version %s. Minimum required version is 3.0.\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            break;
        case kOptFifo:
            optFifo = optarg;
            break;
        case kOptHelp:
            optHelp = true;
            break;
        case kOptRadioUrl:
            optRadioUrl = optarg;
            break;
        default:
            fprintf(stderr, "Invalid option\n");
            exit(EXIT_FAILURE);
        }
    }

    InitLogging();

    Log("Command line options parsed. Interfaces=%d, Config=%d, Dlts=%d, Capture=%d, Interface='%s', "
        "Fifo='%s', Channel=%d, RadioUrl='%s', Debug=%d",
        optInterfaces, optConfig, optDlts, optCapture, optInterface ? optInterface : "", optFifo ? optFifo : "",
        optChannel, optRadioUrl ? optRadioUrl : "", sDebugEnabled);

    if (optHelp || argc == 1)
    {
        PrintHelp();
    }
    else if (optInterfaces)
    {
        ListInterfaces();
    }
    else if (optInterface == nullptr)
    {
        LogAndPrint("Error: An interface must be provided");
        rval = EXIT_FAILURE;
    }
    else if (optConfig)
    {
        ListConfig(optInterface);
    }
    else if (optDlts)
    {
        ListDlts();
    }
    else if (optCapture)
    {
        rval = RunCapture(optInterface, optFifo, optChannel, optRadioUrl);
    }
    else
    {
        LogAndPrint("Error: No action specified (see --help)");
        rval = EXIT_FAILURE;
    }

    closelog();
    return rval;
}
