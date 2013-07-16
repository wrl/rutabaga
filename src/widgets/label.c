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
#include "rutabaga/object.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"

#include "rutabaga/widgets/label.h"

#define SELF_FROM(obj) \
	struct rtb_label *self = RTB_OBJECT_AS(obj, rtb_label)

static struct rtb_object_implementation super;

static void
draw(struct rtb_object *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_text_object_render(self->tobj, obj, self->x, self->y, state);
	super.draw_cb(obj, state);
}

static void
recalculate(struct rtb_object *obj, struct rtb_object *instigator,
		rtb_ev_direction_t direction)
{
	super.recalc_cb(obj, instigator, direction);
	obj->style = obj->parent->style;
}

static void
realize(struct rtb_object *obj, struct rtb_object *parent,
		struct rtb_window *window)
{
	SELF_FROM(obj);

	self->tobj = rtb_text_object_new(window->font_manager,
			self->font, self->text);

	self->outer_pad.x = self->tobj->xpad;
	self->outer_pad.y = self->tobj->ypad;

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.label");
}

static void
size(struct rtb_object *obj,
		const struct rtb_size *avail, struct rtb_size *want)
{
	SELF_FROM(obj);

	if (!self->tobj) {
		want->w = 0.f;
		want->h = 0.f;
	} else {
		want->w = ceilf(self->tobj->w);
		want->h = ceilf(self->tobj->h);
	}
}

/**
 * public
 */

void
rtb_label_set_font(struct rtb_label *self, struct rtb_font *font)
{
	self->font = font;

	if (!self->tobj)
		return;

	self->tobj->font = font;
	rtb_text_object_update(self->tobj, self->text);
	rtb_obj_trigger_recalc(self->parent, RTB_OBJECT(self),
			RTB_DIRECTION_ROOTWARD);
}

void
rtb_label_set_text(struct rtb_label *self, const rtb_utf8_t *text)
{
	if (self->text)
		free(self->text);

	self->text = strdup(text);

	if (!self->tobj)
		return;

	rtb_text_object_update(self->tobj, self->text);
	rtb_obj_trigger_recalc(self->parent, RTB_OBJECT(self),
			RTB_DIRECTION_ROOTWARD);
}

int
rtb_label_init(struct rtb_label *self,
		struct rtb_object_implementation *impl)
{
	rtb_obj_init(RTB_OBJECT(self), &super);

	/* XXX: nasty */

	(*impl) = super;
	impl->draw_cb    = draw;
	impl->realize_cb = realize;
	impl->size_cb    = size;

	if (impl != (void *) self) {
		self->draw_cb    = draw;
		self->realize_cb = realize;
		self->size_cb    = size;
		self->recalc_cb  = recalculate;
	}

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

	rtb_obj_fini(RTB_OBJECT(self));
}

struct rtb_label *
rtb_label_new(const rtb_utf8_t *text)
{
	struct rtb_label *self = calloc(1, sizeof(*self));
	rtb_label_init(self, &self->impl);

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
