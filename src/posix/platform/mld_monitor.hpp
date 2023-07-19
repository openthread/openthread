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

#ifndef POSIX_PLATFORM_MLD_MONITOR_HPP_
#define POSIX_PLATFORM_MLD_MONITOR_HPP_

#include "openthread-posix-config.h"

#include "core/common/non_copyable.hpp"
#include "posix/platform/mainloop.hpp"
#include "posix/platform/platform_base.hpp"

#include <openthread/openthread-system.h>

#if OPENTHREAD_POSIX_USE_MLD_MONITOR

namespace ot {
namespace Posix {

class MldMonitor : public PlatformBase, public Mainloop::Source, private NonCopyable
{
public:
    /**
     * Returns the singleton object of this class.
     *
     */
    static MldMonitor &Get(void);

    // Implements PlatformBase

    void SetUp(otInstance *aInstance) override;
    void TearDown(void) override;

    // Implements Mainloop::Source

    void Update(otSysMainloopContext &aContext) override;
    void Process(const otSysMainloopContext &aContext) override;

private:
    int mFd = -1;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_USE_MLD_MONITOR

#endif // POSIX_PLATFORM_MLD_MONITOR_HPP_
