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

#include <wchar.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/render.h"
#include "rutabaga/surface.h"
#include "rutabaga/layout.h"
#include "rutabaga/window.h"

#include "rutabaga/widgets/patchbay.h"

#include "private/util.h"

#define SELF_FROM(obj) struct rtb_patchbay_node *self = RTB_PATCHBAY_NODE_T(obj)

#define LABEL_PADDING		15.f

static struct rtb_object_implementation super;

/**
 * drawing
 */

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static void cache_to_vbo(rtb_patchbay_node_t *self)
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
	rtb_render_use_style_bg(self, state);

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	super.draw_cb(obj, state);

	glUseProgram(0);
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

static int handle_drag(rtb_patchbay_node_t *self, const rtb_ev_drag_t *e)
{
	switch (e->button) {
	case RTB_MOUSE_BUTTON1:
		self->x += e->delta.x;
		self->y += e->delta.y;

		rtb_obj_trigger_recalc(self, self, RTB_DIRECTION_LEAFWARD);
		rtb_surface_invalidate(self->surface);
		return 1;

	default:
		return 0;
	}
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAGGING:
		return handle_drag(self, RTB_EV_DRAG_T(e));

	default:
		return super.event_cb(obj, e);
	}
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	self->patchbay = (rtb_patchbay_t *) parent;

	rtb_label_set_font(&self->name_label, &window->font_manager->fonts.big);

	super.realize_cb(self, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay.node");

	cache_to_vbo(self);
}

static void size(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	SELF_FROM(obj);
	struct rtb_size label_size;

	rtb_size_vfit_children(self, avail, want);

	self->name_label.size_cb(&self->name_label, avail, &label_size);
	want->w = fmax(want->w, label_size.w + (LABEL_PADDING * 2.f));
}

/**
 * public API
 */

void rtb_patchbay_node_set_name(rtb_patchbay_node_t *self,
		const wchar_t *name)
{
	rtb_label_set_text(&self->name_label, name);
}

void rtb_patchbay_node_init(rtb_patchbay_node_t *self)
{
	rtb_obj_init(self, &super);
	glGenBuffers(1, &self->vbo);

	self->realize_cb = realize;
	self->event_cb   = on_event;
	self->recalc_cb  = recalculate;
	self->draw_cb    = draw;
	self->size_cb    = size;
	self->layout_cb  = rtb_layout_vpack_top;

	self->min_size.w = 200.f;

	self->outer_pad.x = 0.f;
	self->inner_pad.y = 5.f;

	rtb_label_init(&self->name_label, &self->name_label);
	self->name_label.align = RTB_ALIGN_CENTER;

	/**
	 * content area
	 */

	rtb_obj_init(&self->container, &self->container);
	self->container.size_cb = rtb_size_hfill;
	self->container.layout_cb = rtb_layout_hdistribute;
	self->container.outer_pad.x =
		self->container.outer_pad.y =
		self->container.inner_pad.y = 0.f;

	self->container.inner_pad.x = 10.f;

	rtb_obj_init(&self->node_ui, &self->node_ui);

	rtb_obj_init(&self->input_ports, &self->input_ports);
	rtb_obj_init(&self->output_ports, &self->output_ports);

	self->input_ports.outer_pad.x =
		self->input_ports.outer_pad.y =
		self->output_ports.outer_pad.x =
		self->output_ports.outer_pad.y = 0.f;
	self->input_ports.inner_pad.x =
		self->input_ports.inner_pad.y =
		self->output_ports.inner_pad.x =
		self->output_ports.inner_pad.y = 5.f;

	self->input_ports.size_cb =
		self->output_ports.size_cb = rtb_size_vfit_children;

	self->input_ports.layout_cb =
		self->output_ports.layout_cb = rtb_layout_vpack_top;

	self->input_ports.align = RTB_ALIGN_TOP;
	self->output_ports.align = RTB_ALIGN_BOTTOM;

	rtb_obj_add_child(&self->container, &self->input_ports, RTB_ADD_TAIL);
	rtb_obj_add_child(&self->container, &self->node_ui, RTB_ADD_TAIL);
	rtb_obj_add_child(&self->container, &self->output_ports, RTB_ADD_TAIL);

	rtb_obj_add_child(self, &self->name_label, RTB_ADD_HEAD);
	rtb_obj_add_child(self, &self->container, RTB_ADD_TAIL);
}

void rtb_patchbay_node_fini(rtb_patchbay_node_t *self)
{
	glDeleteBuffers(1, &self->vbo);

	rtb_obj_remove_child(self->patchbay, self);

	rtb_label_fini(&self->name_label);

	rtb_obj_fini(&self->input_ports);
	rtb_obj_fini(&self->output_ports);
	rtb_obj_fini(&self->node_ui);
	rtb_obj_fini(&self->container);

	rtb_obj_fini(self);
}

rtb_patchbay_node_t *rtb_patchbay_node_new(rtb_patchbay_t *parent,
		const wchar_t *name)
{
	rtb_patchbay_node_t *self = calloc(1, sizeof(*self));
	rtb_patchbay_node_init(self);

	if (name)
		rtb_patchbay_node_set_name(self, name);

	return self;
}

void rtb_patchbay_node_free(rtb_patchbay_node_t *self)
{
	rtb_patchbay_node_fini(self);
	free(self);
}
