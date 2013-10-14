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
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/surface.h"
#include "rutabaga/shader.h"
#include "rutabaga/render.h"
#include "rutabaga/window.h"
#include "rutabaga/quad.h"

#include "private/util.h"
#include "private/layout-debug.h"

#define SELF_FROM(elem) \
	struct rtb_surface *self = RTB_ELEMENT_AS(elem, rtb_surface)

/**
 * internal stuff
 */

static struct rtb_element_implementation super;

/**
 * element implementation
 */

static void
draw(struct rtb_element *elem, rtb_draw_state_t state)
{
	SELF_FROM(elem);

	rtb_render_push(elem);
	rtb_surface_draw_children(self, state);
	rtb_surface_blit(self);
	rtb_render_pop(elem);
}

static int
recalculate(struct rtb_element *elem, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	struct rtb_rect tex_coords = {
		.as_float = {
			0.f, 1.f,
			1.f, 0.f
		}
	};

	SELF_FROM(elem);
	super.recalculate(elem, instigator, direction);

	if (self->w <= 0 || self->h <= 0)
		return -1;

	mat4_set_orthographic(&self->projection,
			self->x, self->x + self->w,
			self->y + self->h, self->y,
			-1.f, 1.f);

	glBindTexture(GL_TEXTURE_2D, self->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			lrintf(self->w), lrintf(self->h), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, self->texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	rtb_quad_set_vertices(&self->quad, &self->rect);
	rtb_quad_set_tex_coords(&self->quad, &tex_coords);

	rtb_surface_invalidate(self);

	return 1;
}

static void
child_attached(struct rtb_element *elem, struct rtb_element *child)
{
	SELF_FROM(elem);

	child->surface = self;
	child->attached(child, RTB_ELEMENT(self), self->window);
}

static void
attached(struct rtb_element *self,
		struct rtb_element *parent, struct rtb_window *window)
{
	super.attached(self, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.surface");
}

static void
mark_dirty(struct rtb_element *elem)
{
	super.mark_dirty(elem);

	if (elem->surface)
		rtb_elem_mark_dirty(RTB_ELEMENT(elem->surface));
}

/**
 * public API
 */

int
rtb_surface_is_dirty(struct rtb_surface *self)
{
	struct rtb_render_context *ctx = &self->render_ctx;

	if (self->surface_state == RTB_SURFACE_VALID &&
			(!TAILQ_FIRST(&ctx->queues.next_frame) &&
			 !TAILQ_FIRST(&ctx->queues.every_frame))) {
		/* nothing to do. */
		return 0;
	}

	return 1;
}

void
rtb_surface_blit(struct rtb_surface *self)
{
	struct rtb_shader *shader = &self->window->shaders.surface;
	struct rtb_element *elem = RTB_ELEMENT(self);

	rtb_render_reset(elem);
	rtb_render_use_shader(elem, shader);
	rtb_render_set_position(elem, 0, 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, self->texture);
	glUniform1i(shader->texture, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	rtb_render_quad(elem, &self->quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	LAYOUT_DEBUG_DRAW_BOX(elem);
}

void
rtb_surface_draw_children(struct rtb_surface *self, rtb_draw_state_t state)
{
	struct rtb_render_context *ctx = &self->render_ctx;
	struct rtb_element *iter;

	GLint bound_fb;
	GLint viewport[4];

	if (!rtb_surface_is_dirty(self))
		return;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_fb);
	glGetIntegerv(GL_VIEWPORT, viewport);

	glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
	glViewport(0, 0, self->w, self->h);

	switch (self->surface_state) {
	case RTB_SURFACE_INVALID:
		glDisable(GL_SCISSOR_TEST);
		rtb_render_clear(RTB_ELEMENT(self));

		while ((iter = TAILQ_FIRST(&ctx->queues.next_frame))) {
			TAILQ_REMOVE(&ctx->queues.next_frame, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;
		}

		TAILQ_FOREACH(iter, &self->children, child)
			rtb_elem_draw(iter, RTB_DRAW_NORMAL);

		self->surface_state = RTB_SURFACE_VALID;
		break;

	case RTB_SURFACE_VALID:
		while ((iter = TAILQ_FIRST(&ctx->queues.next_frame))) {
			TAILQ_REMOVE(&ctx->queues.next_frame, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;

			rtb_elem_draw(iter, RTB_DRAW_NORMAL);
		}

		TAILQ_FOREACH(iter, &ctx->queues.every_frame, render_entry)
			rtb_elem_draw(iter, RTB_DRAW_NORMAL);

		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, bound_fb);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void
rtb_surface_invalidate(struct rtb_surface *self)
{
	self->surface_state = RTB_SURFACE_INVALID;
	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

int
rtb_surface_init(struct rtb_surface *self,
		struct rtb_element_implementation *impl)
{
	struct rtb_element_implementation *elem_impl = &self->impl;
	rtb_elem_init(RTB_ELEMENT(self), &super);
	(*impl) = super;

	do {
		impl->draw           = draw;
		impl->recalculate    = recalculate;
		impl->attached       = attached;
		impl->mark_dirty     = mark_dirty;
		impl->child_attached = child_attached;
	} while (impl != elem_impl && (impl = elem_impl));

	TAILQ_INIT(&self->render_ctx.queues.every_frame);
	TAILQ_INIT(&self->render_ctx.queues.next_frame);

	glGenTextures(1, &self->texture);
	glGenFramebuffers(1, &self->fbo);
	rtb_quad_init(&self->quad);

	self->surface_state = RTB_SURFACE_INVALID;

	return 0;
}

void
rtb_surface_fini(struct rtb_surface *self)
{
	rtb_quad_fini(&self->quad);

	glDeleteFramebuffers(1, &self->fbo);
	glDeleteTextures(1, &self->texture);

	rtb_elem_fini(RTB_ELEMENT(self));
}
