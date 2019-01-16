/*
 *    Copyright (c) 2016, The OpenThread Authors.
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

#include <errno.h>
#include <limits.h>

#ifndef SPINEL_PLATFORM_HEADER
/* These are all already included in the spinel platform header
 * if SPINEL_PLATFORM_HEADER was defined.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif // #ifndef SPINEL_PLATFORM_HEADER

// ----------------------------------------------------------------------------
// MARK: -

// IAR's errno.h apparently doesn't define EOVERFLOW.
#ifndef EOVERFLOW
// There is no real good choice for what to set
// errno to in this case, so we just pick the
// value '1' somewhat arbitrarily.
#define EOVERFLOW 1
#endif

// IAR's errno.h apparently doesn't define EINVAL.
#ifndef EINVAL
// There is no real good choice for what to set
// errno to in this case, so we just pick the
// value '1' somewhat arbitrarily.
#define EINVAL 1
#endif

// IAR's errno.h apparently doesn't define ENOMEM.
#ifndef ENOMEM
// There is no real good choice for what to set
// errno to in this case, so we just pick the
// value '1' somewhat arbitrarily.
#define ENOMEM 1
#endif

#ifdef _KERNEL_MODE
#define va_copy(destination, source) ((destination) = (source))
#undef errno
#define assert_printf(fmt, ...)
#endif

#if defined(errno) && SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR
#error "SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR is set but errno is already defined."
#endif

// Work-around for platforms that don't implement the `errno` variable.
#if !defined(errno) && SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR
static int spinel_errno_workaround_;
#define errno spinel_errno_workaround_
#endif // SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR

#ifndef assert_printf
#if SPINEL_PLATFORM_DOESNT_IMPLEMENT_FPRINTF
#define assert_printf(fmt, ...) printf(__FILE__ ":%d: " fmt "\n", __LINE__, __VA_ARGS__)
#else // if SPINEL_PLATFORM_DOESNT_IMPLEMENT_FPRINTF
#define assert_printf(fmt, ...) fprintf(stderr, __FILE__ ":%d: " fmt "\n", __LINE__, __VA_ARGS__)
#endif // else SPINEL_PLATFORM_DOESNT_IMPLEMENT_FPRINTF
#endif

#ifndef require_action
#if SPINEL_PLATFORM_SHOULD_LOG_ASSERTS
#define require_action(c, l, a)                           \
    do                                                    \
    {                                                     \
        if (!(c))                                         \
        {                                                 \
            assert_printf("Requirement Failed (%s)", #c); \
            a;                                            \
            goto l;                                       \
        }                                                 \
    } while (0)
#else // if DEBUG
#define require_action(c, l, a) \
    do                          \
    {                           \
        if (!(c))               \
        {                       \
            a;                  \
            goto l;             \
        }                       \
    } while (0)
#endif // else DEBUG
#endif // ifndef require_action

#ifndef require
#define require(c, l) require_action(c, l, {})
#endif

typedef struct
{
    va_list obj;
} va_list_obj;

#define SPINEL_MAX_PACK_LENGTH 32767

// ----------------------------------------------------------------------------
// MARK: -

spinel_ssize_t spinel_packed_uint_decode(const uint8_t *bytes, spinel_size_t len, unsigned int *value_ptr)
{
    spinel_ssize_t ret   = 0;
    unsigned int   value = 0;

    unsigned int i = 0;

    do
    {
        if ((len < sizeof(uint8_t)) || (i >= sizeof(unsigned int) * CHAR_BIT))
        {
            ret = -1;
            break;
        }

        value |= (unsigned int)(bytes[0] & 0x7F) << i;
        i += 7;
        ret += sizeof(uint8_t);
        bytes += sizeof(uint8_t);
        len -= sizeof(uint8_t);
    } while ((bytes[-1] & 0x80) == 0x80);

    if ((ret > 0) && (value_ptr != NULL))
    {
        *value_ptr = value;
    }

    return ret;
}

spinel_ssize_t spinel_packed_uint_size(unsigned int value)
{
    spinel_ssize_t ret;

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

spinel_ssize_t spinel_packed_uint_encode(uint8_t *bytes, spinel_size_t len, unsigned int value)
{
    const spinel_ssize_t encoded_size = spinel_packed_uint_size(value);

    if ((spinel_ssize_t)len >= encoded_size)
    {
        spinel_ssize_t i;

        for (i = 0; i != encoded_size - 1; ++i)
        {
            *bytes++ = (value & 0x7F) | 0x80;
            value    = (value >> 7);
        }

        *bytes++ = (value & 0x7F);
    }

    return encoded_size;
}

const char *spinel_next_packed_datatype(const char *pack_format)
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
    } while ((depth > 0) && *pack_format != 0);

    return pack_format;
}

static spinel_ssize_t spinel_datatype_vunpack_(bool           in_place,
                                               const uint8_t *data_ptr,
                                               spinel_size_t  data_len,
                                               const char *   pack_format,
                                               va_list_obj *  args)
{
    spinel_ssize_t ret = 0;

    // Buffer length sanity check
    require_action(data_len <= SPINEL_MAX_PACK_LENGTH, bail, (ret = -1, errno = EINVAL));

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
            bool *arg_ptr = va_arg(args->obj, bool *);
            require_action(data_len >= sizeof(uint8_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = data_ptr[0] != 0;
            }

            ret += sizeof(uint8_t);
            data_ptr += sizeof(uint8_t);
            data_len -= sizeof(uint8_t);
            break;
        }

        case SPINEL_DATATYPE_INT8_C:
        case SPINEL_DATATYPE_UINT8_C:
        {
            uint8_t *arg_ptr = va_arg(args->obj, uint8_t *);
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
            uint16_t *arg_ptr = va_arg(args->obj, uint16_t *);
            require_action(data_len >= sizeof(uint16_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (uint16_t)((data_ptr[1] << 8) | data_ptr[0]);
            }

            ret += sizeof(uint16_t);
            data_ptr += sizeof(uint16_t);
            data_len -= sizeof(uint16_t);
            break;
        }

        case SPINEL_DATATYPE_INT32_C:
        case SPINEL_DATATYPE_UINT32_C:
        {
            uint32_t *arg_ptr = va_arg(args->obj, uint32_t *);
            require_action(data_len >= sizeof(uint32_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                *arg_ptr = (uint32_t)((data_ptr[3] << 24) | (data_ptr[2] << 16) | (data_ptr[1] << 8) | data_ptr[0]);
            }

            ret += sizeof(uint32_t);
            data_ptr += sizeof(uint32_t);
            data_len -= sizeof(uint32_t);
            break;
        }

        case SPINEL_DATATYPE_INT64_C:
        case SPINEL_DATATYPE_UINT64_C:
        {
            uint64_t *arg_ptr = va_arg(args->obj, uint64_t *);
            require_action(data_len >= sizeof(uint64_t), bail, (ret = -1, errno = EOVERFLOW));

            if (arg_ptr)
            {
                uint32_t l32 = (uint32_t)((data_ptr[3] << 24) | (data_ptr[2] << 16) | (data_ptr[1] << 8) | data_ptr[0]);
                uint32_t h32 = (uint32_t)((data_ptr[7] << 24) | (data_ptr[6] << 16) | (data_ptr[5] << 8) | data_ptr[4]);

                *arg_ptr = ((uint64_t)l32) | (((uint64_t)h32) << 32);
            }

            ret += sizeof(uint64_t);
            data_ptr += sizeof(uint64_t);
            data_len -= sizeof(uint64_t);
            break;
        }

        case SPINEL_DATATYPE_IPv6ADDR_C:
        {
            require_action(data_len >= sizeof(spinel_ipv6addr_t), bail, (ret = -1, errno = EOVERFLOW));

            if (in_place)
            {
                spinel_ipv6addr_t *arg = va_arg(args->obj, spinel_ipv6addr_t *);
                if (arg)
                {
                    memcpy(arg, data_ptr, sizeof(spinel_ipv6addr_t));
                }
            }
            else
            {
                const spinel_ipv6addr_t **arg_ptr = va_arg(args->obj, const spinel_ipv6addr_t **);
                if (arg_ptr)
                {
                    *arg_ptr = (const spinel_ipv6addr_t *)data_ptr;
                }
            }

            ret += sizeof(spinel_ipv6addr_t);
            data_ptr += sizeof(spinel_ipv6addr_t);
            data_len -= sizeof(spinel_ipv6addr_t);
            break;
        }

        case SPINEL_DATATYPE_EUI64_C:
        {
            require_action(data_len >= sizeof(spinel_eui64_t), bail, (ret = -1, errno = EOVERFLOW));

            if (in_place)
            {
                spinel_eui64_t *arg = va_arg(args->obj, spinel_eui64_t *);
                if (arg)
                {
                    memcpy(arg, data_ptr, sizeof(spinel_eui64_t));
                }
            }
            else
            {
                const spinel_eui64_t **arg_ptr = va_arg(args->obj, const spinel_eui64_t **);
                if (arg_ptr)
                {
                    *arg_ptr = (const spinel_eui64_t *)data_ptr;
                }
            }

            ret += sizeof(spinel_eui64_t);
            data_ptr += sizeof(spinel_eui64_t);
            data_len -= sizeof(spinel_eui64_t);
            break;
        }

        case SPINEL_DATATYPE_EUI48_C:
        {
            require_action(data_len >= sizeof(spinel_eui48_t), bail, (ret = -1, errno = EOVERFLOW));

            if (in_place)
            {
                spinel_eui48_t *arg = va_arg(args->obj, spinel_eui48_t *);
                if (arg)
                {
                    memcpy(arg, data_ptr, sizeof(spinel_eui48_t));
                }
            }
            else
            {
                const spinel_eui48_t **arg_ptr = va_arg(args->obj, const spinel_eui48_t **);
                if (arg_ptr)
                {
                    *arg_ptr = (const spinel_eui48_t *)data_ptr;
                }
            }

            ret += sizeof(spinel_eui48_t);
            data_ptr += sizeof(spinel_eui48_t);
            data_len -= sizeof(spinel_eui48_t);
            break;
        }

        case SPINEL_DATATYPE_UINT_PACKED_C:
        {
            unsigned int * arg_ptr = va_arg(args->obj, unsigned int *);
            spinel_ssize_t pui_len = spinel_packed_uint_decode(data_ptr, data_len, arg_ptr);

            // Range check
            require_action(NULL == arg_ptr || (*arg_ptr < SPINEL_MAX_UINT_PACKED), bail, (ret = -1, errno = ERANGE));

            require(pui_len > 0, bail);

            require(pui_len <= (spinel_ssize_t)data_len, bail);

            ret += pui_len;
            data_ptr += pui_len;
            data_len -= (spinel_size_t)pui_len;
            break;
        }

        case SPINEL_DATATYPE_UTF8_C:
        {
            size_t len;

            // Make sure we have at least one byte.
            require_action(data_len > 0, bail, (ret = -1, errno = EOVERFLOW));

            // Add 1 for zero termination. If not zero terminated,
            // len will then be data_len+1, which we will detect
            // in the next check.
            len = strnlen((const char *)data_ptr, data_len) + 1;

            // Verify that the string is zero terminated.
            require_action(len <= data_len, bail, (ret = -1, errno = EOVERFLOW));

            if (in_place)
            {
                char * arg     = va_arg(args->obj, char *);
                size_t len_arg = va_arg(args->obj, size_t);
                if (arg)
                {
                    require_action(len_arg >= len, bail, (ret = -1, errno = ENOMEM));
                    memcpy(arg, data_ptr, len);
                }
            }
            else
            {
                const char **arg_ptr = va_arg(args->obj, const char **);
                if (arg_ptr)
                {
                    *arg_ptr = (const char *)data_ptr;
                }
            }

            ret += (spinel_size_t)len;
            data_ptr += len;
            data_len -= (spinel_size_t)len;
            break;
        }

        case SPINEL_DATATYPE_DATA_C:
        case SPINEL_DATATYPE_DATA_WLEN_C:
        {
            spinel_ssize_t pui_len       = 0;
            uint16_t       block_len     = 0;
            const uint8_t *block_ptr     = data_ptr;
            void *         arg_ptr       = va_arg(args->obj, void *);
            unsigned int * block_len_ptr = va_arg(args->obj, unsigned int *);
            char           nextformat    = *spinel_next_packed_datatype(pack_format);

            if ((pack_format[0] == SPINEL_DATATYPE_DATA_WLEN_C) || ((nextformat != 0) && (nextformat != ')')))
            {
                pui_len = spinel_datatype_unpack(data_ptr, data_len, SPINEL_DATATYPE_UINT16_S, &block_len);
                block_ptr += pui_len;

                require(pui_len > 0, bail);
                require(block_len < SPINEL_FRAME_MAX_SIZE, bail);
            }
            else
            {
                block_len = (uint16_t)data_len;
                pui_len   = 0;
            }

            require_action((spinel_ssize_t)data_len >= (block_len + pui_len), bail, (ret = -1, errno = EOVERFLOW));

            if (in_place)
            {
                require_action(NULL != block_len_ptr && *block_len_ptr >= block_len, bail, (ret = -1, errno = EINVAL));
                memcpy(arg_ptr, block_ptr, block_len);
            }
            else
            {
                const uint8_t **block_ptr_ptr = (const uint8_t **)arg_ptr;
                if (NULL != block_ptr_ptr)
                {
                    *block_ptr_ptr = block_ptr;
                }
            }

            if (NULL != block_len_ptr)
            {
                *block_len_ptr = block_len;
            }

            block_len += (uint16_t)pui_len;
            ret += block_len;
            data_ptr += block_len;
            data_len -= block_len;
            break;
        }

        case 'T':
        case SPINEL_DATATYPE_STRUCT_C:
        {
            spinel_ssize_t pui_len    = 0;
            uint16_t       block_len  = 0;
            spinel_ssize_t actual_len = 0;
            const uint8_t *block_ptr  = data_ptr;
            char           nextformat = *spinel_next_packed_datatype(pack_format);

            if ((pack_format[0] == SPINEL_DATATYPE_STRUCT_C) || ((nextformat != 0) && (nextformat != ')')))
            {
                pui_len = spinel_datatype_unpack(data_ptr, data_len, SPINEL_DATATYPE_UINT16_S, &block_len);
                block_ptr += pui_len;

                require(pui_len > 0, bail);
                require(block_len < SPINEL_FRAME_MAX_SIZE, bail);
            }
            else
            {
                block_len = (uint16_t)data_len;
                pui_len   = 0;
            }

            require_action((spinel_ssize_t)data_len >= (block_len + pui_len), bail, (ret = -1, errno = EOVERFLOW));

            actual_len = spinel_datatype_vunpack_(false, block_ptr, block_len, pack_format + 2, args);

            require_action(actual_len > -1, bail, (ret = -1, errno = EOVERFLOW));

            if (pui_len)
            {
                block_len += (uint16_t)pui_len;
            }
            else
            {
                block_len = (uint16_t)actual_len;
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
            ret   = -1;
            errno = EINVAL;
            goto bail;
        }
    }

    return ret;

bail:
    return ret;
}

spinel_ssize_t spinel_datatype_unpack_in_place(const uint8_t *data_ptr,
                                               spinel_size_t  data_len,
                                               const char *   pack_format,
                                               ...)
{
    spinel_ssize_t ret;
    va_list_obj    args;
    va_start(args.obj, pack_format);

    ret = spinel_datatype_vunpack_(true, data_ptr, data_len, pack_format, &args);

    va_end(args.obj);
    return ret;
}

spinel_ssize_t spinel_datatype_unpack(const uint8_t *data_ptr, spinel_size_t data_len, const char *pack_format, ...)
{
    spinel_ssize_t ret;
    va_list_obj    args;
    va_start(args.obj, pack_format);

    ret = spinel_datatype_vunpack_(false, data_ptr, data_len, pack_format, &args);

    va_end(args.obj);
    return ret;
}

spinel_ssize_t spinel_datatype_vunpack_in_place(const uint8_t *data_ptr,
                                                spinel_size_t  data_len,
                                                const char *   pack_format,
                                                va_list        args)
{
    spinel_ssize_t ret;
    va_list_obj    args_obj;
    va_copy(args_obj.obj, args);

    ret = spinel_datatype_vunpack_(true, data_ptr, data_len, pack_format, &args_obj);

    va_end(args_obj.obj);
    return ret;
}

spinel_ssize_t spinel_datatype_vunpack(const uint8_t *data_ptr,
                                       spinel_size_t  data_len,
                                       const char *   pack_format,
                                       va_list        args)
{
    spinel_ssize_t ret;
    va_list_obj    args_obj;
    va_copy(args_obj.obj, args);

    ret = spinel_datatype_vunpack_(false, data_ptr, data_len, pack_format, &args_obj);

    va_end(args_obj.obj);
    return ret;
}

static spinel_ssize_t spinel_datatype_vpack_(uint8_t *     data_ptr,
                                             spinel_size_t data_len_max,
                                             const char *  pack_format,
                                             va_list_obj * args)
{
    spinel_ssize_t ret = 0;

    // Buffer length sanity check
    require_action(data_len_max <= SPINEL_MAX_PACK_LENGTH, bail, (ret = -1, errno = EINVAL));

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
            bool arg = (bool)va_arg(args->obj, int);
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
            uint8_t arg = (uint8_t)va_arg(args->obj, int);
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
            uint16_t arg = (uint16_t)va_arg(args->obj, int);
            ret += sizeof(uint16_t);

            if (data_len_max >= sizeof(uint16_t))
            {
                data_ptr[1] = (arg >> 8) & 0xff;
                data_ptr[0] = (arg >> 0) & 0xff;
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
            uint32_t arg = (uint32_t)va_arg(args->obj, int);
            ret += sizeof(uint32_t);

            if (data_len_max >= sizeof(uint32_t))
            {
                data_ptr[3] = (arg >> 24) & 0xff;
                data_ptr[2] = (arg >> 16) & 0xff;
                data_ptr[1] = (arg >> 8) & 0xff;
                data_ptr[0] = (arg >> 0) & 0xff;
                data_ptr += sizeof(uint32_t);
                data_len_max -= sizeof(uint32_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_INT64_C:
        case SPINEL_DATATYPE_UINT64_C:
        {
            uint64_t arg = (uint64_t)va_arg(args->obj, uint64_t);

            ret += sizeof(uint64_t);

            if (data_len_max >= sizeof(uint64_t))
            {
                data_ptr[7] = (arg >> 56) & 0xff;
                data_ptr[6] = (arg >> 48) & 0xff;
                data_ptr[5] = (arg >> 40) & 0xff;
                data_ptr[4] = (arg >> 32) & 0xff;
                data_ptr[3] = (arg >> 24) & 0xff;
                data_ptr[2] = (arg >> 16) & 0xff;
                data_ptr[1] = (arg >> 8) & 0xff;
                data_ptr[0] = (arg >> 0) & 0xff;
                data_ptr += sizeof(uint64_t);
                data_len_max -= sizeof(uint64_t);
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_IPv6ADDR_C:
        {
            spinel_ipv6addr_t *arg = va_arg(args->obj, spinel_ipv6addr_t *);
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
            spinel_eui48_t *arg = va_arg(args->obj, spinel_eui48_t *);
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
            spinel_eui64_t *arg = va_arg(args->obj, spinel_eui64_t *);
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
            uint32_t       arg = va_arg(args->obj, uint32_t);
            spinel_ssize_t encoded_size;

            // Range Check
            require_action(arg < SPINEL_MAX_UINT_PACKED, bail, {
                ret   = -1;
                errno = EINVAL;
            });

            encoded_size = spinel_packed_uint_encode(data_ptr, data_len_max, arg);
            ret += encoded_size;

            if ((spinel_ssize_t)data_len_max >= encoded_size)
            {
                data_ptr += encoded_size;
                data_len_max -= (spinel_size_t)encoded_size;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_UTF8_C:
        {
            const char *string_arg     = va_arg(args->obj, const char *);
            size_t      string_arg_len = 0;

            if (string_arg)
            {
                string_arg_len = strlen(string_arg) + 1;
            }
            else
            {
                string_arg     = "";
                string_arg_len = 1;
            }

            ret += (spinel_size_t)string_arg_len;

            if (data_len_max >= string_arg_len)
            {
                memcpy(data_ptr, string_arg, string_arg_len);

                data_ptr += string_arg_len;
                data_len_max -= (spinel_size_t)string_arg_len;
            }
            else
            {
                data_len_max = 0;
            }

            break;
        }

        case SPINEL_DATATYPE_DATA_WLEN_C:
        case SPINEL_DATATYPE_DATA_C:
        {
            const uint8_t *arg           = va_arg(args->obj, const uint8_t *);
            uint32_t       data_size_arg = va_arg(args->obj, uint32_t);
            spinel_ssize_t size_len      = 0;
            char           nextformat    = *spinel_next_packed_datatype(pack_format);

            if ((pack_format[0] == SPINEL_DATATYPE_DATA_WLEN_C) || ((nextformat != 0) && (nextformat != ')')))
            {
                size_len = spinel_datatype_pack(data_ptr, data_len_max, SPINEL_DATATYPE_UINT16_S, data_size_arg);
                require_action(size_len > 0, bail, {
                    ret   = -1;
                    errno = EINVAL;
                });
            }

            ret += (spinel_size_t)size_len + data_size_arg;

            if (data_len_max >= (spinel_size_t)size_len + data_size_arg)
            {
                data_ptr += size_len;
                data_len_max -= (spinel_size_t)size_len;

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

        case 'T':
        case SPINEL_DATATYPE_STRUCT_C:
        {
            spinel_ssize_t struct_len = 0;
            spinel_ssize_t size_len   = 0;
            char           nextformat = *spinel_next_packed_datatype(pack_format);

            require_action(pack_format[1] == '(', bail, {
                ret   = -1;
                errno = EINVAL;
            });

            // First we figure out the size of the struct
            {
                va_list_obj subargs;
                va_copy(subargs.obj, args->obj);
                struct_len = spinel_datatype_vpack_(NULL, 0, pack_format + 2, &subargs);
                va_end(subargs.obj);
            }

            if ((pack_format[0] == SPINEL_DATATYPE_STRUCT_C) || ((nextformat != 0) && (nextformat != ')')))
            {
                size_len = spinel_datatype_pack(data_ptr, data_len_max, SPINEL_DATATYPE_UINT16_S, struct_len);
                require_action(size_len > 0, bail, {
                    ret   = -1;
                    errno = EINVAL;
                });
            }

            ret += size_len + struct_len;

            if (struct_len + size_len <= (spinel_ssize_t)data_len_max)
            {
                data_ptr += size_len;
                data_len_max -= (spinel_size_t)size_len;

                struct_len = spinel_datatype_vpack_(data_ptr, data_len_max, pack_format + 2, args);

                data_ptr += struct_len;
                data_len_max -= (spinel_size_t)struct_len;
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
            ret   = -1;
            errno = EINVAL;
            goto bail;
        }
    }

bail:
    return ret;
}

spinel_ssize_t spinel_datatype_pack(uint8_t *data_ptr, spinel_size_t data_len_max, const char *pack_format, ...)
{
    int         ret;
    va_list_obj args;
    va_start(args.obj, pack_format);

    ret = spinel_datatype_vpack_(data_ptr, data_len_max, pack_format, &args);

    va_end(args.obj);
    return ret;
}

spinel_ssize_t spinel_datatype_vpack(uint8_t *     data_ptr,
                                     spinel_size_t data_len_max,
                                     const char *  pack_format,
                                     va_list       args)
{
    int         ret;
    va_list_obj args_obj;
    va_copy(args_obj.obj, args);

    ret = spinel_datatype_vpack_(data_ptr, data_len_max, pack_format, &args_obj);

    va_end(args_obj.obj);
    return ret;
}

// ----------------------------------------------------------------------------
// MARK: -

// **** LCOV_EXCL_START ****

const char *spinel_command_to_cstr(unsigned int command)
{
    const char *ret = "UNKNOWN";

    switch (command)
    {
    case SPINEL_CMD_NOOP:
        ret = "NOOP";
        break;

    case SPINEL_CMD_RESET:
        ret = "RESET";
        break;

    case SPINEL_CMD_PROP_VALUE_GET:
        ret = "PROP_VALUE_GET";
        break;

    case SPINEL_CMD_PROP_VALUE_SET:
        ret = "PROP_VALUE_SET";
        break;

    case SPINEL_CMD_PROP_VALUE_INSERT:
        ret = "PROP_VALUE_INSERT";
        break;

    case SPINEL_CMD_PROP_VALUE_REMOVE:
        ret = "PROP_VALUE_REMOVE";
        break;

    case SPINEL_CMD_PROP_VALUE_IS:
        ret = "PROP_VALUE_IS";
        break;

    case SPINEL_CMD_PROP_VALUE_INSERTED:
        ret = "PROP_VALUE_INSERTED";
        break;

    case SPINEL_CMD_PROP_VALUE_REMOVED:
        ret = "PROP_VALUE_REMOVED";
        break;

    case SPINEL_CMD_NET_SAVE:
        ret = "NET_SAVE";
        break;

    case SPINEL_CMD_NET_CLEAR:
        ret = "NET_CLEAR";
        break;

    case SPINEL_CMD_NET_RECALL:
        ret = "NET_RECALL";
        break;

    case SPINEL_CMD_HBO_OFFLOAD:
        ret = "HBO_OFFLOAD";
        break;

    case SPINEL_CMD_HBO_RECLAIM:
        ret = "HBO_RECLAIM";
        break;

    case SPINEL_CMD_HBO_DROP:
        ret = "HBO_DROP";
        break;

    case SPINEL_CMD_HBO_OFFLOADED:
        ret = "HBO_OFFLOADED";
        break;

    case SPINEL_CMD_HBO_RECLAIMED:
        ret = "HBO_RECLAIMED";
        break;

    case SPINEL_CMD_HBO_DROPPED:
        ret = "HBO_DROPPED";
        break;

    case SPINEL_CMD_PEEK:
        ret = "PEEK";
        break;

    case SPINEL_CMD_PEEK_RET:
        ret = "PEEK_RET";
        break;

    case SPINEL_CMD_POKE:
        ret = "POKE";
        break;

    case SPINEL_CMD_PROP_VALUE_MULTI_GET:
        ret = "PROP_VALUE_MULTI_GET";
        break;

    case SPINEL_CMD_PROP_VALUE_MULTI_SET:
        ret = "PROP_VALUE_MULTI_SET";
        break;

    case SPINEL_CMD_PROP_VALUES_ARE:
        ret = "PROP_VALUES_ARE";
        break;

    default:
        break;
    }

    return ret;
}

const char *spinel_prop_key_to_cstr(spinel_prop_key_t prop_key)
{
    const char *ret = "UNKNOWN";

    switch (prop_key)
    {
    case SPINEL_PROP_LAST_STATUS:
        ret = "LAST_STATUS";
        break;

    case SPINEL_PROP_PROTOCOL_VERSION:
        ret = "PROTOCOL_VERSION";
        break;

    case SPINEL_PROP_NCP_VERSION:
        ret = "NCP_VERSION";
        break;

    case SPINEL_PROP_INTERFACE_TYPE:
        ret = "INTERFACE_TYPE";
        break;

    case SPINEL_PROP_VENDOR_ID:
        ret = "VENDOR_ID";
        break;

    case SPINEL_PROP_CAPS:
        ret = "CAPS";
        break;

    case SPINEL_PROP_INTERFACE_COUNT:
        ret = "INTERFACE_COUNT";
        break;

    case SPINEL_PROP_POWER_STATE:
        ret = "POWER_STATE";
        break;

    case SPINEL_PROP_HWADDR:
        ret = "HWADDR";
        break;

    case SPINEL_PROP_LOCK:
        ret = "LOCK";
        break;

    case SPINEL_PROP_HBO_MEM_MAX:
        ret = "HBO_MEM_MAX";
        break;

    case SPINEL_PROP_HBO_BLOCK_MAX:
        ret = "HBO_BLOCK_MAX";
        break;

    case SPINEL_PROP_HOST_POWER_STATE:
        ret = "HOST_POWER_STATE";
        break;

    case SPINEL_PROP_MCU_POWER_STATE:
        ret = "MCU_POWER_STATE";
        break;

    case SPINEL_PROP_GPIO_CONFIG:
        ret = "GPIO_CONFIG";
        break;

    case SPINEL_PROP_GPIO_STATE:
        ret = "GPIO_STATE";
        break;

    case SPINEL_PROP_GPIO_STATE_SET:
        ret = "GPIO_STATE_SET";
        break;

    case SPINEL_PROP_GPIO_STATE_CLEAR:
        ret = "GPIO_STATE_CLEAR";
        break;

    case SPINEL_PROP_TRNG_32:
        ret = "TRNG_32";
        break;

    case SPINEL_PROP_TRNG_128:
        ret = "TRNG_128";
        break;

    case SPINEL_PROP_TRNG_RAW_32:
        ret = "TRNG_RAW_32";
        break;

    case SPINEL_PROP_UNSOL_UPDATE_FILTER:
        ret = "UNSOL_UPDATE_FILTER";
        break;

    case SPINEL_PROP_UNSOL_UPDATE_LIST:
        ret = "UNSOL_UPDATE_LIST";
        break;

    case SPINEL_PROP_PHY_ENABLED:
        ret = "PHY_ENABLED";
        break;

    case SPINEL_PROP_PHY_CHAN:
        ret = "PHY_CHAN";
        break;

    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        ret = "PHY_CHAN_SUPPORTED";
        break;

    case SPINEL_PROP_PHY_FREQ:
        ret = "PHY_FREQ";
        break;

    case SPINEL_PROP_PHY_CCA_THRESHOLD:
        ret = "PHY_CCA_THRESHOLD";
        break;

    case SPINEL_PROP_PHY_TX_POWER:
        ret = "PHY_TX_POWER";
        break;

    case SPINEL_PROP_PHY_RSSI:
        ret = "PHY_RSSI";
        break;

    case SPINEL_PROP_PHY_RX_SENSITIVITY:
        ret = "PHY_RX_SENSITIVITY";
        break;

    case SPINEL_PROP_PHY_PCAP_ENABLED:
        ret = "PHY_PCAP_ENABLED";
        break;

    case SPINEL_PROP_JAM_DETECT_ENABLE:
        ret = "JAM_DETECT_ENABLE";
        break;

    case SPINEL_PROP_JAM_DETECTED:
        ret = "JAM_DETECTED";
        break;

    case SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD:
        ret = "JAM_DETECT_RSSI_THRESHOLD";
        break;

    case SPINEL_PROP_JAM_DETECT_WINDOW:
        ret = "JAM_DETECT_WINDOW";
        break;

    case SPINEL_PROP_JAM_DETECT_BUSY:
        ret = "JAM_DETECT_BUSY";
        break;

    case SPINEL_PROP_JAM_DETECT_HISTORY_BITMAP:
        ret = "JAM_DETECT_HISTORY_BITMAP";
        break;

    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_INTERVAL:
        ret = "CHANNEL_MONITOR_SAMPLE_INTERVAL";
        break;

    case SPINEL_PROP_CHANNEL_MONITOR_RSSI_THRESHOLD:
        ret = "CHANNEL_MONITOR_RSSI_THRESHOLD";
        break;

    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_WINDOW:
        ret = "CHANNEL_MONITOR_SAMPLE_WINDOW";
        break;

    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_COUNT:
        ret = "CHANNEL_MONITOR_SAMPLE_COUNT";
        break;

    case SPINEL_PROP_CHANNEL_MONITOR_CHANNEL_OCCUPANCY:
        ret = "CHANNEL_MONITOR_CHANNEL_OCCUPANCY";
        break;

    case SPINEL_PROP_MAC_SCAN_STATE:
        ret = "MAC_SCAN_STATE";
        break;

    case SPINEL_PROP_MAC_SCAN_MASK:
        ret = "MAC_SCAN_MASK";
        break;

    case SPINEL_PROP_MAC_SCAN_PERIOD:
        ret = "MAC_SCAN_PERIOD";
        break;

    case SPINEL_PROP_MAC_SCAN_BEACON:
        ret = "MAC_SCAN_BEACON";
        break;

    case SPINEL_PROP_MAC_15_4_LADDR:
        ret = "MAC_15_4_LADDR";
        break;

    case SPINEL_PROP_MAC_15_4_SADDR:
        ret = "MAC_15_4_SADDR";
        break;

    case SPINEL_PROP_MAC_15_4_PANID:
        ret = "MAC_15_4_PANID";
        break;

    case SPINEL_PROP_MAC_RAW_STREAM_ENABLED:
        ret = "MAC_RAW_STREAM_ENABLED";
        break;

    case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
        ret = "MAC_PROMISCUOUS_MODE";
        break;

    case SPINEL_PROP_MAC_ENERGY_SCAN_RESULT:
        ret = "MAC_ENERGY_SCAN_RESULT";
        break;

    case SPINEL_PROP_MAC_DATA_POLL_PERIOD:
        ret = "MAC_DATA_POLL_PERIOD";
        break;

    case SPINEL_PROP_MAC_WHITELIST:
        ret = "MAC_WHITELIST";
        break;

    case SPINEL_PROP_MAC_WHITELIST_ENABLED:
        ret = "MAC_WHITELIST_ENABLED";
        break;

    case SPINEL_PROP_MAC_EXTENDED_ADDR:
        ret = "MAC_EXTENDED_ADDR";
        break;

    case SPINEL_PROP_MAC_SRC_MATCH_ENABLED:
        ret = "MAC_SRC_MATCH_ENABLED";
        break;

    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
        ret = "MAC_SRC_MATCH_SHORT_ADDRESSES";
        break;

    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
        ret = "MAC_SRC_MATCH_EXTENDED_ADDRESSES";
        break;

    case SPINEL_PROP_MAC_BLACKLIST:
        ret = "MAC_BLACKLIST";
        break;

    case SPINEL_PROP_MAC_BLACKLIST_ENABLED:
        ret = "MAC_BLACKLIST_ENABLED";
        break;

    case SPINEL_PROP_MAC_FIXED_RSS:
        ret = "MAC_FIXED_RSS";
        break;

    case SPINEL_PROP_MAC_CCA_FAILURE_RATE:
        ret = "MAC_CCA_FAILURE_RATE";
        break;

    case SPINEL_PROP_NET_SAVED:
        ret = "NET_SAVED";
        break;

    case SPINEL_PROP_NET_IF_UP:
        ret = "NET_IF_UP";
        break;

    case SPINEL_PROP_NET_STACK_UP:
        ret = "NET_STACK_UP";
        break;

    case SPINEL_PROP_NET_ROLE:
        ret = "NET_ROLE";
        break;

    case SPINEL_PROP_NET_NETWORK_NAME:
        ret = "NET_NETWORK_NAME";
        break;

    case SPINEL_PROP_NET_XPANID:
        ret = "NET_XPANID";
        break;

    case SPINEL_PROP_NET_MASTER_KEY:
        ret = "NET_MASTER_KEY";
        break;

    case SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER:
        ret = "NET_KEY_SEQUENCE_COUNTER";
        break;

    case SPINEL_PROP_NET_PARTITION_ID:
        ret = "NET_PARTITION_ID";
        break;

    case SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING:
        ret = "NET_REQUIRE_JOIN_EXISTING";
        break;

    case SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME:
        ret = "NET_KEY_SWITCH_GUARDTIME";
        break;

    case SPINEL_PROP_NET_PSKC:
        ret = "NET_PSKC";
        break;

    case SPINEL_PROP_THREAD_LEADER_ADDR:
        ret = "THREAD_LEADER_ADDR";
        break;

    case SPINEL_PROP_THREAD_PARENT:
        ret = "THREAD_PARENT";
        break;

    case SPINEL_PROP_THREAD_CHILD_TABLE:
        ret = "THREAD_CHILD_TABLE";
        break;

    case SPINEL_PROP_THREAD_LEADER_RID:
        ret = "THREAD_LEADER_RID";
        break;

    case SPINEL_PROP_THREAD_LEADER_WEIGHT:
        ret = "THREAD_LEADER_WEIGHT";
        break;

    case SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT:
        ret = "THREAD_LOCAL_LEADER_WEIGHT";
        break;

    case SPINEL_PROP_THREAD_NETWORK_DATA:
        ret = "THREAD_NETWORK_DATA";
        break;

    case SPINEL_PROP_THREAD_NETWORK_DATA_VERSION:
        ret = "THREAD_NETWORK_DATA_VERSION";
        break;

    case SPINEL_PROP_THREAD_STABLE_NETWORK_DATA:
        ret = "THREAD_STABLE_NETWORK_DATA";
        break;

    case SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION:
        ret = "THREAD_STABLE_NETWORK_DATA_VERSION";
        break;

    case SPINEL_PROP_THREAD_ON_MESH_NETS:
        ret = "THREAD_ON_MESH_NETS";
        break;

    case SPINEL_PROP_THREAD_OFF_MESH_ROUTES:
        ret = "THREAD_OFF_MESH_ROUTES";
        break;

    case SPINEL_PROP_THREAD_ASSISTING_PORTS:
        ret = "THREAD_ASSISTING_PORTS";
        break;

    case SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE:
        ret = "THREAD_ALLOW_LOCAL_NET_DATA_CHANGE";
        break;

    case SPINEL_PROP_THREAD_MODE:
        ret = "THREAD_MODE";
        break;

    case SPINEL_PROP_THREAD_CHILD_TIMEOUT:
        ret = "THREAD_CHILD_TIMEOUT";
        break;

    case SPINEL_PROP_THREAD_RLOC16:
        ret = "THREAD_RLOC16";
        break;

    case SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD:
        ret = "THREAD_ROUTER_UPGRADE_THRESHOLD";
        break;

    case SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY:
        ret = "THREAD_CONTEXT_REUSE_DELAY";
        break;

    case SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT:
        ret = "THREAD_NETWORK_ID_TIMEOUT";
        break;

    case SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS:
        ret = "THREAD_ACTIVE_ROUTER_IDS";
        break;

    case SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU:
        ret = "THREAD_RLOC16_DEBUG_PASSTHRU";
        break;

    case SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED:
        ret = "THREAD_ROUTER_ROLE_ENABLED";
        break;

    case SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD:
        ret = "THREAD_ROUTER_DOWNGRADE_THRESHOLD";
        break;

    case SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER:
        ret = "THREAD_ROUTER_SELECTION_JITTER";
        break;

    case SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID:
        ret = "THREAD_PREFERRED_ROUTER_ID";
        break;

    case SPINEL_PROP_THREAD_NEIGHBOR_TABLE:
        ret = "THREAD_NEIGHBOR_TABLE";
        break;

    case SPINEL_PROP_THREAD_CHILD_COUNT_MAX:
        ret = "THREAD_CHILD_COUNT_MAX";
        break;

    case SPINEL_PROP_THREAD_LEADER_NETWORK_DATA:
        ret = "THREAD_LEADER_NETWORK_DATA";
        break;

    case SPINEL_PROP_THREAD_STABLE_LEADER_NETWORK_DATA:
        ret = "THREAD_STABLE_LEADER_NETWORK_DATA";
        break;

    case SPINEL_PROP_THREAD_JOINERS:
        ret = "THREAD_JOINERS";
        break;

    case SPINEL_PROP_THREAD_COMMISSIONER_ENABLED:
        ret = "THREAD_COMMISSIONER_ENABLED";
        break;

    case SPINEL_PROP_THREAD_TMF_PROXY_ENABLED:
        ret = "THREAD_TMF_PROXY_ENABLED";
        break;

    case SPINEL_PROP_THREAD_TMF_PROXY_STREAM:
        ret = "THREAD_TMF_PROXY_STREAM";
        break;

    case SPINEL_PROP_THREAD_UDP_FORWARD_STREAM:
        ret = "THREAD_UDP_FORWARD_STREAM";
        break;

    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG:
        ret = "THREAD_DISCOVERY_SCAN_JOINER_FLAG";
        break;

    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING:
        ret = "THREAD_DISCOVERY_SCAN_ENABLE_FILTERING";
        break;

    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID:
        ret = "THREAD_DISCOVERY_SCAN_PANID";
        break;

    case SPINEL_PROP_THREAD_STEERING_DATA:
        ret = "THREAD_STEERING_DATA";
        break;

    case SPINEL_PROP_THREAD_ROUTER_TABLE:
        ret = "THREAD_ROUTER_TABLE";
        break;

    case SPINEL_PROP_THREAD_ACTIVE_DATASET:
        ret = "THREAD_ACTIVE_DATASET";
        break;

    case SPINEL_PROP_THREAD_PENDING_DATASET:
        ret = "THREAD_PENDING_DATASET";
        break;

    case SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET:
        ret = "THREAD_MGMT_SET_ACTIVE_DATASET";
        break;

    case SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET:
        ret = "THREAD_MGMT_SET_PENDING_DATASET";
        break;

    case SPINEL_PROP_DATASET_ACTIVE_TIMESTAMP:
        ret = "DATASET_ACTIVE_TIMESTAMP";
        break;

    case SPINEL_PROP_DATASET_PENDING_TIMESTAMP:
        ret = "DATASET_PENDING_TIMESTAMP";
        break;

    case SPINEL_PROP_DATASET_DELAY_TIMER:
        ret = "DATASET_DELAY_TIMER";
        break;

    case SPINEL_PROP_DATASET_SECURITY_POLICY:
        ret = "DATASET_SECURITY_POLICY";
        break;

    case SPINEL_PROP_DATASET_RAW_TLVS:
        ret = "DATASET_RAW_TLVS";
        break;

    case SPINEL_PROP_THREAD_CHILD_TABLE_ADDRESSES:
        ret = "THREAD_CHILD_TABLE_ADDRESSES";
        break;

    case SPINEL_PROP_THREAD_NEIGHBOR_TABLE_ERROR_RATES:
        ret = "THREAD_NEIGHBOR_TABLE_ERROR_RATES";
        break;

    case SPINEL_PROP_THREAD_ADDRESS_CACHE_TABLE:
        ret = "THREAD_ADDRESS_CACHE_TABLE";
        break;

    case SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET:
        ret = "THREAD_MGMT_GET_ACTIVE_DATASET";
        break;

    case SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET:
        ret = "THREAD_MGMT_GET_PENDING_DATASET";
        break;

    case SPINEL_PROP_DATASET_DEST_ADDRESS:
        ret = "DATASET_DEST_ADDRESS";
        break;

    case SPINEL_PROP_MESHCOP_JOINER_STATE:
        ret = "MESHCOP_JOINER_STATE";
        break;

    case SPINEL_PROP_MESHCOP_JOINER_COMMISSIONING:
        ret = "MESHCOP_JOINER_COMMISSIONING";
        break;

    case SPINEL_PROP_IPV6_LL_ADDR:
        ret = "IPV6_LL_ADDR";
        break;

    case SPINEL_PROP_IPV6_ML_ADDR:
        ret = "IPV6_ML_ADDR";
        break;

    case SPINEL_PROP_IPV6_ML_PREFIX:
        ret = "IPV6_ML_PREFIX";
        break;

    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        ret = "IPV6_ADDRESS_TABLE";
        break;

    case SPINEL_PROP_IPV6_ROUTE_TABLE:
        ret = "IPV6_ROUTE_TABLE";
        break;

    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD:
        ret = "IPV6_ICMP_PING_OFFLOAD";
        break;

    case SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE:
        ret = "IPV6_MULTICAST_ADDRESS_TABLE";
        break;

    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD_MODE:
        ret = "IPV6_ICMP_PING_OFFLOAD_MODE";
        break;

    case SPINEL_PROP_STREAM_DEBUG:
        ret = "STREAM_DEBUG";
        break;

    case SPINEL_PROP_STREAM_RAW:
        ret = "STREAM_RAW";
        break;

    case SPINEL_PROP_STREAM_NET:
        ret = "STREAM_NET";
        break;

    case SPINEL_PROP_STREAM_NET_INSECURE:
        ret = "STREAM_NET_INSECURE";
        break;

    case SPINEL_PROP_STREAM_LOG:
        ret = "STREAM_LOG";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_STATE:
        ret = "MESHCOP_COMMISSIONER_STATE";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS:
        ret = "MESHCOP_COMMISSIONER_JOINERS";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL:
        ret = "MESHCOP_COMMISSIONER_PROVISIONING_URL";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_SESSION_ID:
        ret = "MESHCOP_COMMISSIONER_SESSION_ID";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_ANNOUNCE_BEGIN:
        ret = "MESHCOP_COMMISSIONER_ANNOUNCE_BEGIN";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN:
        ret = "MESHCOP_COMMISSIONER_ENERGY_SCAN";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN_RESULT:
        ret = "MESHCOP_COMMISSIONER_ENERGY_SCAN_RESULT";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_QUERY:
        ret = "MESHCOP_COMMISSIONER_PAN_ID_QUERY";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_CONFLICT_RESULT:
        ret = "MESHCOP_COMMISSIONER_PAN_ID_CONFLICT_RESULT";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_GET:
        ret = "MESHCOP_COMMISSIONER_MGMT_GET";
        break;

    case SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_SET:
        ret = "MESHCOP_COMMISSIONER_MGMT_SET";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL:
        ret = "CHANNEL_MANAGER_NEW_CHANNEL";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_DELAY:
        ret = "CHANNEL_MANAGER_DELAY";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS:
        ret = "CHANNEL_MANAGER_SUPPORTED_CHANNELS";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS:
        ret = "CHANNEL_MANAGER_FAVORED_CHANNELS";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT:
        ret = "CHANNEL_MANAGER_CHANNEL_SELECT";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED:
        ret = "CHANNEL_MANAGER_AUTO_SELECT_ENABLED";
        break;

    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL:
        ret = "CHANNEL_MANAGER_AUTO_SELECT_INTERVAL";
        break;

    case SPINEL_PROP_THREAD_NETWORK_TIME:
        ret = "THREAD_NETWORK_TIME";
        break;

    case SPINEL_PROP_TIME_SYNC_PERIOD:
        ret = "TIME_SYNC_PERIOD";
        break;

    case SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD:
        ret = "TIME_SYNC_XTAL_THRESHOLD";
        break;

    case SPINEL_PROP_CHILD_SUPERVISION_INTERVAL:
        ret = "CHILD_SUPERVISION_INTERVAL";
        break;

    case SPINEL_PROP_CHILD_SUPERVISION_CHECK_TIMEOUT:
        ret = "CHILD_SUPERVISION_CHECK_TIMEOUT";
        break;

    case SPINEL_PROP_RCP_VERSION:
        ret = "RCP_VERSION";
        break;

    case SPINEL_PROP_PARENT_RESPONSE_INFO:
        ret = "PARENT_RESPONSE_INFO";
        break;

    case SPINEL_PROP_UART_BITRATE:
        ret = "UART_BITRATE";
        break;

    case SPINEL_PROP_UART_XON_XOFF:
        ret = "UART_XON_XOFF";
        break;

    case SPINEL_PROP_15_4_PIB_PHY_CHANNELS_SUPPORTED:
        ret = "15_4_PIB_PHY_CHANNELS_SUPPORTED";
        break;

    case SPINEL_PROP_15_4_PIB_MAC_PROMISCUOUS_MODE:
        ret = "15_4_PIB_MAC_PROMISCUOUS_MODE";
        break;

    case SPINEL_PROP_15_4_PIB_MAC_SECURITY_ENABLED:
        ret = "15_4_PIB_MAC_SECURITY_ENABLED";
        break;

    case SPINEL_PROP_CNTR_RESET:
        ret = "CNTR_RESET";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_TOTAL:
        ret = "CNTR_TX_PKT_TOTAL";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_ACK_REQ:
        ret = "CNTR_TX_PKT_ACK_REQ";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_ACKED:
        ret = "CNTR_TX_PKT_ACKED";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ:
        ret = "CNTR_TX_PKT_NO_ACK_REQ";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_DATA:
        ret = "CNTR_TX_PKT_DATA";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_DATA_POLL:
        ret = "CNTR_TX_PKT_DATA_POLL";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BEACON:
        ret = "CNTR_TX_PKT_BEACON";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ:
        ret = "CNTR_TX_PKT_BEACON_REQ";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_OTHER:
        ret = "CNTR_TX_PKT_OTHER";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_RETRY:
        ret = "CNTR_TX_PKT_RETRY";
        break;

    case SPINEL_PROP_CNTR_TX_ERR_CCA:
        ret = "CNTR_TX_ERR_CCA";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_UNICAST:
        ret = "CNTR_TX_PKT_UNICAST";
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BROADCAST:
        ret = "CNTR_TX_PKT_BROADCAST";
        break;

    case SPINEL_PROP_CNTR_TX_ERR_ABORT:
        ret = "CNTR_TX_ERR_ABORT";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_TOTAL:
        ret = "CNTR_RX_PKT_TOTAL";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DATA:
        ret = "CNTR_RX_PKT_DATA";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DATA_POLL:
        ret = "CNTR_RX_PKT_DATA_POLL";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BEACON:
        ret = "CNTR_RX_PKT_BEACON";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ:
        ret = "CNTR_RX_PKT_BEACON_REQ";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_OTHER:
        ret = "CNTR_RX_PKT_OTHER";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_FILT_WL:
        ret = "CNTR_RX_PKT_FILT_WL";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_FILT_DA:
        ret = "CNTR_RX_PKT_FILT_DA";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_EMPTY:
        ret = "CNTR_RX_ERR_EMPTY";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR:
        ret = "CNTR_RX_ERR_UKWN_NBR";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR:
        ret = "CNTR_RX_ERR_NVLD_SADDR";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_SECURITY:
        ret = "CNTR_RX_ERR_SECURITY";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_BAD_FCS:
        ret = "CNTR_RX_ERR_BAD_FCS";
        break;

    case SPINEL_PROP_CNTR_RX_ERR_OTHER:
        ret = "CNTR_RX_ERR_OTHER";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DUP:
        ret = "CNTR_RX_PKT_DUP";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_UNICAST:
        ret = "CNTR_RX_PKT_UNICAST";
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BROADCAST:
        ret = "CNTR_RX_PKT_BROADCAST";
        break;

    case SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL:
        ret = "CNTR_TX_IP_SEC_TOTAL";
        break;

    case SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL:
        ret = "CNTR_TX_IP_INSEC_TOTAL";
        break;

    case SPINEL_PROP_CNTR_TX_IP_DROPPED:
        ret = "CNTR_TX_IP_DROPPED";
        break;

    case SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL:
        ret = "CNTR_RX_IP_SEC_TOTAL";
        break;

    case SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL:
        ret = "CNTR_RX_IP_INSEC_TOTAL";
        break;

    case SPINEL_PROP_CNTR_RX_IP_DROPPED:
        ret = "CNTR_RX_IP_DROPPED";
        break;

    case SPINEL_PROP_CNTR_TX_SPINEL_TOTAL:
        ret = "CNTR_TX_SPINEL_TOTAL";
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_TOTAL:
        ret = "CNTR_RX_SPINEL_TOTAL";
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_ERR:
        ret = "CNTR_RX_SPINEL_ERR";
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_OUT_OF_ORDER_TID:
        ret = "CNTR_RX_SPINEL_OUT_OF_ORDER_TID";
        break;

    case SPINEL_PROP_CNTR_IP_TX_SUCCESS:
        ret = "CNTR_IP_TX_SUCCESS";
        break;

    case SPINEL_PROP_CNTR_IP_RX_SUCCESS:
        ret = "CNTR_IP_RX_SUCCESS";
        break;

    case SPINEL_PROP_CNTR_IP_TX_FAILURE:
        ret = "CNTR_IP_TX_FAILURE";
        break;

    case SPINEL_PROP_CNTR_IP_RX_FAILURE:
        ret = "CNTR_IP_RX_FAILURE";
        break;

    case SPINEL_PROP_MSG_BUFFER_COUNTERS:
        ret = "MSG_BUFFER_COUNTERS";
        break;

    case SPINEL_PROP_CNTR_ALL_MAC_COUNTERS:
        ret = "CNTR_ALL_MAC_COUNTERS";
        break;

    case SPINEL_PROP_NEST_STREAM_MFG:
        ret = "NEST_STREAM_MFG";
        break;

    case SPINEL_PROP_NEST_LEGACY_ULA_PREFIX:
        ret = "NEST_LEGACY_ULA_PREFIX";
        break;

    case SPINEL_PROP_NEST_LEGACY_LAST_NODE_JOINED:
        ret = "NEST_LEGACY_LAST_NODE_JOINED";
        break;

    case SPINEL_PROP_DEBUG_TEST_ASSERT:
        ret = "DEBUG_TEST_ASSERT";
        break;

    case SPINEL_PROP_DEBUG_NCP_LOG_LEVEL:
        ret = "DEBUG_NCP_LOG_LEVEL";
        break;

    case SPINEL_PROP_DEBUG_TEST_WATCHDOG:
        ret = "DEBUG_TEST_WATCHDOG";
        break;

    default:
        break;
    }

    return ret;
}

const char *spinel_net_role_to_cstr(uint8_t net_role)
{
    const char *ret = "NET_ROLE_UNKNONW";

    switch (net_role)
    {
    case SPINEL_NET_ROLE_DETACHED:
        ret = "NET_ROLE_DETACHED";
        break;

    case SPINEL_NET_ROLE_CHILD:
        ret = "NET_ROLE_CHILD";
        break;

    case SPINEL_NET_ROLE_ROUTER:
        ret = "NET_ROLE_ROUTER";
        break;

    case SPINEL_NET_ROLE_LEADER:
        ret = "NET_ROLE_LEADER";
        break;

    default:
        break;
    }

    return ret;
}

const char *spinel_mcu_power_state_to_cstr(spinel_mcu_power_state_t mcu_power_state)
{
    const char *ret = "MCU_POWER_STATE_UNKNOWN";

    switch (mcu_power_state)
    {
    case SPINEL_MCU_POWER_STATE_ON:
        ret = "MCU_POWER_STATE_ON";
        break;

    case SPINEL_MCU_POWER_STATE_LOW_POWER:
        ret = "MCU_POWER_STATE_LOW_POWER";
        break;

    case SPINEL_MCU_POWER_STATE_OFF:
        ret = "MCU_POWER_STATE_OFF";
        break;
    }

    return ret;
}

const char *spinel_status_to_cstr(spinel_status_t status)
{
    const char *ret = "UNKNOWN";

    switch (status)
    {
    case SPINEL_STATUS_OK:
        ret = "OK";
        break;

    case SPINEL_STATUS_FAILURE:
        ret = "FAILURE";
        break;

    case SPINEL_STATUS_UNIMPLEMENTED:
        ret = "UNIMPLEMENTED";
        break;

    case SPINEL_STATUS_INVALID_ARGUMENT:
        ret = "INVALID_ARGUMENT";
        break;

    case SPINEL_STATUS_INVALID_STATE:
        ret = "INVALID_STATE";
        break;

    case SPINEL_STATUS_INVALID_COMMAND:
        ret = "INVALID_COMMAND";
        break;

    case SPINEL_STATUS_INVALID_INTERFACE:
        ret = "INVALID_INTERFACE";
        break;

    case SPINEL_STATUS_INTERNAL_ERROR:
        ret = "INTERNAL_ERROR";
        break;

    case SPINEL_STATUS_SECURITY_ERROR:
        ret = "SECURITY_ERROR";
        break;

    case SPINEL_STATUS_PARSE_ERROR:
        ret = "PARSE_ERROR";
        break;

    case SPINEL_STATUS_IN_PROGRESS:
        ret = "IN_PROGRESS";
        break;

    case SPINEL_STATUS_NOMEM:
        ret = "NOMEM";
        break;

    case SPINEL_STATUS_BUSY:
        ret = "BUSY";
        break;

    case SPINEL_STATUS_PROP_NOT_FOUND:
        ret = "PROP_NOT_FOUND";
        break;

    case SPINEL_STATUS_DROPPED:
        ret = "DROPPED";
        break;

    case SPINEL_STATUS_EMPTY:
        ret = "EMPTY";
        break;

    case SPINEL_STATUS_CMD_TOO_BIG:
        ret = "CMD_TOO_BIG";
        break;

    case SPINEL_STATUS_NO_ACK:
        ret = "NO_ACK";
        break;

    case SPINEL_STATUS_CCA_FAILURE:
        ret = "CCA_FAILURE";
        break;

    case SPINEL_STATUS_ALREADY:
        ret = "ALREADY";
        break;

    case SPINEL_STATUS_ITEM_NOT_FOUND:
        ret = "ITEM_NOT_FOUND";
        break;

    case SPINEL_STATUS_INVALID_COMMAND_FOR_PROP:
        ret = "INVALID_COMMAND_FOR_PROP";
        break;

    case SPINEL_STATUS_JOIN_FAILURE:
        ret = "JOIN_FAILURE";
        break;

    case SPINEL_STATUS_JOIN_SECURITY:
        ret = "JOIN_SECURITY";
        break;

    case SPINEL_STATUS_JOIN_NO_PEERS:
        ret = "JOIN_NO_PEERS";
        break;

    case SPINEL_STATUS_JOIN_INCOMPATIBLE:
        ret = "JOIN_INCOMPATIBLE";
        break;

    case SPINEL_STATUS_JOIN_RSP_TIMEOUT:
        ret = "JOIN_RSP_TIMEOUT";
        break;

    case SPINEL_STATUS_JOIN_SUCCESS:
        ret = "JOIN_SUCCESS";
        break;

    case SPINEL_STATUS_RESET_POWER_ON:
        ret = "RESET_POWER_ON";
        break;

    case SPINEL_STATUS_RESET_EXTERNAL:
        ret = "RESET_EXTERNAL";
        break;

    case SPINEL_STATUS_RESET_SOFTWARE:
        ret = "RESET_SOFTWARE";
        break;

    case SPINEL_STATUS_RESET_FAULT:
        ret = "RESET_FAULT";
        break;

    case SPINEL_STATUS_RESET_CRASH:
        ret = "RESET_CRASH";
        break;

    case SPINEL_STATUS_RESET_ASSERT:
        ret = "RESET_ASSERT";
        break;

    case SPINEL_STATUS_RESET_OTHER:
        ret = "RESET_OTHER";
        break;

    case SPINEL_STATUS_RESET_UNKNOWN:
        ret = "RESET_UNKNOWN";
        break;

    case SPINEL_STATUS_RESET_WATCHDOG:
        ret = "RESET_WATCHDOG";
        break;

    default:
        break;
    }

    return ret;
}

const char *spinel_capability_to_cstr(unsigned int capability)
{
    const char *ret = "UNKNOWN";

    switch (capability)
    {
    case SPINEL_CAP_LOCK:
        ret = "LOCK";
        break;

    case SPINEL_CAP_NET_SAVE:
        ret = "NET_SAVE";
        break;

    case SPINEL_CAP_HBO:
        ret = "HBO";
        break;

    case SPINEL_CAP_POWER_SAVE:
        ret = "POWER_SAVE";
        break;

    case SPINEL_CAP_COUNTERS:
        ret = "COUNTERS";
        break;

    case SPINEL_CAP_JAM_DETECT:
        ret = "JAM_DETECT";
        break;

    case SPINEL_CAP_PEEK_POKE:
        ret = "PEEK_POKE";
        break;

    case SPINEL_CAP_WRITABLE_RAW_STREAM:
        ret = "WRITABLE_RAW_STREAM";
        break;

    case SPINEL_CAP_GPIO:
        ret = "GPIO";
        break;

    case SPINEL_CAP_TRNG:
        ret = "TRNG";
        break;

    case SPINEL_CAP_CMD_MULTI:
        ret = "CMD_MULTI";
        break;

    case SPINEL_CAP_UNSOL_UPDATE_FILTER:
        ret = "UNSOL_UPDATE_FILTER";
        break;

    case SPINEL_CAP_MCU_POWER_STATE:
        ret = "MCU_POWER_STATE";
        break;

    case SPINEL_CAP_PCAP:
        ret = "PCAP";
        break;

    case SPINEL_CAP_802_15_4_2003:
        ret = "802_15_4_2003";
        break;

    case SPINEL_CAP_802_15_4_2006:
        ret = "802_15_4_2006";
        break;

    case SPINEL_CAP_802_15_4_2011:
        ret = "802_15_4_2011";
        break;

    case SPINEL_CAP_802_15_4_PIB:
        ret = "802_15_4_PIB";
        break;

    case SPINEL_CAP_802_15_4_2450MHZ_OQPSK:
        ret = "802_15_4_2450MHZ_OQPSK";
        break;

    case SPINEL_CAP_802_15_4_915MHZ_OQPSK:
        ret = "802_15_4_915MHZ_OQPSK";
        break;

    case SPINEL_CAP_802_15_4_868MHZ_OQPSK:
        ret = "802_15_4_868MHZ_OQPSK";
        break;

    case SPINEL_CAP_802_15_4_915MHZ_BPSK:
        ret = "802_15_4_915MHZ_BPSK";
        break;

    case SPINEL_CAP_802_15_4_868MHZ_BPSK:
        ret = "802_15_4_868MHZ_BPSK";
        break;

    case SPINEL_CAP_802_15_4_915MHZ_ASK:
        ret = "802_15_4_915MHZ_ASK";
        break;

    case SPINEL_CAP_802_15_4_868MHZ_ASK:
        ret = "802_15_4_868MHZ_ASK";
        break;

    case SPINEL_CAP_CONFIG_FTD:
        ret = "CONFIG_FTD";
        break;

    case SPINEL_CAP_CONFIG_MTD:
        ret = "CONFIG_MTD";
        break;

    case SPINEL_CAP_CONFIG_RADIO:
        ret = "CONFIG_RADIO";
        break;

    case SPINEL_CAP_ROLE_ROUTER:
        ret = "ROLE_ROUTER";
        break;

    case SPINEL_CAP_ROLE_SLEEPY:
        ret = "ROLE_SLEEPY";
        break;

    case SPINEL_CAP_NET_THREAD_1_0:
        ret = "NET_THREAD_1_0";
        break;

    case SPINEL_CAP_NET_THREAD_1_1:
        ret = "NET_THREAD_1_1";
        break;

    case SPINEL_CAP_MAC_WHITELIST:
        ret = "MAC_WHITELIST";
        break;

    case SPINEL_CAP_MAC_RAW:
        ret = "MAC_RAW";
        break;

    case SPINEL_CAP_OOB_STEERING_DATA:
        ret = "OOB_STEERING_DATA";
        break;

    case SPINEL_CAP_CHANNEL_MONITOR:
        ret = "CHANNEL_MONITOR";
        break;

    case SPINEL_CAP_CHANNEL_MANAGER:
        ret = "CHANNEL_MANAGER";
        break;

    case SPINEL_CAP_OPENTHREAD_LOG_METADATA:
        ret = "OPENTHREAD_LOG_METADATA";
        break;

    case SPINEL_CAP_TIME_SYNC:
        ret = "TIME_SYNC";
        break;

    case SPINEL_CAP_CHILD_SUPERVISION:
        ret = "CHILD_SUPERVISION";
        break;

    case SPINEL_CAP_POSIX_APP:
        ret = "POSIX_APP";
        break;

    case SPINEL_CAP_ERROR_RATE_TRACKING:
        ret = "ERROR_RATE_TRACKING";
        break;

    case SPINEL_CAP_THREAD_COMMISSIONER:
        ret = "THREAD_COMMISSIONER";
        break;

    case SPINEL_CAP_THREAD_TMF_PROXY:
        ret = "THREAD_TMF_PROXY";
        break;

    case SPINEL_CAP_THREAD_UDP_FORWARD:
        ret = "THREAD_UDP_FORWARD";
        break;

    case SPINEL_CAP_THREAD_JOINER:
        ret = "THREAD_JOINER";
        break;

    case SPINEL_CAP_THREAD_BORDER_ROUTER:
        ret = "THREAD_BORDER_ROUTER";
        break;

    case SPINEL_CAP_NEST_LEGACY_INTERFACE:
        ret = "NEST_LEGACY_INTERFACE";
        break;

    case SPINEL_CAP_NEST_LEGACY_NET_WAKE:
        ret = "NEST_LEGACY_NET_WAKE";
        break;

    case SPINEL_CAP_NEST_TRANSMIT_HOOK:
        ret = "NEST_TRANSMIT_HOOK";
        break;

    default:
        break;
    }

    return ret;
}

// **** LCOV_EXCL_STOP ****

/* -------------------------------------------------------------------------- */

