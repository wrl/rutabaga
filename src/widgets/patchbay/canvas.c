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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "bsd/queue.h"

#include "rutabaga/rutabaga.h"
#include "rutabaga/surface.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/shader.h"
#include "rutabaga/layout.h"
#include "rutabaga/event.h"
#include "rutabaga/mouse.h"
#include "rutabaga/style.h"
#include "rutabaga/asset.h"

#include "rutabaga/widgets/patchbay.h"

#include "private/util.h"

#include "shaders/patchbay-canvas.glsl.h"

#define SELF_FROM(elem) \
	struct rtb_patchbay *self = RTB_ELEMENT_AS(elem, rtb_patchbay)

#define CONNECTION_COLOR	RGB(0x404F3C)
#define DISCONNECT_COLOR	RGB(0x69181B)

static struct rtb_element_implementation super;

/**
 * custom openGL stuff
 */

static struct {
	RTB_INHERIT(rtb_shader);

	struct {
		GLint texture;
		GLint tx_size;
		GLint tx_offset;

		GLint front_color;
		GLint back_color;
	} uniform;
} shader = {
	.program = 0
};

static void
init_shaders()
{
	if (shader.program)
		return;

	if (!rtb_shader_create(RTB_SHADER(&shader),
				PATCHBAY_CANVAS_VERT_SHADER,
				PATCHBAY_CANVAS_FRAG_SHADER))
		puts("rtb_patchbay: init_shaders() failed!");

#define CACHE_UNIFORM_LOCATION(name) \
	shader.uniform.name = glGetUniformLocation(shader.program, #name)
	CACHE_UNIFORM_LOCATION(texture);
	CACHE_UNIFORM_LOCATION(tx_size);
	CACHE_UNIFORM_LOCATION(tx_offset);

	CACHE_UNIFORM_LOCATION(front_color);
	CACHE_UNIFORM_LOCATION(back_color);
#undef CACHE_UNIFORM_LOCATION
}

static void
load_tile(struct rtb_style_texture_definition *definition,
		GLuint into_texture)
{
	if (!RTB_ASSET_IS_LOADED(RTB_ASSET(definition))) {
		printf(" [!] couldn't load tile, aiee!\n");
		return;
	}

	glBindTexture(GL_TEXTURE_2D, into_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			definition->w, definition->h,
			0, GL_BGRA, GL_UNSIGNED_BYTE,
			RTB_ASSET_DATA(RTB_ASSET(definition)));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * drawing
 */

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static void
cache_to_vbo(struct rtb_patchbay *self)
{
	GLfloat x, y, w, h, box[4][2];

	x = self->x,
	y = self->y,
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

	glBindBuffer(GL_ARRAY_BUFFER, self->bg_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void
draw_bg(struct rtb_patchbay *self)
{
	const struct rtb_style_property_definition *prop;
	struct rtb_element *elem = RTB_ELEMENT(self);

	rtb_render_push(elem);
	rtb_render_use_shader(elem, RTB_SHADER(&shader));
	rtb_render_set_position(elem, 0, 0);

	/* draw the background */
	glBindBuffer(GL_ARRAY_BUFFER, self->bg_vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	prop = rtb_style_query_prop(self->style, RTB_DRAW_NORMAL,
			"background-image", RTB_STYLE_PROP_TEXTURE);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, self->bg_texture);
	glUniform1i(shader.uniform.texture, 0);
	glUniform2f(shader.uniform.tx_size, prop->texture.w, prop->texture.h);
	glUniform2f(shader.uniform.tx_offset,
			roundf(self->texture_offset.x),
			roundf(self->texture_offset.y));

	prop = rtb_style_query_prop(self->style, RTB_DRAW_NORMAL,
			"color", RTB_STYLE_PROP_COLOR);

	glUniform4f(shader.uniform.front_color,
			prop->color.r,
			prop->color.g,
			prop->color.b,
			prop->color.a);

	prop = rtb_style_query_prop(self->style, RTB_DRAW_NORMAL,
			"background-color", RTB_STYLE_PROP_COLOR);

	glUniform4f(shader.uniform.back_color,
			prop->color.r,
			prop->color.g,
			prop->color.b,
			prop->color.a);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	rtb_render_pop(elem);
}

static void
draw_line(GLfloat line[2][2])
{
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(GLfloat[2][2]), line, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_LINES, 0, 2);
}

static void
draw_patches(struct rtb_patchbay *self)
{
	GLfloat line[2][2];
	int disconnect_in_progress = 0;
	struct rtb_patchbay_patch *iter;
	struct rtb_patchbay_port *from, *to;
	struct rtb_element *elem = RTB_ELEMENT(self);

	rtb_render_push(elem);
	rtb_render_set_position(elem, 0, 0);

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(3.5f);
	glBindBuffer(GL_ARRAY_BUFFER, self->bg_vbo[1]);

	TAILQ_FOREACH(iter, &self->patches, patchbay_patch) {
		from = iter->from;
		to   = iter->to;

		line[0][0] = from->x + from->w;
		line[0][1] = from->y + floorf(from->h / 2.f);
		line[1][0] = to->x;
		line[1][1] = to->y + floorf(to->h / 2.f);

		if ((self->patch_in_progress.from == from &&
					self->patch_in_progress.to == to) ||
				(self->patch_in_progress.from == to &&
				 self->patch_in_progress.to == from)) {
			disconnect_in_progress = 1;
			continue;
		} else if (self->patch_in_progress.from == from ||
				self->patch_in_progress.from == to)
			rtb_render_set_color(elem, CONNECTION_COLOR, .9f);
		else
			rtb_render_set_color(elem, CONNECTION_COLOR, .6f);

		draw_line(line);
	}

	if (self->patch_in_progress.from) {
		from = self->patch_in_progress.from;
		to   = self->patch_in_progress.to;

		if (from->port_type == PORT_TYPE_OUTPUT) {
			line[0][0] = from->x + from->w;
			line[0][1] = from->y + floorf(from->h / 2.f);
		} else {
			line[0][0] = from->x;
			line[0][1] = from->y + floorf(from->h / 2.f);
		}

		if (to) {
			if (to->port_type == PORT_TYPE_OUTPUT) {
				line[1][0] = to->x + to->w;
				line[1][1] = to->y + floorf(to->h / 2.f);
			} else {
				line[1][0] = to->x;
				line[1][1] = to->y + floorf(to->h / 2.f);
			}
		} else {
			line[1][0] = self->patch_in_progress.cursor.x;
			line[1][1] = self->patch_in_progress.cursor.y;
		}

		if (disconnect_in_progress)
			rtb_render_set_color(elem, DISCONNECT_COLOR, .9f);
		else if (to)
			rtb_render_set_color(elem, CONNECTION_COLOR, .8f);
		else
			rtb_render_set_color(elem, CONNECTION_COLOR, .4f);

		draw_line(line);
	}

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	rtb_render_pop(elem);
}

static void
draw(struct rtb_element *elem, rtb_draw_state_t state)
{
	SELF_FROM(elem);

	draw_bg(self);
	draw_patches(self);

	super.draw(elem, state);
}

/**
 * element implementation
 */

static int
recalculate(struct rtb_element *elem,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	const struct rtb_style_property_definition *prop;

	SELF_FROM(elem);

	super.recalculate(elem, instigator, direction);

	if (self->style) {
		prop = rtb_style_query_prop(self->style, RTB_DRAW_NORMAL,
				"background-image", RTB_STYLE_PROP_TEXTURE);

		/* XXX: casting away const-ness */
		load_tile((struct rtb_style_texture_definition *) &prop->texture,
				self->bg_texture);
	}

	rtb_surface_invalidate(RTB_SURFACE(self));
	cache_to_vbo(self);

	return 1;
}

static void
reposition(struct rtb_patchbay *self, struct rtb_point *by)
{
	struct rtb_element *iter;

	TAILQ_FOREACH(iter, &self->children, child) {
		iter->x += by->x;
		iter->y += by->y;

		rtb_elem_trigger_recalc(iter, RTB_ELEMENT(self),
				RTB_DIRECTION_LEAFWARD);
	}

	self->texture_offset.x -= by->x;
	self->texture_offset.y -= by->y;

	rtb_surface_invalidate(RTB_SURFACE(self));
	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

static int
handle_drag(struct rtb_patchbay *self, struct rtb_drag_event *e)
{
	struct rtb_point delta = {
		e->delta.x,
		e->delta.y
	};

	switch (e->button) {
	case RTB_MOUSE_BUTTON2:
		reposition(self, &delta);
		return 1;

	default:
		if (e->target == RTB_ELEMENT(self))
			return 1;
		return 0;
	}
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAGGING:
		if (handle_drag(self, (struct rtb_drag_event *) e))
			return 1;

	default:
		return super.on_event(elem, e);
	}
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay");

	cache_to_vbo(self);

	rtb_layout_vpack_top(elem);
}

/**
 * layout function
 */

static void
layout(struct rtb_element *elem)
{
	rtb_layout_unmanaged(elem);
}

/**
 * public API
 */

int
rtb_patchbay_init(struct rtb_patchbay *self)
{
	rtb_surface_init(RTB_SURFACE(self), &super);
	TAILQ_INIT(&self->patches);

	self->draw        = draw;
	self->on_event    = on_event;
	self->attached    = attached;
	self->layout_cb   = layout;
	self->recalculate = recalculate;

	init_shaders();

	self->patch_in_progress.from = NULL;

	self->texture_offset.x =
		self->texture_offset.y = 0.f;

	glGenTextures(1, &self->bg_texture);
	glGenBuffers(2, self->bg_vbo);

	return 0;
}

void
rtb_patchbay_fini(struct rtb_patchbay *self)
{
	glDeleteTextures(1, &self->bg_texture);
	rtb_surface_fini(RTB_SURFACE(self));
}

struct rtb_patchbay *
rtb_patchbay_new()
{
	struct rtb_patchbay *self = calloc(1, sizeof(*self));

	if (rtb_patchbay_init(self)) {
		free(self);
		return NULL;
	}

	return self;
}

void
rtb_patchbay_free(struct rtb_patchbay *self)
{
	rtb_patchbay_fini(self);
	free(self);
}
