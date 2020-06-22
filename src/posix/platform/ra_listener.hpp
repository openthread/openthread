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
 *   This file contains the interface for router advertisement listener.
 */

#ifndef OPENTHREAD_POSIX_RA_LISTENER_HPP_
#define OPENTHREAD_POSIX_RA_LISTENER_HPP_

#include <sys/select.h>

#include <core/openthread-core-config.h>
#include <openthread/border_router.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/openthread-system.h>

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#ifndef OPENTHREAD_POSIX_CONFIG_MAX_ROUTER_ENTRIES_COUNT
#define OPENTHREAD_POSIX_CONFIG_MAX_ROUTER_ENTRIES_COUNT 10
#endif

namespace ot {
namespace Posix {

struct RouterEntry
{
    uint32_t             mPreferTimePoint;
    uint32_t             mValidTimePoint;
    otBorderRouterConfig mConfig;
    bool                 mOccupied = false;
};

class RaListener
{
public:
    /**
     * This constructor is the same as the default constructor.
     *
     * Will not open the file descriptor, call @ref Init for initializing
     *
     */
    RaListener(void);

    /**
     * This method initializes the listener.
     *
     * @param[in] aInterfaceIndex   The interface to listen on for router advertisement packets.
     *
     */
    otError Init(unsigned int aInterfaceIndex);

    /**
     * This method deinitializes the listener.
     *
     * Will close the underlying file descriptor.
     *
     */
    void DeInit(void);

    /**
     * This method updates the file descriptor sets with file descriptor used by the listener.
     *
     * @param[inout]  aContext       A pointer to the mainloop context.
     *
     */
    void UpdateFdSet(otSysMainloopContext *aContext);

    /**
     * This method performs the I/O event processing.
     *
     * @param[in]  aInstance      The OpenThread instance object.
     * @param[in]  aContext       A pointer to the mainloop context.
     *
     */
    otError ProcessEvent(otInstance *aInstance, const otSysMainloopContext *aContext);

    /**
     * This destructor deinitializes the listener.
     *
     * Will close the underlying file descriptor if @ref DeInit is not explicitly closed.
     *
     */
    ~RaListener(void);

private:
    RaListener(const RaListener &) = delete;
    RaListener &operator=(const RaListener &) = delete;

    otError      UpdateRouterEntries(otInstance *aInstance);
    RouterEntry &EliminateEntry(void);
    RouterEntry &GetAvailableRouterEntry(const otIp6Address &aPrefix, uint8_t aLength);

    RouterEntry mRouterEntries[OPENTHREAD_POSIX_CONFIG_MAX_ROUTER_ENTRIES_COUNT];

    int mRaFd;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#endif // OPENTHREAD_POSIX_RA_LISTENER_HPP_
