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
#include <wchar.h>
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/object.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"

#include "rutabaga/widgets/label.h"

#define SELF_FROM(obj) struct rtb_label *self = RTB_LABEL_T(obj)

static struct rtb_object_implementation super;

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_text_object_render(self->tobj, self, self->x, self->y);
	super.draw_cb(obj, state);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
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

static void size(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
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

void rtb_label_set_font(rtb_label_t *self, rtb_font_t *font)
{
	self->font = font;

	if (self->tobj) {
		self->tobj->font = font;
		rtb_text_object_update(self->tobj, self->text);
		rtb_obj_trigger_recalc(self->parent, self, RTB_DIRECTION_ROOTWARD);
	}
}

void rtb_label_set_text(rtb_label_t *self, const wchar_t *text)
{
	if (self->text)
		free(self->text);

	self->text = wcsdup(text);

	if (self->tobj) {
		rtb_text_object_update(self->tobj, text);
		rtb_obj_trigger_recalc(self->parent, self, RTB_DIRECTION_ROOTWARD);
	}
}

int rtb_label_init(rtb_label_t *self,
		struct rtb_object_implementation *impl)
{
	rtb_obj_init(self, &super);

	/* XXX: nasty */

	(*impl) = super;
	impl->draw_cb    = draw;
	impl->realize_cb = realize;
	impl->size_cb    = size;

	if (impl != (void *) self) {
		self->draw_cb    = draw;
		self->realize_cb = realize;
		self->size_cb    = size;
	}

	self->text = NULL;
	self->tobj = NULL;
	self->font = NULL;

	return 0;
}

void rtb_label_fini(rtb_label_t *self)
{
	if (self->text)
		free(self->text);

	if (self->tobj)
		rtb_text_object_free(self->tobj);

	rtb_obj_fini(self);
}

rtb_label_t *rtb_label_new(const wchar_t *text)
{
	rtb_label_t *self = calloc(1, sizeof(*self));
	rtb_label_init(self, self);

	if (text)
		self->text = wcsdup(text);

	return self;
}

void rtb_label_free(rtb_label_t *self)
{
	rtb_label_fini(self);
	free(self);
}
