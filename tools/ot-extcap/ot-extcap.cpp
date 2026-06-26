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
#include <fstream>
#include <string>
#include <vector>

// C Standard Library headers
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <glob.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

// OpenThread Public headers
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/openthread-system.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/time.h>
#include <openthread/platform/toolchain.h>

// Local headers
#include "pcapng.hpp"
#include "common/code_utils.hpp"

using namespace ot::Extcap;

namespace {

// Translation-unit-local State (prefixed with s to denote static duration)
bool         sShouldQuit = false;
PcapngWriter sPcapngWriter;
bool         sDebugEnabled = false;

void LogDebug(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

void LogDebug(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    vsyslog(LOG_DEBUG, aFormat, args);
    va_end(args);
}

void InitLogging(void)
{
    // Always open syslog. We use LOG_PERROR to ensure serious errors
    // are printed to stderr so Wireshark can capture and display them.
    openlog("ot-extcap", LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);

    // Default to only logging WARNING and above to avoid cluttering
    // unless debug is explicitly enabled later.
    setlogmask(LOG_UPTO(LOG_WARNING));
}

void ConfigureLogging(bool aDebugEnabled)
{
    if (aDebugEnabled)
    {
        setlogmask(LOG_UPTO(LOG_DEBUG));
        LogDebug("--- ot-extcap started (debug enabled) ---");
    }
}

struct RadioUrlConfig
{
    std::string mKey; // Raw string key (e.g., "5", "0x5", "05")
    uint64_t    mId;  // Parsed 64-bit ID
    std::string mUrl; // Radio URL
    std::string mDisplayName;
};

bool ParseId(const std::string &aStr, uint64_t &aId)
{
    char *endptr;

    // Use strtoull with base 0 to support hex (with 0x prefix) and decimal
    aId = strtoull(aStr.c_str(), &endptr, 0);

    return (endptr != aStr.c_str() && *endptr == '\0');
}

uint64_t Eui64ToUint64(const otExtAddress &aEui64)
{
    uint64_t id = 0;

    for (uint8_t byte : aEui64.m8)
    {
        id = (id << 8) | byte;
    }

    return id;
}

std::string GetWiresharkConfigFilePath(const char *aFilename)
{
    std::string path;
    const char *xdgConfig = getenv("XDG_CONFIG_HOME");

    if (xdgConfig != nullptr && xdgConfig[0] != '\0')
    {
        path = std::string(xdgConfig) + "/wireshark";
    }
    else
    {
        const char *home = getenv("HOME");
        if (home != nullptr)
        {
#if __APPLE__
            path = std::string(home) + "/Library/Application Support/Wireshark";
#else
            path = std::string(home) + "/.config/wireshark";
#endif
        }
    }

    if (!path.empty() && aFilename != nullptr)
    {
        path += "/";
        path += aFilename;
    }

    return path;
}

struct RcpQuery
{
    std::string  mUrl;
    std::string  mPort; // Empty for custom URLs
    pid_t        mPid;
    int          mPipeFd;
    bool         mSuccess;
    otExtAddress mEui64;
};

void QueryRcpEui64Parallel(std::vector<RcpQuery> &aQueries, double aTimeoutSeconds)
{
    // 1. Spawn child processes for all queries in parallel
    for (auto &query : aQueries)
    {
        int pipeFds[2];
        if (pipe(pipeFds) < 0)
        {
            query.mPipeFd = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            close(pipeFds[0]);
            close(pipeFds[1]);
            query.mPipeFd = -1;
            continue;
        }

        if (pid == 0) // Child process
        {
            close(pipeFds[0]); // Close read end

            // Redirect stdout and stderr to /dev/null to keep child silent
            int devNull = open("/dev/null", O_WRONLY);
            if (devNull >= 0)
            {
                dup2(devNull, STDOUT_FILENO);
                dup2(devNull, STDERR_FILENO);
                close(devNull);
            }

            otPlatformConfig config;
            memset(&config, 0, sizeof(config));
            config.mCoprocessorUrls.mUrls[0] = query.mUrl.c_str();
            config.mCoprocessorUrls.mNum     = 1;
            config.mSpeedUpFactor            = 1;
            config.mDryRun                   = true;

            config.mCoprocessorType = otSysInitCoprocessor(&config.mCoprocessorUrls);
            if (config.mCoprocessorType == OT_COPROCESSOR_RCP)
            {
                otInstance *instance = otSysInit(&config);
                if (instance != nullptr)
                {
                    otExtAddress eui64;
                    otLinkGetFactoryAssignedIeeeEui64(instance, &eui64);
                    if (write(pipeFds[1], eui64.m8, sizeof(eui64.m8)) == sizeof(eui64.m8))
                    {
                        // Success
                    }
                }
            }
            close(pipeFds[1]);
            otSysDeinit();
            _exit(0);
        }

        // Parent process
        close(pipeFds[1]); // Close write end
        query.mPid     = pid;
        query.mPipeFd  = pipeFds[0];
        query.mSuccess = false;
        LogDebug("Spawned parallel child PID %d for URL: %s", pid, query.mUrl.c_str());
    }

    // 2. Monitor all pipes in parallel using select() multiplexing
    double         remainingTime = aTimeoutSeconds;
    struct timeval start, now;
    gettimeofday(&start, nullptr);

    while (remainingTime > 0)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        int maxFd       = -1;
        int activeCount = 0;

        for (const auto &query : aQueries)
        {
            if (query.mPipeFd >= 0 && !query.mSuccess)
            {
                FD_SET(query.mPipeFd, &readSet);
                if (query.mPipeFd > maxFd)
                {
                    maxFd = query.mPipeFd;
                }
                activeCount++;
            }
        }

        if (activeCount == 0)
        {
            break; // All active queries completed successfully
        }

        struct timeval timeout;
        timeout.tv_sec  = static_cast<time_t>(remainingTime);
        timeout.tv_usec = static_cast<suseconds_t>((remainingTime - timeout.tv_sec) * 1000000);

        int selectRval = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);

        if (selectRval > 0)
        {
            for (auto &query : aQueries)
            {
                if (query.mPipeFd >= 0 && !query.mSuccess && FD_ISSET(query.mPipeFd, &readSet))
                {
                    ssize_t bytesRead = read(query.mPipeFd, query.mEui64.m8, sizeof(query.mEui64.m8));
                    if (bytesRead == sizeof(query.mEui64.m8))
                    {
                        query.mSuccess = true;
                        LogDebug("Parallel query succeeded for %s. EUI-64: %02x%02x%02x%02x%02x%02x%02x%02x",
                                 query.mUrl.c_str(), query.mEui64.m8[0], query.mEui64.m8[1], query.mEui64.m8[2],
                                 query.mEui64.m8[3], query.mEui64.m8[4], query.mEui64.m8[5], query.mEui64.m8[6],
                                 query.mEui64.m8[7]);
                    }
                    else
                    {
                        LogDebug("Parallel query failed for %s: read %d bytes", query.mUrl.c_str(),
                                 static_cast<int>(bytesRead));
                    }
                    close(query.mPipeFd);
                    query.mPipeFd = -1;
                }
            }
        }
        else if (selectRval == 0)
        {
            break; // Global timeout expired
        }
        else
        {
            if (errno != EINTR)
            {
                LogDebug("select() failed in parallel query: %s", strerror(errno));
                break;
            }
        }

        // Calculate remaining timeout
        gettimeofday(&now, nullptr);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000000.0;
        remainingTime  = aTimeoutSeconds - elapsed;
    }

