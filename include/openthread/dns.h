/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *  This file defines the top-level DNS functions for the OpenThread library.
 */

#ifndef OPENTHREAD_DNS_H_
#define OPENTHREAD_DNS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-dns
 *
 * @brief
 *   This module includes functions that control DNS communication.
 *
 * @{
 *
 */

#define OT_DNS_MAX_NAME_SIZE 255 ///< Maximum name string size (includes null char at the end of string).

#define OT_DNS_MAX_LABEL_SIZE 64 ///< Maximum label string size (include null char at the end of string)

/**
 * Initializer for otDnsTxtIterator.
 */
#define OT_DNS_TXT_ITERATOR_INIT 0

typedef uint16_t otDnsTxtIterator; ///< Used to iterate through the TXT entries.

/**
 * This structure represents a TXT record entry representing a key/value pair (RFC 6763 - section 6.3).
 *
 * The string buffers pointed to by `mKey` and `mValue` MUST persist and remain unchanged after an instance of such
 * structure is passed to OpenThread (as part of `otSrpClientService` instance).
 *
 * An array of `otDnsTxtEntry` entries are used in `otSrpClientService` to specify the full TXT record (a list of
 * entries).
 *
 */
typedef struct otDnsTxtEntry
{
    /**
     * The TXT record key string. It doesn't need to be a null-terminated string and `mKeyLength` gives its length.
     *
     * If `mKey` is not NULL, then the entry is treated as key/value pair with `mValue` buffer providing the value.
     *   - The entry is encoded as follows:
     *        - A single string length byte followed by "key=value" format (without the quotation marks).
              - In this case, the overall encoded length must be 255 bytes or less.
     *   - If `mValue` is NULL, then key is treated as a boolean attribute and encoded as "key" (with no `=`).
     *   - If `mValue` is not NULL but `mValueLength` is zero, then it is treated as empty value and encoded as "key=".
     *
     * If `mKey` is NULL, then `mValue` buffer is treated as an already encoded TXT-DATA and is appended as is in the
     * DNS message.
     *
     */
    const char *   mKey;
    const uint8_t *mValue;       ///< The TXT record value or already encoded TXT-DATA (depending on `mKey`).
    uint16_t       mValueLength; ///< Number of bytes in `mValue` buffer.
    uint8_t mKeyLength; ///< Number of bytes in `mKey` buffer. MUST be set even if `mKey` is a null-terminated string.
} otDnsTxtEntry;

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DNS_H_
