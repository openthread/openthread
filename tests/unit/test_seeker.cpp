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

#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_SEEKER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE

class UnitTester
{
public:
    using Seeker         = MeshCoP::Seeker;
    using ScanResult     = MeshCoP::Seeker::ScanResult;
    using CandidateEntry = MeshCoP::Seeker::CandidateEntry;

    static void CreateScanResult(ScanResult &aResult, uint64_t aExtPanId, uint64_t aExtAddr, int8_t aRssi)
    {
        ClearAllBytes(aResult);
        LittleEndian::WriteUint64(aExtPanId, aResult.mExtendedPanId.m8);
        LittleEndian::WriteUint64(aExtAddr, aResult.mExtAddress.m8);
        aResult.mRssi          = aRssi;
        aResult.mPanId         = static_cast<uint16_t>(aExtPanId & 0xffff);
        aResult.mChannel       = 11;
        aResult.mJoinerUdpPort = 1000;
    }

    static void LogCandidate(const CandidateEntry &aEntry)
    {
        if (aEntry.IsEmpty())
        {
            printf("   empty\n");
        }
        else
        {
            printf("  ext-addr:%2.2s, ext-panid:%4.4s, rssi:%d, prf:%u, conn-attempted:%u\n",
                   aEntry.mExtAddr.ToString().AsCString(), aEntry.mExtPanId.ToString().AsCString(), aEntry.mRssi,
                   aEntry.mPreferred, aEntry.mConnAttempted);
        }
    }

    static void LogCandidates(const Seeker &aSeeker)
    {
        CandidateEntry entry;

        printf("\nCandidates:\n");

        for (entry.InitForIteration(); aSeeker.mCandidates.ReadNext(entry) == kErrorNone;)
        {
            LogCandidate(entry);
        }

        printf("\n");
    }

    static void SaveCandidate(Seeker &aSeeker, uint64_t aExtPanId, uint64_t aExtAddr, int8_t aRssi, bool aPreferred)
    {
        ScanResult result;

        CreateScanResult(result, aExtPanId, aExtAddr, aRssi);
        aSeeker.SaveCandidate(result, aPreferred);
    }

    static bool Contains(const Seeker &aSeeker, uint64_t aExtPanId, uint64_t aExtAddr)
    {
        MeshCoP::ExtendedPanId extPanId;
        Mac::ExtAddress        extAddr;
        CandidateEntry         entry;

        LittleEndian::WriteUint64(aExtPanId, extPanId.m8);
        LittleEndian::WriteUint64(aExtAddr, extAddr.m8);

        return (aSeeker.mCandidates.FindMatching(entry, extPanId, extAddr) == kErrorNone);
    }

    static void StartCandidateSelection(Seeker &aSeeker)
    {
        // Manually set the state so we can call and validate the
        // `SelectNextCandidate()`.

        aSeeker.SetState(Seeker::kStateConnectingNetworks);
    }

    static void SelectNextCandidate(Seeker &aSeeker, CandidateEntry &aEntry)
    {
        Error error = aSeeker.SelectNextCandidate(aEntry);

        if (error == kErrorNone)
        {
            aEntry.mConnAttempted = true;
            SuccessOrQuit(aSeeker.mCandidates.Write(aEntry));
        }
        else
        {
            aEntry.MarkAsEmpty();
        }
    }

    static void CheckSelectionWith(Seeker &aSeeker, const uint64_t *aExtAddrs, uint16_t aNumExtAddrs)
    {
        CandidateEntry  entry;
        Mac::ExtAddress extAddr;

        printf("\nSelection order:\n");

        StartCandidateSelection(aSeeker);

        for (uint16_t index = 0; index < aNumExtAddrs; index++)
        {
            SelectNextCandidate(aSeeker, entry);
            LogCandidate(entry);

            VerifyOrQuit(!entry.IsEmpty());

            LittleEndian::WriteUint64(aExtAddrs[index], extAddr.m8);
            VerifyOrQuit(entry.mExtAddr == extAddr);
            VerifyOrQuit(entry.mConnAttempted);
        }

        SelectNextCandidate(aSeeker, entry);
        VerifyOrQuit(entry.IsEmpty());
    }

    template <uint16_t kExtAddrSize>
    static void CheckSelection(Seeker &aSeeker, const uint64_t (&aExtAddrArray)[kExtAddrSize])
    {
        CheckSelectionWith(aSeeker, &aExtAddrArray[0], kExtAddrSize);
    }

    static void TestSeekerCandidates(void)
    {
        Instance *instance;

        printf("TestSeekerCandidates()\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        Seeker        &seeker = instance->Get<Seeker>();
        CandidateEntry entry;

        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("Basic addition & replacement\n\n");

        seeker.Stop();

        printf("Save a single candidate");
        SaveCandidate(seeker, 0xaaaa, 0xa1, -50, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 1);
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));

