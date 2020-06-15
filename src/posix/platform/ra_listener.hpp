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

#include <openthread/config.h>
#include <openthread/error.h>
#include <openthread/instance.h>

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

namespace ot {
namespace Posix {

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
     * @param[inout]  aReadFdSet   A reference to the read file descriptors.
     * @param[inout]  aWriteFdSet  A reference to the write file descriptors.
     * @param[inout]  aErrorFdSet  A reference to the error file descriptors.
     * @param[inout]  aMaxFd       A reference to the max file descriptor.
     * @param[inout]  aTimeout     A reference to the timeout.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, fd_set &aErrorFdSet, int &aMaxFd);

    /**
     * This method performs the I/O event processing.
     *
     * @param[in]   aInstance      The OpenThread instance object.
     * @param[in]  aReadFdSet      A reference to the read file descriptors.
     * @param[in]  aWriteFdSet     A reference to the write file descriptors.
     * @param[in]  aErrorFdSet     A reference to the error file descriptors.
     *
     */
    otError ProcessEvent(otInstance *  aInstance,
                         const fd_set &aReadFdSet,
                         const fd_set &aWriteFdSet,
                         const fd_set &aErrorFdSet);

    /**
     * This destructor deinitializes the listener.
     *
     * Will close the underlying file descriptor if @ref DeInit is not explicitly closed.
     *
     */
    ~RaListener(void);

private:
    RaListener(const RaListener &);
    RaListener &operator=(const RaListener &);

    int mRaFd;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#endif // OPENTHREAD_POSIX_RA_LISTENER_HPP_
