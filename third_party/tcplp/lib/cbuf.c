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

/*
 * Copiers for copying from/into cbufs into/from arrays or OpenThraed messages
 */

void cbuf_copy_into_buffer(void* buffer, off_t buffer_offset, const void* arr, off_t arr_offset, size_t num_bytes) {
    uint8_t* bufptr = (uint8_t*) buffer;
    const uint8_t* arrptr = (const uint8_t*) arr;
    memcpy(bufptr + buffer_offset, arrptr + arr_offset, num_bytes);
}

void cbuf_copy_from_buffer(void* arr, off_t arr_offset, const void* buffer, off_t buffer_offset, size_t num_bytes) {
    uint8_t* arrptr = (uint8_t*) arr;
    const uint8_t* bufptr = (const uint8_t*) buffer;
    memcpy(arrptr + arr_offset, bufptr + buffer_offset, num_bytes);
}

void cbuf_copy_into_message(void* buffer, off_t buffer_offset, const void* arr, off_t arr_offset, size_t num_bytes) {
    otMessage* message = (otMessage*) buffer;
    uint8_t* arrptr = (uint8_t*) arr;
    otMessageWrite(message, buffer_offset, arrptr + arr_offset, num_bytes);
}

void cbuf_copy_from_message(void* arr, off_t arr_offset, const void* buffer, off_t buffer_offset, size_t num_bytes) {
    uint8_t* arrptr = (uint8_t*) arr;
    const otMessage* message = (const otMessage*) buffer;
    otMessageRead(message, buffer_offset, arrptr + arr_offset, num_bytes);
}

/*
 * Cbuf implementation.
 */

void cbuf_init(struct cbufhead* chdr, uint8_t* buf, size_t len) {
    chdr->r_index = 0;
    chdr->w_index = 0;
    chdr->size = len;
    chdr->buf = buf;
}

size_t cbuf_used_space(struct cbufhead* chdr) {
    if (chdr->w_index >= chdr->r_index) {
        return chdr->w_index - chdr->r_index;
    } else {
        return chdr->size + chdr->w_index - chdr->r_index;
    }
}

/* There's always one byte of lost space so I can distinguish between a full
   buffer and an empty buffer. */
size_t cbuf_free_space(struct cbufhead* chdr) {
    return chdr->size - 1 - cbuf_used_space(chdr);
}

size_t cbuf_size(struct cbufhead* chdr) {
    return chdr->size - 1;
}

bool cbuf_empty(struct cbufhead* chdr) {
    return (chdr->w_index == chdr->r_index);
}

size_t cbuf_write(struct cbufhead* chdr, const void* data, off_t data_offset, size_t data_len, cbuf_copier_t copy_from) {
    size_t free_space = cbuf_free_space(chdr);
    uint8_t* buf_data;
    size_t fw_index;
    size_t bytes_to_end;
    if (free_space < data_len) {
        data_len = free_space;
    }
    buf_data = chdr->buf;
    fw_index = (chdr->w_index + data_len) % chdr->size;
    if (fw_index >= chdr->w_index) {
        copy_from(buf_data, chdr->w_index, data, data_offset, data_len);
    } else {
        bytes_to_end = chdr->size - chdr->w_index;
        copy_from(buf_data, chdr->w_index, data, data_offset, bytes_to_end);
        copy_from(buf_data, 0, data, data_offset + bytes_to_end, data_len - bytes_to_end);
    }
    chdr->w_index = fw_index;
    return data_len;
}

void cbuf_read_unsafe(struct cbufhead* chdr, void* data, off_t data_offset, size_t numbytes, int pop, cbuf_copier_t copy_into) {
    uint8_t* buf_data = chdr->buf;
    size_t fr_index = (chdr->r_index + numbytes) % chdr->size;
    size_t bytes_to_end;
    if (fr_index >= chdr->r_index) {
        copy_into(data, data_offset, buf_data, chdr->r_index, numbytes);
    } else {
        bytes_to_end = chdr->size - chdr->r_index;
        copy_into(data, data_offset, buf_data, chdr->r_index, bytes_to_end);
        copy_into(data, data_offset + bytes_to_end, buf_data, 0, numbytes - bytes_to_end);
    }
    if (pop) {
        chdr->r_index = fr_index;
    }
}

size_t cbuf_read(struct cbufhead* chdr, void* data, off_t data_offset, size_t numbytes, int pop, cbuf_copier_t copy_into) {
    size_t used_space = cbuf_used_space(chdr);
    if (used_space < numbytes) {
        numbytes = used_space;
    }
    cbuf_read_unsafe(chdr, data, data_offset, numbytes, pop, copy_into);
    return numbytes;
}

