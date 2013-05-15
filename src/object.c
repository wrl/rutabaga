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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/object.h"
#include "rutabaga/layout.h"
#include "rutabaga/window.h"

#include "rutabaga/style.h"
#include "rutabaga/atom.h"

#include "rutabaga/event.h"
#include "rutabaga/mouse.h"

#include "private/stdlib-allocator.h"
#include "private/layout-debug.h"

#include "wwrl/vector.h"

/**
 * private stuff
 */

static int recalc_rootward(rtb_obj_t *self, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	rtb_obj_t *iter;
	struct rtb_size inst_old_size = {
		instigator->w,
		instigator->h
	};

	self->layout_cb(self);

	/* don't pass the recalculation any further rootward if the object's
	 * size hasn't changed as a result of it. */
	if ((instigator->w == inst_old_size.w &&
	     instigator->h == inst_old_size.h))
		return 0;

	TAILQ_FOREACH(iter, &self->children, child)
		iter->recalc_cb(iter, self, RTB_DIRECTION_LEAFWARD);

	if (self->parent)
		self->parent->recalc_cb(self->parent, self, direction);

	rtb_obj_mark_dirty(self);
	return 1;
}

static void recalc_leafward(rtb_obj_t *self, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	rtb_obj_t *iter;

	self->layout_cb(self);

	TAILQ_FOREACH(iter, &self->children, child)
		iter->recalc_cb(iter, self, direction);
}

static void recalculate(rtb_obj_t *self, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	/* XXX: invariant: self->window->state != RTB_STATE_UNREALIZED */
	if (!self->style)
		self->style = rtb_style_for_object(self, self->window->style_list);

	switch (direction) {
	case RTB_DIRECTION_ROOTWARD:
		if (!recalc_rootward(self, instigator, RTB_DIRECTION_ROOTWARD))
			return;
		break;

	case RTB_DIRECTION_LEAFWARD:
		recalc_leafward(self, instigator, RTB_DIRECTION_LEAFWARD);
		break;
	}

	self->rect.p1.x = self->rect.p2.x = self->x;
	self->rect.p1.y = self->rect.p2.y = self->y;
	self->rect.p2.x += (GLfloat) self->w;
	self->rect.p2.y += (GLfloat) self->h;
}

/**
 * drawing
 */

static void draw(rtb_obj_t *self, rtb_draw_state_t state)
{
	rtb_obj_t *iter;

	/* if the parent object (`self`, here) has a style defined for this
	 * draw state, propagate the state to its children. otherwise, don't.
	 *
	 * XXX: this is kind of a shitty hack and i don't like it. */
	if (self->style->available_styles & (1 << state)) {
		TAILQ_FOREACH(iter, &self->children, child)
			rtb_obj_draw(iter, state);
	} else {
		TAILQ_FOREACH(iter, &self->children, child)
			rtb_obj_draw(iter, RTB_DRAW_NORMAL);
	}

	LAYOUT_DEBUG_DRAW_BOX(self);
}

static int on_event(rtb_obj_t *self, const rtb_ev_t *e)
{
	switch (e->type) {
	case RTB_MOUSE_ENTER:
	case RTB_MOUSE_LEAVE:
	case RTB_DRAG_ENTER:
	case RTB_DRAG_LEAVE:
		/* eat these events because we already handle propagation
		 * in mouse.c as we dispatch them. */
		return 1;
	}

	return 0;
}

static void realize(rtb_obj_t *self, rtb_obj_t *parent,
		rtb_win_t *window)
{
	rtb_obj_t *iter;

	self->type = rtb_type_ref(window, NULL, "net.illest.rutabaga.object");

	self->layout_cb(self);

	TAILQ_FOREACH(iter, &self->children, child)
		self->attach_child(self, iter);
}

static void attach_child(rtb_obj_t *self, rtb_obj_t *child)
{
	rtb_obj_realize(child, self, self->surface, self->window);
}

static void mark_dirty(rtb_obj_t *self)
{
	struct rtb_render_context *render_ctx = &self->surface->render_ctx;

	if (!self->surface || self->surface->surface_state == RTB_SURFACE_INVALID
			|| self->render_entry.tqe_next || self->render_entry.tqe_prev)
		return;

	TAILQ_INSERT_TAIL(&render_ctx->queues.next_frame, self, render_entry);
}

/**
 * public API
 */

int rtb_obj_deliver_event(rtb_obj_t *self, const rtb_ev_t *e)
{
	int ret;

	/* objects should not receive events before they've had their
	 * realize() callback called. */
	if (self->state == RTB_STATE_UNREALIZED)
		return 0;

	switch (e->type) {
	case RTB_MOUSE_ENTER:
	case RTB_MOUSE_LEAVE:
		if (self->style->available_styles & RTB_STYLE_HOVER)
			rtb_obj_mark_dirty(self);
		break;

	case RTB_MOUSE_DOWN:
	case RTB_MOUSE_UP:
	case RTB_DRAG_DROP:
		if (self->style->available_styles & RTB_STYLE_FOCUS)
			rtb_obj_mark_dirty(self);
		break;
	}

	ret = self->event_cb(self, e);

	if (self->flags & RTB_OBJ_FLAG_EVENT_SNOOP)
		return rtb_handle(self, e) || ret;
	else
		return ret || rtb_handle(self, e);
}

