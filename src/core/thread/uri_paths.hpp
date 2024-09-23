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
 * Represents Thread URIs.
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
    kUriActiveReplace,          ///< MGMT_ACTIVE_REPLACE ("c/ar")
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
 * Returns URI path string for a given URI.
 *
 * @param[in] aUri   A URI.
 *
 * @returns The path string for @p aUri.
 */
const char *PathForUri(Uri aUri);

/**
 * Looks up the URI from a given path string.
 *
 * @param[in] aPath    A path string.
 *
 * @returns The URI associated with @p aPath or `kUriUnknown` if no match is found.
 */
Uri UriFromPath(const char *aPath);

/**
 * This template function converts a given URI to a human-readable string.
 *
 * @tparam kUri   The URI to convert to string.
 *
 * @returns The string representation of @p kUri.
 */
template <Uri kUri> const char *UriToString(void);

// Declaring specializations of `UriToString` for every `Uri`
template <> const char *UriToString<kUriAddressError>(void);
template <> const char *UriToString<kUriAddressNotify>(void);
template <> const char *UriToString<kUriAddressQuery>(void);
template <> const char *UriToString<kUriAddressRelease>(void);
template <> const char *UriToString<kUriAddressSolicit>(void);
template <> const char *UriToString<kUriServerData>(void);
template <> const char *UriToString<kUriAnycastLocate>(void);
template <> const char *UriToString<kUriBackboneAnswer>(void);
template <> const char *UriToString<kUriBackboneMlr>(void);
template <> const char *UriToString<kUriBackboneQuery>(void);
template <> const char *UriToString<kUriAnnounceBegin>(void);
template <> const char *UriToString<kUriActiveGet>(void);
template <> const char *UriToString<kUriActiveReplace>(void);
template <> const char *UriToString<kUriActiveSet>(void);
template <> const char *UriToString<kUriCommissionerKeepAlive>(void);
template <> const char *UriToString<kUriCommissionerGet>(void);
template <> const char *UriToString<kUriCommissionerPetition>(void);
template <> const char *UriToString<kUriCommissionerSet>(void);
template <> const char *UriToString<kUriDatasetChanged>(void);
template <> const char *UriToString<kUriEnergyReport>(void);
template <> const char *UriToString<kUriEnergyScan>(void);
template <> const char *UriToString<kUriJoinerEntrust>(void);
template <> const char *UriToString<kUriJoinerFinalize>(void);
template <> const char *UriToString<kUriLeaderKeepAlive>(void);
template <> const char *UriToString<kUriLeaderPetition>(void);
template <> const char *UriToString<kUriPanIdConflict>(void);
template <> const char *UriToString<kUriPendingGet>(void);
template <> const char *UriToString<kUriPanIdQuery>(void);
template <> const char *UriToString<kUriPendingSet>(void);
template <> const char *UriToString<kUriRelayRx>(void);
template <> const char *UriToString<kUriRelayTx>(void);
template <> const char *UriToString<kUriProxyRx>(void);
template <> const char *UriToString<kUriProxyTx>(void);
template <> const char *UriToString<kUriDiagnosticGetAnswer>(void);
template <> const char *UriToString<kUriDiagnosticGetRequest>(void);
template <> const char *UriToString<kUriDiagnosticGetQuery>(void);
template <> const char *UriToString<kUriDiagnosticReset>(void);
template <> const char *UriToString<kUriDuaRegistrationNotify>(void);
template <> const char *UriToString<kUriDuaRegistrationRequest>(void);
template <> const char *UriToString<kUriMlr>(void);

} // namespace ot

#endif // URI_PATHS_HPP_
