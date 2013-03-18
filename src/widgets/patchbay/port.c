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

static struct rtb_object_implementation super;

#define BACKGROUND_NORMAL	RGB(0x404F3C)

#define RTB_PATCHBAY_PORT_T(x) ((rtb_patchbay_port_t *) x)
#define SELF_FROM(obj) struct rtb_patchbay_port *self = (struct rtb_patchbay_port *) obj

/**
 * private utility functions
 */

static rtb_patchbay_patch_t *get_patch(rtb_patchbay_port_t *from,
		rtb_patchbay_port_t *to)
{
	rtb_patchbay_patch_t *patch;

	TAILQ_FOREACH(patch, &from->patches, from_patch)
		if (patch->to == to)
			return patch;

	return NULL;
}

/**
 * drawing
 */

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static void cache_to_vbo(rtb_patchbay_port_t *self)
{
	GLfloat x, y, w, h, box[4][2];

	x = self->x;
	y = self->y;
	w = self->w;
	h = self->h;

	box[0][0] = x;
	box[0][1] = y;

	box[1][0] = x + w;
	box[1][1] = y;

	box[2][0] = x + w;
	box[2][1] = y + h;

	box[3][0] = x;
	box[3][1] = y + h;

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_render_push(self);
	rtb_render_set_position(self, 0.f, 0.f);

	rtb_render_set_color(self, BACKGROUND_NORMAL, .9f);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(0);

	super.draw_cb(obj, state);
}

/**
 * event dispatching
 */

static void dispatch_disconnect(rtb_patchbay_patch_t *patch)
{
	rtb_ev_patchbay_disconnect_t ev = {
		.type = RTB_PATCHBAY_DISCONNECT,

		.patch     = patch,

		.from.node = patch->from->node,
		.from.port = patch->from,
		.to.node   = patch->to->node,
		.to.port   = patch->to
	};

	rtb_dispatch_raw(patch->to, RTB_EV_T(&ev));
}

static void dispatch_connect(rtb_patchbay_port_t *from,
		rtb_patchbay_port_t *to)
{
	rtb_ev_patchbay_connect_t ev = {
		.type = RTB_PATCHBAY_CONNECT,

		.from.node = from->node,
		.from.port = from,
		.to.node   = to->node,
		.to.port   = to
	};

	rtb_dispatch_raw(to, RTB_EV_T(&ev));
}

/**
 * event handling
 */

static void handle_connection(rtb_patchbay_port_t *a, rtb_patchbay_port_t *b)
{
	rtb_patchbay_port_t *from, *to;
	rtb_patchbay_patch_t *patch;

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

static void start_patching(rtb_patchbay_port_t *self, rtb_ev_mouse_t *e)
{
	rtb_patchbay_t *patchbay = self->node->patchbay;

	patchbay->patch_in_progress.from = self;
	patchbay->patch_in_progress.to   = NULL;

	patchbay->patch_in_progress.cursor.x = e->cursor.x;
	patchbay->patch_in_progress.cursor.y = e->cursor.y;
}

static void stop_patching(rtb_patchbay_port_t *self)
{
	rtb_patchbay_t *patchbay = self->node->patchbay;

	patchbay->patch_in_progress.from = NULL;
	patchbay->patch_in_progress.to   = NULL;
}

static int handle_drag(rtb_patchbay_port_t *self, const rtb_ev_drag_t *e)
{
	rtb_patchbay_t *patchbay = self->node->patchbay;

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

			if (rtb_is_type(self->type, e->target))
				handle_connection(RTB_PATCHBAY_PORT_T(e->target), self);

			return 1;

		default:
			return 0;
		}

	case RTB_MOUSE_BUTTON2: /* right button -- disconnection */
	default:
		return 0;
	}
}

static int handle_mouse(rtb_patchbay_port_t *self, rtb_ev_mouse_t *e)
{
	if (RTB_EV_MOUSE_T(e)->button != RTB_MOUSE_BUTTON1)
		return 0;

	switch (e->type) {
	case RTB_MOUSE_DOWN:
		start_patching(self, RTB_EV_MOUSE_T(e));
		return 1;

	case RTB_MOUSE_UP:
		stop_patching(self);
		return 1;
	}

	return 0;
}

