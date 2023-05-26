/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for managing the Extended PAN ID.
 *
 */

#ifndef MESHCOP_EXTENDED_PANID_HPP_
#define MESHCOP_EXTENDED_PANID_HPP_

#include "openthread-core-config.h"

#include <openthread/dataset.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/string.hpp"

namespace ot {
namespace MeshCoP {

/**
 * Represents an Extended PAN Identifier.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedPanId : public otExtendedPanId, public Equatable<ExtendedPanId>, public Clearable<ExtendedPanId>
{
public:
    static constexpr uint16_t kInfoStringSize = 17; ///< Max chars for the info string (`ToString()`).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Converts an address to a string.
     *
     * @returns An `InfoString` containing the string representation of the Extended PAN Identifier.
     *
     */
    InfoString ToString(void) const;

} OT_TOOL_PACKED_END;

class ExtendedPanIdManager : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit ExtendedPanIdManager(Instance &aInstance);

    /**
     * Returns the Extended PAN Identifier.
     *
     * @returns The Extended PAN Identifier.
     *
     */
    const ExtendedPanId &GetExtPanId(void) const { return mExtendedPanId; }

    /**
     * Sets the Extended PAN Identifier.
     *
     * @param[in]  aExtendedPanId  The Extended PAN Identifier.
     *
     */
    void SetExtPanId(const ExtendedPanId &aExtendedPanId);

private:
    static const otExtendedPanId sExtendedPanidInit;

    ExtendedPanId mExtendedPanId;
};

} // namespace MeshCoP

DefineCoreType(otExtendedPanId, MeshCoP::ExtendedPanId);

} // namespace ot

#endif // MESHCOP_EXTENDED_PANID_HPP_