    // 3. Clean up, kill, and reap any remaining active queries (timed out)
    for (auto &query : aQueries)
    {
        if (query.mPipeFd >= 0) // This means the pipe is still open, so it genuinely timed out!
        {
            LogDebug("Query timed out for %s. Killing child PID %d", query.mUrl.c_str(), query.mPid);
            kill(query.mPid, SIGKILL);
            close(query.mPipeFd);
            query.mPipeFd = -1;
        }

        if (query.mPid >= 0)
        {
            int status;
            waitpid(query.mPid, &status, 0); // Reap child resources
            query.mPid = -1;
        }
    }
}

bool QueryRcpEui64(const std::string &aUrl, otExtAddress &aEui64)
{
    bool                  rval = false;
    std::vector<RcpQuery> queries;
    RcpQuery              query;

    query.mUrl     = aUrl;
    query.mPid     = -1;
    query.mPipeFd  = -1;
    query.mSuccess = false;

    queries.push_back(query);

    QueryRcpEui64Parallel(queries, 1.0);

    VerifyOrExit(queries[0].mSuccess);

    memcpy(aEui64.m8, queries[0].mEui64.m8, sizeof(aEui64.m8));
    rval = true;

exit:
    return rval;
}

std::vector<RadioUrlConfig> ReadRadioUrlConfig(void)
{
    std::vector<RadioUrlConfig> configs;
    std::string                 configPath = GetWiresharkConfigFilePath("openthread_radio_urls");
    std::ifstream               configFile(configPath);

    LogDebug("Reading config file: %s", configPath.c_str());

    if (!configFile.is_open())
    {
        LogDebug("Config file not found or could not be opened");
        return configs;
    }

    std::string line;
    while (std::getline(configFile, line))
    {
        // Strip comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty())
        {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
        {
            LogDebug("Invalid config line (missing '='): %s", line.c_str());
            continue;
        }

        std::string key = line.substr(0, eqPos);
        std::string url = line.substr(eqPos + 1);

        // Trim key and url
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        url.erase(0, url.find_first_not_of(" \t\r\n"));
        url.erase(url.find_last_not_of(" \t\r\n") + 1);

        uint64_t id;
        if (!ParseId(key, id))
        {
            LogDebug("Invalid nodeid (not a number): %s", key.c_str());
            continue;
        }

        RadioUrlConfig cfg;
        cfg.mId  = id;
        cfg.mUrl = url;

        LogDebug("Parsed config entry: id=0x%016" PRIx64 ", url='%s'", cfg.mId, cfg.mUrl.c_str());
        configs.push_back(cfg);
    }
    return configs;
}

bool UpdateConfigEntry(uint64_t aId, const std::string &aUrl)
{
    bool                     isNew = true;
    std::string              configPath;
    std::string              configDir;
    std::ifstream            configFile;
    std::vector<std::string> outputLines;
    std::string              line;
    bool                     processed = false;
    char                     keyHex[20];
    std::ofstream            configFileOut;

    snprintf(keyHex, sizeof(keyHex), "0x%016" PRIx64, aId);

    configPath = GetWiresharkConfigFilePath("openthread_radio_urls");
    VerifyOrExit(!configPath.empty(), LogDebug("Could not resolve Wireshark config directory path for saving"));

    configDir = configPath.substr(0, configPath.find_last_of("/\\"));
    configFile.open(configPath);

    if (configFile.is_open())
    {
        while (std::getline(configFile, line))
        {
            std::string rawLine    = line;
            size_t      commentPos = line.find('#');

            if (commentPos != std::string::npos)
            {
                line = line.substr(0, commentPos);
            }

            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty())
            {
                outputLines.push_back(rawLine);
                continue;
            }

            size_t eqPos = line.find('=');

            if (eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);

                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);

                uint64_t id;

                if (ParseId(key, id) && id == aId)
                {
                    // Match found! Update URL, preserving comment
                    std::string commentPart = (commentPos != std::string::npos) ? rawLine.substr(commentPos) : "";

                    rawLine = std::string(keyHex) + " = " + aUrl;

                    if (!commentPart.empty())
                    {
                        rawLine += " " + commentPart;
                    }

                    processed = true;
                    isNew     = false;
                    LogDebug("Updating config entry for 0x%016" PRIx64 ": %s", aId, aUrl.c_str());
                }
            }

            outputLines.push_back(rawLine);
        }

        configFile.close();
    }

    if (!processed)
    {
        // Append new entry
        std::string newLine = std::string(keyHex) + " = " + aUrl;

        outputLines.push_back(newLine);
        LogDebug("Appending new config entry 0x%016" PRIx64 ": %s", aId, aUrl.c_str());
    }

    // Write back
    // Recursively create the configuration directory programmatically to avoid unsafe shell execution
    {
        size_t pos = 0;
        while ((pos = configDir.find('/', pos + 1)) != std::string::npos)
        {
            std::string subdir = configDir.substr(0, pos);
            if (!subdir.empty())
            {
                mkdir(subdir.c_str(), 0755);
            }
        }
        mkdir(configDir.c_str(), 0755);
    }

    configFileOut.open(configPath);

    if (configFileOut.is_open())
    {
        for (const auto &outLine : outputLines)
        {
            configFileOut << outLine << "\n";
        }

        configFileOut.close();
    }
    else
    {
        LogDebug("Failed to open config file for writing: %s", configPath.c_str());
    }

