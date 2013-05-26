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

#include "bsd/queue.h"
#include "wwrl/vector.h"

#define RTB_OBJECT(x) RTB_UPCAST(x, rtb_object)
#define RTB_OBJECT_AS(x, type) RTB_DOWNCAST(x, type, rtb_object)

typedef enum {
	/* all events, regardless of whether they are already handled by
	 * the object in question, are passed to client code after the
	 * object's event_cb has been run. */
	RTB_OBJ_FLAG_EVENT_SNOOP = 0x01,
} rtb_obj_flags_t;

typedef enum {
	RTB_STATE_UNREALIZED = 0,
	RTB_STATE_REALIZED   = 1
} rtb_obj_state_t;

/**
 * object implementation
 */

typedef void (*rtb_draw_cb_t)(struct rtb_object *obj, rtb_draw_state_t state);
typedef int  (*rtb_internal_event_cb_t)(struct rtb_object *obj,
		const struct rtb_event *event);
typedef void (*rtb_realize_cb_t)(struct rtb_object *obj,
		struct rtb_object *parent, struct rtb_window *window);
typedef void (*rtb_layout_cb_t)(struct rtb_object *);
typedef void (*rtb_size_cb_t)(struct rtb_object *obj,
		const struct rtb_size *avail, struct rtb_size *want);
typedef void (*rtb_recalc_cb_t)(struct rtb_object *obj,
		struct rtb_object *instigator, rtb_ev_direction_t direction);
typedef void (*rtb_attach_child_cb_t)(struct rtb_object *obj,
		struct rtb_object *child);
typedef void (*rtb_mark_dirty_cb_t)(struct rtb_object *);

struct rtb_object_implementation {
	rtb_size_cb_t size_cb;
	rtb_layout_cb_t layout_cb;
	rtb_draw_cb_t draw_cb;
	rtb_realize_cb_t realize_cb;
	rtb_internal_event_cb_t event_cb;
	rtb_recalc_cb_t recalc_cb;
	rtb_attach_child_cb_t attach_child;
	rtb_mark_dirty_cb_t mark_dirty;
};

/**
 * and finally rtb_object itself
 */

struct rtb_object {
	RTB_INHERIT(rtb_type_atom);

	/* public *********************************/
	RTB_INHERIT_AS(rtb_rect, rect);
	struct rtb_size min_size;
	struct rtb_size max_size;

	rtb_obj_flags_t flags;

	RTB_INHERIT_AS(rtb_object_implementation, impl);

	struct rtb_style *style;

	/* XXX: should this stuff be in rtb_style_t? */
	rtb_alignment_t align;
	struct rtb_padding outer_pad;
	struct rtb_padding inner_pad;

	TAILQ_HEAD(children, rtb_object) children;

	/* private ********************************/
	rtb_obj_state_t state;
	struct rtb_rect inner_rect;
	rtb_visibility_t visibility;

	int mouse_in;

	struct rtb_object  *parent;
	struct rtb_window  *window;
	struct rtb_surface *surface;

	VECTOR(handlers, struct rtb_event_handler) handlers;
	TAILQ_ENTRY(rtb_object) child;
	TAILQ_ENTRY(rtb_object) render_entry;
};

int rtb_obj_deliver_event(struct rtb_object *, const struct rtb_event *e);
void rtb_obj_draw(struct rtb_object *, rtb_draw_state_t state);
void rtb_obj_realize(struct rtb_object *, struct rtb_object *parent,
		struct rtb_surface *surface, struct rtb_window *window);

void rtb_obj_mark_dirty(struct rtb_object *);
void rtb_obj_trigger_recalc(struct rtb_object *,
		struct rtb_object *instigator, rtb_ev_direction_t direction);

void rtb_obj_set_size_cb(struct rtb_object *, rtb_size_cb_t size_cb);
void rtb_obj_set_layout(struct rtb_object *, rtb_layout_cb_t layout_cb);
void rtb_obj_set_position_from_point(struct rtb_object *, struct rtb_point *);
void rtb_obj_set_position(struct rtb_object *, float x, float y);
void rtb_obj_set_size(struct rtb_object *, struct rtb_size *);

int rtb_obj_in_tree(struct rtb_object *root, struct rtb_object *leaf);
void rtb_obj_add_child(struct rtb_object *parent, struct rtb_object *child,
		rtb_child_add_loc_t where);
void rtb_obj_remove_child(struct rtb_object *, struct rtb_object *child);

int rtb_obj_init(struct rtb_object *, struct rtb_object_implementation *impl);
void rtb_obj_fini(struct rtb_object *);
