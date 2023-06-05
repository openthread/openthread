/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements CLI for Backbone Router.
 */

#include "cli_bbr.hpp"

#include "cli/cli.hpp"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

namespace ot {
namespace Cli {

void Bbr::OutputConfig(const otBackboneRouterConfig &aConfig)
{
    OutputLine("seqno:    %u", aConfig.mSequenceNumber);
    OutputLine("delay:    %u secs", aConfig.mReregistrationDelay);
    OutputLine("timeout:  %lu secs", ToUlong(aConfig.mMlrTimeout));
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

template <> otError Bbr::Process<Cmd("mlr")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    /**
     * @cli bbr mgmt mlr listener
     * @code
     * bbr mgmt mlr listener
     * ff04:0:0:0:0:0:0:abcd 3534000
     * ff04:0:0:0:0:0:0:eeee 3537610
     * Done
     * @endcode
     * @par
     * Returns the Multicast Listeners with the #otBackboneRouterMulticastListenerInfo
     * `mTimeout` in seconds.
     * @par
     * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` and
     * `OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE` are enabled.
     * @sa otBackboneRouterMulticastListenerGetNext
     */
    if (aArgs[0] == "listener")
    {
        if (aArgs[1].IsEmpty())
        {
            otBackboneRouterMulticastListenerIterator iter = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ITERATOR_INIT;
            otBackboneRouterMulticastListenerInfo     listenerInfo;

            while (otBackboneRouterMulticastListenerGetNext(GetInstancePtr(), &iter, &listenerInfo) == OT_ERROR_NONE)
            {
                OutputIp6Address(listenerInfo.mAddress);
                OutputLine(" %lu", ToUlong(listenerInfo.mTimeout));
            }

            ExitNow(error = OT_ERROR_NONE);
        }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        /**
         * @cli bbr mgmt mlr listener clear
         * @code
         * bbr mgmt mlr listener clear
         * Done
         * @endcode
         * @par api_copy
         * #otBackboneRouterMulticastListenerClear
         */
        if (aArgs[1] == "clear")
        {
            otBackboneRouterMulticastListenerClear(GetInstancePtr());
            error = OT_ERROR_NONE;
        }
        /**
         * @cli bbr mgmt mlr listener add
         * @code
         * bbr mgmt mlr listener add ff04::1
         * Done
         * @endcode
         * @code
         * bbr mgmt mlr listener add ff04::2 300
         * Done
         * @endcode
         * @cparam bbr mgmt mlr listener add @ca{ipaddress} [@ca{timeout-seconds}]
         * @par api_copy
         * #otBackboneRouterMulticastListenerAdd
         */
        else if (aArgs[1] == "add")
        {
            otIp6Address address;
            uint32_t     timeout = 0;

            SuccessOrExit(error = aArgs[2].ParseAsIp6Address(address));

            if (!aArgs[3].IsEmpty())
            {
                SuccessOrExit(error = aArgs[3].ParseAsUint32(timeout));
            }

            error = otBackboneRouterMulticastListenerAdd(GetInstancePtr(), &address, timeout);
        }
    }
    /**
     * @cli bbr mgmt mlr response
     * @code
     * bbr mgmt mlr response 2
     * Done
     * @endcode
     * @cparam bbr mgmt mlr response @ca{status-code}
     * For `status-code`, use:
     * *    0: ST_MLR_SUCCESS
     * *    2: ST_MLR_INVALID
     * *    3: ST_MLR_NO_PERSISTENT
     * *    4: ST_MLR_NO_RESOURCES
     * *    5: ST_MLR_BBR_NOT_PRIMARY
     * *    6: ST_MLR_GENERAL_FAILURE
     * @par api_copy
     * #otBackboneRouterConfigNextMulticastListenerRegistrationResponse
     */
    else if (aArgs[0] == "response")
    {
        uint8_t status;

        SuccessOrExit(error = aArgs[1].ParseAsUint8(status));
        otBackboneRouterConfigNextMulticastListenerRegistrationResponse(GetInstancePtr(), status);
        error = OT_ERROR_NONE;

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    }

exit:
    return error;
}

#endif // #if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

template <> otError Bbr::Process<Cmd("mgmt")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgs[0].IsEmpty())
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * @cli bbr mgmt dua
     * @code
     * bbr mgmt dua 1 2f7c235e5025a2fd
     * Done
     * @endcode
     * @code
     * bbr mgmt dua 160
     * Done
     * @endcode
     * @cparam bbr mgmt dua @ca{status|coap-code} [@ca{meshLocalIid}]
     * For `status` or `coap-code`, use:
     * *    0: ST_DUA_SUCCESS
     * *    1: ST_DUA_REREGISTER
     * *    2: ST_DUA_INVALID
     * *    3: ST_DUA_DUPLICATE
     * *    4: ST_DUA_NO_RESOURCES
     * *    5: ST_DUA_BBR_NOT_PRIMARY
     * *    6: ST_DUA_GENERAL_FAILURE
     * *    160: COAP code 5.00
     * @par
     * With the `meshLocalIid` included, this command configures the response status
     * for the next DUA registration. Without `meshLocalIid`, respond to the next
     * DUA.req with the specified `status` or `coap-code`.
     * @par
     * Available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     * @sa otBackboneRouterConfigNextDuaRegistrationResponse
     */
    if (aArgs[0] == "dua")
    {
        uint8_t                   status;
        otIp6InterfaceIdentifier *mlIid = nullptr;
        otIp6InterfaceIdentifier  iid;

        SuccessOrExit(error = aArgs[1].ParseAsUint8(status));

        if (!aArgs[2].IsEmpty())
        {
            SuccessOrExit(error = aArgs[2].ParseAsHexString(iid.mFields.m8));
            mlIid = &iid;
            VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }

        otBackboneRouterConfigNextDuaRegistrationResponse(GetInstancePtr(), mlIid, status);
        ExitNow();
    }
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    if (aArgs[0] == "mlr")
    {
        error = Process<Cmd("mlr")>(aArgs + 1);
        ExitNow();
    }
#endif

exit:
    return error;
}

