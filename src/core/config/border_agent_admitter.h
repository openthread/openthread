/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the Border Agent Admitter.
 */

#ifndef OT_CORE_CONFIG_BORDER_AGENT_ADMITTER_H_
#define OT_CORE_CONFIG_BORDER_AGENT_ADMITTER_H_

/**
 * @addtogroup config-border-agent
 *
 * @brief
 *   This module includes configuration variables for the Border Agent Admitter.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE
 *
 * Define to 1 to enable Border Agent's Admitter feature.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLED_BY_DEFAULT
 *
 * Define to 1 to enable the Border Admitter by default upon OpenThread stack initialization.
 *
 * Applicable when `OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLED_BY_DEFAULT
#define OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLED_BY_DEFAULT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_DEFAULT_JOINER_UDP_PORT
 *
 * Specifies the default Joiner UDP port used by the Border Admitter.
 *
 * A value of zero indicates that the Joiner UDP port is not specified/fixed by the Admitter, allowing Joiner Routers
 * to pick.
 *
 * Applicable when `OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_DEFAULT_JOINER_UDP_PORT
#define OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_DEFAULT_JOINER_UDP_PORT 0
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_BORDER_AGENT_ADMITTER_H_
