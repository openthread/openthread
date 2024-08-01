/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the SRP (Service Registration Protocol) Client.
 *
 */

#ifndef CONFIG_SRP_CODER_H_
#define CONFIG_SRP_CODER_H_

/**
 * @addtogroup config-srp-coder
 *
 * @brief
 *   This module includes configuration variables for the SRP Coder.
 *
 * @{
 *
 */

/**
 * @def OPENTHREAD_CONFIG_SRP_CODER_ENABLE
 *
 * Define to 1 to enable SRP coder feature.
 *
 * The SRP Coder can be used to encode an SRP message into a compact, compressed format, reducing the message size. The
 * received coded message can be decoded (on server) to reconstruct an exact replica of the original SRP message.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CODER_ENABLE
#define OPENTHREAD_CONFIG_SRP_CODER_ENABLE (OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE)
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CODER_USE_BY_CLIENT_ENABLE
 *
 * Specifies the default behavior of SRP client whether or not it can use of SRP coder to send coded SRP update
 * message.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CODER_USE_BY_CLIENT_ENABLE
#define OPENTHREAD_CONFIG_SRP_CODER_USE_BY_CLIENT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
 *
 * Define to 1 to enable SRP coder test APIs.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
#define OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE 0
#endif

/**
 * @}
 *
 */

#endif // CONFIG_SRP_CODER_H_