exit:
    return isNew;
}

void SignalHandler(int aSignal)
{
    if (aSignal == SIGINT || aSignal == SIGTERM)
    {
        sShouldQuit = true;
    }
}

void HandlePcapCallback(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    uint64_t timeUs;

    OT_UNUSED_VARIABLE(aIsTx);
    OT_UNUSED_VARIABLE(aContext);

    if (aFrame != nullptr && sPcapngWriter.IsOpen())
    {
        timeUs = otPlatTimeGet();

        if (sPcapngWriter.WriteFrame(*aFrame, timeUs) != OT_ERROR_NONE)
        {
            // If the write failed due to EPIPE (broken pipe), it means Wireshark
            // has closed the pipe to stop the capture. We should handle this silently.
            if (errno != EPIPE)
            {
                syslog(LOG_ERR, "Failed to write frame to PCAPNG: %s", strerror(errno));
            }

            sShouldQuit = true;
        }
    }
}

void ListInterfaces(void)
{
    printf("extcap {version=1.0.0}{display=OpenThread Sniffer}{help=https://openthread.io}\n");

    // Read custom configs and list them instantly!
    std::vector<RadioUrlConfig> customConfigs = ReadRadioUrlConfig();
    for (const auto &cfg : customConfigs)
    {
        char idHex[20];
        snprintf(idHex, sizeof(idHex), "0x%016" PRIx64, cfg.mId);
        printf("interface {value=openthread_%s}{display=OpenThread Sniffer %s}\n", idHex, idHex);
    }

    printf("interface {value=spinel_hdlc_uart}{display=OpenThread Sniffer over UART}\n");
    printf("interface {value=openthread_radio_url}{display=OpenThread Sniffer over Radio URL...}\n");
}

