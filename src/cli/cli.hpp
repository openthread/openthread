/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file contains definitions for the CLI interpreter.
 */

#ifndef CLI_HPP_
#define CLI_HPP_

#include "openthread-core-config.h"

#include "cli_config.h"

#include <stdarg.h>

#include <openthread/border_agent.h>
#include <openthread/cli.h>
#include <openthread/dataset.h>
#include <openthread/dns_client.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/logging.h>
#include <openthread/mesh_diag.h>
#include <openthread/netdata.h>
#include <openthread/ping_sender.h>
#include <openthread/sntp.h>
#include <openthread/tcp.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/udp.h>

#include "cli/cli_bbr.hpp"
#include "cli/cli_br.hpp"
#include "cli/cli_coap.hpp"
#include "cli/cli_coap_secure.hpp"
#include "cli/cli_commissioner.hpp"
#include "cli/cli_config.h"
#include "cli/cli_dataset.hpp"
#include "cli/cli_dns.hpp"
#include "cli/cli_history.hpp"
#include "cli/cli_joiner.hpp"
#include "cli/cli_link_metrics.hpp"
#include "cli/cli_mac_filter.hpp"
#include "cli/cli_mdns.hpp"
#include "cli/cli_network_data.hpp"
#include "cli/cli_ping.hpp"
#include "cli/cli_srp_client.hpp"
#include "cli/cli_srp_server.hpp"
#include "cli/cli_tcat.hpp"
#include "cli/cli_tcp.hpp"
#include "cli/cli_udp.hpp"
#include "cli/cli_utils.hpp"

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/type_traits.hpp"
#include "instance/instance.hpp"

namespace ot {

/**
 * @namespace ot::Cli
 *
 * @brief
 *   This namespace contains definitions for the CLI interpreter.
 */
namespace Cli {

extern "C" void otCliPlatLogv(otLogLevel, otLogRegion, const char *, va_list);
extern "C" void otCliAppendResult(otError aError);
extern "C" void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength);
extern "C" void otCliOutputFormat(const char *aFmt, ...);

/**
 * Implements the CLI interpreter.
 */
class Interpreter : public OutputImplementer, public Utils
{
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    friend class Br;
    friend class Bbr;
    friend class Commissioner;
    friend class Dns;
    friend class Joiner;
    friend class LinkMetrics;
    friend class Mdns;
    friend class NetworkData;
    friend class PingSender;
    friend class SrpClient;
    friend class SrpServer;
#endif
    friend void otCliPlatLogv(otLogLevel, otLogRegion, const char *, va_list);
    friend void otCliAppendResult(otError aError);
    friend void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength);
    friend void otCliOutputFormat(const char *aFmt, ...);

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance    The OpenThread instance structure.
     * @param[in]  aCallback    A callback method called to process CLI output.
     * @param[in]  aContext     A user context pointer.
     */
    explicit Interpreter(Instance *aInstance, otCliOutputCallback aCallback, void *aContext);

    /**
     * Returns a reference to the interpreter object.
     *
     * @returns A reference to the interpreter object.
     */
    static Interpreter &GetInterpreter(void)
    {
        OT_ASSERT(sInterpreter != nullptr);

        return *sInterpreter;
    }

    /**
     * Initializes the Console interpreter.
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     * @param[in]  aCallback  A pointer to a callback method.
     * @param[in]  aContext   A pointer to a user context.
     */
    static void Initialize(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext);

    /**
     * Returns whether the interpreter is initialized.
     *
     * @returns  Whether the interpreter is initialized.
     */
    static bool IsInitialized(void) { return sInterpreter != nullptr; }

    /**
     * Interprets a CLI command.
     *
     * @param[in]  aBuf        A pointer to a string.
     */
    void ProcessLine(char *aBuf);

