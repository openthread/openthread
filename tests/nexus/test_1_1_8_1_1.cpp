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
 * Time to advance for the Joiner to complete the joining process, in milliseconds.
 */
static constexpr uint32_t kJoiningProcessTime = 60 * Time::kOneSecondInMsec;

/**
 * The timeout for the Joiner entry on Commissioner, in seconds.
 */
static constexpr uint32_t kJoinerTimeout = 100;

/**
 * The PSKd used for joining.
 */
static const char kPskd[] = "J01NME";

/**
 * The filename for the DTLS key log.
 */
static const char kKeyLogFilename[] = "test_1_1_8_1_1.keys";

/**
 * The master secret length in bytes.
 */
static constexpr uint8_t kMasterSecretLength = 48;

/**
 * The random buffer length in bytes.
 */
static constexpr uint8_t kRandomBufferLength = MeshCoP::SecureTransport::kSecureTransportRandomBufferSize;

/**
 * The channel used for the test.
 */
static constexpr uint8_t kTestChannel = 11;

/**
 * The Joiner UDP port.
 */
static constexpr uint16_t kJoinerPort = 49153;

static void HandleMbedtlsExportKeys(void                       *aContext,
                                    mbedtls_ssl_key_export_type aType,
                                    const unsigned char        *aMasterSecret,
                                    size_t                      aMasterSecretLen,
                                    const unsigned char         aClientRandom[kRandomBufferLength],
                                    const unsigned char         aServerRandom[kRandomBufferLength],
                                    mbedtls_tls_prf_types       aTlsPrfType)
{
    FILE *file = fopen(kKeyLogFilename, "a");

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMasterSecretLen);
    OT_UNUSED_VARIABLE(aServerRandom);
    OT_UNUSED_VARIABLE(aTlsPrfType);

    if (file != nullptr)
    {
        if (aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET)
        {
            fprintf(file, "CLIENT_RANDOM ");
            for (int i = 0; i < kRandomBufferLength; i++)
            {
                fprintf(file, "%02x", aClientRandom[i]);
            }
            fprintf(file, " ");
            for (int i = 0; i < kMasterSecretLength; i++)
            {
                fprintf(file, "%02x", aMasterSecret[i]);
            }
            fprintf(file, "\n");
        }
        fclose(file);
    }
}

static void HandleJoinerComplete(otError aError, void *aContext)
{
    Node *node = static_cast<Node *>(aContext);
    Log("   Joiner on node %u completed with error: %s", node->GetId(), otThreadErrorToString(aError));
}

void Test1_1_8_1_1(void)
{
    /**
     * 8.1.1 On-Mesh Commissioner Joining, no JR, any commissioner, single (correct)
     *
     * 8.1.1.1 Topology
     * (Note: A topology diagram is included in the source material depicting Topology A (DUT as Commissioner) and
     *   Topology B (DUT as Joiner_1), with a point-to-point connection between the Commissioner and Joiner_1.)
     *
     * 8.1.1.2 Purpose & Description
     * The purpose of this test case is to verify the DTLS sessions between the on-mesh Commissioner and a Joiner when
     *   the correct PSKd is used.
     * * Note that many of the messages/records exchanged are encrypted and cannot be observed.
     *
     * Spec Reference   | V1.1 Section  | V1.3.0 Section
     * -----------------|---------------|---------------
     * Joiner Protocol  | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4
     */

    Core nexus;

    Node &commissionerNode = nexus.CreateNode();
    Node &joinerNode       = nexus.CreateNode();

    commissionerNode.SetName("Commissioner");
    joinerNode.SetName("Joiner");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    // Remove any existing key log file.
    remove(kKeyLogFilename);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Joiner_1");

    /**
     * Step 1: Joiner_1
     * - Description: Configure Joiner_1 to a correct PSKd same as PSKd configured on Commissioner.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Begin wireless sniffer and Commissioner.
     * - Pass Criteria: N/A
     */

    commissionerNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    joinerNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);

    commissionerNode.Form();
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(commissionerNode.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kTestChannel);
        commissionerNode.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }
    nexus.AdvanceTime(kFormNetworkTime);

    MeshCoP::Commissioner &commissioner = commissionerNode.Get<MeshCoP::Commissioner>();

    SuccessOrQuit(commissioner.Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kCommissionerStartTime);

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
    static_cast<MeshCoP::SecureTransport &>(commissionerNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleMbedtlsExportKeys, &commissionerNode.Get<Tmf::SecureAgent>());
