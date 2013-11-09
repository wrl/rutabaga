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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/style.h"

#include "rutabaga/widgets/label.h"

#define SELF_FROM(elem) \
	struct rtb_label *self = RTB_ELEMENT_AS(elem, rtb_label)

static struct rtb_element_implementation super;

static void
draw(struct rtb_element *elem)
{
	SELF_FROM(elem);
	struct rtb_render_context *ctx = rtb_render_get_context(elem);

	rtb_text_object_render(self->tobj, ctx, self->x, self->y, self->color);
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.label");

	self->tobj = rtb_text_object_new(&window->font_manager);
}

static void
size(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	SELF_FROM(elem);

	if (!self->tobj) {
		want->w = 0.f;
		want->h = 0.f;
	} else {
		want->w = ceilf(self->tobj->w);
		want->h = ceilf(self->tobj->h);
	}
}

static void
restyle(struct rtb_element *elem)
{
	const struct rtb_style_property_definition *prop;
	SELF_FROM(elem);

	super.restyle(elem);

	prop = rtb_style_query_prop_in_tree(self->parent,
			"font", RTB_STYLE_PROP_FONT, 0);

	assert(prop);

	if (&prop->font.font_internal != self->font) {
		/* XXX: const issues */
		self->font = (struct rtb_font *) &prop->font.font_internal;

		rtb_text_object_update(self->tobj, self->font, self->text);
		rtb_elem_trigger_recalc(self->parent, RTB_ELEMENT(self),
				RTB_DIRECTION_ROOTWARD);
	}

	prop = rtb_style_query_prop_in_tree(self->parent,
			"color", RTB_STYLE_PROP_COLOR, 1);
	self->color = &prop->color;
}

/**
 * public
 */

void
rtb_label_set_text(struct rtb_label *self, const rtb_utf8_t *text)
{
	struct rtb_size old_size;

	if (self->text)
		free(self->text);

	self->text = strdup(text);

	if (!self->tobj)
		return;

	old_size.w = self->tobj->w;
	old_size.h = self->tobj->h;

	rtb_text_object_update(self->tobj, self->font, self->text);

	if (self->tobj->w != old_size.w || self->tobj->h != old_size.h)
		rtb_elem_trigger_recalc(self->parent, RTB_ELEMENT(self),
				RTB_DIRECTION_ROOTWARD);
}

int
rtb_label_init(struct rtb_label *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	self->impl = super;
	self->impl.draw     = draw;
	self->impl.attached = attached;
	self->impl.size_cb  = size;
	self->impl.restyle  = restyle;

	self->text = NULL;
	self->tobj = NULL;
	self->font = NULL;

	return 0;
}

void
rtb_label_fini(struct rtb_label *self)
{
	if (self->text)
		free(self->text);

	if (self->tobj)
		rtb_text_object_free(self->tobj);

	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_label *
rtb_label_new(const rtb_utf8_t *text)
{
	struct rtb_label *self = calloc(1, sizeof(*self));
	rtb_label_init(self);

	if (text)
		self->text = strdup(text);

	return self;
}

void
rtb_label_free(struct rtb_label *self)
{
	rtb_label_fini(self);
	free(self);
}