/**
 * @cli bbr enable
 * @code
 * bbr enable
 * Done
 * @endcode
 * @par api_copy
 * #otBackboneRouterSetEnabled
 */
template <> otError Bbr::Process<Cmd("enable")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    otBackboneRouterSetEnabled(GetInstancePtr(), true);

    return OT_ERROR_NONE;
}

/**
 * @cli bbr disable
 * @code
 * bbr disable
 * Done
 * @endcode
 * @par api_copy
 * #otBackboneRouterSetEnabled
 */
template <> otError Bbr::Process<Cmd("disable")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    otBackboneRouterSetEnabled(GetInstancePtr(), false);

    return OT_ERROR_NONE;
}

/**
 * @cli bbr jitter (get,set)
 * @code
 * bbr jitter
 * 20
 * Done
 * @endcode
 * @code
 * bbr jitter 10
 * Done
 * @endcode
 * @cparam bbr jitter [@ca{jitter}]
 * @par
 * Gets or sets jitter (in seconds) for Backbone Router registration.
 * @par
 * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is enabled.
 * @sa otBackboneRouterGetRegistrationJitter
 * @sa otBackboneRouterSetRegistrationJitter
 */
template <> otError Bbr::Process<Cmd("jitter")>(Arg aArgs[])
{
    return Interpreter::GetInterpreter().ProcessGetSet(aArgs, otBackboneRouterGetRegistrationJitter,
                                                       otBackboneRouterSetRegistrationJitter);
}

/**
 * @cli bbr register
 * @code
 * bbr register
 * Done
 * @endcode
 * @par api_copy
 * #otBackboneRouterRegister
 */
template <> otError Bbr::Process<Cmd("register")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    return otBackboneRouterRegister(GetInstancePtr());
}

/**
 * @cli bbr state
 * @code
 * bbr state
 * Disabled
 * Done
 * @endcode
 * @code
 * bbr state
 * Primary
 * Done
 * @endcode
 * @code
 * bbr state
 * Secondary
 * Done
 * @endcode
 * @par
 * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is enabled.
 * @par api_copy
 * #otBackboneRouterGetState
 */
template <> otError Bbr::Process<Cmd("state")>(Arg aArgs[])