/**
 * object implementation
 */

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	SELF_FROM(obj);

	super.recalc_cb(obj, instigator, direction);
	cache_to_vbo(self);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	cache_to_vbo(self);
	super.realize_cb(self, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay.port");
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_MOUSE_DOWN:
	case RTB_MOUSE_UP:
		if (handle_mouse(self, RTB_EV_MOUSE_T(e))) {
			rtb_obj_mark_dirty(self->node->patchbay);
			return 1;
		}

		break;

	case RTB_DRAG_ENTER:
	case RTB_DRAG_LEAVE:
	case RTB_DRAG_START:
	case RTB_DRAG_DROP:
	case RTB_DRAGGING:
		if (handle_drag(self, RTB_EV_DRAG_T(e))) {
			rtb_obj_mark_dirty(self->node->patchbay);
			return 1;
		}
	}

	return super.event_cb(obj, e);
}

/**
 * public API
 */

int rtb_patchbay_are_ports_connected(rtb_patchbay_port_t *a,
		rtb_patchbay_port_t *b)
{
	rtb_patchbay_port_t *from, *to;

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

void rtb_patchbay_free_patch(rtb_patchbay_t *self,
		rtb_patchbay_patch_t *patch)
{
	TAILQ_REMOVE(&patch->to->patches,   patch, to_patch);
	TAILQ_REMOVE(&patch->from->patches, patch, from_patch);
	TAILQ_REMOVE(&self->patches, patch, patchbay_patch);

	free(patch);
	rtb_obj_mark_dirty(self);
}

void rtb_patchbay_disconnect_ports(rtb_patchbay_t *self,
		rtb_patchbay_port_t *a, rtb_patchbay_port_t *b)
{
	rtb_patchbay_patch_t *patch;
	rtb_patchbay_port_t *from, *to;

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

rtb_patchbay_patch_t *rtb_patchbay_connect_ports(rtb_patchbay_t *self,
		rtb_patchbay_port_t *a, rtb_patchbay_port_t *b)
{
	rtb_patchbay_patch_t *patch;
	rtb_patchbay_port_t *from, *to;

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

	rtb_obj_mark_dirty(self);
	return patch;
}

int rtb_patchbay_port_init(rtb_patchbay_port_t *self,
		rtb_patchbay_node_t *node, const wchar_t *name,
		rtb_patchbay_port_type_t type, rtb_child_add_loc_t location)
{
	rtb_obj_init(self, &super);
	glGenBuffers(1, &self->vbo);
	TAILQ_INIT(&self->patches);

	rtb_label_init(&self->label, &self->label);
	rtb_label_set_text(&self->label, name);
	rtb_obj_add_child(self, &self->label, RTB_ADD_HEAD);

	self->port_type  = type;
	self->node       = node;

	self->draw_cb    = draw;
	self->realize_cb = realize;
	self->recalc_cb  = recalculate;
	self->event_cb   = on_event;
	self->size_cb    = rtb_size_hfill;
	self->layout_cb  = rtb_layout_vpack_top;

	self->outer_pad.x = 8.f;
	self->outer_pad.y = 2.f;

	if (type == PORT_TYPE_INPUT) {
		self->align = self->label.align = RTB_ALIGN_LEFT;
		rtb_obj_add_child(&node->input_ports, self, location);
	} else {
		self->align = self->label.align = RTB_ALIGN_RIGHT;
		rtb_obj_add_child(&node->output_ports, self, location);
	}

	return 0;
}

void rtb_patchbay_port_fini(rtb_patchbay_port_t *self)
{
	rtb_patchbay_patch_t *patch;

	if (self->port_type == PORT_TYPE_INPUT)
		rtb_obj_remove_child(&self->node->input_ports, self);
	else
		rtb_obj_remove_child(&self->node->output_ports, self);

	while ((patch = TAILQ_FIRST(&self->patches)))
		rtb_patchbay_free_patch(self->node->patchbay, patch);

	glDeleteBuffers(1, &self->vbo);
	rtb_label_fini(&self->label);
	rtb_obj_fini(self);
}