void DetectInterfaces(void)
{
    glob_t                globResult;
    std::vector<RcpQuery> queries;
    int                   newCount     = 0;
    int                   updatedCount = 0;
    std::string           configPath;

    LogDebug("Starting manual interface detection");

    // 1. Scan physical ports
    memset(&globResult, 0, sizeof(globResult));
    glob("/dev/ttyACM*", 0, nullptr, &globResult);
    glob("/dev/ttyUSB*", GLOB_APPEND, nullptr, &globResult);

    for (size_t i = 0; i < globResult.gl_pathc; ++i)
    {
        const char *port = globResult.gl_pathv[i];
        RcpQuery    query;

        query.mPort    = port;
        query.mUrl     = "spinel+hdlc+uart://" + std::string(port) + "?uart-baudrate=460800";
        query.mPid     = -1;
        query.mPipeFd  = -1;
        query.mSuccess = false;
        queries.push_back(query);
    }

    globfree(&globResult);

    VerifyOrExit(!queries.empty(), {
        LogDebug("No local USB serial ports found to scan");
        printf("No new devices detected (no serial ports found).\n");
    });

    LogDebug("Running parallel detection queries on %d ports", static_cast<int>(queries.size()));
    QueryRcpEui64Parallel(queries, 1.0);

    // 2. Persist all successfully detected devices using the shared UpdateConfigEntry helper
    for (const auto &query : queries)
    {
        if (query.mSuccess)
        {
            uint64_t devId = Eui64ToUint64(query.mEui64);

            if (UpdateConfigEntry(devId, query.mUrl))
            {
                newCount++;
            }
            else
            {
                updatedCount++;
            }
        }
    }

    configPath = GetWiresharkConfigFilePath("openthread_radio_urls");
    printf("Detection completed. Updated config file: %s\n", configPath.c_str());
    printf("Successfully detected and updated %d devices (%d new, %d updated).\n", newCount + updatedCount, newCount,
           updatedCount);

exit:
    return;
}

void ListConfig(const char *aInterface)
{
    printf("arg {number=0}{call=--channel}{display=Channel}{tooltip=IEEE 802.15.4 "
           "channel}{type=selector}{required=true}{default=11}\n");
    for (int i = 11; i <= 26; ++i)
    {
        printf("value {arg=0}{value=%d}{display=%d}{default=%s}\n", i, i, (i == 11 ? "true" : "false"));
    }

    // Expose a checkbox for verbose debugging in the Wireshark GUI
    printf("arg {number=1}{call=--debug}{display=Verbose Debugging}{tooltip=Enable verbose debug "
           "logging}{type=boolflag}{default=false}\n");

    // If the special custom UART interface is selected, dynamically expose the Radio URL selector input
    // If the special custom generic interface is selected, dynamically expose the free-form text input
    if (aInterface != nullptr)
    {
        if (strcmp(aInterface, "spinel_hdlc_uart") == 0)
        {
            printf("arg {number=2}{call=--spinel-hdlc-uart}{display=Radio URL}{tooltip=Select an existing Radio URL or "
                   "click reload to scan for new USB devices}{type=selector}{reload=true}{placeholder=Select or "
                   "reload...}{required=true}\n");

            std::vector<RadioUrlConfig> customConfigs;

            customConfigs = ReadRadioUrlConfig();

            for (const auto &cfg : customConfigs)
            {
                char idHex[20];
                snprintf(idHex, sizeof(idHex), "0x%016" PRIx64, cfg.mId);
                printf("value {arg=2}{value=%s}{display=OpenThread Sniffer %s}\n", cfg.mUrl.c_str(), idHex);
            }

            if (customConfigs.empty())
            {
                printf("value {arg=2}{value=}{display=Click reload button to scan for devices...}{default=true}\n");
            }
        }
        else if (strcmp(aInterface, "openthread_radio_url") == 0)
        {
            printf("arg {number=2}{call=--radio-url}{display=Radio URL}{tooltip=Enter full Radio URL, e.g. "
                   "spinel+hdlc+fork://[path_to_rcp] or spinel+hdlc+uart://[port]}{type=string}{required=true}\n");
        }
    }
}

