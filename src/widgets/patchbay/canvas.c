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

#include "rutabaga/widgets/patchbay.h"

#include "private/util.h"
#include "private/targa.h"
#include "shaders/patchbay-canvas.glsl.h"

#define SELF_FROM(obj) \
	struct rtb_patchbay *self = (void *) obj

#define CONNECTION_COLOR	RGB(0x404F3C)
#define DISCONNECT_COLOR	RGB(0x69181B)

static struct rtb_object_implementation super;

/**
 * custom openGL stuff
 */

#define TILE_IMG "./assets/tile.tga"
targa_t tile;

static struct {
	RTB_INHERIT(rtb_shader_program);

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

static void init_shaders()
{
	if (shader.program)
		return;

	if (!rtb_shader_program_create(RTB_SHADER_PROGRAM(&shader),
				PATCHBAY_CANVAS_VERT_SHADER,
				PATCHBAY_CANVAS_FRAG_SHADER))
		puts("rtb_patchbay: init_shaders() failed!");

#define CACHE_UNIFORM_LOCATION(name) shader.uniform.name = glGetUniformLocation(shader.program, #name)
	CACHE_UNIFORM_LOCATION(texture);
	CACHE_UNIFORM_LOCATION(tx_size);
	CACHE_UNIFORM_LOCATION(tx_offset);

	CACHE_UNIFORM_LOCATION(front_color);
	CACHE_UNIFORM_LOCATION(back_color);
#undef CACHE_UNIFORM_LOCATION
}

static void load_tile(rtb_patchbay_t *self)
{
	if (!tile.data) {
		if (targa_load(&tile, TILE_IMG) < 0)
			return;
	}

	glGenTextures(1, &self->bg_texture);
	glBindTexture(GL_TEXTURE_2D, self->bg_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			tile.info.width, tile.info.height,
			0, GL_BGRA, GL_UNSIGNED_BYTE,
			tile.data);

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

static void cache_to_vbo(rtb_patchbay_t *self)
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

static void draw_bg(rtb_patchbay_t *self)
{
	rtb_obj_t *obj = RTB_OBJECT(self);
	rtb_style_t *style = self->style;

	rtb_render_push(obj);
	rtb_render_use_program(obj, RTB_SHADER_PROGRAM(&shader));
	rtb_render_set_position(obj, 0, 0);

	/* draw the background */
	glBindBuffer(GL_ARRAY_BUFFER, self->bg_vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, self->bg_texture);
	glUniform1i(shader.uniform.texture, 0);
	glUniform2f(shader.uniform.tx_size, tile.info.width, tile.info.height);
	glUniform2f(shader.uniform.tx_offset,
			roundf(self->texture_offset.x),
			roundf(self->texture_offset.y));

	glUniform4f(shader.uniform.front_color,
			style->states[0].fg.r,
			style->states[0].fg.g,
			style->states[0].fg.b,
			style->states[0].fg.a);

	glUniform4f(shader.uniform.back_color,
			style->states[0].bg.r,
			style->states[0].bg.g,
			style->states[0].bg.b,
			style->states[0].bg.a);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	rtb_render_pop(obj);
}

static void draw_line(GLfloat line[2][2])
{
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(GLfloat[2][2]), line, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_LINES, 0, 2);
}

static void draw_patches(rtb_patchbay_t *self)
{
	GLfloat line[2][2];
	int disconnect_in_progress = 0;
	rtb_patchbay_patch_t *iter;
	rtb_patchbay_port_t *from, *to;
	rtb_obj_t *obj = RTB_OBJECT(self);

	rtb_render_push(obj);
	rtb_render_set_position(obj, 0, 0);

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
			rtb_render_set_color(obj, CONNECTION_COLOR, .9f);
		else
			rtb_render_set_color(obj, CONNECTION_COLOR, .6f);

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
			rtb_render_set_color(obj, DISCONNECT_COLOR, .9f);
		else if (to)
			rtb_render_set_color(obj, CONNECTION_COLOR, .8f);
		else
			rtb_render_set_color(obj, CONNECTION_COLOR, .4f);

		draw_line(line);
	}

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	rtb_render_pop(obj);
}

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	draw_bg(self);
	draw_patches(self);

	super.draw_cb(obj, state);
}

/**
 * object implementation
 */

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	SELF_FROM(obj);

	super.recalc_cb(obj, instigator, direction);
	rtb_surface_invalidate(RTB_SURFACE(self));
	cache_to_vbo(self);
}

static void reposition(rtb_patchbay_t *self, rtb_pt_t *by)
{
	rtb_obj_t *iter;

	TAILQ_FOREACH(iter, &self->children, child) {
		iter->x += by->x;
		iter->y += by->y;

		rtb_obj_trigger_recalc(iter, RTB_OBJECT(self),
				RTB_DIRECTION_LEAFWARD);
	}

	self->texture_offset.x -= by->x;
	self->texture_offset.y -= by->y;

	rtb_surface_invalidate(RTB_SURFACE(self));
	rtb_obj_mark_dirty(RTB_OBJECT(self));
}

static int handle_drag(rtb_patchbay_t *self, rtb_ev_drag_t *e)
{
	rtb_pt_t delta = {
		e->delta.x,
		e->delta.y
	};

	switch (e->button) {
	case RTB_MOUSE_BUTTON2:
		reposition(self, &delta);
		return 1;

	default:
		if (e->target == RTB_OBJECT(self))
			return 1;
		return 0;
	}
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAGGING:
		if (handle_drag(self, (rtb_ev_drag_t *) e))
			return 1;

	default:
		return super.event_cb(obj, e);
	}
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay");

	cache_to_vbo(self);

	rtb_layout_vpack_top(obj);
}

/**
 * layout function
 */

static void layout(rtb_obj_t *obj)
{
	rtb_layout_unmanaged(obj);
}

/**
 * public API
 */

rtb_patchbay_t *rtb_patchbay_new()
{
	rtb_patchbay_t *self = calloc(1, sizeof(*self));

	rtb_surface_init(RTB_SURFACE(self), &super);
	TAILQ_INIT(&self->patches);

	self->draw_cb    = draw;
	self->event_cb   = on_event;
	self->realize_cb = realize;
	self->layout_cb  = layout;
	self->recalc_cb  = recalculate;

	init_shaders();
	load_tile(self);

	self->patch_in_progress.from = NULL;

	self->texture_offset.x =
		self->texture_offset.y = 0.f;

	glGenBuffers(2, self->bg_vbo);

	return self;
}

void rtb_patchbay_free(rtb_patchbay_t *self)
{
	if (self->bg_texture)
		glDeleteTextures(1, &self->bg_texture);

	/* if ref_count is one, this is the last patchbay allocated, and
	 * the type will be freed by the subsequent rtb_surface_fini(). */
	if (self->type->ref_count == 1 && tile.data)
		free(tile.data);

	rtb_surface_fini(RTB_SURFACE(self));

	free(self);
}
