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
 *   This file includes definitions for OpenThread's fault injection Manager
 */

#ifndef OT_FAULT_INJECTION_HPP_
#define OT_FAULT_INJECTION_HPP_

#include <openthread/config.h>

#if OPENTHREAD_ENABLE_FAULT_INJECTION

#include <nlfaultinjection.hpp>

#include <openthread/otfaultinjection.h>

namespace ot {
namespace FaultInjection {

nl::FaultInjection::Manager &GetManager(void);

} // namespace FaultInjection
} // namespace ot

/**
 * Execute the statements included if the OT fault is
 * to be injected.
 *
 * @param[in] aFaultID      An OT fault-injection id
 * @param[in] aStatements   Statements to be executed if the fault is enabled.
 */
#define OT_FAULT_INJECT( aFaultID, aStatements ) \
        nlFAULT_INJECT(ot::FaultInjection::GetManager(), aFaultID, aStatements)

/**
 * Execute the statements included if the fault is
 * to be injected. Also, if there are no arguments stored in the
 * fault, save aMaxArg into the record so it can be printed out
 * to the debug log by a callback installed on purpose.
 *
 * @param[in] aFaultID      An OT fault-injection id
 * @param[in] aMaxArg       The max value accepted as argument by the statements to be injected
 * @param[in] aProtectedStatements   Statements to be executed if the fault is enabled while holding the
 *                          Manager's lock
 * @param[in] aUnprotectedStatements   Statements to be executed if the fault is enabled without holding the
 *                          Manager's lock
 */
#define OT_FAULT_INJECT_MAX_ARG( aFaultID, aMaxArg, aProtectedStatements, aUnprotectedStatements ) \
    do { \
        nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager(); \
        const nl::FaultInjection::Record *records = mgr.GetFaultRecords(); \
        if (records[aFaultID].mNumArguments == 0) \
        { \
            int32_t arg = aMaxArg; \
            mgr.StoreArgsAtFault(aFaultID, 1, &arg); \
        } \
        nlFAULT_INJECT_WITH_ARGS(mgr, aFaultID, aProtectedStatements, aUnprotectedStatements ); \
    } while (0)

/**
 * Execute the statements included if the fault is
 * to be injected.
 *
 * @param[in] aFaultID      An OT fault-injection id
 * @param[in] aMaxArg       The max value accepted as argument by the statements to be injected
 * @param[in] aProtectedStatements   Statements to be executed if the fault is enabled while holding the
 *                          Manager's lock
 * @param[in] aUnprotectedStatements   Statements to be executed if the fault is enabled without holding the
 *                          Manager's lock
 */
#define OT_FAULT_INJECT_WITH_ARGS( aFaultID, aProtectedStatements, aUnprotectedStatements ) \
        nlFAULT_INJECT_WITH_ARGS(ot::FaultInjection::GetManager(), aFaultID,  \
                                 aProtectedStatements, aUnprotectedStatements ); \

#else // OPENTHREAD_ENABLE_FAULT_INJECTION

#define OT_FAULT_INJECT( aFaultID, aStatements )
#define OT_FAULT_INJECT_WITH_ARGS( aFaultID, aProtectedStatements, aUnprotectedStatements )
#define OT_FAULT_INJECT_MAX_ARG( aFaultID, aMaxArg, aProtectedStatements, aUnprotectedStatements )

#endif // OPENTHREAD_ENABLE_FAULT_INJECTION


#endif // OT_FAULT_INJECTION_H_