{
    static const char *const kStateStrings[] = {
        "Disabled",  // (0) OT_BACKBONE_ROUTER_STATE_DISABLED
        "Secondary", // (1) OT_BACKBONE_ROUTER_STATE_SECONDARY
        "Primary",   // (2) OT_BACKBONE_ROUTER_STATE_PRIMARY
    };

    static_assert(0 == OT_BACKBONE_ROUTER_STATE_DISABLED, "OT_BACKBONE_ROUTER_STATE_DISABLED value is incorrect");
    static_assert(1 == OT_BACKBONE_ROUTER_STATE_SECONDARY, "OT_BACKBONE_ROUTER_STATE_SECONDARY value is incorrect");
    static_assert(2 == OT_BACKBONE_ROUTER_STATE_PRIMARY, "OT_BACKBONE_ROUTER_STATE_PRIMARY value is incorrect");

    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%s", Stringify(otBackboneRouterGetState(GetInstancePtr()), kStateStrings));

    return OT_ERROR_NONE;
}

/**
 * @cli bbr config
 * @code
 * bbr config
 * seqno:    10
 * delay:    120 secs
 * timeout:  300 secs
 * Done
 * @endcode
 * @par api_copy
 * #otBackboneRouterGetConfig
 */
template <> otError Bbr::Process<Cmd("config")>(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otBackboneRouterConfig config;

    otBackboneRouterGetConfig(GetInstancePtr(), &config);

    if (aArgs[0].IsEmpty())
    {
        OutputConfig(config);
    }
    else
    {
        // Set local Backbone Router configuration.
        /**
         * @cli bbr config (set)
         * @code
         * bbr config seqno 20 delay 30
         * Done
         * @endcode
         * @cparam bbr config [seqno @ca{seqno}] [delay @ca{delay}] [timeout @ca{timeout}]
         * @par
         * `bbr register` should be issued explicitly to register Backbone Router service to Leader
         * for Secondary Backbone Router.
         * @par api_copy
         * #otBackboneRouterSetConfig
         */
        for (Arg *arg = &aArgs[0]; !arg->IsEmpty(); arg++)
        {
            if (*arg == "seqno")
            {
                arg++;
                SuccessOrExit(error = arg->ParseAsUint8(config.mSequenceNumber));
            }
            else if (*arg == "delay")
            {
                arg++;
                SuccessOrExit(error = arg->ParseAsUint16(config.mReregistrationDelay));
            }
            else if (*arg == "timeout")
            {
                arg++;
                SuccessOrExit(error = arg->ParseAsUint32(config.mMlrTimeout));
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        error = otBackboneRouterSetConfig(GetInstancePtr(), &config);
    }

exit:
    return error;
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

otError Bbr::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString) {aCommandString, &Bbr::Process<Cmd(aCommandString)>}

    otError error = OT_ERROR_INVALID_COMMAND;

    /**
     * @cli bbr
     * @code
     * bbr
     * BBR Primary:
     * server16: 0xE400
     * seqno:    10
     * delay:    120 secs
     * timeout:  300 secs
     * Done
     * @endcode
     * @code
     * bbr
     * BBR Primary: None
     * Done
     * @endcode
     * @par
     * Returns the current Primary Backbone Router information for the Thread device.
     */
    if (aArgs[0].IsEmpty())
    {
        otBackboneRouterConfig config;

        OutputFormat("BBR Primary:");

        if (otBackboneRouterGetPrimary(GetInstancePtr(), &config) == OT_ERROR_NONE)
        {
            OutputNewLine();
            OutputLine("server16: 0x%04X", config.mServer16);
            OutputConfig(config);
        }
        else
        {
            OutputLine(" None");
        }

        error = OT_ERROR_NONE;
        ExitNow();
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    {
        static constexpr Command kCommands[] = {
            CmdEntry("config"), CmdEntry("disable"),  CmdEntry("enable"), CmdEntry("jitter"),
            CmdEntry("mgmt"),   CmdEntry("register"), CmdEntry("state"),
        };

#undef CmdEntry

        static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

        const Command *command;

        command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
        VerifyOrExit(command != nullptr);

        error = (this->*command->mHandler)(aArgs + 1);
    }
#endif // #if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // #if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
