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

/*
 * Verifies TLV parsing in `HistoryTracker::Client::ProcessNetInfoAnswer()`.
 *
 * The answer parse loop previously read a plain 2-byte `Tlv` header and then called
 * `tlv.GetSize()`: a TLV whose length byte is 0xFF (`kExtendedLength`) makes `GetSize()`
 * interpret the object as an `ExtendedTlv` and read a 16-bit extended length that was
 * never read from the message (the codebase norm is to parse the full TLV header from
 * the message first). The loop now uses `Tlv::Info::ParseFrom()`, which reads any
 * extended-length field from the message and validates containment in the offset range.
 *
 * This test asserts: an answer carrying a truncated extended-length TLV is rejected as
 * a parse error (client finalizes with an error, no crash), and a subsequent
 * well-formed query round still completes.
 */

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "coap/coap_message.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"
#include "utils/history_tracker_client.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;

static bool  sNetInfoCallbackInvoked;
static Error sNetInfoLastError;

static void HandleNetInfo(otError aError, const otHistoryTrackerNetworkInfo *, uint32_t, void *)
{
    sNetInfoCallbackInvoked = true;
    sNetInfoLastError       = static_cast<Error>(aError);
}

static void SendAnswer(Node &aServer, uint16_t aClientRloc16, uint16_t aQueryId, bool aMalformed)
{
    Coap::Message *message = aServer.Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriHistoryAnswer);

    VerifyOrQuit(message != nullptr);

    {
        // QueryId TLV (type 0, len 2, big-endian value), Answer TLV (type 1, len 2, index 0).
        uint8_t tlvs[] = {
            0x00, 0x02, static_cast<uint8_t>(aQueryId >> 8), static_cast<uint8_t>(aQueryId & 0xff), 0x01, 0x02,
            0x00, 0x00};

        SuccessOrQuit(message->AppendBytes(tlvs, sizeof(tlvs)));
    }

    if (aMalformed)
    {
        // Malformed TLV: any type, length byte 0xFF (kExtendedLength), NO extended-length bytes.
        uint8_t malformed[] = {0x77, 0xFF};

        SuccessOrQuit(message->AppendBytes(malformed, sizeof(malformed)));
    }
    else
    {
        // Well-formed empty NetworkInfo TLV = "no more entries" terminator.
        uint8_t done[] = {0x03, 0x00};

        SuccessOrQuit(message->AppendBytes(done, sizeof(done)));
    }

    SuccessOrQuit(aServer.Get<Tmf::Agent>().SendMessageToRloc(*message, aClientRloc16));
}

void TestHistoryAnswerMalformedTlv(void)
{
    Core  nexus;
    Node &server = nexus.CreateNode(); // leader; the node the client queries
    Node &client = nexus.CreateNode();

    server.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    client.Join(server);
    nexus.AdvanceTime(20 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsAttached());

    uint16_t serverRloc = server.Get<Mle::Mle>().GetRloc16();
    uint16_t clientRloc = client.Get<Mle::Mle>().GetRloc16();

    Log("Round 1: client queries server; server answers with malformed TLV [0x77 0xFF]");
    sNetInfoCallbackInvoked = false;
    SuccessOrQuit(client.Get<HistoryTracker::Client>().QueryNetInfo(serverRloc, 0, 0, HandleNetInfo, nullptr));

    // First query on a fresh instance => QueryId 1 (mQueryId is a simple increment from 0).
    SendAnswer(server, clientRloc, /* aQueryId */ 1, /* aMalformed */ true);
    nexus.AdvanceTime(5 * 1000);

    // Expected behavior: the malformed answer is rejected as a parse error (the client
    // finalizes the query with an error).
    VerifyOrQuit(sNetInfoCallbackInvoked);
    VerifyOrQuit(sNetInfoLastError != kErrorNone);
    Log("Malformed answer rejected with error %d (no crash)", static_cast<int>(sNetInfoLastError));

    nexus.AdvanceTime(30 * 1000);

    Log("Round 2 (positive control): a well-formed answer round still works");
    sNetInfoCallbackInvoked = false;
    sNetInfoLastError       = kErrorFailed;
    SuccessOrQuit(client.Get<HistoryTracker::Client>().QueryNetInfo(serverRloc, 0, 0, HandleNetInfo, nullptr));
    SendAnswer(server, clientRloc, /* aQueryId */ 2, /* aMalformed */ false);
    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sNetInfoCallbackInvoked);
    VerifyOrQuit(sNetInfoLastError == kErrorNone);
    Log("Well-formed 'done' answer accepted (query finalized cleanly)");

    Log("TestHistoryAnswerMalformedTlv passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestHistoryAnswerMalformedTlv();
    printf("All tests passed\n");
    return 0;
}
