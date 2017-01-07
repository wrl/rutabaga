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
#include "rutabaga/window.h"
#include "rutabaga/mouse.h"
#include "rutabaga/render.h"
#include "rutabaga/event.h"
#include "rutabaga/style.h"
#include "rutabaga/keyboard.h"
#include "rutabaga/platform.h"

#include "rutabaga/widgets/knob.h"

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_knob *self = RTB_ELEMENT_AS(elem, rtb_knob)

#define MIN_DEGREES 35.f
#define MAX_DEGREES (360.f - MIN_DEGREES)
#define DEGREE_RANGE (MAX_DEGREES - MIN_DEGREES)

/* would be cool to have this be like a smoothed equation or smth */
#define DELTA_VALUE_STEP_COARSE	.005f
#define DELTA_VALUE_STEP_FINE	.0005f

static struct rtb_element_implementation super;

/**
 * drawing
 */

static void
draw(struct rtb_element *elem)
{
	SELF_FROM(elem);

	super.draw(elem);
	rtb_stylequad_draw_with_modelview(&self->rotor, elem, &self->modelview);
}

/**
 * event handling
 */

static void
set_value_hook(struct rtb_element *elem)
{
	SELF_FROM(elem);

	mat4_set_rotation(&self->modelview,
			MIN_DEGREES + (self->normalised_value * DEGREE_RANGE),
			0.f, 0.f, 1.f);

	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

static int
handle_drag(struct rtb_knob *self, const struct rtb_drag_event *e)
{
	float new_value;
	float mult;

	if (e->target != RTB_ELEMENT(self))
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

	if (fabsf(e->cursor.x - e->start.x) > 1.f ||
			fabsf(e->cursor.y - e->start.y) > 1.f)
		rtb_mouse_pointer_warp(self->window, e->start.x, e->start.y);

	return 1;
}

static int
handle_mouse_down(struct rtb_knob *self, const struct rtb_mouse_event *e)
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
handle_mouse_click(struct rtb_knob *self, const struct rtb_mouse_event *e)
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
handle_mouse_wheel(struct rtb_knob *self, const struct rtb_mouse_event *e)
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
handle_key(struct rtb_knob *self, const struct rtb_key_event *e)
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
		rtb_mouse_pointer_warp(self->window,
				drag_event->start.x, drag_event->start.y);
		/* fall-through */

	case RTB_MOUSE_UP:
		rtb_mouse_unset_cursor(self->window, &self->window->mouse);
		return 1;

	default:
		return super.on_event(elem, e);
	}

	return 0;
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.knob");

	set_value_hook(elem);
}

static void
restyle(struct rtb_element *elem)
{
	SELF_FROM(elem);

	const struct rtb_style_property_definition *prop;
	super.restyle(elem);

	prop = rtb_style_query_prop(elem,
			"-rtb-knob-rotor", RTB_STYLE_PROP_TEXTURE, 0);
	if (prop &&
			!rtb_stylequad_set_background_image(&self->rotor, &prop->texture))
		rtb_elem_mark_dirty(elem);
}

static int
reflow(struct rtb_element *elem,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	SELF_FROM(elem);

	if (!super.reflow(elem, instigator, direction))
		return 0;

	rtb_stylequad_update_geometry(&self->rotor, &self->rect);
	return 1;
}

/**
 * public API
 */

int
rtb_knob_init(struct rtb_knob *self)
{
	if (RTB_SUBCLASS(RTB_VALUE_ELEMENT(self), rtb_value_element_init, &super))
		return -1;

	self->draw     = draw;
	self->on_event = on_event;
	self->attached = attached;
	self->restyle  = restyle;
	self->reflow   = reflow;

	self->set_value_hook = set_value_hook;

	rtb_stylequad_init(&self->rotor);

	return 0;
}

void
rtb_knob_fini(struct rtb_knob *self)
{
	rtb_stylequad_fini(&self->rotor);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_knob *
rtb_knob_new()
{
	struct rtb_knob *self = calloc(1, sizeof(struct rtb_knob));
	rtb_knob_init(self);
	return self;
}

void
rtb_knob_free(struct rtb_knob *self)
{
	rtb_knob_fini(self);
	free(self);
}