#if SPINEL_SELF_TEST

int main(void)
{
    int                  ret             = -1;
    const spinel_eui64_t static_eui64    = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}};
    const char           static_string[] = "static_string";
    uint8_t              buffer[1024];
    ssize_t              len;

    len =
        spinel_datatype_pack(buffer, sizeof(buffer), "CiiLUE", 0x88, 9, 0xA3, 0xDEADBEEF, static_string, &static_eui64);

    if (len != 30)
    {
        printf("error:%d: len != 30; (%d)\n", __LINE__, (int)len);
        goto bail;
    }

    {
        const char *str = NULL;

        // Length ends right before the string.
        len = spinel_datatype_unpack(buffer, 8, "CiiLU", NULL, NULL, NULL, NULL, &str);

        if (len != -1)
        {
            printf("error:%d: len != -1; (%d)\n", __LINE__, (int)len);
            goto bail;
        }

        if (str != NULL)
        {
            printf("error:%d: str != NULL\n", __LINE__);
            goto bail;
        }
    }

    len = 30;

    {
        uint8_t               c     = 0;
        unsigned int          i1    = 0;
        unsigned int          i2    = 0;
        uint32_t              l     = 0;
        const char *          str   = NULL;
        const spinel_eui64_t *eui64 = NULL;

        len = spinel_datatype_unpack(buffer, (spinel_size_t)len, "CiiLUE", &c, &i1, &i2, &l, &str, &eui64);

        if (len != 30)
        {
            printf("error:%d: len != 30; (%d)\n", __LINE__, (int)len);
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
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", (unsigned int)l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }

        if (memcmp(eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0)
        {
            printf("error:%d: memcmp(eui64, &eui64, sizeof(spinel_eui64_t)) != 0\n", __LINE__);
            goto bail;
        }
    }

    {
        uint8_t        c  = 0;
        unsigned int   i1 = 0;
        unsigned int   i2 = 0;
        uint32_t       l  = 0;
        char           str[sizeof(static_string)];
        spinel_eui64_t eui64 = {{0}};

        len = spinel_datatype_unpack_in_place(buffer, (spinel_size_t)len, "CiiLUE", &c, &i1, &i2, &l, &str, sizeof(str),
                                              &eui64);

        if (len != 30)
        {
            printf("error:%d: len != 30; (%d)\n", __LINE__, (int)len);
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
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", (unsigned int)l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }

        if (memcmp(&eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0)
        {
            printf("error:%d: memcmp(&eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0\n", __LINE__);
            goto bail;
        }
    }

    // -----------------------------------

    memset(buffer, 0xAA, sizeof(buffer));

    len = spinel_datatype_pack(buffer, sizeof(buffer), "Cit(iL)UE", 0x88, 9, 0xA3, 0xDEADBEEF, static_string,
                               &static_eui64);

    if (len != 32)
    {
        printf("error:%d: len != 32; (%d)\n", __LINE__, (int)len);
        goto bail;
    }

    {
        uint8_t         c     = 0;
        unsigned int    i1    = 0;
        unsigned int    i2    = 0;
        uint32_t        l     = 0;
        const char *    str   = NULL;
        spinel_eui64_t *eui64 = NULL;

        len = spinel_datatype_unpack(buffer, (spinel_size_t)len, "Cit(iL)UE", &c, &i1, &i2, &l, &str, &eui64);

        if (len != 32)
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
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", (unsigned int)l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }

        if (memcmp(eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0)
        {
            printf("error:%d: memcmp(eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0\n", __LINE__);
            goto bail;
        }
    }

    {
        uint8_t        c  = 0;
        unsigned int   i1 = 0;
        unsigned int   i2 = 0;
        uint32_t       l  = 0;
        char           str[sizeof(static_string)];
        spinel_eui64_t eui64 = {{0}};

        len = spinel_datatype_unpack_in_place(buffer, (spinel_size_t)len, "Cit(iL)UE", &c, &i1, &i2, &l, &str,
                                              sizeof(str), &eui64);

        if (len != 32)
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
            printf("error: l != 0xDEADBEEF; (0x%08X)\n", (unsigned int)l);
            goto bail;
        }

        if (strcmp(str, static_string) != 0)
        {
            printf("error:%d: strcmp(str,static_string) != 0\n", __LINE__);
            goto bail;
        }

        if (memcmp(&eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0)
        {
            printf("error:%d: memcmp(&eui64, &static_eui64, sizeof(spinel_eui64_t)) != 0\n", __LINE__);
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