void ReloadRadioUrls(void)
{
    std::vector<RadioUrlConfig> customConfigs;
    glob_t                      globResult;
    std::vector<RcpQuery>       queries;

    LogDebug("Handling reload for radio-url");

    // 1. Print already configured URLs from config file first
    customConfigs = ReadRadioUrlConfig();

    for (const auto &cfg : customConfigs)
    {
        char idHex[20];
        snprintf(idHex, sizeof(idHex), "0x%016" PRIx64, cfg.mId);
        printf("value {arg=2}{value=%s}{display=OpenThread Sniffer %s}\n", cfg.mUrl.c_str(), idHex);
    }

    // 2. Scan and detect physical UART devices in parallel
    memset(&globResult, 0, sizeof(globResult));
    glob("/dev/ttyACM*", 0, nullptr, &globResult);
    glob("/dev/ttyUSB*", GLOB_APPEND, nullptr, &globResult);

    for (size_t i = 0; i < globResult.gl_pathc; ++i)
    {
        const char *port = globResult.gl_pathv[i];
        RcpQuery    query;

        query.mPort    = port;
        query.mUrl     = "spinel+hdlc+uart://" + std::string(port) + "?uart-baudrate=460800";
        query.mPid     = -1;
        query.mPipeFd  = -1;
        query.mSuccess = false;
        queries.push_back(query);
    }

    globfree(&globResult);

    if (!queries.empty())
    {
        LogDebug("Running parallel detection queries on %d ports for reload", static_cast<int>(queries.size()));
        QueryRcpEui64Parallel(queries, 1.0);

        for (const auto &query : queries)
        {
            if (query.mSuccess)
            {
                uint64_t devId = Eui64ToUint64(query.mEui64);

                // Avoid duplicate options if the device is already in the config file
                bool alreadyConfigured = false;

                for (const auto &cfg : customConfigs)
                {
                    if (cfg.mId == devId)
                    {
                        alreadyConfigured = true;
                        break;
                    }
                }

                if (!alreadyConfigured)
                {
                    char euiStr[17];
                    char displayName[128];

                    snprintf(euiStr, sizeof(euiStr), "%02x%02x%02x%02x%02x%02x%02x%02x", query.mEui64.m8[0],
                             query.mEui64.m8[1], query.mEui64.m8[2], query.mEui64.m8[3], query.mEui64.m8[4],
                             query.mEui64.m8[5], query.mEui64.m8[6], query.mEui64.m8[7]);

                    snprintf(displayName, sizeof(displayName), "OpenThread Sniffer 0x%s (%s)", euiStr,
                             query.mPort.c_str());

                    printf("value {arg=2}{value=%s}{display=%s}\n", query.mUrl.c_str(), displayName);
                }
            }
        }
    }
}

