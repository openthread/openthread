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
 *   This file includes functions for debugging.
 */

#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include "thread/mle.hpp"

namespace ot {
namespace Settings {

/**
 * Rules for updating existing value structures.
 *
 * 1. Modifying existing otPlatSettings* key value fields MUST only be
 *    done by appending new fields.  Existing fields MUST NOT be
 *    deleted or modified in any way.
 *
 * 2. To support backward compatibility (rolling back to an older
 *    software version), code reading and processing key values MUST
 *    process key values that have longer length.  Additionally, newer
 *    versions MUST update/maintain values in existing key value
 *    fields.
 *
 * 3. To support forward compatibility (rolling forward to a newer
 *    software version), code reading and processing key values MUST
 *    process key values that have shorter length.
 *
 * 4. New Key IDs may be defined in the future with the understanding
 *    that such key values are not backward compatible.
 *
 */

/**
 * This enumeration defines the keys of settings
 *
 */
enum
{
    kKeyActiveDataset   = 0x0001,  ///< Active Operational Dataset
    kKeyPendingDataset  = 0x0002,  ///< Pending Operational Dataset
    kKeyNetworkInfo     = 0x0003,  ///< Thread network information
    kKeyParentInfo      = 0x0004,  ///< Parent information
    kKeyChildInfo       = 0x0005,  ///< Child information
    kKeyThreadAutoStart = 0x0006,  ///< Auto-start information
};

/**
 * This structure represents the device's own network information for settings storage.
 *
 */
struct NetworkInfo
{
    uint8_t          mRole;                    ///< Current Thread role.
    uint8_t          mDeviceMode;              ///< Device mode setting.
    uint16_t         mRloc16;                  ///< RLOC16
    uint32_t         mKeySequence;             ///< Key Sequence
    uint32_t         mMleFrameCounter;         ///< MLE Frame Counter
    uint32_t         mMacFrameCounter;         ///< MAC Frame Counter
    uint32_t         mPreviousPartitionId;     ///< PartitionId
    Mac::ExtAddress  mExtAddress;              ///< Extended Address
    uint8_t          mMlIid[OT_IP6_IID_SIZE];  ///< IID from ML-EID
};

/**
 * This structure represents the parent information for settings storage.
 *
 */
struct ParentInfo
{
    Mac::ExtAddress  mExtAddress;   ///< Extended Address
};

/**
 * This structure represents the child information for settings storage.
 *
 */
struct ChildInfo
{
    Mac::ExtAddress  mExtAddress;    ///< Extended Address
    uint32_t         mTimeout;       ///< Timeout
    uint16_t         mRloc16;        ///< RLOC16
    uint8_t          mMode;          ///< The MLE device mode
};

}  // namespace Settings
}  // namespace ot

#endif  // SETTINGS_HPP_
