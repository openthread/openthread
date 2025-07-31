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
#include "openthread/ext_network_diagnostic.h"

#include "openthread/ip6.h"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/ext_network_diagnostic_types.hpp"
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

    void Start(const ExtNetworkDiagnostic::TlvSet &aHost,
               const ExtNetworkDiagnostic::TlvSet &aChild,
               const ExtNetworkDiagnostic::TlvSet &aNeighbor);
    void Stop(void);

    // Verifies that no rloc occurs more than once
    bool ValidateUnique(void);

    bool ValidateRouter(Node &aNode);
    bool ValidateChild(Node &aNode);

    class Tlvs
    {
    public:
        ExtNetworkDiagnostic::TlvSet mValidTlvs;
        otExtNetworkDiagnosticTlv    mTlvs[OT_EXT_NETWORK_DIAGNOSTIC_DATA_TLV_MAX];
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

    template <typename EntryType>
    bool ValidateTlvValue(Node &aNode, EntryType &aEntry, ExtNetworkDiagnostic::Tlv::Type aTlvType);

    RouterEntry mRouters[Mle::kMaxRouterId + 1];

private:
    static void HandleServerUpdate(const otMessage *aMessage, uint16_t aRloc16, bool aComplete, void *aContext);
    void        HandleServerUpdate(const otMessage *aMessage, uint16_t aRloc16, bool aComplete);

    // Individual TLV validators
    bool ValidateMacAddress(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateMode(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateTimeout(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateLastHeard(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateConnectionTime(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateCsl(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateMlEid(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateThreadSpecVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateVendorName(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateVendorModel(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateVendorSwVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateVendorAppUrl(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateThreadStackVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateIp6AddressList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateAlocList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateIp6LinkLocalAddressList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);
    bool ValidateEui64(Node &aNode, otExtNetworkDiagnosticTlv &aTlv);

    Node &mNode;

    ExtNetworkDiagnostic::TlvSet mHostRequested;
    ExtNetworkDiagnostic::TlvSet mChildRequested;
    ExtNetworkDiagnostic::TlvSet missingTlvs;
};

void DiagnosticValidator::Start(const ExtNetworkDiagnostic::TlvSet &aHost,
                                const ExtNetworkDiagnostic::TlvSet &aChild,
                                const ExtNetworkDiagnostic::TlvSet &aNeighbor)
{
    for (size_t i = 0; i <= Mle::kMaxRouterId; i++)
    {
        mRouters[i].Clear();
    }

    mHostRequested  = aHost;
    mChildRequested = aChild;

    otExtNetworkDiagnosticStartClient(&mNode.GetInstance(), &aHost, &aChild, &aNeighbor, HandleServerUpdate, this);
}

void DiagnosticValidator::Stop(void) { otExtNetworkDiagnosticStopClient(&mNode.GetInstance()); }

bool DiagnosticValidator::ValidateRouter(Node &aNode)
{
    bool         valid = false;
    RouterEntry &entry = mRouters[Mle::RouterIdFromRloc16(aNode.Get<Mle::Mle>().GetRloc16())];
    const char  *msg   = nullptr;

    VerifyOrExit(entry.mValid, msg = "Router not present");

    missingTlvs = mHostRequested.Cut(entry.mValidTlvs);

    // Check if there are any missing TLVs
    if (!missingTlvs.IsEmpty())
    {
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacAddress))
            Log("Missing TLV: kMacAddress");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMode))
            Log("Missing TLV: kMode");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kTimeout))
            Log("Missing TLV: kTimeout");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kRoute64))
            Log("Missing TLV: kRoute64");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMlEid))
            Log("Missing TLV: kMlEid");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6AddressList))
            Log("Missing TLV: kIp6AddressList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kAlocList))
            Log("Missing TLV: kAlocList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion))
            Log("Missing TLV: kThreadSpecVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kThreadStackVersion))
            Log("Missing TLV: kThreadStackVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorName))
            Log("Missing TLV: kVendorName");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorModel))
            Log("Missing TLV: kVendorModel");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorSwVersion))
            Log("Missing TLV: kVendorSwVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorAppUrl))
            Log("Missing TLV: kVendorAppUrl");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList))
            Log("Missing TLV: kIp6LinkLocalAddressList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kEui64))
            Log("Missing TLV: kEui64");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacCounters))
            Log("Missing TLV: kMacCounters");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMleCounters))
            Log("Missing TLV: kMleCounters");
    }

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

    missingTlvs = mChildRequested.Cut(child->mValidTlvs);

    // Check if there are any missing TLVs
    if (!missingTlvs.IsEmpty())
    {
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacAddress))
            Log("Missing TLV: kMacAddress");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMode))
            Log("Missing TLV: kMode");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kTimeout))
            Log("Missing TLV: kTimeout");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kLastHeard))
            Log("Missing TLV: kLastHeard");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kConnectionTime))
            Log("Missing TLV: kConnectionTime");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kCsl))
            Log("Missing TLV: kCsl");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kLinkMarginIn))
            Log("Missing TLV: kLinkMarginIn");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMlEid))
            Log("Missing TLV: kMlEid");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6AddressList))
            Log("Missing TLV: kIp6AddressList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kAlocList))
            Log("Missing TLV: kAlocList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion))
            Log("Missing TLV: kThreadSpecVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kThreadStackVersion))
            Log("Missing TLV: kThreadStackVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorName))
            Log("Missing TLV: kVendorName");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorModel))
            Log("Missing TLV: kVendorModel");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorSwVersion))
            Log("Missing TLV: kVendorSwVersion");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kVendorAppUrl))
            Log("Missing TLV: kVendorAppUrl");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList))
            Log("Missing TLV: kIp6LinkLocalAddressList");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kEui64))
            Log("Missing TLV: kEui64");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacCounters))
            Log("Missing TLV: kMacCounters");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacLinkErrorRatesIn))
            Log("Missing TLV: kMacLinkErrorRatesIn");
        if (missingTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMleCounters))
            Log("Missing TLV: kMleCounters");
    }

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

void DiagnosticValidator::HandleServerUpdate(const otMessage *aMessage,
                                             uint16_t         aRloc16,
                                             bool             aComplete,
                                             void            *aContext)
{
    static_cast<DiagnosticValidator *>(aContext)->HandleServerUpdate(aMessage, aRloc16, aComplete);
}

void DiagnosticValidator::HandleServerUpdate(const otMessage *aMessage, uint16_t aRloc16, bool aComplete)
{
    Error        error  = kErrorNone;
    RouterEntry &router = mRouters[Mle::RouterIdFromRloc16(aRloc16)];

    otExtNetworkDiagnosticIterator contextIter = OT_EXT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
    otExtNetworkDiagnosticContext  context;
    otExtNetworkDiagnosticTlv      tlv;

    uint32_t tlvCount = 0;

    router.mValid = true;

    Log("Diagnostic Update (%04X)", aRloc16);

    while ((error = otExtNetworkDiagnosticGetNextContext(aMessage, &contextIter, &context)) == kErrorNone)
    {
        Tlvs *tlvs = nullptr;

        bool presenceChanged = false;
        bool empty           = true;
        bool modeRemove      = false;

        switch (context.mType)
        {
        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_HOST:
            Log("  Context [Host, %04X]", context.mRloc16);
            tlvs = &router;
            break;

        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD:
            Log("  Context [Child, %04X]", context.mRloc16);
            tlvs = &router.GetOrCreateChild(context.mRloc16);
            break;

        case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR:
            Log("  Context [Neighbor, %04X]", context.mRloc16);
            break;

        default:
            break;
        }

        if (context.mType == OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD ||
            context.mType == OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR)
        {
            presenceChanged = (context.mUpdateMode == OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_ADDED) ||
                              (context.mUpdateMode == OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_REMOVED);
            modeRemove = context.mUpdateMode == OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_REMOVED;
        }

        while ((error = otExtNetworkDiagnosticGetNextTlv(aMessage, &context, &tlv)) == kErrorNone)
        {
            VerifyOrQuit(ExtNetworkDiagnostic::Tlv::IsKnownTlv(tlv.mType));
            VerifyOrQuit(!modeRemove);

            // HANDLE IP6 ADDRESS LIST and IP6 LINK LOCAL ADDRESS LIST
            if (tlv.mType == OT_EXT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDRESS_LIST ||
                tlv.mType == OT_EXT_NETWORK_DIAGNOSTIC_TLV_IP6_LINK_LOCAL_ADDRESS_LIST)
            {
                char         str[OT_IP6_ADDRESS_STRING_SIZE];
                otIp6Address adr[5];

                Log("    Tlv: %s (%lu), count: %lu", ExtNetworkDiagnostic::Tlv::TypeValueToString(tlv.mType),
                    ToUlong(tlv.mType), ToUlong(tlv.mData.mIp6AddressList.mCount));

                if (tlv.mData.mIp6AddressList.mCount <= 5)
                {
                    SuccessOrQuit(otExtNetworkDiagnosticGetIp6Addresses(aMessage, tlv.mData.mIp6AddressList.mDataOffset,
                                                                        tlv.mData.mIp6AddressList.mCount, adr));

                    for (uint32_t i = 0; i < tlv.mData.mIp6AddressList.mCount; i++)
                    {
                        otIp6AddressToString(&adr[i], str, OT_IP6_ADDRESS_STRING_SIZE);
                        Log("      [%lu] %s", ToUlong(i), str);
                    }
                }
            }
            // HANDLE ALOC LIST
            else if (tlv.mType == OT_EXT_NETWORK_DIAGNOSTIC_TLV_ALOC_LIST)
            {
                uint8_t alocs[5];

                Log("    Tlv: %s (%lu), count: %lu", ExtNetworkDiagnostic::Tlv::TypeValueToString(tlv.mType),
                    ToUlong(tlv.mType), ToUlong(tlv.mData.mAlocList.mCount));

                if (tlv.mData.mAlocList.mCount <= 5)
                {
                    SuccessOrQuit(otExtNetworkDiagnosticGetAlocs(aMessage, tlv.mData.mAlocList.mDataOffset,
                                                                 tlv.mData.mAlocList.mCount, alocs));

                    for (uint32_t i = 0; i < tlv.mData.mAlocList.mCount; i++)
                    {
                        Log("      [%lu] 0x%02x", ToUlong(i), alocs[i]);
                    }
                }
            }
            // HANDLE ROUTE64
            else if (tlv.mType == OT_EXT_NETWORK_DIAGNOSTIC_TLV_ROUTE64)
            {
                Log("    Tlv: %s (%lu), seq=%u, routers=%u", ExtNetworkDiagnostic::Tlv::TypeValueToString(tlv.mType),
                    ToUlong(tlv.mType), tlv.mData.mRoute64.mRouterIdSequence, tlv.mData.mRoute64.mRouterCount);

                if (tlv.mData.mRoute64.mRouterCount > 0 && tlv.mData.mRoute64.mRouterCount <= 32)
                {
                    otExtNetworkDiagnosticRouteData routeData[32];

                    if (otExtNetworkDiagnosticGetRouteData(aMessage, tlv.mData.mRoute64.mDataOffset,
                                                           tlv.mData.mRoute64.mRouterIdMask,
                                                           tlv.mData.mRoute64.mRouterCount, routeData) == kErrorNone)
                    {
                        for (uint8_t i = 0; i < tlv.mData.mRoute64.mRouterCount; i++)
                        {
                            Log("      Router %u: LQIn=%u, LQOut=%u, Cost=%u", routeData[i].mRouterId,
                                routeData[i].mLinkQualityIn, routeData[i].mLinkQualityOut, routeData[i].mRouteCost);
                        }
                    }
                }
            }
            else
            {
                Log("    Tlv: %s (%lu)", ExtNetworkDiagnostic::Tlv::TypeValueToString(tlv.mType), ToUlong(tlv.mType));
            }

            if (tlvs != nullptr)
            {
                tlvs->mValidTlvs.Set(static_cast<ExtNetworkDiagnostic::Tlv::Type>(tlv.mType));
                tlvs->mTlvs[tlv.mType] = tlv;
            }

            empty = false;
            // Count processed TLVs for end-of-update summary
            tlvCount++;
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
            case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD:
                router.RemoveChild(context.mRloc16);
                break;

            case OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR:
                break;

            default:
                VerifyOrQuit(false);
            }
        }
    }

    VerifyOrQuit(error == kErrorNotFound);
}

