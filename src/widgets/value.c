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
	struct rtb_value_change_event event = {
		.type   = RTB_VALUE_CHANGE,
		.source = synthetic ? RTB_EVENT_SYNTHETIC : RTB_EVENT_GENUINE,
		.value  = self->value
	};

	return rtb_elem_deliver_event(RTB_ELEMENT(self), RTB_EVENT(&event));
}

/**
 * lightweight state machine
 */

int
change_state(struct rtb_value_element *self,
		rtb_value_element_state_t new_state, int active)
{
	rtb_value_element_state_t old_state;
	struct rtb_value_state_event event = {
		.type   = RTB_VALUE_STATE_CHANGE,
		.source = RTB_EVENT_GENUINE,
		.being_edited = active
	};

	if (new_state == RTB_VALUE_STATE_AT_REST)
		/* invalid value, use active = 0 */
		return -1;

	old_state = self->ve_state;

	if (active) {
		switch (self->ve_state) {
		case RTB_VALUE_STATE_DRAG_EDIT:
			/* can't transition from drag edit to any other active states */
			return 0;

		case RTB_VALUE_STATE_WHEEL_EDIT:
			if (new_state != RTB_VALUE_STATE_DRAG_EDIT)
				/* for example, mousewheel -> key or click should stay on the
				 * mousewheel state until the mouse exits the element. */
				return 0;
			break;

		case RTB_VALUE_STATE_AT_REST:
		case RTB_VALUE_STATE_DISCRETE_EDIT:
			break;
		}

		if (self->ve_state == new_state)
			/* nothing to do */
			return 0;

		self->ve_state = new_state;
		if (new_state && old_state)
			return 0;
	} else if (new_state == self->ve_state) {
		/* we can only transition out of a state from itself. this prevents
		 * issues like mousewheel during drag prematurely exiting from an
		 * active state. */

		self->ve_state = RTB_VALUE_STATE_AT_REST;
	} else {
		return 0;
	}

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

	if ((e->start_mod_keys & (RTB_KEY_MOD_CTRL | RTB_KEY_MOD_ALT)))
		return 1;

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

	mult *= self->delta_mult;
	new_value += -e->delta.y * mult;

	rtb__value_element_set_normalised_value(self, new_value, 0);

	if (fabsf(e->cursor.x - e->start.x) > 1.f ||
			fabsf(e->cursor.y - e->start.y) > 1.f)
		rtb_mouse_pointer_warp(self->window, e->start);

	return 1;
}

static int
handle_mouse_down(struct rtb_value_element *self,
		const struct rtb_mouse_event *e)
{
	if (e->button != RTB_MOUSE_BUTTON1 ||
			(e->mod_keys & (RTB_KEY_MOD_CTRL | RTB_KEY_MOD_ALT)))
		return 0;

	rtb_mouse_set_cursor(self->window, &self->window->mouse,
			RTB_MOUSE_CURSOR_HIDDEN);

	change_state(self, RTB_VALUE_STATE_DRAG_EDIT, 1);
	return 1;
}

static int
handle_mouse_click(struct rtb_value_element *self,
		const struct rtb_mouse_event *e)
{
	if (e->button != RTB_MOUSE_BUTTON1 || !e->click_number
			|| e->button_state == RTB_MOUSE_BUTTON_STATE_DRAG)
		return 0;

	change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 1);
	rtb__value_element_set_value_uncooked(self,
			self->origin, 0);
	change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 0);

	return 1;
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

	mult *= self->delta_mult;

	new_value = self->normalised_value + (e->wheel.delta * mult);

	/* FIXME: how should mousewheel input during an active mouse drag edit be
	 * handled? currently it is allowed, but should we discard it? */

	change_state(self, RTB_VALUE_STATE_WHEEL_EDIT, 1);
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

	step *= self->delta_mult;

	switch (e->keysym) {
	case RTB_KEY_UP:
	case RTB_KEY_NUMPAD_UP:
		break;

	case RTB_KEY_DOWN:
	case RTB_KEY_NUMPAD_DOWN:
		step = -step;
		break;

	default:
		return 0;
	}

	change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 1);
	rtb__value_element_set_normalised_value(self,
			self->normalised_value + step, 0);
	change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 0);

	return 1;
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	const struct rtb_drag_event *drag_event = RTB_EVENT_AS(e, rtb_drag_event);
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_DRAG_START:
		if ((drag_event->mod_keys & (RTB_KEY_MOD_CTRL | RTB_KEY_MOD_ALT))
				|| drag_event->button != RTB_MOUSE_BUTTON1) {
			rtb_mouse_unset_cursor(self->window, &self->window->mouse);
			change_state(self, RTB_VALUE_STATE_DRAG_EDIT, 0);
			return 0;
		}

		rtb_mouse_set_cursor(self->window, &self->window->mouse,
				RTB_MOUSE_CURSOR_HIDDEN);

		/* fall-through */

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
		if (rtb_elem_is_in_tree(RTB_ELEMENT(self), drag_event->target)
				&& !(drag_event->start_mod_keys &
						(RTB_KEY_MOD_CTRL | RTB_KEY_MOD_ALT))) {
			rtb_mouse_pointer_warp(self->window, drag_event->start);
		}

		/* fall-through */

	case RTB_MOUSE_UP:
		if (drag_event->button != RTB_MOUSE_BUTTON1)
			return 0;

		if (rtb_elem_is_in_tree(RTB_ELEMENT(self), drag_event->target)) {
			rtb_mouse_unset_cursor(self->window, &self->window->mouse);
			change_state(self, RTB_VALUE_STATE_DRAG_EDIT, 0);
		}

		return 1;

	case RTB_MOUSE_LEAVE:
		if (self->ve_state == RTB_VALUE_STATE_WHEEL_EDIT)
			change_state(self, RTB_VALUE_STATE_WHEEL_EDIT, 0);
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
		self->set_value_hook(RTB_ELEMENT(self), synthetic);

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
		float new_value, rtb_ev_source_t source)
{
	int synthetic = (source == RTB_EVENT_SYNTHETIC) ? 1 : 0;

	if (synthetic) {
		rtb__value_element_set_normalised_value(self, new_value, synthetic);
	} else {
		change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 1);
		rtb__value_element_set_normalised_value(self, new_value, synthetic);
		change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 0);
	}
}

void
rtb_value_element_set_value(struct rtb_value_element *self, float new_value,
		rtb_ev_source_t source)
{
	int synthetic = (source == RTB_EVENT_SYNTHETIC) ? 1 : 0;

	if (synthetic) {
		rtb__value_element_set_value_uncooked(self, new_value, synthetic);
	} else {
		change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 1);
		rtb__value_element_set_value_uncooked(self, new_value, synthetic);
		change_state(self, RTB_VALUE_STATE_DISCRETE_EDIT, 0);
	}
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

	self->ve_state = RTB_VALUE_STATE_AT_REST;

	self->delta_mult = 1.f;
	self->min = 0.f;
	self->max = 1.f;

	self->flags |= RTB_ELEM_ACTIVE_DURING_DRAG;

	return 0;
}

void
rtb_value_element_fini(struct rtb_value_element *self)
{
	rtb_elem_fini(RTB_ELEMENT(self));
}
