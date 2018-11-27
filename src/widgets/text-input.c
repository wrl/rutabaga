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

#include <string.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/render.h>
#include <rutabaga/window.h>
#include <rutabaga/keyboard.h>
#include <rutabaga/layout.h>
#include <rutabaga/layout-helpers.h>
#include <rutabaga/geometry.h>

#include <rutabaga/widgets/text-input.h>

#include "rtb_private/stdlib-allocator.h"
#include "rtb_private/util.h"
#include "rtb_private/utf8.h"

#define SELF_FROM(elem) \
	struct rtb_text_input *self = RTB_ELEMENT_AS(elem, rtb_text_input)

#define UTF8_IS_CONTINUATION(byte) (((byte) & 0xC0) == 0x80)

static struct rtb_element_implementation super;

/**
 * vbo wrangling
 */

static void
update_cursor(struct rtb_text_input *self)
{
	GLfloat x, y, h, line[2][2];
	struct rtb_rect glyphs[2];

	if (self->cursor_position > 0) {
		rtb_text_object_get_glyph_rect(self->label.tobj,
				self->cursor_position, &glyphs[0]);

		/* if the cursor isn't at the end of the entered text,
		 * we position it halfway between the character it's after
		 * and the one it's before */

		if (rtb_text_object_get_glyph_rect(self->label.tobj,
					self->cursor_position + 1, &glyphs[1]))
			x = self->label.tobj->w;
		else
			x = glyphs[1].x;
	} else
		x = 0.f;

	x += self->label.x;

	if (self->label_offset < 0 &&
			self->label.x2 < self->inner_rect.x2) {
		self->label_offset += self->inner_rect.x2 - self->label.x2;
		self->label_offset = MIN(self->label_offset, 0);

		rtb_elem_trigger_reflow(RTB_ELEMENT(self), RTB_ELEMENT(self),
				RTB_DIRECTION_LEAFWARD);
		return;
	}

	/* if the cursor has wandered outside our bounding box, move the label
	 * so that the cursor is inside it again. */
	if (x < self->inner_rect.x || x > self->inner_rect.x2) {
		if (x < self->inner_rect.x)
			self->label_offset += self->inner_rect.x - x;
		else
			self->label_offset += self->inner_rect.x2 - x;

		rtb_elem_trigger_reflow(RTB_ELEMENT(self), RTB_ELEMENT(self),
				RTB_DIRECTION_LEAFWARD);
		return;
	}

	y  = self->label.y;
	h  = self->label.h;

	line[0][0] = x;
	line[0][1] = y;

	line[1][0] = x;
	line[1][1] = y + h;

	glBindBuffer(GL_ARRAY_BUFFER, self->cursor_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * drawing
 */

static void
draw_cursor(struct rtb_text_input *self)
{
	struct rtb_render_context *ctx;

	rtb_render_reset(RTB_ELEMENT(self));
	ctx = rtb_render_get_context(RTB_ELEMENT(self));
	rtb_render_set_position(ctx, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, self->cursor_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glLineWidth(1.f);

	rtb_render_set_color(ctx, 1.f, 1.f, 1.f, 1.f);

	glDrawArrays(GL_LINES, 0, 2);
}

static void
draw(struct rtb_element *elem)
{
	SELF_FROM(elem);

	super.draw(elem);

	if (self->window->focus == RTB_ELEMENT(self))
		draw_cursor(self);
}

/**
 * text buffer
 */

static void
push_u32(struct rtb_text_input *self, rtb_utf32_t c)
{
	rtb_text_buffer_insert_u32(&self->text, self->cursor_position, c);
	self->cursor_position++;
}

static int
pop_u32(struct rtb_text_input *self)
{
	if (rtb_text_buffer_erase_char(&self->text, self->cursor_position))
		return -1;

	self->cursor_position--;
	return 0;
}

static int
delete_u32(struct rtb_text_input *self)
{
	if (rtb_text_buffer_erase_char(&self->text, self->cursor_position + 1))
		return -1;

	return 0;
}

/**
 * element implementation
 */

static void
fix_cursor(struct rtb_text_input *self)
{
	struct rtb_rect glyph;

	if (self->cursor_position < 0)
		self->cursor_position = 0;
	else if (self->cursor_position > 0 &&
			rtb_text_object_get_glyph_rect(self->label.tobj,
				self->cursor_position, &glyph))
		self->cursor_position =
			rtb_text_object_count_glyphs(self->label.tobj);

	update_cursor(self);
	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

static void
post_change(struct rtb_text_input *self)
{
	rtb_label_set_text(&self->label,
			rtb_text_buffer_get_text(&self->text));
}

static int
handle_key_press(struct rtb_text_input *self, const struct rtb_key_event *e)
{
	switch (e->keysym) {
	case RTB_KEY_NORMAL:
		if (e->mod_keys & ~RTB_KEY_MOD_SHIFT)
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

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_MOUSE_CLICK:
	case RTB_MOUSE_DOWN:
		return 1;

	case RTB_KEY_PRESS:
		if (handle_key_press(self, RTB_EVENT_AS(e, rtb_key_event)))
			return 1;
		break;

	case RTB_FOCUS:
	case RTB_UNFOCUS:
		rtb_elem_mark_dirty(elem);
		break;

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

	if (!super.reflow(elem, instigator, direction))
		return 0;

	self->outer_pad.y = self->label.outer_pad.y;

	rtb_quad_set_vertices(&self->bg_quad, &self->rect);
	update_cursor(self);

	return 1;
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.text-input");
}

static void
restyle(struct rtb_element *elem)
{
	SELF_FROM(elem);
	super.restyle(elem);

	self->outer_pad.x = 5.f;
	self->outer_pad.y = self->label.outer_pad.y;
}

static void
layout(struct rtb_element *elem)
{
	SELF_FROM(elem);

	struct rtb_size avail, child;
	struct rtb_point position;
	struct rtb_element *iter;
	float ystart;

	avail.w = elem->w - (elem->outer_pad.x * 2);
	avail.h = elem->h - (elem->outer_pad.y * 2);

	position.x = elem->x + elem->outer_pad.x + self->label_offset;
	ystart = elem->y + elem->outer_pad.y;

	TAILQ_FOREACH(iter, &elem->children, child) {
		rtb_elem_request_size(iter, &avail, &child);
		position.y = ystart + valign(avail.h, child.h, iter->align);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		avail.w    -= child.w + elem->inner_pad.x;
		position.x += child.w + elem->inner_pad.x;
	}
}

/**
 * public API
 */

int
rtb_text_input_set_text(struct rtb_text_input *self,
		rtb_utf8_t *text, ssize_t nbytes)
{
	rtb_text_buffer_set_text(&self->text, text, nbytes);
	self->cursor_position = u8chars(text);

	post_change(self);

	return 0;
}

const rtb_utf8_t *
rtb_text_input_get_text(struct rtb_text_input *self)
{
	return rtb_text_buffer_get_text(&self->text);
}

int
rtb_text_input_init(struct rutabaga *rtb, struct rtb_text_input *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	rtb_quad_init(&self->bg_quad);

	rtb_label_init(&self->label);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->label),
			RTB_ADD_HEAD);

	rtb_text_buffer_init(rtb, &self->text);

	glGenBuffers(1, &self->cursor_vbo);

	self->label.align = RTB_ALIGN_MIDDLE;
	self->label_offset = 0;

	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->draw      = draw;
	self->on_event  = on_event;
	self->attached  = attached;
	self->reflow    = reflow;
	self->restyle   = restyle;
	self->size_cb   = rtb_size_self;
	self->layout_cb = layout;

	self->cursor_position = 0;
	rtb_label_set_text(&self->label, "");

	self->flags = RTB_ELEM_CLICK_FOCUS | RTB_ELEM_TAB_FOCUS;

	return 0;
}

void
rtb_text_input_fini(struct rtb_text_input *self)
{
	rtb_text_buffer_fini(&self->text);

	rtb_quad_fini(&self->bg_quad);
	glDeleteBuffers(1, &self->cursor_vbo);

	rtb_label_fini(&self->label);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_text_input *
rtb_text_input_new(struct rutabaga *rtb)
{
	struct rtb_text_input *self = calloc(1, sizeof(*self));
	rtb_text_input_init(rtb, self);

	return self;
}

void
rtb_text_input_free(struct rtb_text_input *self)
{
	rtb_text_input_fini(self);
	free(self);
}
