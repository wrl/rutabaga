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
#include "rutabaga/element.h"
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
 * state machine
 */

static rtb_draw_state_t
draw_state_for_elem_state(unsigned int state)
{
	switch (state) {
	case RTB_STATE_NORMAL: return RTB_DRAW_NORMAL;
	case RTB_STATE_HOVER:  return RTB_DRAW_HOVER;
	case RTB_STATE_ACTIVE: return RTB_DRAW_ACTIVE;
	case RTB_STATE_FOCUS:  return RTB_DRAW_FOCUS;
	}

	return RTB_DRAW_NORMAL;
}

int
rtb_elem_change_state(struct rtb_element *self, unsigned int state)
{
	switch (self->state) {
	case RTB_STATE_UNATTACHED:
		self->state = state;
		break;

	default:
		if (rtb_style_elem_has_properties_for_state(self,
					draw_state_for_elem_state(self->state))
				&& rtb_style_elem_has_properties_for_state(self,
					draw_state_for_elem_state(state))) {
			self->state = state;
			rtb_elem_mark_dirty(self);
		} else
			self->state = state;

		break;
	}

	return 0;
}

static void
transition_mouse_enter(struct rtb_element *self)
{
	if (self->state & RTB_STATE_FOCUS)
		rtb_elem_change_state(self, RTB_STATE_HOVER | RTB_STATE_FOCUS);
	else
		rtb_elem_change_state(self, RTB_STATE_HOVER);
}

static void
transition_mouse_leave(struct rtb_element *self)
{
	if (self->state & RTB_STATE_FOCUS)
		rtb_elem_change_state(self, RTB_STATE_FOCUS);
	else
		rtb_elem_change_state(self, RTB_STATE_NORMAL);
}

static void
transition_mouse_down(struct rtb_element *self)
{
	rtb_elem_change_state(self, RTB_STATE_ACTIVE);
}

static void
transition_mouse_up(struct rtb_element *self)
{
	transition_mouse_enter(self);
}

static void
transition_focus(struct rtb_element *self)
{
	rtb_elem_change_state(self, RTB_STATE_FOCUS);
}

static void
transition_unfocus(struct rtb_element *self)
{
	rtb_elem_change_state(self, RTB_STATE_NORMAL);
}

/**
 * reflow
 */

static int
reflow_rootward(struct rtb_element *self,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	struct rtb_element *iter;
	struct rtb_size inst_old_size = {
		instigator->w,
		instigator->h
	};

	self->layout_cb(self);

	/* don't pass the reflow any further rootward if the element's
	 * size hasn't changed as a result of it. */
	if ((instigator->w == inst_old_size.w &&
	     instigator->h == inst_old_size.h))
		return 0;

	TAILQ_FOREACH(iter, &self->children, child)
		iter->reflow(iter, self, RTB_DIRECTION_LEAFWARD);

	if (self->parent)
		self->parent->reflow(self->parent, self, direction);

	rtb_elem_mark_dirty(self);
	return 1;
}

static void
reflow_leafward(struct rtb_element *self,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	struct rtb_element *iter;

	self->layout_cb(self);

	TAILQ_FOREACH(iter, &self->children, child)
		iter->reflow(iter, self, direction);
}

static int
reflow(struct rtb_element *self,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	switch (direction) {
	case RTB_DIRECTION_ROOTWARD:
		if (!reflow_rootward(self, instigator, RTB_DIRECTION_ROOTWARD))
			return 0;
		break;

	case RTB_DIRECTION_LEAFWARD:
		reflow_leafward(self, instigator, RTB_DIRECTION_LEAFWARD);
		break;
	}

	self->x2 = self->x + self->w;
	self->y2 = self->y + self->h;

	self->inner_rect.x  = self->x  + self->outer_pad.x;
	self->inner_rect.y  = self->y  + self->outer_pad.y;
	self->inner_rect.x2 = self->x2 - self->outer_pad.x;
	self->inner_rect.y2 = self->y2 - self->outer_pad.y;

	return 1;
}

