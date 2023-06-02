/*
 *  Copyright (c) 2018, Sam Kumar <samkumar@cs.berkeley.edu>
 *  Copyright (c) 2018, University of California, Berkeley
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

/* BITMAP */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"

void bmp_init(uint8_t* buf, size_t numbytes) {
    memset(buf, 0x00, numbytes);
}

#define _bmp_getrangeinfo(buf, start, len, first_bit_id, first_byte_ptr, last_bit_id, last_byte_ptr) \
    do { \
        first_bit_id = (start & 0x7); \
        first_byte_ptr = buf + (start >> 3); \
        last_bit_id = (len & 0x7) + first_bit_id; \
        last_byte_ptr = first_byte_ptr + (len >> 3) + (last_bit_id >> 3); \
        last_bit_id &= 0x7; \
    } while (0)

void bmp_setrange(uint8_t* buf, size_t start, size_t len) {
    uint8_t first_bit_id;
    uint8_t* first_byte_set;
    uint8_t last_bit_id;
    uint8_t* last_byte_set;
    uint8_t first_byte_mask, last_byte_mask;
    _bmp_getrangeinfo(buf, start, len, first_bit_id, first_byte_set,
                      last_bit_id, last_byte_set);

    first_byte_mask = (uint8_t) (0xFF >> first_bit_id);
    last_byte_mask = (uint8_t) (0xFF << (8 - last_bit_id));

    /* Set the bits. */
    if (first_byte_set == last_byte_set) {
        *first_byte_set |= (first_byte_mask & last_byte_mask);
    } else {
        *first_byte_set |= first_byte_mask;
        memset(first_byte_set + 1, 0xFF, (size_t) (last_byte_set - first_byte_set - 1));
        if (last_byte_mask != 0x00) {
            *last_byte_set |= last_byte_mask;
        }
    }
}

void bmp_clrrange(uint8_t* buf, size_t start, size_t len) {
    uint8_t first_bit_id;
    uint8_t* first_byte_clear;
    uint8_t last_bit_id;
    uint8_t* last_byte_clear;
    uint8_t first_byte_mask, last_byte_mask;
    _bmp_getrangeinfo(buf, start, len, first_bit_id, first_byte_clear,
                      last_bit_id, last_byte_clear);

    first_byte_mask = (uint8_t) (0xFF << (8 - first_bit_id));
    last_byte_mask = (uint8_t) (0xFF >> last_bit_id);

    /* Clear the bits. */
    if (first_byte_clear == last_byte_clear) {
        *first_byte_clear &= (first_byte_mask | last_byte_mask);
    } else {
        *first_byte_clear &= first_byte_mask;
        memset(first_byte_clear + 1, 0x00, (size_t) (last_byte_clear - first_byte_clear - 1));
        if (last_byte_mask != 0xFF) {
            *last_byte_clear &= last_byte_mask;
        }
    }
}

size_t bmp_countset(uint8_t* buf, size_t buflen, size_t start, size_t limit) {
    uint8_t first_bit_id;
    uint8_t first_byte;
    uint8_t ideal_first_byte;
    size_t numset;
    uint8_t curr_byte;
    size_t curr_index = start >> 3;
    first_bit_id = start & 0x7;
    first_byte = *(buf + curr_index);

    numset = 8 - first_bit_id; // initialize optimistically, assuming that the first byte will have all 1's in the part we care about
    ideal_first_byte = (uint8_t) (0xFF >> first_bit_id);
    first_byte &= ideal_first_byte;
    if (first_byte == ideal_first_byte) {
        // All bits in the first byte starting at first_bit_id are set
        for (curr_index = curr_index + 1; curr_index < buflen && numset < limit; curr_index++) {
            curr_byte = buf[curr_index];
            if (curr_byte == (uint8_t) 0xFF) {
                numset += 8;
            } else {
                while (curr_byte & (uint8_t) 0x80) { // we could add a numset < limit check here, but it probably isn't worth it
                    curr_byte <<= 1;
                    numset++;
                }
                break;
            }
        }
    } else {
        // The streak ends within the first byte
        do {
            first_byte >>= 1;
            ideal_first_byte >>= 1;
            numset--;
        } while (first_byte != ideal_first_byte);
    }
    return numset;
}

static inline uint8_t bmp_read_bit(uint8_t* buf, size_t i) {
    size_t byte_index = i >> 3;
    size_t bit_index = i & 0x7; // Amount to left shift to get bit in MSB
    return ((uint8_t) (buf[byte_index] << bit_index)) >> 7;
}

static inline void bmp_write_bit(uint8_t* buf, size_t i, uint8_t bit) {
    size_t byte_index = i >> 3;
    size_t bit_index = i & 0x7; // Amount to left shift to get bit in MSB
    size_t bit_shift = 7 - bit_index; // Amount to right shift to get bit in LSB
    buf[byte_index] = (buf[byte_index] & ~(1 << bit_shift)) | (bit << bit_shift);
}

static inline uint8_t bmp_read_byte(uint8_t* buf, size_t i) {
    size_t byte_index = i >> 3;
    size_t bit_index = i & 0x7; // Amount to left shift to get bit in MSB
    if (bit_index == 0) {
        return buf[byte_index];
    }
    return (buf[byte_index] << bit_index) | (buf[byte_index + 1] >> (8 - bit_index));
}

static inline void bmp_write_byte(uint8_t* buf, size_t i, uint8_t byte) {
    size_t byte_index = i >> 3;
    size_t bit_index = i & 0x7; // Amount to left shift to get bit in MSB
    if (bit_index == 0) {
        buf[byte_index] = byte;
        return;
    }
    buf[byte_index] = (buf[byte_index] & (0xFF << (8 - bit_index))) | (byte >> bit_index);
    buf[byte_index + 1] = (buf[byte_index + 1] & (0xFF >> bit_index)) | (byte << (8 - bit_index));
}

void bmp_swap(uint8_t* buf, size_t start_1, size_t start_2, size_t len) {
    while ((len & 0x7) != 0) {
        uint8_t bit_1 = bmp_read_bit(buf, start_1);
        uint8_t bit_2 = bmp_read_bit(buf, start_2);
        if (bit_1 != bit_2) {
            bmp_write_bit(buf, start_1, bit_2);
            bmp_write_bit(buf, start_2, bit_1);
        }

        start_1++;
        start_2++;
        len--;
    }

    while (len != 0) {
        uint8_t byte_1 = bmp_read_byte(buf, start_1);
        uint8_t byte_2 = bmp_read_byte(buf, start_2);
        if (byte_1 != byte_2) {
            bmp_write_byte(buf, start_1, byte_2);
            bmp_write_byte(buf, start_2, byte_1);
        }

        start_1 += 8;
        start_2 += 8;
        len -= 8;
    }
}

int bmp_isempty(uint8_t* buf, size_t buflen) {
    uint8_t* bufend = buf + buflen;
    while (buf < bufend) {
        if (*(buf++)) {
            return 0;
        }
    }
    return 1;
}

/*
 * This function is unused, but I'm leaving it in the code as a comment in case
 * it becomes useful for debugging later on.
 */
#if 0
void bmp_print(uint8_t* buf, size_t buflen) {
    size_t i;
    for (i = 0; i < buflen; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}
#endif