size_t cbuf_read_offset(struct cbufhead* chdr, void* data, off_t data_offset, size_t numbytes, size_t offset, cbuf_copier_t copy_into) {
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
    return numbytes;
}

/* Writes DATA to the unused portion of the buffer, at the position OFFSET past
   the end of the buffer. BITMAP is updated by setting bits according to which
   bytes now contain data.
   The index of the first byte written is stored into FIRSTINDEX, if it is not
   NULL. */
size_t cbuf_reass_write(struct cbufhead* chdr, size_t offset, const void* data, off_t data_offset, size_t numbytes, uint8_t* bitmap, size_t* firstindex, cbuf_copier_t copy_from) {
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
    start_index = (chdr->w_index + offset) % chdr->size;
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

/* Writes NUMBYTES bytes to the buffer. The bytes are taken from the unused
   space of the buffer, and can be set using cbuf_reass_write. */
size_t cbuf_reass_merge(struct cbufhead* chdr, size_t numbytes, uint8_t* bitmap) {
    size_t old_w = chdr->w_index;
    size_t free_space = cbuf_free_space(chdr);
    size_t bytes_to_end;
    if (numbytes > free_space) {
        numbytes = free_space;
    }
    chdr->w_index = (chdr->w_index + numbytes) % chdr->size;
    if (bitmap) {
        if (chdr->w_index >= old_w) {
            bmp_clrrange(bitmap, old_w, numbytes);
        } else {
            bytes_to_end = chdr->size - old_w;
            bmp_clrrange(bitmap, old_w, bytes_to_end);
            bmp_clrrange(bitmap, 0, numbytes - bytes_to_end);
        }
    }
    return numbytes;
}

size_t cbuf_reass_count_set(struct cbufhead* chdr, size_t offset, uint8_t* bitmap, size_t limit) {
    size_t bitmap_size = BITS_TO_BYTES(chdr->size);
    size_t until_end;
    offset = (chdr->w_index + offset) % chdr->size;
    until_end = bmp_countset(bitmap, bitmap_size, offset, limit);
    if (until_end >= limit || until_end < (chdr->size - offset)) {
        // If we already hit the limit, or if the streak ended before wrapping, then stop here
        return until_end;
    }
    limit -= until_end; // effectively, this is our limit when continuing
    // Continue until either the new limit or until we have scanned OFFSET bits (if we scan more than OFFSET bits, we'll wrap and scan some parts twice)
    return until_end + bmp_countset(bitmap, bitmap_size, 0, limit < offset ? limit : offset);
}

/* Returns a true value iff INDEX is the index of a byte within OFFSET bytes
   past the end of the buffer. */
int cbuf_reass_within_offset(struct cbufhead* chdr, size_t offset, size_t index) {
    size_t range_start = chdr->w_index;
    size_t range_end = (range_start + offset) % chdr->size;
    if (range_end >= range_start) {
        return index >= range_start && index < range_end;
    } else {
        return index < range_end || (index >= range_start && index < chdr->size);
    }
}

#if 0 // The segment functionality doesn't look like it's going to be used

/* Reads NBYTES bytes of the first segment into BUF. If there aren't NBYTES
   to read in the buffer, does nothing and returns 0. Otherwise, returns
   the number of bytes read. */
size_t cbuf_peek_segment(struct cbufhead* chdr, uint8_t* data, size_t numbytes) {
    size_t used_space = cbuf_used_space(chdr);
    size_t old_ridx;
    if (used_space < numbytes + sizeof(size_t)) {
        return 0;
    }
    old_ridx = chdr->r_index;
    chdr->r_index = (chdr->r_index + sizeof(size_t)) % chdr->size;
    _cbuf_read_unsafe(chdr, data, numbytes, 0);
    chdr->r_index = old_ridx;
    return numbytes;
}


int cbuf_write_segment(struct cbufhead* chdr, uint8_t* segment, size_t seglen) {
    if (_cbuf_free_space(chdr) < seglen + sizeof(seglen)) {
        return -1;
    }
    cbuf_write(chdr, (uint8_t*) &seglen, sizeof(seglen));
    cbuf_write(chdr, segment, seglen);
    return 0;
}

size_t cbuf_peek_segment_size(struct cbufhead* chdr) {
    size_t segsize;
    if (cbuf_read(chdr, (uint8_t*) &segsize,
                  sizeof(size_t), 0) < sizeof(size_t)) {
        return 0;
    }
    return segsize;
}

size_t cbuf_pop_segment(struct cbufhead* chdr, size_t segsize) {
    if (!segsize) {
        segsize = cbuf_peek_segment_size(chdr);
    }
    return cbuf_pop(chdr, segsize + sizeof(size_t));
}

#endif
