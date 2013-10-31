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

#include "rutabaga/rutabaga.h"
#include "rutabaga/layout.h"
#include "rutabaga/render.h"
#include "rutabaga/event.h"
#include "rutabaga/mouse.h"
#include "rutabaga/window.h"

#include "rutabaga/widgets/patchbay.h"

#include "private/util.h"

static struct rtb_element_implementation super;

#define SELF_FROM(elem) \
	struct rtb_patchbay_port *self = RTB_ELEMENT_AS(elem, rtb_patchbay_port)

/**
 * private utility functions
 */

static struct rtb_patchbay_patch *
get_patch(struct rtb_patchbay_port *from, struct rtb_patchbay_port *to)
{
	struct rtb_patchbay_patch *patch;

	TAILQ_FOREACH(patch, &from->patches, from_patch)
		if (patch->to == to)
			return patch;

	return NULL;
}

/**
 * event dispatching
 */

static void
dispatch_disconnect(struct rtb_patchbay_patch *patch)
{
	struct rtb_patchbay_event_disconnect ev = {
		.type = RTB_PATCHBAY_DISCONNECT,

		.patch     = patch,

		.from.node = patch->from->node,
		.from.port = patch->from,
		.to.node   = patch->to->node,
		.to.port   = patch->to
	};

	rtb_dispatch_raw(RTB_ELEMENT(patch->to), RTB_EVENT(&ev));
}

static void
dispatch_connect(struct rtb_patchbay_port *from, struct rtb_patchbay_port *to)
{
	struct rtb_patchbay_event_connect ev = {
		.type = RTB_PATCHBAY_CONNECT,

		.from.node = from->node,
		.from.port = from,
		.to.node   = to->node,
		.to.port   = to
	};

	rtb_dispatch_raw(RTB_ELEMENT(to), RTB_EVENT(&ev));
}

/**
 * event handling
 */

static void
handle_connection(struct rtb_patchbay_port *a, struct rtb_patchbay_port *b)
{
	struct rtb_patchbay_port *from, *to;
	struct rtb_patchbay_patch *patch;

	if (a->port_type == b->port_type)
		return;

	if (a->port_type == PORT_TYPE_INPUT) {
		from = b;
		to   = a;
	} else {
		from = a;
		to   = b;
	}

	if ((patch = get_patch(from, to)))
		dispatch_disconnect(patch);
	else
		dispatch_connect(from, to);
}

static void
start_patching(struct rtb_patchbay_port *self, struct rtb_mouse_event *e)
{
	struct rtb_patchbay *patchbay = self->node->patchbay;

	patchbay->patch_in_progress.from = self;
	patchbay->patch_in_progress.to   = NULL;

	patchbay->patch_in_progress.cursor.x = e->cursor.x;
	patchbay->patch_in_progress.cursor.y = e->cursor.y;
}

static void
stop_patching(struct rtb_patchbay_port *self)
{
	struct rtb_patchbay *patchbay = self->node->patchbay;

	patchbay->patch_in_progress.from = NULL;
	patchbay->patch_in_progress.to   = NULL;
}

static int
handle_drag(struct rtb_patchbay_port *self, const struct rtb_drag_event *e)
{
	struct rtb_patchbay *patchbay = self->node->patchbay;

	switch (e->button) {
	case RTB_MOUSE_BUTTON1: /* left button -- connection */
		switch (e->type) {
		case RTB_DRAG_START:
		case RTB_DRAGGING:
			patchbay->patch_in_progress.cursor.x = e->cursor.x;
			patchbay->patch_in_progress.cursor.y = e->cursor.y;
			return 1;

		case RTB_DRAG_ENTER:
			if (patchbay->patch_in_progress.from &&
					patchbay->patch_in_progress.from->port_type != self->port_type)
				patchbay->patch_in_progress.to = self;
			return 1;

		case RTB_DRAG_LEAVE:
			patchbay->patch_in_progress.to = NULL;
			return 1;

		case RTB_DRAG_DROP:
			if (e->target == (void *) self) {
				stop_patching(self);
				return 1;
			}

			if (rtb_is_type(self->type, RTB_TYPE_ATOM(e->target)))
				handle_connection((struct rtb_patchbay_port *) e->target, self);

			return 1;

		default:
			return 0;
		}

	case RTB_MOUSE_BUTTON2: /* right button -- disconnection */
	default:
		return 0;
	}
}

static int
handle_mouse(struct rtb_patchbay_port *self, struct rtb_mouse_event *e)
{
	if (e->button != RTB_MOUSE_BUTTON1)
		return 0;

	switch (e->type) {
	case RTB_MOUSE_DOWN:
		start_patching(self, e);
		return 1;

	case RTB_MOUSE_UP:
		stop_patching(self);
		return 1;
	}

	return 0;
}