static void
restyle(struct rtb_element *self)
{
	struct rtb_element *iter;

	assert(self->window->state != RTB_STATE_UNATTACHED);

	if (!self->style)
		self->style = rtb_style_for_element(self, self->window->style_list);

	TAILQ_FOREACH(iter, &self->children, child)
		iter->restyle(iter);
}

/**
 * misc implementation
 */

static void
draw(struct rtb_element *self, rtb_draw_state_t state)
{
	rtb_elem_draw_children(self, RTB_DRAW_NORMAL);
}

static int
on_event(struct rtb_element *self, const struct rtb_event *e)
{
	switch (e->type) {
	case RTB_MOUSE_ENTER:
	case RTB_MOUSE_LEAVE:
	case RTB_DRAG_ENTER:
	case RTB_DRAG_LEAVE:
		/* eat these events because we already dispatch them up
		 * and down the tree in platform/mouse.c */
		return 1;
	}

	return 0;
}

static void
attached(struct rtb_element *self,
		struct rtb_element *parent, struct rtb_window *window)
{
	struct rtb_element *iter;

	self->parent = parent;
	self->window = window;

	self->type = rtb_type_ref(window, NULL, "net.illest.rutabaga.element");

	self->layout_cb(self);

	TAILQ_FOREACH(iter, &self->children, child)
		self->child_attached(self, iter);

	rtb_elem_change_state(self, RTB_STATE_NORMAL);
}

static void
detached(struct rtb_element *self,
		struct rtb_element *parent, struct rtb_window *window)
{
	return;
}

static void
child_attached(struct rtb_element *self, struct rtb_element *child)
{
	child->surface = self->surface;
	child->attached(child, self, self->window);
}

static void
child_detached(struct rtb_element *self, struct rtb_element *child)
{
	child->detached(child, self, self->window);
}

static void
mark_dirty(struct rtb_element *self)
{
	struct rtb_render_context *render_ctx = &self->surface->render_ctx;

	if (!self->surface || self->surface->surface_state == RTB_SURFACE_INVALID
			|| self->render_entry.tqe_next || self->render_entry.tqe_prev)
		return;

	TAILQ_INSERT_TAIL(&render_ctx->queues.next_frame, self, render_entry);
	rtb_elem_mark_dirty(RTB_ELEMENT(self->surface));
}

/**
 * public API
 */

int
rtb_elem_deliver_event(struct rtb_element *self, const struct rtb_event *e)
{
	int ret;

	/* elements should not receive events if they've not been attached
	 * into the tree yet. */
	if (self->state == RTB_STATE_UNATTACHED)
		return 0;

	ret = self->on_event(self, e);

	if (self->flags & RTB_ELEM_EVENT_SNOOP)
		ret = rtb_handle(self, e) || ret;
	else
		ret = ret || rtb_handle(self, e);

	switch (e->type) {
	case RTB_MOUSE_ENTER:
		transition_mouse_enter(self);
		break;

	case RTB_MOUSE_LEAVE:
		transition_mouse_leave(self);
		break;

	case RTB_FOCUS:
		transition_focus(self);
		break;

	case RTB_UNFOCUS:
		transition_unfocus(self);
		break;

	case RTB_MOUSE_DOWN:
		transition_mouse_down(self);
		break;

	case RTB_MOUSE_UP:
	case RTB_DRAG_DROP:
		transition_mouse_up(self);
		break;
	}

	return ret;
}

void
rtb_elem_draw_children(struct rtb_element *self, rtb_draw_state_t state)
{
	struct rtb_element *iter;

	TAILQ_FOREACH(iter, &self->children, child)
		rtb_elem_draw(iter, state);
}

void
rtb_elem_draw(struct rtb_element *self, rtb_draw_state_t state)
{
	struct rtb_window *window = self->window;

	if (self->visibility == RTB_FULLY_OBSCURED)
		return;

	/* we do this so that child widgets are drawn with their parent's state
	 * if their parent has a draw state. */
	switch (self->state) {
	case RTB_STATE_UNATTACHED:
		return;

	case RTB_STATE_FOCUS | RTB_STATE_HOVER:
	case RTB_STATE_HOVER:
		if (!(window->mouse.buttons_down & RTB_MOUSE_BUTTON1_MASK))
			state = RTB_DRAW_HOVER;
		break;

	case RTB_STATE_FOCUS:
		state = RTB_DRAW_FOCUS;
		break;

	case RTB_STATE_ACTIVE:
	case RTB_STATE_NORMAL:
	default:
		break;
	}

	self->draw(self, state);
	LAYOUT_DEBUG_DRAW_BOX(self);
}

