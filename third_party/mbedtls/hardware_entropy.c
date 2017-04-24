/*
 *    Copyright 2016 The OpenThread Authors. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @file
 *   This file implements to use hardware entropy source.
 *   uncomment MBEDTLS_ENTROPY_HARDWARE_ALT in mbedtls/mbedtls_config.h to use this.
 */

#include <stddef.h>

#include "openthread/types.h"
#include "openthread/platform/random.h"

#include "mbedtls/entropy.h"

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    ThreadError error;

    (void)data;

    error = otPlatRandomSecureGet((uint16_t)len, (uint8_t *)output, (uint16_t *)olen);

    if (error != kThreadError_None)
    {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    return 0;
}