template <typename EntryType>
bool DiagnosticValidator::ValidateTlvValue(Node &aNode, EntryType &aEntry, ExtNetworkDiagnostic::Tlv::Type aTlvType)
{
    if (!aEntry.mValidTlvs.IsSet(aTlvType))
    {
        Log("ERROR: TLV %s not present", ExtNetworkDiagnostic::Tlv::TypeValueToString(aTlvType));
        return false;
    }

    otExtNetworkDiagnosticTlv &tlv = aEntry.mTlvs[aTlvType];

    switch (aTlvType)
    {
    case ExtNetworkDiagnostic::Tlv::kMacAddress:
        return ValidateMacAddress(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kMode:
        return ValidateMode(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kTimeout:
        return ValidateTimeout(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kLastHeard:
        return ValidateLastHeard(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kConnectionTime:
        return ValidateConnectionTime(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kCsl:
        return ValidateCsl(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kMlEid:
        return ValidateMlEid(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kThreadSpecVersion:
        return ValidateThreadSpecVersion(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kVendorName:
        return ValidateVendorName(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kVendorModel:
        return ValidateVendorModel(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kVendorSwVersion:
        return ValidateVendorSwVersion(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kVendorAppUrl:
        return ValidateVendorAppUrl(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kThreadStackVersion:
        return ValidateThreadStackVersion(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kAlocList:
        return ValidateAlocList(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kIp6AddressList:
        return ValidateIp6AddressList(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList:
        return ValidateIp6LinkLocalAddressList(aNode, tlv);

    case ExtNetworkDiagnostic::Tlv::kEui64:
        return ValidateEui64(aNode, tlv);

    default:
        Log("ERROR: Unknown TLV type %u", static_cast<uint8_t>(aTlvType));
        return false;
    }
}

/* Individual TLV validators implementation */
bool DiagnosticValidator::ValidateMacAddress(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    Mac::ExtAddress expectedMac = aNode.Get<Mac::Mac>().GetExtAddress();
    otExtAddress   &actualMac   = aTlv.mData.mExtAddress;

    Mac::ExtAddress actualMacAddr;
    actualMacAddr.Set(actualMac.m8);

    if (actualMacAddr != expectedMac)
    {
        Log("ERROR: MAC mismatch. Expected: %s, Actual: %s", expectedMac.ToString().AsCString(),
            actualMacAddr.ToString().AsCString());
        return false;
    }

    char macStr[24];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", actualMac.m8[0], actualMac.m8[1],
             actualMac.m8[2], actualMac.m8[3], actualMac.m8[4], actualMac.m8[5], actualMac.m8[6], actualMac.m8[7]);
    Log("SUCCESS: MAC Address validated: %s", macStr);
    return true;
}

bool DiagnosticValidator::ValidateMode(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    Mle::DeviceMode expectedMode = aNode.Get<Mle::Mle>().GetDeviceMode();

    Mle::DeviceMode::ModeConfig actualMode = aTlv.mData.mMode;

    VerifyOrQuit(actualMode.mRxOnWhenIdle == expectedMode.IsRxOnWhenIdle(), "ERROR: Mode RxOnWhenIdle mismatch");
    VerifyOrQuit(actualMode.mDeviceType == expectedMode.IsFullThreadDevice(), "ERROR: Mode DeviceType mismatch");
    bool expectedFullNetworkData = (expectedMode.GetNetworkDataType() == NetworkData::kFullSet);
    VerifyOrQuit(actualMode.mNetworkData == expectedFullNetworkData, "ERROR: Mode NetworkData mismatch");
    Log("SUCCESS: Mode validated: %u", expectedMode.Get());
    return true;
}

bool DiagnosticValidator::ValidateTimeout(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint32_t expectedTimeout = aNode.Get<Mle::Mle>().GetTimeout();
    uint32_t actualTimeout   = aTlv.mData.mTimeout;

    if (actualTimeout != expectedTimeout)
    {
        Log("ERROR: Timeout mismatch. Expected: %lu, Actual: %lu", ToUlong(expectedTimeout), ToUlong(actualTimeout));
        return false;
    }

    Log("SUCCESS: Timeout validated: %lu", ToUlong(actualTimeout));
    return true;
}

bool DiagnosticValidator::ValidateLastHeard(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint32_t maximumLastHeard = 10 * 30 * 1000;        // 5 minutes in milliseconds
    uint32_t actualLastHeard  = aTlv.mData.mLastHeard; // in milliseconds

    if (actualLastHeard > maximumLastHeard)
    {
        Log("ERROR: LastHeard mismatch. Expected at most: %lu, Actual: %lu", ToUlong(maximumLastHeard),
            ToUlong(actualLastHeard));
        return false;
    }

    Log("SUCCESS: LastHeard validated: %lu", ToUlong(actualLastHeard));
    return true;
}

bool DiagnosticValidator::ValidateConnectionTime(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint32_t minimumConnectionTime = 1;                          // 1 second
    uint32_t actualConnectionTime  = aTlv.mData.mConnectionTime; // in seconds

    if (actualConnectionTime < minimumConnectionTime)
    {
        Log("ERROR: ConnectionTime mismatch. Expected at least: %lu, Actual: %lu", ToUlong(minimumConnectionTime),
            ToUlong(actualConnectionTime));
        return false;
    }

    Log("SUCCESS: ConnectionTime validated: %lu", ToUlong(actualConnectionTime));
    return true;
}

bool DiagnosticValidator::ValidateCsl(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint32_t timeout = aTlv.mData.mCsl.mTimeout;
    uint16_t period  = aTlv.mData.mCsl.mPeriod;
    uint8_t  channel = aTlv.mData.mCsl.mChannel;

    if (timeout != 0)
    {
        if (timeout < 30 || timeout > 3600)
        {
            Log("ERROR: CSL timeout out of range: %lu (valid: 0 or 30-3600)", timeout);
            return false;
        }
    }
    else
    {
        Log("WARNING: CSL is disabled (timeout = 0)");
    }

    if (period != 0)
    {
        if (period < 16 || period > 65535)
        {
            Log("ERROR: CSL period out of range: %u (valid: 0 or 16-65535)", period);
            return false;
        }
    }
    else
    {
        Log("WARNING: CSL not synchronized (period = 0)");
    }

    if (channel != 0)
    {
        if (channel < 11 || channel > 26)
        {
            Log("ERROR: CSL channel invalid: %u (valid: 0 or 11-26)", channel);
            return false;
        }
    }

    Log("SUCCESS: CSL validated - timeout: %lu, period: %u, channel: %u", timeout, period, channel);
    return true;
}

bool DiagnosticValidator::ValidateMlEid(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const Ip6::Address &meshLocalEid = aNode.Get<Mle::Mle>().GetMeshLocalEid();

    const otIp6InterfaceIdentifier &expectedIid =
        *reinterpret_cast<const otIp6InterfaceIdentifier *>(&meshLocalEid.mFields.m8[8]);

    if (memcmp(&expectedIid, &aTlv.mData.mMlEid, sizeof(otIp6InterfaceIdentifier)) != 0)
    {
        Log("MlEid Interface Identifier mismatch:");
        Log("  Expected: %02x%02x:%02x%02x:%02x%02x:%02x%02x", expectedIid.mFields.m8[0], expectedIid.mFields.m8[1],
            expectedIid.mFields.m8[2], expectedIid.mFields.m8[3], expectedIid.mFields.m8[4], expectedIid.mFields.m8[5],
            expectedIid.mFields.m8[6], expectedIid.mFields.m8[7]);
        Log("  Received: %02x%02x:%02x%02x:%02x%02x:%02x%02x", aTlv.mData.mMlEid.mFields.m8[0],
            aTlv.mData.mMlEid.mFields.m8[1], aTlv.mData.mMlEid.mFields.m8[2], aTlv.mData.mMlEid.mFields.m8[3],
            aTlv.mData.mMlEid.mFields.m8[4], aTlv.mData.mMlEid.mFields.m8[5], aTlv.mData.mMlEid.mFields.m8[6],
            aTlv.mData.mMlEid.mFields.m8[7]);
        return false;
    }

    bool isAllZeros = true;
    bool isAllOnes  = true;
    for (size_t i = 0; i < sizeof(otIp6InterfaceIdentifier); i++)
    {
        if (aTlv.mData.mMlEid.mFields.m8[i] != 0x00)
            isAllZeros = false;
        if (aTlv.mData.mMlEid.mFields.m8[i] != 0xFF)
            isAllOnes = false;
    }

    if (isAllZeros || isAllOnes)
    {
        Log("MlEid Interface Identifier sanity check failed (all zeros or all 0xFF)");
        Log("  Collected IID: %02x%02x:%02x%02x:%02x%02x:%02x%02x", aTlv.mData.mMlEid.mFields.m8[0],
            aTlv.mData.mMlEid.mFields.m8[1], aTlv.mData.mMlEid.mFields.m8[2], aTlv.mData.mMlEid.mFields.m8[3],
            aTlv.mData.mMlEid.mFields.m8[4], aTlv.mData.mMlEid.mFields.m8[5], aTlv.mData.mMlEid.mFields.m8[6],
            aTlv.mData.mMlEid.mFields.m8[7]);
        return false;
    }

    if ((meshLocalEid.mFields.m8[0] != 0xFD))
    {
        Log("Mesh Local EID does not have FD prefix");
        Log("  Prefix bytes: 0x%02x", meshLocalEid.mFields.m8[0]);
        return false;
    }

    Log("  Address: %s", meshLocalEid.ToString().AsCString());
    Log("  Interface ID: %02x%02x:%02x%02x:%02x%02x:%02x%02x", expectedIid.mFields.m8[0], expectedIid.mFields.m8[1],
        expectedIid.mFields.m8[2], expectedIid.mFields.m8[3], expectedIid.mFields.m8[4], expectedIid.mFields.m8[5],
        expectedIid.mFields.m8[6], expectedIid.mFields.m8[7]);

    Log("SUCCESS: MlEid validation successful");
    return true;
}

bool DiagnosticValidator::ValidateThreadSpecVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint16_t version = aTlv.mData.mThreadSpecVersion;

    // Thread Specification Table 4-2 defines ONLY 2 entries:
    // Value 2 = Thread 1.1.x / 1.2.x
    // Value >= 3 = Thread 1.3.x and later

    if (version == 2)
    {
        Log("SUCCESS: ThreadSpecVersion Value: %u - Thread 1.1.x/1.2.x - VALID", version);
        return true;
    }
    else if (version >= 3)
    {
        Log("SUCCESS: ThreadSpecVersion Value: %u - Thread 1.3.x+ - VALID", version);
        return true;
    }
    else
    {
        // Version 0, 1 - NOT in the spec table
        Log("ERROR: ThreadSpecVersion Value: %u - NOT DEFINED - INVALID", version);
        return false;
    }
}

bool DiagnosticValidator::ValidateVendorName(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const char *expectedVendorName = aNode.Get<NetworkDiagnostic::Server>().GetVendorName();
    const char *actualVendorName   = aTlv.mData.mVendorName;
    size_t      actualVendorNameLen;

    actualVendorNameLen = strnlen(actualVendorName, OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH + 1);

    if (actualVendorNameLen > OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH)
    {
        Log("ERROR: Vendor Name TLV length exceeds maximum. Length: %zu (max: %u)", actualVendorNameLen,
            OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH);
        return false;
    }

    if (actualVendorNameLen == 0)
    {
        Log("ERROR: Vendor Name TLV is empty");
        return false;
    }

    if (strncmp(expectedVendorName, actualVendorName, actualVendorNameLen) != 0)
    {
        Log("ERROR: Vendor Name mismatch. Expected: %s, Actual: %s", expectedVendorName, actualVendorName);
        return false;
    }

    for (size_t i = 0; i < actualVendorNameLen; i++)
    {
        unsigned char byte = static_cast<unsigned char>(actualVendorName[i]);

        // Check for control characters (except tab, newline, carriage return)
        if (byte < 0x20 && byte != 0x09 && byte != 0x0A && byte != 0x0D)
        {
            Log("ERROR: Vendor Name contains invalid control character at position %zu: 0x%02x", i, byte);
            return false;
        }
    }

    Log("SUCCESS: Vendor Name TLV validated. Length: %zu bytes. Value: %s", actualVendorNameLen, actualVendorName);
    return true;
}

bool DiagnosticValidator::ValidateVendorModel(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const char *expectedVendorModel = aNode.Get<NetworkDiagnostic::Server>().GetVendorModel();
    const char *actualVendorModel   = aTlv.mData.mVendorModel;
    size_t      actualVendorModelLen;

    actualVendorModelLen = strnlen(actualVendorModel, OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH + 1);

    if (actualVendorModelLen > OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH)
    {
        Log("ERROR: Vendor Model TLV length exceeds maximum. Length: %zu (max: %u)", actualVendorModelLen,
            OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH);
        return false;
    }

    if (actualVendorModelLen == 0)
    {
        Log("ERROR: Vendor Model TLV is empty");
        return false;
    }

    if (strncmp(expectedVendorModel, actualVendorModel, actualVendorModelLen) != 0)
    {
        Log("ERROR: Vendor Model mismatch. Expected: %s, Actual: %s", expectedVendorModel, actualVendorModel);
        return false;
    }

    for (size_t i = 0; i < actualVendorModelLen; i++)
    {
        unsigned char byte = static_cast<unsigned char>(actualVendorModel[i]);

        // Check for control characters
        if (byte < 0x20 && byte != 0x09 && byte != 0x0A && byte != 0x0D)
        {
            Log("ERROR: Vendor Model contains invalid control character at position %zu: 0x%02x", i, byte);
            return false;
        }
    }

    Log("SUCCESS: Vendor Model TLV validated. Length: %zu bytes. Value: %s", actualVendorModelLen, actualVendorModel);
    return true;
}

bool DiagnosticValidator::ValidateVendorSwVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const char *expectedVendorSwVersion = aNode.Get<NetworkDiagnostic::Server>().GetVendorSwVersion();
    const char *actualVendorSwVersion   = aTlv.mData.mVendorSwVersion;
    size_t      actualVendorSwVersionLen;

    actualVendorSwVersionLen =
        strnlen(actualVendorSwVersion, OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1);

    if (actualVendorSwVersionLen > OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH)
    {
        Log("ERROR: Vendor Software Version TLV length exceeds maximum. Length: %zu (max: %u)",
            actualVendorSwVersionLen, OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH);
        return false;
    }

    if (actualVendorSwVersionLen == 0)
    {
        Log("ERROR: Vendor Software Version TLV is empty");
        return false;
    }

    for (size_t i = 0; i < actualVendorSwVersionLen; i++)
    {
        unsigned char byte = static_cast<unsigned char>(actualVendorSwVersion[i]);

        // Check for control characters
        if (byte < 0x20 && byte != 0x09 && byte != 0x0A && byte != 0x0D)
        {
            Log("ERROR: Vendor Software Version contains invalid control character at position %zu: 0x%02x", i, byte);
            return false;
        }
    }

    Log("SUCCESS: Vendor Software Version TLV validated. Length: %zu bytes. Value: %s", actualVendorSwVersionLen,
        actualVendorSwVersion);
    return true;
}

bool DiagnosticValidator::ValidateVendorAppUrl(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const char *expectedVendorAppUrl = aNode.Get<NetworkDiagnostic::Server>().GetVendorAppUrl();
    const char *actualVendorAppUrl   = aTlv.mData.mVendorAppUrl;
    size_t      actualVendorAppUrlLen;

    actualVendorAppUrlLen = strnlen(actualVendorAppUrl, OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH + 1);

    if (actualVendorAppUrlLen > OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH)
    {
        Log("ERROR: Vendor Application URL TLV length exceeds maximum. Length: %zu (max: %u)", actualVendorAppUrlLen,
            OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH);
        return false;
    }

    if (actualVendorAppUrlLen == 0)
    {
        Log("WARNING: Vendor Application URL TLV is empty");
    }

    if (strncmp(expectedVendorAppUrl, actualVendorAppUrl, actualVendorAppUrlLen) != 0)
    {
        Log("ERROR: Vendor Application URL mismatch. Expected: %s, Actual: %s", expectedVendorAppUrl,
            actualVendorAppUrl);
        return false;
    }

    const char *schemeEnd = strstr(actualVendorAppUrl, "://");
    if (schemeEnd == nullptr || schemeEnd == actualVendorAppUrl)
    {
        Log("WARNING: Vendor Application URL does not contain valid URL scheme (://). Value: %s", actualVendorAppUrl);
    }

    for (size_t i = 0; i < actualVendorAppUrlLen; i++)
    {
        unsigned char byte = static_cast<unsigned char>(actualVendorAppUrl[i]);

        // Check for control characters
        if (byte < 0x20 && byte != 0x09 && byte != 0x0A && byte != 0x0D)
        {
            Log("ERROR: Vendor Application URL contains invalid control character at position %zu: 0x%02x", i, byte);
            return false;
        }
    }

    Log("SUCCESS: Vendor Application URL TLV validated. Length: %zu bytes. Value: %s", actualVendorAppUrlLen,
        actualVendorAppUrl);
    return true;
}

bool DiagnosticValidator::ValidateThreadStackVersion(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    const char *expectedVersion          = otGetVersionString();
    const char *actualThreadStackVersion = aTlv.mData.mThreadStackVersion;
    size_t      actualThreadStackVersionLen;

    actualThreadStackVersionLen =
        strnlen(actualThreadStackVersion, OT_EXT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH + 1);

    if (actualThreadStackVersionLen > OT_EXT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH)
    {
        Log("ERROR: Thread Stack Version TLV length exceeds maximum. Length: %zu (max: %u)",
            actualThreadStackVersionLen, OT_EXT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH);
        return false;
    }

    if (actualThreadStackVersionLen == 0)
    {
        Log("ERROR: Thread Stack Version TLV is empty");
        return false;
    }

    if (actualThreadStackVersionLen < 6)
    {
        Log("WARNING: Thread Stack Version string appears too short: %zu bytes. Value: %s", actualThreadStackVersionLen,
            actualThreadStackVersion);
    }

    if (strncmp(expectedVersion, actualThreadStackVersion, actualThreadStackVersionLen) != 0)
    {
        Log("ERROR: Thread Stack Version mismatch. Expected: %s, Actual: %s", expectedVersion,
            actualThreadStackVersion);
        return false;
    }

    bool hasValidVersionSeparators = false;
    for (size_t i = 0; i < actualThreadStackVersionLen; i++)
    {
        unsigned char byte = static_cast<unsigned char>(actualThreadStackVersion[i]);

        // Check for control characters
        if (byte < 0x20 && byte != 0x09 && byte != 0x0A && byte != 0x0D)
        {
            Log("ERROR: Thread Stack Version contains invalid control character at position %zu: 0x%02x", i, byte);
            return false;
        }

        // Look for version separators (dots, colons, hyphens, plus signs)
        if (actualThreadStackVersion[i] == '.' || actualThreadStackVersion[i] == ':' ||
            actualThreadStackVersion[i] == '-' || actualThreadStackVersion[i] == '+')
        {
            hasValidVersionSeparators = true;
        }
    }

    // Thread Stack Version should contain some structural separators
    if (!hasValidVersionSeparators)
    {
        Log("WARNING: Thread Stack Version does not contain expected version separators. Value: %s",
            actualThreadStackVersion);
    }

    Log("SUCCESS: Thread Stack Version TLV validated. Length: %zu bytes. Value: %s", actualThreadStackVersionLen,
        actualThreadStackVersion);
    return true;
}

bool DiagnosticValidator::ValidateIp6AddressList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint8_t count = aTlv.mData.mIp6AddressList.mCount;

    Log("DEBUG: All Unicast Addresses:");
    for (const Ip6::Netif::UnicastAddress &address : aNode.Get<ThreadNetif>().GetUnicastAddresses())
    {
        Log("  %s", address.GetAddress().ToString().AsCString());
    }

    Log("DEBUG: All Multicast Addresses:");
    for (const Ip6::Netif::MulticastAddress &address : aNode.Get<ThreadNetif>().GetMulticastAddresses())
    {
        Log("  %s", address.GetAddress().ToString().AsCString());
    }

    if (count >= 0)
    {
        Log("SUCCESS: OMR IPv6 Address List count is valid: %u address(es)", count);
        return true;
    }
    else
    {
        Log("ERROR: OMR IPv6 Address List count invalid");
        return false;
    }

    if (count > 16)
    {
        Log("WARNING: OMR IPv6 Address List count exceeds reasonable limit: %u", count);
    }

    return true;
}

bool DiagnosticValidator::ValidateIp6LinkLocalAddressList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint8_t count = aTlv.mData.mIp6LinkLocalAddressList.mCount;

    if (count == 1)
    {
        Log("SUCCESS: IPv6 Link-Local Address List count is valid: %u address (expected)", count);
        return true;
    }
    else if (count > 1)
    {
        Log("SUCCESS: IPv6 Link-Local Address List count is %u", count);
        return true;
    }
    else
    {
        Log("WARNING: IPv6 Link-Local Address List count is zero");
    }

    return true;
}

bool DiagnosticValidator::ValidateAlocList(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    uint8_t count = aTlv.mData.mAlocList.mCount;

    if (count == 0)
    {
        Log("WARNING: ALOC List count is zero (device may not have ALOC entries)");
        return true;
    }
    else
    {
        Log("SUCCESS: ALOC List count is valid: %u address(es)", count);
        return true;
    }

    if (count > 32)
    {
        Log("WARNING: ALOC List count exceeds reasonable limit: %u", count);
    }

    return true;
}

bool DiagnosticValidator::ValidateEui64(Node &aNode, otExtNetworkDiagnosticTlv &aTlv)
{
    // Nexus convention: child EUI64 is all zeros except the last byte equals the child index
    uint16_t rloc16 = aNode.Get<Mle::Mle>().GetRloc16();
    uint8_t  index  = static_cast<uint8_t>(rloc16 & 0xFF);

    uint8_t expected[OT_EXT_ADDRESS_SIZE] = {0};
    expected[OT_EXT_ADDRESS_SIZE - 1]     = index;

    if (memcmp(aTlv.mData.mEui64.m8, expected, sizeof(expected)) == 0)
    {
        Log("SUCCESS: EUI64 validated: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", aTlv.mData.mEui64.m8[0],
            aTlv.mData.mEui64.m8[1], aTlv.mData.mEui64.m8[2], aTlv.mData.mEui64.m8[3], aTlv.mData.mEui64.m8[4],
            aTlv.mData.mEui64.m8[5], aTlv.mData.mEui64.m8[6], aTlv.mData.mEui64.m8[7]);
        return true;
    }

    Log("ERROR: EUI64 mismatch. expected=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
        "actual=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
        expected[0], expected[1], expected[2], expected[3], expected[4], expected[5], expected[6], expected[7],
        aTlv.mData.mEui64.m8[0], aTlv.mData.mEui64.m8[1], aTlv.mData.mEui64.m8[2], aTlv.mData.mEui64.m8[3],
        aTlv.mData.mEui64.m8[4], aTlv.mData.mEui64.m8[5], aTlv.mData.mEui64.m8[6], aTlv.mData.mEui64.m8[7]);

    return false;
}

/* Tests that validate presence of TLVs */
void TestDiagnosticServerBasic(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &mtd1    = nexus.CreateNode();
    Node &mtd2    = nexus.CreateNode();
    Node &client  = nexus.CreateNode();

    Log("========================================================================================");
    Log("=== Test: Basic Diagnostic Server Functionality ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - 1 additional router");
    Log("  - 2 MTD children attached to leader");
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kMacAddress, kMleCounters, kIp6AddressList, kAlocList, kIp6LinkLocalAddressList");
    Log("- Child TLVs: kMacAddress, kMleCounters, kIp6AddressList, kAlocList, kIp6LinkLocalAddressList");
    Log("- Neighbor TLVs: kMacAddress");
    Log("Summary of tested TLV Ids: 0, 11, 12, 22, 26");
    Log("========================================================================================");

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    ExtNetworkDiagnostic::TlvSet hostSet;
    ExtNetworkDiagnostic::TlvSet childSet;
    ExtNetworkDiagnostic::TlvSet neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);

    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);

    nexus.AdvanceTime(0);

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
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
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
    constexpr size_t kNumChildren = 32;

    Log("========================================================================================");
    Log("=== Test: Large Child Table Diagnostic Server Stress Test ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - %u MTD children attached to leader", static_cast<unsigned>(kNumChildren));
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kMacAddress, kMleCounters, kIp6AddressList");
    Log("- Child TLVs: kMacAddress, kMleCounters, kIp6AddressList");
    Log("- Neighbor TLVs: kMacAddress");
    Log("Summary of tested TLV Ids: 0, 11, 26");
    Log("Purpose: Tests MTU batching with 32 children to verify message fragmentation works");
    Log("========================================================================================");
    Log("");

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &client = nexus.CreateNode();
    Node *children[kNumChildren];

    ExtNetworkDiagnostic::TlvSet hostSet;
    ExtNetworkDiagnostic::TlvSet childSet;
    ExtNetworkDiagnostic::TlvSet neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);

    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);

    for (size_t i = 0; i < kNumChildren; i++)
    {
        children[i] = &nexus.CreateNode();
    }

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    nexus.AdvanceTime(0);

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
    nexus.AdvanceTime(240 * 1000);
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

void TestDiagnosticServerAllAvailableTlvs(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &mtd1    = nexus.CreateNode();
    Node &mtd2    = nexus.CreateNode();
    Node &client  = nexus.CreateNode();

    Log("========================================================================================");
    Log("=== Test: All Available TLVs Diagnostic Server Test ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - 1 additional router");
    Log("  - 2 MTD children attached to leader");
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kMacAddress, kMode, kMlEid, kIp6AddressList, kAlocList, kThreadSpecVersion,");
    Log("             kThreadStackVersion, kVendorName, kVendorModel, kVendorAppUrl,");
    Log("             kIp6LinkLocalAddressList, kMleCounters");
    Log("- Child TLVs: kMacAddress, kMode, kTimeout, kLastHeard, kConnectionTime, kCsl, kMlEid,");
    Log("              kIp6AddressList, kAlocList, kThreadSpecVersion, kThreadStackVersion,");
    Log("              kVendorName, kVendorModel, kVendorAppUrl, kIp6LinkLocalAddressList, kMleCounters");
    Log("- Neighbor TLVs: kMacAddress, kLastHeard, kConnectionTime, kThreadSpecVersion");
    Log("Summary of tested TLV Ids: 0, 1, 2, 3, 4, 5, 10, 11, 12, 16, 17, 18, 19, 21, 22, 26");
    Log("Purpose: Tests comprehensive TLV set excluding only unavailable/redundant TLVs");
    Log("========================================================================================");
    Log("");

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    ExtNetworkDiagnostic::TlvSet hostSet;
    ExtNetworkDiagnostic::TlvSet childSet;
    ExtNetworkDiagnostic::TlvSet neighborSet;

    // Set only confirmed available host TLVs
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);

    // Set confirmed available child TLVs
    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kCsl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);

    // Set confirmed available neighbor TLVs
    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);

    nexus.AdvanceTime(0);

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
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Test available TLVs collection");

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    VerifyOrQuit(validator->ValidateRouter(leader));
    VerifyOrQuit(validator->ValidateRouter(router1));
    VerifyOrQuit(validator->ValidateChild(mtd1));
    VerifyOrQuit(validator->ValidateChild(mtd2));

    validator->Stop();
    nexus.AdvanceTime(30 * 60 * 1000); // 30min

    Log("---------------------------------------------------------------------------------------");

    delete validator;
}

void TestDiagnosticServerCoreTlvs(void)
{
    Core  nexus;
    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &mtd1    = nexus.CreateNode();
    Node &client  = nexus.CreateNode();

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Core TLVs Diagnostic Server Test ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - 1 additional router");
    Log("  - 1 MTD child attached to leader");
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates multiple core TLV combinations:");
    Log("Test Case 1: Basic Identification TLVs");
    Log("  - Host: kMacAddress, kMode, kThreadSpecVersion");
    Log("  - Child: kMacAddress, kMode, kTimeout");
    Log("  - Neighbor: kMacAddress, kThreadSpecVersion");
    Log("Test Case 2: Link Quality TLVs");
    Log("  - Child: kLastHeard, kConnectionTime");
    Log("  - Neighbor: kLastHeard, kConnectionTime");
    Log("Test Case 3: Statistics TLVs");
    Log("  - Host: kMleCounters");
    Log("  - Child: kMleCounters");
    Log("Summary of tested TLV Ids: 0, 1, 2, 3, 4, 16, 26");
    Log("Purpose: Tests individual core TLV categories with focused validation");
    Log("========================================================================================");
    Log("");

    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    mtd1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild());

    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Testing core TLVs");

    ExtNetworkDiagnostic::TlvSet hostSet, childSet, neighborSet;

    Log("Test Case 1: Basic Identification TLVs");
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);

    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(50 * 1000);
    VerifyOrQuit(validator->ValidateRouter(leader));
    VerifyOrQuit(validator->ValidateChild(mtd1));
    validator->Stop();
    nexus.AdvanceTime(10 * 1000);

    Log("Test Case 2: Network Addressing TLVs");
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);

    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(50 * 1000);
    VerifyOrQuit(validator->ValidateRouter(leader));
    VerifyOrQuit(validator->ValidateChild(mtd1));
    validator->Stop();
    nexus.AdvanceTime(10 * 1000);

    Log("Test Case 3: Performance Monitoring TLVs");
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);

    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(50 * 1000);
    VerifyOrQuit(validator->ValidateRouter(leader));
    VerifyOrQuit(validator->ValidateChild(mtd1));
    validator->Stop();
    nexus.AdvanceTime(10 * 1000);

    Log("---------------------------------------------------------------------------------------");
    delete validator;
}