void rtb_obj_draw(rtb_obj_t *self, rtb_draw_state_t state)
{
	rtb_win_t *window = self->window;

	if (self->visibility == RTB_FULLY_OBSCURED)
		return;

	/* we do this so that child widgets are drawn with their parent's state
	 * if their parent has a draw state. */
	if (state == RTB_DRAW_NORMAL) {
		if (window->focus == self)
			state = RTB_DRAW_FOCUS;
		else if (self->mouse_in &&
				!(window->mouse.buttons_down & RTB_MOUSE_BUTTON1_MASK))
			state = RTB_DRAW_HOVER;
	}

	self->draw_cb(self, state);
}

void rtb_obj_realize(rtb_obj_t *self, rtb_obj_t *parent,
		rtb_surface_t *surface, rtb_win_t *window)
{
	self->parent  = parent;
	self->surface = surface;
	self->window  = window;

	self->realize_cb(self, parent, window);

	if (!parent || parent->state != RTB_STATE_UNREALIZED)
		self->recalc_cb(self, parent, RTB_DIRECTION_LEAFWARD);

	self->state = RTB_STATE_REALIZED;
}

void rtb_obj_trigger_recalc(rtb_obj_t *self, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	self->recalc_cb(self, instigator, direction);
}

void rtb_obj_mark_dirty(rtb_obj_t *self)
{
	self->mark_dirty(self);
}

void rtb_obj_set_size_cb(rtb_obj_t *self, rtb_size_cb_t size_cb)
{
	self->size_cb = size_cb;
}

void rtb_obj_set_layout(rtb_obj_t *self, rtb_layout_cb_t layout_cb)
{
	self->layout_cb = layout_cb;
}

void rtb_obj_set_position_from_point(rtb_obj_t *self, rtb_pt_t *pos)
{
	self->x = floorf(pos->x);
	self->y = floorf(pos->y);
}

void rtb_obj_set_position(rtb_obj_t *self, float x, float y)
{
	rtb_pt_t positition = {x, y};
	rtb_obj_set_position_from_point(self, &positition);
}

void rtb_obj_set_size(rtb_obj_t *self, struct rtb_size *sz)
{
	self->w = sz->w;
	self->h = sz->h;
}

int rtb_obj_in_tree(rtb_obj_t *root, rtb_obj_t *leaf)
{
	for (; leaf; leaf = leaf->parent)
		if (leaf == root)
			return 1;

	return 0;
}

void rtb_obj_add_child(rtb_obj_t *self, rtb_obj_t *child,
		rtb_child_add_loc_t where)
{
	assert(child->draw_cb);
	assert(child->event_cb);
	assert(child->realize_cb);
	assert(child->layout_cb);
	assert(child->size_cb);
	assert(child->recalc_cb);

	if (where == RTB_ADD_HEAD)
		TAILQ_INSERT_HEAD(&self->children, child, child);
	else
		TAILQ_INSERT_TAIL(&self->children, child, child);

	if (self->window) {
		self->attach_child(self, child);
		self->recalc_cb(self, child, RTB_DIRECTION_ROOTWARD);
	}
}

void rtb_obj_remove_child(rtb_obj_t *self, rtb_obj_t *child)
{
	TAILQ_REMOVE(&self->children, child, child);

	if (!self->window)
		return;

	if (child->mouse_in) {
		if (self->window->mouse.buttons_down) {
			struct rtb_mouse_button *b;
			int i;

			for (i = 0; i < RTB_MOUSE_BUTTON_MAX + 1; i++) {
				b = &self->window->mouse.button[i];
				if (rtb_obj_in_tree(child, b->target))
					b->target = self;
			}
		}

		self->window->mouse.object_underneath = self;
	}

	child->parent = NULL;
	child->style  = NULL;
	child->state  = RTB_STATE_UNREALIZED;

	self->recalc_cb(self, NULL, RTB_DIRECTION_LEAFWARD);
}

static struct rtb_object_implementation default_impl = {
	.draw_cb      = draw,
	.event_cb     = on_event,
	.realize_cb   = realize,
	.layout_cb    = rtb_layout_hpack_left,
	.size_cb      = rtb_size_self,
	.recalc_cb    = recalculate,
	.attach_child = attach_child,
	.mark_dirty   = mark_dirty
};

int rtb_obj_init(rtb_obj_t *self, struct rtb_object_implementation *impl)
{
	struct rtb_object_implementation *obj_impl = &self->impl;

	memset(self, 0, sizeof(*self));
	TAILQ_INIT(&self->children);

	if (impl != obj_impl)
		(*obj_impl) = default_impl;

	(*impl) = default_impl;

	self->metatype    = RTB_TYPE_ATOM;
	self->style       = NULL;
	self->state       = RTB_STATE_UNREALIZED;

	self->outer_pad.x = RTB_DEFAULT_OUTER_XPAD;
	self->outer_pad.y = RTB_DEFAULT_OUTER_YPAD;

	self->inner_pad.x = RTB_DEFAULT_INNER_XPAD;
	self->inner_pad.y = RTB_DEFAULT_INNER_YPAD;

	self->visibility  = RTB_UNOBSCURED;
	self->window      = NULL;

	VECTOR_INIT(&self->handlers, &stdlib_allocator, 1);

	LAYOUT_DEBUG_INIT();

	return 0;
}

void rtb_obj_fini(rtb_obj_t *self)
{
	VECTOR_FREE(&self->handlers);
	rtb_type_unref(self->type);
}
