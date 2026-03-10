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
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * Time::kOneSecondInMsec;

/**
 * Time to advance for the joining process to complete, in milliseconds.
 */
static constexpr uint32_t kJoiningProcessTime = 30 * Time::kOneSecondInMsec;

/**
 * Timeout for Joiner entry in Commissioner, in seconds.
 */
static constexpr uint32_t kJoinerTimeout = 120;

/**
 * PSKd used for joining.
 */
static const char kPskd[] = "J01NME";

/**
 * The channel used for the test.
 */
static constexpr uint8_t kTestChannel = 11;

/**
 * The Joiner UDP port.
 */
static constexpr uint16_t kJoinerPort = 49153;

/**
 * The default JSON file name.
 */
static const char kDefaultJsonFile[] = "test_1_1_8_2_1.json";

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
/**
 * The filename for the DTLS key log.
 */
static const char kKeyLogFilename[] = "test_1_1_8_2_1.keys";

/**
 * The random buffer length in bytes.
 */
static constexpr uint8_t kRandomBufferLength = MeshCoP::SecureTransport::kSecureTransportRandomBufferSize;

static void HandleKeylog(void                       *aContext,
                         mbedtls_ssl_key_export_type aType,
                         const unsigned char        *aMasterSecret,
                         size_t                      aMasterSecretLen,
                         const unsigned char         aClientRandom[kRandomBufferLength],
                         const unsigned char         aServerRandom[kRandomBufferLength],
                         mbedtls_tls_prf_types       aTlsPrfType)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aServerRandom);
    OT_UNUSED_VARIABLE(aTlsPrfType);

    if (aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET)
    {
        FILE *f = fopen(kKeyLogFilename, "a");

        if (f != nullptr)
        {
            fprintf(f, "CLIENT_RANDOM ");
            for (size_t i = 0; i < kRandomBufferLength; i++)
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

void Test1_1_8_2_1(const char *aJsonFileName)
{
    /**
     * 8.2.1 On Mesh Commissioner Joining with JR, any commissioner, single (correct)
     *
     * 8.2.1.1 Topology
     * - Commissioner (Leader)
     * - Joiner Router
     * - Joiner_1
     *
     * 8.2.1.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT sends and receives relayed DTLS traffic correctly when
     *   the correct PSKd is used by the Joiner. It also verifies that the DUT (as a Joiner Router) sends JOIN_ENT.ntf
     *   encrypted with KEK.
     *
     * Spec Reference | V1.1 Section  | V1.3.0 Section
     * ---------------|---------------|---------------
     * Relay traffic  | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4
     */

    Core nexus;

    Node &commNode   = nexus.CreateNode();
    Node &jrNode     = nexus.CreateNode();
    Node &joinerNode = nexus.CreateNode();

    commNode.SetName("LEADER");
    jrNode.SetName("ROUTER");
    joinerNode.SetName("JOINER");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: All");

    /**
     * Step 0: All
     * - Description: Form topology excluding the Joiner and including the DUT. The DUT may be joined to the network
     *   using either Thread Commissioning or OOB (out-of-band) Commissioning.
     * - Pass Criteria: N/A.
     */

    commNode.Form();
    commNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    jrNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(commNode.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kTestChannel);
        commNode.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(commNode.Get<Mle::Mle>().IsLeader());

    jrNode.Join(commNode);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(jrNode.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Joiner_1");

    /**
     * Step 1: Joiner_1
     * - Description: PSKd configured on Joiner_1 is the same as PSKd configured on the Commissioner.
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Begin wireless sniffer and power on the Commissioner and Joiner Router.
     * - Pass Criteria: N/A.
     */

    SuccessOrQuit(commNode.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(Time::kOneSecondInMsec);
    VerifyOrQuit(commNode.Get<MeshCoP::Commissioner>().IsActive());

    SuccessOrQuit(commNode.Get<MeshCoP::Commissioner>().AddJoinerAny(kPskd, kJoinerTimeout));
    commNode.Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(kJoinerPort);
    jrNode.Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(kJoinerPort);

    nexus.AdvanceTime(Time::kOneSecondInMsec);

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
    static_cast<MeshCoP::SecureTransport &>(commNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleKeylog, &commNode.Get<Tmf::SecureAgent>());
    static_cast<MeshCoP::SecureTransport &>(joinerNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleKeylog, &joinerNode.Get<Tmf::SecureAgent>());
#endif

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Joiner_1");

    /**
     * Step 3: Joiner_1
     * - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner.
     * - Pass Criteria: Verify that the following details occur in the exchange between the Joiner, the Joiner_Router,
     *   and the Commissioner:
     *   - 1. UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
     *     from Joiner_1 to Commissioner.
     *   - 2. Joiner_1 sends an initial DTLS-ClientHello handshake record to the Joiner_Router.
     *   - 3. The Joiner_Router receives the initial DTLS-ClientHello handshake record and sends a RLY_RX.ntf message
     *     to the Commissioner containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = initial
     *         DTLS-ClientHello handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 4. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<JR>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x011), Length = Variable (8 bit size), Value =
     *         DTLS-HelloVerifyRequest handshake record destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 5. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-HelloVerifyRequest
     *     handshake record in one UDP datagram to Joiner_1.
     *   - 6. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
     *     handshake record in one UDP datagram to the Commissioner.
     *     - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
     *   - 7. The Joiner_Router receives the subsequent DTLS-ClientHello handshake record and sends a RLY_RX.ntf
     *     message to the Commissioner containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = subsequent
     *         DTLS-ClientHello handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 8. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<JR>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records, in that order,
     *         destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 9. Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-ServerHello,
     *     DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records in one UDP datagram to Joiner_1.
     *   - 10. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake
     *     records and sends a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an
     *     encrypted DTLS-Finished handshake record, in that order, to Joiner_Router.
     *   - 11. Joiner_Router receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record and
     *     the encrypted DTLS-Finished handshake record; it then sends a RLY_RX.ntf message to the Commissioner
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         DTLS-ClientKeyExchange handshake record, DTLS-ChangeCipherSpec record and encrypted DTLS-Finished
     *         handshake record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 12. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<JR>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         DTLS-ChangeCipherSpec record and encrypted DTLS-Finished handshake record, in that order, destined
     *         for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 13. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-ChangeCipherSpec
     *     record and encrypted DTLS-Finished handshake record in one UDP datagram to Joiner_1.
     *   - 14. Joiner_1 receives the DTLS-ChangeCipherSpec record and the encrypted DTLS-Finished handshake record; it
     *     then sends a JOIN_FIN.req message in an encrypted DTLS-ApplicationData record in a single UDP datagram to
     *     the Joiner_Router.
     *     - The JOIN_FIN.req message must not contain a Provisioning URL TLV.
     *   - 15. The Joiner_Router receives the encrypted DTLS-ApplicationData record and sends a RLY_RX.ntf message to
     *     the Commissioner containing:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         JOIN_FIN.req message in an encrypted DTLS-ApplicationData record received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 16. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
     *     containing:
     *     - CoAP URI-Path: NON POST coap://<JR>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
     *         JOIN_FIN.rsp message in an encrypted DTLS-ApplicationData record destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 8, Value = RLOC of Joiner_Router.
     *       - Joiner_Router KEK TLV: Type = 21 (0x15), Length = 16, Value = KEK used for JOIN_ENT messages.
     *   - 17. The Joiner_Router receives the RLY_TX.ntf message and sends a JOIN_FIN.rsp and the encapsulated
     *     DTLS-ApplicationData to Joiner_1.
     *   - 18. The Joiner_Router sends an encrypted JOIN_ENT.ntf message to Joiner_1.
     *   - 19. Joiner_1 receives the encrypted JOIN_ENT.ntf message and sends an encrypted JOIN_ENT.ntf dummy
     *     response to the Joiner_Router.
     *   - 20. Joiner_1 sends an encrypted DTLS-Alert record with a code of 0 (close_notify) to the Joiner_Router.
     *   - 21. The Joiner_Router receives the encrypted DTLS-Alert record and sends a RLY_RX.ntf message to the
     *     Commissioner on behalf of Joiner_1. The RLY_RX.ntf contains:
     *     - CoAP URI-Path: NON POST coap://<C>/c/rx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = encrypted
     *         DTLS-Alert record (close_notify = 0) received from Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 22. The Commissioner receives the RLY_RX.ntf messages and sends RLY_TX.ntf messages to the Joiner_Router
     *     for delivery to Joiner_1. The RLY_TX.ntf for Joiner_1 contains:
     *     - CoAP URI-Path: NON POST coap://<JR>/c/tx
     *     - CoAP Payload:
     *       - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x011), Length = Variable (8/16 bit size), Value = encrypted
     *         DTLS-Alert record (close_notify = 0) destined for Joiner_1.
     *       - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
     *       - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
     *       - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
     *   - 23. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated, encrypted DTLS-Alert
     *     record with a code of 0 (close_notify) to Joiner_1.
     */

    joinerNode.Get<ThreadNetif>().Up();
    SuccessOrQuit(joinerNode.Get<Mac::Mac>().SetPanChannel(kTestChannel));
    joinerNode.Get<Mac::Mac>().SetSupportedChannelMask(Mac::ChannelMask(1 << kTestChannel));

    joinerNode.AllowList(jrNode);

    SuccessOrQuit(joinerNode.Get<MeshCoP::Seeker>().SetUdpPort(kJoinerPort));
    SuccessOrQuit(
        joinerNode.Get<MeshCoP::Joiner>().Start(kPskd, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));

    nexus.AdvanceTime(kJoiningProcessTime);
    Log("   Joiner state: %s", MeshCoP::Joiner::StateToString(joinerNode.Get<MeshCoP::Joiner>().GetState()));

    VerifyOrQuit(joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateJoined ||
                 joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateIdle);

    {
        NetworkKey networkKey;
        NetworkKey joinerKey;

        commNode.Get<KeyManager>().GetNetworkKey(networkKey);
        joinerNode.Get<KeyManager>().GetNetworkKey(joinerKey);
        VerifyOrQuit(joinerKey == networkKey);
    }

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFile = (argc > 2) ? argv[2] : ot::Nexus::kDefaultJsonFile;
    ot::Nexus::Test1_1_8_2_1(jsonFile);
    printf("All tests passed\n");
    return 0;
}
