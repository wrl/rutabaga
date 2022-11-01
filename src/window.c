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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/platform.h>
#include <rutabaga/event.h>
#include <rutabaga/element.h>
#include <rutabaga/layout.h>
#include <rutabaga/container.h>
#include <rutabaga/window.h>
#include <rutabaga/font-manager.h>
#include <rutabaga/shader.h>
#include <rutabaga/surface.h>
#include <rutabaga/style.h>
#include <rutabaga/mat4.h>

#include "rtb_private/util.h"
#include "rtb_private/window_impl.h"

#include "shaders/default.glsl.h"
#include "shaders/surface.glsl.h"
#include "shaders/stylequad.glsl.h"

#define ERR(...) fprintf(stderr, "rutabaga: " __VA_ARGS__)
#define SELF_FROM(elem) \
	struct rtb_window *self = RTB_ELEMENT_AS(elem, rtb_window)

static struct rtb_element_implementation super;

/**
 * index buffer objects
 *
 * XXX: kind of bullshit to have these here, should have a better way
 *      of sharing and managing window-local variables.
 */

static const GLubyte stylequad_border_indices[] = {
	/**
	 * +---+---+---+
	 * | 1 | 2 | 3 |
	 * +---+---+---+
	 * | 4 | 5 | 6 |
	 * +---+---+---+
	 * | 7 | 8 | 9 |
	 * +---+---+---+
	 *
	 * the middle section is not drawn because it's conditionally drawn
	 * based on whether the texture has defined RTB_TEXTURE_FILL.
	 * if it is, we'll just draw with solid_indices.
	 */

	/* 1 */  0,  2,  1,  3,  2,  0,
	/* 2 */  1,  7,  4,  2,  7,  1,
	/* 3 */  4,  6,  5,  7,  6,  4,

	/* 4 */  3,  9,  2,  8,  9,  3,
	/* 5 */
	/* 6 */  7, 13,  6, 12, 13,  7,

	/* 7 */  8, 10,  9, 11, 10,  8,
	/* 8 */  9, 15, 12, 10, 15,  9,
	/* 9 */ 12, 14, 13, 15, 14, 12
};

static const GLubyte stylequad_solid_indices[] = {
	2, 7, 9, 12
};

static const GLubyte stylequad_outline_indices[] = {
	2, 7, 12, 9
};

static const GLubyte quad_solid_indices[] = {
	0, 1, 3, 2
};

static const GLubyte quad_outline_indices[] = {
	0, 1, 2, 3
};

static GLuint
ibo_new(const void *data, size_t size)
{
	GLuint ibo;

	glGenBuffers(1, &ibo);
	if (!ibo)
		return 0;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return ibo;
}

static void
ibo_free(GLuint ibo)
{
	if (ibo)
		glDeleteBuffers(1, &ibo);
}

static int
ibos_init(struct rtb_window *self)
{
#define IBO_ALLOC(dst, src) (self->local_storage.ibo.dst = ibo_new(src, sizeof(src)))
	if (!IBO_ALLOC(quad.solid, quad_solid_indices))
		goto err_quad_solid;

	if (!IBO_ALLOC(quad.outline, quad_outline_indices))
		goto err_quad_outline;

	if (!IBO_ALLOC(stylequad.border, stylequad_border_indices))
		goto err_stylequad_border;

	if (!IBO_ALLOC(stylequad.solid, stylequad_solid_indices))
		goto err_stylequad_solid;

	if (!IBO_ALLOC(stylequad.outline, stylequad_outline_indices))
		goto err_stylequad_outline;
#undef IBO_ALLOC

	return 0;

err_stylequad_outline:
	ibo_free(self->local_storage.ibo.stylequad.solid);
err_stylequad_solid:
	ibo_free(self->local_storage.ibo.stylequad.border);
err_stylequad_border:
	ibo_free(self->local_storage.ibo.quad.outline);
err_quad_outline:
	ibo_free(self->local_storage.ibo.quad.solid);
err_quad_solid:
	return -1;
}

