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
 * @brief
 *  This file defines the OpenThread Network Data Publisher API.
 */

#ifndef OPENTHREAD_NETDATA_PUBLISHER_H_
#define OPENTHREAD_NETDATA_PUBLISHER_H_

#include <openthread/netdata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 *
 * The Network Data Publisher provides mechanisms to limit the number of similar DNS/SRP service entries in the Thread
 * Network Data by monitoring the Network Data and managing if or when to add or remove entries.
 *
 * All the functions in this module require `OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE` to be enabled.
 *
 */

/**
 * This function requests "DNS/SRP Service Anycast Address" to be published in the Thread Network Data.
 *
 * This function requires the feature `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` to be enabled.
 *
 * A call to this function will remove and replace any previous "DNS/SRP Service" entry that was being published (from
 * earlier call to any of `otNetDataPublishDnsSrpService{Type}()` functions).
 *
 * @param[in] aInstance        A pointer to an OpenThread instance.
 * @param[in] aSequenceNUmber  The sequence number of DNS/SRP Anycast Service.
 *
 */
void otNetDataPublishDnsSrpServiceAnycast(otInstance *aInstance, uint8_t aSequenceNUmber);

/**
 * This function requests "DNS/SRP Service Unicast Address" to be published in the Thread Network Data.
 *
 * This function requires the feature `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` to be enabled.
 *
 * A call to this function will remove and replace any previous "DNS/SRP Service" entry that was being published (from
 * earlier call to any of `otNetDataPublishDnsSrpService{Type}()` functions).
 *
 * This function publishes the "DNS/SRP Service Unicast Address" by including the address and port info in the Service
 * TLV data.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aAddress   The DNS/SRP server address to publish (MUST NOT be NULL).
 * @param[in] aPort      The SRP server port number to publish.
 *
 */
void otNetDataPublishDnsSrpServiceUnicast(otInstance *aInstance, const otIp6Address *aAddress, uint16_t aPort);

/**
 * This function requests "DNS/SRP Service Unicast Address" to be published in the Thread Network Data.
 *
 * This function requires the feature `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` to be enabled.
 *
 * A call to this function will remove and replace any previous "DNS/SRP Service" entry that was being published (from
 * earlier call to any of `otNetDataPublishDnsSrpService{Type}()` functions).
 *
 * Unlike `otNetDataPublishDnsSrpServiceUnicast()` which requires the published address to be given and includes the
 * info in the Service TLV data, this function uses the device's mesh-local EID and includes the info in the Server TLV
 * data.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aPort      The SRP server port number to publish.
 *
 */
void otNetDataPublishDnsSrpServiceUnicastMeshLocalEid(otInstance *aInstance, uint16_t aPort);

/**
 * This function unpublishes any previously added "DNS/SRP (Anycast or Unicast) Service" entry from the Thread Network
 * Data.
 *
 * This function requires the feature `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` to be enabled.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 */
void otNetDataUnpublishDnsSrpService(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_NETDATA_PUBLISHER_H_