void TestDiagnosticServerVendorTlvs(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &mtd1   = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Vendor TLVs Diagnostic Server Test ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - 1 MTD child attached to leader");
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kVendorName, kVendorModel, kVendorAppUrl, kThreadStackVersion");
    Log("- Child TLVs: kVendorName, kVendorModel, kThreadStackVersion");
    Log("- Neighbor TLVs: (none)");
    Log("Summary of tested TLV Ids: 17, 18, 19, 21");
    Log("Purpose: Tests vendor-specific string TLVs and stack version information");
    Log("========================================================================================");
    Log("");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);

    mtd1.Join(leader, Node::kAsMed);
    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);

    ExtNetworkDiagnostic::TlvSet hostSet, childSet, neighborSet;

    Log("Testing vendor information TLVs");
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);

    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);
    VerifyOrQuit(validator->ValidateRouter(leader));
    VerifyOrQuit(validator->ValidateChild(mtd1));
    validator->Stop();
    nexus.AdvanceTime(30 * 1000);

    Log("---------------------------------------------------------------------------------------");
    delete validator;
}

void TestDiagnosticServerComprehensiveStress(void)
{
    constexpr size_t   kNumChildren              = 32;
    constexpr uint32_t kStressIterations         = 5;
    constexpr uint32_t kNetworkStabilizationTime = 500 * 1000;
    constexpr uint32_t kTestIterationTime        = 200 * 1000;
    constexpr uint32_t kCooldownTime             = 30 * 60 * 1000;

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &client = nexus.CreateNode();
    Node *children[kNumChildren];

    for (size_t i = 0; i < kNumChildren; i++)
    {
        children[i] = &nexus.CreateNode();
    }

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    ExtNetworkDiagnostic::TlvSet hostSet;
    ExtNetworkDiagnostic::TlvSet childSet;
    ExtNetworkDiagnostic::TlvSet neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kCsl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kEui64);

    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);

    nexus.AdvanceTime(0);

    Log("========================================================================================");
    Log("=== Test: Comprehensive Stress Test with Maximum TLV Coverage ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - %u MTD children attached to leader", static_cast<unsigned>(kNumChildren));
    Log("  - 1 client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kMacAddress, kMode, kMlEid, kIp6AddressList, kAlocList, kThreadSpecVersion,");
    Log("             kThreadStackVersion, kVendorName, kVendorModel, kVendorSwVersion, kVendorAppUrl,");
    Log("             kIp6LinkLocalAddressList, kMleCounters");
    Log("- Child TLVs: kMacAddress, kMode, kTimeout, kLastHeard, kConnectionTime, kCsl, kMlEid,");
    Log("              kIp6AddressList, kAlocList, kThreadSpecVersion, kThreadStackVersion,");
    Log("              kVendorName, kVendorModel, kVendorSwVersion, kVendorAppUrl,");
    Log("              kIp6LinkLocalAddressList, kEui64, kMleCounters");
    Log("- Neighbor TLVs: kMacAddress, kLastHeard, kConnectionTime, kThreadSpecVersion");
    Log("Summary of tested TLV Ids: 0, 1, 2, 3, 4, 5, 10, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23, 26");
    Log("Stress parameters: %u iterations, %u children per iteration", static_cast<unsigned>(kStressIterations),
        static_cast<unsigned>(kNumChildren));
    Log("Purpose: Maximum stress test with all TLVs and 32 children over 5 iterations");
    Log("========================================================================================");
    Log("");

    Log("Phase 1: Network Formation");
    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Phase 2: Mass Child Joining (%u children)", static_cast<unsigned>(kNumChildren));
    for (size_t i = 0; i < kNumChildren; i++)
    {
        Log("Joining child %u/%u", static_cast<unsigned>(i + 1), static_cast<unsigned>(kNumChildren));
        children[i]->Join(leader, Node::kAsMed);
        nexus.AdvanceTime(60 * 1000);
        VerifyOrQuit(children[i]->Get<Mle::Mle>().IsChild());
    }

    Log("Phase 3: Client Router Formation");
    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kNetworkStabilizationTime);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    Log("Phase 4: Comprehensive Stress Testing (%u iterations)", static_cast<unsigned>(kStressIterations));

    for (uint32_t iteration = 0; iteration < kStressIterations; iteration++)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Stress Test Iteration %u/%u", static_cast<unsigned>(iteration + 1),
            static_cast<unsigned>(kStressIterations));
        Log("Testing comprehensive TLV collection with %u children", static_cast<unsigned>(kNumChildren));

        validator->Start(hostSet, childSet, neighborSet);
        nexus.AdvanceTime(kTestIterationTime);

        Log("Validating leader with comprehensive TLV set");
        VerifyOrQuit(validator->ValidateRouter(leader));

        Log("Validating %u children with comprehensive TLV set", static_cast<unsigned>(kNumChildren));
        for (size_t i = 0; i < kNumChildren; i++)
        {
            VerifyOrQuit(validator->ValidateChild(*children[i]));

            Log("Validated %u child/ren", static_cast<unsigned>(i + 1));
        }

        validator->Stop();

        Log("Iteration %u completed successfully", static_cast<unsigned>(iteration + 1));

        if (iteration < kStressIterations - 1)
        {
            Log("Cooldown period (%u minutes)", static_cast<unsigned>(kCooldownTime / (60 * 1000)));
            nexus.AdvanceTime(kCooldownTime);
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 5: Final Network State Validation");

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    uint32_t validChildren = 0;
    for (size_t i = 0; i < kNumChildren; i++)
    {
        if (children[i]->Get<Mle::Mle>().IsChild())
        {
            validChildren++;
        }
    }

    Log("Final validation: %u/%u children still connected", static_cast<unsigned>(validChildren),
        static_cast<unsigned>(kNumChildren));

    // We expect at least 90% of children to remain connected after stress testing
    VerifyOrQuit(validChildren >= (kNumChildren * 9 / 10));

    Log("---------------------------------------------------------------------------------------");
    Log("Comprehensive Diagnostic Server Stress Test PASSED");
    Log("Successfully tested %u children with comprehensive TLV sets over %u iterations",
        static_cast<unsigned>(kNumChildren), static_cast<unsigned>(kStressIterations));
    Log("---------------------------------------------------------------------------------------");

    delete validator;
}

