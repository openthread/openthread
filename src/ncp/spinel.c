/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  -------------------------------------------------------------------
 *
 *  ## Unit Test ##
 *
 *  This file includes its own unit test. To compile the unit test,
 *  simply compile this file with the macro SPINEL_SELF_TEST set to 1.
 *  For example:
 *
 *      cc spinel.c -Wall -DSPINEL_SELF_TEST=1 -o spinel
 *
 *  -------------------------------------------------------------------
 */

// ----------------------------------------------------------------------------
// MARK: -
// MARK: Headers

#include "spinel.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
// MARK: -

#ifndef assert_printf
#define assert_printf(fmt, ...) \
fprintf(stderr, \
        __FILE__ ":%d: " fmt "\n", \
        __LINE__, \
        __VA_ARGS__)
#endif

#ifndef require_action
#define require_action(c, l, a) \
    do { if (!(c)) { \
        assert_printf("Requirement Failed (%s)", # c); \
        a; \
        goto l; \
    } } while (0)
#endif

#ifndef require
#define require(c, l)   require_action(c, l, {})
#endif

// ----------------------------------------------------------------------------
// MARK: -

spinel_ssize_t
spinel_packed_uint_decode(const uint8_t *bytes, spinel_size_t len, unsigned int *value_ptr)
{
    spinel_ssize_t ret = 0;
    unsigned int value = 0;

    int i = 0;

    do
    {
        if (len < sizeof(uint8_t))
        {
            ret = -1;
            break;
        }

        value |= ((bytes[0] & 0x7F) << i);
        i += 7;
        ret += sizeof(uint8_t);
        bytes += sizeof(uint8_t);
        len -= sizeof(uint8_t);
    }
    while ((bytes[-1] & 0x80) == 0x80);

    if ((ret > 0) && (value_ptr != NULL))
    {
        *value_ptr = value;
    }

    return ret;
}

spinel_ssize_t
spinel_packed_uint_size(unsigned int value)
{
    spinel_size_t ret;

    if (value < (1 << 7))
    {
        ret = 1;
    }
    else if (value < (1 << 14))
    {
        ret = 2;
    }
    else if (value < (1 << 21))
    {
        ret = 3;
    }
    else if (value < (1 << 28))
    {
        ret = 4;
    }
    else
    {
        ret = 5;
    }

    return ret;
}

spinel_ssize_t
spinel_packed_uint_encode(uint8_t *bytes, spinel_size_t len, unsigned int value)
{
    const spinel_ssize_t encoded_size = spinel_packed_uint_size(value);

    if (len >= encoded_size)
    {
        spinel_size_t i;

        for (i = 0; i != encoded_size - 1; ++i)
        {
            *bytes++ = (value & 0x7F) | 0x80;
            value = (value >> 7);
        }

        *bytes++ = (value & 0x7F);
    }

    return encoded_size;
}

const char *
spinel_next_packed_datatype(const char *pack_format)
{
    int depth = 0;

    do
    {
        switch (*++pack_format)
        {
        case '(':
            depth++;
            break;

        case ')':
            depth--;

            if (depth == 0)
            {
                pack_format++;
            }

            break;
        }
    }
    while ((depth > 0) && *pack_format != 0);

    return pack_format;
}

spinel_ssize_t
spinel_datatype_unpack(const uint8_t *data_ptr, spinel_size_t data_len, const char *pack_format, ...)
{
    spinel_ssize_t ret;
    va_list args;
    va_start(args, pack_format);

    ret = spinel_datatype_vunpack(data_ptr, data_len, pack_format, args);

    va_end(args);
    return ret;
}

spinel_ssize_t
spinel_datatype_vunpack(const uint8_t *data_ptr, spinel_size_t data_len, const char *pack_format, va_list args)
{
    spinel_ssize_t ret = 0;

    for (; *pack_format != 0; pack_format = spinel_next_packed_datatype(pack_format))
    {
        if (*pack_format == ')')
        {
            // Don't go past the end of a struct.
            break;
        }

        switch ((spinel_datatype_t)pack_format[0])
        {
        case SPINEL_DATATYPE_BOOL_C:
        {
            bool *arg_ptr = va_arg(args, bool *);
            require_action(data_len >= sizeof(uint8_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = data_ptr[0];
            }

            ret += sizeof(uint8_t);
            data_ptr += sizeof(uint8_t);
            data_len -= sizeof(uint8_t);
            break;
        }

        case SPINEL_DATATYPE_INT8_C:
        case SPINEL_DATATYPE_UINT8_C:
        {
            uint8_t *arg_ptr = va_arg(args, uint8_t *);
            require_action(data_len >= sizeof(uint8_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = data_ptr[0];
            }

            ret += sizeof(uint8_t);
            data_ptr += sizeof(uint8_t);
            data_len -= sizeof(uint8_t);
            break;
        }

        case SPINEL_DATATYPE_INT16_C:
        case SPINEL_DATATYPE_UINT16_C:
        {
            uint16_t *arg_ptr = va_arg(args, uint16_t *);
            require_action(data_len >= sizeof(uint16_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = ((data_ptr[1] << 8) | data_ptr[0]);
            }

            ret += sizeof(uint16_t);
            data_ptr += sizeof(uint16_t);
            data_len -= sizeof(uint16_t);
            break;
        }

        case SPINEL_DATATYPE_INT32_C:
        case SPINEL_DATATYPE_UINT32_C:
        {
            uint32_t *arg_ptr = va_arg(args, uint32_t *);
            require_action(data_len >= sizeof(uint32_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = ((data_ptr[3] << 24) | (data_ptr[2] << 16) | (data_ptr[1] << 8) | data_ptr[0]);
            }

            ret += sizeof(uint32_t);
            data_ptr += sizeof(uint32_t);
            data_len -= sizeof(uint32_t);
            break;
        }

        case SPINEL_DATATYPE_IPv6ADDR_C:
        {
            spinel_ipv6addr_t **arg_ptr = va_arg(args, spinel_ipv6addr_t **);
            require_action(data_len >= sizeof(spinel_ipv6addr_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (spinel_ipv6addr_t *)data_ptr;
            }

            ret += sizeof(spinel_ipv6addr_t);
            data_ptr += sizeof(spinel_ipv6addr_t);
            data_len -= sizeof(spinel_ipv6addr_t);
            break;
        }

        case SPINEL_DATATYPE_EUI64_C:
        {
            spinel_eui64_t **arg_ptr = va_arg(args, spinel_eui64_t **);
            require_action(data_len >= sizeof(spinel_eui64_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (spinel_eui64_t *)data_ptr;
            }

            ret += sizeof(spinel_eui64_t);
            data_ptr += sizeof(spinel_eui64_t);
            data_len -= sizeof(spinel_eui64_t);
            break;
        }

        case SPINEL_DATATYPE_EUI48_C:
        {
            spinel_eui48_t **arg_ptr = va_arg(args, spinel_eui48_t **);
            require_action(data_len >= sizeof(spinel_eui48_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (spinel_eui48_t *)data_ptr;
            }

            ret += sizeof(spinel_eui48_t);
            data_ptr += sizeof(spinel_eui48_t);
            data_len -= sizeof(spinel_eui48_t);
            break;
        }

        case SPINEL_DATATYPE_UINT_PACKED_C:
        {
            uint32_t *arg_ptr = va_arg(args, uint32_t *);
            spinel_ssize_t pui_len = spinel_packed_uint_decode(data_ptr, data_len, arg_ptr);

            require(pui_len > 0, bail);

            require(pui_len <= data_len, bail);

            ret += pui_len;
            data_ptr += pui_len;
            data_len -= pui_len;
            break;
        }

        case SPINEL_DATATYPE_UTF8_C:
        {
            const char **arg_ptr = va_arg(args, const char **);
            ssize_t len = strnlen((const char *)data_ptr, data_len) + 1;
            require_action((len <= data_len) || (data_ptr[data_len - 1] != 0), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (const char *)data_ptr;
            }

            ret += len;
            data_ptr += len;
            data_len -= len;
            break;
        }

        case SPINEL_DATATYPE_DATA_C:
        {
            spinel_ssize_t pui_len = 0;
            uint16_t block_len = 0;
            const uint8_t *block_ptr = data_ptr;
            const uint8_t **block_ptr_ptr =  va_arg(args, const uint8_t **);
            unsigned int *block_len_ptr = va_arg(args, unsigned int *);
            char nextformat = *spinel_next_packed_datatype(pack_format);

            if ((nextformat != 0) && (nextformat != ')'))
            {
                pui_len = spinel_datatype_unpack(data_ptr, data_len, SPINEL_DATATYPE_UINT16_S, &block_len);
                //pui_len = spinel_packed_uint_decode(data_ptr, data_len, &block_len);
                block_ptr += pui_len;

                require(pui_len > 0, bail);
                require(block_len < SPINEL_FRAME_MAX_SIZE, bail);
            }
            else
            {
                block_len = data_len;
                pui_len = 0;
            }

            require_action(data_len >= (block_len + pui_len), bail, (ret = -1, errno = EOVERFLOW));

            if (NULL != block_ptr_ptr)
            {
                *block_ptr_ptr = block_ptr;
            }

            if (NULL != block_len_ptr)
            {
                *block_len_ptr = block_len;
            }

            block_len += pui_len;
            ret += block_len;
            data_ptr += block_len;
            data_len -= block_len;
            break;
        }

        case SPINEL_DATATYPE_STRUCT_C:
        {
            spinel_ssize_t pui_len = 0;
            uint16_t block_len = 0;
            unsigned int actual_len = 0;
            const uint8_t *block_ptr = data_ptr;
            char nextformat = *spinel_next_packed_datatype(pack_format);

            if ((nextformat != 0) && (nextformat != ')'))
            {
                pui_len = spinel_datatype_unpack(data_ptr, data_len, SPINEL_DATATYPE_UINT16_S, &block_len);
                block_ptr += pui_len;

                require(pui_len > 0, bail);
                require(block_len < SPINEL_FRAME_MAX_SIZE, bail);
            }
            else
            {
                block_len = data_len;
                pui_len = 0;
            }

            require_action(data_len >= (block_len + pui_len), bail, (ret = -1, errno = EOVERFLOW));

            actual_len = spinel_datatype_vunpack(block_ptr, block_len, pack_format + 2, args);

            require_action((int)actual_len > -1, bail, (ret = -1, errno = EOVERFLOW));

            if (pui_len)
            {
                block_len += pui_len;
            }
            else
            {
                block_len = actual_len;
            }

            ret += block_len;
            data_ptr += block_len;
            data_len -= block_len;
            break;
        }

        case '.':
            // Skip.
            break;

        case SPINEL_DATATYPE_ARRAY_C:
        default:
            // Unsupported Type!
            ret = -1;
            errno = EINVAL;
            goto bail;
        }
    }

    return ret;

bail:
    return ret;
}

spinel_ssize_t
spinel_datatype_pack(uint8_t *data_ptr, spinel_size_t data_len_max, const char *pack_format, ...)
{
    int ret;
    va_list args;
    va_start(args, pack_format);

    ret = spinel_datatype_vpack(data_ptr, data_len_max, pack_format, args);

    va_end(args);
    return ret;
}

spinel_ssize_t
spinel_datatype_vpack(uint8_t *data_ptr, spinel_size_t data_len_max, const char *pack_format, va_list args)
{
    spinel_ssize_t ret = 0;

    for (; *pack_format != 0; pack_format = spinel_next_packed_datatype(pack_format))
    {
        if (*pack_format == ')')
        {
            // Don't go past the end of a struct.
            break;
        }

        switch ((spinel_datatype_t)*pack_format)
        {
        case SPINEL_DATATYPE_BOOL_C:
        {
            bool arg = va_arg(args, int);
            ret += sizeof(uint8_t);

            if (data_len_max >= sizeof(uint8_t))
            {
                data_ptr[0] = (arg != false);
                data_ptr += sizeof(uint8_t);
                data_len_max -= sizeof(uint8_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_INT8_C:
        case SPINEL_DATATYPE_UINT8_C:
        {
            uint8_t arg = va_arg(args, int);
            ret += sizeof(uint8_t);

            if (data_len_max >= sizeof(uint8_t))
            {
                data_ptr[0] = arg;
                data_ptr += sizeof(uint8_t);
                data_len_max -= sizeof(uint8_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_INT16_C:
        case SPINEL_DATATYPE_UINT16_C:
        {
            uint16_t arg = va_arg(args, int);
            ret += sizeof(uint16_t);

            if (data_len_max >= sizeof(uint16_t))
            {
                data_ptr[1] = (arg >> 8);
                data_ptr[0] = (arg >> 0);
                data_ptr += sizeof(uint16_t);
                data_len_max -= sizeof(uint16_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_INT32_C:
        case SPINEL_DATATYPE_UINT32_C:
        {
            uint32_t arg = va_arg(args, int);
            ret += sizeof(uint32_t);

            if (data_len_max >= sizeof(uint32_t))
            {
                data_ptr[3] = (arg >> 24);
                data_ptr[2] = (arg >> 16);
                data_ptr[1] = (arg >> 8);
                data_ptr[0] = (arg >> 0);
                data_ptr += sizeof(uint32_t);
                data_len_max -= sizeof(uint32_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_IPv6ADDR_C:
        {
            spinel_ipv6addr_t *arg = va_arg(args, spinel_ipv6addr_t *);
            ret += sizeof(spinel_ipv6addr_t);

            if (data_len_max >= sizeof(spinel_ipv6addr_t))
            {
                *(spinel_ipv6addr_t *)data_ptr = *arg;
                data_ptr += sizeof(spinel_ipv6addr_t);
                data_len_max -= sizeof(spinel_ipv6addr_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_EUI48_C:
        {
            spinel_eui48_t *arg = va_arg(args, spinel_eui48_t *);
            ret += sizeof(spinel_eui48_t);

            if (data_len_max >= sizeof(spinel_eui48_t))
            {
                *(spinel_eui48_t *)data_ptr = *arg;
                data_ptr += sizeof(spinel_eui48_t);
                data_len_max -= sizeof(spinel_eui48_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_EUI64_C:
        {
            spinel_eui64_t *arg = va_arg(args, spinel_eui64_t *);
            ret += sizeof(spinel_eui64_t);

            if (data_len_max >= sizeof(spinel_eui64_t))
            {
                *(spinel_eui64_t *)data_ptr = *arg;
                data_ptr += sizeof(spinel_eui64_t);
                data_len_max -= sizeof(spinel_eui64_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_UINT_PACKED_C:
        {
            uint32_t arg = va_arg(args, uint32_t);
            spinel_ssize_t encoded_size = spinel_packed_uint_encode(data_ptr, data_len_max, arg);
            ret += encoded_size;

            if (data_len_max >= encoded_size)
            {
                data_ptr += encoded_size;
                data_len_max -= encoded_size;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_UTF8_C:
        {
            const char *string_arg = va_arg(args, const char *);
            size_t string_arg_len = 0;

            if (string_arg)
            {
                string_arg_len = strlen(string_arg) + 1;
            }
            else
            {
                string_arg = "";
                string_arg_len = 1;
            }

            ret += string_arg_len;

            if (data_len_max >= string_arg_len)
            {
                memcpy(data_ptr, string_arg, string_arg_len);

                data_ptr += string_arg_len;
                data_len_max -= string_arg_len;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_DATA_C:
        {
            uint8_t *arg = va_arg(args, uint8_t *);
            uint32_t data_size_arg = va_arg(args, uint32_t);
            spinel_ssize_t size_len = 0;
            char nextformat = *spinel_next_packed_datatype(pack_format);

            if (nextformat != 0 && nextformat != ')')
            {
                size_len = spinel_datatype_pack(data_ptr, data_len_max, SPINEL_DATATYPE_UINT16_S, data_size_arg);
                require_action(size_len > 0, bail, {ret = -1; errno = EINVAL;});
            }

            ret += size_len + data_size_arg;

            if (data_len_max >= size_len + data_size_arg)
            {
                data_ptr += size_len;
                data_len_max -= size_len;

                memcpy(data_ptr, arg, data_size_arg);

                data_ptr += data_size_arg;
                data_len_max -= data_size_arg;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_STRUCT_C:
        {
            spinel_ssize_t struct_len = 0;
            spinel_ssize_t size_len = 0;
            char nextformat = *spinel_next_packed_datatype(pack_format);

            require_action(pack_format[1] == '(', bail, {ret = -1; errno = EINVAL;});

            // First we figure out the size of the struct
            {
                va_list subargs;
                va_copy(subargs, args);
                struct_len = spinel_datatype_vpack(NULL, 0, pack_format + 2, subargs);
                va_end(subargs);
            }

            if (nextformat != 0 && nextformat != ')')
            {
                size_len = spinel_datatype_pack(data_ptr, data_len_max, SPINEL_DATATYPE_UINT16_S, struct_len);
                require_action(size_len > 0, bail, {ret = -1; errno = EINVAL;});
            }

            ret += size_len + struct_len;

            if (struct_len + size_len <= data_len_max)
            {
                data_ptr += size_len;
                data_len_max -= size_len;

                struct_len = spinel_datatype_vpack(data_ptr, data_len_max, pack_format + 2, args);

                data_ptr += struct_len;
                data_len_max -= struct_len;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case '.':
            // Skip.
            break;

        default:
            // Unsupported Type!
            ret = -1;
            errno = EINVAL;
            goto bail;

        }
    }

bail:
    return ret;
}

// ----------------------------------------------------------------------------
// MARK: -

const char *
spinel_prop_key_to_cstr(spinel_prop_key_t prop_key)
{
    const char *ret = "UNKNOWN";

    switch (prop_key)
    {
    case SPINEL_PROP_LAST_STATUS:
        ret = "PROP_LAST_STATUS";
        break;

    case SPINEL_PROP_PROTOCOL_VERSION:
        ret = "PROP_PROTOCOL_VERSION";
        break;

    case SPINEL_PROP_INTERFACE_TYPE:
        ret = "PROP_INTERFACE_TYPE";
        break;

    case SPINEL_PROP_VENDOR_ID:
        ret = "PROP_VENDOR_ID";
        break;

    case SPINEL_PROP_CAPS:
        ret = "PROP_CAPS";
        break;

    case SPINEL_PROP_NCP_VERSION:
        ret = "PROP_NCP_VERSION";
        break;

    case SPINEL_PROP_INTERFACE_COUNT:
        ret = "PROP_INTERFACE_COUNT";
        break;

    case SPINEL_PROP_POWER_STATE:
        ret = "PROP_POWER_STATE";
        break;

    case SPINEL_PROP_HWADDR:
        ret = "PROP_HWADDR";
        break;

    case SPINEL_PROP_LOCK:
        ret = "PROP_LOCK";
        break;

    case SPINEL_PROP_HBO_MEM_MAX:
        ret = "PROP_HBO_MEM_MAX";
        break;

    case SPINEL_PROP_HBO_BLOCK_MAX:
        ret = "PROP_HBO_BLOCK_MAX";
        break;

    case SPINEL_PROP_STREAM_DEBUG:
        ret = "PROP_STREAM_DEBUG";
        break;

    case SPINEL_PROP_STREAM_RAW:
        ret = "PROP_STREAM_RAW";
        break;

    case SPINEL_PROP_STREAM_NET:
        ret = "PROP_STREAM_NET";
        break;

    case SPINEL_PROP_STREAM_NET_INSECURE:
        ret = "PROP_STREAM_NET_INSECURE";
        break;

    case SPINEL_PROP_PHY_ENABLED:
        ret = "PROP_PHY_ENABLED";
        break;

    case SPINEL_PROP_PHY_CHAN:
        ret = "PROP_PHY_CHAN";
        break;

    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        ret = "PROP_PHY_CHAN_SUPPORTED";
        break;

    case SPINEL_PROP_PHY_FREQ:
        ret = "PROP_PHY_FREQ";
        break;

    case SPINEL_PROP_PHY_CCA_THRESHOLD:
        ret = "PROP_PHY_CCA_THRESHOLD";
        break;

    case SPINEL_PROP_PHY_TX_POWER:
        ret = "PROP_PHY_TX_POWER";
        break;

    case SPINEL_PROP_PHY_RSSI:
        ret = "PROP_PHY_RSSI";
        break;

    case SPINEL_PROP_PHY_RAW_STREAM_ENABLED:
        ret = "PROP_PHY_RAW_STREAM_ENABLED";
        break;

    case SPINEL_PROP_PHY_MODE:
        ret = "PROP_PHY_MODE";
        break;

    case SPINEL_PROP_MAC_SCAN_STATE:
        ret = "PROP_MAC_SCAN_STATE";
        break;

    case SPINEL_PROP_MAC_SCAN_MASK:
        ret = "PROP_MAC_SCAN_MASK";
        break;

    case SPINEL_PROP_MAC_SCAN_BEACON:
        ret = "PROP_MAC_SCAN_BEACON";
        break;

    case SPINEL_PROP_MAC_SCAN_PERIOD:
        ret = "PROP_MAC_SCAN_PERIOD";
        break;

    case SPINEL_PROP_MAC_15_4_LADDR:
        ret = "PROP_MAC_15_4_LADDR";
        break;

    case SPINEL_PROP_MAC_15_4_SADDR:
        ret = "PROP_MAC_15_4_SADDR";
        break;

    case SPINEL_PROP_MAC_15_4_PANID:
        ret = "PROP_MAC_15_4_PANID";
        break;

    case SPINEL_PROP_NET_SAVED:
        ret = "PROP_NET_SAVED";
        break;

    case SPINEL_PROP_NET_ENABLED:
        ret = "PROP_NET_ENABLED";
        break;

    case SPINEL_PROP_NET_STATE:
        ret = "PROP_NET_STATE";
        break;

    case SPINEL_PROP_NET_ROLE:
        ret = "PROP_NET_ROLE";
        break;

    case SPINEL_PROP_NET_NETWORK_NAME:
        ret = "PROP_NET_NETWORK_NAME";
        break;

    case SPINEL_PROP_NET_XPANID:
        ret = "PROP_NET_XPANID";
        break;

    case SPINEL_PROP_NET_MASTER_KEY:
        ret = "PROP_NET_MASTER_KEY";
        break;

    case SPINEL_PROP_NET_KEY_SEQUENCE:
        ret = "PROP_NET_KEY_SEQUENCE";
        break;

    case SPINEL_PROP_NET_PARTITION_ID:
        ret = "PROP_NET_PARTITION_ID";
        break;

    case SPINEL_PROP_THREAD_LEADER_ADDR:
        ret = "PROP_THREAD_LEADER_ADDR";
        break;

    case SPINEL_PROP_THREAD_LEADER_RID:
        ret = "PROP_THREAD_LEADER_RID";
        break;

    case SPINEL_PROP_THREAD_LEADER_WEIGHT:
        ret = "PROP_THREAD_LEADER_WEIGHT";
        break;

    case SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT:
        ret = "PROP_THREAD_LOCAL_LEADER_WEIGHT";
        break;

    case SPINEL_PROP_THREAD_NETWORK_DATA_VERSION:
        ret = "PROP_THREAD_NETWORK_DATA_VERSION";
        break;

    case SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION:
        ret = "PROP_THREAD_STABLE_NETWORK_DATA_VERSION";
        break;

    case SPINEL_PROP_THREAD_NETWORK_DATA:
        ret = "PROP_THREAD_NETWORK_DATA";
        break;

    case SPINEL_PROP_THREAD_CHILD_TABLE:
        ret = "PROP_THREAD_CHILD_TABLE";
        break;

    case SPINEL_PROP_IPV6_LL_ADDR:
        ret = "PROP_IPV6_LL_ADDR";
        break;

    case SPINEL_PROP_IPV6_ML_ADDR:
        ret = "PROP_IPV6_ML_ADDR";
        break;

    case SPINEL_PROP_IPV6_ML_PREFIX:
        ret = "PROP_IPV6_ML_PREFIX";
        break;

    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        ret = "PROP_IPV6_ADDRESS_TABLE";
        break;

    case SPINEL_PROP_IPV6_ROUTE_TABLE:
        ret = "PROP_IPV6_ROUTE_TABLE";
        break;

    case SPINEL_PROP_IPV6_EXT_ROUTE_TABLE:
        ret = "PROP_IPV6_EXT_ROUTE_TABLE";
        break;

    default:
        break;
    }

    return ret;
}


/* -------------------------------------------------------------------------- */

#if SPINEL_SELF_TEST

#include <stdlib.h>
#include <string.h>


int
main(void)
{
    int ret = -1;

    const char static_string[] = "static_string";
    uint8_t buffer[1024];
    ssize_t len;

    len = spinel_datatype_pack(buffer, sizeof(buffer), "CiiLU", 0x88, 9, 0xA3, 0xDEADBEEF, static_string);

    if (len != 22)
    {
        printf("error:%d: len != 22; (%d)\n", __LINE__, (int)len);
        goto bail;
    }

    {
        uint8_t c = 0;
        unsigned int i1 = 0;
        unsigned int i2 = 0;
        uint32_t l = 0;
        const char *str = NULL;

        len = spinel_datatype_unpack(buffer, (spinel_size_t)len, "CiiLU", &c, &i1, &i2, &l, &str);

        if (len != 22)
        {
            printf("error:%d: len != 22; (%d)\n", __LINE__, (int)len);
            goto bail;
        }

        if (c != 0x88)
        {
            printf("error: x != 0x88; (%d)\n", c);
            goto bail;
        }

        if (i1 != 9)
        {
            printf("error: i1 != 9; (%d)\n", i1);
            goto bail;
        }

        if (i2 != 0xA3)
        {
            printf("error: i2 != 0xA3; (0x%02X)\n", i2);
            goto bail;
        }

        if (l != 0xDEADBEEF)
        {
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }
    }

    // -----------------------------------

    memset(buffer, 0xAA, sizeof(buffer));

    len = spinel_datatype_pack(buffer, sizeof(buffer), "CiT(iL)U", 0x88, 9, 0xA3, 0xDEADBEEF, static_string);

    if (len != 24)
    {
        printf("error:%d: len != 24; (%d)\n", __LINE__, (int)len);
        goto bail;
    }



    {
        uint8_t c = 0;
        unsigned int i1 = 0;
        unsigned int i2 = 0;
        uint32_t l = 0;
        const char *str = NULL;

        len = spinel_datatype_unpack(buffer, (spinel_size_t)len, "CiT(iL)U", &c, &i1, &i2, &l, &str);

        if (len != 24)
        {
            printf("error:%d: len != 24; (%d)\n", __LINE__, (int)len);
            goto bail;
        }

        if (c != 0x88)
        {
            printf("error: x != 0x88; (%d)\n", c);
            goto bail;
        }

        if (i1 != 9)
        {
            printf("error: i1 != 9; (%d)\n", i1);
            goto bail;
        }

        if (i2 != 0xA3)
        {
            printf("error: i2 != 0xA3; (0x%02X)\n", i2);
            goto bail;
        }

        if (l != 0xDEADBEEF)
        {
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }
    }



    printf("OK\n");
    ret = 0;
    return ret;

bail:
    printf("FAILURE\n");
    return ret;
}

#endif // #if SPINEL_SELF_TEST
