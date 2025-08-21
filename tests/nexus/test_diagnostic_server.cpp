/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "common/error.hpp"
#include "common/num_utils.hpp"
#include "openthread/diag_server.h"

#include "openthread/ip6.h"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/diagnostic_server_types.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace Nexus {

class DiagnosticValidator
{
public:
    DiagnosticValidator(Node &aNode)
        : mNode(aNode)
    {
    }

    void Start(const DiagnosticServer::TlvSet &aHost,
               const DiagnosticServer::TlvSet &aChild,
               const DiagnosticServer::TlvSet &aNeighbor);
    void Stop(void);

    // Verifies that no rloc occurs more than once
    bool ValidateUnique(void);

    bool ValidateRouter(Node &aNode);
    bool ValidateChild(Node &aNode);

private:
    class Tlvs
    {
    public:
        DiagnosticServer::TlvSet mValidTlvs;
        otDiagServerTlv          mTlvs[OT_DIAG_SERVER_DATA_TLV_MAX];
    };

    class ChildEntry : public Tlvs, public Clearable<ChildEntry>
    {
    public:
        uint16_t mRloc16;
    };

    class RouterEntry : public Tlvs, public Clearable<RouterEntry>
    {
    public:
        ChildEntry &GetOrCreateChild(uint16_t aRloc16);
        ChildEntry *GetChild(uint16_t aRloc16);
        void        RemoveChild(uint16_t aRloc16);

        bool    mValid : 1;
        uint8_t mRouterId : 6;

        ChildEntry mChildren[Mle::kMaxChildren];
        uint16_t   mChildCount;
    };

    static void UpdateCallback(const otMessage *aMessage, uint16_t aRloc16, bool aComplete, void *aContext);
    void        UpdateCallback(const otMessage *aMessage, uint16_t aRloc16, bool aComplete);

    Node &mNode;

    DiagnosticServer::TlvSet mHostRequested;
    DiagnosticServer::TlvSet mChildRequested;

    RouterEntry mRouters[Mle::kMaxRouterId + 1];
};

void DiagnosticValidator::Start(const DiagnosticServer::TlvSet &aHost,
                                const DiagnosticServer::TlvSet &aChild,
                                const DiagnosticServer::TlvSet &aNeighbor)
{
    for (size_t i = 0; i <= Mle::kMaxRouterId; i++)
    {
        mRouters[i].Clear();
    }

    mHostRequested  = aHost;
    mChildRequested = aChild;

    otDiagServerStartClient(&mNode.GetInstance(), &aHost, &aChild, &aNeighbor, UpdateCallback, this);
}

void DiagnosticValidator::Stop(void) { otDiagServerStopClient(&mNode.GetInstance()); }

bool DiagnosticValidator::ValidateRouter(Node &aNode)
{
    bool         valid = false;
    RouterEntry &entry = mRouters[Mle::RouterIdFromRloc16(aNode.Get<Mle::Mle>().GetRloc16())];
    const char  *msg   = nullptr;

    VerifyOrExit(entry.mValid, msg = "Router not present");

    VerifyOrExit(entry.mValidTlvs.ContainsAll(mHostRequested), msg = "Router missing tlvs");

    valid = true;

exit:
    if (msg != nullptr)
    {
        Log("ERROR: %s", msg);
    }
    return valid;
}

bool DiagnosticValidator::ValidateChild(Node &aNode)
{
    bool         valid  = false;
    RouterEntry &router = mRouters[Mle::RouterIdFromRloc16(aNode.Get<Mle::Mle>().GetRloc16())];
    ChildEntry  *child;
    const char  *msg = nullptr;

    VerifyOrExit(router.mValid, msg = "Router not present");

    child = router.GetChild(aNode.Get<Mle::Mle>().GetRloc16());
    VerifyOrExit(child != nullptr, msg = "Child not present");

    VerifyOrExit(child->mValidTlvs.ContainsAll(mChildRequested), msg = "Child missing tlvs");

    valid = true;

exit:
    if (msg != nullptr)
    {
        Log("ERROR: %s", msg);
    }
    return valid;
}

DiagnosticValidator::ChildEntry &DiagnosticValidator::RouterEntry::GetOrCreateChild(uint16_t aRloc16)
{
    ChildEntry *entry = nullptr;

    for (size_t i = 0; i < mChildCount; i++)
    {
        if (mChildren[i].mRloc16 == aRloc16)
        {
            entry = &mChildren[i];
            break;
        }
    }

    if (entry == nullptr)
    {
        VerifyOrQuit(mChildCount < Mle::kMaxChildren);
        entry = &mChildren[mChildCount++];

        entry->Clear();
        entry->mRloc16 = aRloc16;
    }

    VerifyOrQuit(entry != nullptr);
    return *entry;
}

DiagnosticValidator::ChildEntry *DiagnosticValidator::RouterEntry::GetChild(uint16_t aRloc16)
{
    ChildEntry *entry = nullptr;

    for (size_t i = 0; i < mChildCount; i++)
    {
        if (mChildren[i].mRloc16 == aRloc16)
        {
            entry = &mChildren[i];
            break;
        }
    }

    return entry;
}