    /**
     * Adds commands to the user command table.
     *
     * @param[in]  aCommands  A pointer to an array with user commands.
     * @param[in]  aLength    @p aUserCommands length.
     * @param[in]  aContext   @p aUserCommands length.
     *
     * @retval OT_ERROR_NONE    Successfully updated command table with commands from @p aCommands.
     * @retval OT_ERROR_FAILED  No available UserCommandsEntry to register requested user commands.
     */
    otError SetUserCommands(const otCliCommand *aCommands, uint8_t aLength, void *aContext);

protected:
    static Interpreter *sInterpreter;

private:
    static constexpr uint8_t  kIndentSize            = 4;
    static constexpr uint16_t kMaxArgs               = 32;
    static constexpr uint16_t kMaxLineLength         = OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH;
    static constexpr uint16_t kMaxUserCommandEntries = OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES;

    static constexpr uint32_t kNetworkDiagnosticTimeoutMsecs = 5000;
    static constexpr uint32_t kLocateTimeoutMsecs            = 2500;

    static constexpr uint16_t kMaxTxtDataSize = OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE;

    using Command = CommandEntry<Interpreter>;

    void OutputPrompt(void);
    void OutputResult(otError aError);

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    void OutputBorderRouterCounters(void);
#endif

    otError ProcessCommand(Arg aArgs[]);

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ProcessUserCommands(Arg aArgs[]);

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    otError ProcessBackboneRouterLocal(Arg aArgs[]);
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    otError ProcessBackboneRouterMgmtMlr(Arg aArgs[]);
    void    PrintMulticastListenersTable(void);
#endif
#endif
#endif

#if OPENTHREAD_FTD
    void OutputEidCacheEntry(const otCacheEntryInfo &aEntry);
#endif
#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    static void HandleLocateResult(void               *aContext,
                                   otError             aError,
                                   const otIp6Address *aMeshLocalAddress,
                                   uint16_t            aRloc16);
    void        HandleLocateResult(otError aError, const otIp6Address *aMeshLocalAddress, uint16_t aRloc16);
#endif
#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
    static void HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext);
    void        HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo);
    static void HandleMeshDiagQueryChildTableResult(otError                     aError,
                                                    const otMeshDiagChildEntry *aChildEntry,
                                                    void                       *aContext);
    void        HandleMeshDiagQueryChildTableResult(otError aError, const otMeshDiagChildEntry *aChildEntry);
    static void HandleMeshDiagQueryChildIp6Addrs(otError                    aError,
                                                 uint16_t                   aChildRloc16,
                                                 otMeshDiagIp6AddrIterator *aIp6AddrIterator,
                                                 void                      *aContext);
    void        HandleMeshDiagQueryChildIp6Addrs(otError                    aError,
                                                 uint16_t                   aChildRloc16,
                                                 otMeshDiagIp6AddrIterator *aIp6AddrIterator);
    static void HandleMeshDiagQueryRouterNeighborTableResult(otError                              aError,
                                                             const otMeshDiagRouterNeighborEntry *aNeighborEntry,
                                                             void                                *aContext);
    void        HandleMeshDiagQueryRouterNeighborTableResult(otError                              aError,
                                                             const otMeshDiagRouterNeighborEntry *aNeighborEntry);

#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    static void HandleMlrRegResult(void               *aContext,
                                   otError             aError,
                                   uint8_t             aMlrStatus,
                                   const otIp6Address *aFailedAddresses,
                                   uint8_t             aFailedAddressNum);
    void        HandleMlrRegResult(otError             aError,
                                   uint8_t             aMlrStatus,
                                   const otIp6Address *aFailedAddresses,
                                   uint8_t             aFailedAddressNum);
#endif
#if OPENTHREAD_CONFIG_MULTI_RADIO
    void OutputMultiRadioInfo(const otMultiRadioNeighborInfo &aMultiRadioInfo);
