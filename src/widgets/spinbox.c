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
#include "rutabaga/layout.h"
#include "rutabaga/event.h"
#include "rutabaga/mouse.h"
#include "rutabaga/platform.h"

#include "rutabaga/widgets/spinbox.h"

#include "rtb_private/util.h"

#define SELF_FROM(elem) \
	struct rtb_spinbox *self = RTB_ELEMENT_AS(elem, rtb_spinbox)

static struct rtb_element_implementation super;

/**
 * internal API hooks
 */

static void
set_value_hook(struct rtb_element *elem)
{
	SELF_FROM(elem);
	char buf[32];

	snprintf(buf, sizeof(buf), self->format_string, self->value);
	rtb_label_set_text(&self->value_label, buf);
}

static void
attached(struct rtb_element *elem,
		struct rtb_element *parent, struct rtb_window *window)
{
	SELF_FROM(elem);

	super.attached(elem, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.spinbox");

	set_value_hook(elem);
}

/**
 * public API
 */

int
rtb_spinbox_init(struct rtb_spinbox *self)
{
	if (RTB_SUBCLASS(RTB_VALUE_ELEMENT(self), rtb_value_element_init, &super))
		return -1;

	self->attached = attached;

	self->size_cb   = rtb_size_hfit_children;
	self->layout_cb = rtb_layout_hpack_center;

	self->set_value_hook = set_value_hook;

	self->format_string = "%.2f";

	rtb_label_init(&self->value_label);
	rtb_elem_add_child(RTB_ELEMENT(self), RTB_ELEMENT(&self->value_label),
			RTB_ADD_TAIL);

	self->value_label.align = RTB_ALIGN_CENTER | RTB_ALIGN_MIDDLE;
	return 0;
}

void
rtb_spinbox_fini(struct rtb_spinbox *self)
{
	rtb_label_fini(&self->value_label);
	rtb_elem_fini(RTB_ELEMENT(self));
}

struct rtb_spinbox *
rtb_spinbox_new()
{
	struct rtb_spinbox *self = calloc(1, sizeof(struct rtb_spinbox));
	rtb_spinbox_init(self);
	return self;
}

void
rtb_spinbox_free(struct rtb_spinbox *self)
{
	rtb_spinbox_fini(self);
	free(self);
}
