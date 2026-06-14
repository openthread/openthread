/*
 *  Copyright (c) 2026, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Fuzzer for TCPlp bitmap operations.
 *
 *  Targets the out-of-bounds access in bmp_read_byte and bmp_write_byte
 *  when byte indices from cbuf_contiguify are passed as bit indices to
 *  bmp_swap. The OOB occurs when bit_index != 0 and byte_index is the
 *  last byte of the bitmap (i.e., byte_index + 1 == buflen).
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

#include "bitmap.h"

static size_t read_size(const uint8_t *data, size_t size, size_t *offset, size_t max_val)
{
    if (*offset + sizeof(size_t) > size) return 0;
    size_t val = 0;
    for (size_t i = 0; i < sizeof(size_t) && *offset < size; i++, (*offset)++)
        val = (val << 8) | data[*offset];
    return val % (max_val + 1);
}

} // extern "C"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 9) return 0;

    size_t offset = 0;

    // Determine bitmap size (1 to 256 bytes)
    size_t buflen   = 1 + (read_size(data, size, &offset, 255));
    size_t max_bits = buflen * 8;

    // Clamp remaining data
    if (offset + buflen > size) buflen = size - offset;

    uint8_t *buf = (uint8_t *)malloc(buflen);
    if (!buf) return 0;
    memcpy(buf, data + offset, buflen);
    offset += buflen;

    // If not enough data left, just test set/clr
    size_t avail = size - offset;

    if (avail >= 3 * sizeof(size_t))
    {
        size_t op      = read_size(data, size, &offset, 3);
        size_t start_1 = read_size(data, size, &offset, max_bits + 8);
        size_t start_2 = read_size(data, size, &offset, max_bits + 8);
        size_t len     = read_size(data, size, &offset, max_bits + 8);

        switch (op)
        {
        case 0:
            bmp_setrange(buf, buflen, start_1, len);
            break;
        case 1:
            bmp_clrrange(buf, buflen, start_1, len);
            break;
        case 2:
            bmp_swap(buf, buflen, start_1, start_2, len);
            break;
        default:
            // Combined stress
            bmp_setrange(buf, buflen, start_1, len);
            bmp_clrrange(buf, buflen, start_2, len / 2 + 1);
            bmp_swap(buf, buflen, start_1, start_2, len / 2 + 1);
            break;
        }
    }
    else
    {
        // Minimal test: bmp_swap near boundary to trigger OOB
        if (buflen >= 2 && max_bits >= 8)
        {
            bmp_swap(buf, buflen, max_bits - 4, 0, 8);
        }
    }

    free(buf);
    return 0;
}
