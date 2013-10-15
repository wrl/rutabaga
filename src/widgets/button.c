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
#include <string.h>

#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"
#include "rutabaga/quad.h"

#include "private/util.h"
#include "rutabaga/widgets/button.h"

#define SELF_FROM(elem) \
	struct rtb_button *self = RTB_ELEMENT_AS(elem, rtb_button)

static struct rtb_element_implementation super;

/**
 * drawing-related things
 */

static void
draw(struct rtb_element *elem, rtb_draw_state_t state)
{
	SELF_FROM(elem);

	rtb_render_push(elem);
	rtb_render_clear(elem);
	rtb_render_set_position(elem, 0, 0);
	rtb_render_use_style_bg(elem, state);

	rtb_render_quad(elem, &self->bg_quad);

	rtb_elem_draw_children(elem, state);
	rtb_render_pop(elem);
}

/**
 * event handlers
 */

static int
dispatch_click_event(struct rtb_button *self, const struct rtb_mouse_event *e)
{
	struct rtb_button_event event = *((struct rtb_button_event *) e);

	event.type = RTB_BUTTON_CLICK;
	event.cursor.x -= self->x;
	event.cursor.y -= self->y;

	return rtb_handle(RTB_ELEMENT(self), RTB_EVENT(&event));
}

static int
handle_key_press(struct rtb_button *self, const struct rtb_key_event *e)
{
	struct rtb_button_event event = {
		.type   = RTB_BUTTON_CLICK,
		.source = RTB_EVENT_SYNTHETIC
	};

	if ((e->keysym == RTB_KEY_NORMAL && e->character == ' ')
			|| (e->keysym == RTB_KEY_ENTER)) {
		rtb_handle(RTB_ELEMENT(self), RTB_EVENT(&event));
		return 1;
	}

	return 0;
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_MOUSE_DOWN:
	case RTB_DRAG_START:
		return 1;

	case RTB_KEY_PRESS:
		if (handle_key_press(self, RTB_EVENT_AS(e, rtb_key_event)))
			return 1;
		break;

	case RTB_MOUSE_CLICK:
		if (((struct rtb_mouse_event *) e)->button != RTB_MOUSE_BUTTON1)
			return 0;

		return dispatch_click_event(self, RTB_EVENT_AS(e, rtb_mouse_event));

	default:
		return super.on_event(elem, e);
	}

	return 0;
}

static int
reflow(struct rtb_element *elem, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	SELF_FROM(elem);

	super.reflow(elem, instigator, direction);

	self->outer_pad.x = self->label.outer_pad.x;
	self->outer_pad.y = self->label.outer_pad.y;

	rtb_quad_set_vertices(&self->bg_quad, &self->rect);

	return 1;
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.button");

	self->outer_pad.x = self->label.outer_pad.x;
	self->outer_pad.y = self->label.outer_pad.y;
}

/**
 * public API
 */

void
rtb_button_set_label(struct rtb_button *self, const rtb_utf8_t *text)
{
	rtb_label_set_text(&self->label, text);
}

int
rtb_button_init(struct rtb_button *self,
		struct rtb_element_implementation *impl)
{
	rtb_elem_init(RTB_ELEMENT(self), &super);

	rtb_label_init(&self->label, &self->label.impl);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->label),
			RTB_ADD_HEAD);

	rtb_quad_init(&self->bg_quad);

	self->label.align = RTB_ALIGN_MIDDLE;
	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->min_size.w = 70.f;
	self->min_size.h = 26.f;

	self->draw      = draw;
	self->on_event  = on_event;
	self->attached  = attached;
	self->layout_cb = rtb_layout_hpack_center;
	self->size_cb   = rtb_size_hfit_children;
	self->reflow    = reflow;

	return 0;
}

void
rtb_button_fini(struct rtb_button *self)
{
	rtb_quad_fini(&self->bg_quad);
	rtb_label_fini(&self->label);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_button *
rtb_button_new(const rtb_utf8_t *label)
{
	struct rtb_button *self = calloc(1, sizeof(*self));
	rtb_button_init(self, &self->impl);

	if (label)
		rtb_button_set_label(self, label);

	return self;
}

void
rtb_button_free(struct rtb_button *self)
{
	rtb_button_fini(self);
	free(self);
}
