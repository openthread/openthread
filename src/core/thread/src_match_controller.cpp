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

#define WPP_NAME "src_match_controller.tmh"

#include <openthread/config.h>

#include "src_match_controller.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

SourceMatchController::SourceMatchController(MeshForwarder &aMeshForwarder) :
    MeshForwarderLocator(aMeshForwarder),
    mEnabled(false)
{
    ClearTable();
}

void SourceMatchController::IncrementMessageCount(Child &aChild)
{
    if (aChild.GetIndirectMessageCount() == 0)
    {
        AddEntry(aChild);
    }

    aChild.IncrementIndirectMessageCount();
}

void SourceMatchController::DecrementMessageCount(Child &aChild)
{
    if (aChild.GetIndirectMessageCount() == 0)
    {
        otLogWarnMac(GetInstance(), "DecrementMessageCount(child 0x%04x) called when already at zero count.",
                     aChild.GetRloc16());
        ExitNow();
    }

    aChild.DecrementIndirectMessageCount();

    if (aChild.GetIndirectMessageCount() == 0)
    {
        ClearEntry(aChild);
    }

exit:
    return;
}

void SourceMatchController::ResetMessageCount(Child &aChild)
{
    aChild.ResetIndirectMessageCount();
    ClearEntry(aChild);
}

void SourceMatchController::SetSrcMatchAsShort(Child &aChild, bool aUseShortAddress)
{
    VerifyOrExit(aChild.IsIndirectSourceMatchShort() != aUseShortAddress);

    if (aChild.GetIndirectMessageCount() > 0)
    {
        ClearEntry(aChild);
        aChild.SetIndirectSourceMatchShort(aUseShortAddress);
        AddEntry(aChild);
    }
    else
    {
        aChild.SetIndirectSourceMatchShort(aUseShortAddress);
    }

exit:
    return;
}

void SourceMatchController::ClearTable(void)
{
    otPlatRadioClearSrcMatchShortEntries(&GetInstance());
    otPlatRadioClearSrcMatchExtEntries(&GetInstance());
    otLogDebgMac(GetInstance(), "SrcAddrMatch - Cleared all entries");
}

void SourceMatchController::Enable(bool aEnable)
{
    mEnabled = aEnable;
    otPlatRadioEnableSrcMatch(&GetInstance(), mEnabled);
    otLogDebgMac(GetInstance(), "SrcAddrMatch - %sabling", mEnabled ? "En" : "Dis");
}

void SourceMatchController::AddEntry(Child &aChild)
{
    aChild.SetIndirectSourceMatchPending(true);

    if (!IsEnabled())
    {
        SuccessOrExit(AddPendingEntries());
        Enable(true);
    }
    else
    {
        VerifyOrExit(AddAddress(aChild) == OT_ERROR_NONE, Enable(false));
        aChild.SetIndirectSourceMatchPending(false);
    }

exit:
    return;
}

otError SourceMatchController::AddAddress(const Child &aChild)
{
    otError error = OT_ERROR_NONE;

    if (aChild.IsIndirectSourceMatchShort())
    {
        error = otPlatRadioAddSrcMatchShortEntry(&GetInstance(), aChild.GetRloc16());

        otLogDebgMac(GetInstance(), "SrcAddrMatch - Adding short addr: 0x%04x -- %s (%d)",
                     aChild.GetRloc16(), otThreadErrorToString(error), error);
    }
    else
    {
        otExtAddress addr;

        for (uint8_t i = 0; i < sizeof(addr); i++)
        {
            addr.m8[i] = aChild.GetExtAddress().m8[sizeof(addr) - 1 - i];
        }

        error = otPlatRadioAddSrcMatchExtEntry(&GetInstance(), &addr);

        otLogDebgMac(GetInstance(), "SrcAddrMatch - Adding addr: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x -- %s (%d)",
                     addr.m8[7], addr.m8[6], addr.m8[5], addr.m8[4], addr.m8[3], addr.m8[2], addr.m8[1], addr.m8[0],
                     otThreadErrorToString(error), error);
    }

    return error;
}

void SourceMatchController::ClearEntry(Child &aChild)
{
    otError error = OT_ERROR_NONE;

    if (aChild.IsIndirectSourceMatchPending())
    {
        otLogDebgMac(GetInstance(), "SrcAddrMatch - Clearing pending flag for 0x%04x", aChild.GetRloc16());
        aChild.SetIndirectSourceMatchPending(false);
        ExitNow();
    }

    if (aChild.IsIndirectSourceMatchShort())
    {
        error = otPlatRadioClearSrcMatchShortEntry(&GetInstance(), aChild.GetRloc16());

        otLogDebgMac(GetInstance(), "SrcAddrMatch - Clearing short address: 0x%04x -- %s (%d)",
                     aChild.GetRloc16(), otThreadErrorToString(error), error);
    }
    else
    {
        otExtAddress addr;

        for (uint8_t i = 0; i < sizeof(addr); i++)
        {
            addr.m8[i] = aChild.GetExtAddress().m8[sizeof(aChild.GetExtAddress()) - 1 - i];
        }

        error = otPlatRadioClearSrcMatchExtEntry(&GetInstance(), &addr);

        otLogDebgMac(GetInstance(), "SrcAddrMatch - Clearing addr: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x -- %s (%d)",
                     addr.m8[7], addr.m8[6], addr.m8[5], addr.m8[4], addr.m8[3], addr.m8[2], addr.m8[1], addr.m8[0],
                     otThreadErrorToString(error), error);
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
    uint8_t numChildren;
    Child *child;

    child = GetMeshForwarder().GetNetif().GetMle().GetChildren(&numChildren);

    for (uint8_t i = 0; i < numChildren; i++, child++)
    {
        if (child->IsStateValidOrRestoring() && child->IsIndirectSourceMatchPending())
        {
            SuccessOrExit(error = AddAddress(*child));
            child->SetIndirectSourceMatchPending(false);
        }
    }

exit:
    return error;
}

}  // namespace ot
