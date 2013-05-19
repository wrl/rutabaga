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
#include "rutabaga/window.h"
#include "rutabaga/keyboard.h"
#include "rutabaga/layout.h"

#include "rutabaga/widgets/text-input.h"

#include "private/stdlib-allocator.h"
#include "private/util.h"
#include "private/utf8.h"

#define SELF_FROM(obj) \
	struct rtb_text_input *self = RTB_OBJECT_AS(obj, rtb_text_input)

#define UTF8_IS_CONTINUATION(byte) (((byte) & 0xC0) == 0x80)

static struct rtb_object_implementation super;

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static const GLubyte line_indices[] = {
	0, 1, 2, 3
};

/**
 * vbo wrangling
 */

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

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void update_cursor(rtb_text_input_t *self)
{
	GLfloat x, y, h, line[2][2];
	rtb_rect_t glyphs[2];

	if (self->cursor_position > 0) {
		rtb_text_object_get_glyph_rect(self->label.tobj,
				self->cursor_position, &glyphs[0]);

		/* if the cursor isn't at the end of the entered text,
		 * we position it halfway between the character it's after
		 * and the one it's before */

		if (rtb_text_object_get_glyph_rect(self->label.tobj,
					self->cursor_position + 1, &glyphs[1]))
			x = glyphs[0].p2.x;
		else
			x = glyphs[1].p1.x;
	} else
		x = 0.f;

	x += self->label.x;
	y  = self->label.y;
	h  = self->label.h;

	line[0][0] = x;
	line[0][1] = y;

	line[1][0] = x;
	line[1][1] = y + h;

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * drawing
 */

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	/* XXX: hack to get the outline to render in front of the
	 *      label. need to fix the render push/pop system. */

	rtb_render_push(obj);
	rtb_render_clear(obj);

	super.draw_cb(obj, state);

	rtb_render_reset(obj);
	rtb_render_set_position(obj, 0, 0);

	if (self->window->focus == RTB_OBJECT(self)) {
		glBindBuffer(GL_ARRAY_BUFFER, self->vbo[1]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glLineWidth(1.f);

		rtb_render_set_color(obj, 1.f, 1.f, 1.f, 1.f);

		glDrawArrays(GL_LINES, 0, 2);
	}

	rtb_render_use_style_bg(obj, state);
	glBindBuffer(GL_ARRAY_BUFFER, self->vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glLineWidth(2.f);

	glDrawElements(
			GL_LINE_LOOP, ARRAY_LENGTH(line_indices),
			GL_UNSIGNED_BYTE, line_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	rtb_render_pop(obj);
}

/**
 * text buffer
 */

static void push_u32(rtb_text_input_t *self, rtb_utf32_t c)
{
	rtb_text_buffer_insert_u32(&self->text, self->cursor_position, c);
	self->cursor_position++;
}

static int pop_u32(rtb_text_input_t *self)
{
	if (rtb_text_buffer_erase_char(&self->text, self->cursor_position))
		return -1;

	self->cursor_position--;
	return 0;
}

static int delete_u32(rtb_text_input_t *self)
{
	if (rtb_text_buffer_erase_char(&self->text, self->cursor_position + 1))
		return -1;

	return 0;
}

/**
 * object implementation
 */

static void fix_cursor(rtb_text_input_t *self)
{
	rtb_rect_t glyph;

	if (self->cursor_position < 0)
		self->cursor_position = 0;
	else if (self->cursor_position > 0 &&
			rtb_text_object_get_glyph_rect(self->label.tobj,
				self->cursor_position, &glyph))
		self->cursor_position =
			rtb_text_object_count_glyphs(self->label.tobj);

	update_cursor(self);
	rtb_obj_mark_dirty(RTB_OBJECT(self));
}

static void post_change(rtb_text_input_t *self)
{
	rtb_label_set_text(&self->label,
			rtb_text_buffer_get_text(&self->text));
}

static int handle_key_press(rtb_text_input_t *self, const rtb_ev_key_t *e)
{
	switch (e->keysym) {
	case RTB_KEY_NORMAL:
		if (e->modkeys & ~RTB_KEY_MOD_SHIFT)
			return 0;

		push_u32(self, e->character);
		post_change(self);
		break;

	case RTB_KEY_BACKSPACE:
		pop_u32(self);
		post_change(self);
		break;

	case RTB_KEY_DELETE:
	case RTB_KEY_NUMPAD_DELETE:
		delete_u32(self);
		post_change(self);
		break;

	case RTB_KEY_HOME:
	case RTB_KEY_NUMPAD_HOME:
		self->cursor_position = 0;
		fix_cursor(self);
		break;

	case RTB_KEY_END:
	case RTB_KEY_NUMPAD_END:
		self->cursor_position = INT_MAX;
		fix_cursor(self);
		break;

	case RTB_KEY_LEFT:
	case RTB_KEY_NUMPAD_LEFT:
		self->cursor_position--;
		fix_cursor(self);
		break;

	case RTB_KEY_RIGHT:
	case RTB_KEY_NUMPAD_RIGHT:
		self->cursor_position++;
		fix_cursor(self);
		break;

	default:
		return 0;
	}

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

	update_cursor(self);
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
	rtb_text_buffer_set_text(&self->text, text, nbytes);
	self->cursor_position = u8chars(text);

	post_change(self);

	return 0;
}

const rtb_utf8_t *rtb_text_input_get_text(rtb_text_input_t *self)
{
	return rtb_text_buffer_get_text(&self->text);
}

int rtb_text_input_init(rtb_t *rtb, rtb_text_input_t *self,
		struct rtb_object_implementation *impl)
{
	rtb_obj_init(RTB_OBJECT(self), &super);

	rtb_label_init(&self->label, &self->label.impl);
	rtb_obj_add_child(RTB_OBJECT(self), RTB_OBJECT(&self->label),
			RTB_ADD_HEAD);

	rtb_text_buffer_init(rtb, &self->text);

	glGenBuffers(2, self->vbo);

	self->label.align = RTB_ALIGN_MIDDLE;

	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->min_size.h = 30.f;
	self->min_size.w = 150.f;

	self->realize_cb = realize;
	self->recalc_cb  = recalculate;
	self->draw_cb    = draw;
	self->size_cb    = rtb_size_self;
	self->layout_cb  = rtb_layout_hpack_left;
	self->event_cb   = on_event;

	self->cursor_position = 0;
	rtb_label_set_text(&self->label, "");

	return 0;
}

void rtb_text_input_fini(rtb_text_input_t *self)
{
	rtb_text_buffer_fini(&self->text);
	rtb_label_fini(&self->label);
	rtb_obj_fini(RTB_OBJECT(self));
}

rtb_text_input_t *rtb_text_input_new(rtb_t *rtb)
{
	rtb_text_input_t *self = calloc(1, sizeof(*self));
	rtb_text_input_init(rtb, self, &self->impl);

	return self;
}

void rtb_text_input_free(rtb_text_input_t *self)
{
	rtb_text_input_fini(self);
	free(self);
}
