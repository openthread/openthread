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

#ifndef POSIX_PLATFORM_RADIO_URL_HPP_
#define POSIX_PLATFORM_RADIO_URL_HPP_

#include <stdio.h>
#include <stdlib.h>

#include <openthread/openthread-system.h>

struct otPosixRadioArguments
{
    const char *mPath; ///< The path to the executable or device
};

namespace ot {
namespace Posix {

class Arguments : public otPosixRadioArguments
{
public:
    Arguments(const char *aUrl);

    /**
     * This method gets the path in url
     *
     * @returns The path in url.
     *
     */
    const char *GetPath(void) const { return mPath; }

    /**
     * This method returns the url agument value.
     *
     * @param[in] aName       Argument name.
     * @param[in] aLastValue  The last iterated argument value, NULL for first value.
     *
     * @returns The argument value.
     *
     */
    const char *GetValue(const char *aName, const char *aLastValue = NULL);

private:
    enum
    {
        kRadioUrlMaxSize = 512,
    };
    char mUrl[kRadioUrlMaxSize];

    char *mStart;
    char *mEnd;
};

} // namespace Posix
} // namespace ot

#endif // POSIX_PLATFORM_RADIO_URL_HPP_