static void
ibos_fini(struct rtb_window *self)
{
	ibo_free(self->local_storage.ibo.stylequad.outline);
	ibo_free(self->local_storage.ibo.stylequad.solid);
	ibo_free(self->local_storage.ibo.stylequad.border);
	ibo_free(self->local_storage.ibo.quad.outline);
	ibo_free(self->local_storage.ibo.quad.solid);
}

/**
 * shaders
 */

static int
shaders_init(struct rtb_window *self)
{
	if (!rtb_shader_create(&self->local_storage.shader.dfault,
				DEFAULT_VERT_SHADER, NULL, DEFAULT_FRAG_SHADER))
		goto err_dfault;

	if (!rtb_shader_create(&self->local_storage.shader.surface,
				SURFACE_VERT_SHADER, NULL, SURFACE_FRAG_SHADER))
		goto err_surface;

	if (!rtb_shader_create(&self->local_storage.shader.stylequad,
				STYLEQUAD_VERT_SHADER, NULL, STYLEQUAD_FRAG_SHADER))
		goto err_stylequad;

	return 0;

err_stylequad:
	rtb_shader_free(&self->local_storage.shader.surface);
err_surface:
	rtb_shader_free(&self->local_storage.shader.dfault);
err_dfault:
	return -1;
}

static void
shaders_fini(struct rtb_window *self)
{
	rtb_shader_free(&self->local_storage.shader.stylequad);
	rtb_shader_free(&self->local_storage.shader.surface);
	rtb_shader_free(&self->local_storage.shader.dfault);

}

/**
 * element implementation
 */

static int
win_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_WINDOW_SHOULD_CLOSE:
		if (rtb_handle(elem, e) < 1) {
			struct rtb_window_event wc_ev = {
				.type = RTB_WINDOW_WILL_CLOSE,
				.window = self,
			};

			rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&wc_ev));
		}

		return 1;

	case RTB_WINDOW_WILL_CLOSE:
		if (rtb_handle(elem, e) < 1)
			rtb_event_loop_stop(self->rtb);

		return 1;

	case RTB_KEY_PRESS:
	case RTB_KEY_RELEASE:
		if (self->focus && self->focus != RTB_ELEMENT(self))
			if (rtb_elem_deliver_event(self->focus, e))
				return 1;

		if (rtb_handle(elem, e) != -1)
			return 1;

		break;

	case RTB_MOUSE_ENTER:
		rtb__platform_set_cursor(self, &self->mouse,
				self->mouse.current_cursor);
		break;
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

	self->overlay_surface.attached(
			RTB_ELEMENT(&self->overlay_surface),
			elem, self);

	rtb_style_resolve_list(self, self->style_list);
	self->restyle(RTB_ELEMENT(self));
}

static void
restyle(struct rtb_element *elem)
{
	SELF_FROM(elem);

	super.restyle(elem);
	self->overlay_surface.restyle(RTB_ELEMENT(&self->overlay_surface));
}

static int
reflow(struct rtb_element *elem, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	SELF_FROM(elem);
	int ret;

	ret = super.reflow(elem, instigator, direction);

	rtb_elem_set_size(RTB_ELEMENT(&self->overlay_surface),
			&self->rect.size);
	self->overlay_surface.reflow(RTB_ELEMENT(&self->overlay_surface),
			instigator, direction);

	return ret;
}

static void
mark_dirty(struct rtb_element *elem)
{
	elem->window->dirty = 1;
}

/**
 * overlays
 */

void
rtb_window_add_overlay(struct rtb_window *self, struct rtb_element *child,
		rtb_child_add_loc_t where)
{
	rtb_elem_add_child(RTB_ELEMENT(&self->overlay_surface), child, where);
}

