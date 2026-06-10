/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes definitions for the mainloop events and manager.
 */

#ifndef OT_POSIX_PLATFORM_MAINLOOP_HPP_
#define OT_POSIX_PLATFORM_MAINLOOP_HPP_

#include <openthread/openthread-system.h>

namespace ot {
namespace Posix {
namespace Mainloop {

typedef otSysMainloopContext Context; ///<  Represents a `Mainloop` context.

/**
 * Adds a file descriptor to the read set in the mainloop context.
 *
 * If the file descriptor @p aFd is valid (non-negative), this method adds it to `aContext.mReadFdSet` and
 * updates `aContext.mMaxFd` if @p aFd is larger than the current max. If @p aFd is negative, no action is taken.
 *
 * @param[in]      aFd       The file descriptor to add.
 * @param[in,out]  aContext  A reference to the mainloop context.
 */
void AddToReadFdSet(int aFd, Context &aContext);

/**
 * Adds a file descriptor to the write set in the mainloop context.
 *
 * If the file descriptor @p aFd is valid (non-negative), this method adds it to `aContext.mWriteFdSet` and
 * updates `aContext.mMaxFd` if @p aFd is larger than the current max. If @p aFd is negative, no action is taken.
 *
 * @param[in]      aFd       The file descriptor to add.
 * @param[in,out]  aContext  A reference to the mainloop context.
 */
void AddToWriteFdSet(int aFd, Context &aContext);

/**
 * Adds a file descriptor to the error set in the mainloop context.
 *
 * If the file descriptor @p aFd is valid (non-negative), this method adds it to `aContext.mErrorFdSet` and
 * updates `aContext.mMaxFd` if @p aFd is larger than the current max. If @p aFd is negative, no action is taken.
 *
 * @param[in]      aFd       The file descriptor to add.
 * @param[in,out]  aContext  A reference to the mainloop context.
 */
void AddToErrorFdSet(int aFd, Context &aContext);

/**
 * Checks if a file descriptor is in the read set of the mainloop context.
 *
 * This is intended to be called after the `select()` call has returned.
 *
 * @param[in]  aFd       The file descriptor to check.
 * @param[in]  aContext  A reference to the mainloop context.
 *
 * @returns `true` if the given file descriptor is readable, `false` otherwise.
 */
inline bool IsFdReadable(int aFd, const Context &aContext) { return FD_ISSET(aFd, &aContext.mReadFdSet); }

/**
 * Checks if a file descriptor is in the write set of the mainloop context.
 *
 * This is intended to be called after the `select()` call has returned.
 *
 * @param[in]  aFd       The file descriptor to check.
 * @param[in]  aContext  A reference to the mainloop context.
 *
 * @returns `true` if the given file descriptor is writable, `false` otherwise.
 */
inline bool IsFdWritable(int aFd, const Context &aContext) { return FD_ISSET(aFd, &aContext.mWriteFdSet); }

/**
 * Checks if a file descriptor is in the error set of the mainloop context.
 *
 * This is intended to be called after the `select()` call has returned.
 *
 * @param[in]  aFd       The file descriptor to check.
 * @param[in]  aContext  A reference to the mainloop context.
 *
 * @returns `true` if the given file descriptor has an error, `false` otherwise.
 */
inline bool HasFdErrored(int aFd, const Context &aContext) { return FD_ISSET(aFd, &aContext.mErrorFdSet); }

/**
 * Gets the current timeout value from the mainloop context.
 *
 * @param[in]  aContext  A reference to the mainloop context.
 *
 * @returns The timeout value.
 */
uint64_t GetTimeout(const Context &aContext);

/**
 * Sets the timeout in the mainloop context if the new timeout is earlier than the existing one.
 *
 * This method compares `aTimeout` with the current timeout in `aContext` and updates the context's
 * timeout to `aTimeout` if it is smaller (earlier).
 *
 * @param[in]      aTimeout  The new timeout value to potentially set.
 * @param[in,out]  aContext  A reference to the mainloop context.
 */
void SetTimeoutIfEarlier(uint64_t aTimeout, Context &aContext);

/**
 * Is the base for all mainloop event sources.
 */
class Source
{
    friend class Manager;

public:
    /**
     * Registers events in the mainloop.
     *
     * @param[in,out]   aContext    A reference to the mainloop context.
     */
    virtual void Update(Context &aContext) = 0;

    /**
     * Processes the mainloop events.
     *
     * @param[in]   aContext    A reference to the mainloop context.
     */
    virtual void Process(const Context &aContext) = 0;

    /**
     * Marks destructor virtual method.
     */
    virtual ~Source(void) = default;

private:
    static void AddFd(int aFd, Context &aContext, fd_set &aFdSet);

    Source *mNext = nullptr;
};

/**
 * Manages mainloop.
 */
class Manager
{
public:
    /**
     * Updates event polls in the mainloop context.
     *
     * @param[in,out]   aContext    A reference to the mainloop context.
     */
    void Update(Context &aContext);

    /**
     * Processes events in the mainloop context.
     *
     * @param[in]   aContext    A reference to the mainloop context.
     */
    void Process(const Context &aContext);

    /**
     * Adds a new event source into the mainloop.
     *
     * @param[in]   aSource     A reference to the event source.
     */
    void Add(Source &aSource);

    /**
     * Removes an event source from the mainloop.
     *
     * @param[in]   aSource     A reference to the event source.
     */
    void Remove(Source &aSource);

    /**
     * Returns the Mainloop singleton.
     *
     * @returns A reference to the Mainloop singleton.
     */
    static Manager &Get(void);

private:
    Source *mSources = nullptr;
};

} // namespace Mainloop
} // namespace Posix
} // namespace ot
#endif // OT_POSIX_PLATFORM_MAINLOOP_HPP_
