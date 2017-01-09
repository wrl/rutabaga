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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/layout.h"
#include "rutabaga/event.h"
#include "rutabaga/mouse.h"
#include "rutabaga/platform.h"

#include "rutabaga/widgets/spinbox.h"

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_spinbox *self = RTB_ELEMENT_AS(elem, rtb_spinbox)

static struct rtb_element_implementation super;

#define DELTA_VALUE_STEP_COARSE	.005f
#define DELTA_VALUE_STEP_FINE	.0005f

/**
 * event handling
 */

static int
handle_drag(struct rtb_spinbox *self, const struct rtb_drag_event *e)
{
	float new_value;
	float mult;

	if (!rtb_elem_is_in_tree(RTB_ELEMENT(self), e->target))
		return 0;

	new_value = self->normalised_value;

	switch (e->button) {
	case RTB_MOUSE_BUTTON1:
		if (e->mod_keys & RTB_KEY_MOD_SHIFT)
			mult = DELTA_VALUE_STEP_FINE;
		else
			mult = DELTA_VALUE_STEP_COARSE;

		break;

	case RTB_MOUSE_BUTTON2:
		mult = DELTA_VALUE_STEP_FINE;
		break;

	default:
		return 1;
	}

	new_value += -e->delta.y * mult;

	rtb__value_element_set_normalised_value(RTB_VALUE_ELEMENT(self),
			new_value, 0);

	rtb_mouse_pointer_warp(self->window, e->start.x, e->start.y);
	return 1;
}

static int
handle_mouse_down(struct rtb_spinbox *self, const struct rtb_mouse_event *e)
{
	switch (e->button) {
	case RTB_MOUSE_BUTTON1:
	case RTB_MOUSE_BUTTON2:
		rtb_mouse_set_cursor(self->window, &self->window->mouse,
				RTB_MOUSE_CURSOR_HIDDEN);
		return 1;

	default:
		return 0;
	}
}

static int
handle_mouse_click(struct rtb_spinbox *self, const struct rtb_mouse_event *e)
{
	if (e->button == RTB_MOUSE_BUTTON1 && e->click_number > 0
			&& e->button_state != RTB_MOUSE_BUTTON_STATE_DRAG) {
		rtb__value_element_set_value_uncooked(RTB_VALUE_ELEMENT(self),
				self->origin, 0);
		return 1;
	}

	return 0;
}

static int
handle_mouse_wheel(struct rtb_spinbox *self, const struct rtb_mouse_event *e)
{
	float new_value, mult;

	if (e->mod_keys & RTB_KEY_MOD_SHIFT)
		mult = DELTA_VALUE_STEP_FINE;
	else
		mult = DELTA_VALUE_STEP_COARSE;

	new_value = self->normalised_value + (e->wheel.delta * mult);
	rtb__value_element_set_normalised_value(RTB_VALUE_ELEMENT(self),
			new_value, 0);
	return 1;
}

static int
handle_key(struct rtb_spinbox *self, const struct rtb_key_event *e)
{
	float step;

	if (e->mod_keys & RTB_KEY_MOD_SHIFT)
		step = DELTA_VALUE_STEP_FINE;
	else if (e->mod_keys & RTB_KEY_MOD_CTRL)
		step = DELTA_VALUE_STEP_COARSE * 2;
	else
		step = DELTA_VALUE_STEP_COARSE;

	switch (e->keysym) {
	case RTB_KEY_UP:
	case RTB_KEY_NUMPAD_UP:
		rtb__value_element_set_normalised_value(RTB_VALUE_ELEMENT(self),
				self->normalised_value + step, 0);
		return 1;

	case RTB_KEY_DOWN:
	case RTB_KEY_NUMPAD_DOWN:
		rtb__value_element_set_normalised_value(RTB_VALUE_ELEMENT(self),
				self->normalised_value - step, 0);
		return 1;

	default:
		return 0;
	}
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAG_MOTION:
		return handle_drag(self, RTB_EVENT_AS(e, rtb_drag_event));

	case RTB_MOUSE_DOWN:
		return handle_mouse_down(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_MOUSE_CLICK:
		return handle_mouse_click(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_MOUSE_WHEEL:
		return handle_mouse_wheel(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_KEY_PRESS:
		return handle_key(self, RTB_EVENT_AS(e, rtb_key_event));

	case RTB_MOUSE_UP:
	case RTB_DRAG_DROP:
		rtb_mouse_unset_cursor(self->window, &self->window->mouse);
		return 1;

	default:
		return super.on_event(elem, e);
	}

	return 0;
}


/**
 * internal API hooks
 */

static void
set_value_hook(struct rtb_element *elem)
{
	SELF_FROM(elem);
	char buf[32];

	snprintf(buf, sizeof(buf), self->format_string, self->value);
	rtb_label_set_text(&self->value_label, buf);
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.spinbox");

	set_value_hook(elem);
}

/**
 * public API
 */

int
rtb_spinbox_init(struct rtb_spinbox *self)
{
	if (RTB_SUBCLASS(RTB_VALUE_ELEMENT(self), rtb_value_element_init, &super))
		return -1;

	self->attached = attached;
	self->on_event = on_event;

	self->size_cb   = rtb_size_hfit_children;
	self->layout_cb = rtb_layout_hpack_center;

	self->set_value_hook = set_value_hook;

	self->format_string = "%.2f";

	rtb_label_init(&self->value_label);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->value_label),
			RTB_ADD_TAIL);

	self->value_label.align = RTB_ALIGN_CENTER | RTB_ALIGN_MIDDLE;
	return 0;
}

void
rtb_spinbox_fini(struct rtb_spinbox *self)
{
	rtb_label_fini(&self->value_label);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_spinbox *
rtb_spinbox_new()
{
	struct rtb_spinbox *self = calloc(1, sizeof(struct rtb_spinbox));
	rtb_spinbox_init(self);
	return self;
}

void
rtb_spinbox_free(struct rtb_spinbox *self)
{
	rtb_spinbox_fini(self);
	free(self);
}
