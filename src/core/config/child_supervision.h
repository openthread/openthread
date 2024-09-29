/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes compile-time configurations for Child Supervision.
 */

#ifndef CONFIG_CHILD_SUPERVISION_H_
#define CONFIG_CHILD_SUPERVISION_H_

/**
 * @addtogroup config-child-supervision
 *
 * @brief
 *   This module includes configuration variables for Child Supervision.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL
 *
 * The default supervision interval in seconds to use when in child state. Zero indicates no supervision needed.
 *
 * The current supervision interval can be changed using `otChildSupervisionSetInterval()`.
 *
 * Child supervision feature provides a mechanism for parent to ensure that a message is sent to each sleepy child
 * within the supervision interval. If there is no transmission to the child within the supervision interval, child
 * supervisor will enqueue and send a supervision message (a data message with empty payload) to the child.
 */
#ifndef OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL 129
#endif

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT
 *
 * The default supervision check timeout interval (in seconds) used by a device in child state. Set to zero to disable
 * the supervision check process on the child.
 *
 * The check timeout interval can be changed using `otChildSupervisionSetCheckTimeout()`.
 *
 * If the sleepy child does not hear from its parent within the specified timeout interval, it initiates the re-attach
 * process (MLE Child Update Request/Response exchange with its parent).
 */
#ifndef OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT 190
#endif

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_OLDER_VERSION_CHILD_DEFAULT_INTERVAL
 *
 * Specifies the default supervision interval to use on parent for children that do not explicitly indicate their
 * desired supervision internal (do not include a "Supervision Interval TLV") and are running older Thread versions
 * (version <= 1.3.0).
 *
 * This config is added to allow backward compatibility on parent with SED children that used Child Supervision
 * feature in OT stack before adoption of it by Thread specification and addition of the "Supervision Interval TLV" as
 * the mechanism for child to inform the parent of its desired supervision interval.
 *
 * The config can be set to zero to effectively disable it, i.e., if a child does not provide "Supervision Interval TLV"
 * it indicates that it does not want to be supervised and then parent will use zero interval for the child.
 */
#ifndef OPENTHREAD_CONFIG_CHILD_SUPERVISION_OLDER_VERSION_CHILD_DEFAULT_INTERVAL
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_OLDER_VERSION_CHILD_DEFAULT_INTERVAL 129
#endif

/**
 * @}
 */

#endif // CONFIG_CHILD_SUPERVISION_H_
