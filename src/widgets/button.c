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

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"

#include "rtb_private/util.h"
#include "rutabaga/widgets/button.h"

#define SELF_FROM(elem) \
	struct rtb_button *self = RTB_ELEMENT_AS(elem, rtb_button)

static struct rtb_element_implementation super;

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
		if (RTB_EVENT_AS(e, rtb_mouse_event)->button != RTB_MOUSE_BUTTON1)
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
rtb_button_init(struct rtb_button *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	rtb_label_init(&self->label);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->label),
			RTB_ADD_HEAD);

	self->label.align = RTB_ALIGN_MIDDLE;
	self->outer_pad.x =
		self->outer_pad.y = 0.f;

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
	rtb_label_fini(&self->label);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_button *
rtb_button_new(const rtb_utf8_t *label)
{
	struct rtb_button *self = calloc(1, sizeof(*self));
	rtb_button_init(self);

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
