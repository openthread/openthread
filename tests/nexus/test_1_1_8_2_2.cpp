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

#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "meshcop/commissioner.hpp"
#include "meshcop/joiner.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * Time::kOneSecondInMsec;

/**
 * Time to advance for the Commissioner to start, in milliseconds.
 */
static constexpr uint32_t kCommissionerStartTime = 1 * Time::kOneSecondInMsec;

/**
 * Time to advance for a node to join and become router, in milliseconds.
 */
static constexpr uint32_t kJoinAndRouterTime = 200 * Time::kOneSecondInMsec;

/**
 * Time to advance for the Joiner to complete the joining process, in milliseconds.
 */
static constexpr uint32_t kJoiningProcessTime = 30 * Time::kOneSecondInMsec;

/**
 * The timeout for the Joiner entry on Commissioner, in seconds.
 */
static constexpr uint32_t kJoinerTimeout = 120;

/**
 * The PSKd used by Commissioner.
 */
static const char kCorrectPskd[] = "J01NME";

/**
 * The PSKd used by Joiner (incorrect).
 */
static const char kIncorrectPskd[] = "WR0NGPSK";

/**
 * The channel used for the test.
 */
static constexpr uint8_t kTestChannel = 11;

/**
 * The Joiner UDP port.
 */
static constexpr uint16_t kJoinerPort = 49153;

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
static void HandleKeylog(void                       *aContext,
                         mbedtls_ssl_key_export_type aType,
                         const unsigned char        *aMasterSecret,
                         size_t                      aMasterSecretLen,
                         const unsigned char         aClientRandom[32],
                         const unsigned char         aServerRandom[32],
                         mbedtls_tls_prf_types       aTlsPrfType)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aServerRandom);
    OT_UNUSED_VARIABLE(aTlsPrfType);

    if (aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET)
    {
        FILE *f = fopen("test_1_1_8_2_2.keys", "a");

        if (f != nullptr)
        {
            fprintf(f, "CLIENT_RANDOM ");
            for (size_t i = 0; i < 32; i++)
            {
                fprintf(f, "%02x", aClientRandom[i]);
            }
            fprintf(f, " ");
            for (size_t i = 0; i < aMasterSecretLen; i++)
            {
                fprintf(f, "%02x", aMasterSecret[i]);
            }
            fprintf(f, "\n");
            fclose(f);
        }
    }
}
#endif

static void HandleJoinerComplete(otError aError, void *aContext)
{
    Node *node = static_cast<Node *>(aContext);
    Log("   Joiner on node %u completed with error: %s", node->GetId(), otThreadErrorToString(aError));
}