        printf("Save same candidate with better RSSI");
        SaveCandidate(seeker, 0xaaaa, 0xa1, -40, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 1);
        VerifyOrQuit(seeker.mCandidates.ReadAt(0, entry) == kErrorNone);
        VerifyOrQuit(entry.mRssi == -40);

        printf("Save same candidate with worse RSSI, still should replace as it is same extAddr\n");
        SaveCandidate(seeker, 0xaaaa, 0xa1, -60, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 1);
        VerifyOrQuit(seeker.mCandidates.ReadAt(0, entry) == kErrorNone);
        VerifyOrQuit(entry.mRssi == -60);

        printf("Validate candidate selection with single entry in array\n\n");

        CheckSelection(seeker, {0xa1});

        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("Max candidates per network (limit = 3)\n\n");

        seeker.Stop();

        printf("Save 3 candidates for network 0xaaaa along with some extra entries\n");

        SaveCandidate(seeker, 0xaaaa, 0xa1, -50, false);
        SaveCandidate(seeker, 0xbbbb, 0xb1, -70, true);
        SaveCandidate(seeker, 0xaaaa, 0xa2, -52, false);
        SaveCandidate(seeker, 0xcccc, 0xc1, -80, true);
        SaveCandidate(seeker, 0xaaaa, 0xa3, -51, false);
        SaveCandidate(seeker, 0xdddd, 0xd1, -40, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 6);
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa2));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));

        printf("Try adding 4th for 0xaaaa (worse RSSI) -> should be dropped\n");

        SaveCandidate(seeker, 0xaaaa, 0xa4, -90, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 6);
        VerifyOrQuit(!Contains(seeker, 0xaaaa, 0xa4));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa2));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));

        printf("Try adding 4th for 0xaaaa (better RSSI) -> should replace a2 (lowest RSSI)\n");

        SaveCandidate(seeker, 0xaaaa, 0xa5, -40, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 6);
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(!Contains(seeker, 0xaaaa, 0xa2));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));

        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("Behavior under full candidates array and eviction\n\n");

        SaveCandidate(seeker, 0xbbbb, 0xb2, -75, true);
        SaveCandidate(seeker, 0xeeee, 0xe1, -30, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb2));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc1));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe1));

        printf("Try adding new entry 0xb3 for 0xbbbb with better RSSI -> should replace 0xb2\n");
        SaveCandidate(seeker, 0xbbbb, 0xb3, -65, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(!Contains(seeker, 0xbbbb, 0xb2));

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc1));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe1));

        printf("Try adding new entry 0xb4 for 0xbbbb with worst RSSI -> should be dropped\n");
        SaveCandidate(seeker, 0xbbbb, 0xb4, -95, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc1));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe1));

        printf("Try adding new entry 0xc2 for 0xcccc with better RSSI but not preferred -> should be ignored\n");

        SaveCandidate(seeker, 0xcccc, 0xc2, -40, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc1));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe1));

        printf("Try adding new entry 0xc3 for 0xcccc with better RSSI and preferred -> should replace 0xc1\n");

        SaveCandidate(seeker, 0xcccc, 0xc3, -40, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc3));
        VerifyOrQuit(!Contains(seeker, 0xcccc, 0xc1));

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe1));

        printf("Try adding new entry 0xe2 for 0xeeee with worse RSSI but preferred -> should replace 0xe1\n");

        SaveCandidate(seeker, 0xeeee, 0xe2, -99, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(!Contains(seeker, 0xeeee, 0xe1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe2));

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa3));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc3));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));

        printf("Try adding new network, 0xf1 for 0xffff -> should evict 0xa3\n");

        SaveCandidate(seeker, 0xffff, 0xf1, -65, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(!Contains(seeker, 0xaaaa, 0xa3));

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb1));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc3));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe2));
        VerifyOrQuit(Contains(seeker, 0xffff, 0xf1));

        printf("Adding two new entries for new network -> should evict 0xa1 and 0xb1\n");

        SaveCandidate(seeker, 0x1234, 0x01, -80, false);
        SaveCandidate(seeker, 0x5678, 0x02, -70, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(!Contains(seeker, 0xaaaa, 0xa1));
        VerifyOrQuit(!Contains(seeker, 0xbbbb, 0xb1));

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc3));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe2));
        VerifyOrQuit(Contains(seeker, 0xffff, 0xf1));
        VerifyOrQuit(Contains(seeker, 0x1234, 0x01));
        VerifyOrQuit(Contains(seeker, 0x5678, 0x02));

        printf("The candidates array is full and consists of distinct networks\n");
        printf("Try adding a new entry for yet another network -> should be dropped\n");

        SaveCandidate(seeker, 0xabcd, 0x03, -80, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);
        VerifyOrQuit(seeker.mCandidates.IsFull());

        VerifyOrQuit(Contains(seeker, 0xaaaa, 0xa5));
        VerifyOrQuit(Contains(seeker, 0xbbbb, 0xb3));
        VerifyOrQuit(Contains(seeker, 0xcccc, 0xc3));
        VerifyOrQuit(Contains(seeker, 0xdddd, 0xd1));
        VerifyOrQuit(Contains(seeker, 0xeeee, 0xe2));
        VerifyOrQuit(Contains(seeker, 0xffff, 0xf1));
        VerifyOrQuit(Contains(seeker, 0x1234, 0x01));
        VerifyOrQuit(Contains(seeker, 0x5678, 0x02));

        CheckSelection(seeker, {0xc3, 0xb3, 0xe2, 0xa5, 0xd1, 0xf1, 0x02, 0x01});

        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("Selection strategy\n\n");

        seeker.Stop();

        SaveCandidate(seeker, 0xdddd, 0xd1, -30, false);
        SaveCandidate(seeker, 0xaaaa, 0xa1, -60, false);
        SaveCandidate(seeker, 0xeeee, 0xe1, -30, true);
        SaveCandidate(seeker, 0xbbbb, 0xb1, -65, true);
        SaveCandidate(seeker, 0xaaaa, 0xa2, -40, false);
        SaveCandidate(seeker, 0xcccc, 0xc1, -90, true);
        SaveCandidate(seeker, 0xaaaa, 0xa3, -70, false);
        SaveCandidate(seeker, 0xcccc, 0xc2, -40, false);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 8);

        // First we should go through all distinct networks, starting
        // with most favored over all. Then go through the extra
        // backup candidates.
        //
        // For `0xaaaa`, we have 3 candidates:
        //   ext-addr:a2, ext-panid:aaaa, rssi:-40, prf:0, conn-attempted:0
        //   ext-addr:a1, ext-panid:aaaa, rssi:-60, prf:0, conn-attempted:0
        //   ext-addr:a3, ext-panid:aaaa, rssi:-70, prf:0, conn-attempted:0
        //
        // For `0xbbbb`, only one candidate:
        //   ext-addr:b1, ext-panid:bbbb, rssi:-65, prf:1, conn-attempted:0
        //
        // For `0xcccc`, we have two:
        //   ext-addr:c1, ext-panid:cccc, rssi:-90, prf:1, conn-attempted:0
        //   ext-addr:c2, ext-panid:cccc, rssi:-40, prf:0, conn-attempted:0
        //
        // For `0xdddd`, we have one:
        //   ext-addr:d1, ext-panid:dddd, rssi:-30, prf:0, conn-attempted:0
        //
        // For `0xeeee`, we have one:
        // ext-addr:e1, ext-panid:eeee, rssi:-30, prf:1, conn-attempted:0
        //
        // We go through networks first
        //  - e1 has highest RSSI and also preferred
        //  - b1 is preferred with high RSSI
        //  - c1 is also preferred even though it has low RSSI
        //  - d1 has best RSSI among non-preferred
        //  - a2 would be next among all `0xaaaa` candidates
        //
        // Next we go through remaining candidates
        // - c2, a1 and a3

        CheckSelection(seeker, {0xe1, 0xb1, 0xc1, 0xd1, 0xa2, 0xc2, 0xa1, 0xa3});

        seeker.Stop();

        // Adding two candidates for 3 networks (total 6)

        SaveCandidate(seeker, 0xcccc, 0xc2, -92, true);
        SaveCandidate(seeker, 0xaaaa, 0xa2, -76, true);
        SaveCandidate(seeker, 0xbbbb, 0xb2, -56, false);
        SaveCandidate(seeker, 0xbbbb, 0xb1, -55, false);
        SaveCandidate(seeker, 0xcccc, 0xc1, -90, true);
        SaveCandidate(seeker, 0xaaaa, 0xa1, -75, true);
        LogCandidates(seeker);

        VerifyOrQuit(seeker.mCandidates.GetLength() == 6);

        CheckSelection(seeker, {0xa1, 0xc1, 0xb1, 0xa2, 0xc2, 0xb2});

        printf("\nTestSeekerCandidates() passed\n\n");

        testFreeInstance(instance);
    }
};

#endif // #if OPENTHREAD_CONFIG_SEEKER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_SEEKER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE
#if (OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES == 8)
    ot::UnitTester::TestSeekerCandidates();
    printf("All tests passed\n");
#else
    printf("Skipping tests as the test expects `OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES` to be `8`\n");
    printf("This config is specifically set to 8 in `toranj-config` for this test\n");
#endif
#else
    printf("Seeker feature is disabled, skipping the test\n");
#endif

    return 0;
}
