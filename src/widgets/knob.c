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
#include "rutabaga/object.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/event.h"
#include "rutabaga/keyboard.h"

#include "rutabaga/widgets/knob.h"

#include "private/util.h"

#define SELF_FROM(obj) \
	struct rtb_knob *self = RTB_OBJECT_AS(obj, rtb_knob)

#define MIN_DEGREES 35.f
#define MAX_DEGREES (360.f - MIN_DEGREES)
#define DEGREE_RANGE (MAX_DEGREES - MIN_DEGREES)

#define CIRCLE_SEGMENTS 32

/* would be cool to have this be like a smoothed equation or smth */
#define DELTA_VALUE_STEP_BUTTON1 .005f
#define DELTA_VALUE_STEP_BUTTON3 .0005f

struct rtb_object_implementation super;

static const GLubyte indicator_indices[] = {
	0, 1
};

static GLushort circle_indices[CIRCLE_SEGMENTS + 2] = {[1] = 0};

/**
 * drawing
 */

static void circle_vertex_array(GLfloat dest[][2], float r, int segments)
{
	/* borrowed from http://slabode.exofire.net/circle_draw.shtml, thanks! */

	float theta = 2 * 3.1415926f / (float) segments;
	float c = cosf(theta); /* precalculate the sine and cosine */
	float s = sinf(theta);
	float t;

	float x = r; /* we start at angle = 0 */
	float y = 0;

	int i;

	for (i = 0; i < segments; i++) {
		dest[i][0] = x;
		dest[i][1] = y;

		/* apply the rotation matrix */
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
}

static void cache_to_vbo(struct rtb_knob *self)
{
	GLfloat w, h, indicator[2][2], circle[CIRCLE_SEGMENTS + 1][2];

	w = self->w;
	h = self->h;

	indicator[0][0] = 0;
	indicator[0][1] = (h / 7.f);

	indicator[1][0] = 0;
	indicator[1][1] = (h / 2.f);

	circle[0][0] = 0.f;
	circle[1][0] = 0.f;

	circle_vertex_array(&circle[1], (w / 2.f) - 1.f, ARRAY_LENGTH(circle) - 1);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(indicator), indicator, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw(struct rtb_object *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_render_push(obj);
	rtb_render_clear(obj);
	rtb_render_set_position(obj,
			self->x + ((self->w) / 2),
			self->y + ((self->h) / 2));

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	/* background */
	rtb_render_use_style_bg(obj, state);

	glDrawElements(
			GL_TRIANGLE_FAN, ARRAY_LENGTH(circle_indices),
			GL_UNSIGNED_SHORT, circle_indices);

	/* indicator */
	rtb_render_use_style_fg(obj, state);

	glLineWidth(2.5f);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[1]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	rtb_render_set_modelview(obj, self->modelview.data);

	glDrawElements(
			GL_LINES, ARRAY_LENGTH(indicator_indices),
			GL_UNSIGNED_BYTE, indicator_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	super.draw_cb(obj, state);
}

/**
 * event dispatching
 */

static int dispatch_value_change_event(struct rtb_knob *self)
{
	struct rtb_knob_event event = {
		.type = KNOB_VALUE_CHANGE,
		.value = self->value * (self->max - self->min),
	};

	return rtb_handle(RTB_OBJECT(self), RTB_EVENT(&event));
}

/**
 * event handling
 */

static int handle_drag(struct rtb_knob *self, const struct rtb_drag_event *e)
{
	float new_value;

	if (e->target != RTB_OBJECT(self))
		return 0;

	new_value = self->value;

	switch (e->button) {
	case RTB_MOUSE_BUTTON1: new_value += -e->delta.y * DELTA_VALUE_STEP_BUTTON1; break;
	case RTB_MOUSE_BUTTON3: new_value += -e->delta.y * DELTA_VALUE_STEP_BUTTON3; break;
	default: return 1;
	}

	rtb_knob_set_value(self, new_value);
	return 1;
}

static int handle_mouse_down(struct rtb_knob *self,
		const struct rtb_mouse_event *e)
{
	switch (e->button) {
	case RTB_MOUSE_BUTTON2:
		rtb_knob_set_value(self, self->origin);
		dispatch_value_change_event(self);
		return 1;

	case RTB_MOUSE_BUTTON1:
		return 1;

	default:
		return 0;
	}
}

static int handle_key(struct rtb_knob *self, const struct rtb_key_event *e)
{
	float step;

	if (e->modkeys & RTB_KEY_MOD_ALT)
		step = DELTA_VALUE_STEP_BUTTON3;
	else if (e->modkeys & RTB_KEY_MOD_SHIFT)
		step = DELTA_VALUE_STEP_BUTTON1 * 2;
	else
		step = DELTA_VALUE_STEP_BUTTON1;

	switch (e->keysym) {
	case RTB_KEY_UP:
	case RTB_KEY_NUMPAD_UP:
		rtb_knob_set_value(self, self->value + step);
		return 1;

	case RTB_KEY_DOWN:
	case RTB_KEY_NUMPAD_DOWN:
		rtb_knob_set_value(self, self->value - step);
		return 1;

	default:
		return 0;
	}
}

static int on_event(struct rtb_object *obj, const struct rtb_event *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAGGING:
		return handle_drag(self, RTB_EVENT_AS(e, rtb_drag_event));

	case RTB_MOUSE_DOWN:
		return handle_mouse_down(self, RTB_EVENT_AS(e, rtb_mouse_event));

	case RTB_KEY_PRESS:
		return handle_key(self, RTB_EVENT_AS(e, rtb_key_event));

	default:
		return super.event_cb(obj, e);
	}

	return 0;
}

static void realize(struct rtb_object *obj,
		struct rtb_object *parent, struct rtb_window *window)
{
	SELF_FROM(obj);
	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.knob");

	rtb_knob_set_value(self, self->origin);
	cache_to_vbo(self);
}

static void init_circle_indices()
{
	int i;

	if (circle_indices[1] != 0)
		return;

	for (i = 0; i < ARRAY_LENGTH(circle_indices); i++)
		circle_indices[i] = i;

	circle_indices[i - 1] = 1;
}

/**
 * public API
 */

void rtb_knob_set_value(struct rtb_knob *self, float new_value)
{
	self->value = fmin(fmax(new_value, 0.f), 1.f);

	mat4_set_rotation(&self->modelview,
			MIN_DEGREES + (self->value * DEGREE_RANGE),
			0.f, 0.f, 1.f);

	rtb_obj_mark_dirty(RTB_OBJECT(self));

	if (self->state != RTB_STATE_UNREALIZED)
		dispatch_value_change_event(self);
}

struct rtb_knob *rtb_knob_new()
{
	struct rtb_knob *self = calloc(1, sizeof(struct rtb_knob));
	rtb_obj_init(RTB_OBJECT(self), &super);

	self->origin = 0.f;
	self->min = 0.f;
	self->max = 1.f;

	self->w = 30;
	self->h = 30;

	glGenBuffers(2, self->vbo);

	self->draw_cb    = draw;
	self->event_cb   = on_event;
	self->realize_cb = realize;

	init_circle_indices();
	return self;
}

void rtb_knob_free(struct rtb_knob *self)
{
	glDeleteBuffers(2, self->vbo);
	rtb_obj_fini(RTB_OBJECT(self));
	free(self);
}
