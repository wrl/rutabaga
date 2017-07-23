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

#include <string.h>

#include "wwrl/vector.h"

#include "rutabaga/rutabaga.h"
#include "rutabaga/text-buffer.h"

#include "rtb_private/utf8.h"

#define UTF8_IS_CONTINUATION(byte) (((byte) & 0xC0) == 0x80)

static int
utf8_idx(struct rtb_text_buffer *self, int idx)
{
	int u8_idx, size = self->size;

	for (u8_idx = 0; idx >= 0 && u8_idx <= size; idx--, u8_idx++)
		while (UTF8_IS_CONTINUATION(self->data[u8_idx]) && u8_idx <= size)
			u8_idx++;

	if (u8_idx > size)
		return -1;

	if (u8_idx > 0)
		u8_idx--;

	return u8_idx;
}

/**
 * single character operations
 */

int
rtb_text_buffer_insert_u32(struct rtb_text_buffer *self,
		int after_idx, rtb_utf32_t c)
{
	rtb_utf8_t utf[6];
	int len;

	len = u8enc(c, utf);
	VECTOR_INSERT_DATA(self, utf8_idx(self, after_idx), utf, len);

	return 0;
}

int
rtb_text_buffer_erase_char(struct rtb_text_buffer *self, int idx)
{
	const uint8_t *front, *utf8_seq;
	int u8_idx, size = self->size;

	u8_idx = utf8_idx(self, idx);

	if (!(size > 1) || u8_idx < 0)
		return -1;

	front = (void *) VECTOR_FRONT(self);
	utf8_seq = &front[u8_idx - 1];

	/* seek backward to the start of the utf-8 sequence */
	while (UTF8_IS_CONTINUATION(*utf8_seq) && utf8_seq >= front)
		utf8_seq--;

	VECTOR_ERASE_RANGE(self, (utf8_seq - front), u8_idx);
	return 0;
}

/**
 * whole-buffer operations
 */

int
rtb_text_buffer_set_text(struct rtb_text_buffer *self,
		rtb_utf8_t *text, ssize_t nbytes)
{
	char null = '\0';

	if (nbytes < 0)
		nbytes = strlen(text);

	VECTOR_CLEAR(self);
	VECTOR_PUSH_BACK_DATA(self, text, nbytes);
	VECTOR_PUSH_BACK(self, &null);

	return 0;
}

const rtb_utf8_t *
rtb_text_buffer_get_text(struct rtb_text_buffer *self)
{
	return self->data;
}

/**
 * lifecycle
 */

int
rtb_text_buffer_init(struct rutabaga *rtb, struct rtb_text_buffer *self)
{
	char null = '\0';

	VECTOR_INIT(self, &rtb->allocator, 32);
	VECTOR_PUSH_BACK(self, &null);

	return 0;
}

void
rtb_text_buffer_fini(struct rtb_text_buffer *self)
{
	VECTOR_FREE(self);
}
