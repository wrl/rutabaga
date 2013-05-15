/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013 William Light.
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

#include <unistd.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/object.h"
#include "rutabaga/event.h"

#include "rutabaga/widgets/label.h"
#include "wwrl/vector.h"

#define RTB_TEXT_INPUT(x) RTB_UPCAST(x, rtb_text_input)

typedef struct rtb_text_input rtb_text_input_t;

struct rtb_text_input {
	RTB_INHERIT(rtb_object);

	/* private ********************************/
	struct {
		int draw;
		int edit;
	} cursor;

	VECTOR(text, rtb_utf8_t) text;
	rtb_label_t label;
	GLuint vbo[2];
};

/**
 * if `nbytes` is -1, strlen() will be run on `text` to determine
 * the length.
 */
int rtb_text_input_set_text(rtb_text_input_t *,
		rtb_utf8_t *text, ssize_t nbytes);
const rtb_utf8_t *rtb_text_input_get_text(rtb_text_input_t *);

int rtb_text_input_init(rtb_text_input_t *,
		struct rtb_object_implementation *impl);
void rtb_text_input_fini(rtb_text_input_t *);
rtb_text_input_t *rtb_text_input_new(void);
void rtb_text_input_free(rtb_text_input_t *);
