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

#include <openthread/config.h>
#include <openthread/tasklet.h>
#include <openthread/thread_direct.h>
#include <openthread/platform/alarm-milli.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/num_utils.hpp"
#include "instance/instance.hpp"
#include "thread_direct/td_key_manager.hpp"
#include "thread_direct/td_peer.hpp"

/*
 * Unit tests for Thread Direct `Peer`, `PeerList`, fixed-array LRU eviction, and heap-mode 20-day
 * periodic peer aging in `KeyManager`.
 */

#if OT_CONFIG_THREAD_DIRECT_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

static uint32_t sNow     = 0;
static uint32_t sAlarm   = 0;
static bool     sAlarmOn = false;

extern "C" {

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmOn = false;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmOn = true;
    sAlarm   = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

} // extern "C"

namespace ot {
namespace ThreadDirect {

class UnitTester
{
public:
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    static bool IsPeerTimerRunning(const KeyManager &aKeyManager) { return aKeyManager.mPeerTimer.IsRunning(); }
#endif
};

static void ResetClock(void)
{
    sNow     = 0;
    sAlarm   = 0;
    sAlarmOn = false;
}

static void AdvanceTime(Instance &aInstance, uint64_t aDurationMsec)
{
    while (aDurationMsec > 0)
    {
        uint64_t step = Min<uint64_t>(aDurationMsec, Time::kOneHourInMsec);

        if (sAlarmOn)
        {
            uint32_t remaining = sAlarm - sNow;

            if (TimeMilli(sAlarm) <= TimeMilli(sNow))
            {
                remaining = 0;
            }

            if (remaining <= step)
            {
                sNow += remaining;
                aDurationMsec -= remaining;
                sAlarmOn = false;
                otPlatAlarmMilliFired(&aInstance);

                while (otTaskletsArePending(&aInstance))
                {
                    otTaskletsProcess(&aInstance);
                }

                continue;
            }
        }

        sNow += static_cast<uint32_t>(step);
        aDurationMsec -= step;
    }

    while (sAlarmOn && (TimeMilli(sAlarm) <= TimeMilli(sNow)))
    {
        sAlarmOn = false;
        otPlatAlarmMilliFired(&aInstance);

        while (otTaskletsArePending(&aInstance))
        {
            otTaskletsProcess(&aInstance);
        }
    }
}

static void MakeKeyMaterial(otThreadDirectKeyMaterial &aMaterial, uint8_t aSeed)
{
    ClearAllBytes(aMaterial);

    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_SIZE; i++)
    {
        aMaterial.mKeyMaterial.mKey.m8[i] = static_cast<uint8_t>((aSeed * 17U) + i + 1U);
    }
}

static void MakeKeyId(otThreadDirectKeyId &aKeyId, uint8_t aSeed)
{
    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_ID_SIZE; i++)
    {
        aKeyId.m8[i] = static_cast<uint8_t>((aSeed << 4) + i + 1U);
    }
}

static void MakeExtAddress(Mac::ExtAddress &aExtAddress, uint8_t aSeed)
{
    for (uint16_t i = 0; i < sizeof(Mac::ExtAddress); i++)
    {
        aExtAddress.m8[i] = static_cast<uint8_t>((aSeed * 0x11U) + i);
    }

    aExtAddress.SetLocal(true);
}

//----------------------------------------------------------------------------------------------------------------------
// Test 1: `Peer` properties, `VerifyAndUpdateFrameCounter()`, `PeerList` operations, and key cleanup

