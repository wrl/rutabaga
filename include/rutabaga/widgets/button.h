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
#include "rutabaga/object.h"
#include "rutabaga/event.h"

#include "rutabaga/widgets/label.h"

#define RTB_BUTTON(x) RTB_UPCAST(x, rtb_button)

typedef struct rtb_button rtb_button_t;
typedef struct rtb_event_button rtb_ev_button_t;

typedef enum {
	BUTTON_CLICK = 1
} rtb_ev_button_type_t;

struct rtb_event_button {
	RTB_INHERIT(rtb_event_mouse);
};

struct rtb_button {
	RTB_INHERIT(rtb_object);

	/* private ********************************/
	rtb_label_t label;
	GLuint vbo;
};

void rtb_button_set_label(rtb_button_t *self, const rtb_utf8_t *text);

int rtb_button_init(rtb_button_t *, struct rtb_object_implementation *impl);
void rtb_button_fini(rtb_button_t *);
rtb_button_t *rtb_button_new(const rtb_utf8_t *label);
void rtb_button_free(rtb_button_t *);
