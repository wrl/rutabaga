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

#include <stdio.h>
#include <math.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>
#include <rutabaga/surface.h>
#include <rutabaga/shader.h>
#include <rutabaga/render.h>
#include <rutabaga/window.h>
#include <rutabaga/quad.h>

#include "rtb_private/util.h"
#include "rtb_private/layout-debug.h"

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
draw(struct rtb_element *elem)
{
	SELF_FROM(elem);

	rtb_surface_draw_children(self);
	rtb_surface_blit(self);
}

static int
reflow(struct rtb_element *elem, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	struct rtb_phy_size phy_size;
	struct rtb_rect phy_rect;
	struct rtb_rect tex_coords = {
		.as_float = {
			0.f, 1.f,
			1.f, 0.f
		}
	};

	SELF_FROM(elem);
	if (!super.reflow(elem, instigator, direction))
		return 0;

	if (self->w <= 0 || self->h <= 0)
		return -1;

	if (RTB_SURFACE(self->window) == self) {
		phy_size = self->phy_size;
	} else {
		self->phy_size = phy_size =
			rtb_size_to_phy(self->window, self->rect.size);
	}

	mat4_set_orthographic(&self->render_ctx.projection,
			self->x,
			self->x + self->w,
			self->y + self->h,
			self->y,
			-1.f, 1.f);

	mat4_set_orthographic(&self->phy_projection,
			self->x,
			self->x + phy_size.w,
			self->y + phy_size.h,
			self->y,
			-1.f, 1.f);

	glBindTexture(GL_TEXTURE_2D, self->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			phy_size.w, phy_size.h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, self->texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	phy_rect = self->rect;
	phy_rect.size.w *= self->window->scale.x;
	phy_rect.size.h *= self->window->scale.y;
	rtb_rect_update_points_from_size(&phy_rect);

	rtb_quad_set_vertices(&self->quad, &phy_rect);
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

	if (elem->surface && RTB_ELEMENT(elem->surface) != elem)
		rtb_elem_mark_dirty(RTB_ELEMENT(elem->surface));
}

/**
 * public API
 */

int
rtb_surface_is_dirty(struct rtb_surface *self)
{
	if (self->surface_state == RTB_SURFACE_VALID &&
			!TAILQ_FIRST(&self->render_queue)) {
		/* nothing to do. */
		return 0;
	}

	return 1;
}

void
rtb_surface_blit(struct rtb_surface *self)
{
	struct rtb_shader *shader = &self->window->local_storage.shader.surface;
	struct rtb_element *elem = RTB_ELEMENT(self);
	struct rtb_render_context *ctx;

	ctx = rtb_render_get_context(elem);

	rtb_render_reset(elem, shader);
	rtb_render_set_position(ctx, 0, 0);

	glUniformMatrix4fv(shader->matrices.projection,
		1, GL_FALSE, self->phy_projection.data);

	glBindTexture(GL_TEXTURE_2D, self->texture);
	glUniform1i(shader->tex, 0);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	rtb_render_quad(ctx, &self->quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	LAYOUT_DEBUG_DRAW_BOX(elem);
}

void
rtb_surface_draw_children(struct rtb_surface *self)
{
	struct rtb_element *iter;

	GLint bound_fb;
	GLint viewport[4];

	if (!rtb_surface_is_dirty(self))
		return;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_fb);
	glGetIntegerv(GL_VIEWPORT, viewport);

	glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
	glViewport(0, 0, self->phy_size.w, self->phy_size.h);

	self->render_ctx.window = self->window;

	/* we have slightly different ways of handling this redraw depending
	 * on what the state of the surface is. */
	switch (self->surface_state) {
	case RTB_SURFACE_INVALID:
		/* if we're marked as invalid, we clear the entire surface and
		 * redraw it from scratch. */

		glDisable(GL_SCISSOR_TEST);
		rtb_render_clear(RTB_ELEMENT(self));
		glEnable(GL_SCISSOR_TEST);

		/* first, we clean out the renderqueue for dirty elements (since
		 * we're going to be redrawing everything anyway.) */
		while ((iter = TAILQ_FIRST(&self->render_queue))) {
			TAILQ_REMOVE(&self->render_queue, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;
		}

		/* then we draw all the children. */
		TAILQ_FOREACH(iter, &self->children, child)
			rtb_elem_draw(iter, 0);

		self->surface_state = RTB_SURFACE_VALID;
		break;

	case RTB_SURFACE_VALID:
		/* if we're marked valid, we'll just do an incremental redraw
		 * just of the elements which have requested it. */
		while ((iter = TAILQ_FIRST(&self->render_queue))) {
			TAILQ_REMOVE(&self->render_queue, iter, render_entry);

			iter->render_entry.tqe_next = NULL;
			iter->render_entry.tqe_prev = NULL;

			rtb_elem_draw(iter, 1);
		}

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
rtb_surface_init(struct rtb_surface *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	self->impl = super;
	self->impl.draw           = draw;
	self->impl.reflow         = reflow;
	self->impl.attached       = attached;
	self->impl.mark_dirty     = mark_dirty;
	self->impl.child_attached = child_attached;

	TAILQ_INIT(&self->render_queue);

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