void Test8_2_2(const char *aJsonFileName)
{
    /**
     * 8.2.2 On Mesh Commissioner Joining with JR, any commissioner, single (incorrect).
     *
     * 8.2.2.1 Topology
     * (Note: A topology diagram is implicitly referenced, showing Topology A with the DUT as the
     *   Commissioner/Leader, a Joiner Router, and Joiner_1, and Topology B with the Commissioner/Leader, the DUT as
     *   the Joiner Router, and Joiner_1.)
     *
     * 8.2.2.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT sends and receives relayed DTLS traffic correctly, when
     *   an incorrect PSKd is used by the Joiner.
     *
     * Spec Reference                          | V1.1 Section  | V1.3.0 Section
     * ----------------------------------------|---------------|---------------
     * Relay DTLS Traffic / Joiner Protocol    | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4
     */

    Core nexus;

    Node &commissionerNode = nexus.CreateNode();
    Node &joinerRouterNode = nexus.CreateNode();
    Node &joinerNode       = nexus.CreateNode();

    commissionerNode.SetName("Commissioner");
    joinerRouterNode.SetName("Joiner_Router");
    joinerNode.SetName("Joiner_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
    remove("test_1_1_8_2_2.keys");
#endif

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: All");

    /**
     * Step 0: All
     * - Description: Form topology excluding the Joiner and including the DUT. The DUT may be joined to the network
     *   using either Thread Commissioning or OOB (out-of-band) Commissioning.
     * - Pass Criteria: N/A.
     */

    commissionerNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    joinerRouterNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);

    commissionerNode.Form();
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(commissionerNode.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kTestChannel);
        commissionerNode.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }
    nexus.AdvanceTime(kFormNetworkTime);

    joinerRouterNode.Join(commissionerNode);
    nexus.AdvanceTime(kJoinAndRouterTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Joiner_1");

    /**
     * Step 1: Joiner_1
     * - Description: PSKd configured on Joiner_1 is different to PSKd configured on the Commissioner.
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Begin wireless sniffer and power on the Commissioner and Joiner Router.
     * - Pass Criteria: N/A.
     */

    MeshCoP::Commissioner &commissioner = commissionerNode.Get<MeshCoP::Commissioner>();

    SuccessOrQuit(commissioner.Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kCommissionerStartTime);

    commissionerNode.Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(kJoinerPort);
    joinerRouterNode.Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(kJoinerPort);

    SuccessOrQuit(commissioner.AddJoinerAny(kCorrectPskd, kJoinerTimeout));
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
    static_cast<MeshCoP::SecureTransport &>(commissionerNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleKeylog, &commissionerNode.Get<Tmf::SecureAgent>());
    static_cast<MeshCoP::SecureTransport &>(joinerNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleKeylog, &joinerNode.Get<Tmf::SecureAgent>());
#endif

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Joiner_1");

    /**
     * Step 3: Joiner_1
     * - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner.
     * - Pass Criteria: Verify that the following details occur in the exchange between the Joiner, Joiner_Router and
     *   the Commissioner:
     *   - UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
     *     from Joiner_1 to Commissioner.
     *   - Joiner_1 sends an initial DTLS-ClientHello handshake record to Joiner_Router.
     *   - The Joiner_Router receives the initial DTLS-ClientHello handshake record and sends a RLY_RX.ntf message to
     *     the Commissioner containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = initial
     *         DTLS-ClientHello handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x011), Length = Variable (8 bit size), Value =
     *         DTLS-HelloVerifyRequest handshake record destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-HelloVerifyRequest
     *     handshake record in one UDP datagram to Joiner_1.
     *   - Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
     *     handshake record in one UDP datagram to the Commissioner.
     *     - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
     *   - The Joiner_Router receives the subsequent DTLS-ClientHello handshake record and sends a RLY_RX.ntf message
     *     to the Commissioner containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = subsequent
     *         DTLS-ClientHello handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records, in that order,
     *         destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-ServerHello,
     *     DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records in one UDP datagram to Joiner_1.
     *   - Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
     *     and sends a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an encrypted
     *     DTLS-Finished handshake record, in that order, to Joiner_Router.
     *   - The Joiner_Router receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record
     *     and the encrypted DTLS-Finished handshake record; it then sends a RLY_RX.ntf message to the Commissioner
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         DTLS-ClientKeyExchange handshake record, DTLS-ChangeCipherSpec record and encrypted DTLS-Finished
     *         handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = DTLS-Alert
     *         record with error code 40 (handshake failure) or 20 (bad record MAC) destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-Alert record with error
     *     code 40 (handshake failure) or 20 (bad record MAC) handshake record in one UDP datagram to Joiner_1.
     */

    joinerNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    joinerNode.Get<ThreadNetif>().Up();
    SuccessOrQuit(joinerNode.Get<Mac::Mac>().SetPanChannel(kTestChannel));
    joinerNode.Get<Mac::Mac>().SetSupportedChannelMask(Mac::ChannelMask(1 << kTestChannel));

    // Force Joiner to use Joiner Router by setting its AllowList.
    joinerNode.AllowList(joinerRouterNode);

    SuccessOrQuit(joinerNode.Get<MeshCoP::Seeker>().SetUdpPort(kJoinerPort));

    SuccessOrQuit(joinerNode.Get<MeshCoP::Joiner>().Start(kIncorrectPskd, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                          HandleJoinerComplete, &joinerNode));

    nexus.AdvanceTime(kJoiningProcessTime);

    Log("   Joiner state: %u", joinerNode.Get<MeshCoP::Joiner>().GetState());

    VerifyOrQuit(joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateIdle);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFile = (argc > 2) ? argv[2] : "test_1_1_8_2_2.json";
    ot::Nexus::Test8_2_2(jsonFile);
    printf("All tests passed\n");
    return 0;
}
