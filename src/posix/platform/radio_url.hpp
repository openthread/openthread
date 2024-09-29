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

#ifndef OT_POSIX_PLATFORM_RADIO_URL_HPP_
#define OT_POSIX_PLATFORM_RADIO_URL_HPP_

#include <stdio.h>
#include <stdlib.h>

#include <openthread/openthread-system.h>

#include "lib/url/url.hpp"

namespace ot {
namespace Posix {

/**
 * Implements the radio URL processing.
 */
class RadioUrl : public ot::Url::Url
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]   aUrl    The null-terminated URL string.
     */
    explicit RadioUrl(const char *aUrl) { Init(aUrl); };

    /**
     * Initializes the radio URL.
     *
     * @param[in]   aUrl    The null-terminated URL string.
     */
    void Init(const char *aUrl);

    RadioUrl(const RadioUrl &)            = delete;
    RadioUrl(RadioUrl &&)                 = delete;
    RadioUrl &operator=(const RadioUrl &) = delete;
    RadioUrl &operator=(RadioUrl &&)      = delete;

private:
    enum
    {
        kRadioUrlMaxSize = 512,
    };
    char mUrl[kRadioUrlMaxSize];
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_RADIO_URL_HPP_
