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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/event.h"
#include "rutabaga/object.h"
#include "rutabaga/layout.h"
#include "rutabaga/container.h"
#include "rutabaga/window.h"
#include "rutabaga/font-manager.h"
#include "rutabaga/shader.h"
#include "rutabaga/surface.h"
#include "rutabaga/style.h"
#include "rutabaga/mat4.h"

#include "private/util.h"

#include "shaders/default.glsl.h"
#include "shaders/surface.glsl.h"

#define ERR(...) fprintf(stderr, "rutabaga: " __VA_ARGS__)
#define SELF_FROM(obj) \
	struct rtb_window *self = RTB_OBJECT_AS(obj, rtb_window)

static struct rtb_object_implementation super;

static int
initialize_shaders(struct rtb_window *self)
{
	struct rtb_shader *s;

	if (!rtb_shader_create(&self->shaders.dfault,
				DEFAULT_VERT_SHADER, DEFAULT_FRAG_SHADER))
		goto err_dfault;

	if (!rtb_shader_create(&self->shaders.surface,
				SURFACE_VERT_SHADER, SURFACE_FRAG_SHADER))
		goto err_surface;

	s = &self->shaders.surface;
	s->tex_coord = glGetAttribLocation(s->program, "tex_coord");
	s->texture   = glGetUniformLocation(s->program, "texture");

	return 0;

err_surface:
	rtb_shader_free(&self->shaders.dfault);
err_dfault:
	return -1;
}

/**
 * object implementation
 */

static int
win_event(struct rtb_object *obj, const struct rtb_event *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_WINDOW_CLOSE:
		if (!rtb_handle(obj, e))
			rtb_stop_event_loop(self->rtb);
		return 1;

	case RTB_KEY_PRESS:
	case RTB_KEY_RELEASE:
		if (self->focus && self->focus != RTB_OBJECT(self))
			if (rtb_obj_deliver_event(self->focus, e))
				return 1;

		if (rtb_handle(obj, e))
			return 1;
	}

	return super.event_cb(obj, e);
}

static void
realize(struct rtb_object *obj, struct rtb_object *parent,
		struct rtb_window *window)
{
	SELF_FROM(obj);

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.window");

	rtb_style_resolve_list(self, self->style_list);
}

static void
mark_dirty(struct rtb_object *obj)
{
	return;
}

/**
 * public API
 */

void
rtb_window_focus_object(struct rtb_window *self, struct rtb_object *focused)
{
	struct rtb_object *unfocused = self->focus;

	self->focus = focused;
	rtb_obj_mark_dirty(focused);

	if (unfocused)
		rtb_obj_mark_dirty(unfocused);
}

void
rtb_window_draw(struct rtb_window *self)
{
	struct rtb_style_props *style = &self->style->states[0];

	glViewport(0, 0, self->w, self->h);

	glClearColor(
			style->bg.r,
			style->bg.g,
			style->bg.b,
			style->bg.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	rtb_render_push(RTB_OBJECT(self));
	self->draw_cb(RTB_OBJECT(self), RTB_DRAW_NORMAL);
	rtb_render_pop(RTB_OBJECT(self));

	window_impl_swap_buffers(self);
}

void
rtb_window_reinit(struct rtb_window *self)
{
	struct rtb_object *obj = RTB_OBJECT(self);

	self->x = self->y = 0.f;

	glScissor(0, 0, self->w, self->h);

	if (!self->window)
		rtb_obj_realize(obj, NULL, RTB_SURFACE(self), self);
	else
		rtb_obj_trigger_recalc(obj, obj, RTB_DIRECTION_LEAFWARD);
}

struct rtb_window *
rtb_window_open(struct rutabaga *r,
		int h, int w, const char *title)
{
	struct rtb_window *self;

	assert(r);
	assert(h > 0);
	assert(w > 0);
	assert(!r->win);

	self = window_impl_open(r, h, w, title);
	if (!self)
		goto err_window_impl;

	rtb_surface_init(RTB_SURFACE(self), &super);

	self->style_list = rtb_style_get_defaults();

	if (initialize_shaders(self))
		goto err_shaders;

	if (rtb_font_manager_init(self))
		goto err_font;

	mat4_set_identity(&self->identity);
	rtb_obj_set_layout(RTB_OBJECT(self), rtb_layout_vpack_top);

	self->event_cb   = win_event;
	self->mark_dirty = mark_dirty;
	self->realize_cb = realize;

	r->win = self;
	return self;

err_font:
err_shaders:
	rtb_window_close(self);
err_window_impl:
	return NULL;
}

void
rtb_window_close(struct rtb_window *self)
{
	assert(self);

	rtb_shader_free(&self->shaders.surface);
	rtb_shader_free(&self->shaders.dfault);

	rtb_font_manager_fini(self);
	rtb_type_unref(self->type);

	window_impl_close(self);
}