void TestPeerAndPeerListBasics(void)
{
    Instance                 *instance;
    otThreadDirectKeyMaterial keyMat1;
    otThreadDirectKeyMaterial keyMat2;
    otThreadDirectKeyId       keyId1;
    otThreadDirectKeyId       keyId2;
    Key                      *key1;
    Key                      *key2;
    Mac::ExtAddress           addr1;
    Mac::ExtAddress           addr2;
    Peer                     *peer1;
    Peer                     *peer2;

    ResetClock();

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    MakeKeyMaterial(keyMat1, 1);
    MakeKeyId(keyId1, 1);
    MakeKeyMaterial(keyMat2, 2);
    MakeKeyId(keyId2, 2);

    MakeExtAddress(addr1, 0x11);
    MakeExtAddress(addr2, 0x22);

    key1 = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat1, &keyId1));
    key2 = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat2, &keyId2));
    VerifyOrQuit(key1 != nullptr);
    VerifyOrQuit(key2 != nullptr);

    VerifyOrQuit(key1->GetPeers().IsEmpty());
    VerifyOrQuit(key2->GetPeers().IsEmpty());
    VerifyOrQuit(key1->GetPeers().Find(addr1) == nullptr);

    // Add `addr1` to `key1` at uptime 10s with initial frame counter 0.
    AdvanceTime(*instance, 10 * Time::kOneSecondInMsec);
    VerifyOrQuit(instance->Get<UptimeTracker>().GetUptimeInSeconds() == 10);

    peer1 = key1->GetPeers().AddNew(addr1, 0);
    VerifyOrQuit(peer1 != nullptr);
    VerifyOrQuit(!key1->GetPeers().IsEmpty());
    VerifyOrQuit(key2->GetPeers().IsEmpty());

    VerifyOrQuit(key1->GetPeers().Find(addr1) == peer1);
    VerifyOrQuit(key1->GetPeers().Find(addr2) == nullptr);
    VerifyOrQuit(key2->GetPeers().Find(addr1) == nullptr);

    VerifyOrQuit(peer1->GetExtAddress() == addr1);
    VerifyOrQuit(peer1->Matches(addr1));
    VerifyOrQuit(!peer1->Matches(addr2));
    VerifyOrQuit(peer1->GetFrameCounter() == 0);
    VerifyOrQuit(peer1->GetLastRecvTime() == 10);
    VerifyOrQuit(peer1->GetAge(10) == 0);
    VerifyOrQuit(peer1->GetAge(25) == 15);

    // Duplicate counter (0 == 0) at 15s: returns `kErrorDuplicated`, counter stays 0, `LastRecvTime` updates to 15.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(0) == kErrorDuplicated);
    VerifyOrQuit(peer1->GetFrameCounter() == 0);
    VerifyOrQuit(peer1->GetLastRecvTime() == 15);

    // Max counter (`0xffffffff`) at 20s: returns `kErrorSecurity`, counter stays 0, `LastRecvTime` updates to 20.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(NumericLimits<uint32_t>::kMax) == kErrorSecurity);
    VerifyOrQuit(peer1->GetFrameCounter() == 0);
    VerifyOrQuit(peer1->GetLastRecvTime() == 20);

    // Strictly greater counter (1 > 0) at 25s: returns `kErrorNone`, counter updates to 1, `LastRecvTime` updates.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    SuccessOrQuit(peer1->VerifyAndUpdateFrameCounter(1));
    VerifyOrQuit(peer1->GetFrameCounter() == 1);
    VerifyOrQuit(peer1->GetLastRecvTime() == 25);

    // Jump forward to 100 at 30s.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    SuccessOrQuit(peer1->VerifyAndUpdateFrameCounter(100));
    VerifyOrQuit(peer1->GetFrameCounter() == 100);
    VerifyOrQuit(peer1->GetLastRecvTime() == 30);

    // Stale counter (50 < 100) at 35s: returns `kErrorDuplicated`, counter stays 100, `LastRecvTime` updates to 35.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(50) == kErrorDuplicated);
    VerifyOrQuit(peer1->GetFrameCounter() == 100);
    VerifyOrQuit(peer1->GetLastRecvTime() == 35);

    // Equal counter (100 == 100) at 40s: returns `kErrorDuplicated`, counter stays 100, `LastRecvTime` updates to 40.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(100) == kErrorDuplicated);
    VerifyOrQuit(peer1->GetFrameCounter() == 100);
    VerifyOrQuit(peer1->GetLastRecvTime() == 40);

    // Advance up to `kMax - 1` (`0xfffffffe`) at 45s, then verify `kMax` (`0xffffffff`) and `kMax - 1` are rejected.
    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    SuccessOrQuit(peer1->VerifyAndUpdateFrameCounter(NumericLimits<uint32_t>::kMax - 1));
    VerifyOrQuit(peer1->GetFrameCounter() == NumericLimits<uint32_t>::kMax - 1);
    VerifyOrQuit(peer1->GetLastRecvTime() == 45);

    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(NumericLimits<uint32_t>::kMax) == kErrorSecurity);
    VerifyOrQuit(peer1->GetFrameCounter() == NumericLimits<uint32_t>::kMax - 1);
    VerifyOrQuit(peer1->GetLastRecvTime() == 50);

    AdvanceTime(*instance, 5 * Time::kOneSecondInMsec);
    VerifyOrQuit(peer1->VerifyAndUpdateFrameCounter(NumericLimits<uint32_t>::kMax - 1) == kErrorDuplicated);
    VerifyOrQuit(peer1->GetFrameCounter() == NumericLimits<uint32_t>::kMax - 1);
    VerifyOrQuit(peer1->GetLastRecvTime() == 55);

    // Add a second peer to `key1` and an entry to `key2`, then verify `Clear()` and key unregistration.
    peer2 = key1->GetPeers().AddNew(addr2, 7);
    VerifyOrQuit(peer2 != nullptr);
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE || (OPENTHREAD_CONFIG_TD_PEER_LIST_SIZE_PER_KEY >= 2)
    VerifyOrQuit(key1->GetPeers().Find(addr1) != nullptr);