void TestDiagnosticServerMultiRouterWithFtdChildren(void)
{
    constexpr size_t   kNumLeaderChildren        = 32;
    constexpr size_t   kNumAdditionalRouters     = 30;
    constexpr size_t   kNumFtdChildren           = 32;
    constexpr uint32_t kStressIterations         = 3;
    constexpr uint32_t kNetworkStabilizationTime = 500 * 1000;
    constexpr uint32_t kTestIterationTime        = 800 * 1000;
    constexpr uint32_t kCooldownTime             = 10 * 60 * 1000;

    Core nexus;

    // Leader router (diag server running on it)
    Node &leader = nexus.CreateNode();
    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    leader.Get<Mle::Mle>().SetRouterDowngradeThreshold(32);

    // Client router (diag client running on it)
    Node &client = nexus.CreateNode();

    // MTD children attached to leader
    Node *leaderChildren[kNumLeaderChildren];
    for (size_t i = 0; i < kNumLeaderChildren; i++)
    {
        leaderChildren[i] = &nexus.CreateNode();
    }

    // Additional routers
    Node *additionalRouters[kNumAdditionalRouters];
    for (size_t i = 0; i < kNumAdditionalRouters; i++)
    {
        additionalRouters[i] = &nexus.CreateNode();
    }

    // FTD children attached to first additional router
    Node *ftdChildren[kNumFtdChildren];
    for (size_t i = 0; i < kNumFtdChildren; i++)
    {
        ftdChildren[i] = &nexus.CreateNode();
    }

    DiagnosticValidator *validator = new DiagnosticValidator(client);

    ExtNetworkDiagnostic::TlvSet hostSet;
    ExtNetworkDiagnostic::TlvSet childSet;
    ExtNetworkDiagnostic::TlvSet neighborSet;

    // Host TLVs
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kRoute64);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kEui64);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacCounters);

    // Child TLVs (MTD and FTD children)
    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kCsl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLinkMarginIn);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kEui64);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacCounters);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMacLinkErrorRatesIn);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMleCounters);

    // Neighbor TLVs
    neighborSet.Clear();
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    neighborSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);

    nexus.AdvanceTime(0);

    Log("========================================================================================");
    Log("=== Test: Multi-Router Diagnostic Server Stress Test with MTD and FTD Children ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - Client router (diag client)");
    Log("  - %u MTD children attached to leader", static_cast<unsigned>(kNumLeaderChildren));
    Log("  - %u additional routers", static_cast<unsigned>(kNumAdditionalRouters));
    Log("  - %u FTD children attached to additional router 0", static_cast<unsigned>(kNumFtdChildren));
    Log("---------------------------------------------------------------------------------------");
    Log("The test requests the following TLVs:");
    Log("- Host TLVs: kMacAddress, kMode, kRoute64, kMlEid, kIp6AddressList, kAlocList, kThreadSpecVersion, "
        "kThreadStackVersion, "
        "kVendorName,");
    Log("             kVendorModel, kVendorSwVersion, kVendorAppUrl, kIp6LinkLocalAddressList, kMleCounters, kEui64, "
        "kMacCounters");
    Log("- Child TLVs: kMacAddress, kMode, kTimeout, kLastHeard, kConnectionTime, kCsl, kMlEid, kIp6AddressList, "
        "kAlocList,");
    Log("             kThreadSpecVersion, kThreadStackVersion, kVendorName, kVendorModel, kVendorSwVersion, "
        "kVendorAppUrl, kIp6LinkLocalAddressList, kEui64, kMacCounters, kMacLinkErrorRatesIn, kMleCounters");
    Log("- Neighbor TLVs: kMacAddress, kLastHeard, kConnectionTime, kLinkMarginIn, kThreadSpecVersion");
    Log("Summary of tested TLV Ids: 0, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26");
    Log("---------------------------------------------------------------------------------------");
    Log("Phase 1: Network Formation - Leader");
    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    Log("Leader formed: RLOC16=%04X", leader.Get<Mle::Mle>().GetRloc16());

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 2: Join MTD Children to Leader (%u children)", static_cast<unsigned>(kNumLeaderChildren));
    for (size_t i = 0; i < kNumLeaderChildren; i++)
    {
        leaderChildren[i]->Join(leader, Node::kAsMed);
        nexus.AdvanceTime(5 * 1000);
        VerifyOrQuit(leaderChildren[i]->Get<Mle::Mle>().IsChild());
        if ((i + 1) % 8 == 0)
        {
            Log("  Joined %u/%u MTD children to leader", static_cast<unsigned>(i + 1),
                static_cast<unsigned>(kNumLeaderChildren));
        }
    }
    Log("All %u MTD children joined to leader", static_cast<unsigned>(kNumLeaderChildren));

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 3: Join Additional Routers (%u routers)", static_cast<unsigned>(kNumAdditionalRouters));
    for (size_t i = 0; i < kNumAdditionalRouters; i++)
    {
        additionalRouters[i]->Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
        additionalRouters[i]->Get<Mle::Mle>().SetRouterDowngradeThreshold(32);
        additionalRouters[i]->Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(240 * 1000);
        VerifyOrQuit(additionalRouters[i]->Get<Mle::Mle>().IsRouter());
        Log("  Additional router %u became router: RLOC16=%04X", static_cast<unsigned>(i),
            additionalRouters[i]->Get<Mle::Mle>().GetRloc16());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 4: Join FTD Children to Additional Router 0 (%u children)", static_cast<unsigned>(kNumFtdChildren));
    for (size_t i = 0; i < kNumFtdChildren; i++)
    {
        ftdChildren[i]->Join(*additionalRouters[0], Node::kAsFed);
        nexus.AdvanceTime(5 * 1000);
        VerifyOrQuit(ftdChildren[i]->Get<Mle::Mle>().IsChild());
        if ((i + 1) % 8 == 0)
        {
            Log("  Joined %u/%u FTD children to additional router 0", static_cast<unsigned>(i + 1),
                static_cast<unsigned>(kNumFtdChildren));
        }
    }
    Log("All %u FTD children joined to additional router 0", static_cast<unsigned>(kNumFtdChildren));

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 5: Join Client Router (diag client)");
    client.Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    client.Get<Mle::Mle>().SetRouterDowngradeThreshold(32);
    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kNetworkStabilizationTime);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());
    Log("Client router joined: RLOC16=%04X", client.Get<Mle::Mle>().GetRloc16());

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 6: Network Stabilization");
    nexus.AdvanceTime(kNetworkStabilizationTime);

    Log("Network state verification:");
    Log("  Leader: %s (RLOC16=%04X)", leader.Get<Mle::Mle>().IsLeader() ? "LEADER" : "NOT LEADER",
        leader.Get<Mle::Mle>().GetRloc16());
    Log("  Client: %s (RLOC16=%04X)", client.Get<Mle::Mle>().IsRouter() ? "ROUTER" : "NOT ROUTER",
        client.Get<Mle::Mle>().GetRloc16());

    uint32_t validLeaderChildren = 0;
    for (size_t i = 0; i < kNumLeaderChildren; i++)
    {
        if (leaderChildren[i]->Get<Mle::Mle>().IsChild())
        {
            validLeaderChildren++;
        }
    }
    Log("  Leader MTD children: %u/%u connected", static_cast<unsigned>(validLeaderChildren),
        static_cast<unsigned>(kNumLeaderChildren));

    uint32_t validAdditionalRouters = 0;
    for (size_t i = 0; i < kNumAdditionalRouters; i++)
    {
        if (additionalRouters[i]->Get<Mle::Mle>().IsRouter())
        {
            validAdditionalRouters++;
        }
    }
    Log("  Additional routers: %u/%u are routers", static_cast<unsigned>(validAdditionalRouters),
        static_cast<unsigned>(kNumAdditionalRouters));

    uint32_t validFtdChildren = 0;
    for (size_t i = 0; i < kNumFtdChildren; i++)
    {
        if (ftdChildren[i]->Get<Mle::Mle>().IsChild())
        {
            validFtdChildren++;
        }
    }
    Log("  FTD children (on additional router 0): %u/%u connected", static_cast<unsigned>(validFtdChildren),
        static_cast<unsigned>(kNumFtdChildren));

    uint32_t totalRouters = leader.Get<RouterTable>().GetActiveRouterCount();
    Log("Total active routers in network: %u (expected: 32)", static_cast<unsigned>(totalRouters));
    VerifyOrQuit(totalRouters == 32, "Expected 32 routers (1 leader + 1 client + 30 additional)");

    Log("Router (leader) RLOC16=%04X Neighbor Table", leader.Get<Mle::Mle>().GetRloc16());

    uint8_t neighborCount = 0;
    for (Router &router : leader.Get<RouterTable>())
    {
        if (router.IsStateValid())
        {
            Log("  Neighbor router ID=%u RLOC=%04X", router.GetRouterId(), router.GetRloc16());
            neighborCount++;
        }
    }

    Log("Total neighbors: %u", neighborCount);

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 7: Diagnostic Server Stress Testing (%u iterations)", static_cast<unsigned>(kStressIterations));

    for (uint32_t iteration = 0; iteration < kStressIterations; iteration++)
    {
        Log("=======================================================================================");
        Log("Stress Test Iteration %u/%u", static_cast<unsigned>(iteration + 1),
            static_cast<unsigned>(kStressIterations));
        Log("=======================================================================================");

        validator->Start(hostSet, childSet, neighborSet);
        nexus.AdvanceTime(kTestIterationTime);

        Log("---------------------------------------------------------------------------------------");
        Log("Validating Leader Router");
        VerifyOrQuit(validator->ValidateRouter(leader));

        Log("---------------------------------------------------------------------------------------");
        Log("Validating Additional Routers (%u routers)", static_cast<unsigned>(kNumAdditionalRouters));
        for (size_t i = 0; i < kNumAdditionalRouters; i++)
        {
            if (additionalRouters[i]->Get<Mle::Mle>().IsRouter())
            {
                VerifyOrQuit(validator->ValidateRouter(*additionalRouters[i]));
                Log("  Additional router %u validated", static_cast<unsigned>(i));
            }
        }

        Log("---------------------------------------------------------------------------------------");
        Log("Validating Leader's MTD Children (%u children)", static_cast<unsigned>(kNumLeaderChildren));
        uint32_t validatedLeaderChildren = 0;
        for (size_t i = 0; i < kNumLeaderChildren; i++)
        {
            if (leaderChildren[i]->Get<Mle::Mle>().IsChild())
            {
                VerifyOrQuit(validator->ValidateChild(*leaderChildren[i]));
                validatedLeaderChildren++;
            }
        }
        Log("  Validated %u/%u MTD children", static_cast<unsigned>(validatedLeaderChildren),
            static_cast<unsigned>(kNumLeaderChildren));

        Log("---------------------------------------------------------------------------------------");
        Log("Validating Additional Router 0's FTD Children (%u children)", static_cast<unsigned>(kNumFtdChildren));
        uint32_t validatedFtdChildren = 0;
        for (size_t i = 0; i < kNumFtdChildren; i++)
        {
            if (ftdChildren[i]->Get<Mle::Mle>().IsChild())
            {
                VerifyOrQuit(validator->ValidateChild(*ftdChildren[i]));
                validatedFtdChildren++;
            }
        }
        Log("  Validated %u/%u FTD children", static_cast<unsigned>(validatedFtdChildren),
            static_cast<unsigned>(kNumFtdChildren));

        validator->Stop();

        Log("Iteration %u completed successfully", static_cast<unsigned>(iteration + 1));

        if (iteration < kStressIterations - 1)
        {
            Log("Cooldown period (%u minutes)", static_cast<unsigned>(kCooldownTime / (60 * 1000)));
            nexus.AdvanceTime(kCooldownTime);
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Phase 8: Final Network State Validation");

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    validLeaderChildren = 0;
    for (size_t i = 0; i < kNumLeaderChildren; i++)
    {
        if (leaderChildren[i]->Get<Mle::Mle>().IsChild())
        {
            validLeaderChildren++;
        }
    }

    validAdditionalRouters = 0;
    for (size_t i = 0; i < kNumAdditionalRouters; i++)
    {
        if (additionalRouters[i]->Get<Mle::Mle>().IsRouter())
        {
            validAdditionalRouters++;
        }
    }

    validFtdChildren = 0;
    for (size_t i = 0; i < kNumFtdChildren; i++)
    {
        if (ftdChildren[i]->Get<Mle::Mle>().IsChild())
        {
            validFtdChildren++;
        }
    }

    Log("========================================================================================");
    Log("Multi-Router Diagnostic Server Stress Test PASSED");
    Log("Successfully tested:");
    Log("  - 1 leader router with %u MTD children", static_cast<unsigned>(kNumLeaderChildren));
    Log("  - 1 client router (diag client)");
    Log("  - %u additional routers", static_cast<unsigned>(kNumAdditionalRouters));
    Log("  - %u FTD children on additional router 0", static_cast<unsigned>(kNumFtdChildren));
    Log("  - %u stress iterations", static_cast<unsigned>(kStressIterations));
    Log("========================================================================================");

    delete validator;
}

/* Tests that validate value of TLVs */
void TestDiagnosticValidateCoreTlvs(void)
{
    Core                 nexus;
    Node                &router1   = nexus.CreateNode();
    Node                &client    = nexus.CreateNode();
    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Core TLV Value Validation ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Router1 (diag server)");
    Log("  - Client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates actual TLV values:");
    Log("- Host TLVs: kMacAddress, kMode");
    Log("Summary of validated TLV Ids: 0, 1");
    Log("Purpose: Validates that reported TLV values match actual device state");
    Log("========================================================================================");
    Log("");

    router1.Form();
    nexus.AdvanceTime(13 * 1000);
    client.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);

    ExtNetworkDiagnostic::TlvSet hostSet;
    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);

    ExtNetworkDiagnostic::TlvSet childSet, neighborSet;
    childSet.Clear();
    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Host (Router1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          rloc16   = router1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           routerId = Mle::RouterIdFromRloc16(rloc16);
    DiagnosticValidator::RouterEntry *entry    = &validator->mRouters[routerId];

    VerifyOrQuit(entry->mValid, "Router1 entry not valid");
    VerifyOrQuit(entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMacAddress), "kMacAddress not collected");
    VerifyOrQuit(entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMode), "kMode not collected");

    VerifyOrQuit(validator->ValidateTlvValue(router1, *entry, ExtNetworkDiagnostic::Tlv::kMacAddress),
                 "kMacAddress validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *entry, ExtNetworkDiagnostic::Tlv::kMode),
                 "kMode validation failed");

    validator->Stop();
    delete validator;
}

