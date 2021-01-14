/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements source address match controller.
 */

#include "src_match_controller.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "mac/mac_types.hpp"
#include "radio/radio.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/thread_netif.hpp"
#include "thread/topology.hpp"

namespace ot {

SourceMatchController::SourceMatchController(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
{
    ClearTable();
}

void SourceMatchController::IncrementMessageCount(SedCapableNeighbor &aSedCapableNeighbor)
{
    if (aSedCapableNeighbor.GetIndirectMessageCount() == 0)
    {
        AddEntry(aSedCapableNeighbor);
    }

    aSedCapableNeighbor.IncrementIndirectMessageCount();
}

void SourceMatchController::DecrementMessageCount(SedCapableNeighbor &aSedCapableNeighbor)
{
    if (aSedCapableNeighbor.GetIndirectMessageCount() == 0)
    {
        otLogWarnMac("DecrementMessageCount(child 0x%04x) called when already at zero count.",
                     aSedCapableNeighbor.GetRloc16());
        ExitNow();
    }

    aSedCapableNeighbor.DecrementIndirectMessageCount();

    if (aSedCapableNeighbor.GetIndirectMessageCount() == 0)
    {
        ClearEntry(aSedCapableNeighbor);
    }

exit:
    return;
}

void SourceMatchController::ResetMessageCount(SedCapableNeighbor &aSedCapableNeighbor)
{
    aSedCapableNeighbor.ResetIndirectMessageCount();
    ClearEntry(aSedCapableNeighbor);
}

void SourceMatchController::SetSrcMatchAsShort(SedCapableNeighbor &aSedCapableNeighbor, bool aUseShortAddress)
{
    VerifyOrExit(aSedCapableNeighbor.IsIndirectSourceMatchShort() != aUseShortAddress);

    if (aSedCapableNeighbor.GetIndirectMessageCount() > 0)
    {
        ClearEntry(aSedCapableNeighbor);
        aSedCapableNeighbor.SetIndirectSourceMatchShort(aUseShortAddress);
        AddEntry(aSedCapableNeighbor);
    }
    else
    {
        aSedCapableNeighbor.SetIndirectSourceMatchShort(aUseShortAddress);
    }

exit:
    return;
}

void SourceMatchController::ClearTable(void)
{
    Get<Radio>().ClearSrcMatchShortEntries();
    Get<Radio>().ClearSrcMatchExtEntries();
    otLogDebgMac("SrcAddrMatch - Cleared all entries");
}

void SourceMatchController::Enable(bool aEnable)
{
    mEnabled = aEnable;
    Get<Radio>().EnableSrcMatch(mEnabled);
    otLogDebgMac("SrcAddrMatch - %sabling", mEnabled ? "En" : "Dis");
}

void SourceMatchController::AddEntry(SedCapableNeighbor &aSedCapableNeighbor)
{
    aSedCapableNeighbor.SetIndirectSourceMatchPending(true);

    if (!IsEnabled())
    {
        SuccessOrExit(AddPendingEntries());
        Enable(true);
    }
    else
    {
        VerifyOrExit(AddAddress(aSedCapableNeighbor) == OT_ERROR_NONE, Enable(false));
        aSedCapableNeighbor.SetIndirectSourceMatchPending(false);
    }

exit:
    return;
}

otError SourceMatchController::AddAddress(const SedCapableNeighbor &aSedCapableNeighbor)
{
    otError error = OT_ERROR_NONE;

    if (aSedCapableNeighbor.IsIndirectSourceMatchShort())
    {
        error = Get<Radio>().AddSrcMatchShortEntry(aSedCapableNeighbor.GetRloc16());

        otLogDebgMac("SrcAddrMatch - Adding short addr: 0x%04x -- %s (%d)", aSedCapableNeighbor.GetRloc16(),
                     otThreadErrorToString(error), error);
    }
    else
    {
        Mac::ExtAddress address;

        address.Set(aSedCapableNeighbor.GetExtAddress().m8, Mac::ExtAddress::kReverseByteOrder);
        error = Get<Radio>().AddSrcMatchExtEntry(address);

        otLogDebgMac("SrcAddrMatch - Adding addr: %s -- %s (%d)",
                     aSedCapableNeighbor.GetExtAddress().ToString().AsCString(), otThreadErrorToString(error), error);
    }

    return error;
}

void SourceMatchController::ClearEntry(SedCapableNeighbor &aSedCapableNeighbor)
{
    otError error = OT_ERROR_NONE;

    if (aSedCapableNeighbor.IsIndirectSourceMatchPending())
    {
        otLogDebgMac("SrcAddrMatch - Clearing pending flag for 0x%04x", aSedCapableNeighbor.GetRloc16());
        aSedCapableNeighbor.SetIndirectSourceMatchPending(false);
        ExitNow();
    }

    if (aSedCapableNeighbor.IsIndirectSourceMatchShort())
    {
        error = Get<Radio>().ClearSrcMatchShortEntry(aSedCapableNeighbor.GetRloc16());

        otLogDebgMac("SrcAddrMatch - Clearing short addr: 0x%04x -- %s (%d)", aSedCapableNeighbor.GetRloc16(),
                     otThreadErrorToString(error), error);
    }
    else
    {
        Mac::ExtAddress address;

        address.Set(aSedCapableNeighbor.GetExtAddress().m8, Mac::ExtAddress::kReverseByteOrder);
        error = Get<Radio>().ClearSrcMatchExtEntry(address);

        otLogDebgMac("SrcAddrMatch - Clearing addr: %s -- %s (%d)",
                     aSedCapableNeighbor.GetExtAddress().ToString().AsCString(), otThreadErrorToString(error), error);
    }

    SuccessOrExit(error);

    if (!IsEnabled())
    {
        SuccessOrExit(AddPendingEntries());
        Enable(true);
    }

exit:
    return;
}

otError SourceMatchController::AddPendingEntries(void)
{
    otError error = OT_ERROR_NONE;

    for (SedCapableNeighbor &neighbor :
         Get<SedCapableNeighborTable>().Iterate(SedCapableNeighbor::kInStateValidOrRestoring))
    {
        if (neighbor.IsIndirectSourceMatchPending())
        {
            SuccessOrExit(error = AddAddress(neighbor));
            neighbor.SetIndirectSourceMatchPending(false);
        }
    }

exit:
    return error;
}

} // namespace ot