#endif
    VerifyOrQuit(key1->GetPeers().Find(addr2) == peer2);

    VerifyOrQuit(key2->GetPeers().AddNew(addr1, 3) != nullptr);
    VerifyOrQuit(key2->GetPeers().Find(addr1) != nullptr);

    key1->GetPeers().Clear();
    VerifyOrQuit(key1->GetPeers().IsEmpty());
    VerifyOrQuit(key1->GetPeers().Find(addr1) == nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addr2) == nullptr);
    VerifyOrQuit(!key2->GetPeers().IsEmpty());

    // Re-populate `key1`, unregister `key1`, re-register `key1`, and verify its `PeerList` starts empty.
    VerifyOrQuit(key1->GetPeers().AddNew(addr1, 42) != nullptr);
    VerifyOrQuit(!key1->GetPeers().IsEmpty());

    otThreadDirectUnregisterKey(instance, key1);
    key1 = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat1, &keyId1));
    VerifyOrQuit(key1 != nullptr);
    VerifyOrQuit(key1->GetPeers().IsEmpty());
    VerifyOrQuit(key2->GetPeers().Find(addr1) != nullptr);

    testFreeInstance(instance);

    printf("TestPeerAndPeerListBasics() passed\n");
}

//----------------------------------------------------------------------------------------------------------------------
// Test 2: Heap-mode 20-day periodic aging (`#if`) or fixed-array LRU eviction (`#else`)

#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE

void TestPeerHeapAgingTimer(void)
{
    static constexpr uint8_t kExtraPeerCount = 10;

    Instance                 *instance;
    KeyManager               *keyManager;
    otThreadDirectKeyMaterial keyMat1;
    otThreadDirectKeyMaterial keyMat2;
    otThreadDirectKeyId       keyId1;
    otThreadDirectKeyId       keyId2;
    Key                      *key1;
    Key                      *key2;
    Mac::ExtAddress           addrA;
    Mac::ExtAddress           addrB;
    Mac::ExtAddress           addrC;
    Mac::ExtAddress           addrD;
    Mac::ExtAddress           extraAddrs[kExtraPeerCount];
    Peer                     *peerB;

    ResetClock();

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    keyManager = &instance->Get<KeyManager>();

    MakeKeyMaterial(keyMat1, 11);
    MakeKeyId(keyId1, 11);
    MakeKeyMaterial(keyMat2, 12);
    MakeKeyId(keyId2, 12);

    MakeExtAddress(addrA, 0x31);
    MakeExtAddress(addrB, 0x32);
    MakeExtAddress(addrC, 0x33);
    MakeExtAddress(addrD, 0x34);

    for (uint8_t i = 0; i < kExtraPeerCount; i++)
    {
        MakeExtAddress(extraAddrs[i], static_cast<uint8_t>(0x40 + i));
    }

    // Peer aging timer must not run when no peers are tracked.
    VerifyOrQuit(!UnitTester::IsPeerTimerRunning(*keyManager));

    key1 = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat1, &keyId1));
    key2 = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat2, &keyId2));
    VerifyOrQuit(key1 != nullptr);
    VerifyOrQuit(key2 != nullptr);
    VerifyOrQuit(!UnitTester::IsPeerTimerRunning(*keyManager));

    // Day 0 (`uptime == 0`): add `addrA` and `addrB` on `key1`, and `kExtraPeerCount` (10) peers on `key2`
    // (confirming heap mode has no fixed size cap).
    VerifyOrQuit(key1->GetPeers().AddNew(addrA, 10) != nullptr);
    peerB = key1->GetPeers().AddNew(addrB, 20);
    VerifyOrQuit(peerB != nullptr);
    VerifyOrQuit(UnitTester::IsPeerTimerRunning(*keyManager));

    for (uint8_t i = 0; i < kExtraPeerCount; i++)
    {
        VerifyOrQuit(key2->GetPeers().AddNew(extraAddrs[i], 100U + i) != nullptr);
    }

    for (uint8_t i = 0; i < kExtraPeerCount; i++)
    {
        Peer *found = key2->GetPeers().Find(extraAddrs[i]);

        VerifyOrQuit(found != nullptr);
        VerifyOrQuit(found->GetFrameCounter() == 100U + i);
    }

    // Advance 5 days (`uptime == 5 * kOneDayInSec`):
    // - Add `addrC` on `key1`.
    // - Send a rejected stale counter (`5 <= 20`) to `peerB`, refreshing `peerB`'s `mLastRecvTime` to Day 5.
    AdvanceTime(*instance, 5ULL * Time::kOneDayInMsec);
    VerifyOrQuit(instance->Get<UptimeTracker>().GetUptimeInSeconds() == 5 * Time::kOneDayInSec);

    VerifyOrQuit(key1->GetPeers().AddNew(addrC, 30) != nullptr);
    VerifyOrQuit(peerB->VerifyAndUpdateFrameCounter(5) == kErrorDuplicated);
    VerifyOrQuit(peerB->GetFrameCounter() == 20);
    VerifyOrQuit(peerB->GetLastRecvTime() == 5 * Time::kOneDayInSec);

    // Advance 15 more days to Day 20 (`uptime == 20 * kOneDayInSec`):
    // `addrA` and `extraAddrs` have `GetAge() == 20 * kOneDayInSec == kMaxAge` (not strictly older than 20 days),
    // so they must still be retained on the Day 20 timer tick.
    AdvanceTime(*instance, 15ULL * Time::kOneDayInMsec);
    VerifyOrQuit(instance->Get<UptimeTracker>().GetUptimeInSeconds() == 20 * Time::kOneDayInSec);

    VerifyOrQuit(key1->GetPeers().Find(addrA) != nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addrB) != nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addrC) != nullptr);
    VerifyOrQuit(!key2->GetPeers().IsEmpty());

    for (uint8_t i = 0; i < kExtraPeerCount; i++)
    {
        VerifyOrQuit(key2->GetPeers().Find(extraAddrs[i]) != nullptr);
    }

    // Advance 1 more day to Day 21 (`uptime == 21 * kOneDayInSec`):
    // `addrA` and all `extraAddrs` now have `GetAge() == 21 * kOneDayInSec > kMaxAge` and must be removed,
    // while `addrB` (refreshed at Day 5) and `addrC` (added at Day 5) have `GetAge() == 16 * kOneDayInSec` and stay.
    AdvanceTime(*instance, Time::kOneDayInMsec);
    VerifyOrQuit(instance->Get<UptimeTracker>().GetUptimeInSeconds() == 21 * Time::kOneDayInSec);

    VerifyOrQuit(key1->GetPeers().Find(addrA) == nullptr);
    VerifyOrQuit(key2->GetPeers().IsEmpty());

    VerifyOrQuit(key1->GetPeers().Find(addrB) != nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addrB)->GetFrameCounter() == 20);
    VerifyOrQuit(key1->GetPeers().Find(addrC) != nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addrC)->GetFrameCounter() == 30);
    VerifyOrQuit(UnitTester::IsPeerTimerRunning(*keyManager));

    // Advance 4 more days to Day 25 (`GetAge() == 20 * kOneDayInSec == kMaxAge` for `addrB` and `addrC`): still kept.
    AdvanceTime(*instance, 4ULL * Time::kOneDayInMsec);
    VerifyOrQuit(key1->GetPeers().Find(addrB) != nullptr);
    VerifyOrQuit(key1->GetPeers().Find(addrC) != nullptr);
    VerifyOrQuit(UnitTester::IsPeerTimerRunning(*keyManager));

    // Advance 1 more day to Day 26 (`GetAge() == 21 * kOneDayInSec > kMaxAge`):
    // `addrB` and `addrC` expire, all peer lists are now empty, and `mPeerTimer` stops automatically.
    AdvanceTime(*instance, Time::kOneDayInMsec);
    VerifyOrQuit(key1->GetPeers().IsEmpty());
    VerifyOrQuit(key2->GetPeers().IsEmpty());
    VerifyOrQuit(!UnitTester::IsPeerTimerRunning(*keyManager));

    // Adding a new peer restarts `mPeerTimer`, and `otThreadDirectUnregisterAllKeys()` stops it immediately.
    VerifyOrQuit(key1->GetPeers().AddNew(addrD, 99) != nullptr);
    VerifyOrQuit(UnitTester::IsPeerTimerRunning(*keyManager));

    otThreadDirectUnregisterAllKeys(instance);
    VerifyOrQuit(!UnitTester::IsPeerTimerRunning(*keyManager));

    testFreeInstance(instance);

    printf("TestPeerHeapAgingTimer() passed\n");
}

