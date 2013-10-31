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

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/text-object.h"

#define RTB_LABEL(x) RTB_UPCAST(x, rtb_label)

struct rtb_label {
	RTB_INHERIT(rtb_element);

	/* private ********************************/
	rtb_utf8_t *text;
	struct rtb_font *font;
	struct rtb_text_object *tobj;
	const struct rtb_rgb_color *color;
};

void rtb_label_set_font(struct rtb_label *, struct rtb_font *font);
void rtb_label_set_text(struct rtb_label *, const rtb_utf8_t *text);

int rtb_label_init_subclass(struct rtb_label *,
		struct rtb_element_implementation *label_impl);
int rtb_label_init(struct rtb_label *);
void rtb_label_fini(struct rtb_label *);

struct rtb_label *rtb_label_new(const rtb_utf8_t *text);
void rtb_label_free(struct rtb_label *);
