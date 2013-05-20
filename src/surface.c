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
#include "rutabaga/object.h"
#include "rutabaga/surface.h"
#include "rutabaga/shader.h"
#include "rutabaga/render.h"
#include "rutabaga/window.h"
#include "rutabaga/quad.h"

#include "private/util.h"
#include "private/layout-debug.h"

#define SELF_FROM(obj) \
	struct rtb_surface *self = RTB_OBJECT_AS(obj, rtb_surface)

/**
 * internal stuff
 */

static struct rtb_object_implementation super;

/**
 * object implementation
 */

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_render_push(obj);
	rtb_surface_draw_children(self, state);
	rtb_surface_blit(self);
	rtb_render_pop(obj);
}

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	struct rtb_rect tex_coords = {
		{0.f, 1.f},
		{1.f, 0.f}
	};

	SELF_FROM(obj);
	super.recalc_cb(obj, instigator, direction);

	if (self->w <= 0 || self->h <= 0)
		return;

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
}

static void attach_child(rtb_obj_t *obj, rtb_obj_t *child)
{
	SELF_FROM(obj);
	rtb_obj_realize(child, obj, self, self->window);
}

static void realize(rtb_obj_t *self, rtb_obj_t *parent,
		rtb_win_t *window)
{
	super.realize_cb(self, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.surface");
}

/**
 * public API
 */

void rtb_surface_blit(rtb_surface_t *self)
{
	struct rtb_shader *shader = &self->window->shaders.surface;
	rtb_obj_t *obj = RTB_OBJECT(self);

	rtb_render_reset(obj);
	rtb_render_use_shader(obj, shader);
	rtb_render_set_position(obj, 0, 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, self->texture);
	glUniform1i(shader->texture, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	rtb_render_quad(obj, &self->quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	LAYOUT_DEBUG_DRAW_BOX(obj);
}

void rtb_surface_draw_children(rtb_surface_t *self, rtb_draw_state_t state)
{
	struct rtb_render_context *ctx = &self->render_ctx;
	rtb_obj_t *iter;

	GLint bound_fb;
	GLint viewport[4];

	if (self->surface_state == RTB_SURFACE_VALID &&
			(!TAILQ_FIRST(&ctx->queues.next_frame) &&
			 !TAILQ_FIRST(&ctx->queues.every_frame))) {
		/* nothing to do. */
		return;
	}

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_fb);
	glGetIntegerv(GL_VIEWPORT, viewport);

	glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
	glViewport(0, 0, self->w, self->h);

	switch (self->surface_state) {
	case RTB_SURFACE_INVALID:
		glDisable(GL_SCISSOR_TEST);
		rtb_render_clear(RTB_OBJECT(self));

		while ((iter = TAILQ_FIRST(&ctx->queues.next_frame))) {
			TAILQ_REMOVE(&ctx->queues.next_frame, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;
		}

		TAILQ_FOREACH(iter, &self->children, child)
			rtb_obj_draw(iter, RTB_DRAW_NORMAL);

		self->surface_state = RTB_SURFACE_VALID;
		break;

	case RTB_SURFACE_VALID:
		while ((iter = TAILQ_FIRST(&ctx->queues.next_frame))) {
			TAILQ_REMOVE(&ctx->queues.next_frame, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;

			rtb_obj_draw(iter, RTB_DRAW_NORMAL);
		}

		TAILQ_FOREACH(iter, &ctx->queues.every_frame, render_entry)
			rtb_obj_draw(iter, RTB_DRAW_NORMAL);

		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, bound_fb);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void rtb_surface_invalidate(rtb_surface_t *self)
{
	self->surface_state = RTB_SURFACE_INVALID;
	rtb_obj_mark_dirty(RTB_OBJECT(self));
}

int rtb_surface_init(rtb_surface_t *self,
		struct rtb_object_implementation *impl)
{
	struct rtb_object_implementation *obj_impl = &self->impl;
	rtb_obj_init(RTB_OBJECT(self), &super);
	(*impl) = super;

	do {
		impl->draw_cb      = draw;
		impl->recalc_cb    = recalculate;
		impl->attach_child = attach_child;
		impl->realize_cb   = realize;
	} while (impl != obj_impl && (impl = obj_impl));

	TAILQ_INIT(&self->render_ctx.queues.every_frame);
	TAILQ_INIT(&self->render_ctx.queues.next_frame);

	glGenTextures(1, &self->texture);
	glGenFramebuffers(1, &self->fbo);
	rtb_quad_init(&self->quad);

	self->surface_state = RTB_SURFACE_INVALID;

	return 0;
}

void rtb_surface_fini(rtb_surface_t *self)
{
	rtb_quad_fini(&self->quad);

	glDeleteFramebuffers(1, &self->fbo);
	glDeleteTextures(1, &self->texture);

	rtb_obj_fini(RTB_OBJECT(self));
}
