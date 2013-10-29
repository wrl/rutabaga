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

#pragma once

#include "rutabaga/types.h"
#include "rutabaga/atom.h"
#include "rutabaga/event.h"
#include "rutabaga/geometry.h"
#include "rutabaga/stylequad.h"

#include "bsd/queue.h"
#include "wwrl/vector.h"

#define RTB_ELEMENT(x) RTB_UPCAST(x, rtb_element)
#define RTB_ELEMENT_AS(x, type) RTB_DOWNCAST(x, type, rtb_element)

typedef enum {
	/* all events, regardless of whether they are already handled by
	 * the element in question, are passed to client code after the
	 * element's event_cb has been run. */
	RTB_ELEM_EVENT_SNOOP = 0x01,

	RTB_ELEM_NO_FOCUS    = 0,
	RTB_ELEM_CLICK_FOCUS = 0x02,
	RTB_ELEM_TAB_FOCUS   = 0x04
} rtb_elem_flags_t;

typedef enum {
	RTB_STATE_UNATTACHED = 0,

	RTB_STATE_NORMAL,
	RTB_STATE_HOVER,
	RTB_STATE_ACTIVE,

	RTB_STATE_FOCUS,
	RTB_STATE_FOCUS_HOVER,
	RTB_STATE_FOCUS_ACTIVE
} rtb_elem_state_t;

/**
 * element implementation
 */

typedef void (*rtb_elem_cb_t)
	(struct rtb_element *);

typedef int (*rtb_elem_cb_internal_event_t)
	(struct rtb_element *, const struct rtb_event *event);

typedef void (*rtb_elem_cb_state_change_t)
	(struct rtb_element *,
	 struct rtb_element *parent, struct rtb_window *window);

typedef void (*rtb_elem_cb_size_t)
	(struct rtb_element *,
	 const struct rtb_size *avail, struct rtb_size *want);

typedef int (*rtb_elem_cb_recalc_t)
	(struct rtb_element *elem,
	 struct rtb_element *instigator, rtb_ev_direction_t direction);

typedef void (*rtb_elem_cb_child_state_t)
	(struct rtb_element *elem, struct rtb_element *child);

struct rtb_element_implementation {
	/**
	 * rtb_element_implementation.draw
	 *
	 * called when the element should redraw itself
	 */
	rtb_elem_cb_t draw;

	/**
	 * rtb_element_implementation.on_event
	 *
	 * called when the element should handle an event.
	 * distinct from rtb_event_cb_t because of the lack of a userdata
	 * pointer.
	 *
	 * a return value of 0 indicates that the element doesn't wish to
	 * handle the event, and it will be propagated rootward in the tree
	 * until. a return value of 1 indicates that the element has handled
	 * the event, and no propagation in the tree will take place.
	 */
	rtb_elem_cb_internal_event_t on_event;


	/**
	 * rtb_element_implementation.size_cb
	 *
	 * called when the element should report its desired size.
	 */
	rtb_elem_cb_size_t size_cb;

	/**
	 * rtb_element_implementation.layout_cb
	 *
	 * called when the element should layout its children.
	 */
	rtb_elem_cb_t layout_cb;


	/**
	 * rtb_element_implementation.attached
	 *
	 * called when the element is attached to the tree.
	 */
	rtb_elem_cb_state_change_t attached;

	/**
	 * rtb_element_implementation.detached
	 *
	 * called when the element is detached from the tree.
	 */
	rtb_elem_cb_state_change_t detached;

	/**
	 * rtb_element_implementation.child_attached
	 *
	 * called when `child` is attached directly under `elem`.
	 *
	 * it is not propagated up the tree. for example, let's consider
	 * two elements, `a` and `b`, and that `b` is a child element of `a`.
	 * if a third element, `c`, is added under `b`, then only `b`'s
	 * child_attached callback will be called. (it will not propagate rootward
	 * to `a`).
	 */
	rtb_elem_cb_child_state_t child_attached;

	/**
	 * rtb_element_implementation.child_detached
	 *
	 * inverse of child_attached.
	 */
	rtb_elem_cb_child_state_t child_detached;


	/**
	 * rtb_element_implementation.restyle
	 *
	 * called when a parent element's style has changed.
	 */
	rtb_elem_cb_t restyle;

	/**
	 * rtb_element_implementation.reflow
	 *
	 * called when an element needs to respond to a layout change
	 * caused by an element above it somewhere in the tree, generally
	 * by a containing element changing size.
	 *
	 * should return 1 if recalculation was necessary, 0 if none was,
	 * and -1 in an exceptional condition (for example, if an element
	 * is less than 1 pixel square in size).
	 *
	 * the exceptional condition will not be handled or propagated up
	 * the tree, but can be useful for debugging.
	 */
	rtb_elem_cb_recalc_t reflow;


	/**
	 * rtb_element_implementation.mark_dirty
	 *
	 * called when a redraw has been requested of an element. generally,
	 * an element will add itself to its surface's renderqueue to be
	 * repainted on the next frame, but in special cases (for example,
	 * the implementation of a surface itself), this behavior can be
	 * changed.
	 */
	rtb_elem_cb_t mark_dirty;
};

struct rtb_element_event {
	RTB_INHERIT(rtb_event);
	struct rtb_element *element;
};

/**
 * and finally rtb_element itself
 */

struct rtb_element {
	RTB_INHERIT(rtb_type_atom);

	/* public *********************************/
	RTB_INHERIT_AS(rtb_rect, rect);
	struct rtb_size min_size;
	struct rtb_size max_size;

	rtb_elem_flags_t flags;

	RTB_INHERIT_AS(rtb_element_implementation, impl);

	struct rtb_style *style;

	/* XXX: should this stuff be in rtb_style_t? */
	rtb_alignment_t align;
	struct rtb_padding outer_pad;
	struct rtb_padding inner_pad;

	TAILQ_HEAD(children, rtb_element) children;

	/* private ********************************/
	rtb_elem_state_t state;
	struct rtb_rect inner_rect;
	rtb_visibility_t visibility;
	struct rtb_stylequad stylequad;

	int mouse_in;

	struct rtb_element *parent;
	struct rtb_window  *window;
	struct rtb_surface *surface;

	VECTOR(handlers, struct rtb_event_handler) handlers;
	TAILQ_ENTRY(rtb_element) child;
	TAILQ_ENTRY(rtb_element) render_entry;
};

int rtb_elem_deliver_event(struct rtb_element *, const struct rtb_event *e);
void rtb_elem_draw_children(struct rtb_element *);
void rtb_elem_draw(struct rtb_element *);

void rtb_elem_mark_dirty(struct rtb_element *);
void rtb_elem_trigger_recalc(struct rtb_element *,
		struct rtb_element *instigator, rtb_ev_direction_t direction);

void rtb_elem_set_size_cb(struct rtb_element *, rtb_elem_cb_size_t size_cb);
void rtb_elem_set_layout(struct rtb_element *, rtb_elem_cb_t layout_cb);
void rtb_elem_set_position_from_point(struct rtb_element *, struct rtb_point *);
void rtb_elem_set_position(struct rtb_element *, float x, float y);
void rtb_elem_set_size(struct rtb_element *, struct rtb_size *);

int rtb_elem_is_in_tree(struct rtb_element *root, struct rtb_element *leaf);
void rtb_elem_add_child(struct rtb_element *parent, struct rtb_element *child,
		rtb_child_add_loc_t where);
void rtb_elem_remove_child(struct rtb_element *, struct rtb_element *child);

int rtb_elem_init(struct rtb_element *, struct rtb_element_implementation *impl);
void rtb_elem_fini(struct rtb_element *);
