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

#include "bsd/queue.h"

#define RTB_OBJECT(x) RTB_AS_TYPE(x, rtb_object)

#define RTB_EVENT_HANDLERS_PER_OBJECT 32

typedef struct rtb_object_implementation rtb_obj_impl_t;

typedef enum {
	/* all events, regardless of whether they are already handled by
	 * the object in question, are passed to client code after the
	 * object's event_cb has been run. */
	RTB_OBJ_FLAG_EVENT_SNOOP = 0x01,
} rtb_obj_flags_t;

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

struct rtb_object {
	RTB_INHERIT(rtb_type_atom);

	/* public *********************************/
	RTB_INHERIT_AS(rtb_point, pt);  /* provides x, y */
	RTB_INHERIT_AS(rtb_size, size); /* provides w, h */
	struct rtb_size min_size;

	rtb_obj_flags_t flags;

	RTB_INHERIT_AS(rtb_object_implementation, impl);

	rtb_style_t *style;

	/* XXX: should this stuff be in rtb_style_t? */
	rtb_alignment_t align;
	struct rtb_padding outer_pad;
	struct rtb_padding inner_pad;

	TAILQ_HEAD(children, rtb_object) children;

	/* private ********************************/
	int mouse_in;
	rtb_rect_t rect;
	rtb_visibility_t visibility;
	rtb_obj_state_t state;

	rtb_obj_t *parent;
	rtb_win_t *window;
	rtb_surface_t *surface;

	struct rtb_event_handler handlers[RTB_EVENT_HANDLERS_PER_OBJECT];
	TAILQ_ENTRY(rtb_object) child;
	TAILQ_ENTRY(rtb_object) render_entry;
};

int rtb_obj_deliver_event(rtb_obj_t *, const rtb_ev_t *e);
void rtb_obj_draw(rtb_obj_t *, rtb_draw_state_t state);
void rtb_obj_realize(rtb_obj_t *, rtb_obj_t *parent,
		rtb_surface_t *surface, rtb_win_t *window);

void rtb_obj_mark_dirty(rtb_obj_t *);
void rtb_obj_trigger_recalc(rtb_obj_t *, rtb_obj_t *instigator,
		rtb_event_direction_t direction);

void rtb_obj_set_size_cb(rtb_obj_t *, rtb_size_cb_t size_cb);
void rtb_obj_set_layout(rtb_obj_t *, rtb_layout_cb_t layout_cb);
void rtb_obj_set_position_from_point(rtb_obj_t *, rtb_pt_t *pos);
void rtb_obj_set_position(rtb_obj_t *, float x, float y);
void rtb_obj_set_size(rtb_obj_t *, struct rtb_size *sz);

int rtb_obj_in_tree(rtb_obj_t *root, rtb_obj_t *leaf);
void rtb_obj_add_child(rtb_obj_t *parent, rtb_obj_t *child,
		rtb_child_add_loc_t where);
void rtb_obj_remove_child(rtb_obj_t *self, rtb_obj_t *child);

int rtb_obj_init(rtb_obj_t *obj, struct rtb_object_implementation *impl);
void rtb_obj_fini(rtb_obj_t *self);