void
rtb_elem_trigger_recalc(struct rtb_element *self, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	self->reflow(self, instigator, direction);
}

void
rtb_elem_mark_dirty(struct rtb_element *self)
{
	self->mark_dirty(self);
}

void
rtb_elem_set_size_cb(struct rtb_element *self, rtb_elem_cb_size_t size_cb)
{
	self->size_cb = size_cb;
}

void
rtb_elem_set_layout(struct rtb_element *self, rtb_elem_cb_t layout_cb)
{
	self->layout_cb = layout_cb;
}

void
rtb_elem_set_position_from_point(struct rtb_element *self,
		struct rtb_point *pos)
{
	self->x = floorf(pos->x);
	self->y = floorf(pos->y);
}

void
rtb_elem_set_position(struct rtb_element *self, float x, float y)
{
	struct rtb_point positition = {x, y};
	rtb_elem_set_position_from_point(self, &positition);
}

void
rtb_elem_set_size(struct rtb_element *self, struct rtb_size *sz)
{
	self->w = sz->w;
	self->h = sz->h;
}

int
rtb_elem_is_in_tree(struct rtb_element *root, struct rtb_element *leaf)
{
	for (; leaf; leaf = leaf->parent)
		if (leaf == root)
			return 1;

	return 0;
}

void
rtb_elem_add_child(struct rtb_element *self, struct rtb_element *child,
		rtb_child_add_loc_t where)
{
	assert(child->draw);
	assert(child->on_event);
	assert(child->layout_cb);
	assert(child->size_cb);
	assert(child->attached);
	assert(child->detached);
	assert(child->child_attached);
	assert(child->child_detached);
	assert(child->reflow);
	assert(child->restyle);
	assert(child->mark_dirty);

	if (where == RTB_ADD_HEAD)
		TAILQ_INSERT_HEAD(&self->children, child, child);
	else
		TAILQ_INSERT_TAIL(&self->children, child, child);

	if (self->window) {
		self->child_attached(self, child);

		if (self->window->state != RTB_STATE_UNATTACHED)
			self->restyle(self);

		self->reflow(self, child, RTB_DIRECTION_ROOTWARD);
	}
}

void
rtb_elem_remove_child(struct rtb_element *self, struct rtb_element *child)
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
				if (rtb_elem_is_in_tree(child, b->target))
					b->target = self;
			}
		}

		self->window->mouse.element_underneath = self;
	}

	self->child_detached(self, child);

	child->parent = NULL;
	child->style  = NULL;
	child->state  = RTB_STATE_UNATTACHED;

	self->reflow(self, NULL, RTB_DIRECTION_LEAFWARD);
}

static struct rtb_element_implementation base_impl = {
	.draw           = draw,
	.on_event       = on_event,

	.layout_cb      = rtb_layout_hpack_left,
	.size_cb        = rtb_size_self,

	.attached       = attached,
	.detached       = detached,

	.child_attached = child_attached,
	.child_detached = child_detached,

	.reflow         = reflow,
	.restyle        = restyle,

	.mark_dirty     = mark_dirty
};

int
rtb_elem_init(struct rtb_element *self,
		struct rtb_element_implementation *impl)
{
	struct rtb_element_implementation *elem_impl = &self->impl;

	memset(self, 0, sizeof(*self));
	TAILQ_INIT(&self->children);

	if (impl != elem_impl)
		(*elem_impl) = base_impl;

	(*impl) = base_impl;

	self->metatype    = RTB_TYPE_ATOM;
	self->style       = NULL;
	self->state       = RTB_STATE_UNATTACHED;

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

void
rtb_elem_fini(struct rtb_element *self)
{
	VECTOR_FREE(&self->handlers);
	rtb_type_unref(self->type);
}