void DiagnosticValidator::RouterEntry::RemoveChild(uint16_t aRloc16)
{
    for (size_t i = 0; i < mChildCount; i++)
    {
        if (mChildren[i].mRloc16 == aRloc16)
        {
            mChildCount--;
            if (i < mChildCount)
            {
                mChildren[i] = mChildren[mChildCount];
            }
            break;
        }
    }
}

void DiagnosticValidator::UpdateCallback(const otMessage *aMessage, uint16_t aRloc16, bool aComplete, void *aContext)
{
    static_cast<DiagnosticValidator *>(aContext)->UpdateCallback(aMessage, aRloc16, aComplete);
}

void DiagnosticValidator::UpdateCallback(const otMessage *aMessage, uint16_t aRloc16, bool aComplete)
{
    Error        error  = kErrorNone;
    RouterEntry &router = mRouters[Mle::RouterIdFromRloc16(aRloc16)];

    otDiagServerIterator contextIter = OT_DIAG_SERVER_ITERATOR_INIT;
    otDiagServerContext  context;
    otDiagServerTlv      tlv;

    router.mValid = true;

    Log("Diagnostic Update (%04X)", aRloc16);

    while ((error = otDiagServerGetNextContext(aMessage, &contextIter, &context)) == kErrorNone)
    {
        Tlvs *tlvs = nullptr;

        bool presenceChanged = false;
        bool empty           = true;
        bool modeRemove      = false;

        switch (context.mType)
        {
        case OT_DIAG_SERVER_DEVICE_HOST:
            Log("  Context [Host, %04X]", context.mRloc16);
            tlvs = &router;
            break;

        case OT_DIAG_SERVER_DEVICE_CHILD:
            Log("  Context [Child, %04X]", context.mRloc16);
            tlvs = &router.GetOrCreateChild(context.mRloc16);
            break;

        case OT_DIAG_SERVER_DEVICE_NEIGHBOR:
            Log("  Context [Neighbor, %04X]", context.mRloc16);
            break;

        default:
            break;
        }

        if (context.mType == OT_DIAG_SERVER_DEVICE_CHILD || context.mType == OT_DIAG_SERVER_DEVICE_NEIGHBOR)
        {
            presenceChanged = (context.mUpdateMode == OT_DIAG_SERVER_UPDATE_MODE_ADDED) ||
                              (context.mUpdateMode == OT_DIAG_SERVER_UPDATE_MODE_REMOVED);
            modeRemove = context.mUpdateMode == OT_DIAG_SERVER_UPDATE_MODE_REMOVED;
        }

        while ((error = otDiagServerGetNextTlv(aMessage, &context, &tlv)) == kErrorNone)
        {
            VerifyOrQuit(DiagnosticServer::Tlv::IsKnownTlv(tlv.mType));
            VerifyOrQuit(!modeRemove);

            if (tlv.mType == OT_DIAG_SERVER_TLV_IP6_ADDRESS_LIST ||
                tlv.mType == OT_DIAG_SERVER_TLV_IP6_LINK_LOCAL_ADDRESS_LIST)
            {
                char         str[OT_IP6_ADDRESS_STRING_SIZE];
                otIp6Address adr[5];

                Log("    Tlv: %s (%lu), count: %lu", DiagnosticServer::Tlv::TypeValueToString(tlv.mType),
                    ToUlong(tlv.mType), ToUlong(tlv.mData.mIp6AddressList.mCount));

                if (tlv.mData.mIp6AddressList.mCount < 5)
                {
                    SuccessOrQuit(otDiagServerGetIp6Addresses(aMessage, tlv.mData.mIp6AddressList.mDataOffset,
                                                              tlv.mData.mIp6AddressList.mCount, adr));

                    for (uint32_t i = 0; i < tlv.mData.mIp6AddressList.mCount; i++)
                    {
                        otIp6AddressToString(&adr[i], str, OT_IP6_ADDRESS_STRING_SIZE);
                        Log("      %s", str);
                    }
                }
            }
            else if (tlv.mType == OT_DIAG_SERVER_TLV_ALOC_LIST)
            {
                Log("    Tlv: %s (%lu), count: %lu", DiagnosticServer::Tlv::TypeValueToString(tlv.mType),
                    ToUlong(tlv.mType), ToUlong(tlv.mData.mAlocList.mCount));
            }
            else
            {
                Log("    Tlv: %s (%lu)", DiagnosticServer::Tlv::TypeValueToString(tlv.mType), ToUlong(tlv.mType));
            }

            if (tlvs != nullptr)
            {
                tlvs->mValidTlvs.Set(static_cast<DiagnosticServer::Tlv::Type>(tlv.mType));
                tlvs->mTlvs[tlv.mType] = tlv;
            }

            empty = false;
        }

        VerifyOrQuit(error == kErrorNotFound);
        if (empty && !presenceChanged)
        {
            Log("WARN: Context with update mode updated contains no tlvs");
        }

        if (modeRemove)
        {
            switch (context.mType)
            {
            case OT_DIAG_SERVER_DEVICE_CHILD:
                router.RemoveChild(context.mRloc16);
                break;

            case OT_DIAG_SERVER_DEVICE_NEIGHBOR:
                break;

            default:
                VerifyOrQuit(false);
            }
        }
    }

    VerifyOrQuit(error == kErrorNotFound);
}

