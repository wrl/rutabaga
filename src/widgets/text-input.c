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

#include <string.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/render.h"
#include "rutabaga/keyboard.h"
#include "rutabaga/layout.h"

#include "private/util.h"
#include "private/utf8.h"
#include "rutabaga/widgets/text-input.h"

#define SELF_FROM(obj) \
	struct rtb_text_input *self = (void *) obj

static struct rtb_object_implementation super;

static const GLubyte box_indices[] = {
	0, 1, 2, 3
};

static void cache_to_vbo(rtb_text_input_t *self)
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
	rtb_render_set_position(obj, 0, 0);

	rtb_render_use_style_bg(obj, state);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glLineWidth(2.f);

	glDrawElements(
			GL_LINE_LOOP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	super.draw_cb(obj, state);
}

/**
 * object implementation
 */

static int handle_key_press(rtb_text_input_t *self, const rtb_ev_key_t *e)
{
	rtb_utf8_t utf[6];
	char null = '\0';
	int len;

	switch (e->keysym) {
	case RTB_KEY_NORMAL:
		if (e->modkeys & ~RTB_KEY_MOD_SHIFT)
			return 0;

		len = u8enc(e->character, utf);
		vector_pop_back(self->entered); /* pop the terminating NULL */
		vector_push_back_data(self->entered, utf, len);
		vector_push_back(self->entered, &null); /* and push it back on */
		break;

	case RTB_KEY_BACKSPACE:
		if (!(vector_size(self->entered) > 1))
			return 1;

		vector_erase(self->entered, vector_size(self->entered) - 2);
		break;

	default:
		return 0;
	}

	rtb_label_set_text(&self->label, rtb_text_input_get_text(self));
	return 1;
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_MOUSE_CLICK:
	case RTB_MOUSE_DOWN:
		return 1;

	case RTB_KEY_PRESS:
		if (handle_key_press(self, (rtb_ev_key_t *) e))
			return 1;
		break;

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

	self->outer_pad.y = self->label.outer_pad.y;
	cache_to_vbo(self);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.text-input");

	self->outer_pad.x = 5.f;
	self->outer_pad.y = self->label.outer_pad.y;
}

/**
 * public API
 */

int rtb_text_input_set_text(rtb_text_input_t *self,
		rtb_utf8_t *text, ssize_t nbytes)
{
	char null = '\0';

	if (nbytes < 0)
		nbytes = strlen(text);

	vector_clear(self->entered);
	vector_push_back_data(self->entered, text, nbytes);
	vector_push_back(self->entered, &null);

	printf(" :: setting to \"%s\" (%ld)\n", text, nbytes);

	rtb_label_set_text(&self->label, rtb_text_input_get_text(self));

	return 0;
}

const rtb_utf8_t *rtb_text_input_get_text(rtb_text_input_t *self)
{
	return vector_front(self->entered);
}

int rtb_text_input_init(rtb_text_input_t *self,
		struct rtb_object_implementation *impl)
{
	char null = '\0';

	rtb_obj_init(RTB_OBJECT(self), &super);

	rtb_label_init(&self->label, &self->label.impl);
	rtb_obj_add_child(RTB_OBJECT(self), RTB_OBJECT(&self->label),
			RTB_ADD_HEAD);

	self->entered = vector_new(sizeof(rtb_utf8_t));
	vector_push_back(self->entered, &null);

	glGenBuffers(1, &self->vbo);

	self->label.align = RTB_ALIGN_MIDDLE;

	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->min_size.w = 120.f;

	self->realize_cb = realize;
	self->recalc_cb  = recalculate;
	self->draw_cb    = draw;
	self->size_cb    = rtb_size_hfit_children;
	self->layout_cb  = rtb_layout_hpack_left;
	self->event_cb   = on_event;

	rtb_label_set_text(&self->label, "");

	return 0;
}

void rtb_text_input_fini(rtb_text_input_t *self)
{
	vector_delete(self->entered);
	rtb_label_fini(&self->label);
	rtb_obj_fini(RTB_OBJECT(self));
}

rtb_text_input_t *rtb_text_input_new(void)
{
	rtb_text_input_t *self = calloc(1, sizeof(*self));
	rtb_text_input_init(self, &self->impl);

	return self;
}

void rtb_text_input_free(rtb_text_input_t *self)
{
	rtb_text_input_fini(self);
	free(self);
}
