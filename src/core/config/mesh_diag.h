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

/**
 * @file
 *   This file includes compile-time configurations for Mesh Diagnostic module.
 *
 */

#ifndef CONFIG_MESH_DIAG_H_
#define CONFIG_MESH_DIAG_H_

/**
 * @addtogroup config-mesh-diag
 *
 * @brief
 *   This module includes configuration variables for Mesh Diagnostic.
 *
 * @{
 *
 */

#include "config/border_routing.h"

/**
 * @def OPENTHREAD_CONFIG_MESH_DIAG_ENABLE
 *
 * Define to 1 to enable Mesh Diagnostic module.
 *
 * By default this feature is enabled if device is configured to act as Border Router.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESH_DIAG_ENABLE
#define OPENTHREAD_CONFIG_MESH_DIAG_ENABLE OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_MESH_DIAG_RESPONSE_TIMEOUT
 *
 * Specifies the timeout interval in milliseconds waiting for response from router during discover.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESH_DIAG_RESPONSE_TIMEOUT
#define OPENTHREAD_CONFIG_MESH_DIAG_RESPONSE_TIMEOUT 5000
#endif

/**
 * @}
 *
 */

#endif // CONFIG_MESH_DIAG_H_
