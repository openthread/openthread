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

/* CIRCULAR BUFFER */
#include "cbuf.h"
#include "bitmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <openthread/message.h>
#include <openthread/tcp.h>

/*
 * Copiers for copying from/into cbufs into/from arrays or OpenThraed messages
 */

void cbuf_copy_into_buffer(void* buffer, size_t buffer_offset, const void* arr, size_t arr_offset, size_t num_bytes) {
    uint8_t* bufptr = (uint8_t*) buffer;
    const uint8_t* arrptr = (const uint8_t*) arr;
    memcpy(bufptr + buffer_offset, arrptr + arr_offset, num_bytes);
}

void cbuf_copy_from_buffer(void* arr, size_t arr_offset, const void* buffer, size_t buffer_offset, size_t num_bytes) {
    uint8_t* arrptr = (uint8_t*) arr;
    const uint8_t* bufptr = (const uint8_t*) buffer;
    memcpy(arrptr + arr_offset, bufptr + buffer_offset, num_bytes);
}

void cbuf_copy_into_message(void* buffer, size_t buffer_offset, const void* arr, size_t arr_offset, size_t num_bytes) {
    otMessage* message = (otMessage*) buffer;
    uint8_t* arrptr = (uint8_t*) arr;
    otMessageWrite(message, (uint16_t) buffer_offset, arrptr + arr_offset, (uint16_t) num_bytes);
}

void cbuf_copy_from_message(void* arr, size_t arr_offset, const void* buffer, size_t buffer_offset, size_t num_bytes) {
    uint8_t* arrptr = (uint8_t*) arr;
    const otMessage* message = (const otMessage*) buffer;
    otMessageRead(message, (uint16_t) buffer_offset, arrptr + arr_offset, (uint16_t) num_bytes);
}

/*
 * Cbuf implementation.
 */

void cbuf_init(struct cbufhead* chdr, uint8_t* buf, size_t len) {
    chdr->r_index = 0;
    chdr->used = 0;
    chdr->size = len;
    chdr->buf = buf;
}

size_t cbuf_used_space(struct cbufhead* chdr) {
    return chdr->used;
}

size_t cbuf_free_space(struct cbufhead* chdr) {
    return chdr->size - chdr->used;
}

size_t cbuf_size(struct cbufhead* chdr) {
    return chdr->size;
}

bool cbuf_empty(struct cbufhead* chdr) {
    return chdr->used == 0;
}

static inline size_t cbuf_get_w_index(const struct cbufhead* chdr) {
    size_t until_end = chdr->size - chdr->r_index;
    if (chdr->used < until_end) {
        return chdr->r_index + chdr->used;
    } else {
        return chdr->used - until_end;
    }
}

size_t cbuf_write(struct cbufhead* chdr, const void* data, size_t data_offset, size_t data_len, cbuf_copier_t copy_from) {
    size_t free_space = cbuf_free_space(chdr);
    uint8_t* buf_data;
    size_t w_index;
    size_t bytes_to_end;
    if (free_space < data_len) {
        data_len = free_space;
    }
    buf_data = chdr->buf;
    w_index = cbuf_get_w_index(chdr);
    bytes_to_end = chdr->size - w_index;
    if (data_len <= bytes_to_end) {
        copy_from(buf_data, w_index, data, data_offset, data_len);
    } else {
        copy_from(buf_data, w_index, data, data_offset, bytes_to_end);
        copy_from(buf_data, 0, data, data_offset + bytes_to_end, data_len - bytes_to_end);
    }
    chdr->used += data_len;
    return data_len;
}

void cbuf_read_unsafe(struct cbufhead* chdr, void* data, size_t data_offset, size_t numbytes, int pop, cbuf_copier_t copy_into) {
    uint8_t* buf_data = chdr->buf;
    size_t bytes_to_end = chdr->size - chdr->r_index;
    if (numbytes < bytes_to_end) {
        copy_into(data, data_offset, buf_data, chdr->r_index, numbytes);
        if (pop) {
            chdr->r_index += numbytes;
            chdr->used -= numbytes;
        }
    } else {
        copy_into(data, data_offset, buf_data, chdr->r_index, bytes_to_end);
        copy_into(data, data_offset + bytes_to_end, buf_data, 0, numbytes - bytes_to_end);
        if (pop) {
            chdr->r_index = numbytes - bytes_to_end;
            chdr->used -= numbytes;
        }
    }
}

size_t cbuf_read(struct cbufhead* chdr, void* data, size_t data_offset, size_t numbytes, int pop, cbuf_copier_t copy_into) {
    size_t used_space = cbuf_used_space(chdr);
    if (used_space < numbytes) {
        numbytes = used_space;
    }
    cbuf_read_unsafe(chdr, data, data_offset, numbytes, pop, copy_into);
    return numbytes;
}

