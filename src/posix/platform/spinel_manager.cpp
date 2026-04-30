/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "platform-posix.h"

#include "posix/platform/spinel_manager.hpp"

#include "common/code_utils.hpp"
#include "common/new.hpp"
#include "lib/spinel/spinel_driver.hpp"
#include "posix/platform/hdlc_interface.hpp"
#include "posix/platform/radio_url.hpp"
#include "posix/platform/spi_interface.hpp"
#include "posix/platform/spinel_driver_getter.hpp"
#include "posix/platform/vendor_interface.hpp"

static ot::Posix::SpinelManager sSpinelManager;

namespace ot {
namespace Posix {

// Implements `GetSpinelDriver` in spinel_driver_getter.hpp for external access to SpinelDriver
Spinel::SpinelDriver &GetSpinelDriver(void) { return sSpinelManager.GetSpinelDriver(); }

SpinelManager &SpinelManager::GetSpinelManager(void) { return sSpinelManager; }

SpinelManager::SpinelManager(void)
    : mUrl(nullptr)
    , mSpinelDriver()
    , mSpinelInterface(nullptr)
{
}

SpinelManager::~SpinelManager(void) { Deinit(); }

CoprocessorType SpinelManager::Init(const char *aUrl)
{
    bool            swReset;
    spinel_iid_t    iidList[Spinel::kSpinelHeaderMaxNumIid];
    CoprocessorType mode;

    mUrl.Init(aUrl);
    VerifyOrDie(mUrl.GetPath() != nullptr, OT_EXIT_INVALID_ARGUMENTS);

    GetIidListFromUrl(iidList);

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    VirtualTimeInit();
#endif

    mSpinelInterface = CreateSpinelInterface(mUrl.GetProtocol());
    VerifyOrDie(mSpinelInterface != nullptr, OT_EXIT_FAILURE);

    swReset = !mUrl.HasParam("no-reset");

    mode = mSpinelDriver.Init(*mSpinelInterface, swReset, iidList, OT_ARRAY_LENGTH(iidList));

    otLogDebgPlat("instance init:%p - iid = %d", (void *)&mSpinelDriver, iidList[0]);

    return mode;
}

void SpinelManager::Deinit(void)
{
    if (mSpinelInterface != nullptr)
    {
        mSpinelInterface->Deinit();
        mSpinelInterface = nullptr;
    }

    mSpinelDriver.Deinit();
}

Spinel::SpinelInterface *SpinelManager::CreateSpinelInterface(const char *aInterfaceName)
{
    Spinel::SpinelInterface *interface;

    if (aInterfaceName == nullptr)
    {
        DieNow(OT_ERROR_FAILED);
    }
#if OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
    else if (HdlcInterface::IsInterfaceNameMatch(aInterfaceName))
    {
        interface = new (&mSpinelInterfaceRaw) HdlcInterface(mUrl);
    }
#endif
#if OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
    else if (Posix::SpiInterface::IsInterfaceNameMatch(aInterfaceName))
    {
        interface = new (&mSpinelInterfaceRaw) SpiInterface(mUrl);
    }
#endif
#if OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE
    else if (VendorInterface::IsInterfaceNameMatch(aInterfaceName))
    {
        interface = new (&mSpinelInterfaceRaw) VendorInterface(mUrl);
    }
#endif
    else
    {
        otLogCritPlat("The Spinel interface name \"%s\" is not supported!", aInterfaceName);
        DieNow(OT_ERROR_FAILED);
    }

    return interface;
}

void SpinelManager::GetIidListFromUrl(spinel_iid_t (&aIidList)[Spinel::kSpinelHeaderMaxNumIid])
{
    const char *iidString;
    const char *iidListString;

    memset(aIidList, SPINEL_HEADER_INVALID_IID, sizeof(aIidList));

    iidString     = mUrl.GetValue("iid");
    iidListString = mUrl.GetValue("iid-list");

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    // First entry to the aIidList must be the IID of the host application.
    VerifyOrDie(iidString != nullptr, OT_EXIT_INVALID_ARGUMENTS);
    aIidList[0] = static_cast<spinel_iid_t>(atoi(iidString));

    if (iidListString != nullptr)
    {
        // Convert string to an array of integers.
        // Integer i is for traverse the iidListString.
        // Integer j is for aIidList array offset location.
        // First entry of aIidList is for host application iid hence j start from 1.
        for (uint8_t i = 0, j = 1; iidListString[i] != '\0' && j < Spinel::kSpinelHeaderMaxNumIid; i++)
        {
            if (iidListString[i] == ',')
            {
                j++;
                continue;
            }

            if (iidListString[i] < '0' || iidListString[i] > '9')
            {
                DieNow(OT_EXIT_INVALID_ARGUMENTS);
            }
            else
            {
                aIidList[j] = iidListString[i] - '0';
                VerifyOrDie(aIidList[j] < Spinel::kSpinelHeaderMaxNumIid, OT_EXIT_INVALID_ARGUMENTS);
            }
        }
    }
#else  // !OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    VerifyOrDie(iidString == nullptr, OT_EXIT_INVALID_ARGUMENTS);
    VerifyOrDie(iidListString == nullptr, OT_EXIT_INVALID_ARGUMENTS);
    aIidList[0] = 0;
#endif // OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void SpinelManager::VirtualTimeInit(void)
{
    // The last argument must be the node id
    const char *nodeId = nullptr;

    for (const char *arg = nullptr; (arg = mUrl.GetValue("forkpty-arg", arg)) != nullptr; nodeId = arg)
    {
    }

    virtualTimeInit(static_cast<uint16_t>(atoi(nodeId)));
}
#endif

} // namespace Posix
} // namespace ot

CoprocessorType platformSpinelManagerInit(const char *aUrl) { return sSpinelManager.Init(aUrl); }

void platformSpinelManagerDeinit(void) { return sSpinelManager.Deinit(); }

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void virtualTimeSpinelProcess(otInstance *aInstance, const struct VirtualTimeEvent *aEvent)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Posix::GetSpinelDriver().Process(aEvent);
}
#else
void platformSpinelManagerProcess(otInstance *aInstance, const otSysMainloopContext *aContext)
{
    OT_UNUSED_VARIABLE(aInstance);

    ot::Posix::GetSpinelDriver().Process(aContext);
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

void platformSpinelManagerUpdateFdSet(otSysMainloopContext *aContext)
{
    sSpinelManager.GetSpinelInterface().UpdateFdSet(aContext);

    if (ot::Posix::GetSpinelDriver().HasPendingFrame())
    {
        aContext->mTimeout.tv_sec  = 0;
        aContext->mTimeout.tv_usec = 0;
    }
}
