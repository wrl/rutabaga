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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>
#include <rutabaga/event.h>
#include <rutabaga/mouse.h>
#include <rutabaga/platform.h>

#include <rutabaga/widgets/value.h>

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_value_element *self = RTB_ELEMENT_AS(elem, rtb_value_element)

static struct rtb_element_implementation super;

/* would be cool to have this be like a smoothed equation or smth */
#define DELTA_VALUE_STEP_COARSE	.005f
#define DELTA_VALUE_STEP_FINE	.0005f

/**
 * event dispatching
 */

static int
dispatch_value_change_event(struct rtb_value_element *self, int synthetic)
{
	struct rtb_value_event event = {
		.type   = RTB_VALUE_CHANGE,
		.source = synthetic ? RTB_EVENT_SYNTHETIC : RTB_EVENT_GENUINE,
		.value  = self->value
	};

	return rtb_elem_deliver_event(RTB_ELEMENT(self), RTB_EVENT(&event));
}

/**
 * event handling
 */

static int
handle_drag(struct rtb_value_element *self, const struct rtb_drag_event *e)
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

	rtb__value_element_set_normalised_value(self, new_value, 0);

	if (fabsf(e->cursor.x - e->start.x) > 1.f ||
			fabsf(e->cursor.y - e->start.y) > 1.f)
		rtb_mouse_pointer_warp(self->window, e->start.x, e->start.y);

	return 1;
}

static int
handle_mouse_down(struct rtb_value_element *self,
		const struct rtb_mouse_event *e)
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
handle_mouse_click(struct rtb_value_element *self,
		const struct rtb_mouse_event *e)
{
	if (e->button == RTB_MOUSE_BUTTON1 && e->click_number > 0
			&& e->button_state != RTB_MOUSE_BUTTON_STATE_DRAG) {
		rtb__value_element_set_value_uncooked(self,
				self->origin, 0);
		return 1;
	}

	return 0;
}

static int
handle_mouse_wheel(struct rtb_value_element *self,
		const struct rtb_mouse_event *e)
{
	float new_value, mult;

	if (e->mod_keys & RTB_KEY_MOD_SHIFT)
		mult = DELTA_VALUE_STEP_FINE;
	else
		mult = DELTA_VALUE_STEP_COARSE;

	new_value = self->normalised_value + (e->wheel.delta * mult);
	rtb__value_element_set_normalised_value(self, new_value, 0);
	return 1;
}

static int
handle_key(struct rtb_value_element *self, const struct rtb_key_event *e)
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
		rtb__value_element_set_normalised_value(self,
				self->normalised_value + step, 0);
		return 1;

	case RTB_KEY_DOWN:
	case RTB_KEY_NUMPAD_DOWN:
		rtb__value_element_set_normalised_value(self,
				self->normalised_value - step, 0);
		return 1;

	default:
		return 0;
	}
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	const struct rtb_drag_event *drag_event = RTB_EVENT_AS(e, rtb_drag_event);
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAG_MOTION:
		return handle_drag(self, drag_event);

	case RTB_MOUSE_DOWN:
		return handle_mouse_down(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_MOUSE_CLICK:
		return handle_mouse_click(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_MOUSE_WHEEL:
		return handle_mouse_wheel(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_KEY_PRESS:
		return handle_key(self, RTB_EVENT_AS(e, rtb_key_event));

	case RTB_DRAG_DROP:
		if (rtb_elem_is_in_tree(RTB_ELEMENT(self), drag_event->target))
			rtb_mouse_pointer_warp(self->window,
					drag_event->start.x, drag_event->start.y);
		/* fall-through */

	case RTB_MOUSE_UP:
		if (rtb_elem_is_in_tree(RTB_ELEMENT(self), drag_event->target))
			rtb_mouse_unset_cursor(self->window, &self->window->mouse);
		return 1;

	default:
		return super.on_event(elem, e);
	}

	return 0;
}


/**
 * element impl
 */

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);

	rtb__value_element_set_value_uncooked(self,
		(self->normalised_value == -1.f) ? self->origin : self->value, 1);

	if (self->normalised_value == -1.f)
		rtb__value_element_set_value_uncooked(self, self->origin, 1);
	else
		rtb__value_element_set_value_uncooked(self, self->value, 1);
}

/**
 * protected API
 */

void
rtb__value_element_set_normalised_value(struct rtb_value_element *self,
		float new_normalised_value, int synthetic)
{
	float new_value;

	new_normalised_value = fmin(fmax(new_normalised_value, 0.f), 1.f);
	self->normalised_value = new_normalised_value;

	new_value = self->min +
		(new_normalised_value * (self->max - self->min));

	if (self->granularity != 0.f) {
		new_value = floorf(new_value / self->granularity) * self->granularity;

		if (new_value == self->value)
			return;
	}

	self->value = new_value;

	if (self->set_value_hook)
		self->set_value_hook(RTB_ELEMENT(self));

	if (self->state != RTB_STATE_UNATTACHED)
		dispatch_value_change_event(self, synthetic);
}

void
rtb__value_element_set_value_uncooked(struct rtb_value_element *self,
		float new_value, int synthetic)
{
	float range = self->max - self->min;
	rtb__value_element_set_normalised_value(self,
			(new_value - self->min) / range, synthetic);
}


/**
 * public API
 */

void
rtb_value_element_set_normalised_value(struct rtb_value_element *self,
		float new_value)
{
	rtb__value_element_set_normalised_value(self, new_value, 1);
}

void
rtb_value_element_set_value(struct rtb_value_element *self, float new_value)
{
	rtb__value_element_set_value_uncooked(self, new_value, 1);
}

int
rtb_value_element_init(struct rtb_value_element *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	self->attached = attached;
	self->on_event = on_event;

	self->granularity  =
		self->value    =
		self->origin   = 0.f;

	/* -1.f is a special value indicating that no user/client code has
	 * not modified the value in-between creating the element and window
	 * initialisation. */
	self->normalised_value = -1.f;

	self->min = 0.f;
	self->max = 1.f;

	return 0;
}

void
rtb_value_element_fini(struct rtb_value_element *self)
{
	rtb_elem_fini(RTB_ELEMENT(self));
}