void ListDlts(void) { printf("dlt {number=283}{name=IEEE802_15_4_TAP}{display=IEEE 802.15.4 TAP}\n"); }

} // namespace

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    syslog(LOG_WARNING, "OpenThread instance requested reset, exiting");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int              rval     = EXIT_SUCCESS;
    otInstance      *instance = nullptr;
    otPlatformConfig config;
    std::string      radioUrl;
    std::string      interfaceVal;
    uint64_t         targetId     = 0;
    bool             hasNumericId = false;

    InitLogging();

    static struct option longOptions[] = {
        {"extcap-interfaces", no_argument, nullptr, 'i'},
        {"extcap-config", no_argument, nullptr, 'c'},
        {"extcap-dlts", no_argument, nullptr, 'D'}, // Remapped short option to 'D'
        {"extcap-interface", required_argument, nullptr, 'g'},
        {"capture", no_argument, nullptr, 's'},
        {"fifo", required_argument, nullptr, 'f'},
        {"channel", required_argument, nullptr, 'h'},
        {"spinel-hdlc-uart", required_argument, nullptr, 'u'}, // Add spinel-hdlc-uart (short 'u') for custom interface
        {"radio-url", required_argument, nullptr, 'U'},        // Add radio-url (short 'U') for generic custom URL
        {"extcap-reload-option", required_argument, nullptr, 'r'}, // Add extcap-reload-option (short 'r')
        {"extcap-version", required_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'},             // Map debug to short option 'd'
        {"detect-interfaces", no_argument, nullptr, 'x'}, // Add detect-interfaces (short 'x')
        {nullptr, 0, nullptr, 0}};

    bool        optInterfaces     = false;
    bool        optConfig         = false;
    bool        optDlts           = false;
    bool        optCapture        = false;
    bool        optDetect         = false;
    const char *optInterface      = nullptr;
    const char *optFifo           = nullptr;
    const char *optSpinelHdlcUart = nullptr;
    const char *optRadioUrl       = nullptr;
    const char *optReloadOption   = nullptr;
    uint8_t     optChannel        = 11;

    int c;
    // Added 'x', 'u', 'U' and 'r' to short options string
    while ((c = getopt_long(argc, argv, "icDg:sf:h:u:U:r:v:dx", longOptions, nullptr)) != -1)
    {
        switch (c)
        {
        case 'i':
            optInterfaces = true;
            break;
        case 'c':
            optConfig = true;
            break;
        case 'D': // Remapped from 'd'
            optDlts = true;
            break;
        case 'g':
            optInterface = optarg;
            break;
        case 's':
            optCapture = true;
            break;
        case 'f':
            optFifo = optarg;
            break;
        case 'h':
            optChannel = static_cast<uint8_t>(atoi(optarg));
            break;
        case 'u':
            optSpinelHdlcUart = optarg;
            break;
        case 'U':
            optRadioUrl = optarg;
            break;
        case 'r':
            optReloadOption = optarg;
            break;
        case 'v':
            fprintf(stderr, "Wireshark version: %s\n", optarg);
            break;
        case 'd': // Debug enabled
            sDebugEnabled = true;
            break;
        case 'x': // Detect interfaces
            optDetect = true;
            break;
        default:
            fprintf(stderr, "Invalid option\n");
            exit(EXIT_FAILURE);
        }
    }

    // Configure logging based on debug flag
    ConfigureLogging(sDebugEnabled);

    LogDebug("Command line options parsed. Interfaces=%d, Detect=%d, Config=%d, Dlts=%d, Capture=%d, Interface='%s', "
             "Fifo='%s', Channel=%d, SpinelHdlcUart='%s', RadioUrl='%s', ReloadOption='%s', Debug=%d",
             optInterfaces, optDetect, optConfig, optDlts, optCapture, optInterface ? optInterface : "",
             optFifo ? optFifo : "", optChannel, optSpinelHdlcUart ? optSpinelHdlcUart : "",
             optRadioUrl ? optRadioUrl : "", optReloadOption ? optReloadOption : "", sDebugEnabled);

    if (optInterfaces)
    {
        ListInterfaces();
        rval = EXIT_SUCCESS;
        ExitNow();
    }

    if (optDetect)
    {
        DetectInterfaces();
        rval = EXIT_SUCCESS;
        ExitNow();
    }

    // Intercept and handle dynamic option reload BEFORE normal configuration is printed
    if (optReloadOption != nullptr)
    {
        if (strcmp(optReloadOption, "spinel-hdlc-uart") == 0)
        {
            ReloadRadioUrls();
        }
        rval = EXIT_SUCCESS;
        ExitNow();
    }

    VerifyOrExit(optInterface != nullptr, {
        fprintf(stderr, "An interface must be provided\n");
        rval = EXIT_FAILURE;
    });

    if (optConfig)
    {
        LogDebug("Listing config options for interface: %s", optInterface ? optInterface : "none");
        ListConfig(optInterface);
        rval = EXIT_SUCCESS;
        ExitNow();
    }

    if (optDlts)
    {
        LogDebug("Listing DLTs");
        ListDlts();
        rval = EXIT_SUCCESS;
        ExitNow();
    }

    if (optCapture)
    {
        LogDebug("Starting capture phase");
        VerifyOrExit(optFifo != nullptr, {
            fprintf(stderr, "The fifo must be provided to capture\n");
            rval = EXIT_FAILURE;
        });

        // Setup Signal Handlers
        struct sigaction sa;
        sa.sa_handler = SignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);

        struct sigaction sa_pipe;
        sa_pipe.sa_handler = SIG_IGN;
        sigemptyset(&sa_pipe.sa_mask);
        sa_pipe.sa_flags = 0;
        sigaction(SIGPIPE, &sa_pipe, nullptr);

        // 1. Open PCAPNG Writer (blocks until Wireshark opens the FIFO for reading)
        LogDebug("Opening FIFO: %s (blocking until Wireshark opens it)", optFifo);
        VerifyOrExit(sPcapngWriter.Open(optFifo) == OT_ERROR_NONE, {
            fprintf(stderr, "Failed to open FIFO for writing\n");
            rval = EXIT_FAILURE;
        });
        LogDebug("FIFO opened successfully");

        // 2. Resolve the interface value to a full Radio URL
        interfaceVal = optInterface;

        // Strip "spinel_" prefix if present
        if (interfaceVal.compare(0, 7, "spinel_") == 0)
        {
            interfaceVal = interfaceVal.substr(7);
            LogDebug("Stripped 'spinel_' prefix. Resolving key: %s", interfaceVal.c_str());
        }

        radioUrl = "";

        targetId     = 0;
        hasNumericId = ParseId(interfaceVal, targetId);

        if (interfaceVal == "hdlc_uart")
        {
            otExtAddress eui64;
            uint64_t     devId = 0;

            VerifyOrExit(optSpinelHdlcUart != nullptr && optSpinelHdlcUart[0] != '\0', {
                fprintf(stderr, "Error: Spinel HDLC UART URL must be provided for custom interface\n");
                rval = EXIT_FAILURE;
            });

            LogDebug("Detecting and validating custom URL: %s", optSpinelHdlcUart);
            VerifyOrExit(QueryRcpEui64(optSpinelHdlcUart, eui64), {
                fprintf(stderr, "Error: Failed to detect Thread RCP at URL: %s\n", optSpinelHdlcUart);
                rval = EXIT_FAILURE;
            });

            devId = Eui64ToUint64(eui64);

            LogDebug("Custom URL validated. EUI-64: %02x%02x%02x%02x%02x%02x%02x%02x.", eui64.m8[0], eui64.m8[1],
                     eui64.m8[2], eui64.m8[3], eui64.m8[4], eui64.m8[5], eui64.m8[6], eui64.m8[7]);

            // Persist to config dynamically!
            UpdateConfigEntry(devId, optSpinelHdlcUart);

            radioUrl = optSpinelHdlcUart;
        }
        else if (interfaceVal == "openthread_radio_url")
        {
            otExtAddress eui64;
            uint64_t     devId = 0;

            VerifyOrExit(optRadioUrl != nullptr && optRadioUrl[0] != '\0', {
                fprintf(stderr, "Error: Radio URL must be provided for custom interface\n");
                rval = EXIT_FAILURE;
            });

            LogDebug("Detecting and validating custom Radio URL: %s", optRadioUrl);
            VerifyOrExit(QueryRcpEui64(optRadioUrl, eui64), {
                fprintf(stderr, "Error: Failed to detect Thread RCP at URL: %s\n", optRadioUrl);
                rval = EXIT_FAILURE;
            });

            devId = Eui64ToUint64(eui64);

            LogDebug("Custom Radio URL validated. EUI-64: %02x%02x%02x%02x%02x%02x%02x%02x.", eui64.m8[0], eui64.m8[1],
                     eui64.m8[2], eui64.m8[3], eui64.m8[4], eui64.m8[5], eui64.m8[6], eui64.m8[7]);

            // Persist to config dynamically!
            UpdateConfigEntry(devId, optRadioUrl);

            radioUrl = optRadioUrl;
        }
        else
        {
            // Check if it matches a key (by numeric ID) in the custom configs
            std::vector<RadioUrlConfig> customConfigs = ReadRadioUrlConfig();
            for (const auto &cfg : customConfigs)
            {
                if (hasNumericId && cfg.mId == targetId)
                {
                    radioUrl = cfg.mUrl;
                    LogDebug("Matched config entry by ID 0x%016" PRIx64 " to URL: %s", targetId, radioUrl.c_str());
                    break;
                }
            }
        }

        // If not found in config, check if the interface value can be parsed as a 64-bit ID (like an EUI-64)
        if (radioUrl.empty() && ParseId(interfaceVal, targetId))
        {
            glob_t                globResult;
            std::vector<uint32_t> baudrates;
            bool                  found = false;

            LogDebug("Interface value parsed as 64-bit ID: 0x%016" PRIx64 ", scanning serial ports to resolve EUI-64",
                     targetId);

            memset(&globResult, 0, sizeof(globResult));
            glob("/dev/ttyACM*", 0, nullptr, &globResult);
            glob("/dev/ttyUSB*", GLOB_APPEND, nullptr, &globResult);

            LogDebug("Scanning %d serial ports to resolve EUI-64", static_cast<int>(globResult.gl_pathc));

            baudrates = {460800, 115200};

            for (size_t i = 0; i < globResult.gl_pathc; ++i)
            {
                const char *port = globResult.gl_pathv[i];
                for (uint32_t baudrate : baudrates)
                {
                    std::string url =
                        "spinel+hdlc+uart://" + std::string(port) + "?uart-baudrate=" + std::to_string(baudrate);
                    otExtAddress eui64;
                    LogDebug("Trying port %s at baudrate %d", port, baudrate);
                    if (QueryRcpEui64(url, eui64))
                    {
                        uint64_t devId = Eui64ToUint64(eui64);
                        if (devId == targetId)
                        {
                            radioUrl = url;
                            LogDebug("SUCCESS: Resolved EUI-64 0x%016" PRIx64 " to port %s (URL: %s)", targetId, port,
                                     radioUrl.c_str());
                            found = true;
                            break;
                        }
                    }
                }
                if (found)
                {
                    break;
                }
            }
            globfree(&globResult);
        }

        if (radioUrl.empty())
        {
            // If still not resolved, treat the interface value itself as the Radio URL
            radioUrl = optInterface;
            LogDebug("Could not resolve interface key/EUI-64, falling back to raw interface value as URL: %s",
                     radioUrl.c_str());
        }

        // Auto-inject baudrate parameter for serial transport if missing (default 460800)
        if (radioUrl.find("spinel+hdlc+uart") != std::string::npos)
        {
            if (radioUrl.find("uart-baudrate") == std::string::npos)
            {
                char separator = (radioUrl.find('?') == std::string::npos) ? '?' : '&';
                radioUrl += separator;
                radioUrl += "uart-baudrate=460800";
                LogDebug("Injected default baudrate 460800 into URL: %s", radioUrl.c_str());
            }
        }

        // 3. Initialize OpenThread POSIX Platform
        LogDebug("Initializing OpenThread POSIX platform with URL: %s", radioUrl.c_str());
        memset(&config, 0, sizeof(config));
        config.mCoprocessorUrls.mUrls[0] = radioUrl.c_str();
        config.mCoprocessorUrls.mNum     = 1;
        config.mSpeedUpFactor            = 1;

        config.mCoprocessorType = otSysInitCoprocessor(&config.mCoprocessorUrls);
        LogDebug("Coprocessor type: %d", config.mCoprocessorType);
        VerifyOrExit(config.mCoprocessorType == OT_COPROCESSOR_RCP, {
            fprintf(stderr, "Coprocessor must be RCP\n");
            rval = EXIT_FAILURE;
        });

        instance = otSysInit(&config);
        VerifyOrExit(instance != nullptr, {
            fprintf(stderr, "Failed to initialize OpenThread instance\n");
            rval = EXIT_FAILURE;
        });
        LogDebug("OpenThread instance initialized successfully");

        // 4. Configure Radio using Standard Link API
        // Register PCAP callback to capture all MAC frames (RX and TX)
        otLinkSetPcapCallback(instance, HandlePcapCallback, nullptr);

        // Enable Link layer (enables MAC and Radio, but keeps interface down)
        rval = otLinkSetEnabled(instance, true);
        VerifyOrExit(rval == OT_ERROR_NONE, {
            fprintf(stderr, "Failed to enable Link layer: %s\n", otThreadErrorToString(static_cast<otError>(rval)));
            rval = EXIT_FAILURE;
        });
        LogDebug("Link layer enabled");

        // Set Channel
        rval = otLinkSetChannel(instance, optChannel);
        VerifyOrExit(rval == OT_ERROR_NONE, {
            fprintf(stderr, "Failed to set channel to %d: %s\n", optChannel,
                    otThreadErrorToString(static_cast<otError>(rval)));
            rval = EXIT_FAILURE;
        });
        LogDebug("Channel set to %d", optChannel);

        // Enable Promiscuous mode to receive all packets
        rval = otLinkSetPromiscuous(instance, true);
        VerifyOrExit(rval == OT_ERROR_NONE, {
            fprintf(stderr, "Failed to enable promiscuous mode: %s\n",
                    otThreadErrorToString(static_cast<otError>(rval)));
            rval = EXIT_FAILURE;
        });
        LogDebug("Promiscuous mode enabled. Starting capture loop...");

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

            otSysMainloopUpdate(instance, &mainloopContext);

            int selectRval = otSysMainloopPoll(&mainloopContext);

            if (selectRval >= 0)
            {
                otSysMainloopProcess(instance, &mainloopContext);
            }
            else if (errno != EINTR)
            {
                fprintf(stderr, "select() failed: %s\n", strerror(errno));
                LogDebug("select() failed in main loop: %s", strerror(errno));
                break;
            }
        }

        LogDebug("Exited main loop. shouldQuit=%d", sShouldQuit);

        // Cleanup
        IgnoreReturnValue(otLinkSetPromiscuous(instance, false));
        IgnoreReturnValue(otLinkSetEnabled(instance, false));
        otLinkSetPcapCallback(instance, nullptr, nullptr);
    }

exit:
    LogDebug("Performing final cleanups");
    sPcapngWriter.Close();
    if (instance != nullptr)
    {
        otInstanceFinalize(instance);
        otSysDeinit();
        LogDebug("OpenThread instance finalized");
    }

    if (sDebugEnabled)
    {
        LogDebug("--- ot-extcap terminated (exit code: %d) ---", rval);
        closelog();
    }
    return rval;
}
