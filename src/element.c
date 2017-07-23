/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
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

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>
#include <rutabaga/layout.h>
#include <rutabaga/window.h>
#include <rutabaga/stylequad.h>
#include <rutabaga/render.h>

#include <rutabaga/style.h>
#include <rutabaga/atom.h>

#include <rutabaga/event.h>
#include <rutabaga/mouse.h>

#include "rtb_private/stdlib-allocator.h"
#include "rtb_private/layout-debug.h"

#include "wwrl/vector.h"

/**
 * state machine
 */

#define FOCUSED_P(elem) (elem->window->focus == elem)

static int
change_state(struct rtb_element *self, rtb_elem_state_t state)
{
	switch (self->state) {
	default:
		if (self->state == state)
			return 0;

		self->state = state;
		self->restyle(self);
		break;

	case RTB_STATE_UNATTACHED:
		self->state = state;
		break;
	}

	return 0;
}

static void
transition_mouse_enter(struct rtb_element *self,
		const struct rtb_mouse_event *ev)
{
	struct rtb_mouse *m = &self->window->mouse;

	if (m->buttons_down & RTB_MOUSE_BUTTON1_MASK) {
		if (m->button[RTB_MOUSE_BUTTON1].target == self) {
			if (FOCUSED_P(self))
				change_state(self, RTB_STATE_FOCUS_ACTIVE);
			else
				change_state(self, RTB_STATE_ACTIVE);
		}
	} else if (FOCUSED_P(self))
		change_state(self, RTB_STATE_FOCUS_HOVER);
	else
		change_state(self, RTB_STATE_HOVER);
}

static void
transition_mouse_leave(struct rtb_element *self,
		const struct rtb_mouse_event *ev)
{
	if (FOCUSED_P(self))
		change_state(self, RTB_STATE_FOCUS);
	else
		change_state(self, RTB_STATE_NORMAL);
}

static void
transition_mouse_down(struct rtb_element *self,
		const struct rtb_mouse_event *ev)
{
	if (ev->button != RTB_MOUSE_BUTTON1)
		return;

	if (FOCUSED_P(self))
		change_state(self, RTB_STATE_FOCUS_ACTIVE);
	else
		change_state(self, RTB_STATE_ACTIVE);
}

static void
transition_mouse_up(struct rtb_element *self,
		const struct rtb_mouse_event *ev)
{
	if (self->mouse_in)
		transition_mouse_enter(self, ev);
	else
		transition_mouse_leave(self, ev);
}

static void
transition_focus(struct rtb_element *self)
{
	switch (self->state) {
	case RTB_STATE_HOVER:
		change_state(self, RTB_STATE_FOCUS_HOVER);
		break;

	case RTB_STATE_ACTIVE:
		change_state(self, RTB_STATE_FOCUS_ACTIVE);
		break;

	default:
		change_state(self, RTB_STATE_FOCUS);
		break;
	}
}

static void
transition_unfocus(struct rtb_element *self)
{
	switch (self->state) {
	case RTB_STATE_FOCUS_HOVER:
		change_state(self, RTB_STATE_HOVER);
		break;

	case RTB_STATE_FOCUS_ACTIVE:
		change_state(self, RTB_STATE_ACTIVE);
		break;

	default:
		change_state(self, RTB_STATE_NORMAL);
		break;
	}
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
	rtb_rect_update_points_from_size(&self->rect);

	self->inner_rect.x  = self->x  + self->outer_pad.x;
	self->inner_rect.y  = self->y  + self->outer_pad.y;
	self->inner_rect.x2 = self->x2 - self->outer_pad.x;
	self->inner_rect.y2 = self->y2 - self->outer_pad.y;
	rtb_rect_update_size_from_points(&self->inner_rect);

	rtb_stylequad_update_geometry(&self->stylequad, &self->rect);

	switch (direction) {
	case RTB_DIRECTION_ROOTWARD:
		if (!reflow_rootward(self, instigator, RTB_DIRECTION_ROOTWARD))
			return 0;
		break;

	case RTB_DIRECTION_LEAFWARD:
		reflow_leafward(self, instigator, RTB_DIRECTION_LEAFWARD);
		break;
	}

	return 1;
}

/**
 * styling
 */

static void
reload_style(struct rtb_element *self)
{
	const struct rtb_style_property_definition *prop;
	int need_reflow = 0;

	/* layout-related properties trigger a reflow if they change, so
	 * we'll handle them first. */

#define ASSIGN_LAYOUT_FLOAT(pname, dest) do {                         \
	prop = rtb_style_query_prop(self,                                 \
			pname, RTB_STYLE_PROP_FLOAT, 0);                          \
	if (!prop)                                                        \
		break;                                                        \
	if (self->dest != prop->flt                                       \
			&& self->window->state != RTB_STATE_UNATTACHED) {         \
		self->dest = prop->flt;                                       \
		need_reflow = 1;                                              \
	} else                                                            \
		self->dest = prop->flt;                                       \
} while (0)

	ASSIGN_LAYOUT_FLOAT("min-width", min_size.w);
	ASSIGN_LAYOUT_FLOAT("min-height", min_size.h);

#undef ASSIGN_LAYOUT_FLOAT

#define LOAD_PROP(name, type, member, load_func)                      \
	if ((prop = rtb_style_query_prop(self, name, type, 0))            \
			&& !load_func(&self->stylequad, &prop->member))           \

