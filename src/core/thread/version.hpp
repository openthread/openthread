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
 *   This file includes definitions for Thread Version.
 */

#ifndef VERSION_HPP_
#define VERSION_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

namespace ot {

constexpr uint16_t kThreadVersion = OPENTHREAD_CONFIG_THREAD_VERSION; ///< Thread Version of this device.

constexpr uint16_t kThreadVersion1p1 = OT_THREAD_VERSION_1_1; ///< Thread Version 1.1
constexpr uint16_t kThreadVersion1p2 = OT_THREAD_VERSION_1_2; ///< Thread Version 1.2
constexpr uint16_t kThreadVersion1p3 = OT_THREAD_VERSION_1_3; ///< Thread Version 1.3
// Support projects on legacy "1.3.1" version, which is now "1.4"
constexpr uint16_t kThreadVersion1p3p1 = OT_THREAD_VERSION_1_3_1; ///< Thread Version 1.3.1
constexpr uint16_t kThreadVersion1p4   = OT_THREAD_VERSION_1_4;   ///< Thread Version 1.4

} // namespace ot

#endif // VERSION_HPP_