void TestDiagnosticValidateChildTlvs(void)
{
    Core                 nexus;
    Node                &leader    = nexus.CreateNode();
    Node                &mtd1      = nexus.CreateNode();
    Node                &client    = nexus.CreateNode();
    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Child TLV Value Validation ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Leader router (diag server)");
    Log("  - 1 MTD child attached to leader");
    Log("  - Client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates actual child TLV values:");
    Log("- Child TLVs: kTimeout, kLastHeard, kConnectionTime, kCsl, kMlEid");
    Log("Summary of validated TLV Ids: 2, 3, 4, 5, 10");
    Log("Purpose: Validates that reported child TLV values match actual child state");
    Log("========================================================================================");
    Log("");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader(), "Leader not formed");

    mtd1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild(), "MTD1 not joined as child");

    client.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter(), "Client not became router");

    ExtNetworkDiagnostic::TlvSet hostSet, neighborSet;
    hostSet.Clear();
    neighborSet.Clear();

    ExtNetworkDiagnostic::TlvSet childSet;
    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kCsl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          childRloc16 = mtd1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           routerId    = Mle::RouterIdFromRloc16(childRloc16);
    DiagnosticValidator::RouterEntry *routerEntry = &validator->mRouters[routerId];

    VerifyOrQuit(routerEntry->mValid, "Router entry not valid");

    DiagnosticValidator::ChildEntry *childEntry = routerEntry->GetChild(childRloc16);
    VerifyOrQuit(childEntry != nullptr, "Child entry is null");

    VerifyOrQuit(childEntry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kTimeout), "kTimeout not collected");
    VerifyOrQuit(childEntry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kLastHeard), "kLastHeard not collected");
    VerifyOrQuit(childEntry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kConnectionTime),
                 "kConnectionTime not collected");
    VerifyOrQuit(childEntry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kCsl), "kCsl not collected");
    VerifyOrQuit(childEntry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kMlEid), "kMlEid not collected");

    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *childEntry, ExtNetworkDiagnostic::Tlv::kTimeout),
                 "kTimeout validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *childEntry, ExtNetworkDiagnostic::Tlv::kLastHeard),
                 "kLastHeard validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *childEntry, ExtNetworkDiagnostic::Tlv::kConnectionTime),
                 "kConnectionTime validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *childEntry, ExtNetworkDiagnostic::Tlv::kCsl),
                 "kCsl validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *childEntry, ExtNetworkDiagnostic::Tlv::kMlEid),
                 "kMlEid validation failed");

    validator->Stop();
    delete validator;
}