/**
 * element implementation
 */

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(RTB_ELEMENT(self), parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay.port");
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_MOUSE_DOWN:
	case RTB_MOUSE_UP:
		if (handle_mouse(self, (struct rtb_mouse_event *) e)) {
			rtb_elem_mark_dirty(RTB_ELEMENT(self->node->patchbay));
			return 1;
		}

		break;

	case RTB_DRAG_ENTER:
	case RTB_DRAG_LEAVE:
	case RTB_DRAG_START:
	case RTB_DRAG_DROP:
	case RTB_DRAGGING:
		if (handle_drag(self, (struct rtb_drag_event *) e)) {
			rtb_elem_mark_dirty(RTB_ELEMENT(self->node->patchbay));
			return 1;
		}
	}

	return super.on_event(elem, e);
}

/**
 * public API
 */

int
rtb_patchbay_are_ports_connected(struct rtb_patchbay_port *a,
		struct rtb_patchbay_port *b)
{
	struct rtb_patchbay_port *from, *to;

	if (a->port_type == b->port_type)
		return 0;

	if (a->port_type == PORT_TYPE_INPUT) {
		from = b;
		to   = a;
	} else {
		from = a;
		to   = b;
	}

	if (get_patch(from, to))
		return 1;
	return 0;
}

void
rtb_patchbay_free_patch(struct rtb_patchbay *self,
		struct rtb_patchbay_patch *patch)
{
	TAILQ_REMOVE(&patch->to->patches,   patch, to_patch);
	TAILQ_REMOVE(&patch->from->patches, patch, from_patch);
	TAILQ_REMOVE(&self->patches, patch, patchbay_patch);

	free(patch);
	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

void
rtb_patchbay_disconnect_ports(struct rtb_patchbay *self,
		struct rtb_patchbay_port *a, struct rtb_patchbay_port *b)
{
	struct rtb_patchbay_patch *patch;
	struct rtb_patchbay_port *from, *to;

	if (a->port_type == b->port_type)
		return;

	if (a->port_type == PORT_TYPE_INPUT) {
		from = b;
		to   = a;
	} else {
		from = a;
		to   = b;
	}

	if ((patch = get_patch(from, to)))
		rtb_patchbay_free_patch(self, patch);
}

struct rtb_patchbay_patch *
rtb_patchbay_connect_ports(struct rtb_patchbay *self,
		struct rtb_patchbay_port *a, struct rtb_patchbay_port *b)
{
	struct rtb_patchbay_patch *patch;
	struct rtb_patchbay_port *from, *to;

	if (a->port_type == b->port_type)
		return NULL;

	if (a->port_type == PORT_TYPE_INPUT) {
		from = b;
		to   = a;
	} else {
		from = a;
		to   = b;
	}

	/* XXX: check this here or leave it up to the API client to ensure
	 *      no duplicate connections? */
	if ((patch = get_patch(from, to)))
		return patch;

	patch = calloc(1, sizeof(*patch));

	patch->from = from;
	patch->to   = to;

	TAILQ_INSERT_TAIL(&to->patches,   patch, to_patch);
	TAILQ_INSERT_TAIL(&from->patches, patch, from_patch);
	TAILQ_INSERT_TAIL(&self->patches, patch, patchbay_patch);

	rtb_elem_mark_dirty(RTB_ELEMENT(self));
	return patch;
}

int
rtb_patchbay_port_init(struct rtb_patchbay_port *self,
		struct rtb_patchbay_node *node, const rtb_utf8_t *name,
		rtb_patchbay_port_type_t type, rtb_child_add_loc_t location)
{
	rtb_elem_init_subclass(RTB_ELEMENT(self), &super);
	TAILQ_INIT(&self->patches);

	rtb_label_init(&self->label);
	rtb_label_set_text(&self->label, name);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->label),
			RTB_ADD_HEAD);

	self->port_type  = type;
	self->node       = node;

	self->attached  = attached;
	self->on_event  = on_event;
	self->size_cb   = rtb_size_hfill;
	self->layout_cb = rtb_layout_vpack_top;

	self->outer_pad.x = 8.f;
	self->outer_pad.y = 2.f;

	if (type == PORT_TYPE_INPUT) {
		self->align = self->label.align = RTB_ALIGN_LEFT;
		rtb_elem_add_child(&node->input_ports, RTB_ELEMENT(self), location);
	} else {
		self->align = self->label.align = RTB_ALIGN_RIGHT;
		rtb_elem_add_child(&node->output_ports, RTB_ELEMENT(self), location);
	}

	return 0;
}

void
rtb_patchbay_port_fini(struct rtb_patchbay_port *self)
{
	struct rtb_patchbay_patch *patch;

	if (self->port_type == PORT_TYPE_INPUT)
		rtb_elem_remove_child(&self->node->input_ports, RTB_ELEMENT(self));
	else
		rtb_elem_remove_child(&self->node->output_ports, RTB_ELEMENT(self));

	while ((patch = TAILQ_FIRST(&self->patches)))
		rtb_patchbay_free_patch(self->node->patchbay, patch);

	rtb_label_fini(&self->label);
	rtb_elem_fini(RTB_ELEMENT(self));
}
