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

#ifndef CBUF_H_
#define CBUF_H_

/* CIRCULAR BUFFER
   The circular buffer can be treated either as a buffer of bytes, or a buffer
   of TCP segments. Don't mix and match the functions unless you know what
   you're doing! */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

struct cbufhead {
    size_t r_index;
    size_t w_index;
    size_t size;
    uint8_t* buf;
};

void cbuf_init(struct cbufhead* chdr, uint8_t* buf, size_t len);

void cbuf_copy_into_buffer(void* buffer, off_t buffer_offset, const void* arr, off_t arr_offset, size_t num_bytes);
void cbuf_copy_from_buffer(void* arr, off_t arr_offset, const void* buffer, off_t buffer_offset, size_t num_bytes);

void cbuf_copy_into_message(void* buffer, off_t buffer_offset, const void* arr, off_t arr_offset, size_t num_bytes);
void cbuf_copy_from_message(void* arr, off_t arr_offset, const void* buffer, off_t buffer_offset, size_t num_bytes);

typedef void(*cbuf_copier_t)(void*, off_t, const void*, off_t, size_t);

size_t cbuf_write(struct cbufhead* chdr, const void* data, off_t data_offset, size_t data_len, cbuf_copier_t copy_from);
size_t cbuf_read(struct cbufhead* chdr, void* data, off_t data_offset, size_t numbytes, int pop, cbuf_copier_t copy_into);
size_t cbuf_read_offset(struct cbufhead* chdr, void* data, off_t data_offset, size_t numbytes, size_t offset, cbuf_copier_t copy_into);
size_t cbuf_pop(struct cbufhead* chdr, size_t numbytes);
size_t cbuf_used_space(struct cbufhead* chdr);
size_t cbuf_free_space(struct cbufhead* chdr);
size_t cbuf_size(struct cbufhead* chdr);
bool cbuf_empty(struct cbufhead* chdr);

size_t cbuf_reass_write(struct cbufhead* chdr, size_t offset, const void* data, off_t data_offset, size_t numbytes, uint8_t* bitmap, size_t* firstindex, cbuf_copier_t copy_from);
size_t cbuf_reass_merge(struct cbufhead* chdr, size_t numbytes, uint8_t* bitmap);
size_t cbuf_reass_count_set(struct cbufhead* chdr, size_t offset, uint8_t* bitmap, size_t limit);
int cbuf_reass_within_offset(struct cbufhead* chdr, size_t offset, size_t index);

/*
int cbuf_write_segment(struct cbufhead* chdr, uint8_t* segment, size_t seglen);
size_t cbuf_pop_segment(struct cbufhead* chdr, size_t segsize);
size_t cbuf_peek_segment_size(struct cbufhead* chdr);
size_t cbuf_peek_segment(struct cbufhead* chdr, uint8_t* data, size_t numbytes);
*/

#endif