void TestDiagnosticValidateVersionAndVendorTlvs(void)
{
    Core                 nexus;
    Node                &router1   = nexus.CreateNode();
    Node                &mtd1      = nexus.CreateNode();
    Node                &mtd2      = nexus.CreateNode();
    Node                &router2   = nexus.CreateNode();
    Node                &client    = nexus.CreateNode();
    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Version and Vendor TLV Value Validation ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Router1 (diag server)");
    Log("  - Router2 (additional router)");
    Log("  - 2 MTD children attached to router1");
    Log("  - Client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates actual version and vendor TLV values:");
    Log("- Host TLVs: kThreadSpecVersion, kThreadStackVersion, kVendorName, kVendorModel,");
    Log("             kVendorSwVersion, kVendorAppUrl");
    Log("- Child TLVs: kThreadSpecVersion, kThreadStackVersion, kVendorName, kVendorModel,");
    Log("              kVendorSwVersion, kVendorAppUrl");
    Log("Summary of validated TLV Ids: 16, 17, 18, 19, 20, 21");
    Log("Purpose: Validates version and vendor information string TLVs");
    Log("========================================================================================");
    Log("");

    router1.Form();
    nexus.AdvanceTime(13 * 1000);

    mtd1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild(), "MTD1 failed to join");

    mtd2.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd2.Get<Mle::Mle>().IsChild(), "MTD2 failed to join");

    router2.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter(), "Router2 failed to become router");

    client.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter(), "Client failed to become router");

    ExtNetworkDiagnostic::TlvSet hostSet, childSet, neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);

    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Host (Router1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          router1Rloc16 = router1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           router1Id     = Mle::RouterIdFromRloc16(router1Rloc16);
    DiagnosticValidator::RouterEntry *router1Entry  = &validator->mRouters[router1Id];

    VerifyOrQuit(router1Entry != nullptr, "Router1 entry is null");
    VerifyOrQuit(router1Entry->mValid, "Router1 entry not valid");

    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kThreadSpecVersion),
                 "Host kThreadSpecVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kThreadStackVersion),
                 "Host kThreadStackVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorName),
                 "Host kVendorName validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorModel),
                 "Host kVendorModel validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorSwVersion),
                 "Host kVendorSwVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorAppUrl),
                 "Host kVendorAppUrl validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd1Rloc16      = mtd1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd1ParentId    = Mle::RouterIdFromRloc16(mtd1Rloc16);
    DiagnosticValidator::RouterEntry *mtd1ParentEntry = &validator->mRouters[mtd1ParentId];
    DiagnosticValidator::ChildEntry  *mtd1Entry       = mtd1ParentEntry->GetChild(mtd1Rloc16);

    VerifyOrQuit(mtd1Entry != nullptr, "MTD1 entry is null");

    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kThreadSpecVersion),
                 "MTD1 kThreadSpecVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kVendorName),
                 "MTD1 kVendorName validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD2) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd2Rloc16      = mtd2.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd2ParentId    = Mle::RouterIdFromRloc16(mtd2Rloc16);
    DiagnosticValidator::RouterEntry *mtd2ParentEntry = &validator->mRouters[mtd2ParentId];
    DiagnosticValidator::ChildEntry  *mtd2Entry       = mtd2ParentEntry->GetChild(mtd2Rloc16);

    VerifyOrQuit(mtd2Entry != nullptr, "MTD2 entry is null");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kThreadSpecVersion),
                 "MTD2 kThreadSpecVersion validation failed");

    validator->Stop();
    delete validator;
}

