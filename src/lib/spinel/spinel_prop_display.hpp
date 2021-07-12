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
 * This file ...
 */

#ifndef SPINEL_PROP_DISPLAY_HPP_
#define SPINEL_PROP_DISPLAY_HPP_

#include <inttypes.h>
#include <stdio.h>

#include "spinel.h"

#include "common/code_utils.hpp"

namespace ot {
namespace Spinel {

/**
 * This function prints a basic spinel data type (not a STRUCT or an ARRAY) to a buffer.
 *
 * @param[in]   aDataType   The data type.
 * @param[in]   aPtrArgs    If the argument in the argument list is a pointer to the value or a value.
 * @param[in]   aArgs       A value identifying a variable arguments list.
 * @param[out]  aBuf        The output buffer.
 * @param[in]   aBufSize    The size of the output buffer.
 *
 * @returns If written successfully, it returns the length of bytes written. Otherwise a negative error code is
 * returned.
 *
 */
int32_t SpinelPropDisplaySimpleDataType(spinel_datatype_t aDataType,
                                        bool              aPtrArgs,
                                        va_list           aArgs,
                                        char *            aBuf,
                                        uint16_t          aBufSize)
{
    int32_t ret   = 0;
    int32_t bytes = 0;

    VerifyOrExit(aBuf != nullptr, ret = -1);

    switch (aDataType)
    {
    case SPINEL_DATATYPE_BOOL_C:
    {
        bool val = aPtrArgs ? *va_arg(aArgs, bool *) : static_cast<bool>(va_arg(aArgs, int));
        bytes    = snprintf(aBuf, aBufSize, "(BOOL: %d)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_UINT8_C:
    {
        uint8_t val = aPtrArgs ? (*va_arg(aArgs, uint8_t *)) : static_cast<uint8_t>(va_arg(aArgs, int));
        bytes       = snprintf(aBuf, aBufSize, "(UINT8: %u)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_INT8_C:
    {
        int8_t val = aPtrArgs ? (*va_arg(aArgs, int8_t *)) : static_cast<int8_t>(va_arg(aArgs, int));
        bytes      = snprintf(aBuf, aBufSize, "(INT8: %d)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_UINT16_C:
    {
        uint16_t val = aPtrArgs ? (*va_arg(aArgs, uint16_t *)) : static_cast<uint16_t>(va_arg(aArgs, int));
        bytes        = snprintf(aBuf, aBufSize, "(UINT16: %u)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_INT16_C:
    {
        int16_t val = aPtrArgs ? (*va_arg(aArgs, int16_t *)) : static_cast<int16_t>(va_arg(aArgs, int));
        bytes       = snprintf(aBuf, aBufSize, "(INT16: %d)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_UINT32_C:
    {
        uint32_t val = aPtrArgs ? (*va_arg(aArgs, uint32_t *)) : va_arg(aArgs, uint32_t);
        bytes        = snprintf(aBuf, aBufSize, "(UINT32: %u)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_INT32_C:
    {
        int32_t val = aPtrArgs ? (*va_arg(aArgs, int32_t *)) : va_arg(aArgs, int32_t);
        bytes       = snprintf(aBuf, aBufSize, "(INT32: %d)", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_UINT64_C:
    {
        uint64_t val = aPtrArgs ? (*va_arg(aArgs, uint64_t *)) : va_arg(aArgs, uint64_t);
        bytes        = snprintf(aBuf, aBufSize, "(UINT64: %" PRIu64 ")", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_INT64_C:
    {
        int64_t val = aPtrArgs ? (*va_arg(aArgs, int64_t *)) : va_arg(aArgs, int64_t);
        bytes       = snprintf(aBuf, aBufSize, "(INT64: %" PRId64 ")", val);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_IPv6ADDR_C:
    {
        spinel_ipv6addr_t *aArgsPtr = va_arg(aArgs, spinel_ipv6addr_t *);
        bytes =
            snprintf(aBuf, aBufSize, "(IPv6: %x:%x:%x:%x:%x:%x:%x:%x)", (aArgsPtr->bytes[0] << 8) | aArgsPtr->bytes[1],
                     (aArgsPtr->bytes[2] << 8) | aArgsPtr->bytes[3], (aArgsPtr->bytes[4] << 8) | aArgsPtr->bytes[5],
                     (aArgsPtr->bytes[6] << 8) | aArgsPtr->bytes[7], (aArgsPtr->bytes[8] << 8) | aArgsPtr->bytes[9],
                     (aArgsPtr->bytes[10] << 8) | aArgsPtr->bytes[11], (aArgsPtr->bytes[12] << 8) | aArgsPtr->bytes[13],
                     (aArgsPtr->bytes[14] << 8) | aArgsPtr->bytes[15]);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_EUI48_C:
    {
        spinel_eui48_t *aArgsPtr = va_arg(aArgs, spinel_eui48_t *);
        bytes = snprintf(aBuf, aBufSize, "(EUI48: %x:%x:%x:%x:%x:%x)", aArgsPtr->bytes[0], aArgsPtr->bytes[1],
                         aArgsPtr->bytes[2], aArgsPtr->bytes[3], aArgsPtr->bytes[4], aArgsPtr->bytes[5]);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_EUI64_C:
    {
        spinel_eui64_t *aArgsPtr = va_arg(aArgs, spinel_eui64_t *);
        bytes = snprintf(aBuf, aBufSize, "(EUI64: %x:%x:%x:%x:%x:%x:%x:%x)", aArgsPtr->bytes[0], aArgsPtr->bytes[1],
                         aArgsPtr->bytes[2], aArgsPtr->bytes[3], aArgsPtr->bytes[4], aArgsPtr->bytes[5],
                         aArgsPtr->bytes[6], aArgsPtr->bytes[7]);
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
        break;
    }

    case SPINEL_DATATYPE_UTF8_C:
    {
        uint8_t *data    = va_arg(aArgs, uint8_t *);
        size_t   len_arg = va_arg(aArgs, size_t);
        size_t   n;

        bytes = snprintf(aBuf, aBufSize, "(UTF8: ");
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);

        for (uint8_t i = 0, n = 0; i < len_arg; i++)
        {
            n = snprintf(aBuf + bytes, aBufSize - bytes, "%02x", data[i]);
            VerifyOrExit(n > 0 && n < aBufSize - bytes, ret = -1);
            bytes += n;
        }

        n = snprintf(aBuf + bytes, aBufSize - bytes, ")");
        VerifyOrExit(n > 0 && n < aBufSize - bytes, ret = -1);
        bytes += n;

        break;
    }

    case SPINEL_DATATYPE_DATA_C:
    {
        uint8_t *data    = va_arg(aArgs, uint8_t *);
        size_t   len_arg = va_arg(aArgs, size_t);
        size_t   n;

        bytes = snprintf(aBuf, aBufSize, "(DATA: ");
        VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);

        for (uint8_t i = 0, n = 0; i < len_arg; i++)
        {
            n = snprintf(aBuf + bytes, aBufSize - bytes, "%02x", data[i]);
            VerifyOrExit(n > 0 && n < aBufSize - bytes, ret = -1);
            bytes += n;
        }

        n = snprintf(aBuf + bytes, aBufSize - bytes, ")");
        VerifyOrExit(n > 0 && n < aBufSize - bytes, ret = -1);
        bytes += n;

        break;
    }

    default:
    {
        ExitNow(ret = -1);
        break;
    }
    }

    ret = bytes;

exit:
    return ret;
}

/**
 * This function prints a spinel property operated with Get/Set method to a buffer.
 *
 * @param[in]   aKey           The aKey of the spinel property.
 * @param[in]   aPackFormat    C string that contains a format string follows the same specification of spinel.
 * @param[in]   aPtrArgs       If the argument in the argument list is a pointer to the value or a value.
 * @param[in]   aArgs          A value identifying a variable arguments list.
 * @param[out]  aBuf           The output aBuffer.
 * @param[in]   aBufSize       The size of the output aBuffer.
 *
 * @returns If written successfully, it returns the length of bytes written. Otherwise a negative error code is
 * returned.
 *
 */
int32_t SpinelPropDisplay(spinel_prop_key_t aKey,
                          const char *      aPackFormat,
                          bool              aPtrArgs,
                          va_list           aArgs,
                          char *            aBuf,
                          uint16_t          aBufSize)
{
    int32_t ret   = 0;
    int32_t bytes = 0;
    int8_t  depth = 0;

    VerifyOrExit(aPackFormat != nullptr && aBuf != nullptr, ret = -1);

    bytes = snprintf(aBuf, aBufSize, "%s: ", spinel_prop_key_to_cstr(aKey));
    VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
    ret += bytes;
    aBuf += bytes;
    aBufSize -= bytes;

    for (; *aPackFormat != 0; aPackFormat++)
    {
        if (*aPackFormat == SPINEL_DATATYPE_STRUCT_C)
        {
            VerifyOrExit(*(++aPackFormat) == '(', ret = -1);
            depth++;
            bytes = snprintf(aBuf, aBufSize, "{");
            VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
            ret += bytes;
            aBuf += bytes;
            aBufSize -= bytes;
        }
        else if (*aPackFormat == ')')
        {
            VerifyOrExit(--depth >= 0, ret = -1);
            bytes = snprintf(aBuf, aBufSize, "}");
            VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
            ret += bytes;
            aBuf += bytes;
            aBufSize -= bytes;
        }
        else
        {
            bytes = SpinelPropDisplaySimpleDataType(*aPackFormat, aPtrArgs, aArgs, aBuf, aBufSize);
            VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
            ret += bytes;
            aBuf += bytes;
            aBufSize -= bytes;
        }

        if (aPackFormat[0] != '(' && aPackFormat[1] != 0 && aPackFormat[1] != ')')
        {
            bytes = snprintf(aBuf, aBufSize, ", ");
            VerifyOrExit(bytes > 0 && bytes < aBufSize, ret = -1);
            ret += bytes;
            aBuf += bytes;
            aBufSize -= bytes;
        }
    }

    VerifyOrExit(depth == 0, ret = -1);

exit:
    return ret;
}

} // namespace Spinel
} // namespace ot

#endif // SPINEL_PROP_DISPLAY_HPP_
