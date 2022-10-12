/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for Thread URIs.
 */

#ifndef URI_PATHS_HPP_
#define URI_PATHS_HPP_

#include "openthread-core-config.h"

#include "common/error.hpp"

namespace ot {

/**
 * This enumeration represents Thread URIs.
 *
 */
enum Uri : uint8_t
{
    kUriAddressError,           ///< Address Error ("a/ae")
    kUriAddressNotify,          ///< Address Notify ("a/an")
    kUriAddressQuery,           ///< Address Query ("a/aq")
    kUriAddressRelease,         ///< Address Release ("a/ar")
    kUriAddressSolicit,         ///< Address Solicit ("a/as")
    kUriServerData,             ///< Server Data Registration ("a/sd")
    kUriAnycastLocate,          ///< Anycast Locate ("a/yl")
    kUriBackboneAnswer,         ///< Backbone Answer / Backbone Notification ("b/ba")
    kUriBackboneMlr,            ///< Backbone Multicast Listener Report ("b/bmr")
    kUriBackboneQuery,          ///< Backbone Query ("b/bq")
    kUriAnnounceBegin,          ///< Announce Begin ("c/ab")
    kUriActiveGet,              ///< MGMT_ACTIVE_GET "c/ag"
    kUriActiveSet,              ///< MGMT_ACTIVE_SET ("c/as")
    kUriCommissionerKeepAlive,  ///< Commissioner Keep Alive ("c/ca")
    kUriCommissionerGet,        ///< MGMT_COMMISSIONER_GET ("c/cg")
    kUriCommissionerPetition,   ///< Commissioner Petition ("c/cp")
    kUriCommissionerSet,        ///< MGMT_COMMISSIONER_SET ("c/cs")
    kUriDatasetChanged,         ///< MGMT_DATASET_CHANGED ("c/dc")
    kUriEnergyReport,           ///< Energy Report ("c/er")
    kUriEnergyScan,             ///< Energy Scan ("c/es")
    kUriJoinerEntrust,          ///< Joiner Entrust  ("c/je")
    kUriJoinerFinalize,         ///< Joiner Finalize ("c/jf")
    kUriLeaderKeepAlive,        ///< Leader Keep Alive ("c/la")
    kUriLeaderPetition,         ///< Leader Petition ("c/lp")
    kUriPanIdConflict,          ///< PAN ID Conflict ("c/pc")
    kUriPendingGet,             ///< MGMT_PENDING_GET ("c/pg")
    kUriPanIdQuery,             ///< PAN ID Query ("c/pq")
    kUriPendingSet,             ///< MGMT_PENDING_SET ("c/ps")
    kUriRelayRx,                ///< Relay RX ("c/rx")
    kUriRelayTx,                ///< Relay TX ("c/tx")
    kUriProxyRx,                ///< Proxy RX ("c/ur")
    kUriProxyTx,                ///< Proxy TX ("c/ut")
    kUriDiagnosticGetAnswer,    ///< Network Diagnostic Get Answer ("d/da")
    kUriDiagnosticGetRequest,   ///< Network Diagnostic Get Request ("d/dg")
    kUriDiagnosticGetQuery,     ///< Network Diagnostic Get Query ("d/dq")
    kUriDiagnosticReset,        ///< Network Diagnostic Reset ("d/dr")
    kUriDuaRegistrationNotify,  ///< DUA Registration Notification ("n/dn")
    kUriDuaRegistrationRequest, ///< DUA Registration Request ("n/dr")
    kUriMlr,                    ///< Multicast Listener Registration ("n/mr")
    kUriUnknown,                ///< Unknown URI
};

/**
 * This function returns URI path string for a given URI.
 *
 * @param[in] aUri   A URI.
 *
 * @returns The path string for @p aUri.
 *
 */
const char *PathForUri(Uri aUri);

/**
 * This function looks up the URI from a given path string.
 *
 * @param[in] aPath    A path string.
 *
 * @returns The URI associated with @p aPath or `kUriUnknown` if no match is found.
 *
 */
Uri UriFromPath(const char *aPath);

} // namespace ot

#endif // URI_PATHS_HPP_