void TestDiagnosticValidateAddressTlvs(void)
{
    Core                 nexus;
    Node                &router1   = nexus.CreateNode();
    Node                &mtd1      = nexus.CreateNode();
    Node                &mtd2      = nexus.CreateNode();
    Node                &router2   = nexus.CreateNode();
    Node                &client    = nexus.CreateNode();
    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Address TLV Value Validation ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Router1 (diag server) with off-mesh address fd12:3456:789a:1::1");
    Log("  - Router2 (additional router)");
    Log("  - 2 MTD children attached to router1");
    Log("  - Client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates actual address TLV values:");
    Log("- Host TLVs: kIp6AddressList, kAlocList, kIp6LinkLocalAddressList");
    Log("- Child TLVs: kIp6AddressList, kAlocList, kIp6LinkLocalAddressList");
    Log("Summary of validated TLV Ids: 11, 12, 22");
    Log("Purpose: Validates IPv6 address, ALOC, and link-local address list TLVs");
    Log("========================================================================================");
    Log("");

    router1.Form();
    nexus.AdvanceTime(13 * 1000);

    Ip6::Netif::UnicastAddress offMeshAddr;
    offMeshAddr.GetAddress().FromString("fd12:3456:789a:1::1");
    router1.Get<ThreadNetif>().AddUnicastAddress(offMeshAddr);

    mtd1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild(), "MTD1 failed to join");

    mtd2.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd2.Get<Mle::Mle>().IsChild(), "MTD2 failed to join");

    router2.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter(), "Router2 failed to become router");

    client.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter(), "Client failed to become router");

    ExtNetworkDiagnostic::TlvSet hostSet, childSet, neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);

    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Host (Router1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          router1Rloc16 = router1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           router1Id     = Mle::RouterIdFromRloc16(router1Rloc16);
    DiagnosticValidator::RouterEntry *router1Entry  = &validator->mRouters[router1Id];

    VerifyOrQuit(router1Entry != nullptr, "Router1 entry is null");
    VerifyOrQuit(router1Entry->mValid, "Router1 entry not valid");
    VerifyOrQuit(router1Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "Host kIp6AddressList not collected");
    VerifyOrQuit(router1Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "Host kIp6LinkLocalAddressList not collected");
    VerifyOrQuit(router1Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kAlocList), "Host kAlocList not collected");

    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "Host kIp6AddressList validation failed");
    VerifyOrQuit(
        validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
        "Host kIp6LinkLocalAddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kAlocList),
                 "Host kAlocList validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd1Rloc16      = mtd1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd1ParentId    = Mle::RouterIdFromRloc16(mtd1Rloc16);
    DiagnosticValidator::RouterEntry *mtd1ParentEntry = &validator->mRouters[mtd1ParentId];
    DiagnosticValidator::ChildEntry  *mtd1Entry       = mtd1ParentEntry->GetChild(mtd1Rloc16);

    VerifyOrQuit(mtd1Entry != nullptr, "MTD1 entry is null");
    VerifyOrQuit(mtd1Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "MTD1 kIp6AddressList not collected");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "MTD1 kIp6AddressList validation failed");
    VerifyOrQuit(mtd1Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "MTD1 kIp6LinkLocalAddressList not collected");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "MTD1 kIp6LinkLocalAddressList validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD2) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd2Rloc16      = mtd2.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd2ParentId    = Mle::RouterIdFromRloc16(mtd2Rloc16);
    DiagnosticValidator::RouterEntry *mtd2ParentEntry = &validator->mRouters[mtd2ParentId];
    DiagnosticValidator::ChildEntry  *mtd2Entry       = mtd2ParentEntry->GetChild(mtd2Rloc16);

    VerifyOrQuit(mtd2Entry != nullptr, "MTD2 entry is null");
    VerifyOrQuit(mtd2Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "MTD2 kIp6AddressList not collected");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "MTD2 kIp6AddressList validation failed");
    VerifyOrQuit(mtd2Entry->mValidTlvs.IsSet(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "MTD2 kIp6LinkLocalAddressList not collected");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "MTD2 kIp6LinkLocalAddressList validation failed");

    validator->Stop();
    delete validator;
}

