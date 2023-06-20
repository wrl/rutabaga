/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2018 William Light.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>
#include <rutabaga/window.h>
#include <rutabaga/mouse.h>
#include <rutabaga/render.h>
#include <rutabaga/style.h>
#include <rutabaga/keyboard.h>
#include <rutabaga/platform.h>

#include <rutabaga/widgets/knob.h>

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_knob *self = RTB_ELEMENT_AS(elem, rtb_knob)

#define MIN_DEGREES 35.f
#define MAX_DEGREES (360.f - MIN_DEGREES)
#define DEGREE_RANGE (MAX_DEGREES - MIN_DEGREES)

static struct rtb_element_implementation super;

/**
 * drawing
 */

static void
draw(struct rtb_element *elem)
{
	SELF_FROM(elem);

	super.draw(elem);
	rtb_stylequad_draw_with_modelview(&self->rotor, elem, &self->modelview,
			RTB_STYLEQUAD_DRAW_ALL);
}

/**
 * event handling
 */

static void
set_value_hook(struct rtb_element *elem, const struct rtb_event *derive_from)
{
	SELF_FROM(elem);

	mat4_set_rotation(&self->modelview,
			MIN_DEGREES + (self->normalised_value * DEGREE_RANGE),
			0.f, 0.f, 1.f);

	rtb_elem_mark_dirty(RTB_ELEMENT(self));
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.knob");

	set_value_hook(elem, NULL);
}

static void
restyle(struct rtb_element *elem)
{
	SELF_FROM(elem);

	const struct rtb_style_property_definition *prop;
	super.restyle(elem);

	prop = rtb_style_query_prop(elem,
			"-rtb-knob-rotor", RTB_STYLE_PROP_TEXTURE, 0);
	if (prop &&
			!rtb_stylequad_set_background_image(&self->rotor, &prop->texture))
		rtb_elem_mark_dirty(elem);
}

static int
reflow(struct rtb_element *elem,
		struct rtb_element *instigator, rtb_ev_direction_t direction)
{
	SELF_FROM(elem);

	if (!super.reflow(elem, instigator, direction))
		return 0;

	rtb_stylequad_update_geometry(&self->rotor, &self->rect);
	return 1;
}

/**
 * public API
 */

int
rtb_knob_init(struct rtb_knob *self)
{
	if (RTB_SUBCLASS(RTB_VALUE_ELEMENT(self), rtb_value_element_init, &super))
		return -1;

	self->draw     = draw;
	self->attached = attached;
	self->restyle  = restyle;
	self->reflow   = reflow;

	self->set_value_hook = set_value_hook;

	rtb_stylequad_init(&self->rotor);

	return 0;
}

void
rtb_knob_fini(struct rtb_knob *self)
{
	rtb_stylequad_fini(&self->rotor);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_knob *
rtb_knob_new()
{
	struct rtb_knob *self = calloc(1, sizeof(struct rtb_knob));
	rtb_knob_init(self);
	return self;
}

void
rtb_knob_free(struct rtb_knob *self)
{
	rtb_knob_fini(self);
	free(self);
}
