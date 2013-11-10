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
#include "rutabaga/stylequad.h"
#include "rutabaga/event.h"
#include "rutabaga/mat4.h"

#define RTB_KNOB(x) RTB_UPCAST(x, rtb_knob)

typedef enum {
	KNOB_VALUE_CHANGE = 1
} rtb_knob_event_type_t;

struct rtb_knob_event {
	RTB_INHERIT(rtb_event);
	float value;
};

struct rtb_knob {
	RTB_INHERIT(rtb_element);

	/* public *********************************/
	float origin;
	float min;
	float max;

	/* private ********************************/
	struct rtb_stylequad rotor;
	mat4 modelview;
	float value;
};

void rtb_knob_set_value(struct rtb_knob *, float new_value);

int rtb_knob_init(struct rtb_knob *);
void rtb_knob_fini(struct rtb_knob *);
struct rtb_knob *rtb_knob_new(void);
void rtb_knob_free(struct rtb_knob *);
