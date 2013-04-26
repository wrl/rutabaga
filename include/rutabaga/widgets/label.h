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

#include <wchar.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/text-object.h"
#include "rutabaga/object.h"

#define RTB_LABEL(x) (&(x)->_rtb_label)

typedef struct rtb_label rtb_label_t;

struct rtb_label {
	RTB_INHERIT(rtb_object);

	/* private ********************************/
	wchar_t *text;
	rtb_font_t *font;
	rtb_text_object_t *tobj;
};

void rtb_label_set_font(rtb_label_t *self, rtb_font_t *font);
void rtb_label_set_text(rtb_label_t *self, const wchar_t *text);

int rtb_label_init(rtb_label_t *label,
		struct rtb_object_implementation *impl);
void rtb_label_fini(rtb_label_t *self);

rtb_label_t *rtb_label_new(const wchar_t *text);
void rtb_label_free(rtb_label_t *);
