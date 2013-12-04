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

#include "rutabaga/rutabaga.h"
#include "rutabaga/event.h"
#include "rutabaga/element.h"
#include "rutabaga/layout.h"
#include "rutabaga/container.h"
#include "rutabaga/window.h"
#include "rutabaga/font-manager.h"
#include "rutabaga/shader.h"
#include "rutabaga/surface.h"
#include "rutabaga/style.h"
#include "rutabaga/mat4.h"

#include "private/util.h"
#include "private/window_impl.h"

#include "shaders/default.glsl.h"
#include "shaders/surface.glsl.h"
#include "shaders/stylequad.glsl.h"

#define ERR(...) fprintf(stderr, "rutabaga: " __VA_ARGS__)
#define SELF_FROM(elem) \
	struct rtb_window *self = RTB_ELEMENT_AS(elem, rtb_window)

static struct rtb_element_implementation super;

static int
initialize_shaders(struct rtb_window *self)
{
	if (!rtb_shader_create(&self->shaders.dfault,
				DEFAULT_VERT_SHADER, DEFAULT_FRAG_SHADER))
		goto err_dfault;

	if (!rtb_shader_create(&self->shaders.surface,
				SURFACE_VERT_SHADER, SURFACE_FRAG_SHADER))
		goto err_surface;

	if (!rtb_shader_create(&self->shaders.stylequad,
				STYLEQUAD_VERT_SHADER, STYLEQUAD_FRAG_SHADER))
		goto err_surface;

	return 0;

err_surface:
	rtb_shader_free(&self->shaders.dfault);
err_dfault:
	return -1;
}

/**
 * element implementation
 */

static int
win_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_WINDOW_CLOSE:
		if (!rtb_handle(elem, e))
			rtb_stop_event_loop(self->rtb);
		return 1;

	case RTB_KEY_PRESS:
	case RTB_KEY_RELEASE:
		if (self->focus && self->focus != RTB_ELEMENT(self))
			if (rtb_elem_deliver_event(self->focus, e))
				return 1;

		if (rtb_handle(elem, e))
			return 1;
	}

	return super.on_event(elem, e);
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.window");

	rtb_style_resolve_list(self, self->style_list);
	self->restyle(RTB_ELEMENT(self));
}

static void
mark_dirty(struct rtb_element *elem)
{
	SELF_FROM(elem);
	self->dirty = 1;
}

/**
 * public API
 */

void
rtb_window_focus_element(struct rtb_window *self, struct rtb_element *focused)
{
	struct rtb_event ev;
	ev.type = RTB_FOCUS;

	if (self->focus == focused)
		return;

	rtb_elem_deliver_event(focused, &ev);

	if (self->focus) {
		ev.type = RTB_UNFOCUS;
		rtb_elem_deliver_event(self->focus, &ev);
	}

	self->focus = focused;
}

void
rtb_window_draw(struct rtb_window *self)
{
	const struct rtb_style_property_definition *prop;
	struct rtb_window_event ev;

	if (self->state == RTB_STATE_UNATTACHED)
		return;

	glViewport(0, 0, self->w, self->h);

	ev.type = RTB_FRAME_START;
	ev.source = RTB_EVENT_GENUINE;
	ev.window = self;
	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&ev));

	prop = rtb_style_query_prop(RTB_ELEMENT(self),
			"background-color", RTB_STYLE_PROP_COLOR, 1);

	glClearColor(
			prop->color.r,
			prop->color.g,
			prop->color.b,
			prop->color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	rtb_render_push(RTB_ELEMENT(self));
	self->draw(RTB_ELEMENT(self));
	rtb_render_pop(RTB_ELEMENT(self));

	self->dirty = 0;

	ev.type = RTB_FRAME_END;
	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&ev));
}

void
rtb_window_reinit(struct rtb_window *self)
{
	struct rtb_element *elem = RTB_ELEMENT(self);

	self->x = self->y = 0.f;

	glScissor(0, 0, self->w, self->h);

	if (!self->window)
		self->attached(elem, NULL, self);

	rtb_elem_trigger_reflow(elem, elem, RTB_DIRECTION_LEAFWARD);
}

static int
init_gl(void)
{
	int missing;

	missing = ogl_LoadFunctions();

	if (missing == ogl_LOAD_FAILED) {
		ERR("couldn't initialize openGL.\n");
		return -1;
	} else if (missing > ogl_LOAD_SUCCEEDED)
		ERR("openGL initialized, but missing %d functions.\n",
				missing - ogl_LOAD_SUCCEEDED);

	return 0;
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

	init_gl();

	if (RTB_SUBCLASS(RTB_SURFACE(self), rtb_surface_init, &super))
		goto err_surface_init;

	self->surface = RTB_SURFACE(self);
	self->style_list = rtb_style_get_defaults();

	if (initialize_shaders(self))
		goto err_shaders;

	if (rtb_font_manager_init(&self->font_manager,
				self->dpi.x, self->dpi.y))
		goto err_font;

	rtb_elem_set_layout(RTB_ELEMENT(self), rtb_layout_vpack_top);

	self->on_event   = win_event;
	self->mark_dirty = mark_dirty;
	self->attached   = attached;

	self->flags = RTB_ELEM_CLICK_FOCUS;

	self->rtb = r;
	r->win = self;
	return self;

err_font:
	rtb_shader_free(&self->shaders.stylequad);
	rtb_shader_free(&self->shaders.surface);
	rtb_shader_free(&self->shaders.dfault);
err_shaders:
err_surface_init:
	rtb_window_close(self);
err_window_impl:
	return NULL;
}

void
rtb_window_close(struct rtb_window *self)
{
	assert(self);

	rtb_shader_free(&self->shaders.stylequad);
	rtb_shader_free(&self->shaders.surface);
	rtb_shader_free(&self->shaders.dfault);

	rtb_font_manager_fini(&self->font_manager);
	rtb_type_unref(self->type);

	window_impl_close(self);
}
