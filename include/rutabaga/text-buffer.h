/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "rutabaga/types.h"

#include "wwrl/vector.h"

VECTOR(rtb_text_buffer, rtb_utf8_t);

int rtb_text_buffer_insert_u32(struct rtb_text_buffer *,
		int after_idx, rtb_utf32_t c);
int rtb_text_buffer_erase_char(struct rtb_text_buffer *, int idx);

/**
 * if `nbytes` is -1, it will be determined with strlen().
 */
int rtb_text_buffer_set_text(struct rtb_text_buffer *,
		rtb_utf8_t *text, ssize_t nbytes);
const rtb_utf8_t *rtb_text_buffer_get_text(struct rtb_text_buffer *);

int rtb_text_buffer_init(struct rutabaga *, struct rtb_text_buffer *);
void rtb_text_buffer_fini(struct rtb_text_buffer *);
