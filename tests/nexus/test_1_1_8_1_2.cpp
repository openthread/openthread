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
static constexpr uint32_t kJoiningProcessTime = 30 * Time::kOneSecondInMsec;

/**
 * The timeout for the Joiner entry on Commissioner, in seconds.
 */
static constexpr uint32_t kJoinerTimeout = 100;

/**
 * The PSKd used by Commissioner.
 */
static const char kCorrectPskd[] = "J01NME";

/**
 * The PSKd used by Joiner (incorrect).
 */
static const char kIncorrectPskd[] = "WR0NGPSK";

/**
 * The filename for the DTLS key log.
 */
static const char kKeyLogFilename[] = "test_1_1_8_1_2.keys";

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

void Test8_1_2(void)
{
    /**
     * 8.1.2 On-Mesh Commissioner Joining, no JR, any commissioner, single (incorrect)
     *
     * 8.1.2.1 Topology
     * (Note: A topology diagram is included in the source material depicting Topology A (DUT as Commissioner) and
     *   Topology B (DUT as Joiner_1), with a point-to-point connection between the Commissioner and Joiner_1.)
     *
     * 8.1.2.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT - as an on-mesh Commissioner - detects the use of an
     *   incorrect PSKd by a Joiner.
     * Note that many of the messages/records exchanged are encrypted and cannot be observed.
     *
     * Spec Reference                             | V1.1 Section  | V1.3.0 Section
     * -------------------------------------------|---------------|---------------
     * Dissemination of Datasets / Joiner Protocol | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4
     */

    Core nexus;

    Node &commissionerNode = nexus.CreateNode();
    Node &joinerNode       = nexus.CreateNode();

    commissionerNode.SetName("Commissioner");
    joinerNode.SetName("Joiner_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    // Remove any existing key log file.
    remove(kKeyLogFilename);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Joiner_1");

    /**
     * Step 1: Joiner_1
     * - Description: PSKd configured on Joiner_1 is different to PSKd configured on the DUT.
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner (DUT)");

    /**
     * Step 2: Commissioner (DUT)
     * - Description: Begin wireless sniffer and power on the DUT.
     * - Pass Criteria: N/A.
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
    static_cast<MeshCoP::SecureTransport &>(joinerNode.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleMbedtlsExportKeys, &joinerNode.Get<Tmf::SecureAgent>());
#endif

    SuccessOrQuit(commissioner.AddJoinerAny(kCorrectPskd, kJoinerTimeout));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Joiner_1");

    /**
     * Step 3: Joiner_1
     * - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the DUT.
     * - Pass Criteria: Verify the following details occur in the exchange between Joiner_1 and the DUT:
     *   1. UDP port (Specified by the DUT in Discovery Response) is used as destination port for UDP datagrams from
     *     Joiner_1 to the DUT.
     *   2. Joiner_1 sends an initial DTLS-ClientHello handshake record to the DUT.
     *   3. The DUT must correctly receive the initial DTLS-ClientHello handshake record and send a
     *     DTLS-HelloVerifyRequest handshake record to Joiner_1.
     *   4. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
     *     handshake record in one UDP datagram to the DUT.
     *   5. Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
     *   6. The DUT must correctly receive the subsequent DTLS-ClientHello handshake record and then send, in order,
     *     DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records to Joiner_1.
     *   7. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
     *     and sends, in order, a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an
     *     encrypted DTLS-Finished handshake record to the DUT.
     *   8. The DUT must receive the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec and the
     *     encrypted DTLS-Finished handshake record, and then send a DTLS-Alert record with error code 40 (handshake
     *     failure) or 20 (bad record MAC) - in one UDP datagram - to Joiner_1.
     */

    joinerNode.Get<ThreadNetif>().Up();
    SuccessOrQuit(joinerNode.Get<Mac::Mac>().SetPanChannel(kTestChannel));
    joinerNode.Get<Mac::Mac>().SetSupportedChannelMask(Mac::ChannelMask(1 << kTestChannel));

    SuccessOrQuit(joinerNode.Get<MeshCoP::Seeker>().SetUdpPort(kJoinerPort));

    SuccessOrQuit(joinerNode.Get<MeshCoP::Joiner>().Start(kIncorrectPskd, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                          HandleJoinerComplete, &joinerNode));

    nexus.AdvanceTime(kJoiningProcessTime);

    Log("   Joiner state: %u", joinerNode.Get<MeshCoP::Joiner>().GetState());

    VerifyOrQuit(joinerNode.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateIdle);

    nexus.SaveTestInfo("test_1_1_8_1_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test8_1_2();
    printf("All tests passed\n");
    return 0;
}
