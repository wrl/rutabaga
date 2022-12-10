/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2018 William Light.
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

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>

#define RTB_VALUE_ELEMENT(x) RTB_UPCAST(x, rtb_value_element)
#define RTB_VALUE_ELEMENT_AS(x, type) RTB_DOWNCAST(x, type, rtb_value_element)

typedef enum {
	RTB_VALUE_CHANGE = 1,
	RTB_VALUE_STATE_CHANGE,

	RTB_VALUE_DELTA,
	RTB_VALUE_RESET
} rtb_value_event_type_t;

struct rtb_value_change_event {
	RTB_INHERIT(rtb_event);
	float value;
};

struct rtb_value_state_event {
	RTB_INHERIT(rtb_event);
	int being_edited;
};

struct rtb_value_delta_event {
	RTB_INHERIT(rtb_event);
	float delta;
};

typedef enum {
	RTB_VALUE_STATE_AT_REST       = 0,
	RTB_VALUE_STATE_DRAG_EDIT     = 1,
	RTB_VALUE_STATE_WHEEL_EDIT    = 2,
	RTB_VALUE_STATE_DISCRETE_EDIT = 3
} rtb_value_element_state_t;

struct rtb_value_element {
	RTB_INHERIT(rtb_element);

	/* public *********************************/
	float origin;
	float min;
	float max;
	float granularity;
	float delta_mult;

	unsigned deny_drag_start_mod_mask;

	/* read-only ******************************/
	float value;

	/* private ********************************/
	rtb_value_element_state_t ve_state;
	float normalised_value;

	void (*set_value_hook)(struct rtb_element *, int synthetic);
};

/**
 * protected
 */

void
rtb__value_element_set_normalised_value(struct rtb_value_element *,
		float new_normalised_value, int synthetic);

void
rtb__value_element_set_value_uncooked(struct rtb_value_element *self,
		float new_value, int synthetic);

/**
 * public
 */

void rtb_value_element_set_normalised_value(struct rtb_value_element *,
		float new_value, rtb_ev_source_t source);

void rtb_value_element_set_value(struct rtb_value_element *,
		float new_value, rtb_ev_source_t source);

int rtb_value_element_init(struct rtb_value_element *);
void rtb_value_element_fini(struct rtb_value_element *);
