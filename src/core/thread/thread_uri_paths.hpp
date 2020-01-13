/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#ifndef THREAD_URIS_HPP_
#define THREAD_URIS_HPP_

#include "openthread-core-config.h"

namespace ot {

/**
 * The URI Path for Address Query.
 *
 */
#define OT_URI_PATH_ADDRESS_QUERY "a/aq"

/**
 * @def OT_URI_PATH_ADDRESS_NOTIFY
 *
 * The URI Path for Address Notify.
 *
 */
#define OT_URI_PATH_ADDRESS_NOTIFY "a/an"

/**
 * @def OT_URI_PATH_ADDRESS_ERROR
 *
 * The URI Path for Address Error.
 *
 */
#define OT_URI_PATH_ADDRESS_ERROR "a/ae"

/**
 * @def OT_URI_PATH_ADDRESS_RELEASE
 *
 * The URI Path for Address Release.
 *
 */
#define OT_URI_PATH_ADDRESS_RELEASE "a/ar"

/**
 * @def OT_URI_PATH_ADDRESS_SOLICIT
 *
 * The URI Path for Address Solicit.
 *
 */
#define OT_URI_PATH_ADDRESS_SOLICIT "a/as"

/**
 * @def OT_URI_PATH_ACTIVE_GET
 *
 * The URI Path for MGMT_ACTIVE_GET
 *
 */
#define OT_URI_PATH_ACTIVE_GET "c/ag"

/**
 * @def OT_URI_PATH_ACTIVE_SET
 *
 * The URI Path for MGMT_ACTIVE_SET
 *
 */
#define OT_URI_PATH_ACTIVE_SET "c/as"

/**
 * @def OT_URI_PATH_DATASET_CHANGED
 *
 * The URI Path for MGMT_DATASET_CHANGED
 *
 */
#define OT_URI_PATH_DATASET_CHANGED "c/dc"

/**
 * @def OT_URI_PATH_ENERGY_SCAN
 *
 * The URI Path for Energy Scan
 *
 */
#define OT_URI_PATH_ENERGY_SCAN "c/es"

/**
 * @def OT_URI_PATH_ENERGY_REPORT
 *
 * The URI Path for Energy Report
 *
 */
#define OT_URI_PATH_ENERGY_REPORT "c/er"

/**
 * @def OT_URI_PATH_PENDING_GET
 *
 * The URI Path for MGMT_PENDING_GET
 *
 */
#define OT_URI_PATH_PENDING_GET "c/pg"

/**
 * @def OT_URI_PATH_PENDING_SET
 *
 * The URI Path for MGMT_PENDING_SET
 *
 */
#define OT_URI_PATH_PENDING_SET "c/ps"

/**
 * @def OT_URI_PATH_SERVER_DATA
 *
 * The URI Path for Server Data Registration.
 *
 */
#define OT_URI_PATH_SERVER_DATA "a/sd"

/**
 * @def OT_URI_PATH_ANNOUNCE_BEGIN
 *
 * The URI Path for Announce Begin.
 *
 */
#define OT_URI_PATH_ANNOUNCE_BEGIN "c/ab"

/**
 * @def OT_URI_PATH_PROXY_RX
 *
 * The URI Path for Proxy RX.
 *
 */
#define OT_URI_PATH_PROXY_RX "c/ur"

/**
 * @def OT_URI_PATH_PROXY_TX
 *
 * The URI Path for Proxy TX.
 *
 */
#define OT_URI_PATH_PROXY_TX "c/ut"

/**
 * @def OT_URI_PATH_RELAY_RX
 *
 * The URI Path for Relay RX.
 *
 */
#define OT_URI_PATH_RELAY_RX "c/rx"

/**
 * @def OT_URI_PATH_RELAY_TX
 *
 * The URI Path for Relay TX.
 *
 */
#define OT_URI_PATH_RELAY_TX "c/tx"

/**
 * @def OT_URI_PATH_JOINER_FINALIZE
 *
 * The URI Path for Joiner Finalize
 *
 */
#define OT_URI_PATH_JOINER_FINALIZE "c/jf"

/**
 * @def OT_URI_PATH_JOINER_ENTRUST
 *
 * The URI Path for Joiner Entrust
 *
 */
#define OT_URI_PATH_JOINER_ENTRUST "c/je"

/**
 * @def OT_URI_PATH_LEADER_PETITION
 *
 * The URI Path for Leader Petition
 *
 */
#define OT_URI_PATH_LEADER_PETITION "c/lp"

/**
 * @def OT_URI_PATH_LEADER_KEEP_ALIVE
 *
 * The URI Path for Leader Keep Alive
 *
 */
#define OT_URI_PATH_LEADER_KEEP_ALIVE "c/la"

/**
 * @def OT_URI_PATH_PANID_CONFLICT
 *
 * The URI Path for PAN ID Conflict
 *
 */
#define OT_URI_PATH_PANID_CONFLICT "c/pc"

/**
 * @def OT_URI_PATH_PANID_QUERY
 *
 * The URI Path for PAN ID Query
 *
 */
#define OT_URI_PATH_PANID_QUERY "c/pq"

/**
 * @def OT_URI_PATH_COMMISSIONER_GET
 *
 * The URI Path for MGMT_COMMISSIONER_GET
 *
 */
#define OT_URI_PATH_COMMISSIONER_GET "c/cg"

/**
 * @def OT_URI_PATH_COMMISSIONER_KEEP_ALIVE
 *
 * The URI Path for Commissioner Keep Alive.
 *
 */
#define OT_URI_PATH_COMMISSIONER_KEEP_ALIVE "c/ca"

/**
 * @def OT_URI_PATH_COMMISSIONER_PETITION
 *
 * The URI Path for Commissioner Petition.
 *
 */
#define OT_URI_PATH_COMMISSIONER_PETITION "c/cp"

/**
 * @def OT_URI_PATH_COMMISSIONER_SET
 *
 * The URI Path for MGMT_COMMISSIONER_SET
 *
 */
#define OT_URI_PATH_COMMISSIONER_SET "c/cs"

/**
 * @def OT_URI_PATH_DIAGNOSTIC_GET_REQUEST
 *
 * The URI Path for Network Diagnostic Get Request.
 *
 */
#define OT_URI_PATH_DIAGNOSTIC_GET_REQUEST "d/dg"

/**
 * @def OT_URI_PATH_DIAGNOSTIC_GET_QUERY
 *
 * The URI Path for Network Diagnostic Get Query.
 *
 */
#define OT_URI_PATH_DIAGNOSTIC_GET_QUERY "d/dq"

/**
 * @def OT_URI_PATH_DIAGNOSTIC_GET_ANSWER
 *
 * The URI Path for Network Diagnostic Get Answer.
 *
 */
#define OT_URI_PATH_DIAGNOSTIC_GET_ANSWER "d/da"

/**
 * @def OT_URI_PATH_DIAG_RST
 *
 * The URI Path for Network Diagnostic Reset.
 *
 */
#define OT_URI_PATH_DIAGNOSTIC_RESET "d/dr"

} // namespace ot

#endif // THREAD_URIS_HPP_
