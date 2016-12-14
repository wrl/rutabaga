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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/event.h"

#include "rutabaga/widgets/value.h"

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_value_element *self = RTB_ELEMENT_AS(elem, rtb_value_element)

static struct rtb_element_implementation super;

/**
 * event dispatching
 */

static int
dispatch_value_change_event(struct rtb_value_element *self, int synthetic)
{
	struct rtb_value_event event = {
		.type   = RTB_VALUE_CHANGE,
		.source = synthetic ? RTB_EVENT_SYNTHETIC : RTB_EVENT_GENUINE,
		.value  = self->value
	};

	return rtb_elem_deliver_event(RTB_ELEMENT(self), RTB_EVENT(&event));
}

/**
 * element impl
 */

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);

	rtb__value_element_set_value_uncooked(self,
		(self->normalised_value == -1.f) ? self->origin : self->value, 1);

	if (self->normalised_value == -1.f)
		rtb__value_element_set_value_uncooked(self, self->origin, 1);
	else
		rtb__value_element_set_value_uncooked(self, self->value, 1);
}

/**
 * protected API
 */

void
rtb__value_element_set_normalised_value(struct rtb_value_element *self,
		float new_normalised_value, int synthetic)
{
	float new_value;

	new_normalised_value = fmin(fmax(new_normalised_value, 0.f), 1.f);
	self->normalised_value = new_normalised_value;

	new_value = self->min +
		(new_normalised_value * (self->max - self->min));

	if (self->granularity != 0.f) {
		new_value = floorf(new_value / self->granularity) * self->granularity;

		if (new_value == self->value)
			return;
	}

	self->value = new_value;

	if (self->set_value_hook)
		self->set_value_hook(RTB_ELEMENT(self));

	if (self->state != RTB_STATE_UNATTACHED)
		dispatch_value_change_event(self, synthetic);
}

void
rtb__value_element_set_value_uncooked(struct rtb_value_element *self,
		float new_value, int synthetic)
{
	float range = self->max - self->min;
	rtb__value_element_set_normalised_value(self,
			(new_value - self->min) / range, synthetic);
}


/**
 * public API
 */

void
rtb_value_element_set_value(struct rtb_value_element *self, float new_value)
{
	rtb__value_element_set_value_uncooked(self, new_value, 1);
}

int
rtb_value_element_init(struct rtb_value_element *self)
{
	if (RTB_SUBCLASS(RTB_ELEMENT(self), rtb_elem_init, &super))
		return -1;

	self->attached = attached;

	self->granularity  =
		self->value    =
		self->origin   = 0.f;

	/* -1.f is a special value indicating that no user/client code has
	 * not modified the value in-between creating the element and window
	 * initialisation. */
	self->normalised_value = -1.f;

	self->min = 0.f;
	self->max = 1.f;

	return 0;
}

void
rtb_value_element_fini(struct rtb_value_element *self)
{
	rtb_elem_fini(RTB_ELEMENT(self));
}