#else // OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE

void TestPeerArrayLruEviction(void)
{
    static constexpr uint16_t kListSize = OPENTHREAD_CONFIG_TD_PEER_LIST_SIZE_PER_KEY;

    Instance                 *instance;
    otThreadDirectKeyMaterial keyMat;
    otThreadDirectKeyId       keyId;
    Key                      *key;
    Mac::ExtAddress           addrs[kListSize];
    Mac::ExtAddress           extraAddr1;
    Mac::ExtAddress           extraAddr2;
    Peer                     *peer;

    ResetClock();

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    MakeKeyMaterial(keyMat, 21);
    MakeKeyId(keyId, 21);

    for (uint16_t i = 0; i < kListSize; i++)
    {
        MakeExtAddress(addrs[i], static_cast<uint8_t>(0x60 + i));
    }

    MakeExtAddress(extraAddr1, 0xa1);
    MakeExtAddress(extraAddr2, 0xa2);

    key = AsCoreTypePtr(otThreadDirectRegisterKey(instance, &keyMat, &keyId));
    VerifyOrQuit(key != nullptr);

    // Fill all `kListSize` slots at uptimes `1s, 2s, ..., kListSize s`.
    for (uint16_t i = 0; i < kListSize; i++)
    {
        AdvanceTime(*instance, Time::kOneSecondInMsec);
        peer = key->GetPeers().AddNew(addrs[i], 10U + i);
        VerifyOrQuit(peer != nullptr);
        VerifyOrQuit(peer->GetLastRecvTime() == static_cast<UptimeSec>(i + 1));
    }

    for (uint16_t i = 0; i < kListSize; i++)
    {
        peer = key->GetPeers().Find(addrs[i]);
        VerifyOrQuit(peer != nullptr);
        VerifyOrQuit(peer->GetFrameCounter() == 10U + i);
    }

#if OPENTHREAD_CONFIG_TD_PEER_LIST_SIZE_PER_KEY == 1
    OT_UNUSED_VARIABLE(extraAddr2);

    // Single-slot list: adding `extraAddr1` replaces `addrs[0]`.
    AdvanceTime(*instance, Time::kOneSecondInMsec);
    VerifyOrQuit(key->GetPeers().AddNew(extraAddr1, 500) != nullptr);
    VerifyOrQuit(key->GetPeers().Find(addrs[0]) == nullptr);
    VerifyOrQuit(key->GetPeers().Find(extraAddr1) != nullptr);
#else
    // Refresh `addrs[0]` via a rejected stale counter so `addrs[1]` becomes the oldest entry (`mLastRecvTime == 2`).
    AdvanceTime(*instance, Time::kOneSecondInMsec);
    peer = key->GetPeers().Find(addrs[0]);
    VerifyOrQuit(peer != nullptr);
    VerifyOrQuit(peer->VerifyAndUpdateFrameCounter(5) == kErrorDuplicated);
    VerifyOrQuit(peer->GetFrameCounter() == 10);

    // Adding `extraAddr1` must evict `addrs[1]` (oldest) while keeping `addrs[0]` and `addrs[2..kListSize-1]`.
    AdvanceTime(*instance, Time::kOneSecondInMsec);
    VerifyOrQuit(key->GetPeers().AddNew(extraAddr1, 500) != nullptr);
    VerifyOrQuit(key->GetPeers().Find(addrs[1]) == nullptr);
    VerifyOrQuit(key->GetPeers().Find(addrs[0]) != nullptr);
    VerifyOrQuit(key->GetPeers().Find(addrs[0])->GetFrameCounter() == 10);
    VerifyOrQuit(key->GetPeers().Find(extraAddr1) != nullptr);
    VerifyOrQuit(key->GetPeers().Find(extraAddr1)->GetFrameCounter() == 500);

    for (uint16_t i = 2; i < kListSize; i++)
    {
        VerifyOrQuit(key->GetPeers().Find(addrs[i]) != nullptr);
    }

    // Adding `extraAddr2` next must evict the next oldest entry (`addrs[2]` if `kListSize >= 3`, else `addrs[0]`).
    AdvanceTime(*instance, Time::kOneSecondInMsec);
    VerifyOrQuit(key->GetPeers().AddNew(extraAddr2, 600) != nullptr);
    VerifyOrQuit(key->GetPeers().Find(extraAddr2) != nullptr);

#if OPENTHREAD_CONFIG_TD_PEER_LIST_SIZE_PER_KEY >= 3
    VerifyOrQuit(key->GetPeers().Find(addrs[2]) == nullptr);
    VerifyOrQuit(key->GetPeers().Find(addrs[0]) != nullptr);
#else
    VerifyOrQuit(key->GetPeers().Find(addrs[0]) == nullptr);
#endif

    VerifyOrQuit(key->GetPeers().Find(extraAddr1) != nullptr);
#endif

    testFreeInstance(instance);

    printf("TestPeerArrayLruEviction() passed\n");
}

#endif // OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

int main(void)
{
#if OT_CONFIG_THREAD_DIRECT_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    ot::ThreadDirect::TestPeerAndPeerListBasics();
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    ot::ThreadDirect::TestPeerHeapAgingTimer();
#else
    ot::ThreadDirect::TestPeerArrayLruEviction();
#endif

    printf("All tests passed\n");
#else
    printf("Thread Direct is not enabled (or platform key refs enabled)\n");
#endif

    return 0;
}
