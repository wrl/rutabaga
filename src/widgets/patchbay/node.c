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
#include "rutabaga/render.h"
#include "rutabaga/surface.h"
#include "rutabaga/layout.h"
#include "rutabaga/window.h"

#include "rutabaga/widgets/patchbay.h"

#include "private/util.h"

#define SELF_FROM(elem) \
	struct rtb_patchbay_node *self = RTB_ELEMENT_AS(elem, rtb_patchbay_node)

#define LABEL_PADDING		15.f

static struct rtb_element_implementation super;

/**
 * element implementation
 */

static int
handle_drag(struct rtb_patchbay_node *self, const struct rtb_drag_event *e)
{
	struct rtb_element *elem = RTB_ELEMENT(self);

	switch (e->button) {
	case RTB_MOUSE_BUTTON1:
		self->x += e->delta.x;
		self->y += e->delta.y;

		rtb_elem_trigger_reflow(elem, elem, RTB_DIRECTION_LEAFWARD);
		rtb_surface_invalidate(self->surface);
		return 1;

	default:
		return 0;
	}
}

static int
on_event(struct rtb_element *elem, const struct rtb_event *e)
{
	SELF_FROM(elem);

	switch (e->type) {
	case RTB_DRAG_START:
	case RTB_DRAGGING:
		return handle_drag(self, RTB_EVENT_AS(e, rtb_drag_event));

	default:
		return super.on_event(elem, e);
	}
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	self->patchbay = (struct rtb_patchbay *) parent;

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.patchbay.node");
}

static void
size(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	SELF_FROM(elem);
	struct rtb_size label_size;

	rtb_size_vfit_children(elem, avail, want);

	self->name_label.size_cb(RTB_ELEMENT(&self->name_label),
			avail, &label_size);
	want->w = fmax(want->w, label_size.w + (LABEL_PADDING * 2.f));
}

/**
 * public API
 */

void
rtb_patchbay_node_set_name(struct rtb_patchbay_node *self,
		const rtb_utf8_t *name)
{
	rtb_label_set_text(&self->name_label, name);
}

int
rtb_patchbay_node_init(struct rtb_patchbay_node *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	self->on_event  = on_event;
	self->attached  = attached;
	self->size_cb   = size;
	self->layout_cb = rtb_layout_vpack_top;

	self->outer_pad.x = 0.f;
	self->inner_pad.y = 5.f;

	rtb_label_init(&self->name_label);
	self->name_label.align = RTB_ALIGN_CENTER;

	/**
	 * content area
	 */

	rtb_elem_init(&self->container);
	self->container.size_cb = rtb_size_hfill;
	self->container.layout_cb = rtb_layout_hdistribute;
	self->container.outer_pad.x =
		self->container.outer_pad.y =
		self->container.inner_pad.y = 0.f;

	self->container.inner_pad.x = 10.f;

	rtb_elem_init(&self->node_ui);

	rtb_elem_init(&self->input_ports);
	rtb_elem_init(&self->output_ports);

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

	rtb_elem_add_child(&self->container, &self->input_ports, RTB_ADD_TAIL);
	rtb_elem_add_child(&self->container, &self->node_ui, RTB_ADD_TAIL);
	rtb_elem_add_child(&self->container, &self->output_ports, RTB_ADD_TAIL);

	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->name_label),
			RTB_ADD_HEAD);
	rtb_elem_add_child(RTB_ELEMENT(self), &self->container, RTB_ADD_TAIL);

	return 0;
}

void
rtb_patchbay_node_fini(struct rtb_patchbay_node *self)
{
	rtb_elem_remove_child(RTB_ELEMENT(self->patchbay), RTB_ELEMENT(self));
	rtb_label_fini(&self->name_label);

	rtb_elem_fini(&self->input_ports);
	rtb_elem_fini(&self->output_ports);
	rtb_elem_fini(&self->node_ui);
	rtb_elem_fini(&self->container);

	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_patchbay_node *
rtb_patchbay_node_new(struct rtb_patchbay *parent, const rtb_utf8_t *name)
{
	struct rtb_patchbay_node *self = calloc(1, sizeof(*self));
	rtb_patchbay_node_init(self);

	if (name)
		rtb_patchbay_node_set_name(self, name);

	return self;
}

void
rtb_patchbay_node_free(struct rtb_patchbay_node *self)
{
	rtb_patchbay_node_fini(self);
	free(self);
}
