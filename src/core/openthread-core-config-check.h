/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   Sanity checking for configuration options.
 */

#ifndef OPENTHREAD_CORE_CONFIG_CHECK_H_
#define OPENTHREAD_CORE_CONFIG_CHECK_H_

/*
 * Removed or replaced OPENTHREAD_CONFIG options.
 *
 */

#ifdef OPENTHREAD_CONFIG_DISABLE_CCA_ON_LAST_ATTEMPT
#error \
    "OPENTHREAD_CONFIG_DISABLE_CCA_ON_LAST_ATTEMPT was replaced by OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT."
#endif

#ifdef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT
#error "OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT was replaced by OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT."
#endif

#ifdef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL
#error \
    "OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL was replaced by OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT."
#endif

#endif // OPENTHREAD_CORE_CONFIG_CHECK_H_
