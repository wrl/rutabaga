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
#include "rutabaga/object.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"

#include "private/util.h"
#include "rutabaga/widgets/button.h"

#define SELF_FROM(obj) \
	struct rtb_button *self = RTB_OBJECT_AS(obj, rtb_button)

static struct rtb_object_implementation super;

/**
 * drawing-related things
 */

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static void cache_to_vbo(rtb_button_t *self)
{
	GLfloat x, y, w, h, box[4][2];

	x = self->x;
	y = self->y;
	w = self->w;
	h = self->h;

	box[0][0] = x;
	box[0][1] = y;

	box[1][0] = x + w;
	box[1][1] = y;

	box[2][0] = x + w;
	box[2][1] = y + h;

	box[3][0] = x;
	box[3][1] = y + h;

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_render_push(obj);
	rtb_render_clear(obj);
	rtb_render_set_position(obj, 0, 0);

	rtb_render_use_style_bg(obj, state);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	super.draw_cb(obj, state);
	rtb_render_pop(obj);
}

/**
 * event handlers
 */

static int dispatch_click_event(rtb_button_t *self, const rtb_ev_mouse_t *e)
{
	rtb_ev_button_t event = *((rtb_ev_button_t *) e);

	event.type = BUTTON_CLICK;
	event.cursor.x -= self->rect.p1.x;
	event.cursor.y -= self->rect.p1.y;

	return rtb_handle(RTB_OBJECT(self), RTB_EVENT(&event));
}

static int handle_key_press(rtb_button_t *self, const rtb_ev_key_t *e)
{
	rtb_ev_button_t event = {
		.type   = BUTTON_CLICK,
		.source = RTB_EVENT_SYNTHETIC
	};

	if ((e->keysym == RTB_KEY_NORMAL && e->character == ' ')
			|| (e->keysym == RTB_KEY_ENTER)) {
		rtb_handle(RTB_OBJECT(self), RTB_EVENT(&event));
		return 1;
	}

	return 0;
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_MOUSE_DOWN:
	case RTB_DRAG_START:
		return 1;

	case RTB_KEY_PRESS:
		if (handle_key_press(self, (rtb_ev_key_t *) e))
			return 1;
		break;

	case RTB_MOUSE_CLICK:
		if (((rtb_ev_mouse_t *) e)->button != RTB_MOUSE_BUTTON1)
			return 0;

		return dispatch_click_event(self, (rtb_ev_mouse_t *) e);

	default:
		return super.event_cb(obj, e);
	}

	return 0;
}

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	SELF_FROM(obj);

	super.recalc_cb(obj, instigator, direction);

	self->outer_pad.x = self->label.outer_pad.x;
	self->outer_pad.y = self->label.outer_pad.y;

	cache_to_vbo(self);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.button");

	self->outer_pad.x = self->label.outer_pad.x;
	self->outer_pad.y = self->label.outer_pad.y;
}

/**
 * public API
 */

void rtb_button_set_label(rtb_button_t *self, const rtb_utf8_t *text)
{
	rtb_label_set_text(&self->label, text);
}

int rtb_button_init(rtb_button_t *self,
		struct rtb_object_implementation *impl)
{
	rtb_obj_init(RTB_OBJECT(self), &super);

	rtb_label_init(&self->label, &self->label.impl);
	rtb_obj_add_child(RTB_OBJECT(self), RTB_OBJECT(&self->label),
			RTB_ADD_HEAD);

	glGenBuffers(1, &self->vbo);

	self->label.align = RTB_ALIGN_MIDDLE;
	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->min_size.w = 70.f;
	self->min_size.h = 26.f;

	self->draw_cb    = draw;
	self->event_cb   = on_event;
	self->realize_cb = realize;
	self->layout_cb  = rtb_layout_hpack_center;
	self->size_cb    = rtb_size_hfit_children;
	self->recalc_cb  = recalculate;

	return 0;
}

void rtb_button_fini(rtb_button_t *self)
{
	rtb_label_fini(&self->label);
	rtb_obj_fini(RTB_OBJECT(self));
}

rtb_button_t *rtb_button_new(const rtb_utf8_t *label)
{
	rtb_button_t *self = calloc(1, sizeof(*self));
	rtb_button_init(self, &self->impl);

	if (label)
		rtb_button_set_label(self, label);

	return self;
}

void rtb_button_free(rtb_button_t *self)
{
	rtb_button_fini(self);
	free(self);
}