void TestDiagnosticValidateComprehensiveTlvs(void)
{
    Core                 nexus;
    Node                &router1   = nexus.CreateNode();
    Node                &mtd1      = nexus.CreateNode();
    Node                &mtd2      = nexus.CreateNode();
    Node                &router2   = nexus.CreateNode();
    Node                &client    = nexus.CreateNode();
    DiagnosticValidator *validator = new DiagnosticValidator(client);

    Log("========================================================================================");
    Log("=== Test: Comprehensive TLV Value Validation ===");
    Log("========================================================================================");
    Log("Network topology:");
    Log("  - Router1 (diag server)");
    Log("  - Router2 (additional router)");
    Log("  - 2 MTD children attached to router1");
    Log("  - Client router (diag client)");
    Log("---------------------------------------------------------------------------------------");
    Log("The test validates comprehensive TLV values:");
    Log("- Host TLVs: kMacAddress, kMode, kThreadSpecVersion, kThreadStackVersion, kVendorName,");
    Log("             kVendorModel, kVendorAppUrl, kVendorSwVersion, kIp6AddressList, kAlocList,");
    Log("             kIp6LinkLocalAddressList");
    Log("- Child TLVs: kTimeout, kLastHeard, kConnectionTime, kMlEid, kThreadSpecVersion,");
    Log("              kVendorName, kIp6AddressList, kCsl, kAlocList, kIp6LinkLocalAddressList, kEui64");
    Log("- Neighbor TLVs: (none)");
    Log("Summary of validated TLV Ids: 0, 1, 2, 3, 4, 5, 10, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23");
    Log("Purpose: Comprehensive validation of all major TLV categories in one test");
    Log("========================================================================================");
    Log("");

    router1.Form();
    nexus.AdvanceTime(13 * 1000);

    mtd1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd1.Get<Mle::Mle>().IsChild(), "MTD1 failed to join");

    mtd2.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(mtd2.Get<Mle::Mle>().IsChild(), "MTD2 failed to join");

    router2.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter(), "Router2 failed to become router");

    client.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(240 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter(), "Client failed to become router");

    // Request multiple TLV types for comprehensive validation
    ExtNetworkDiagnostic::TlvSet hostSet, childSet, neighborSet;

    hostSet.Clear();
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMacAddress);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kMode);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kThreadStackVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorModel);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorAppUrl);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kVendorSwVersion);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    hostSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);

    childSet.Clear();
    childSet.Set(ExtNetworkDiagnostic::Tlv::kTimeout);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kLastHeard);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kConnectionTime);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kMlEid);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kThreadSpecVersion);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kVendorName);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6AddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kCsl);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kAlocList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList);
    childSet.Set(ExtNetworkDiagnostic::Tlv::kEui64);

    neighborSet.Clear();

    validator->Start(hostSet, childSet, neighborSet);
    nexus.AdvanceTime(100 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Host (Router1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          router1Rloc16 = router1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           router1Id     = Mle::RouterIdFromRloc16(router1Rloc16);
    DiagnosticValidator::RouterEntry *router1Entry  = &validator->mRouters[router1Id];

    VerifyOrQuit(router1Entry->mValid, "Router1 entry not valid");

    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kMacAddress),
                 "kMacAddress validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kMode),
                 "kMode validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kThreadSpecVersion),
                 "kThreadSpecVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kThreadStackVersion),
                 "kThreadStackVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorName),
                 "kVendorName validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorModel),
                 "kVendorModel validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorAppUrl),
                 "kVendorAppUrl validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kVendorSwVersion),
                 "kVendorSwVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "kIp6AddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kAlocList),
                 "kAlocList validation failed");
    VerifyOrQuit(
        validator->ValidateTlvValue(router1, *router1Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
        "kIp6LinkLocalAddressList validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD1) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd1Rloc16      = mtd1.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd1ParentId    = Mle::RouterIdFromRloc16(mtd1Rloc16);
    DiagnosticValidator::RouterEntry *mtd1ParentEntry = &validator->mRouters[mtd1ParentId];
    DiagnosticValidator::ChildEntry  *mtd1Entry       = mtd1ParentEntry->GetChild(mtd1Rloc16);

    VerifyOrQuit(mtd1Entry != nullptr, "MTD1 entry is null");

    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kTimeout),
                 "kTimeout validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kLastHeard),
                 "kLastHeard validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kConnectionTime),
                 "kConnectionTime validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kMlEid),
                 "kMlEid validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kThreadSpecVersion),
                 "kThreadSpecVersion validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kVendorName),
                 "kVendorName validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "kIp6AddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kCsl),
                 "kCsl validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kAlocList),
                 "kAlocList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "kIp6LinkLocalAddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd1, *mtd1Entry, ExtNetworkDiagnostic::Tlv::kEui64),
                 "kEui64 validation failed");

    Log("---------------------------------------------------------------------------------------");
    Log("Validating Child (MTD2) TLVs");
    Log("---------------------------------------------------------------------------------------");

    uint16_t                          mtd2Rloc16      = mtd2.Get<Mle::Mle>().GetRloc16();
    uint8_t                           mtd2ParentId    = Mle::RouterIdFromRloc16(mtd2Rloc16);
    DiagnosticValidator::RouterEntry *mtd2ParentEntry = &validator->mRouters[mtd2ParentId];
    DiagnosticValidator::ChildEntry  *mtd2Entry       = mtd2ParentEntry->GetChild(mtd2Rloc16);

    VerifyOrQuit(mtd2Entry != nullptr, "MTD2 entry is null");

    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kTimeout),
                 "kTimeout validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kMlEid),
                 "kMlEid validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kIp6AddressList),
                 "kIp6AddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kAlocList),
                 "kAlocList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kIp6LinkLocalAddressList),
                 "kIp6LinkLocalAddressList validation failed");
    VerifyOrQuit(validator->ValidateTlvValue(mtd2, *mtd2Entry, ExtNetworkDiagnostic::Tlv::kEui64),
                 "kEui64 validation failed");

    validator->Stop();
    delete validator;
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDiagnosticServerBasic();
    ot::Nexus::TestDiagnosticServerLargeChildTable();
    ot::Nexus::TestDiagnosticServerAllAvailableTlvs();
    ot::Nexus::TestDiagnosticServerCoreTlvs();
    ot::Nexus::TestDiagnosticServerVendorTlvs();
    ot::Nexus::TestDiagnosticServerComprehensiveStress();
    ot::Nexus::TestDiagnosticServerMultiRouterWithFtdChildren();
    ot::Nexus::TestDiagnosticValidateCoreTlvs();
    ot::Nexus::TestDiagnosticValidateChildTlvs();
    ot::Nexus::TestDiagnosticValidateVersionAndVendorTlvs();
    ot::Nexus::TestDiagnosticValidateAddressTlvs();
    ot::Nexus::TestDiagnosticValidateComprehensiveTlvs();
    return 0;
}