void TestDiagnosticServerBasic(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &mtd1    = nexus.CreateNode();
    Node &mtd2    = nexus.CreateNode();
    Node &client  = nexus.CreateNode();

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    DiagnosticServer::TlvSet hostSet;
    DiagnosticServer::TlvSet childSet;
    DiagnosticServer::TlvSet neighborSet;

    hostSet.Clear();
    hostSet.Set(DiagnosticServer::Tlv::kMacAddress);
    hostSet.Set(DiagnosticServer::Tlv::kMleCounters);
    hostSet.Set(DiagnosticServer::Tlv::kIp6AddressList);
    hostSet.Set(DiagnosticServer::Tlv::kAlocList);
    hostSet.Set(DiagnosticServer::Tlv::kIp6LinkLocalAddressList);

    childSet.Clear();
    childSet.Set(DiagnosticServer::Tlv::kMacAddress);
    childSet.Set(DiagnosticServer::Tlv::kMleCounters);
    childSet.Set(DiagnosticServer::Tlv::kIp6AddressList);
    childSet.Set(DiagnosticServer::Tlv::kAlocList);
    childSet.Set(DiagnosticServer::Tlv::kIp6LinkLocalAddressList);

    neighborSet.Clear();
    neighborSet.Set(DiagnosticServer::Tlv::kMacAddress);

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("--- Test Basic                                                                      ---");
    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Join nodes");

    mtd1.Join(leader, Node::kAsMed);
    mtd2.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(mtd2.Get<Mle::Mle>().IsChild());

    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(120 * 1000);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(120 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    for (uint32_t i = 0; i < 5; i++)
    {
        Log("Start Stop Iteration %lu", ToUlong(i));

        validator->Start(hostSet, childSet, neighborSet);
        nexus.AdvanceTime(100 * 1000);

        VerifyOrQuit(validator->ValidateRouter(leader));
        VerifyOrQuit(validator->ValidateRouter(router1));
        // VerifyOrQuit(validator->ValidateRouter(client));
        VerifyOrQuit(validator->ValidateChild(mtd1));
        VerifyOrQuit(validator->ValidateChild(mtd2));

        validator->Stop();
        nexus.AdvanceTime(30 * 60 * 1000); // 30min
    }

    Log("---------------------------------------------------------------------------------------");

    delete validator;
}

void TestDiagnosticServerLargeChildTable(void)
{
    constexpr size_t kNumChildren = 30;

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &client = nexus.CreateNode();
    Node *children[kNumChildren];

    DiagnosticServer::TlvSet hostSet;
    DiagnosticServer::TlvSet childSet;
    DiagnosticServer::TlvSet neighborSet;

    hostSet.Clear();
    hostSet.Set(DiagnosticServer::Tlv::kMacAddress);
    hostSet.Set(DiagnosticServer::Tlv::kMleCounters);
    hostSet.Set(DiagnosticServer::Tlv::kIp6AddressList);

    childSet.Clear();
    childSet.Set(DiagnosticServer::Tlv::kMacAddress);
    childSet.Set(DiagnosticServer::Tlv::kMleCounters);
    childSet.Set(DiagnosticServer::Tlv::kIp6AddressList);

    neighborSet.Clear();
    neighborSet.Set(DiagnosticServer::Tlv::kMacAddress);

    for (size_t i = 0; i < kNumChildren; i++)
    {
        children[i] = &nexus.CreateNode();
    }

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("--- Test Large Child Table                                                          ---");
    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Join nodes");

    for (size_t i = 0; i < kNumChildren; i++)
    {
        children[i]->Join(leader, Node::kAsMed);
        nexus.AdvanceTime(2 * 1000);
        VerifyOrQuit(children[i]->Get<Mle::Mle>().IsChild());
    }

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(120 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    for (uint32_t i = 0; i < 5; i++)
    {
        Log("Start Stop Iteration %lu", ToUlong(i));

        validator->Start(hostSet, childSet, neighborSet);
        nexus.AdvanceTime(100 * 1000);

        VerifyOrQuit(validator->ValidateRouter(leader));
        for (size_t i = 0; i < kNumChildren; i++)
        {
            VerifyOrQuit(validator->ValidateChild(*children[i]));
        }

        validator->Stop();
        nexus.AdvanceTime(30 * 60 * 1000); // 30min
    }

    Log("---------------------------------------------------------------------------------------");

    delete validator;
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDiagnosticServerBasic();
    ot::Nexus::TestDiagnosticServerLargeChildTable();
    printf("All tests passed\n");
    return 0;
}