#endif

    SuccessOrQuit(commissioner.AddJoinerAny(kPskd, kJoinerTimeout));
    nexus.AdvanceTime(Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Joiner_1");

    /**
     * Step 3: Joiner_1
     * - Description: Sends MLE Discovery Request.
     * - Pass Criteria: For DUT = Joiner: The DUT MUST send MLE Discovery Request, including the following values:
     *   - MLE Security Suite = 255 (No MLE Security)
     *   - Thread Discovery TLV Sub-TLVs:
     *     - Discovery Request TLV
     *     - Protocol Version = 2, 3, 4 or 5
     *   - Optional: [list of one or more concatenated Extended PAN ID TLVs]
     */

    joinerNode.Get<ThreadNetif>().Up();
    SuccessOrQuit(joinerNode.Get<Mac::Mac>().SetPanChannel(kTestChannel));
    joinerNode.Get<Mac::Mac>().SetSupportedChannelMask(Mac::ChannelMask(1 << kTestChannel));

    SuccessOrQuit(joinerNode.Get<MeshCoP::Seeker>().SetUdpPort(kJoinerPort));

    SuccessOrQuit(joinerNode.Get<MeshCoP::Joiner>().Start(kPskd, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                          HandleJoinerComplete, &joinerNode));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Commissioner");

    /**
     * Step 4: Commissioner
     * - Description: Automatically sends MLE Discovery Response.
     * - Pass Criteria: For DUT = Commissioner: The DUT MUST send MLE Discovery Response, including these values:
     *   - MLE Security Suite: 255 (No MLE Security)
     *   - Source Address in IEEE 802.15.4 header MUST be set to the MAC Extended Address (64-bit)
     *   - Destination Address in IEEE 802.15.4 header MUST be set to Discovery Request Source Address
     *   - Thread Discovery TLV Sub-TLVs:
     *     - Discovery Response TLV (Protocol Version = 2, 3, 4 or 5)
     *     - Extended PAN ID TLV
     *     - Joiner UDP Port TLV
     *     - Network Name TLV
     *     - Steering Data TLV
     *     - Commissioner UDP Port TLV (optional)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Joiner_1 and Commissioner");

    /**
     * Step 5: Joiner_1 and Commissioner
     * - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner
     *   from Joiner_1.
     * - Pass Criteria: For DUT = Joiner:
     *   - 1. Joiner_1 sends an initial DTLS-ClientHello handshake record to Commissioner. The DUT must send
     *     DTLS_Client_Hello to the Commissioner, using UDP port 49153. If the Commissioner responds with HelloVerify,
     *     the DUT MUST send DTLS-ClientHello on UDP port 49153 and with the same cookie sent by the Commissioner. The
     *     DUT must send a combined packet of DTLS-clientKeyExchange, DTLS-changeCipherSpec and encrypted DTLS-Finished.
     *     Verify the following details occur in the exchange between Joiner_1 and the Commissioner:
     *   - 2. UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
     *     from Joiner_1 to Commissioner.
     *   - 3. Joiner_1 sends an initial DTLS-ClientHello handshake record to Commissioner.
     *   - 4. Commissioner receives the initial DTLS-ClientHello handshake record and sends a DTLS-HelloVerifyRequest
     *     handshake record to Joiner_1.
     *   - 5. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
     *     handshake record in one UDP datagram to Commissioner.
     *     - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
     *   - 6. Commissioner receives the subsequent DTLS-ClientHello handshake record and sends DTLS-ServerHello,
     *     DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records in that order to Joiner_1.
     *   - 7. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
     *     and sends a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an encrypted
     *     DTLS-Finished handshake record in that order to Commissioner.
     *   - 8. Commissioner receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record and
     *     the encrypted DTLS-Finished handshake record and sends a DTLS-ChangeCipherSpec record and an encrypted
     *     DTLS-Finished handshake record in that order to Joiner_1.
     *   - 9. Joiner_1 receives the DTLS-ChangeCipherSpec record and the encrypted DTLS-Finished handshake record and
     *     sends a JOIN_FIN.req message in an encrypted DTLS-ApplicationData record in a single UDP datagram to
     *     Commissioner.
     *     - The JOIN_FIN.req message must not contain a Provisioning URL TLV.
     *   - 10. Commissioner receives the encrypted DTLS-ApplicationData record and sends a JOIN_FIN.rsp message in an
     *     encrypted DTLS-ApplicationData record in a single UDP datagram to Joiner_1.
     *   - 11. Commissioner sends an encrypted JOIN_ENT.ntf message to Joiner_1.
     *   - 12. Joiner_1 receives the encrypted JOIN_ENT.ntf message and sends an encrypted JOIN_ENT.ntf dummy response
     *     to Commissioner.
     *   - 13. Joiner_1 sends an encrypted DTLS-Alert record with a code of 0 (close_notify) to Commissioner.
     * - Fail Conditions:
     *   - 1. The UDP port used is not the one specified.
     *   - 2. One of the records described in pass conditions is not present.
     *   - 3. A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or Commissioner.
     */

    nexus.AdvanceTime(kJoiningProcessTime);

    Log("   Joiner state: %u", joinerNode.Get<MeshCoP::Joiner>().GetState());

    VerifyOrQuit(joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateJoined ||
                 joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateIdle);

    nexus.SaveTestInfo("test_1_1_8_1_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_1_8_1_1();
    printf("All tests passed\n");
    return 0;
}