#define LOAD_COLOR(name, load_func)                                   \
		LOAD_PROP(name, RTB_STYLE_PROP_COLOR, color, load_func) {     \
			rtb_elem_mark_dirty(self);                                \
		}

#define LOAD_TEXTURE(name, load_func)                                 \
		LOAD_PROP(name, RTB_STYLE_PROP_TEXTURE, texture, load_func) { \
			rtb_elem_mark_dirty(self);                                \
		}

	LOAD_COLOR("background-color", rtb_stylequad_set_background_color);
	LOAD_COLOR("border-color", rtb_stylequad_set_border_color);

	LOAD_TEXTURE("border-image", rtb_stylequad_set_border_image);
	LOAD_TEXTURE("background-image", rtb_stylequad_set_background_image);

#undef LOAD_TEXTURE
#undef LOAD_COLOR
#undef LOAD_PROP

	if (need_reflow)
		rtb_elem_reflow_rootward(self);
}

static void
restyle(struct rtb_element *self)
{
	struct rtb_element *iter;

	assert(self->window->state != RTB_STATE_UNATTACHED);

	if (!self->style)
		self->style = rtb_style_for_element(self, self->window->style_list);

	reload_style(self);

	TAILQ_FOREACH(iter, &self->children, child)
		iter->restyle(iter);
}

/**
 * misc implementation
 */

static void
draw(struct rtb_element *self)
{
	rtb_stylequad_draw_on_element(&self->stylequad, self);
	rtb_elem_draw_children(self);
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

	change_state(self, RTB_STATE_NORMAL);
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
	struct rtb_surface *surface = self->surface;

	self = rtb_elem_nearest_clearable(self);

	if (!surface || surface->surface_state == RTB_SURFACE_INVALID
			|| self->render_entry.tqe_next || self->render_entry.tqe_prev)
		return;

	TAILQ_INSERT_TAIL(&surface->render_queue, self, render_entry);
	rtb_elem_mark_dirty(RTB_ELEMENT(surface));
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
	ret = rtb_handle(self, e) || ret;

	switch (e->type) {
	case RTB_MOUSE_ENTER:
		transition_mouse_enter(self, RTB_EVENT_AS(e, rtb_mouse_event));
		break;

	case RTB_MOUSE_LEAVE:
		transition_mouse_leave(self, RTB_EVENT_AS(e, rtb_mouse_event));
		break;

	case RTB_FOCUS:
		transition_focus(self);
		break;

	case RTB_UNFOCUS:
		transition_unfocus(self);
		break;

	case RTB_MOUSE_DOWN:
		transition_mouse_down(self, RTB_EVENT_AS(e, rtb_mouse_event));
		break;

	case RTB_MOUSE_UP:
	case RTB_DRAG_DROP:
		transition_mouse_up(self, RTB_EVENT_AS(e, rtb_mouse_event));
		break;
	}

	return ret;
}

void
rtb_elem_draw_children(struct rtb_element *self)
{
	struct rtb_element *iter;

	TAILQ_FOREACH(iter, &self->children, child)
		rtb_elem_draw(iter, 0);
}

void
rtb_elem_draw(struct rtb_element *self, int clear_first)
{
	if (self->visibility == RTB_FULLY_OBSCURED)
		return;

	rtb_render_push(self);
	if (clear_first)
		rtb_render_clear(self);

	self->draw(self);
	LAYOUT_DEBUG_DRAW_BOX(self);

	rtb_render_pop(self);
}

int
rtb_elem_is_clearable(struct rtb_element *self)
{
	struct rtb_surface *surface = self->surface;

	/* XXX: shouldn't depend on stylequad like this */

	for (self = self->parent;
			self != RTB_ELEMENT(surface); self = self->parent)
		if (self->stylequad.properties.bg_color)
			return 0;

	return 1;
}

struct rtb_element *
rtb_elem_nearest_clearable(struct rtb_element *self)
{
	struct rtb_surface *surface = self->surface;

	for (; self != RTB_ELEMENT(surface); self = self->parent)
		if (rtb_elem_is_clearable(self))
			return self;

	return RTB_ELEMENT(surface);
}

void
rtb_elem_trigger_reflow(struct rtb_element *self, struct rtb_element *instigator,
		rtb_ev_direction_t direction)
{
	self->reflow(self, instigator, direction);
}

void
rtb_elem_reflow_leafward(struct rtb_element *self)
{
	rtb_elem_trigger_reflow(self, NULL, RTB_DIRECTION_LEAFWARD);
}

void
rtb_elem_reflow_rootward(struct rtb_element *self)
{
	rtb_elem_trigger_reflow(self->parent, self, RTB_DIRECTION_ROOTWARD);
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

	/* XXX: remove from renderqueue if we're marked for redraw */

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
rtb_elem_init(struct rtb_element *self)
{
	memset(self, 0, sizeof(*self));
	TAILQ_INIT(&self->children);

	self->impl = base_impl;

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

	rtb_stylequad_init(&self->stylequad);

	LAYOUT_DEBUG_INIT();

	return 0;
}

void
rtb_elem_fini(struct rtb_element *self)
{
	rtb_stylequad_fini(&self->stylequad);
	VECTOR_FREE(&self->handlers);
	rtb_type_unref(self->type);
}
