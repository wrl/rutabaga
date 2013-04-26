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

#include "private/util.h"
#include "private/layout-debug.h"
#include "shaders/surface.glsl.h"

#define SELF_FROM(obj) \
	struct rtb_surface *self = (void *) obj

/**
 * internal stuff
 */

static struct rtb_object_implementation super;

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static struct {
	RTB_INHERIT(rtb_shader_program);

	GLint texture;
	GLint position;
	GLint tex_coord;
} shader = {
	.program = 0
};

static void init_shaders()
{
	if (shader.program)
		return;

	if (!rtb_shader_program_create(RTB_SHADER_PROGRAM(&shader),
				SURFACE_VERT_SHADER,
				SURFACE_FRAG_SHADER))
		puts("rtb_surface: init_shaders() failed!");

	shader.position  = glGetAttribLocation(shader.program, "position");
	shader.tex_coord = glGetAttribLocation(shader.program, "tex_coord");
	shader.texture   = glGetUniformLocation(shader.program, "texture");
}

static void cache_to_vbo(rtb_surface_t *self)
{
	GLfloat x, y, w, h;
	struct vertex {
		float x, y;
		float s, t;
	} box[4];

	x = self->x;
	y = self->y;
	w = self->w;
	h = self->h;

	box[0].x = x;
	box[0].y = y;
	box[0].s = 0.f;
	box[0].t = 1.f;

	box[1].x = x + w;
	box[1].y = y;
	box[1].s = 1.f;
	box[1].t = 1.f;

	box[2].x = x + w;
	box[2].y = y + h;
	box[2].s = 1.f;
	box[2].t = 0.f;

	box[3].x = x;
	box[3].y = y + h;
	box[3].s = 0.f;
	box[3].t = 0.f;

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * object implementation
 */

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_surface_draw_children(self, state);
	rtb_surface_blit(self);
}

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
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

	rtb_surface_invalidate(self);
	cache_to_vbo(self);
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
	rtb_obj_t *obj = RTB_OBJECT(self);

	rtb_render_push(obj);
	rtb_render_use_program(obj, RTB_SHADER_PROGRAM(&shader));
	rtb_render_set_position(obj, 0, 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, self->texture);
	glUniform1i(shader.texture, 0);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(shader.position);
	glVertexAttribPointer(shader.position,
			2, GL_FLOAT, GL_FALSE, 16, 0);
	glEnableVertexAttribArray(shader.tex_coord);
	glVertexAttribPointer(shader.tex_coord,
			2, GL_FLOAT, GL_FALSE, 16, (void *) 8);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	rtb_render_pop(obj);

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	switch (self->surface_state) {
	case RTB_SURFACE_INVALID:
		glDisable(GL_SCISSOR_TEST);
		rtb_render_clear(RTB_OBJECT(self));

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
	glGenBuffers(1, &self->vbo);

	self->surface_state = RTB_SURFACE_INVALID;

	init_shaders();

	return 0;
}

void rtb_surface_fini(rtb_surface_t *self)
{
	glDeleteTextures(1, &self->texture);
	glDeleteFramebuffers(1, &self->fbo);
	glDeleteBuffers(1, &self->vbo);

	rtb_obj_fini(RTB_OBJECT(self));
}