void
rtb_window_remove_overlay(struct rtb_window *self, struct rtb_element *child)
{
	rtb_elem_remove_child(RTB_ELEMENT(&self->overlay_surface), child);
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

int
rtb_window_draw(struct rtb_window *self, int force_redraw)
{
	const struct rtb_style_property_definition *prop;
	struct rtb_window_event ev;

	if (self->state == RTB_STATE_UNATTACHED
			|| self->visibility == RTB_FULLY_OBSCURED)
		return 0;

	ev.type = RTB_FRAME_START;
	ev.source = RTB_EVENT_GENUINE;
	ev.window = self;
	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&ev));

	if (!self->dirty || force_redraw)
		return 0;

	glViewport(0, 0, self->phy_size.w, self->phy_size.h);

	prop = rtb_style_query_prop(RTB_ELEMENT(self),
			"background-color", RTB_STYLE_PROP_COLOR, 1);

	glEnable(GL_DITHER);
	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);

	glClearColor(
			prop->color.r,
			prop->color.g,
			prop->color.b,
			prop->color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	rtb_render_push(RTB_ELEMENT(self));
	self->draw(RTB_ELEMENT(self));
	rtb_render_pop(RTB_ELEMENT(self));

	ev.type = RTB_FRAME_END;
	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&ev));

	rtb_render_push(RTB_ELEMENT(self));
	self->overlay_surface.draw(RTB_ELEMENT(&self->overlay_surface));
	rtb_render_pop(RTB_ELEMENT(self));

	self->dirty = 0;

	return 1;
}

void
rtb_window_reinit(struct rtb_window *self)
{
	struct rtb_element *elem = RTB_ELEMENT(self);

	self->finished_initialising = 0;

	self->w = ceilf(self->phy_size.w / self->scale.x);
	self->h = ceilf(self->phy_size.h / self->scale.y);

	self->x = self->y = 0.f;

	glScissor(0, 0, self->phy_size.w, self->phy_size.h);

	if (!self->window)
		self->attached(elem, NULL, self);

	self->finished_initialising = 1;
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
rtb_window_open(struct rutabaga *r, const struct rtb_window_open_options *opt)
{
	struct rtb_style_data stdata;
	struct rtb_window *self;

	assert(r);
	assert(opt->width > 0);
	assert(opt->height > 0);
	assert(!r->win);

	self = window_impl_open(r, opt);
	if (!self)
		goto err_window_impl;

	init_gl();

	if (RTB_SUBCLASS(RTB_SURFACE(self), rtb_surface_init, &super))
		goto err_surface_init;

	self->w = ceilf(opt->width  * self->scale_recip.x);
	self->h = ceilf(opt->height * self->scale_recip.y);

	self->surface = RTB_SURFACE(self);

	stdata = rtb_style_get_defaults();
	self->style_list = stdata.style;
	self->style_fonts = calloc(stdata.nfonts, sizeof(*self->style_fonts));

	if (shaders_init(self))
		goto err_shaders;

	if (ibos_init(self))
		goto err_ibos;

	if (rtb_font_manager_init(&self->font_manager,
				self->dpi.x, self->dpi.y))
		goto err_font;

	rtb_elem_set_layout(RTB_ELEMENT(self), rtb_layout_vpack_top);

	self->on_event   = win_event;
	self->mark_dirty = mark_dirty;
	self->attached   = attached;
	self->restyle    = restyle;
	self->reflow     = reflow;

	self->flags = RTB_ELEM_CLICK_FOCUS;

	/* for core profiles */
	glGenVertexArrays(1, &self->vao);
	glBindVertexArray(self->vao);

	self->rtb = r;
	r->win = self;

	rtb_surface_init(&self->overlay_surface);
	self->overlay_surface.surface = &self->overlay_surface;
	self->overlay_surface.mark_dirty = mark_dirty;

	self->outer_pad.x = 0;
	self->outer_pad.y = 0;

	self->overlay_surface.outer_pad.x = 0;
	self->overlay_surface.outer_pad.y = 0;

	self->mouse_in_overlay = 0;
	self->overlay_surface.layout_cb = rtb_layout_unmanaged;

	self->mouse.current_cursor = RTB_MOUSE_CURSOR_DEFAULT;
	return self;

err_font:
	ibos_fini(self);
err_ibos:
	shaders_fini(self);
err_shaders:
err_surface_init:
	window_impl_close(self);
err_window_impl:
	return NULL;
}

void
rtb_window_close(struct rtb_window *self)
{
	assert(self);

	rtb_surface_fini(&self->overlay_surface);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &self->vao);

	rtb_font_manager_fini(&self->font_manager);

	ibos_fini(self);
	shaders_fini(self);

	free(self->style_fonts);
	free(self->style_list);

	rtb_surface_fini(RTB_SURFACE(self));
	window_impl_close(self);
}