#endif

    static void HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext);
    static void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext);

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
    void HandleDiagnosticGetResponse(otError aError, const otMessage *aMessage, const Ip6::MessageInfo *aMessageInfo);
    static void HandleDiagnosticGetResponse(otError              aError,
                                            otMessage           *aMessage,
                                            const otMessageInfo *aMessageInfo,
                                            void                *aContext);

    void OutputMode(uint8_t aIndentSize, const otLinkModeConfig &aMode);
    void OutputConnectivity(uint8_t aIndentSize, const otNetworkDiagConnectivity &aConnectivity);
    void OutputRoute(uint8_t aIndentSize, const otNetworkDiagRoute &aRoute);
    void OutputRouteData(uint8_t aIndentSize, const otNetworkDiagRouteData &aRouteData);
    void OutputLeaderData(uint8_t aIndentSize, const otLeaderData &aLeaderData);
    void OutputNetworkDiagMacCounters(uint8_t aIndentSize, const otNetworkDiagMacCounters &aMacCounters);
    void OutputNetworkDiagMleCounters(uint8_t aIndentSize, const otNetworkDiagMleCounters &aMleCounters);
    void OutputChildTableEntry(uint8_t aIndentSize, const otNetworkDiagChildEntry &aChildEntry);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void OutputTrelCounters(const otTrelCounters &aCounters);
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    void OutputNat64Counters(const otNat64Counters &aCounters);
#endif
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE
    void OutputRadioStatsTime(const char *aTimeName, uint64_t aTimeUs, uint64_t aTotalTime);
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    static void HandleSntpResponse(void *aContext, uint64_t aTime, otError aResult);
#endif

    void HandleActiveScanResult(otActiveScanResult *aResult);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);
    void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx);
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    void HandleSntpResponse(uint64_t aTime, otError aResult);
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    void OutputBorderAgentCounters(const otBorderAgentCounters &aCounters);
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    static void HandleBorderAgentEphemeralKeyStateChange(void *aContext);
    void        HandleBorderAgentEphemeralKeyStateChange(void);
#endif
#endif

    static void HandleDetachGracefullyResult(void *aContext);
    void        HandleDetachGracefullyResult(void);

#if OPENTHREAD_FTD
    static void HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo *aInfo, void *aContext);
    void        HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo &aInfo);
#endif

#if OPENTHREAD_CONFIG_CLI_REGISTER_IP6_RECV_CALLBACK
    static void HandleIp6Receive(otMessage *aMessage, void *aContext);
#endif

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    static void HandleDiagOutput(const char *aFormat, va_list aArguments, void *aContext);
    void        HandleDiagOutput(const char *aFormat, va_list aArguments);
#endif

    void SetCommandTimeout(uint32_t aTimeoutMilli);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    struct UserCommandsEntry
    {
        const otCliCommand *mCommands;
        uint8_t             mLength;
        void               *mContext;
    };

    UserCommandsEntry mUserCommands[kMaxUserCommandEntries];
    bool              mCommandIsPending;
    bool              mInternalDebugCommand;

    TimerMilliContext mTimer;

#if OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    bool mSntpQueryingInProgress;
#endif

    Dataset     mDataset;
    NetworkData mNetworkData;
    UdpExample  mUdp;

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    MacFilter mMacFilter;
#endif

#if OPENTHREAD_CLI_DNS_ENABLE
    Dns mDns;
#endif

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
    Mdns mMdns;
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    Bbr mBbr;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    Br mBr;
#endif

#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
    TcpExample mTcp;
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    Coap mCoap;
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    CoapSecure mCoapSecure;
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    Commissioner mCommissioner;
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
    Joiner mJoiner;
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    SrpClient mSrpClient;
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    SrpServer mSrpServer;
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    History mHistory;
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    LinkMetrics mLinkMetrics;
#endif
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE && OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
    Tcat mTcat;
#endif
#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
    PingSender mPing;
#endif
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    bool mLocateInProgress : 1;
#endif
};

} // namespace Cli
} // namespace ot

#endif // CLI_HPP_
