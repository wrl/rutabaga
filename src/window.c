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
#include <wchar.h>
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

#define ERR(...) fprintf(stderr, "rutabaga: " __VA_ARGS__)
#define SELF_FROM(obj) rtb_win_t *self = (rtb_win_t *) obj

static struct rtb_object_implementation super;

static int initialize_default_shader(rtb_win_t *self)
{
	if (!rtb_shader_program_create(&self->default_shader,
				DEFAULT_VERT_SHADER, DEFAULT_FRAG_SHADER))
		return 1;
	return 0;
}

/**
 * object implementation
 */

static int win_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	rtb_win_t *self = RTB_WIN_T(obj);

	switch (e->type) {
	case RTB_WINDOW_CLOSE:
		if (!rtb_handle(self, e))
			rtb_stop_event_loop(self->rtb);
		return 1;

	case RTB_KEY_PRESS:
	case RTB_KEY_RELEASE:
		if (rtb_handle(self, e))
			return 1;
	}

	return super.event_cb(self, e);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent,
		rtb_win_t *window)
{
	rtb_win_t *self = RTB_WIN_T(obj);

	super.realize_cb(self, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.window");

	rtb_style_resolve_list(self, self->style_list);
}

static void mark_dirty(rtb_obj_t *obj)
{
	return;
}

/**
 * public API
 */

void rtb_window_draw(rtb_win_t *self)
{
	rtb_style_props_t *style = &self->style->states[0];

	glViewport(0, 0, self->w, self->h);

	glClearColor(
			style->bg.r,
			style->bg.g,
			style->bg.b,
			style->bg.a);
	glClear(GL_COLOR_BUFFER_BIT);

	self->draw_cb(self, RTB_DRAW_NORMAL);
	window_impl_swap_buffers(self);
}

void rtb_window_reinit(rtb_win_t *self)
{
	self->x = self->y = 0.f;

	glScissor(0, 0, self->w, self->h);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!self->window)
		rtb_obj_realize(self, NULL, self, self);
	else
		rtb_obj_trigger_recalc(self, self, RTB_DIRECTION_LEAFWARD);
}

rtb_win_t *rtb_window_open(rtb_t *r, int h, int w, const char *title)
{
	rtb_win_t *self;

	assert(r);
	assert(h > 0);
	assert(w > 0);
	assert(!r->win);

	self = window_impl_open(r, h, w, title);
	if (!self)
		goto err_window_impl;

	rtb_surface_init(self, &super);
	pthread_mutex_init(&self->lock, NULL);

	self->style_list = rtb_style_get_defaults();

	if (initialize_default_shader(self))
		goto err_shaders;

	if (rtb_font_manager_init(self))
		goto err_font;

	mat4_set_identity(&self->identity);
	rtb_obj_set_layout(self, rtb_layout_vpack_top);

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

void rtb_window_close(rtb_win_t *self)
{
	assert(self);

	rtb_font_manager_fini(self);
	rtb_shader_program_free(&self->default_shader);
	rtb_type_unref(self->type);

	window_impl_close(self);
}
