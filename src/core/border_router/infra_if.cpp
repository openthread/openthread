/*
 *    Copyright (c) 2022, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements infrastructure network interface.
 */

#include "infra_if.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "border_router/routing_manager.hpp"
#include "common/as_core_type.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "net/icmp6.hpp"

namespace ot {
namespace BorderRouter {

RegisterLogModule("InfraIf");

InfraIf::InfraIf(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mInitialized(false)
    , mIsRunning(false)
    , mIfIndex(0)
{
}

Error InfraIf::Init(uint32_t aIfIndex)
{
    Error error = kErrorNone;

    VerifyOrExit(!mInitialized, error = kErrorInvalidState);
    VerifyOrExit(aIfIndex > 0, error = kErrorInvalidArgs);

    mIfIndex     = aIfIndex;
    mInitialized = true;

    LogInfo("Init %s", ToString().AsCString());

exit:
    return error;
}

void InfraIf::Deinit(void)
{
    mInitialized = false;
    mIsRunning   = false;
    mIfIndex     = 0;

    LogInfo("Deinit");
}

bool InfraIf::HasAddress(const Ip6::Address &aAddress)
{
    OT_ASSERT(mInitialized);

    return otPlatInfraIfHasAddress(mIfIndex, &aAddress);
}

Error InfraIf::Send(const Icmp6Packet &aPacket, const Ip6::Address &aDestination)
{
    OT_ASSERT(mInitialized);

    return otPlatInfraIfSendIcmp6Nd(mIfIndex, &aDestination, aPacket.GetBytes(), aPacket.GetLength());
}

void InfraIf::HandledReceived(uint32_t aIfIndex, const Ip6::Address &aSource, const Icmp6Packet &aPacket)
{
    Error error = kErrorNone;

    VerifyOrExit(mInitialized && mIsRunning, error = kErrorInvalidState);
    VerifyOrExit(aIfIndex == mIfIndex, error = kErrorDrop);
    VerifyOrExit(aPacket.GetBytes() != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(aPacket.GetLength() >= sizeof(Ip6::Icmp::Header), error = kErrorParse);

    Get<RoutingManager>().HandleReceived(aPacket, aSource);

exit:
    if (error != kErrorNone)
    {
        LogDebg("Dropped ICMPv6 message: %s", ErrorToString(error));
    }
}

Error InfraIf::HandleStateChanged(uint32_t aIfIndex, bool aIsRunning)
{
    Error error = kErrorNone;

    VerifyOrExit(mInitialized, error = kErrorInvalidState);
    VerifyOrExit(aIfIndex == mIfIndex, error = kErrorInvalidArgs);

    VerifyOrExit(aIsRunning != mIsRunning);
    LogInfo("State changed: %sRUNNING -> %sRUNNING", mIsRunning ? "" : "NOT ", aIsRunning ? "" : "NOT ");

    mIsRunning = aIsRunning;

    Get<RoutingManager>().HandleInfraIfStateChanged();

exit:
    return error;
}

InfraIf::InfoString InfraIf::ToString(void) const
{
    InfoString string;

    string.Append("infra netif %u", mIfIndex);
    return string;
}

//---------------------------------------------------------------------------------------------------------------------

extern "C" void otPlatInfraIfRecvIcmp6Nd(otInstance *        aInstance,
                                         uint32_t            aInfraIfIndex,
                                         const otIp6Address *aSrcAddress,
                                         const uint8_t *     aBuffer,
                                         uint16_t            aBufferLength)
{
    InfraIf::Icmp6Packet packet;

    packet.Init(aBuffer, aBufferLength);
    AsCoreType(aInstance).Get<InfraIf>().HandledReceived(aInfraIfIndex, AsCoreType(aSrcAddress), packet);
}

extern "C" otError otPlatInfraIfStateChanged(otInstance *aInstance, uint32_t aInfraIfIndex, bool aIsRunning)
{
    return AsCoreType(aInstance).Get<InfraIf>().HandleStateChanged(aInfraIfIndex, aIsRunning);
}

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