size_t cbuf_read_offset(struct cbufhead* chdr, void* data, size_t data_offset, size_t numbytes, size_t offset, cbuf_copier_t copy_into) {
    size_t used_space = cbuf_used_space(chdr);
    size_t oldpos;
    if (used_space <= offset) {
        return 0;
    } else if (used_space < offset + numbytes) {
        numbytes = used_space - offset;
    }
    oldpos = chdr->r_index;
    chdr->r_index = (chdr->r_index + offset) % chdr->size;
    cbuf_read_unsafe(chdr, data, data_offset, numbytes, 0, copy_into);
    chdr->r_index = oldpos;
    return numbytes;
}

size_t cbuf_pop(struct cbufhead* chdr, size_t numbytes) {
    size_t used_space = cbuf_used_space(chdr);
    if (used_space < numbytes) {
        numbytes = used_space;
    }
    chdr->r_index = (chdr->r_index + numbytes) % chdr->size;
    chdr->used -= numbytes;
    return numbytes;
}

void cbuf_reference(const struct cbufhead* chdr, otLinkedBuffer* first, otLinkedBuffer* second) {
    size_t until_end = chdr->size - chdr->r_index;
    if (chdr->used <= until_end) {
        first->mNext = NULL;
        first->mData = &chdr->buf[chdr->r_index];
        first->mLength = (uint16_t) chdr->used;
    } else {
        first->mNext = second;
        first->mData = &chdr->buf[chdr->r_index];
        first->mLength = (uint16_t) until_end;

        second->mNext = NULL;
        second->mData = &chdr->buf[0];
        second->mLength = (uint16_t) (chdr->used - until_end);
    }
}

size_t cbuf_reass_write(struct cbufhead* chdr, size_t offset, const void* data, size_t data_offset, size_t numbytes, uint8_t* bitmap, size_t* firstindex, cbuf_copier_t copy_from) {
    uint8_t* buf_data = chdr->buf;
    size_t free_space = cbuf_free_space(chdr);
    size_t start_index;
    size_t end_index;
    size_t bytes_to_end;
    if (offset > free_space) {
        return 0;
    } else if (offset + numbytes > free_space) {
        numbytes = free_space - offset;
    }
    start_index = (cbuf_get_w_index(chdr) + offset) % chdr->size;
    end_index = (start_index + numbytes) % chdr->size;
    if (end_index >= start_index) {
        copy_from(buf_data, start_index, data, data_offset, numbytes);
        if (bitmap) {
            bmp_setrange(bitmap, start_index, numbytes);
        }
    } else {
        bytes_to_end = chdr->size - start_index;
        copy_from(buf_data, start_index, data, data_offset, bytes_to_end);
        copy_from(buf_data, 0, data, data_offset + bytes_to_end, numbytes - bytes_to_end);
        if (bitmap) {
            bmp_setrange(bitmap, start_index, bytes_to_end);
            bmp_setrange(bitmap, 0, numbytes - bytes_to_end);
        }
    }
    if (firstindex) {
        *firstindex = start_index;
    }
    return numbytes;
}

size_t cbuf_reass_merge(struct cbufhead* chdr, size_t numbytes, uint8_t* bitmap) {
    size_t old_w = cbuf_get_w_index(chdr);
    size_t free_space = cbuf_free_space(chdr);
    size_t bytes_to_end;
    if (numbytes > free_space) {
        numbytes = free_space;
    }
    if (bitmap) {
        bytes_to_end = chdr->size - old_w;
        if (numbytes <= bytes_to_end) {
            bmp_clrrange(bitmap, old_w, numbytes);
        } else {
            bmp_clrrange(bitmap, old_w, bytes_to_end);
            bmp_clrrange(bitmap, 0, numbytes - bytes_to_end);
        }
    }
    chdr->used += numbytes;
    return numbytes;
}

size_t cbuf_reass_count_set(struct cbufhead* chdr, size_t offset, uint8_t* bitmap, size_t limit) {
    size_t bitmap_size = BITS_TO_BYTES(chdr->size);
    size_t until_end;
    offset = (cbuf_get_w_index(chdr) + offset) % chdr->size;
    until_end = bmp_countset(bitmap, bitmap_size, offset, limit);
    if (until_end >= limit || until_end < (chdr->size - offset)) {
        // If we already hit the limit, or if the streak ended before wrapping, then stop here
        return until_end;
    }
    limit -= until_end; // effectively, this is our limit when continuing
    // Continue until either the new limit or until we have scanned OFFSET bits (if we scan more than OFFSET bits, we'll wrap and scan some parts twice)
    return until_end + bmp_countset(bitmap, bitmap_size, 0, limit < offset ? limit : offset);
}

int cbuf_reass_within_offset(struct cbufhead* chdr, size_t offset, size_t index) {
    size_t range_start = cbuf_get_w_index(chdr);
    size_t range_end = (range_start + offset) % chdr->size;
    if (range_end >= range_start) {
        return index >= range_start && index < range_end;
    } else {
        return index < range_end || (index >= range_start && index < chdr->size);
    }
}
